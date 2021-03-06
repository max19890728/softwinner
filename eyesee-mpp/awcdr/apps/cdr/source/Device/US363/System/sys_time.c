/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/System/sys_time.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <math.h>
#include <time.h>

#include "device_model/system/rtc.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::SysTime"

void get_current_usec(unsigned long long *tm) {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    *tm = ((unsigned long long)tv.tv_sec * 1000000) + tv.tv_usec;
    //return(*tm);        // 用return傳值，高位元資料會錯誤
}

int gettimeformt(time_t t, char *t_str) {
    struct tm *timeinfo;
    timeinfo = localtime(&t);
    strftime (t_str, 80, "%Y%m%d_%H%M%S", timeinfo);
    return 0;
}

/*
 * rex+ 151225
 *   設定系統時間
 * */
void setSysTime(unsigned long long time) {
    int ret;
    struct timespec ts;
    struct tm *tt;

    ts.tv_sec = time / 1000;
    ts.tv_nsec = (time % 1000) * 1000000;

    tt = localtime(&ts.tv_sec);
    db_debug("setSysTime: %d/%d/%d %d:%d:%d time=%lld\n", tt->tm_year+1900, tt->tm_mon+1, tt->tm_mday,
            tt->tm_hour, tt->tm_min, tt->tm_sec, time);

    rtc_set_time(tt);
    if(ret < 0)
        db_error("setSysTime: err! ret=%d time=%lld\n", ret, time);
}