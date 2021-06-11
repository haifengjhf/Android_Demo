package com.jhf.nativelib.bridge;

import com.jhf.nativelib.R;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.View;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

public class NativeEntranceTestActivity extends AppCompatActivity {
    protected NativeEntrance mNativeEntrance;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_nativeentrancetest);
        mNativeEntrance = new NativeEntrance();
    }


    public void onConvertBtnClick(View view){
        String folderurl= this.getExternalCacheDir().getParent();
//        String folderurl=Environment.getExternalStorageDirectory().getPath();
//        String folderurl= "/sdcard/Android/data/com.jhf.test";
        String urltext_input="sintel.mp4";
        String inputurl=folderurl+"/"+urltext_input;

        String urltext_output="sintel.yuv";
        String outputurl=folderurl+"/"+urltext_output;

        Log.i("inputurl",inputurl);
        Log.i("outputurl",outputurl);

        mNativeEntrance.decodeToYUV(inputurl,outputurl);
    }
}
