package com.jhf.nativelib.bridge;

import android.media.AudioTrack;
import android.view.Surface;

public class NativePlayer {
    protected long mNativePlayerPtr;

    public native int play(String filePath);

    public native int setVideoSurface(Surface surface);

}
