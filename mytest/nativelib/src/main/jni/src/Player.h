//
// Created by 金海峰 on 2021/6/10.
//

#ifndef MY_TEST_APPLICATION_PLAYER_H
#define MY_TEST_APPLICATION_PLAYER_H


#ifdef __cplusplus
extern "C" {
#endif

#include "../../../../../../../../../../../sdk/android-ndk-r21b/toolchains/llvm/prebuilt/darwin-x86_64/sysroot/usr/include/jni.h"

class Player {
public:
    static int JNIEXPORT play(JNIEnv *env,jobject thiz,jstring filePath);

protected:
    static int playInner(const char* filePath);
};

#ifdef __cplusplus
}
#endif

#endif //MY_TEST_APPLICATION_PLAYER_H
