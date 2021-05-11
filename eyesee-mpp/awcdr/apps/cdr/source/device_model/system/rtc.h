/* *******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 * *******************************************************************************/

#pragma once

#undef LOG_TAG
#define LOG_TAG "RTC"

#include <linux/rtc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "common/app_log.h"

#ifdef __cplusplus
extern "C" {
#endif

int set_date_time(struct tm *ptm);
time_t get_date_time(struct tm **local_time);
void reset_date_time(void);
int rtc_get_time(struct tm *tm_time);

#ifdef __cplusplus
}
#endif

namespace EyeseeLinux {

typedef struct RTCTimeTest {
 public:
  RTCTimeTest(const char *msg) {
    this->msg = strdup(msg);
    clock_gettime(CLOCK_MONOTONIC, &measureTime);
    db_msg("start %s time is %ld secs, %ld nsecs", msg, measureTime.tv_sec,
           measureTime.tv_nsec);
  }
  ~RTCTimeTest() {
    clock_gettime(CLOCK_MONOTONIC, &measureTime);
    db_msg("end %s time is %ld secs, %ld nsecs", msg, measureTime.tv_sec,
           measureTime.tv_nsec);
    free(msg);
    msg = NULL;
  }

 private:
  char *msg;
  struct timespec measureTime;
} RTCTimeTest;

}  // namespace EyeseeLinux
