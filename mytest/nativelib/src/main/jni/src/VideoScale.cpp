//
// Created by 金海峰 on 2021/7/5.
//

#include "VideoScale.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "libavutil/imgutils.h"
#include "libavutil/frame.h"

VideoScale::VideoScale(): mSwsContext(nullptr), mVideoBuffer(nullptr){

}

VideoScale::~VideoScale(){
    if(mSwsContext){
        sws_freeContext(mSwsContext);
        mSwsContext = nullptr;
    }

    if(mVideoBuffer){
        av_free(mVideoBuffer);
        mVideoBuffer = nullptr;
    }
}

int VideoScale::scale(AVFrame *pFrame) {
    int linesize[8] = {pFrame->linesize[0] * 4};
    sws_scale(mSwsContext, pFrame->data, pFrame->linesize, 0, pFrame->height,
              reinterpret_cast<unsigned char *const *>(mVideoBuffer), linesize);
    return 1;
}

int VideoScale::setVideoGeometry(int videoWidth, int videoHeight){
    if(mVideoWidth != videoWidth || mVideoHeight != videoHeight){
        mVideoWidth = videoWidth;
        mVideoHeight = videoHeight;

        if(mSwsContext){
            sws_freeContext(mSwsContext);
        }

        if(mVideoBuffer){
            av_free(mVideoBuffer);
        }
    }

//    mSwsContext = sws_getContext(mVideoWidth,mVideoHeight,AV_PIX_FMT_YUV420P
//            ,mVideoWidth, mVideoHeight, AV_PIX_FMT_RGBA
//            , SWS_BICUBIC, NULL, NULL, NULL);

    mVideoBufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGBA, mVideoWidth, mVideoHeight, 1);
//    mVideoBuffer = static_cast<unsigned char *>(malloc(mVideoBufferSize));
    av_malloc(mVideoBufferSize);

    return 1;
}



#ifdef __cplusplus
}
#endif