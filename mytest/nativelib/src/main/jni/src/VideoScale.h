//
// Created by 金海峰 on 2021/7/5.
//

#ifndef MY_TEST_APPLICATION_VIDEOSCALE_H
#define MY_TEST_APPLICATION_VIDEOSCALE_H


#ifdef __cplusplus
extern "C" {
#endif

#include "libswscale/swscale.h"
#include "libavutil/frame.h"

class VideoScale {
public:
    VideoScale();
    ~VideoScale();

    int scale(AVFrame	*pFrame);

    int setVideoGeometry(int videoWidth,int videoHeight);

    unsigned char* getBuffer(){
        return mVideoBuffer;
    }

    int getBufferSize(){
        return mVideoBufferSize;
    }

protected:
    SwsContext *mSwsContext;
    unsigned char *mVideoBuffer;
    int mVideoBufferSize;
    int mVideoWidth;
    int mVideoHeight;
};

#ifdef __cplusplus
}
#endif

#endif //MY_TEST_APPLICATION_VIDEOSCALE_H
