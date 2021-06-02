package com.jhf.test.activity;

import com.jhf.test.R;

import android.Manifest;
import android.app.Instrumentation;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.hardware.display.DisplayManager;
import android.os.Bundle;
import android.util.Log;
import android.view.MotionEvent;
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

        Log.d(TAG," widthPixels:" + getResources().getDisplayMetrics().widthPixels + " heightPixels:" + getResources().getDisplayMetrics().heightPixels);

        mulBleTest();
    }

    public void requestPermission(){
//        if(PackageManager.PERMISSION_GRANTED == ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_COARSE_LOCATION)){
//            ActivityCompat.requestPermissions(this,new String[]{Manifest.permission.BLUETOOTH},0);
//        }
        if(PackageManager.PERMISSION_GRANTED == ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH)){
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
}
