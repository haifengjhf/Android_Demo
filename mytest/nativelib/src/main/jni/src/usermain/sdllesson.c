#include <jni.h>
#include <android/native_window_jni.h>
#include "SDL.h"
#include "SDL_thread.h"
#include "SDL_events.h"
#include "LogUtils.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/pixfmt.h"
#include "libswscale/swscale.h"


#define SDL_AUDIO_BUFFER_SIZE 1024
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

typedef struct PacketQueue {
    AVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    SDL_mutex *mutex;
    SDL_cond *cond;
} PacketQueue;

PacketQueue audioq;

int quit = 0;

void packet_queue_init(PacketQueue *q) {
    memset(q, 0, sizeof(PacketQueue));
    q->mutex = SDL_CreateMutex();
    q->cond = SDL_CreateCond();
}

int packet_queue_put(PacketQueue *q, AVPacket *pkt) {

    AVPacketList *pkt1;
    if (av_dup_packet(pkt) < 0) {
        return -1;
    }
    pkt1 = av_malloc(sizeof(AVPacketList));
    if (!pkt1)
        return -1;
    pkt1->pkt = *pkt;
    pkt1->next = NULL;

    SDL_LockMutex(q->mutex);

    if (!q->last_pkt)
        q->first_pkt = pkt1;
    else
        q->last_pkt->next = pkt1;
    q->last_pkt = pkt1;
    q->nb_packets++;
    q->size += pkt1->pkt.size;
    SDL_CondSignal(q->cond);

    SDL_UnlockMutex(q->mutex);
    return 0;
}

static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block) {
    AVPacketList *pkt1;
    int ret;

    SDL_LockMutex(q->mutex);

    for (;;) {

        if (quit) {
            ret = -1;
            break;
        }

        pkt1 = q->first_pkt;
        if (pkt1) {
            q->first_pkt = pkt1->next;
            if (!q->first_pkt)
                q->last_pkt = NULL;
            q->nb_packets--;
            q->size -= pkt1->pkt.size;
            *pkt = pkt1->pkt;
            av_free(pkt1);
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            SDL_CondWait(q->cond, q->mutex);
        }
    }
    SDL_UnlockMutex(q->mutex);
    return ret;
}

int audio_decode_frame(AVCodecContext *aCodecCtx, uint8_t *audio_buf,
                       int buf_size) {

    static AVPacket pkt;
    static uint8_t *audio_pkt_data = NULL;
    static int audio_pkt_size = 0;
    static AVFrame frame;

    int len1, data_size = 0;

    for (;;) {
        while (audio_pkt_size > 0) {
            int got_frame = 0;
            len1 = avcodec_decode_audio4(aCodecCtx, &frame, &got_frame, &pkt);
            if (len1 < 0) {
                /* if error, skip frame */
                audio_pkt_size = 0;
                break;
            }
            audio_pkt_data += len1;
            audio_pkt_size -= len1;
            if (got_frame) {
                data_size = frame.linesize[0];
                /*
                 data_size = av_samples_get_buffer_size(NULL,
                 aCodecCtx->channels, frame.nb_samples,
                 aCodecCtx->sample_fmt, 1);
                 */
                memcpy(audio_buf, frame.data[0], data_size);
            }
            if (data_size <= 0) {
                /* No data yet, get more frames */
                continue;
            }
            /* We have data, return it and come back for more later */
            return data_size;
        }
        if (pkt.data)
            av_free_packet(&pkt);

        if (quit) {
            return -1;
        }

        if (packet_queue_get(&audioq, &pkt, 1) < 0) {
            return -1;
        }
        audio_pkt_data = pkt.data;
        audio_pkt_size = pkt.size;
    }

    return 0;
}

void audio_callback(void *userdata, Uint8 *stream, int len) {

    AVCodecContext *aCodecCtx = (AVCodecContext *) userdata;
    int len1, audio_size;

    static uint8_t audio_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];
    static unsigned int audio_buf_size = 0;
    static unsigned int audio_buf_index = 0;

    while (len > 0) {
        if (audio_buf_index >= audio_buf_size) {
            /* We have already sent all our data; get more */
            audio_size = audio_decode_frame(aCodecCtx, audio_buf,
                                            sizeof(audio_buf));
            if (audio_size < 0) {
                /* If error, output silence */
                audio_buf_size = 1024; // arbitrary?
                memset(audio_buf, 0, audio_buf_size);
            } else {
                audio_buf_size = audio_size;
            }
            audio_buf_index = 0;
        }
        len1 = audio_buf_size - audio_buf_index;
        if (len1 > len)
            len1 = len;
        memcpy(stream, (uint8_t *) audio_buf + audio_buf_index, len1);
        len -= len1;
        stream += len1;
        audio_buf_index += len1;
    }
}

int main(int argc, char *argv[]) {
//    char *file_path = argv[1];
    char *file_path = "/storage/emulated/0/Android/data/com.jhf.test/sintel.mp4";
    native_print_d("file_path:%s", file_path);

    AVFormatContext *pFormatCtx;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVFrame *pFrame, *pFrameYUV;
    AVPacket *packet;
    uint8_t *out_buffer;

    AVCodecContext *aCodecCtx;
    AVCodec *aCodec;

    SDL_Texture *bmp = NULL;
    SDL_Window *screen = NULL;
    SDL_Rect rect;
    SDL_Event event;

    SDL_AudioSpec wanted_spec, spec;

    static struct SwsContext *img_convert_ctx;

    int videoStream, audioStream, i, numBytes;
    int ret, got_picture;

    av_register_all();
    pFormatCtx = avformat_alloc_context();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        native_print_e("Could not initialize SDL - %s. \n", SDL_GetError());
        exit(1);
    }

    if (avformat_open_input(&pFormatCtx, file_path, NULL, NULL) != 0) {
        native_print_e("can't open the file. \n");
        return -1;
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        native_print_e("Could't find stream infomation.\n");
        return -1;
    }

    videoStream = 1;
    audioStream = -1;
    for (i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
        }
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO
            && audioStream < 0) {
            audioStream = i;
        }
    }

    native_print_d("videoStream:%d", videoStream);
    if (videoStream == -1) {
        native_print_e("Didn't find a video stream.\n");
        return -1;
    }

    if (audioStream == -1) {
        native_print_e("Didn't find a audio stream.\n");
        return -1;
    }

    aCodecCtx = pFormatCtx->streams[audioStream]->codec;
    // Set audio settings from codec info
    wanted_spec.freq = aCodecCtx->sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = aCodecCtx->channels;
    wanted_spec.silence = 0;
    wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
    wanted_spec.callback = audio_callback;
    wanted_spec.userdata = aCodecCtx;

    if (SDL_OpenAudio(&wanted_spec, &spec) < 0) {
        native_print_e("SDL_OpenAudio: %s\n", SDL_GetError());
        return -1;
    }
    aCodec = avcodec_find_decoder(aCodecCtx->codec_id);
    if (!aCodec) {
        native_print_e("Unsupported codec!\n");
        return -1;
    }
    avcodec_open2(aCodecCtx, aCodec, NULL);

    // audio_st = pFormatCtx->streams[index]
    packet_queue_init(&audioq);
    SDL_PauseAudio(0);

    pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);

    if (pCodec == NULL) {
        native_print_e("Codec not found.\n");
        return -1;
    }

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        native_print_e("Could not open codec.\n");
        return -1;
    }

    pFrame = av_frame_alloc();
    pFrameYUV = av_frame_alloc();

    //---------------------------init sdl---------------------------//
    screen = SDL_CreateWindow("My Player Window", SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, pCodecCtx->width, pCodecCtx->height,
                              SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL);

    SDL_Renderer *renderer = SDL_CreateRenderer(screen, -1, 0);

    bmp = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12,
                            SDL_TEXTUREACCESS_STREAMING, pCodecCtx->width, pCodecCtx->height);

    //-------------------------------------------------------------//

    numBytes = avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width,
                                  pCodecCtx->height);
    out_buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
    avpicture_fill((AVPicture *) pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P,
                   pCodecCtx->width, pCodecCtx->height);

    rect.x = 0;
    rect.y = 0;
    rect.w = pCodecCtx->width;
    rect.h = pCodecCtx->height;

    int y_size = pCodecCtx->width * pCodecCtx->height;

    packet = (AVPacket *) malloc(sizeof(AVPacket));
    av_new_packet(packet, y_size);

    av_dump_format(pFormatCtx, 0, file_path, 0);

    while (av_read_frame(pFormatCtx, packet) >= 0) {
        if (packet->stream_index == videoStream) {
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture,
                                        packet);

            if (ret < 0) {
                native_print_e("decode error.\n");
                return -1;
            }

            native_print_d("got_picture:%d", got_picture);
            if (got_picture) {
                img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
                                                 pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
                                                 AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
                sws_scale(img_convert_ctx,
                          (uint8_t const * const *) pFrame->data,
                          pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data,
                          pFrameYUV->linesize);
                sws_freeContext(img_convert_ctx);
                ////iPitch 计算yuv一行数据占的字节数
                SDL_UpdateYUVTexture(bmp, &rect,
                                     pFrameYUV->data[0], pFrameYUV->linesize[0],
                                     pFrameYUV->data[1], pFrameYUV->linesize[1],
                                     pFrameYUV->data[2], pFrameYUV->linesize[2]);
                SDL_RenderClear(renderer);
                SDL_RenderCopy(renderer, bmp, &rect, &rect);
                SDL_RenderPresent(renderer);
            }
            av_free_packet(packet);
        } else if (packet->stream_index == audioStream) {
            packet_queue_put(&audioq, packet);
        } else {
            av_free_packet(packet);
        }

        SDL_PollEvent(&event);
        switch (event.type) {
            case SDL_QUIT:
                SDL_Quit();
                exit(0);
                break;
            default:
                break;
        }
    }
    SDL_DestroyTexture(bmp);

    av_free(out_buffer);
    av_free(pFrameYUV);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);

    return 0;
}