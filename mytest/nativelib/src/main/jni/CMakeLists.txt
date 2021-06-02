cmake_minimum_required(VERSION 3.4.1)

MESSAGE(STATUS "print ${PROJECT_SOURCE_DIR}/../libs/${ANDROID_ABI}")

add_subdirectory(SDL2)

set(FFMPEG_LIB_PATH ${PROJECT_SOURCE_DIR}/../libs/${ANDROID_ABI})

include_directories(src src/utils)
include_directories(src ffmpeg/include)


set(LIB_SRC src/NativelibMain.cpp
        src/FirstTest.cpp
        src/utils/LogUtils.cpp
        )

add_library(native-lib SHARED
        ${LIB_SRC})


#导入FFmpeg的库
add_library(libavcodec SHARED IMPORTED)
#so文件的位置
set_target_properties(libavcodec
        PROPERTIES IMPORTED_LOCATION
        ${FFMPEG_LIB_PATH}/libavcodec.so)

add_library(libavfilter SHARED IMPORTED)
set_target_properties(libavfilter
        PROPERTIES IMPORTED_LOCATION
        ${FFMPEG_LIB_PATH}/libavfilter.so)

add_library(libavformat SHARED IMPORTED)
set_target_properties(libavformat
        PROPERTIES IMPORTED_LOCATION
        ${FFMPEG_LIB_PATH}/libavformat.so)

add_library(libavutil SHARED IMPORTED)
set_target_properties(libavutil
        PROPERTIES IMPORTED_LOCATION
        ${FFMPEG_LIB_PATH}/libavutil.so)

add_library(libswresample SHARED IMPORTED)
set_target_properties(libswresample
        PROPERTIES IMPORTED_LOCATION
        ${FFMPEG_LIB_PATH}/libswresample.so)

add_library(libswscale SHARED IMPORTED)
set_target_properties(libswscale
        PROPERTIES IMPORTED_LOCATION
        ${FFMPEG_LIB_PATH}/libswscale.so)


# Include libraries needed for hello-jni lib
target_link_libraries(native-lib
                      android
                      log
        libavcodec
        libavfilter
        libavformat
        libavutil
        libswresample
        libswscale
        )


set_target_properties(native-lib PROPERTIES LINKER_LANGUAGE CXX)