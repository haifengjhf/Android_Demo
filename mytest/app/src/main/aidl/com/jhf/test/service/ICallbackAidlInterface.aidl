// ICallbackAidlInterface.aidl
package com.jhf.test.service;

// Declare any non-default types here with import statements

interface ICallbackAidlInterface {
    /**
     * Demonstrates some basic types that you can use as parameters
     * and return values in AIDL.
     */
    void onComplete(int result);
}