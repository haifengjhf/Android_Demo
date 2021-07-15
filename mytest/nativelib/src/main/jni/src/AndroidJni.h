#ifndef NATIVE_GLOBAL_H
#define NATIVE_GLOBAL_H

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

JavaVM *JNI_GetJvm();

#ifdef __cplusplus
}
#endif

#endif