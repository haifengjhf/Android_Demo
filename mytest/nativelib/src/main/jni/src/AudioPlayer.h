//
// Created by 金海峰 on 2021/7/9.
//

#ifndef MY_TEST_APPLICATION_AUDIOPLAYER_H
#define MY_TEST_APPLICATION_AUDIOPLAYER_H

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "audiochunk.h"

#define DEFAULT_SAMPLE_RATE 44100
#define DEFAULT_CHANNEL_SIZE 2

class AudioPlayer {
public:
    AudioPlayer();
    ~AudioPlayer();

    int createAudio();

    void releaseAudio();

    int addAudioBuffer(uint8_t *buffer, int len);

    int writeBuffer(uint8_t *buffer, int len);

    void flush(uint8_t *buffer, int len);

    void flushDirect(uint8_t *buffer, int len);

    void flushDirectEx(uint8_t *buffer, int len);

protected:
    jobject mAudioTrack;
    int mMinBufferSize;
    jbyteArray mAudioByteArray;
    jbyte* mPAudioBuffer;
    int mAudioByteOffset;
    AudioChunkQueue *mAudioChunkQueue;
};

#ifdef __cplusplus
}
#endif
#endif //MY_TEST_APPLICATION_AUDIOPLAYER_H
