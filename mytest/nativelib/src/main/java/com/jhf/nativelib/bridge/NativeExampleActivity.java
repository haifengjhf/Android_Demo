package com.jhf.nativelib.bridge;

import com.jhf.nativelib.R;

import android.os.Bundle;
import android.view.View;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

public class NativeExampleActivity extends AppCompatActivity {
    protected NativeEntrance mNativeEntrance;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_native_example);

        mNativeEntrance = new NativeEntrance();
    }

    public void onVideoEncodeH264(View view){
        mNativeEntrance.exampelEncodingAndDecoding("123","h264");
    }

    public void onVideoEncodeDecodeMpg(View view){
        mNativeEntrance.exampelEncodingAndDecoding("123","mpg");
    }

    public void onDemuxingAndDecoding(View view){
        mNativeEntrance.exampelDemuxingAndDecoding("-refcount=new_norefcount","/sdcard/Android/data/com.jhf.test/sintel.mp4",
                "/sdcard/Android/data/com.jhf.test/sintel.video","/sdcard/Android/data/com.jhf.test/sintel.audio");
    }
}
