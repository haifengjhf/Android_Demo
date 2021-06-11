package com.jhf.nativelib.bridge;



public class NativeEntrance {
    public void init(){
        System.loadLibrary("native-lib");
    }

    public native void test();

    public native void ffmpegTest();

    public native int decodeToYUV(String filePath,String outFilePath);

    public native int play(String filePath);
}
