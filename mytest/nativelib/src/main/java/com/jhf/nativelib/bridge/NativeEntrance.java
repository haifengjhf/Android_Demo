package com.jhf.nativelib.bridge;


import android.view.Surface;

public class NativeEntrance {
    public void init(){
        System.loadLibrary("native-lib");
    }

    public native void test();

    public native void ffmpegTest();

    public native int decodeToYUV(String filePath,String outFilePath);

    public native void initPlayer();

    public native int play(int index,String filePath);

    public native int setVideoSurface(int index,Surface surface);

    public native int seek(int index,long timeInMilSecond);

    public native int setSpeed(int index,float speed);

    public native int exampelEncodingAndDecoding(String arg1,String args2);

    public native int exampelDemuxingAndDecoding(String arg1,String args2,String arg3,String arg4);

    public native int exampelExtractMVS(String arg1);

    public native int translate(String inFilePath,String outFilePath);

}
