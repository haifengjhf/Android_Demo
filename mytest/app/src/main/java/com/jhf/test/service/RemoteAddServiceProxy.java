package com.jhf.test.service;

import android.os.IBinder;
import android.os.Parcel;
import android.os.RemoteException;

public class RemoteAddServiceProxy implements IAddAidlInterface{
    protected IBinder mRemoteBinder;

    public RemoteAddServiceProxy(IBinder remoteBinder){
        mRemoteBinder =remoteBinder;
    }



    @Override
    public IBinder asBinder() {
        //do nothing
        return null;
    }


    @Override
    public void add(int i, int j, ICallbackAidlInterface callback) throws RemoteException {
//        AddServiceCallbackImpl callbackImpl = null;
//        if(callback instanceof AddServiceCallbackImpl){
//            callbackImpl = (AddServiceCallbackImpl)callback;
//        }
        Parcel data = Parcel.obtain();
        Parcel reply = Parcel.obtain();
        data.writeInt(i);
        data.writeInt(j);
        data.writeStrongBinder(callback.asBinder());

        mRemoteBinder.transact(Stub.TRANSACTION_add,data,reply, 0);
    }
}
