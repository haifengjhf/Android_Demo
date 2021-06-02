package com.jhf.test.service;

import android.os.IBinder;
import android.os.Parcel;
import android.os.RemoteException;

public class RemoteAddService implements IAddAidlInterface {
    protected RemoveAddServiceStub mRemoveAddServiceStub;


    @Override
    public void add(int i, int j, ICallbackAidlInterface calback) throws RemoteException {

    }

    @Override
    public IBinder asBinder() {
        return mRemoveAddServiceStub.asBinder();
    }


    public static class RemoveAddServiceStub extends IAddAidlInterface.Stub {

        @Override
        public boolean onTransact(int code, Parcel data, Parcel reply, int flags) throws RemoteException {

            return super.onTransact(code, data, reply, flags);
        }

        @Override
        public void add(int i, int j, ICallbackAidlInterface calback) throws RemoteException {

        }
    }

}
