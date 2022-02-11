package com.android.webrtc;

import android.app.Application;
import android.util.Log;

public class App extends Application {
    private final static String TAG = "App";

    @Override
    public void onCreate() {
        super.onCreate();

        try{
            System.setProperty("java.net.preferIPv4Stack", "true");
        }catch (Exception e){
            Log.e(TAG,"onCreate setProperty preferIPv4Stack e:" + e.toString());
        }

    }
}
