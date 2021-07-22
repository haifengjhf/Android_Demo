//
// Created by 金海峰 on 2021/7/5.
//


#include <unistd.h>
#include "PlayerEx.h"
#include "VideoScale.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "LogUtils.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

#include "libavutil/log.h"
#include "libavutil/imgutils.h"

#include "SDL.h"

const int MAX_PLAYER_NUMBER = 2;
PlayerEx* playerArray[MAX_PLAYER_NUMBER];

PlayerEx::PlayerEx(MyNativeWindow* myNativeWindow):mNextIVideo(false),mVideoSeekTimeInMilsecond(0),mAudioSeekTimeInMilsecond(0),mStop(false){
    mNativeWindow = myNativeWindow;

    packet_queue_init(&mVideoPacketQueue);
    packet_queue_init(&mAudioPacketQueue);
}

PlayerEx::~PlayerEx(){
    if(mNativeWindow){
        delete mNativeWindow;
        mNativeWindow = nullptr;
    }

    packet_queue_destroy(&mAudioPacketQueue);
    packet_queue_destroy(&mAudioPacketQueue);
}

int PlayerEx::seekInner(long timeInMilSecond){
    mAudioSeekTimeInMilsecond = timeInMilSecond;
    mVideoSeekTimeInMilsecond = timeInMilSecond;
    return 1;
}

int PlayerEx::playInner(const char *filePath) {
    int result = -1;

    //ffmpeg struct
    AVFormatContext *pFormatContext = nullptr;
    int videoindex(-1),audioIndex(-1);

    AVCodec *pVideoCodec(nullptr),*pAudioCodec(nullptr);
    AVPacket* pPacket = av_packet_alloc();

    //other
    char info[1000]={0};
    int packetCnt = 0;
    //init ffmpeg
//    av_log_set_callback(custom_log);

    av_register_all();
    avformat_network_init();
    pFormatContext = avformat_alloc_context();

    FILE *dst_file = fopen("/storage/emulated/0/Android/data/com.jhf.test/0.rgb", "wb");

    native_write_d("av init ready");

    if(avformat_open_input(&pFormatContext,filePath,NULL,NULL) != 0){
        native_write_e("avformat_open_input error");
        goto EXIT0;
    }
    native_write_d("avformat_open_input ready");

    if(avformat_find_stream_info(pFormatContext,NULL) != 0){
        native_write_e("avformat_find_stream_info error");
        goto EXIT0;
    }
    native_write_d("avformat_find_stream_info ready");


    for(int i=0; i<pFormatContext->nb_streams; i++){
        native_print_d("streams nb_streams: %d, i :%d, type: %d",pFormatContext->nb_streams,i,pFormatContext->streams[i]->codec->codec_type);
        if(pFormatContext->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
            videoindex = i;
        }
        else if(pFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
            audioIndex = i;
        }
    }
    if(videoindex==-1){
        native_write_e("Couldn't find a video stream.\n");
        goto EXIT0;
    }
    if(audioIndex==-1){
        native_write_e("Couldn't find a audio stream.\n");
        goto EXIT0;
    }

    pVideoCodecContext = pFormatContext->streams[videoindex]->codec;
    pVideoCodec = avcodec_find_decoder(pVideoCodecContext->codec_id);
    if(pVideoCodec == nullptr){
        native_write_e("avcodec_find_decoder error");
        goto EXIT0;
    }
    if(avcodec_open2(pVideoCodecContext,pVideoCodec,NULL) != 0){
        native_write_e("avcodec_open2 error");
        goto EXIT0;
    }

    pSwsContext = sws_getContext(pVideoCodecContext->width, pVideoCodecContext->height, pVideoCodecContext->pix_fmt,
                                 pVideoCodecContext->width,pVideoCodecContext->height, dstFormat, SWS_BICUBIC, NULL, NULL, NULL);

    native_write_d("codec video ready");

    pAudioCodecContext = pFormatContext->streams[audioIndex]->codec;
    pAudioCodec = avcodec_find_decoder(pAudioCodecContext->codec_id);
    if (!pAudioCodec) {
        native_print_e("Unsupported codec!\n");
        return -1;
    }
    if(avcodec_open2(pAudioCodecContext, pAudioCodec,NULL) != 0){
        native_write_e("avcodec_open2 audio error");
        goto EXIT0;
    }

    pSwrContext = swr_alloc();
    pSwrContext = swr_alloc_set_opts(pSwrContext
            ,channel_layout, outSampleFormat, DEFAULT_SAMPLE_RATE
            ,pAudioCodecContext->channel_layout, pAudioCodecContext->sample_fmt, pAudioCodecContext->sample_rate
            ,0, NULL);
    swr_init(pSwrContext);

    native_write_d("codec audio ready");


    sprintf(info,   "\n[Input     ]%s\n", filePath);
    sprintf(info, "%s[Format    ]%s\n",info, pFormatContext->iformat->name);
    sprintf(info, "%s[Codec     ]%s\n",info, pVideoCodecContext->codec->name);
    sprintf(info, "%s[Resolution]%dx%d\n",info, pVideoCodecContext->width,pVideoCodecContext->height);
    sprintf(info, "%s[timebase]num:%d/den:%d\n",info, pFormatContext->streams[videoindex]->time_base.num,pFormatContext->streams[videoindex]->time_base.den);
    native_write_d(info);

    //start audio ,video thread
    pthread_t audioThread,videoThread;
    pthread_create(&audioThread,NULL,PlayerEx::audio_thread,this);
    pthread_create(&videoThread,NULL,PlayerEx::video_thread,this);

    packet_queue_start(&mAudioPacketQueue);
    packet_queue_start(&mVideoPacketQueue);


    while(av_read_frame(pFormatContext, pPacket)>=0){
        JLOGD("av_read_frame %d,stream_index:%d",++packetCnt,pPacket->stream_index);
        if(pPacket->stream_index==videoindex){
            if(mVideoSeekTimeInMilsecond > 0){
                double timeBase = av_q2d(pFormatContext->streams[videoindex]->time_base) * 1000;
                double startTime = pPacket->dts * timeBase;
                double endTime = (pPacket->dts + pPacket->duration) * timeBase;
                if(startTime <= mVideoSeekTimeInMilsecond && mVideoSeekTimeInMilsecond <= endTime){
                    mVideoSeekTimeInMilsecond = 0;
                    mNextIVideo = true;
                }
                else{
                    goto ContinueFlag;
                }
            }

            packet_queue_put(&mVideoPacketQueue,pPacket);
        }
        else if(pPacket->stream_index == audioIndex){
            if(mAudioSeekTimeInMilsecond > 0){
                double timeBase = av_q2d(pFormatContext->streams[audioIndex]->time_base) * 1000;
                double startTime = pPacket->dts * timeBase;
                double endTime = (pPacket->dts + pPacket->duration) * timeBase;
                if(startTime <= mAudioSeekTimeInMilsecond && mAudioSeekTimeInMilsecond <= endTime){
                    mAudioSeekTimeInMilsecond = 0;
                }
                else{
                    goto ContinueFlag;
                }
            }

            packet_queue_put(&mAudioPacketQueue,pPacket);
        }
        else{
ContinueFlag:
            av_free_packet(pPacket);
        }
    }

    JLOGD("play thread read file end wait thread exit");

    //等待音视频线程退出，然后释放资源
    pthread_join(audioThread,NULL);
    pthread_join(videoThread,NULL);

EXIT0:
    JLOGD("play thread exit");

    if(dst_file){
        fclose(dst_file);
    }

    if(pPacket != nullptr){
        av_packet_free(&pPacket);
    }

    if(pFormatContext != nullptr){
        avformat_close_input(&pFormatContext);
    }
    if(pVideoCodecContext){
        avcodec_close(pVideoCodecContext);
    }

    return result;
}

void* PlayerEx::video_thread(void *arg) {
    PlayerEx* playerEx = (PlayerEx*)arg;
    int ret ;
    int got_picture;
    AVFrame* pFrame = av_frame_alloc();
    AVPacket* pPacket = av_packet_alloc();
    uint8_t * dst_data[AV_NUM_DATA_POINTERS];
    int dst_linesize[AV_NUM_DATA_POINTERS];
    int frame_cnt = 0;

    ret = av_image_alloc(dst_data, dst_linesize,playerEx->pVideoCodecContext->width,playerEx->pVideoCodecContext->height, playerEx->dstFormat, 1);
    if (ret< 0) {
        native_write_e( "Could not allocate destination image\n");
        return NULL;
    }

    while (!playerEx->mStop){
        ret = packet_queue_get(&playerEx->mVideoPacketQueue,pPacket,1,NULL);
        if(ret > 0){
            ret = avcodec_decode_video2(playerEx->pVideoCodecContext, pFrame, &got_picture, pPacket);
            if(ret < 0){
                native_write_e("Video Decode Error.\n");
                goto ContinueFlag;
            }
            if(got_picture){
                if(playerEx->mNextIVideo){
                    if(pFrame->pict_type == AV_PICTURE_TYPE_I || pFrame->pict_type == AV_PICTURE_TYPE_SI){
                        playerEx->mNextIVideo = false;
                    }else{
                        goto ContinueFlag;
                    }
                }

                sws_scale(playerEx->pSwsContext, pFrame->data, pFrame->linesize, 0, playerEx->pVideoCodecContext->height,
                                   dst_data, dst_linesize);

                playerEx->mNativeWindow->renderRGBA(dst_data,dst_linesize,playerEx->pVideoCodecContext->height,av_get_bits_per_pixel(av_pix_fmt_desc_get(playerEx->dstFormat)));

                native_print_d("av_read_video_frame Frame Index: %5d., pts:%lld,duration:%lld",frame_cnt,pFrame->pts,pFrame->pkt_duration);
                frame_cnt++;
            }
        }
        else if(ret == 0){

        }
        else{
            break;
        }

ContinueFlag:
        JLOGD("video loop one again");
        av_free_packet(pPacket);
    }

    if(pFrame){
        av_frame_free(&pFrame);
    }

    if(pPacket){
        av_packet_free(&pPacket);
    }

    return NULL;
}

void* PlayerEx::audio_thread(void *arg) {
    PlayerEx* playerEx = (PlayerEx*)arg;
    AVFrame* pFrame = av_frame_alloc();
    AVPacket* pPacket = av_packet_alloc();
    int got_frame;
    int ret ;
    int audio_frame_cnt = 0;
    int audioBufferSize = DEFAULT_SAMPLE_RATE * DEFAULT_CHANNEL_SIZE * 2;//rate * channelsize * bytes per sample
    uint8_t* pAudioOutBuffer = (uint8_t *)malloc(audioBufferSize) ;
    uint8_t* pAudioOutTmpBuffer = (uint8_t *)malloc(audioBufferSize) ;

    //audio
    playerEx->mAudioPlayer.createAudio();

    while (!playerEx->mStop){
        ret = packet_queue_get(&playerEx->mAudioPacketQueue,pPacket,1,NULL);
        if(ret > 0){
            avcodec_decode_audio4(playerEx->pAudioCodecContext, pFrame, &got_frame, pPacket);
            JLOGD("av_read_audio_frame Frame Index: %5d. getaudio:%d, pts:%lld,duration:%lld",audio_frame_cnt,got_frame,pFrame->pts,pFrame->pkt_duration);
            if (got_frame) {
                audio_frame_cnt++;

                int nb = swr_convert(playerEx->pSwrContext, &pAudioOutBuffer, audioBufferSize/DEFAULT_CHANNEL_SIZE,
                                     (const uint8_t **)(pFrame->data), pFrame->nb_samples);
                if (nb < 0)
                {
                    goto ContinueFlag;
                }
                int out_buffer_size = av_samples_get_buffer_size(NULL, DEFAULT_CHANNEL_SIZE, nb, playerEx->outSampleFormat, 1);
                memcpy(pAudioOutTmpBuffer, pAudioOutBuffer, out_buffer_size);
                int nb1 = 0, total_offset = out_buffer_size;
                while ((nb1 = swr_convert(playerEx->pSwrContext, &pAudioOutBuffer, audioBufferSize/DEFAULT_CHANNEL_SIZE, NULL, 0)) > 0)
                {
                    int out_buffer_size1 = av_samples_get_buffer_size(NULL, DEFAULT_CHANNEL_SIZE, nb1, playerEx->outSampleFormat, 1);
                    memcpy(pAudioOutTmpBuffer + total_offset, pAudioOutBuffer, out_buffer_size1);
                    total_offset += out_buffer_size1;
                }
                playerEx->mAudioPlayer.flushDirectEx(pAudioOutTmpBuffer,total_offset);
            }
            else{
                native_write_e("Audio Decode Error.\n");
                goto ContinueFlag;
            }
        }
        else if(ret == 0){

        }
        else{
            break;
        }

ContinueFlag:
        JLOGD("audio loop one again");
        av_free_packet(pPacket);
    }

    if(pAudioOutBuffer){
        free(pAudioOutBuffer);
    }

    if(pAudioOutTmpBuffer){
        free(pAudioOutTmpBuffer);
    }

    if(pFrame){
        av_frame_free(&pFrame);
    }

    if(pPacket){
        av_packet_free(&pPacket);
    }

    playerEx->mAudioPlayer.releaseAudio();

    return NULL;
}

int PlayerEx::android_render_rgb_on_rgb(ANativeWindow_Buffer *out_buffer, Uint8 **pixels ,int pitches[],int h, int bpp)
{
    int min_height = out_buffer->height > h ? h:out_buffer->height;
    int dst_stride = out_buffer->stride;
    int src_line_size = pitches[0];
    int dst_line_size = dst_stride * bpp / 8;

    uint8_t *dst_pixels = static_cast<uint8_t *>(out_buffer->bits);
    const uint8_t *src_pixels = pixels[0];

    if (dst_line_size == src_line_size) {
        int plane_size = src_line_size * min_height;
        // ALOGE("android_render_rgb_on_rgb (pix-match) %p %p %d", dst_pixels, src_pixels, plane_size);
        memcpy(dst_pixels, src_pixels, plane_size);
    } else {
        // TODO: 9 padding
        int bytewidth = dst_line_size > src_line_size ? src_line_size : dst_line_size;

        // ALOGE("android_render_rgb_on_rgb (pix-mismatch) %p %d %p %d %d %d", dst_pixels, dst_line_size, src_pixels, src_line_size, bytewidth, min_height);
        av_image_copy_plane(dst_pixels, dst_line_size, src_pixels, src_line_size, bytewidth, min_height);
    }

    return 0;
}

int JNIEXPORT PlayerEx::setVideoSurface(JNIEnv *env,jobject thiz,jint playerIndex,jobject surface){
    native_print_d("setVideoSurface %d",&surface);
    return playerArray[playerIndex]->getNativeWindow()->setSurface(env,surface);
}

int JNIEXPORT PlayerEx::seek(JNIEnv *env, jobject thiz,jint playerIndex, jlong timeInMilsecond) {
    return playerArray[playerIndex]->seekInner(timeInMilsecond);
}

int PlayerEx::play(JNIEnv *env, jobject thiz,jint playerIndex, jstring filePath) {

    const char *pFilePath = env->GetStringUTFChars(filePath, 0);
    jclass thizClass = env->GetObjectClass(thiz);
    native_print_d("play %s",pFilePath);

    int result = playerArray[playerIndex]->playInner(pFilePath);

    env->ReleaseStringUTFChars(filePath, pFilePath);
    return result;
}

void JNIEXPORT PlayerEx::initPlayer(JNIEnv *env,jobject thiz){
    for(int i = 0 ; i < MAX_PLAYER_NUMBER; i ++){
        MyNativeWindow *myNativeWindow = new MyNativeWindow();
        playerArray[i] = new PlayerEx(myNativeWindow);
    }
}
#ifdef __cplusplus
}
#endif