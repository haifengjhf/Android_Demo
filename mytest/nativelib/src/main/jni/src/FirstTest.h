//
// Created by 金海峰 on 2021/5/26.
//

#ifndef MY_TEST_APPLICATION_FIRSTTEST_H
#define MY_TEST_APPLICATION_FIRSTTEST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <jni.h>

class FirstTest{
public:
    static JNIEXPORT void firstTest(JNIEnv *env, jobject thiz);

    static JNIEXPORT void ffmpegTest(JNIEnv *env, jobject thiz);
};




#ifdef __cplusplus
}
#endif
#endif //MY_TEST_APPLICATION_FIRSTTEST_H
