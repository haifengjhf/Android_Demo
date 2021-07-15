//
// Created by 金海峰 on 2021/7/12.
//

#ifndef MY_TEST_APPLICATION_JNIENV_H
#define MY_TEST_APPLICATION_JNIENV_H
#include <jni.h>
#include <android/log.h>
#include <unistd.h>
#include <pthread.h>

class JniEnv{
public:
    JniEnv(JavaVM *javaVm):mJniEnv(nullptr){
        mJavaVm = javaVm;
    }

    ~JniEnv(){
        mJniEnv = nullptr;
    }

    JNIEnv* getJniEnv(){
        pthread_once(&g_key_once, make_thread_key);

        JNIEnv *env = (JNIEnv*) pthread_getspecific(g_thread_key);
        if (env) {
            mJniEnv = env;
            return mJniEnv;
        }

        if(mJavaVm == nullptr){
            __android_log_print(ANDROID_LOG_ERROR,"JniEnv"," javaVm NULL Pointer Error");
            return mJniEnv;
        }

        int result = mJavaVm->AttachCurrentThread(&env,NULL);
        if (result == JNI_OK || result == JNI_EEXIST) {
            pthread_setspecific(g_thread_key, env);
            mJniEnv = env;
        }

        return mJniEnv;
    };

    static void make_thread_key()
    {
        pthread_key_create(&g_thread_key, destroy_thread_key);
    }

    static void destroy_thread_key(void* value)
    {
        JNIEnv *env = (JNIEnv*) value;
        if (env != nullptr && mJavaVm != nullptr) {
            pthread_setspecific(g_thread_key, NULL);
            mJavaVm->DetachCurrentThread();
        }
        pthread_key_delete(g_thread_key);
    }

protected:
    JNIEnv *mJniEnv;

    static JavaVM *mJavaVm;
    static pthread_key_t g_thread_key;
    static pthread_once_t g_key_once;;
};

JavaVM* JniEnv::mJavaVm = nullptr;
pthread_once_t JniEnv::g_thread_key;
pthread_once_t JniEnv::g_key_once = PTHREAD_ONCE_INIT;

#endif //MY_TEST_APPLICATION_JNIENV_H
