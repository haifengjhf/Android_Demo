//
// Created by 金海峰 on 2021/7/5.
//

#ifndef MY_TEST_APPLICATION_PLAYEREX_H
#define MY_TEST_APPLICATION_PLAYEREX_H


#include "jni.h"
#include "MyNativeWindow.h"
#include "AudioPlayer.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "packet_queue.h"
#include "libswscale/swscale.h"
#include <libswresample/swresample.h>
#include <libavformat/avformat.h>


class PlayerEx {
public:
    PlayerEx(MyNativeWindow* myNativeWindow);

    ~PlayerEx();

    static int JNIEXPORT play(JNIEnv *env,jobject thiz,jint playerIndex,jstring filePath);

    static int JNIEXPORT setVideoSurface(JNIEnv *env,jobject thiz,jint playerIndex,jobject surface);

    static int JNIEXPORT seek(JNIEnv *env,jobject thiz,jint playerIndex,jlong timeInMilSecond);

    static void JNIEXPORT initPlayer(JNIEnv *env,jobject thiz);

    int playInner(const char* filePath);

    int seekInner(long timeInMilSecond);

    MyNativeWindow* getNativeWindow(){
        return mNativeWindow;
    };

protected:
    int android_render_rgb_on_rgb(ANativeWindow_Buffer *out_buffer, unsigned char **pixels ,int pitches[],int h, int bpp);

    static void* audio_thread(void *arg);

    static void* video_thread(void *arg);

    void audioTimeSync(AVFrame* pFrame);

    void videoTimeSync(AVFrame* pFrame);

protected:
    MyNativeWindow *mNativeWindow;
    AudioPlayer mAudioPlayer;
    bool mNextIVideo;
    long mVideoSeekTimeInMilsecond;
    long mAudioSeekTimeInMilsecond;
    bool mStop;

    PacketQueue mVideoPacketQueue;
    PacketQueue mAudioPacketQueue;

    AVFormatContext *mFormatContext;
    AVCodecContext* pVideoCodecContext;
    AVCodecContext* pAudioCodecContext;
    SwsContext* pSwsContext;
    SwrContext* pSwrContext;
    int mVideoIndex;
    int mAudioIndex;

    AVPixelFormat dstFormat = AV_PIX_FMT_RGBA;
    int channel_layout = AV_CH_LAYOUT_STEREO;
    AVSampleFormat outSampleFormat = AV_SAMPLE_FMT_S16;

    //视频同步音频,s
    double mCurSyncClock;

    //audio
    long mAudioLastPts;
    double mAudioLastPlayClock;

    //video
    long mVideoLastPts;
    double mVideoLastPlayClock;
};

#ifdef __cplusplus
};
#endif

#endif //MY_TEST_APPLICATION_PLAYEREX_H
