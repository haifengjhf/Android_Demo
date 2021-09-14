#include "examplemain.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "decoding_encoding.h"
#include "demuxing_decoding.h"
#include "extract_mvs.h"

int example_of_decoding_encoding(JNIEnv *env,jobject thiz,jstring args1,jstring args2){
    char* args[2];
    args[0] = (char*)(env->GetStringUTFChars(args1,0));
    args[1] = (char*)(env->GetStringUTFChars(args2,0));

    main_of_decoding_encoding(2,&args[0]);

    env->ReleaseStringUTFChars(args1,args[0]);
    env->ReleaseStringUTFChars(args2,args[1]);

//    args[0] = (char*)((*env)->GetStringUTFChars(env,args1,0));
//    args[1] = (char*)((*env)->GetStringUTFChars(env,args2,0));
//
//    main_of_decoding_encoding(2,&args[0]);
//
//    (*env)->ReleaseStringUTFChars(env,args1,args[0]);
//    (*env)->ReleaseStringUTFChars(env,args2,args[1]);

    return 1;
}

int example_of_demuxing_decoding(JNIEnv *env,jobject thiz,jstring args1,jstring args2,jstring args3,jstring args4){
    char* args[5];
    args[0] = "demuxing_and_decoding";
    args[1] = (char*)(env->GetStringUTFChars(args1,0));
    args[2] = (char*)(env->GetStringUTFChars(args2,0));
    args[3] = (char*)(env->GetStringUTFChars(args3,0));
    args[4] = (char*)(env->GetStringUTFChars(args4,0));

    main_of_demuxing_decoding(5,&args[0]);

    env->ReleaseStringUTFChars(args1,args[1]);
    env->ReleaseStringUTFChars(args2,args[2]);
    env->ReleaseStringUTFChars(args3,args[3]);
    env->ReleaseStringUTFChars(args4,args[4]);

    return 1;
}

int example_of_extract_mvs(JNIEnv *env,jobject thiz,jstring args1){
    char* args[2];
    args[0] = "demuxing_and_decoding";
    args[1] = (char*)(env->GetStringUTFChars(args1,0));

    main_of_extract_mvs(2,&args[0]);

    env->ReleaseStringUTFChars(args1,args[1]);

    return 1;
}

#ifdef __cplusplus
};
#endif