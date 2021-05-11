/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/System/sys_power.h"

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

#include "Device/US363/us360.h"
#include "Device/US363/Cmd/fpga_driver.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::SysPower"

/*
 * rex+ 151211
 *   設定電源計畫
 *   echo standby > /sys/power/state
 *   echo on > /sys/power/state
 *   參數 :
 *   standby mem bootfast
 * */
void setPowerMode(int mode) {
    int file, n, nbytes; char cbuf[64];
    db_debug("setPowerMode: mode=%d\n", mode);
    if((file = open("/sys/power/state\0", O_RDWR)) < 0){
        db_error("setPowerMode: openErr. file=%d\n", file);
        return;
    }
    switch(mode){
        case 0: sprintf(cbuf, "standby\0"); break;
        case 1: sprintf(cbuf, "mem\0"); break;
        case 2: sprintf(cbuf, "bootfast\0"); break;
        case 3: sprintf(cbuf, "on\0");
                Set_Init_Image_State(0);
//tmp                Set_Init_Image_Time(0);
                Ep_Change_Init();
                break;
        default: close(file);
            db_error("setPowerMode: Err! mode=%d\n", mode);
            return;
    }
    nbytes = strlen(cbuf);
    n = write(file, cbuf, nbytes);
    if(n != nbytes){
        db_error("setPowerMode: file=%d n=%d nbytes=%d\n", file, n, nbytes);
    }
    close(file);
}

/*
 * 設定axp81x充電電流
 * state:
 *             0 = run, 1 = suspend, 2 = shutdown
 */
int init_fp_axp81x = 0, fp_axp81x = -1;
void SetAxp81xChg(int state) {
    if(init_fp_axp81x == 0){
        fp_axp81x = fopen("/proc/spi/axp81x_chg", "wb");
        if(fp_axp81x < 0){
            db_error("SetAxp81xChg fopen error!\n");
            return;
        }
        init_fp_axp81x = 1;
    }

    char str[2];
    sprintf(str, "%d\0", state);
    int len = strlen(str);
    db_debug("SetAxp81xChg : len = %d, str = %s\n", len, str);
    fwrite(str, len, 1, fp_axp81x);
}