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
#include <libavutil/file.h>

#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include <libavutil/opt.h>

const int MAX_PLAYER_NUMBER = 2;
PlayerEx* playerArray[MAX_PLAYER_NUMBER];

#define ALPHA_BLEND(a, oldp, newp, s)\
((((oldp << s) * (255 - (a))) + (newp * (a))) / (255 << s))

struct buffer_data {
    uint8_t *ptr;
    size_t size; ///< size left in the buffer
};

static void blend_subrect(AVPicture *dst, const AVSubtitleRect *rect, int imgw, int imgh)
{
    int x, y, Y, U, V, A;
    uint8_t *lum, *cb, *cr;
    int dstx, dsty, dstw, dsth;
    const AVPicture *src = &rect->pict;

    dstw = av_clip(rect->w, 0, imgw);
    dsth = av_clip(rect->h, 0, imgh);
    dstx = av_clip(rect->x, 0, imgw - dstw);
    dsty = av_clip(rect->y, 0, imgh - dsth);
    lum = dst->data[0] + dstx + dsty * dst->linesize[0];
    cb  = dst->data[1] + dstx/2 + (dsty >> 1) * dst->linesize[1];
    cr  = dst->data[2] + dstx/2 + (dsty >> 1) * dst->linesize[2];

    for (y = 0; y<dsth; y++) {
        for (x = 0; x<dstw; x++) {
            Y = src->data[0][x + y*src->linesize[0]];
            A = src->data[3][x + y*src->linesize[3]];
            lum[0] = ALPHA_BLEND(A, lum[0], Y, 0);
            lum++;
        }
        lum += dst->linesize[0] - dstw;
    }

    for (y = 0; y<dsth/2; y++) {
        for (x = 0; x<dstw/2; x++) {
            U = src->data[1][x + y*src->linesize[1]];
            V = src->data[2][x + y*src->linesize[2]];
            A = src->data[3][2*x     +  2*y   *src->linesize[3]]
                + src->data[3][2*x + 1 +  2*y   *src->linesize[3]]
                + src->data[3][2*x + 1 + (2*y+1)*src->linesize[3]]
                + src->data[3][2*x     + (2*y+1)*src->linesize[3]];
            cb[0] = ALPHA_BLEND(A>>2, cb[0], U, 0);
            cr[0] = ALPHA_BLEND(A>>2, cr[0], V, 0);
            cb++;
            cr++;
        }
        cb += dst->linesize[1] - dstw/2;
        cr += dst->linesize[2] - dstw/2;
    }
}

static int read_packet_from_memory(void *opaque, uint8_t *buf, int buf_size)
{
    struct buffer_data *bd = (struct buffer_data *)opaque;
    buf_size = FFMIN(buf_size, bd->size);

    /* copy internal buffer data to buf */
    memcpy(buf, bd->ptr, buf_size);
    bd->ptr  += buf_size;
    bd->size -= buf_size;

    JLOGD("ptr:%p size:%zu\n", bd->ptr, bd->size);
    return buf_size;
}


//Callback
int read_packet_from_file(void *opaque, uint8_t *buf, int buf_size) {
    FILE *fp_open = (FILE*)opaque;
    if (!feof(fp_open)) {
        int true_size = fread(buf, 1, buf_size, fp_open);
        return true_size;
    } else {
        return -1;
    }
}

PlayerEx::PlayerEx(MyNativeWindow* myNativeWindow):mNextIVideo(false),mVideoSeekTimeInMilsecond(0),mAudioSeekTimeInMilsecond(0),mStop(false){
    mNativeWindow = myNativeWindow;

    packet_queue_init(&mVideoPacketQueue);
    packet_queue_init(&mAudioPacketQueue);
    packet_queue_init(&mSubTitlePacketQueue);
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

int PlayerEx::setSpeedReal(float speed){
    if(mSpeedChange){
        return 0;
    }

    if(mSpeed == speed){
        return 0;
    }

    mSpeed = speed;
    mSpeedChange = true;
    return 1;
}

int PlayerEx::playInner(const char *filePath) {
    int result = -1;

    //ffmpeg struct
    AVCodec *pVideoCodec(nullptr),*pAudioCodec(nullptr),*pSubtitleCodec(nullptr);
    AVPacket* pPacket = av_packet_alloc();
    AVIOContext  *avio_ctx;

    size_t buffer_size = 0, avio_ctx_buffer_size = 4096;
    uint8_t *buffer = NULL, *avio_ctx_buffer = NULL;
    struct buffer_data bd = { 0 };
    FILE *fp_open;

    //other
    char info[1000]={0};
    int packetCnt = 0;
    //init ffmpeg
//    av_log_set_callback(custom_log);

    av_register_all();
    avformat_network_init();
    mFormatContext = avformat_alloc_context();

    native_write_d("av init ready");

//    const char *filter_descr = "lutyuv='u=128:v=128'";
//    const char *filter_descr = "boxblur";
    //const char *filter_descr = "hflip";
//    const char *filter_descr = "hue='h=60:s=-3'";
    //const char *filter_descr = "crop=2/3*in_w:2/3*in_h";
//    const char *filter_descr = "drawbox=x=100:y=100:w=100:h=100:color=pink@0.5";
//    const char *filter_descr = "drawtext=fontfile=arial.ttf:fontcolor=green:fontsize=30:text='Jin Haifeng'";
//    const char *filter_descr = "drawtext=fontcolor=green:fontsize=30:text='Jin Haifeng'";
    const char *filter_descr = "movie=/sdcard/Android/data/com.jhf.test/IMG_1902.PNG,scale=60:30[watermark];[in][watermark]overlay=5:5[out]";

    if(customInput == 1){
        result = av_file_map(filePath, &buffer, &buffer_size, 0, NULL);
        if (result < 0)
            goto EXIT0;

        /* fill opaque structure used by the AVIOContext read callback */
        bd.ptr  = buffer;
        bd.size = buffer_size;

        avio_ctx_buffer = (uint8_t *)av_malloc(avio_ctx_buffer_size);

        avio_ctx = avio_alloc_context(avio_ctx_buffer,avio_ctx_buffer_size,0,&bd,read_packet_from_memory,NULL,NULL);
        mFormatContext->pb = avio_ctx;
        mFormatContext->flags |= AVFMT_FLAG_CUSTOM_IO;

        // Now we set the ProbeData-structure for av_probe_input_format:
        const int probeSize = 32 * 1024;
        uint8_t* pBuffer = (uint8_t*)av_malloc(probeSize + AVPROBE_PADDING_SIZE);
        memcpy(pBuffer,buffer,probeSize);
        memset(pBuffer + probeSize,0,AVPROBE_PADDING_SIZE);

        AVProbeData probeData;
        probeData.buf = pBuffer;
        probeData.buf_size = probeSize;
        probeData.filename = filePath;
        probeData.mime_type = filePath;
        // Determine the input-format:
        mFormatContext->iformat = av_probe_input_format(&probeData, 1);
        JLOGD("av_probe_input_format ready");

        if(avformat_open_input(&mFormatContext,filePath,NULL,NULL) != 0){
            native_write_e("avformat_open_input error");
            goto EXIT0;
        }

        av_free(pBuffer);
    }
    else if(customInput == 2){
        fp_open = fopen(filePath,"rb+");
        avio_ctx_buffer = (uint8_t *)av_malloc(avio_ctx_buffer_size);

        avio_ctx = avio_alloc_context(avio_ctx_buffer,avio_ctx_buffer_size,0,fp_open,read_packet_from_file,NULL,NULL);
        mFormatContext->pb = avio_ctx;

        if(avformat_open_input(&mFormatContext,NULL,NULL,NULL) != 0){
            native_write_e("avformat_open_input error");
            goto EXIT0;
        }
    }
    else{
        if(avformat_open_input(&mFormatContext,filePath,NULL,NULL) != 0){
            native_write_e("avformat_open_input error");
            goto EXIT0;
        }
    }

    native_write_d("avformat_open_input ready");

    if(avformat_find_stream_info(mFormatContext,NULL) != 0){
        native_write_e("avformat_find_stream_info error");
        goto EXIT0;
    }
    native_write_d("avformat_find_stream_info ready");


    for(int i=0; i<mFormatContext->nb_streams; i++){
        native_print_d("streams nb_streams: %d, i :%d, type: %d",mFormatContext->nb_streams,i,mFormatContext->streams[i]->codec->codec_type);
        if(mFormatContext->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
            mVideoIndex = i;
        }
        else if(mFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
            mAudioIndex = i;
        }
        else if(mFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE){
            mSubTitleIndex = i;
        }
    }
    if(mVideoIndex==-1){
        native_write_e("Couldn't find a video stream.\n");
        goto EXIT0;
    }
    if(mAudioIndex==-1){
        native_write_e("Couldn't find a audio stream.\n");
        goto EXIT0;
    }

    //subtitle
    if(mSubTitleIndex != -1){
        pSubtitleCodecContext = mFormatContext->streams[mSubTitleIndex]->codec;
        pSubtitleCodec = avcodec_find_decoder(pSubtitleCodecContext->codec_id);
        result = avcodec_open2(pSubtitleCodecContext, pSubtitleCodec, NULL);
        if(result != 0 ){
            JLOGE("subtitle open error :%d",result);
        }
    }

    pVideoCodecContext = mFormatContext->streams[mVideoIndex]->codec;
    pVideoCodec = avcodec_find_decoder(pVideoCodecContext->codec_id);
    if(pVideoCodec == nullptr){
        native_write_e("avcodec_find_decoder error");
        goto EXIT0;
    }
    if(avcodec_open2(pVideoCodecContext,pVideoCodec,NULL) != 0){
        native_write_e("avcodec_open2 error");
        goto EXIT0;
    }

    if(pVideoCodecContext->pix_fmt == AV_PIX_FMT_NONE){
        pVideoCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
    }
    pSwsContext = sws_getContext(pVideoCodecContext->width, pVideoCodecContext->height, pVideoCodecContext->pix_fmt,
                                 pVideoCodecContext->width,pVideoCodecContext->height, dstFormat, SWS_BICUBIC, NULL, NULL, NULL);

    native_write_d("codec video ready");

    pAudioCodecContext = mFormatContext->streams[mAudioIndex]->codec;
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


    //filters
    init_filters(filter_descr);


    sprintf(info,   "\n[Input     ]%s\n", filePath);
    sprintf(info, "%s[Format    ]%s\n",info, mFormatContext->iformat->name);
    sprintf(info, "%s[Codec     ]%s\n",info, pVideoCodecContext->codec->name);
    sprintf(info, "%s[Resolution]%dx%d\n",info, pVideoCodecContext->width,pVideoCodecContext->height);
    sprintf(info, "%s[timebase]num:%d/den:%d\n",info, mFormatContext->streams[mVideoIndex]->time_base.num,mFormatContext->streams[mVideoIndex]->time_base.den);
    native_write_d(info);

    //start audio ,video thread
    pthread_t audioThread,videoThread;
    pthread_create(&audioThread,NULL,PlayerEx::audio_thread,this);
    pthread_create(&videoThread,NULL,PlayerEx::video_thread,this);

    packet_queue_start(&mAudioPacketQueue);
    packet_queue_start(&mVideoPacketQueue);
    packet_queue_start(&mSubTitlePacketQueue);

    mCurSyncClock = 0;
    mAudioLastPts = 0;
    mAudioLastPlayClock = 0;
    mVideoLastPts = 0;
    mVideoLastPlayClock = 0;

    while((result = av_read_frame(mFormatContext, pPacket))>=0){
        JLOGD("av_read_frame %d,stream_index:%d",++packetCnt,pPacket->stream_index);
        if(pPacket->stream_index==mVideoIndex){
            if(mVideoSeekTimeInMilsecond > 0){
                double timeBase = av_q2d(mFormatContext->streams[mVideoIndex]->time_base) * 1000;
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
        else if(pPacket->stream_index == mAudioIndex){
            if(mAudioSeekTimeInMilsecond > 0){
                double timeBase = av_q2d(mFormatContext->streams[mAudioIndex]->time_base) * 1000;
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
        else if(pPacket->stream_index == mSubTitleIndex){
            packet_queue_put(&mSubTitlePacketQueue,pPacket);
        }
        else{
ContinueFlag:
            av_free_packet(pPacket);
        }
    }

    JLOGD("play thread read file end wait thread exit result:%d",result);

    //等待音视频线程退出，然后释放资源
    pthread_join(audioThread,NULL);
    pthread_join(videoThread,NULL);

EXIT0:
    JLOGD("play thread exit");

    /* note: the internal buffer could have changed, and be != avio_ctx_buffer */
    if (avio_ctx) {
        av_freep(&avio_ctx->buffer);
        av_freep(&avio_ctx);
    }
    av_file_unmap(buffer, buffer_size);

    if(pPacket != nullptr){
        av_packet_free(&pPacket);
    }

    if(mFormatContext != nullptr){
        avformat_close_input(&mFormatContext);
    }
    if(pVideoCodecContext){
        avcodec_close(pVideoCodecContext);
    }
    if(filter_graph){
        avfilter_graph_free(&filter_graph);
    }



    return result;
}

void* PlayerEx::video_thread(void *arg) {


    PlayerEx* player = (PlayerEx*)arg;
    int ret ;
    int got_picture,got_subtitle;
    AVFrame* pFrame = av_frame_alloc();
    AVFrame *filt_frame = av_frame_alloc();
    AVPacket packet;// = av_packet_alloc();
    AVPacket subTitlePacket ;
    AVSubtitle avSubtitle;
    AVPicture pict;

    uint8_t * dst_data[AV_NUM_DATA_POINTERS];
    int dst_linesize[AV_NUM_DATA_POINTERS];
    int frame_cnt = 0;

    ret = av_image_alloc(dst_data, dst_linesize,player->pVideoCodecContext->width,player->pVideoCodecContext->height, player->dstFormat, 1);
    if (ret< 0) {
        native_write_e( "Could not allocate destination image\n");
        return NULL;
    }


    while (!player->mStop){
        ret = packet_queue_get(&player->mVideoPacketQueue,&packet,1,NULL);
        if(ret > 0){
            ret = avcodec_decode_video2(player->pVideoCodecContext, filt_frame, &got_picture, &packet);
            if(ret < 0 || !got_picture){
                native_write_e("Video Decode Error.\n");
                goto ContinueFlag;
            }

            if(player->mNextIVideo){
                if(filt_frame->pict_type == AV_PICTURE_TYPE_I || filt_frame->pict_type == AV_PICTURE_TYPE_SI){
                    player->mNextIVideo = false;
                }else{
                    goto ContinueFlag;
                }
            }

            filt_frame->pts = av_frame_get_best_effort_timestamp(filt_frame);
            /* push the decoded frame into the filtergraph */
            if (av_buffersrc_add_frame_flags(player->buffersrc_ctx, filt_frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
                JLOGE("Error while feeding the filtergraph\n");
                break;
            }

            while (1) {
                ret = av_buffersink_get_frame(player->buffersink_ctx, pFrame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF || ret < 0){
                    JLOGE("Error while av_buffersink_get_frame ret:%d\n",ret);
                    break;
                }

                sws_scale(player->pSwsContext, pFrame->data, pFrame->linesize, 0, player->pVideoCodecContext->height,
                          dst_data, dst_linesize);

                //subtitle
                if(player->mSubTitleIndex != -1){
                    ret = packet_queue_get(&player->mSubTitlePacketQueue,&subTitlePacket,0,NULL);
                    if(ret > 0){
                        ret = avcodec_decode_subtitle2(player->pSubtitleCodecContext, &avSubtitle, &got_subtitle, &subTitlePacket);
                        if(got_subtitle){
                            for (int i = 0; i < avSubtitle.num_rects; i++)
                            {
                                int in_w = avSubtitle.rects[i]->w;
                                int in_h = avSubtitle.rects[i]->h;
                                int subw = player->pSubtitleCodecContext->width  ? player->pSubtitleCodecContext->width  : player->pVideoCodecContext->width;
                                int subh = player->pSubtitleCodecContext->height ? player->pSubtitleCodecContext->height : player->pVideoCodecContext->height;
                                int out_w = player->pVideoCodecContext->width  ? in_w * player->pVideoCodecContext->width  / subw : in_w;
                                int out_h = player->pVideoCodecContext->height ? in_h * player->pVideoCodecContext->height / subh : in_h;
                                AVPicture newpic;

                                //can not use avpicture_alloc as it is not compatible with avsubtitle_free()
                                av_image_fill_linesizes(newpic.linesize, AV_PIX_FMT_YUVA420P, out_w);
                                newpic.data[0] = (uint8_t *)av_malloc(newpic.linesize[0] * out_h);
                                newpic.data[3] = (uint8_t *)av_malloc(newpic.linesize[3] * out_h);
                                newpic.data[1] = (uint8_t *)av_malloc(newpic.linesize[1] * ((out_h+1)/2));
                                newpic.data[2] = (uint8_t *)av_malloc(newpic.linesize[2] * ((out_h+1)/2));

                                player->pSubSwsContext = sws_getCachedContext(player->pSubSwsContext,
                                                                              in_w, in_h, AV_PIX_FMT_PAL8, out_w, out_h,
                                                                              AV_PIX_FMT_YUVA420P, SWS_BICUBIC, NULL, NULL, NULL);
                                if (!player->pSubSwsContext || !newpic.data[0] || !newpic.data[3] ||
                                    !newpic.data[1] || !newpic.data[2]
                                        ) {
                                    av_log(NULL, AV_LOG_FATAL, "Cannot initialize the sub conversion context\n");
                                    exit(1);
                                }
                                sws_scale(player->pSubSwsContext,
                                          avSubtitle.rects[i]->pict.data, avSubtitle.rects[i]->pict.linesize,
                                          0, in_h, newpic.data, newpic.linesize);

                                av_free(avSubtitle.rects[i]->pict.data[0]);
                                av_free(avSubtitle.rects[i]->pict.data[1]);
                                avSubtitle.rects[i]->pict = newpic;
                                avSubtitle.rects[i]->w = out_w;
                                avSubtitle.rects[i]->h = out_h;
                                avSubtitle.rects[i]->x = avSubtitle.rects[i]->x * out_w / in_w;
                                avSubtitle.rects[i]->y = avSubtitle.rects[i]->y * out_h / in_h;


                                if (pFrame->pts >= avSubtitle.pts + ((float) avSubtitle.start_display_time / 1000)) {

                                    pict.data[0] = dst_data[0];
                                    pict.data[1] = dst_data[2];
                                    pict.data[2] = dst_data[1];

                                    pict.linesize[0] = dst_linesize[0];
                                    pict.linesize[1] = dst_linesize[2];
                                    pict.linesize[2] = dst_linesize[1];

                                    for (i = 0; i < avSubtitle.num_rects; i++)
                                        blend_subrect(&pict,avSubtitle.rects[i],
                                                      player->pVideoCodecContext->width, player->pVideoCodecContext->height);
                                }
                            }
                        }
                    }
                }

                player->videoTimeSync(pFrame);

                player->mNativeWindow->renderRGBA(dst_data,dst_linesize,player->pVideoCodecContext->height,av_get_bits_per_pixel(av_pix_fmt_desc_get(player->dstFormat)));

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
        ;
//        av_free_packet(pPacket);
    }

    if(pFrame){
        av_frame_free(&pFrame);
    }
    if(filt_frame){
        av_frame_free(&filt_frame);
    }

    return NULL;
}

void* PlayerEx::audio_thread(void *arg) {
    PlayerEx* player = (PlayerEx*)arg;
    AVFrame* pFrame = av_frame_alloc();
    AVPacket packet;// = av_packet_alloc();
    int got_frame;
    int ret ;
    int audio_frame_cnt = 0;
    int audioBufferSize = DEFAULT_SAMPLE_RATE * DEFAULT_CHANNEL_SIZE * 2;//rate * channelsize * bytes per sample
    uint8_t* pAudioOutBuffer = (uint8_t *)malloc(audioBufferSize) ;
    uint8_t* pAudioOutTmpBuffer = (uint8_t *)malloc(audioBufferSize) ;

    //audio
    player->mAudioPlayer.createAudio();

    while (!player->mStop){
        ret = packet_queue_get(&player->mAudioPacketQueue,&packet,1,NULL);
        if(ret > 0){
            avcodec_decode_audio4(player->pAudioCodecContext, pFrame, &got_frame, &packet);
            JLOGD("av_read_audio_frame Frame Index: %5d. getaudio:%d, pts:%lld,duration:%lld",audio_frame_cnt,got_frame,pFrame->pts,pFrame->pkt_duration);
            if (got_frame) {
                audio_frame_cnt++;

                if(player->mSpeedChange){
                    player->mAudioPlayer.setSpeed(player->mSpeed);
                    player->mSpeedChange = false;
                }
                int nb = swr_convert(player->pSwrContext, &pAudioOutBuffer, audioBufferSize/DEFAULT_CHANNEL_SIZE,
                                     (const uint8_t **)(pFrame->data), pFrame->nb_samples);
                if (nb < 0)
                {
                    goto ContinueFlag;
                }
                int out_buffer_size = av_samples_get_buffer_size(NULL, DEFAULT_CHANNEL_SIZE, nb, player->outSampleFormat, 1);
                memcpy(pAudioOutTmpBuffer, pAudioOutBuffer, out_buffer_size);
                int nb1 = 0, total_offset = out_buffer_size;
                while ((nb1 = swr_convert(player->pSwrContext, &pAudioOutBuffer, audioBufferSize/DEFAULT_CHANNEL_SIZE, NULL, 0)) > 0)
                {
                    int out_buffer_size1 = av_samples_get_buffer_size(NULL, DEFAULT_CHANNEL_SIZE, nb1, player->outSampleFormat, 1);
                    memcpy(pAudioOutTmpBuffer + total_offset, pAudioOutBuffer, out_buffer_size1);
                    total_offset += out_buffer_size1;
                }

                player->audioTimeSync(pFrame);

                player->mAudioPlayer.flushDirectEx(pAudioOutTmpBuffer,total_offset);
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
        ;
//        av_free_packet(pPacket);
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

    player->mAudioPlayer.releaseAudio();

    return NULL;
}

void PlayerEx::audioTimeSync(AVFrame* pFrame){
    struct timespec timeSpec;
    clock_gettime(CLOCK_MONOTONIC,&timeSpec);
    double curTime = timeSpec.tv_sec + ((double)timeSpec.tv_nsec)/1000000000;
    if(mCurSyncClock == 0){
        //第一帧则直接播放
        mAudioLastPlayClock = curTime;
    }
    else{
        //timebase 使用stream 的timebase
        double diff = (mAudioLastPlayClock + (double)(pFrame->pts - mAudioLastPts) * av_q2d(mFormatContext->streams[mAudioIndex]->time_base)/mSpeed) - curTime ;
        JLOGD("audioTimeSync diff:%f,mAudioLastPlayClock:%f,curTime:%f, curPts:%lld, lastPts:%ld,timebase:%f",diff,mAudioLastPlayClock,curTime,pFrame->pts,mAudioLastPts,av_q2d(mFormatContext->streams[mAudioIndex]->time_base));
        if(diff > 0){
            //预期时间还未到，休眠一段间隔时间
            timeSpec.tv_sec = (int)(diff);
            timeSpec.tv_nsec = (long)((diff - timeSpec.tv_sec)*1000000000);
            nanosleep(&timeSpec,NULL);

            clock_gettime(CLOCK_MONOTONIC,&timeSpec);
            curTime = timeSpec.tv_sec + (double)timeSpec.tv_nsec/1000000000;
            mAudioLastPlayClock = curTime;
        }
        else{
            //预期时间已经过了，马上播放
            //如果是视频同步音频，所以音频不存在丢包
            //如果是实时音频，则需要判断超时时间长度，时间超过阀值需要丢弃,咱不处理

            mAudioLastPlayClock = curTime;
        }
    }

    mAudioLastPts = pFrame->pts;

    mCurSyncClock = pFrame->pts * av_q2d(mFormatContext->streams[mAudioIndex]->time_base)/mSpeed;
}

void PlayerEx::videoTimeSync(AVFrame* pFrame){
    struct timespec timeSpec;
    clock_gettime(CLOCK_MONOTONIC,&timeSpec);
    double curTime = timeSpec.tv_sec + (double)timeSpec.tv_nsec/1000000000;
    if(mCurSyncClock == 0){
        //第一帧则直接播放
        mVideoLastPlayClock = curTime;
    }
    else{
        //timebase 使用stream 的timebase
        double diff = (pFrame->pts) * av_q2d(mFormatContext->streams[mVideoIndex]->time_base)/mSpeed - mCurSyncClock ;
        JLOGD("videoTimeSync diff:%f,mVideoLastPlayClock:%f,curTime:%f, curPts:%lld, lastPts:%ld,timebase:%f",diff,mVideoLastPlayClock,curTime,pFrame->pts,mVideoLastPts,av_q2d(mFormatContext->streams[mVideoIndex]->time_base));
        if(diff > 0){
            //预期时间还未到，休眠一段间隔时间
            timeSpec.tv_sec = (int)(diff);
            timeSpec.tv_nsec = (long)((diff - timeSpec.tv_sec)*1000000000);
            nanosleep(&timeSpec,NULL);

            clock_gettime(CLOCK_MONOTONIC,&timeSpec);
            curTime = timeSpec.tv_sec + (double)timeSpec.tv_nsec/1000000000;
            mVideoLastPlayClock = curTime;
        }
        else{
            //预期时间已经过了，马上播放
            //如果是视频同步音频，所以音频不存在丢包
            //如果是实时音频，则需要判断超时时间长度，时间超过阀值需要丢弃,咱不处理

            mVideoLastPlayClock = curTime;
        }
    }

    mVideoLastPts = pFrame->pts;
}

int PlayerEx::init_filters(const char *filters_descr)
{
    char args[512];
    int ret;
    const AVFilter *buffersrc  = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();
    enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE };
    AVBufferSinkParams *buffersink_params;

    filter_graph = avfilter_graph_alloc();

    /* buffer video source: the decoded frames from the decoder will be inserted here. */
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             pVideoCodecContext->width, pVideoCodecContext->height, pVideoCodecContext->pix_fmt,
             pVideoCodecContext->time_base.num, pVideoCodecContext->time_base.den,
             pVideoCodecContext->sample_aspect_ratio.num, pVideoCodecContext->sample_aspect_ratio.den);

    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                                       args, NULL, filter_graph);
    if (ret < 0) {
        JLOGE("Cannot create buffer source\n");
        return ret;
    }

    /* buffer video sink: to terminate the filter chain. */
    buffersink_params = av_buffersink_params_alloc();
    buffersink_params->pixel_fmts = pix_fmts;

    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
                                       NULL, buffersink_params, filter_graph);
    if (ret < 0) {
        JLOGE("Cannot create buffer sink\n");
        return ret;
    }
//    ret = av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts,
//                              AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);

//    av_free(buffersink_params);


    /* Endpoints for the filter graph. */
    outputs->name       = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

    inputs->name       = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;

    if ((ret = avfilter_graph_parse_ptr(filter_graph, filters_descr,
                                        &inputs, &outputs, NULL)) < 0){
        JLOGE("avfilter_graph_parse_ptr ret:%d\n",ret);
        return ret;
    }


    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0){
        JLOGE("avfilter_graph_config ret:%d\n",ret);
        return ret;
    }

    return 0;
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

int JNIEXPORT PlayerEx::setSpeed(JNIEnv *env,jobject thiz,jint playerIndex,jfloat speed){
    return playerArray[playerIndex]->setSpeedReal(speed);
}

#ifdef __cplusplus
}
#endif