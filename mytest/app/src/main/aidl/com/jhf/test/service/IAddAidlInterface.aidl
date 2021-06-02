// IAddAidlInterface.aidl
package com.jhf.test.service;

// Declare any non-default types here with import statements
import com.jhf.test.service.ICallbackAidlInterface;

interface IAddAidlInterface{
    /**
     * Demonstrates some basic types that you can use as parameters
     * and return values in AIDL.
     */
    void add(int i,int j,ICallbackAidlInterface calback);
}