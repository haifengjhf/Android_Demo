//
// Created by 金海峰 on 2021/5/26.
//

#ifndef MY_TEST_APPLICATION_LOGUTILS_H
#define MY_TEST_APPLICATION_LOGUTILS_H

#include <jni.h>
#include <android/log.h>

#ifdef __cpluscplus
extern "C"{
#endif

extern const char* TAG;

#include "stdarg.h"

#define JLOGV(...)  __android_log_print(ANDROID_LOG_VERBOSE,    TAG, __VA_ARGS__)
#define JLOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,      TAG, __VA_ARGS__)
#define JLOGI(...)  __android_log_print(ANDROID_LOG_INFO,       TAG, __VA_ARGS__)
#define JLOGW(...)  __android_log_print(ANDROID_LOG_WARN,       TAG, __VA_ARGS__)
#define JLOGE(...)  __android_log_print(ANDROID_LOG_ERROR,      TAG, __VA_ARGS__)

void native_write_d(const char *msg);

void native_write_w(const char *msg);

void native_write_e(const char *msg);

///////////////////////////////////

void native_print_d(const char *format, ...);

void native_print_w(const char *format, ...);

void native_print_e(const char *format, ...);


///////////////////////////////////

void custom_log(void *ptr, int level, const char *fmt, va_list vl);


#ifdef __cpluscplus
}
#endif
#endif //MY_TEST_APPLICATION_LOGUTILS_H
