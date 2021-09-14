package com.jhf.test.activity;

import com.jhf.test.R;

import android.Manifest;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.hardware.Camera;
import android.media.MediaRecorder;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.ToggleButton;

import java.io.IOException;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;

public class MediaRecorderActivity extends AppCompatActivity implements SurfaceHolder.Callback{
    private final static String TAG = "MediaRecorderActivity";
    private final String VIDEO_PATH_NAME = "/sdcard/Android/data/com.jhf.test/VGA_30fps_512vbrate.mp4";

    private MediaRecorder mMediaRecorder;
    private Camera mCamera;
    private SurfaceView mSurfaceView;
    private SurfaceHolder mHolder;
//    private ToggleButton mToggleButton;
    private volatile boolean mInitSuccesful;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_media_recorder_recipe);

        // we shall take the video in landscape orientation
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);

        mSurfaceView = (SurfaceView) findViewById(R.id.surfaceView);
        mHolder = mSurfaceView.getHolder();
        mHolder.addCallback(this);
        mHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);

//        mToggleButton = (ToggleButton) findViewById(R.id.toggleRecordingButton);
//        mToggleButton.setOnClickListener(new View.OnClickListener() {
//            @Override
//            // toggle video recording
//            public void onClick(View v) {
//                if(ActivityCompat.checkSelfPermission(MediaRecorderActivity.this,Manifest.permission.CAMERA) == PackageManager.PERMISSION_DENIED){
//                    return;
//                }
//
//                if (mToggleButton.isChecked()) {
//                    mMediaRecorder.start();
//                    try {
//                        Thread.sleep(10 * 1000);
//                    } catch (Exception e) {
//                        e.printStackTrace();
//                    }
//                    finish();
//                }
//                else {
//                    mMediaRecorder.stop();
//                    mMediaRecorder.reset();
//                    try {
//                        initRecorder(mHolder.getSurface());
//                    } catch (IOException e) {
//                        e.printStackTrace();
//                    }
//                }
//            }
//        });


        if(ActivityCompat.checkSelfPermission(this,Manifest.permission.CAMERA) == PackageManager.PERMISSION_DENIED
                ||ActivityCompat.checkSelfPermission(this,Manifest.permission.RECORD_AUDIO) == PackageManager.PERMISSION_DENIED
                ||ActivityCompat.checkSelfPermission(this,Manifest.permission_group.CAMERA) == PackageManager.PERMISSION_DENIED
        ){
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.CAMERA,Manifest.permission.RECORD_AUDIO,Manifest.permission_group.CAMERA},1);
            return;
        }

    }

    public void onMediaRecordStartBtnClick(View view){
        new Thread(new Runnable() {
            @Override
            public void run() {
                mMediaRecorder.start();
                try {
                    Thread.sleep(10 * 1000);
                } catch (Exception e) {
                    e.printStackTrace();
                }
                MediaRecorderActivity.this.finish();
            }
        }).start();

    }

    public void onMediaRecordStopBtnClick(View view){
        mMediaRecorder.stop();
        mMediaRecorder.reset();
        try {
            initRecorder(mHolder.getSurface());
        } catch (IOException e) {
            e.printStackTrace();
        }
        finish();
    }

    private Camera openFrontFacingCameraGingerbread() {
        Camera cam = Camera.open(Camera.CameraInfo.CAMERA_FACING_FRONT);

//        int cameraCount = 0;
//        Camera cam = null;
//        Camera.CameraInfo cameraInfo = new Camera.CameraInfo();
//        cameraCount = Camera.getNumberOfCameras();
//        for (int camIdx = 0; camIdx < cameraCount; camIdx++) {
//            Camera.getCameraInfo(camIdx, cameraInfo);
//            if (cameraInfo.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
//                try {
//                    cam = Camera.open(camIdx);
//                } catch (RuntimeException e) {
//                    Log.e(TAG, "Camera failed to open: " + e.getLocalizedMessage());
//                }
//            }
//        }

        return cam;
    }

    /* Init the MediaRecorder, the order the methods are called is vital to
     * its correct functioning */
    private void initRecorder(Surface surface) throws IOException {
        // It is very important to unlock the camera before doing setCamera
        // or it will results in a black preview

        if(mCamera == null) {
            mCamera = openFrontFacingCameraGingerbread();
            mCamera.unlock();
        }

        if(mMediaRecorder == null)  mMediaRecorder = new MediaRecorder();
        mMediaRecorder.setPreviewDisplay(surface);
        mMediaRecorder.setCamera(mCamera);

        mMediaRecorder.setAudioSource(MediaRecorder.AudioSource.DEFAULT);
        mMediaRecorder.setVideoSource(MediaRecorder.VideoSource.DEFAULT);
        //       mMediaRecorder.setOutputFormat(8);
        mMediaRecorder.setOutputFormat(MediaRecorder.OutputFormat.MPEG_4);

        mMediaRecorder.setVideoEncoder(MediaRecorder.VideoEncoder.H264);
        mMediaRecorder.setAudioEncoder(MediaRecorder.AudioEncoder.AAC);

        mMediaRecorder.setVideoEncodingBitRate(512 * 1000);
        mMediaRecorder.setVideoFrameRate(30);
        mMediaRecorder.setVideoSize(640, 480);
        mMediaRecorder.setOutputFile(VIDEO_PATH_NAME);

        try {
            mMediaRecorder.prepare();
        } catch (IllegalStateException e) {
            // This is thrown if the previous calls are not called with the
            // proper order
            e.printStackTrace();
        }

        mInitSuccesful = true;
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        try {
            if(!mInitSuccesful)
                initRecorder(mHolder.getSurface());
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        shutdown();
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {}

    private void shutdown() {
        // Release MediaRecorder and especially the Camera as it's a shared
        // object that can be used by other applications
        mMediaRecorder.reset();
        mMediaRecorder.release();
        mCamera.release();

        // once the objects have been released they can't be reused
        mMediaRecorder = null;
        mCamera = null;
    }

}
