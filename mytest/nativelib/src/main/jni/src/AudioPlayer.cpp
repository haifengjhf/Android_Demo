//
// Created by 金海峰 on 2021/7/9.
//

#include <malloc.h>
#include "AudioPlayer.h"

enum ChannelConfig {
    CHANNEL_OUT_INVALID = 0x0,
    CHANNEL_OUT_DEFAULT = 0x1, /* f-l */
    CHANNEL_OUT_MONO = 0x4, /* f-l, f-r */
    CHANNEL_OUT_STEREO = 0xc, /* f-l, f-r, b-l, b-r */
    CHANNEL_OUT_QUAD = 0xcc, /* f-l, f-r, b-l, b-r */
    CHANNEL_OUT_SURROUND = 0x41c, /* f-l, f-r, f-c, b-c */
    CHANNEL_OUT_5POINT1 = 0xfc, /* f-l, f-r, b-l, b-r, f-c, low */
    CHANNEL_OUT_7POINT1 = 0x3fc, /* f-l, f-r, b-l, b-r, f-c, low, f-lc, f-rc */

    CHANNEL_OUT_FRONT_LEFT = 0x4,
    CHANNEL_OUT_FRONT_RIGHT = 0x8,
    CHANNEL_OUT_BACK_LEFT = 0x40,
    CHANNEL_OUT_BACK_RIGHT = 0x80,
    CHANNEL_OUT_FRONT_CENTER = 0x10,
    CHANNEL_OUT_LOW_FREQUENCY = 0x20,
    CHANNEL_OUT_FRONT_LEFT_OF_CENTER = 0x100,
    CHANNEL_OUT_FRONT_RIGHT_OF_CENTER = 0x200,
    CHANNEL_OUT_BACK_CENTER = 0x400,
} channel_config;

enum AudioFormat {
    ENCODING_INVALID = 0,
    ENCODING_DEFAULT = 1,
    ENCODING_PCM_16BIT = 2, // signed, guaranteed to be supported by devices.
    ENCODING_PCM_8BIT = 3, // unsigned, not guaranteed to be supported by devices.
    ENCODING_PCM_FLOAT = 4, // single-precision floating-point per sample
} audio_format;

enum StreamType {
    STREAM_VOICE_CALL = 0,
    STREAM_SYSTEM = 1,
    STREAM_RING = 2,
    STREAM_MUSIC = 3,
    STREAM_ALARM = 4,
    STREAM_NOTIFICATION = 5,
} stream_type;

enum Mode {
    MODE_STATIC = 0,
    MODE_STREAM = 1,
} mode;

#ifdef __cplusplus
extern "C" {
#endif

#include <j4a/j4a_allclasses.h>
#include <j4au/class/android/media/AudioTrack.util.h>
#include "j4a/class/android/media/AudioTrack.h"
#include "j4a/class/android/os/Build.h"
#include "JniEnv.h"
#include "AndroidJni.h"
#include "LogUtils.h"

AudioPlayer::AudioPlayer() {
    mAudioChunkQueue = createAudioChunkQueue(50);
    mAudioByteOffset = 0;
}

AudioPlayer::~AudioPlayer() {
    freeAudioChunkQueue(&mAudioChunkQueue);
}

void AudioPlayer::releaseAudio(){
    JLOGE("AudioPlayer releaseAudio");
    JniEnv jniEnv(JNI_GetJvm());

    J4A_DeleteGlobalRef__p(jniEnv.getJniEnv(), reinterpret_cast<jobject *>(&mAudioByteArray));

    J4AC_android_media_AudioTrack__release__catchAll(jniEnv.getJniEnv(),(jobject)mAudioTrack);
}

int AudioPlayer::createAudio() {
    JLOGE("AudioPlayer createAudio");
    JniEnv jniEnv(JNI_GetJvm());

    J4A_loadClass__J4AC_android_os_Build(jniEnv.getJniEnv());
    J4A_loadClass__J4AC_android_media_AudioTrack(jniEnv.getJniEnv());
    J4A_loadClass__J4AC_android_media_PlaybackParams(jniEnv.getJniEnv());

    int audioFormat = ENCODING_PCM_16BIT;
    int channelConfig = CHANNEL_OUT_STEREO;
    mMinBufferSize = J4AC_AudioTrack__getMinBufferSize__catchAll(jniEnv.getJniEnv(),
                                                                 DEFAULT_SAMPLE_RATE,
                                                                 channelConfig,
                                                                 audioFormat);
    mMinBufferSize *= 2;//支持最大 2倍速

    mAudioTrack = J4AC_android_media_AudioTrack__AudioTrack__catchAll(jniEnv.getJniEnv(),STREAM_MUSIC
            ,DEFAULT_SAMPLE_RATE
            ,channelConfig
            ,audioFormat
            ,mMinBufferSize
            ,MODE_STREAM);

    int audioSessionId = J4AC_AudioTrack__getAudioSessionId__catchAll(jniEnv.getJniEnv(), mAudioTrack);

    mAudioByteArray = J4A_NewByteArray__asGlobalRef__catchAll(jniEnv.getJniEnv()
            ,mMinBufferSize);

    jboolean  isCopy = JNI_FALSE;
    mPAudioBuffer = jniEnv.getJniEnv()->GetByteArrayElements(mAudioByteArray,&isCopy);

    J4AC_android_media_AudioTrack__play(jniEnv.getJniEnv(),mAudioTrack);

    return 1;
}

int AudioPlayer::addAudioBuffer(uint8_t *buffer, int len){
    AudioChunk* audioChunk = (AudioChunk *)malloc(sizeof(AudioChunk));
    audioChunk->size = len;
    audioChunk->data = (uint8_t*)malloc(len);
    memcpy(audioChunk->data,buffer,len);

    insertAudioChunk(audioChunk,mAudioChunkQueue);

    return 1;
}

void AudioPlayer::flushDirectEx(uint8_t *buffer, int len) {
    //work no noise
    JniEnv jniEnv(JNI_GetJvm());

    if(len > mMinBufferSize){
        len = mMinBufferSize;
    }
    jniEnv.getJniEnv()->SetByteArrayRegion(mAudioByteArray,0,len,(jbyte*)buffer);
//    memcpy(mPAudioBuffer,buffer,len);
//    jniEnv.getJniEnv()->ReleaseByteArrayElements(mAudioByteArray,mPAudioBuffer,JNI_COMMIT);

    J4AC_android_media_AudioTrack__write__catchAll(jniEnv.getJniEnv(),(jobject)mAudioTrack,mAudioByteArray,0,len);
}

void AudioPlayer::flushDirect(uint8_t *buffer, int len) {
    //work no noise
    JniEnv jniEnv(JNI_GetJvm());

    if(len > mMinBufferSize){
        len = mMinBufferSize;
    }

    memcpy(mPAudioBuffer,buffer,len);
    jniEnv.getJniEnv()->ReleaseByteArrayElements(mAudioByteArray,mPAudioBuffer,JNI_COMMIT);

    J4AC_android_media_AudioTrack__write__catchAll(jniEnv.getJniEnv(),(jobject)mAudioTrack,mAudioByteArray,0,len);
}

void AudioPlayer::flush(uint8_t *buffer, int len) {
    JniEnv jniEnv(JNI_GetJvm());
    if(len >= mMinBufferSize - mAudioByteOffset){
        int writeOff = 0;
        while(len > writeOff){
            int writeSize = len - writeOff;
            if(writeSize > mMinBufferSize - mAudioByteOffset){
                writeSize = mMinBufferSize - mAudioByteOffset;
            }

            jniEnv.getJniEnv()->SetByteArrayRegion(mAudioByteArray,mAudioByteOffset,writeSize,(jbyte*)buffer + writeOff);
            mAudioByteOffset += writeSize;

            if(mAudioByteOffset >= mMinBufferSize){
                J4AC_android_media_AudioTrack__write__catchAll(jniEnv.getJniEnv(),(jobject)mAudioTrack,mAudioByteArray,0,writeSize);
            }

            if(mAudioByteOffset >= mMinBufferSize){
                mAudioByteOffset -= mMinBufferSize;
            }
            writeOff += writeSize;
        }
    }else{
        jniEnv.getJniEnv()->SetByteArrayRegion(mAudioByteArray,mAudioByteOffset,len,(jbyte*)buffer);
        mAudioByteOffset += len;
    }
}

int AudioPlayer::writeBuffer(uint8_t *buffer, int len){
    AudioChunk* audioChunk;
    int cacheSize = 0;
    while (cacheSize < len){
        int result = tryGetNextAudioChunk(&audioChunk,mAudioChunkQueue,len - cacheSize);
        if(result == 0){
            JLOGE("writeBuffer mAudioChunkQueue 0");
            memset(buffer + cacheSize,0,len - cacheSize);
            break;
        }

        memcpy(buffer,audioChunk->data,audioChunk->size);
        cacheSize += audioChunk->size;
    }

    return 1;
}

void AudioPlayer::setSpeed(float speed){
    JniEnv jniEnv(JNI_GetJvm());
    J4AC_android_media_AudioTrack__setSpeed(jniEnv.getJniEnv(),mAudioTrack,speed);
}

#ifdef __cplusplus
}
#endif