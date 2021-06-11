//
// Created by 金海峰 on 2021/5/26.
//
#include "LogUtils.h"
#include "android/log.h"
#include "iostream"
#include "string"

const static std::string TAG = "nativeLib";


void native_write_d(const char* msg){
    __android_log_write(ANDROID_LOG_DEBUG,TAG.c_str(),msg);
}

void native_write_d(const char* tag, const char* msg){
    __android_log_write(ANDROID_LOG_DEBUG,tag,msg);
}

void native_print_d(const char* format,...){
    va_list argList;
    va_start(argList,format);
    char buf[1024]={0};
    vsnprintf(buf,1024,format,argList);
    va_end(argList);
    native_write_d(buf);
}

void native_print_e(const char* format,...){
    va_list argList;
    va_start(argList,format);
    char buf[1024]={0};
    vsnprintf(buf,1024,format,argList);
    va_end(argList);
    native_write_e(buf);
}


void native_write_w(const char* msg){
    __android_log_write(ANDROID_LOG_WARN,TAG.c_str(),msg);
}

void native_write_e(const char* msg){
    __android_log_write(ANDROID_LOG_ERROR,TAG.c_str(),msg);
}

void custom_log(void *ptr, int level, const char* fmt, va_list vl){
    FILE *fp=fopen("/storage/emulated/0/av_log.txt","a+");
    if(fp){
        vfprintf(fp,fmt,vl);
        fflush(fp);
        fclose(fp);
    }
}