//#include "Player.h"
//#ifdef __cpluscplus
//extern "C" {
//#endif

#include "LogUtils.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/log.h"
#include "libavutil/imgutils.h"
#include "SDL.h"
#include "audiochunk.h"
#include <pthread.h>
#include <libavutil/audio_fifo.h>
#include <libswresample/swresample.h>

#define SDL_AUDIO_MIN_BUFFER_SIZE 512

#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30

/* no AV sync correction is done if below the minimum AV sync threshold 秒*/
#define AV_SYNC_THRESHOLD_MIN 0.04 * 1000
/* AV sync correction is done if above the maximum AV sync threshold 秒 */
#define AV_SYNC_THRESHOLD_MAX 0.01 * 1000

#define SDL_AUDIO_BUFFER_SIZE 1024
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000

static pthread_cond_t cond;
static pthread_mutex_t mutex;

//static AVAudioFifo *pAvAudioFifo;

//Microsecond
unsigned long long lastPlayTime = 0;


typedef struct PacketQueue {
    AVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    SDL_mutex *mutex;
    SDL_cond *cond;
} PacketQueue;

PacketQueue audioq;
int quit = 0;
AudioChunkQueue *audioChunkQueue = NULL;

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


unsigned long long getTimeInMillisecond(){
    struct timespec curTime;
    clock_gettime(CLOCK_MONOTONIC,&curTime);
    return curTime.tv_sec * 1000 + curTime.tv_nsec/1000000;
}

void displayVideo(SDL_Renderer *pRender,SDL_Texture *pTexture,AVStream *stream,AVFrame *pFrameYUV,AVRational tb){
    double expectTime = 1000 / (stream->avg_frame_rate.num/stream->avg_frame_rate.den) + lastPlayTime;
    double curTime = getTimeInMillisecond();
    int waitResult;
    int waitMillsSeconds = expectTime - curTime;

    native_print_d("displayVideo enter lastPlayTime:%lld, expectTime:%f,curTime:%f av_q2d:%f,waitMillsSeconds:%d",lastPlayTime,expectTime,curTime,av_q2d(tb),waitMillsSeconds);
    //如果当前时间还未到播放时间，但时间误差大于AV_SYNC_THRESHOLD_MAX，需要等待
    if(expectTime - curTime > AV_SYNC_THRESHOLD_MAX && lastPlayTime > 0){
        struct timespec timeSpec;
        timeSpec.tv_sec = (int)(waitMillsSeconds / 1000);
        timeSpec.tv_nsec = (waitMillsSeconds % 1000)*1000000;
        if (timeSpec.tv_nsec > 1000000000) {
            timeSpec.tv_sec += 1;
            timeSpec.tv_nsec -= 1000000000;
        }
//        pthread_mutex_lock(&mutex);
//        waitResult = pthread_cond_timedwait(&cond,&mutex,&timeSpec);
//        pthread_mutex_unlock(&mutex);

        nanosleep(&timeSpec,NULL);
    }
    else{
        //如果当前时间还未到播放时间，但时间误差小于AV_SYNC_THRESHOLD_MAX，则直接播放
        //如果当前时间已经超过期望播放时间，则立刻播放
    }

    lastPlayTime = getTimeInMillisecond();
    native_print_d("displayVideo leave");
    SDL_SetRenderDrawColor(pRender, 0, 0, 0, 255);
    SDL_RenderClear(pRender);
    SDL_UpdateYUVTexture(pTexture,NULL,pFrameYUV->data[0],pFrameYUV->linesize[0],pFrameYUV->data[1],pFrameYUV->linesize[1],pFrameYUV->data[2],pFrameYUV->linesize[2]);
//    SDL_UpdateTexture( pTexture, NULL , pFrameYUV->data[0], pFrameYUV->linesize[0] );
    SDL_RenderCopy(pRender, pTexture, NULL, NULL);
    SDL_RenderPresent(pRender);
}

void SDLCALL myAudioCallback(void *userdata, Uint8 * stream,
                                            int streamLength){
    AudioChunk *chunk = NULL;
    int bytesFree = streamLength;

    while (bytesFree > 0) {
//        pthread_mutex_lock(&producerHasFinishedMutex);
//        if (sem_trywait(audioChunkQueue->full) != 0) {
//            if (producerHasFinished && !shouldBroadcast) {
//                shouldBroadcast = 1;
//                memset(stream, 0, bytesFree); // filling the remaining space of stream with 0s
//                pthread_mutex_unlock(&producerHasFinishedMutex);
//                break;
//            }
//        } else {
//            sem_post(audioChunkQueue->full);
//        }
//        pthread_mutex_unlock(&producerHasFinishedMutex);

//        getNextAudioChunk(&chunk, audioChunkQueue, bytesFree);
//        memcpy(stream, chunk->data, chunk->size);
//        bytesFree -= chunk->size;
//        stream += chunk->size;
//        free(chunk->data);
//        free(chunk);
    }
}


int playInner(const char *filePath) {
    int result = -1;

    //sdl struct
    struct SDL_Window *pWindow = NULL;
    struct SDL_Renderer *pRender = NULL;
    int videoWidth,videoHeight;
    SDL_RendererInfo rendererInfo;
    SDL_Texture *pTexture = NULL;
    SDL_AudioSpec desiredSpec;
    SDL_AudioSpec realSpec;
    SDL_AudioDeviceID audioDeviceId = 0;
    int data_size;

    //ffmpeg struct
    AVFormatContext *pFormatContext = NULL;
    AVCodecContext *pVideoCodecContext = NULL;
    const AVCodec *pVideoCodec = NULL;
    AVCodecContext *pAudioCodecContext = NULL;
    const AVCodec *pAudioCodec = NULL;

    int videoIndex = -1;
    int audioIndex = -1;
    AVFrame	*pFrame = NULL,*pFrameYUV = NULL;
    AVPacket* pPacket = NULL;

    struct SwsContext* pSwsContext = NULL;
    struct SwrContext *swrContext = swr_alloc();

    //other
    char info[1000]={0};
    int frame_cnt = 0;
    int ret = 0;
    int got_picture;
    long ySize;
    char *pAudioOutBuffer = NULL;
    unsigned char * pVideoOutBuffer = NULL;

    //params init
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    packet_queue_init(&audioq);

    //init ffmpeg
//    av_log_set_callback(custom_log);

    av_register_all();
    avformat_network_init();
    pFormatContext = avformat_alloc_context();

    if(avformat_open_input(&pFormatContext,filePath,NULL,NULL) != 0){
        native_write_e("avformat_open_input error");
        goto EXIT0;
    }

    if(avformat_find_stream_info(pFormatContext,NULL) != 0){
        native_write_e("avformat_find_stream_info error");
        goto EXIT0;
    }


    for(int i=0; i<pFormatContext->nb_streams; i++){
        if(pFormatContext->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
            videoIndex = i;
        }
        else if(pFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
            audioIndex = i;
        }
    }
    if(videoIndex == -1){
        native_write_e("Couldn't find a video stream.\n");
        goto EXIT0;
    }
    if(audioIndex == -1){
        native_write_e("Couldn't find a audio stream.\n");
        goto EXIT0;
    }

    pVideoCodecContext = pFormatContext->streams[videoIndex]->codec;
    pVideoCodec = pVideoCodecContext->codec;
    if(pVideoCodec == NULL){
        pVideoCodec = avcodec_find_decoder(pVideoCodecContext->codec_id);
    }

    if(pVideoCodec == NULL){
        native_write_e("avcodec_find_decoder error");
        goto EXIT0;
    }


    if(avcodec_open2(pVideoCodecContext, pVideoCodec, NULL) != 0){
        native_write_e("avcodec_open2 error");
        goto EXIT0;
    }

    pFrame = av_frame_alloc();
    pFrameYUV = av_frame_alloc();
    pVideoOutBuffer = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pVideoCodecContext->width, pVideoCodecContext->height, 1));
    av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, pVideoOutBuffer,
                         AV_PIX_FMT_YUV420P, pVideoCodecContext->width, pVideoCodecContext->height, 1);

    pPacket = (AVPacket *)av_malloc(sizeof(AVPacket));

    pSwsContext = sws_getContext(pVideoCodecContext->width, pVideoCodecContext->height, pVideoCodecContext->pix_fmt,
                                 pVideoCodecContext->width, pVideoCodecContext->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);


    //audio
    pAudioCodecContext = pFormatContext->streams[audioIndex]->codec;
    pAudioCodec = avcodec_find_decoder(pAudioCodecContext->codec_id);
    if (!pAudioCodec) {
        native_print_e("Unsupported codec!\n");
        return -1;
    }
    avcodec_open2(pAudioCodecContext, pAudioCodec,NULL);

//    pAudioCodecContext = avcodec_alloc_context3(NULL);
//    if (!pAudioCodecContext){
//        native_write_d("avcodec_alloc_context3 fail");
//        goto EXIT0;
//    }
//    ret = avcodec_parameters_to_context(pAudioCodecContext, pFormatContext->streams[audioIndex]->codecpar);
//    if(ret < 0){
//        native_print_d("avcodec_parameters_to_context fail %d:" ,ret);
//        goto EXIT0;
//    }


    sprintf(info,   "\n[Input     ]%s\n", filePath);
    sprintf(info, "%s[Format    ]%s\n",info, pFormatContext->iformat->name);
    sprintf(info, "%s[Codec     ]%s\n", info, pVideoCodecContext->codec->name);
    sprintf(info, "%s[Resolution]%dx%d\n", info, pVideoCodecContext->width, pVideoCodecContext->height);

    sprintf(info, "pCodecContext %s[timebase]num:%d/den:%d\n", info, pVideoCodecContext->time_base.num, pVideoCodecContext->time_base.den);
    sprintf(info, "pFormatContext %s[timebase]num:%d/den:%d\n", info, pFormatContext->streams[videoIndex]->time_base.num, pFormatContext->streams[videoIndex]->time_base.den);
    native_write_d(info);

    //init sdl
    /* This interface could expand with ABI negotiation, calbacks, etc. */
//    SDL_Android_Init(env, thizClass);
//    SDL_SetMainReady();

    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER) != 0){
//        native_print_e("SDL_Init error: %s",SDL_GetError());
        goto EXIT0;
    }

    videoWidth = pVideoCodecContext->width;
    videoHeight = pVideoCodecContext->height;
    pWindow = SDL_CreateWindow("player",0,0,videoWidth,videoHeight,SDL_WINDOW_SHOWN);
    if(pWindow == NULL){
        native_write_e("SDL_CreateWindow error");
        goto EXIT0;
    }

    pRender = SDL_CreateRenderer(pWindow, -1,SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if(pRender == NULL){
        native_write_e("SDL_CreateRenderer error");
        goto EXIT0;
    }

    SDL_GetRendererInfo(pRender, &rendererInfo);
    native_print_d("Using %s rendering\n", rendererInfo.name);

    pTexture = SDL_CreateTexture(pRender, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, pVideoCodecContext->width, pVideoCodecContext->height);
    if (!pTexture) {
        native_print_e( "Couldn't set create texture: %s\n", SDL_GetError());
        goto EXIT0;
    }


    //audio
    memset(&desiredSpec,0,sizeof(desiredSpec));
//    desiredSpec.channels = 1;
    desiredSpec.channels = pAudioCodecContext->channels;
    desiredSpec.format = AUDIO_S16SYS;
    desiredSpec.samples = pAudioCodecContext->frame_size;
    desiredSpec.freq = pAudioCodecContext->sample_rate;
    if (desiredSpec.freq <= 0 || desiredSpec.channels <= 0) {
        native_write_e("Invalid sample rate or channel count!\n");
        goto EXIT0;
    }

    desiredSpec.silence = 0;
//    desiredSpec.callback = myAudioCallback;
    desiredSpec.userdata = pAudioCodecContext;

    data_size = av_get_bytes_per_sample(pAudioCodecContext->sample_fmt);

    audioDeviceId = SDL_OpenAudioDevice(NULL,0,&desiredSpec,&realSpec,SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
    if(audioDeviceId == 0){
        native_print_e( "SDL_OpenAudioDevice error: %s\n", SDL_GetError());
        goto EXIT0;
    }

    native_print_d("audio info data_size:%d \n pAudioCodecContext channels:%d,format:%d,samples:%d,freq:%d \n desiredSpec channels:%d,format:%d,samples:%d,freq:%d \n realSpec channels:%d,format:%d,samples:%d,freq:%d"
            ,data_size
            ,pAudioCodecContext->channels,pAudioCodecContext->sample_fmt,pAudioCodecContext->frame_size,pAudioCodecContext->sample_rate
            ,desiredSpec.channels,desiredSpec.format,desiredSpec.samples,desiredSpec.freq
            ,realSpec.channels,realSpec.format,realSpec.samples,realSpec.freq
            );

    swrContext = swr_alloc_set_opts(swrContext,
                                    pAudioCodecContext->channel_layout, AV_SAMPLE_FMT_S16, pAudioCodecContext->sample_rate,
                       pAudioCodecContext->channel_layout, pAudioCodecContext->sample_fmt, pAudioCodecContext->sample_rate,
            0, NULL);

    if(swrContext == NULL){
        native_write_e("swr_alloc_set_opts error");
        goto EXIT0;
    }

    // 初始化
    swr_init(swrContext);

//    pAvAudioFifo = av_audio_fifo_alloc(realSpec.format,realSpec.channels,realSpec.samples);
    SDL_PauseAudioDevice(audioDeviceId,0);

//    pAudioOutBuffer = av_malloc(pAudioCodecContext->channels * pAudioCodecContext->frame_size * sizeof(short));

//    playStartTime = clock()/CLOCKS_PER_SEC;
    while(av_read_frame(pFormatContext, pPacket)>=0){
        if(pPacket->stream_index == videoIndex){
            ret = avcodec_decode_video2(pVideoCodecContext, pFrame, &got_picture, pPacket);
            if(ret < 0){
                native_write_e("Decode Error.\n");
                return -1;
            }
            if(got_picture){
                sws_scale(pSwsContext, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pVideoCodecContext->height,
                          pFrameYUV->data, pFrameYUV->linesize);

                ySize = pVideoCodecContext->width * pVideoCodecContext->height;
//                fwrite(pFrameYUV->data[0],1,ySize,fp_yuv);    //Y
//                fwrite(pFrameYUV->data[1],1,ySize/4,fp_yuv);  //U
//                fwrite(pFrameYUV->data[2],1,ySize/4,fp_yuv);  //V
                //Output info
                const int pictype_buffer_size = 100;
                char pictype_str[100]={0};
                switch(pFrame->pict_type){
                    case AV_PICTURE_TYPE_I:sprintf(pictype_str,"I");break;
                    case AV_PICTURE_TYPE_P:sprintf(pictype_str,"P");break;
                    case AV_PICTURE_TYPE_B:sprintf(pictype_str,"B");break;
                    default:snprintf(pictype_str,pictype_buffer_size,"Other %d",pFrame->pict_type);break;
                }
                native_print_d("av_read_frame Frame Index: %5d. Type:%s, pts:%lld,duration:%lld",frame_cnt,pictype_str,pFrame->pts,pFrame->pkt_duration);
                frame_cnt++;
                displayVideo(pRender, pTexture, pFormatContext->streams[videoIndex], pFrameYUV, pVideoCodecContext->time_base);
            }
            else{
                native_print_d("av_read_frame Frame skip Index: %5d.",frame_cnt);
            }
        }
        else if(pPacket->stream_index == audioIndex){
            result = avcodec_decode_audio4(pAudioCodecContext, pFrame, &got_picture, pPacket);

            if (got_picture) {
                int size  = av_samples_get_buffer_size(NULL, pAudioCodecContext->channels, pFrame->nb_samples, AV_SAMPLE_FMT_S16, 1);
                pAudioOutBuffer = malloc(size);
                result = swr_convert(swrContext, (uint8_t **)&pAudioOutBuffer, size, (const uint8_t **) pFrame->data, pFrame->nb_samples);
                result = SDL_QueueAudio(audioDeviceId,pAudioOutBuffer,size);
                free(pAudioOutBuffer);
            }
        }
        av_free_packet(pPacket);
    }
    //flush decoder
    //FIX: Flush Frames remained in Codec
    while (1) {
        ret = avcodec_decode_video2(pVideoCodecContext, pFrame, &got_picture, pPacket);
        if (ret < 0)
            break;
        if (!got_picture)
            break;
//        sws_scale(pSwsContext, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pVideoCodecContext->height,
//                  pFrameYUV->data, pFrameYUV->linesize);
//        int y_size= pVideoCodecContext->width * pVideoCodecContext->height;
//        fwrite(pFrameYUV->data[0],1,y_size,fp_yuv);    //Y
//        fwrite(pFrameYUV->data[1],1,y_size/4,fp_yuv);  //U
//        fwrite(pFrameYUV->data[2],1,y_size/4,fp_yuv);  //V
        //Output info
        char pictype_str[10]={0};
        switch(pFrame->pict_type){
            case AV_PICTURE_TYPE_I:sprintf(pictype_str,"I");break;
            case AV_PICTURE_TYPE_P:sprintf(pictype_str,"P");break;
            case AV_PICTURE_TYPE_B:sprintf(pictype_str,"B");break;
            default:sprintf(pictype_str,"Other");break;
        }
        native_print_d("av_read_frame Frame Index: %5d. Type:%s, pts:%lld,duration:%lld",frame_cnt,pictype_str,pFrame->pts,pFrame->pkt_duration);
        frame_cnt++;
//        displayVideo(pRender, pTexture, pFormatContext->streams[videoIndex], pFrameYUV, pFormatContext->streams[videoIndex]->time_base);
    }


    EXIT0:
    if(pVideoOutBuffer != NULL)av_free(pVideoOutBuffer);

    if(pFrameYUV != NULL){av_frame_free(&pFrameYUV);}
    if(pFrame != NULL){av_frame_free(&pFrame);}
    if(pPacket != NULL){av_packet_free(&pPacket);}
    if(pFormatContext != NULL){avformat_close_input(&pFormatContext);}

    sws_freeContext(pSwsContext);
    avcodec_close(pVideoCodecContext);

    if(pRender != NULL){SDL_DestroyRenderer(pRender);}
    if(pWindow != NULL ){SDL_DestroyWindow(pWindow);}

//    if(pAudioOutBuffer != NULL){av_free(pAudioOutBuffer);}
//    if(pAvAudioFifo != NULL){av_audio_fifo_free(pAvAudioFifo);}
    avcodec_close(pAudioCodecContext);
    if(audioDeviceId > 0){
        SDL_CloseAudioDevice(audioDeviceId);
    }


    SDL_Quit();
    return result;
}


int main(int argc, char *argv[]) {
    playInner("/storage/emulated/0/Android/data/com.jhf.test/sintel.mp4");
}


//#ifdef __cpluscplus
//}
//#endif