//
// Created by 金海峰 on 2021/5/26.
//

#include <string>
#include "FirstTest.h"


#ifdef __cplusplus
extern "C" {
#endif

#include "utils/LogUtils.h"
#include <jni.h>
#include "android/log.h"
#include "libavcodec/avcodec.h"

JNIEXPORT void FirstTest::firstTest(JNIEnv *env, jobject thiz) {
    native_print_d("firstTest %u", thiz);

    std::shared_ptr<JNIEnv> *sharedPtr = new std::shared_ptr<JNIEnv>(env);
    sharedPtr->get()->DeleteLocalRef(thiz);

//    std::shared_ptr<int> sp (new int);
//
//    std::weak_ptr<int> wp1;
//    std::weak_ptr<int> wp2 (wp1);
//    std::weak_ptr<int> wp3 (sp);

}


JNIEXPORT void FirstTest::ffmpegTest(JNIEnv *env, jobject thiz) {
    native_print_d("ffmpegTest %u", thiz);

    const std::string hello = avcodec_configuration();

    delete hello.c_str();
    native_print_d("format %s",hello.c_str());
}

#ifdef __cplusplus
}
#endif