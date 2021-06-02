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

LOCAL_C_INCLUDES := $(LOCAL_PATH)/src/utils $(LOCAL_PATH)/ffmpeg/include

LOCAL_SRC_FILES := src/NativelibMain.cpp \
                           src/FirstTest.cpp \
                           src/utils/LogUtils.cpp

# Warnings we haven't fixed (yet)
LOCAL_CFLAGS += -Wno-unused-parameter -Wno-sign-compare

LOCAL_SHARED_LIBRARIES := libavcodec libavfilter libavformat libavutil libswresample libswscale

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

include $(CLEAR_VARS)
LOCAL_MODULE := libavcodec
LOCAL_SRC_FILES = ${LOCAL_PATH}/../libs/arm64-v8a/libavcodec.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libavfilter
LOCAL_SRC_FILES = ${LOCAL_PATH}/../libs/arm64-v8a/libavfilter.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libavformat
LOCAL_SRC_FILES = ${LOCAL_PATH}/../libs/arm64-v8a/libavformat.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libavutil
LOCAL_SRC_FILES = ${LOCAL_PATH}/../libs/arm64-v8a/libavutil.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libswresample
LOCAL_SRC_FILES = ${LOCAL_PATH}/../libs/arm64-v8a/libswresample.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libswscale
LOCAL_SRC_FILES = ${LOCAL_PATH}/../libs/arm64-v8a/libswscale.so
include $(PREBUILT_SHARED_LIBRARY)


###########################
#
# sdl2 shared library
#
###########################
LOCAL_PATH := $(MY_LOCAL_PATH)

include ${LOCAL_PATH}/SDL2/Android.mk