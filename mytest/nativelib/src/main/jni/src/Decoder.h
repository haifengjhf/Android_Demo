//
// Created by 金海峰 on 2021/6/10.
//

#ifndef MY_TEST_APPLICATION_DECODER_H
#define MY_TEST_APPLICATION_DECODER_H

#include "string"


#ifdef __cplusplus
extern "C" {
#endif


#include "jni.h"



class Decoder {
public:
    static JNIEXPORT jint decodeToYUV(JNIEnv *env, jobject thiz, jstring filePath, jstring outFilePath);

protected:
    static int decodeToYUVInner(const char *pFilePath, const char *pOutPath);
};

#ifdef __cplusplus
}
#endif

#endif //MY_TEST_APPLICATION_DECODER_H
