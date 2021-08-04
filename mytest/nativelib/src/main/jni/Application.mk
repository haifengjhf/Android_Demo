
# Uncomment this if you're using STL in your project
# You can find more information here:
# https://developer.android.com/ndk/guides/cpp-support
# APP_STL := c++_shared

#APP_ABI := armeabi-v7a

# Min runtime API level
APP_PLATFORM = android-19

APP_STL += c++_shared

APP_CPPFLAGS += -std=c++11

APP_CFLAGS += -std=c99

#Adress Sanitize config
#APP_CPPFLAGS += -fsanitize=address -fno-omit-frame-pointer
#APP_CFLAGS += -fsanitize=address -fno-omit-frame-pointer
#APP_LDFLAGS += -fsanitize=address

#NDK_TOOLCHAIN := arm-linux-androideabi-4.9
#NDK_TOOLCHAIN_VERSION := clang