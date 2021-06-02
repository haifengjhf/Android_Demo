//
// Created by 金海峰 on 2021/5/26.
//

#ifndef MY_TEST_APPLICATION_LOGUTILS_H
#define MY_TEST_APPLICATION_LOGUTILS_H


void native_write_d(const char *msg);

void native_write_d(const char *tag, const char *msg);

void native_write_w(const char *msg);

void native_write_e(const char *msg);

void native_print_d(const char *format, ...);


#endif //MY_TEST_APPLICATION_LOGUTILS_H
