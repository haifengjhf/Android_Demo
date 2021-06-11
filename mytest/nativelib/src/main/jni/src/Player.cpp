//
// Created by 金海峰 on 2021/6/10.
//

#include "Player.h"
#include "LogUtils.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/log.h"
#include "libavutil/imgutils.h"


int Player::playInner(const char *filePath) {

}

int Player::play(JNIEnv *env, jobject thiz, jstring filePath) {
    const char *pFilePath = env->GetStringUTFChars(filePath, 0);
    int result = playInner(pFilePath);

    env->ReleaseStringUTFChars(filePath, pFilePath);
    return result;
}

#ifdef __cplusplus
}
#endif