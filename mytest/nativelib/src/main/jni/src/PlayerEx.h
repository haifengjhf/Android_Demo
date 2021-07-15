//
// Created by 金海峰 on 2021/7/5.
//

#ifndef MY_TEST_APPLICATION_PLAYEREX_H
#define MY_TEST_APPLICATION_PLAYEREX_H
#include "jni.h"
#include "MyNativeWindow.h"
#include "AudioPlayer.h"

class PlayerEx {
public:
    PlayerEx(MyNativeWindow* myNativeWindow);

    static int JNIEXPORT play(JNIEnv *env,jobject thiz,jstring filePath);

    static int JNIEXPORT setVideoSurface(JNIEnv *env,jobject thiz,jobject surface);

    int playInner(const char* filePath);

protected:
    int android_render_rgb_on_rgb(ANativeWindow_Buffer *out_buffer, unsigned char **pixels ,int pitches[],int h, int bpp);

protected:
    MyNativeWindow *mNativeWindow;
    AudioPlayer mAudioPlayer;
};


#endif //MY_TEST_APPLICATION_PLAYEREX_H
