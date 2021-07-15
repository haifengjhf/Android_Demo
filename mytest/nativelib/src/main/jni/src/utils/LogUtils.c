//
// Created by 金海峰 on 2021/5/26.
//
#ifdef __cpluscplus
extern "C"{
#endif
#include "stdio.h"
#include "LogUtils.h"
#include "android/log.h"

const char* TAG = "nativeLib";

void native_write_d(const char* msg){
    __android_log_write(ANDROID_LOG_DEBUG,TAG,msg);
}

//void native_write_d(const char* tag, const char* msg){
//    __android_log_write(ANDROID_LOG_DEBUG,tag,msg);
//}

void native_print_d(const char* format,...){
    va_list argList;
    va_start(argList,format);
    __android_log_vprint(ANDROID_LOG_DEBUG,TAG,format,argList);
    va_end(argList);
}

void native_print_w(const char *format, ...){
    va_list argList;
    va_start(argList,format);
    __android_log_vprint(ANDROID_LOG_WARN,TAG,format,argList);
    va_end(argList);
}

void native_print_e(const char* format,...){
    va_list argList;
    va_start(argList,format);
    __android_log_vprint(ANDROID_LOG_ERROR,TAG,format,argList);
    va_end(argList);
}


void native_write_w(const char* msg){
    __android_log_write(ANDROID_LOG_WARN,TAG,msg);
}

void native_write_e(const char* msg){
    __android_log_write(ANDROID_LOG_ERROR,TAG,msg);
}

void custom_log(void *ptr, int level, const char* fmt, va_list vl){
    __android_log_vprint(ANDROID_LOG_WARN,TAG,fmt,vl);
}

#ifdef __cpluscplus
extern "C"{
#endif