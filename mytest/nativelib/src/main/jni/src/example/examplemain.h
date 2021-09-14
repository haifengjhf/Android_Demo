#ifndef example_main_h
#define example_main_h

#ifdef __cplusplus
extern "C" {
#endif

#include <jni.h>

int example_of_decoding_encoding(JNIEnv *env,jobject thiz,jstring args1,jstring args2);

int example_of_demuxing_decoding(JNIEnv *env,jobject thiz,jstring args1,jstring args2,jstring args3,jstring args4);

int example_of_extract_mvs(JNIEnv *env,jobject thiz,jstring args1);

#ifdef __cplusplus
};
#endif

#endif