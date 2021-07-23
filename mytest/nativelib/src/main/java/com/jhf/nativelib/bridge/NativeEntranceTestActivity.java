package com.jhf.nativelib.bridge;

import com.jhf.nativelib.R;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

public class NativeEntranceTestActivity extends AppCompatActivity {
    protected NativeEntrance mNativeEntrance;
    SurfaceHolder mSurfaceHolder1;
    SurfaceHolder mSurfaceHolder2;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_nativeentrancetest);
        mNativeEntrance = new NativeEntrance();
        mNativeEntrance.initPlayer();

        SurfaceView videoView1 = (SurfaceView)findViewById(R.id.video_view1);
        SurfaceHolder surfaceHolder1 = videoView1.getHolder();
        surfaceHolder1.addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(@NonNull SurfaceHolder holder) {
                mSurfaceHolder1 = holder;
            }

            @Override
            public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width, int height) {

            }

            @Override
            public void surfaceDestroyed(@NonNull SurfaceHolder holder) {

            }
        });

        SurfaceView videoView2 = (SurfaceView)findViewById(R.id.video_view2);
        SurfaceHolder surfaceHolder2 = videoView2.getHolder();
        surfaceHolder2.addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(@NonNull SurfaceHolder holder) {
                mSurfaceHolder2 = holder;
            }

            @Override
            public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width, int height) {

            }

            @Override
            public void surfaceDestroyed(@NonNull SurfaceHolder holder) {

            }
        });
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

    public void onPlayBtn1Click(View view){
        String folderurl= this.getExternalCacheDir().getParent();
        String urltext_input="sintel.mp4";
        String inputurl=folderurl+"/"+urltext_input;

        String urltext_output="sintel.yuv";
        String outputurl=folderurl+"/"+urltext_output;

        Log.i("inputurl",inputurl);
        Log.i("outputurl",outputurl);

        new Thread(new Runnable() {
            @Override
            public void run() {
                mNativeEntrance.setVideoSurface(0,mSurfaceHolder1.getSurface());
                mNativeEntrance.play(0,inputurl);
            }
        }).start();
    }

    public void onPlayBtn2Click(View view){
        String folderurl= this.getExternalCacheDir().getParent();
        String urltext_input="sintel.mp4";
        String inputurl=folderurl+"/"+urltext_input;

        String urltext_output="sintel.yuv";
        String outputurl=folderurl+"/"+urltext_output;

        Log.i("inputurl",inputurl);
        Log.i("outputurl",outputurl);


        new Thread(new Runnable() {
            @Override
            public void run() {
                mNativeEntrance.setVideoSurface(0,mSurfaceHolder1.getSurface());
                mNativeEntrance.play(0,inputurl);
            }
        }).start();

        new Thread(new Runnable() {
            @Override
            public void run() {
                try{
                    Thread.sleep(2000);
                }catch (Exception e){}

                mNativeEntrance.setVideoSurface(1,mSurfaceHolder2.getSurface());
                mNativeEntrance.play(1,inputurl);
            }
        }).start();
    }

    public void onSeekBtnClick(View view){
        mNativeEntrance.seek(0,20 * 1000);
    }


    public void onSetPeed(View view){
        mNativeEntrance.setSpeed(0,1.5f);
    }
}
