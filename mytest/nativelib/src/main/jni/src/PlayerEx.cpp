//
// Created by 金海峰 on 2021/7/5.
//


#include "PlayerEx.h"
#include "VideoScale.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "LogUtils.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/log.h"
#include "libavutil/imgutils.h"
#include <libswresample/swresample.h>
#include "SDL.h"

MyNativeWindow clsWindow;
PlayerEx clsPlayer(&clsWindow);

PlayerEx::PlayerEx(MyNativeWindow* myNativeWindow) {
    mNativeWindow = myNativeWindow;
}

int PlayerEx::playInner(const char *filePath) {
    int result = -1;

    //ffmpeg struct
    AVFormatContext *pFormatContext = nullptr;
    int videoindex(-1),audioIndex(-1);
    AVCodecContext *pVideoCodecContext(nullptr),*pAudioCodecContext(nullptr);
    AVCodec *pVideoCodec(nullptr),*pAudioCodec(nullptr);
    AVFrame	*pFrame(nullptr);
    AVPacket* pPacket(nullptr);

    struct SwrContext *pSwrContext = swr_alloc();
    struct SwsContext *pSwsContext;
//    uint8_t *pVideoOutBuffer = NULL;
//    int videoBufferSize = 0;
//    VideoDecorate clsVideoDecorate;

    //other
    char info[1000]={0};
    int frame_cnt = 0;
    int time_start = clock();
    int ret;
    int got_picture;
    uint8_t* pAudioOutBuffer;
    AVPixelFormat dstFormat = AV_PIX_FMT_RGBA;
    bool isDone = true;

    //init ffmpeg
    av_log_set_callback(custom_log);

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

    native_write_d("codec video ready");

    //audio
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

    native_write_d("codec ready");

    pFrame = av_frame_alloc();
    pPacket = (AVPacket *)av_malloc(sizeof(AVPacket));

    pSwsContext = sws_getContext(pVideoCodecContext->width, pVideoCodecContext->height, pVideoCodecContext->pix_fmt,
                                 pVideoCodecContext->width,pVideoCodecContext->height, dstFormat, SWS_BICUBIC, NULL, NULL, NULL);

//    clsVideoDecorate.setVideoGeometry(pVideoCodecContext->width,pVideoCodecContext->height);
//    videoBufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGBA, pVideoCodecContext->width, pVideoCodecContext->height, 1);
//    pVideoOutBuffer = (uint8_t *) av_malloc(videoBufferSize);


    uint8_t *dst_data[AV_NUM_DATA_POINTERS];
    int dst_linesize[AV_NUM_DATA_POINTERS];
    ret = av_image_alloc(dst_data, dst_linesize,pVideoCodecContext->width,pVideoCodecContext->height, dstFormat, 1);
    if (ret< 0) {
        native_write_e( "Could not allocate destination image\n");
        return -1;
    }

    sprintf(info,   "\n[Input     ]%s\n", filePath);
    sprintf(info, "%s[Format    ]%s\n",info, pFormatContext->iformat->name);
    sprintf(info, "%s[Codec     ]%s\n",info, pVideoCodecContext->codec->name);
    sprintf(info, "%s[Resolution]%dx%d\n",info, pVideoCodecContext->width,pVideoCodecContext->height);
    sprintf(info, "%s[timebase]num:%d/den:%d\n",info, pFormatContext->streams[videoindex]->time_base.num,pFormatContext->streams[videoindex]->time_base.den);
    native_write_d(info);

    while(av_read_frame(pFormatContext, pPacket)>=0){
        if(pPacket->stream_index==videoindex){
            ret = avcodec_decode_video2(pVideoCodecContext, pFrame, &got_picture, pPacket);
            if(ret < 0){
                native_write_e("Decode Error.\n");
                return -1;
            }
            if(got_picture){
                result = sws_scale(pSwsContext, pFrame->data, pFrame->linesize, 0, pVideoCodecContext->height,
                          dst_data, dst_linesize);
                mNativeWindow->renderRGBA(dst_data,dst_linesize,pVideoCodecContext->height,av_get_bits_per_pixel(av_pix_fmt_desc_get(dstFormat)));

                if(!isDone){
                    isDone = true;
                    fwrite(dst_data[0],1,pVideoCodecContext->width*pVideoCodecContext->height * av_get_bits_per_pixel(av_pix_fmt_desc_get(dstFormat))/8,dst_file);
                }

                const int pictype_buffer_size = 100;
                char pictype_str[pictype_buffer_size]={0};
                switch(pFrame->pict_type){
                    case AV_PICTURE_TYPE_I:sprintf(pictype_str,"I");break;
                    case AV_PICTURE_TYPE_P:sprintf(pictype_str,"P");break;
                    case AV_PICTURE_TYPE_B:sprintf(pictype_str,"B");break;
                    default:snprintf(pictype_str,pictype_buffer_size,"Other %d",pFrame->pict_type);break;
                }
                native_print_d("av_read_frame Frame Index: %5d. Type:%s, pts:%lld,duration:%lld",frame_cnt,pictype_str,pFrame->pts,pFrame->pkt_duration);
                frame_cnt++;
            }
            else{
                native_print_d("av_read_frame Frame skip Index: %5d.",frame_cnt);
            }
        }
        else if(pPacket->stream_index == audioIndex){
//            result = avcodec_decode_audio4(pAudioCodecContext, pFrame, &got_picture, pPacket);

//            if (got_picture) {
//                int size  = av_samples_get_buffer_size(NULL, pAudioCodecContext->channels, pFrame->nb_samples, AV_SAMPLE_FMT_S16, 1);
//                pAudioOutBuffer = (uint8_t *)malloc(size);
//                result = swr_convert(swrContext, (uint8_t **)&pAudioOutBuffer, size, (const uint8_t **) pFrame->data, pFrame->nb_samples);
//                result = SDL_QueueAudio(audioDeviceId,pAudioOutBuffer,size);
//                free(pAudioOutBuffer);
//            }
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

        result = sws_scale(pSwsContext, pFrame->data, pFrame->linesize, 0, pVideoCodecContext->height,
                           dst_data, dst_linesize);
        mNativeWindow->renderRGBA(dst_data,dst_linesize,pVideoCodecContext->height,av_get_bits_per_pixel(av_pix_fmt_desc_get(dstFormat)));

        char pictype_str[10]={0};
        switch(pFrame->pict_type){
            case AV_PICTURE_TYPE_I:sprintf(pictype_str,"I");break;
            case AV_PICTURE_TYPE_P:sprintf(pictype_str,"P");break;
            case AV_PICTURE_TYPE_B:sprintf(pictype_str,"B");break;
            default:sprintf(pictype_str,"Other");break;
        }
        native_print_d("av_read_frame Frame Index: %5d. Type:%s, pts:%lld,duration:%lld",frame_cnt,pictype_str,pFrame->pts,pFrame->pkt_duration);
        frame_cnt++;    }

EXIT0:
    if(dst_file){
        fclose(dst_file);
    }

    if(pFrame != nullptr){
        av_frame_free(&pFrame);
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

int PlayerEx::android_render_rgb_on_rgb(ANativeWindow_Buffer *out_buffer, Uint8 **pixels ,int pitches[],int h, int bpp)
{
    // SDLTRACE("SDL_VoutAndroid: android_render_rgb_on_rgb(%p)", overlay);
//    assert(overlay->format == SDL_FCC_RV16);
//    assert(overlay->planes == 1);

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

int JNIEXPORT PlayerEx::setVideoSurface(JNIEnv *env,jobject thiz,jobject surface){
    native_print_d("setVideoSurface %d",&surface);
    return clsWindow.setSurface(env,surface);
}

int PlayerEx::play(JNIEnv *env, jobject thiz, jstring filePath) {

    const char *pFilePath = env->GetStringUTFChars(filePath, 0);
    jclass thizClass = env->GetObjectClass(thiz);
    native_print_d("play %s",pFilePath);

    int result = clsPlayer.playInner(pFilePath);

    env->ReleaseStringUTFChars(filePath, pFilePath);
    return result;
}


#ifdef __cplusplus
}
#endif