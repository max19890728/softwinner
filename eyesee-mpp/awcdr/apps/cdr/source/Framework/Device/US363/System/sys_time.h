/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __SYS_TIME_H__
#define __SYS_TIME_H__

#include <sys/time.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

void get_current_usec(unsigned long long *tm);
int gettimeformt(time_t t, char *t_str);
void setSysTime(unsigned long long time);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__SYS_TIME_H__