package com.jhf.test.app;

import com.jhf.nativelib.bridge.NativeEntrance;

import android.webkit.JavascriptInterface;
import android.webkit.WebView;
import android.webkit.WebViewClient;

import androidx.multidex.MultiDexApplication;

public class DemoAPP extends MultiDexApplication {
    @JavascriptInterface
    @Override
    public void onCreate() {
        super.onCreate();

        NativeEntrance nativeEntrance = new NativeEntrance();
        nativeEntrance.init();
        nativeEntrance.test();

        nativeEntrance.ffmpegTest();
    }
}
