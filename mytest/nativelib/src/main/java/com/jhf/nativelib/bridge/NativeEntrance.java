package com.jhf.nativelib.bridge;


import android.view.Surface;

public class NativeEntrance {
    public void init(){
        System.loadLibrary("native-lib");
    }

    public native void test();

    public native void ffmpegTest();

    public native int decodeToYUV(String filePath,String outFilePath);

    public native int play(String filePath);

    public native int setVideoSurface(Surface surface);
}
