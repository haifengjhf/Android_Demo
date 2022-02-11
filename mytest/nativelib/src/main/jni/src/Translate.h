//
// Created by 金海峰 on 2021/10/11.
//

#ifndef MY_TEST_APPLICATION_TRANSLATE_H
#define MY_TEST_APPLICATION_TRANSLATE_H
#include "jni.h"


#ifdef __cplusplus
extern "C" {
#endif

class Translate {
public:
    static int JNIEXPORT translate(JNIEnv *env, jobject thiz, jstring filePath,jstring outPath);
};


#ifdef __cplusplus
}
#endif

#endif //MY_TEST_APPLICATION_TRANSLATE_H
