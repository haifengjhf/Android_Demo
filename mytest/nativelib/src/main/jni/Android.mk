MY_LOCAL_PATH := $(call my-dir)

LOCAL_PATH := $(MY_LOCAL_PATH)

###########################
#
# native-lib shared library
#
###########################
LOCAL_PATH := $(MY_LOCAL_PATH)

include $(CLEAR_VARS)

LOCAL_MODULE := native-lib

LOCAL_C_INCLUDES := $(LOCAL_PATH)/src $(LOCAL_PATH)/src/utils $(LOCAL_PATH)/ffmpeg/include

LOCAL_SRC_FILES := src/NativelibMain.cpp \
                           src/FirstTest.cpp \
                           src/utils/LogUtils.c \
                           src/MyNativeWindow.cpp \
						   src/PlayerEx.cpp \
						   src/VideoScale.cpp \
                           src/Decoder.cpp


LOCAL_CPPFLAGS += -std=c++11

# Warnings we haven't fixed (yet)
#LOCAL_CFLAGS += -Wno-unused-parameter -Wno-sign-compare

$(warning "LOCAL_SHARED_LIBRARIES TARGET_ARCH_ABI $(TARGET_ARCH_ABI)")
ifeq ($(TARGET_ARCH_ABI),arm64)
	LOCAL_SHARED_LIBRARIES := libavcodec libavfilter libavformat libavutil libswresample libswscale libpostproc SDL2
else
	LOCAL_SHARED_LIBRARIES := libavcodec libavfilter libavformat libavutil libswresample libswscale libpostproc SDL2
endif

LOCAL_LDLIBS := -llog -landroid

ifeq ($(NDK_DEBUG),1)
    cmd-strip :=
endif

include $(BUILD_SHARED_LIBRARY)



###########################
#
# ffmpeg prebuild shared library
#
###########################
LOCAL_PATH := $(MY_LOCAL_PATH)
MY_LIB_PATH := ${LOCAL_PATH}/../libs/${TARGET_ARCH_ABI}

$(warning "the value of MY_LIB_PATH is $(MY_LIB_PATH)") #jni

include $(CLEAR_VARS)
LOCAL_MODULE := libavcodec
LOCAL_SRC_FILES = ${MY_LIB_PATH}/libavcodec.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libavfilter
LOCAL_SRC_FILES = ${MY_LIB_PATH}/libavfilter.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libavformat
LOCAL_SRC_FILES = ${MY_LIB_PATH}/libavformat.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libavutil
LOCAL_SRC_FILES = ${MY_LIB_PATH}/libavutil.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libswresample
LOCAL_SRC_FILES = ${MY_LIB_PATH}/libswresample.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libswscale
LOCAL_SRC_FILES = ${MY_LIB_PATH}/libswscale.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libpostproc
LOCAL_SRC_FILES = ${MY_LIB_PATH}/libpostproc.so
include $(PREBUILT_SHARED_LIBRARY)

###########################
#
# sdl2 shared library
#
###########################
LOCAL_PATH := $(MY_LOCAL_PATH)
include $(CLEAR_VARS)
#include $(call all-subdir-makefiles)
include ${LOCAL_PATH}/SDL2/Android.mk



##########################
#
# SDL main library
#
##########################

include $(CLEAR_VARS)
LOCAL_PATH := $(MY_LOCAL_PATH)
LOCAL_MODULE := main
LOCAL_CPPFLAGS += -std=c++11

SDL_PATH := ./SDL2
LOCAL_C_INCLUDES := $(LOCAL_PATH)/ffmpeg/include $(LOCAL_PATH)/$(SDL_PATH)/include $(LOCAL_PATH)/src $(LOCAL_PATH)/src/utils $(LOCAL_PATH)/src/usermain

# Add your application source files here...
LOCAL_SRC_FILES := $(LOCAL_PATH)/$(SDL_PATH)/src/main/android/SDL_android_main.c \
	$(LOCAL_PATH)/src/usermain/userplayer.c
#	$(LOCAL_PATH)/src/usermain/defaultAudio.c
#	$(LOCAL_PATH)/src/usermain/sdllesson.c
LOCAL_SHARED_LIBRARIES := native-lib libavcodec libavfilter libavformat libavutil libswresample libswscale libpostproc SDL2
LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog
include $(BUILD_SHARED_LIBRARY)

##########################
