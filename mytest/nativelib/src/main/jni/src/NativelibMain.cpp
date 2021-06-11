//
// Created by 金海峰 on 2021/5/25.
//
#ifndef MY_TEST_APPLICATION_NATIVELIBMAIN_CPP
#define MY_TEST_APPLICATION_NATIVELIBMAIN_CPP


#include <jni.h>
#include <stddef.h>
#include <android/log.h>
#include "LogUtils.h"
#include "FirstTest.h"
#include "Decoder.h"


JNINativeMethod jniNativeMethod[] = {
        {
                "test",
                "()V",
                (void *) &FirstTest::firstTest
        },
        {
                "ffmpegTest",
                "()V",
                (void *) &FirstTest::ffmpegTest
        },
        {
            "decodeToYUV",
                "(Ljava/lang/String;Ljava/lang/String;)I",
            (void *) &Decoder::decodeToYUV
        }
};

jint dynamicReg(JNIEnv *env) {
    //获取对应声明native方法的Java类
    jint result = JNI_FALSE;
    jclass clazz = env->FindClass("com/jhf/nativelib/bridge/NativeEntrance");
    if (clazz == NULL) {
        return result;
    }
    //注册方法，成功返回正确的JNIVERSION。
    if (env->RegisterNatives(clazz, jniNativeMethod,
                             sizeof(jniNativeMethod) / sizeof(jniNativeMethod[0])) == JNI_OK) {
        result = JNI_VERSION_1_4;
    }

    env->DeleteLocalRef(clazz);
    return result;
}


JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    //OnLoad方法是没有JNIEnv参数的，需要通过vm获取。
    native_write_d("JNI_OnLoad");
    jint result;
    JNIEnv *env = NULL;
    if (vm->AttachCurrentThread(&env, NULL) == JNI_OK) {
        result = dynamicReg(env);
    }

    native_write_d( "JNI_OnUnload");
    return result;
}


JNIEXPORT void JNI_OnUnload(JavaVM *vm, void *reserved) {
    native_write_d("JNI_OnUnload");
}

#endif