//
// Created by 金海峰 on 2021/5/26.
//

#ifndef MY_TEST_APPLICATION_LOGUTILS_H
#define MY_TEST_APPLICATION_LOGUTILS_H

#ifdef __cpluscplus
extern "C"{
#endif


void native_write_d(const char *msg);

void native_write_w(const char *msg);

void native_write_e(const char *msg);

///////////////////////////////////

void native_print_d(const char *format, ...);

void native_print_e(const char *format, ...);


///////////////////////////////////

//void custom_log(void *ptr, int level, const char *fmt, va_list vl);


#ifdef __cpluscplus
}
#endif
#endif //MY_TEST_APPLICATION_LOGUTILS_H
