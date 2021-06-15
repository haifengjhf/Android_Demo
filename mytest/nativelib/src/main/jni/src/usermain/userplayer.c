//#include "Player.h"
#include "LogUtils.h"

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/log.h"
#include "libavutil/imgutils.h"
#include "SDL.h"
#include <pthread.h>

static pthread_cond_t cond;
static pthread_mutex_t mutex;

//Microsecond
unsigned long long lastPlayTime = 0;
/* no AV sync correction is done if below the minimum AV sync threshold 秒*/
#define AV_SYNC_THRESHOLD_MIN 0.04 * 1000
/* AV sync correction is done if above the maximum AV sync threshold 秒 */
#define AV_SYNC_THRESHOLD_MAX 0.01 * 1000

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
    SDL_UpdateTexture( pTexture, NULL , pFrameYUV->data[0], pFrameYUV->linesize[0] );
    SDL_RenderCopy(pRender, pTexture, NULL, NULL);
    SDL_RenderPresent(pRender);
}

int playInner(const char *filePath) {
    int result = -1;

    //sdl struct
    struct SDL_Window *pWindow = NULL;
    struct SDL_Renderer *pRender = NULL;
    int videoWidth,videoHeight;
    SDL_RendererInfo rendererInfo;
    SDL_Texture *pTexture = NULL;

    //ffmpeg struct
    AVFormatContext *pFormatContext = NULL;
    AVCodecContext *pCodecContext = NULL;
    AVCodec *pCodec = NULL;

    int videoindex = -1;
    AVFrame	*pFrame,*pFrameYUV = NULL;
    AVPacket* pPacket = NULL;
    unsigned char * pOutBuffer = NULL;
    struct SwsContext* pSwsContext = NULL;

    //other
    char info[1000]={0};
    int frame_cnt = 0;
    int ret;
    int got_picture;
    long ySize;

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);


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
            videoindex=i;
            break;
        }
    }
    if(videoindex==-1){
        native_write_e("Couldn't find a video stream.\n");
        goto EXIT0;
    }

    pCodecContext = pFormatContext->streams[videoindex]->codec;
//    avcodec_find_decoder(pFormatContext->streams[videoindex]->codecpar->codec_id);
    pCodec = avcodec_find_decoder(pCodecContext->codec_id);
    if(pCodec == NULL){
        native_write_e("avcodec_find_decoder error");
        goto EXIT0;
    }


    if(avcodec_open2(pCodecContext,pCodec,NULL) != 0){
        native_write_e("avcodec_open2 error");
        goto EXIT0;
    }

    pFrame = av_frame_alloc();
    pFrameYUV = av_frame_alloc();
    pOutBuffer = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P,  pCodecContext->width, pCodecContext->height,1));
    av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize,pOutBuffer,
                         AV_PIX_FMT_YUV420P,pCodecContext->width, pCodecContext->height,1);

    pPacket = (AVPacket *)av_malloc(sizeof(AVPacket));

    pSwsContext = sws_getContext(pCodecContext->width, pCodecContext->height, pCodecContext->pix_fmt,
                                 pCodecContext->width, pCodecContext->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

    sprintf(info,   "\n[Input     ]%s\n", filePath);
    sprintf(info, "%s[Format    ]%s\n",info, pFormatContext->iformat->name);
    sprintf(info, "%s[Codec     ]%s\n",info, pCodecContext->codec->name);
    sprintf(info, "%s[Resolution]%dx%d\n",info, pCodecContext->width,pCodecContext->height);

    sprintf(info, "pCodecContext %s[timebase]num:%d/den:%d\n",info, pCodecContext->time_base.num,pCodecContext->time_base.den);
    sprintf(info, "pFormatContext %s[timebase]num:%d/den:%d\n",info, pFormatContext->streams[videoindex]->time_base.num,pFormatContext->streams[videoindex]->time_base.den);
    native_write_d(info);

    //init sdl
    /* This interface could expand with ABI negotiation, calbacks, etc. */
//    SDL_Android_Init(env, thizClass);
//    SDL_SetMainReady();

    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER) != 0){
//        native_print_e("SDL_Init error: %s",SDL_GetError());
        goto EXIT0;
    }

    videoWidth = pCodecContext->width;
    videoHeight = pCodecContext->height;
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

    pTexture = SDL_CreateTexture(pRender, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, pCodecContext->width, pCodecContext->height);
    if (!pTexture) {
        native_print_e( "Couldn't set create texture: %s\n", SDL_GetError());
        goto EXIT0;
    }


//    playStartTime = clock()/CLOCKS_PER_SEC;
    while(av_read_frame(pFormatContext, pPacket)>=0){
        if(pPacket->stream_index==videoindex){
            ret = avcodec_decode_video2(pCodecContext, pFrame, &got_picture, pPacket);
            if(ret < 0){
                native_write_e("Decode Error.\n");
                return -1;
            }
            if(got_picture){
                sws_scale(pSwsContext, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecContext->height,
                          pFrameYUV->data, pFrameYUV->linesize);

                ySize = pCodecContext->width * pCodecContext->height;
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
                displayVideo(pRender,pTexture,pFormatContext->streams[videoindex],pFrameYUV,pCodecContext->time_base);
            }
            else{
                native_print_d("av_read_frame Frame skip Index: %5d.",frame_cnt);
            }
        }
        av_free_packet(pPacket);
    }
    //flush decoder
    //FIX: Flush Frames remained in Codec
    while (1) {
        ret = avcodec_decode_video2(pCodecContext, pFrame, &got_picture, pPacket);
        if (ret < 0)
            break;
        if (!got_picture)
            break;
        sws_scale(pSwsContext, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecContext->height,
                  pFrameYUV->data, pFrameYUV->linesize);
        int y_size=pCodecContext->width*pCodecContext->height;
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
        displayVideo(pRender,pTexture,pFormatContext->streams[videoindex],pFrameYUV,pFormatContext->streams[videoindex]->time_base);
    }


    EXIT0:
    if(pOutBuffer != NULL)av_free(pOutBuffer);

    if(pFrameYUV != NULL){av_frame_free(&pFrameYUV);}
    if(pFrame != NULL){av_frame_free(&pFrame);}
    if(pPacket != NULL){av_packet_free(&pPacket);}
    if(pFormatContext != NULL){avformat_close_input(&pFormatContext);}

    sws_freeContext(pSwsContext);
    avcodec_close(pCodecContext);

    if(pRender != NULL){SDL_DestroyRenderer(pRender);}
    if(pWindow != NULL ){SDL_DestroyWindow(pWindow);}

    return result;
}


int main(int argc, char *argv[]) {
    playInner("/storage/emulated/0/Android/data/com.jhf.test/sintel.mp4");
}