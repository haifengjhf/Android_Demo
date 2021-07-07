//
// Created by 金海峰 on 2021/7/5.
//


#include "MyNativeWindow.h"
#include "android/native_window_jni.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <utils/LogUtils.h>
#include "libavutil/imgutils.h"

MyNativeWindow::~MyNativeWindow() {
    if(mNativeWindow){
        ANativeWindow_release(mNativeWindow);
        mNativeWindow = nullptr;
    }
}

int MyNativeWindow::setSurface(JNIEnv *env,jobject surface) {
    if(mNativeWindow != nullptr){
        ANativeWindow_release(mNativeWindow);
        mNativeWindow = nullptr;
    }
    mNativeWindow = ANativeWindow_fromSurface(env,surface);
    ANativeWindow_acquire(mNativeWindow);

    mWidth = ANativeWindow_getWidth(mNativeWindow);
    mHeight = ANativeWindow_getHeight(mNativeWindow);
    ANativeWindow_setBuffersGeometry(mNativeWindow,mWidth,mHeight,WINDOW_FORMAT_RGBA_8888);
    return 0;
}

int MyNativeWindow::renderRGBA(uint8_t **srcPixels, int *strideSize, int videoHeight, int bitPerPixel) {
    ANativeWindow_Buffer outBuffer;
    ANativeWindow_lock(mNativeWindow,&outBuffer,0);

    int min_height = outBuffer.height > videoHeight ? videoHeight:outBuffer.height;
    int dst_stride = outBuffer.stride;
    int src_line_size = strideSize[0];
    int dst_line_size = dst_stride * bitPerPixel / 8;

    uint8_t *dst_pixels = static_cast<uint8_t *>(outBuffer.bits);
    const uint8_t *src_pixels = srcPixels[0];

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

    ANativeWindow_unlockAndPost(mNativeWindow);
    return 0;
}

//int MyNativeWindow::renderRGBA(const uint8_t *pData, const int length){
//    ANativeWindow_Buffer outBuffer;
//    ANativeWindow_lock(mNativeWindow,&outBuffer,0);
//    memcpy(outBuffer.bits,pData,length);
//
//    ANativeWindow_unlockAndPost(mNativeWindow);
//    return 0;
//}

ANativeWindow_Buffer* MyNativeWindow::getWindowBuffer() {
    ANativeWindow_lock(mNativeWindow,&mWindowBuffer,0);
    return &mWindowBuffer;
}

void MyNativeWindow::render() {
    ANativeWindow_unlockAndPost(mNativeWindow);
}


int MyNativeWindow::getWidth() {
    return mWidth;
}

int MyNativeWindow::getHeight() {
    return mHeight;
}

#ifdef __cplusplus
}
#endif