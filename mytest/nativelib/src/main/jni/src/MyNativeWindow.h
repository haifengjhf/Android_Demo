//
// Created by 金海峰 on 2021/7/5.
//

#ifndef MY_TEST_APPLICATION_MYNATIVEWINDOW_H
#define MY_TEST_APPLICATION_MYNATIVEWINDOW_H
#include "jni.h"
#include "android/native_window.h"

#ifdef __cplusplus
extern "C" {
#endif

class MyNativeWindow {
public:
    virtual ~MyNativeWindow();

    int setSurface(JNIEnv *env,jobject surface);

    int renderRGBA(uint8_t * srcData[], int strideSize[],int videoHeight,int bitPerPixel);

    ANativeWindow_Buffer *getWindowBuffer();
    void render();

    int getWidth();
    int getHeight();


protected:
    ANativeWindow  *mNativeWindow;
    ANativeWindow_Buffer mWindowBuffer;;
    int mWidth;
    int mHeight;
};


#ifdef __cplusplus
}
#endif

#endif //MY_TEST_APPLICATION_MYNATIVEWINDOW_H
