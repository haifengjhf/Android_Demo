package com.jhf.test.activity;

import com.jhf.nativelib.bridge.NativeEntranceTestActivity;
import com.jhf.nativelib.bridge.NativeExampleActivity;
import com.jhf.nativelib.bridge.sdl.SDLActivity;
import com.jhf.test.R;

import android.Manifest;
import android.app.Instrumentation;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.hardware.display.DisplayManager;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Adapter;

import java.util.List;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

public class MainActivity extends AppCompatActivity {
    private final static String TAG = "MainActivity";

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        requestPermission();
//        requestStoragePermission();

        Log.d(TAG," widthPixels:" + getResources().getDisplayMetrics().widthPixels + " heightPixels:" + getResources().getDisplayMetrics().heightPixels);

        mulBleTest();


    }

    public void requestPermission(){
        if(PackageManager.PERMISSION_DENIED == ContextCompat.checkSelfPermission(this, Manifest.permission.MANAGE_EXTERNAL_STORAGE)){
            ActivityCompat.requestPermissions(this,new String[]{Manifest.permission.MANAGE_EXTERNAL_STORAGE},0);
        }
        if(PackageManager.PERMISSION_DENIED == ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH)){
            ActivityCompat.requestPermissions(this,new String[]{Manifest.permission.BLUETOOTH},0);
        }
    }

    public void inputSimulate(){
        Instrumentation inst = new Instrumentation();
//        MotionEvent motionEvent = MotionEvent.obtain();
//        inst.sendPointerSync(motionEvent);
    }

    public void mulBleTest(){
        BluetoothManager bluetoothManager = (BluetoothManager)getSystemService(Context.BLUETOOTH_SERVICE);
        List<BluetoothDevice> connectedList = bluetoothManager.getConnectedDevices(BluetoothProfile.GATT);
        Log.w(TAG,"onTextConnectedDevice getConnectedDevices connectedList:" + connectedList);

        if(connectedList != null && !connectedList.isEmpty()){
            BluetoothDevice bluetoothDevice = connectedList.get(0);
            if(bluetoothDevice != null){
                BluetoothGatt bluetoothGatt = bluetoothDevice.connectGatt(this, false, new BluetoothGattCallback() {
                    @Override
                    public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
                        super.onConnectionStateChange(gatt, status, newState);
                        Log.w(TAG,"onTextConnectedDevice onConnectionStateChange gatt:" + gatt + " status:" + status + " newState:" + newState);
                    }
                });
            }
        }
    }

    public void WiFiDisplay(){
        DisplayManager displayManager = (DisplayManager)getSystemService(Context.DISPLAY_SERVICE);
    }

    @Override
    public void onConfigurationChanged(@NonNull Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        Log.d(TAG," widthPixels:" + getResources().getDisplayMetrics().widthPixels + " heightPixels:" + getResources().getDisplayMetrics().heightPixels);
    }

    public void onSDLBtnClick(View view){
        Intent intent = new Intent(this, SDLActivity.class);
        startActivity(intent);
    }

    public void onNativeEntranceBtnClick(View view){
        Intent intent = new Intent(this, NativeEntranceTestActivity.class);
        startActivity(intent);
    }

    public void onNativeExampleBtnClick(View view){
        Intent intent = new Intent(this, NativeExampleActivity.class);
        startActivity(intent);
    }

    public void onMediaRecordBtnClick(View view){
//        Intent intent = new Intent(this, MediaRecorderActivity.class);
//        startActivity(intent);

        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.setData(Uri.parse("native://thirdsdk/wedoctor/health"));
        startActivity(intent);
    }

    public void onBannerBtnClick(View view){
//        Intent intent = new Intent(this, BannerActivity.class);
//        startActivity(intent);

        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.setData(Uri.parse("native://thirdsdk/lianren/health"));
        startActivity(intent);
    }


    public void requestStoragePermission(){
        Uri uri1 = Uri.parse("content://com.android.externalstorage.documents/tree/primary%3AAndroid%2Fdata");
        Intent intent1 = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        intent1.setFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION
                | Intent.FLAG_GRANT_WRITE_URI_PERMISSION
                | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION
                | Intent.FLAG_GRANT_PREFIX_URI_PERMISSION);
        intent1.putExtra("EXTRA_INITIAL_URI", uri1);
        startActivityForResult(intent1, 11);
    }
}
