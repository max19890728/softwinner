/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/System/sys_cpu.h"

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

#include "Device/US363/Test/test.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::SysCPU"

int Cpu_Core_Num = 1;

/*
 * rex+ 151211
 *   設定CPU模式
 *   echo userspace > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
 *   參數:
 *   fantasys conservative ondemand userspace powersave performance
 * */
/*void setCpuMode(int mode) {
    int file, n, nbytes; char cbuf[64];
    db_debug("setCpuMode: mode=%d\n", mode);
    if((file = open("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor\0", O_RDWR)) < 0){
        db_error("setCpuMode: openErr! file=%d\n", file);
        return;
    }
    switch(mode){
        case 0: sprintf(cbuf, "fantasys\0"); break;
        case 1: sprintf(cbuf, "conservative\0"); break;
        case 2: sprintf(cbuf, "ondemand\0"); break;
        case 3: sprintf(cbuf, "userspace\0"); break;
        case 4: sprintf(cbuf, "powersave\0"); break;
        case 5: sprintf(cbuf, "performance\0"); break;
        default: close(file);
            db_error("setCpuMode: Err! mode=%d\n", mode);
            return;
    }
    nbytes = strlen(cbuf);
    n = write(file, cbuf, nbytes);
    if(n != nbytes){
        db_error("setCpuMode: writeErr! n=%d nbytes=%d\n", n, nbytes);
    }
    close(file);
}*/

/* 
 * rex+ 170104
 *   設定CPU核心數目
 *   echo 1 > /sys/devices/system/cpu/cpu0/online
 * */
void setCpuMultiCore(int cpu_num) {
    int i, j, n; char fstr[128];
    int file;
    Cpu_Core_Num = cpu_num;
    for(i = 1; i < cpu_num; i++){
        sprintf(fstr, "/sys/devices/system/cpu/cpu%d/online\0", i);
        if((file = open(fstr, O_RDWR)) < 0){
            db_error("setCpuMultiCore: echo 1 > %s failed!\n", fstr);
            return;
        }
        n = write(file, "1", 1);
        if(n != 1){
            db_error("setCpuMultiCore: warning! i=%d n=%d\n", i, n);
        }
        close(file);
    }
    for(j = i; j < 4; j++){
        sprintf(fstr, "/sys/devices/system/cpu/cpu%d/online\0", j);
        if((file = open(fstr, O_RDWR)) < 0){
            db_error("setCpuMultiCore: echo 0 > %s failed!\n", fstr);
            return;
        }
        n = write(file, "0", 1);
        if(n != 1){
            db_error("setCpuMultiCore: warning! j=%d n=%d\n", j, n);
        }
        close(file);
    }
}

int getCpuCoreNum() {
    return Cpu_Core_Num;
}

/*
 * rex+ 151211
 *   設定CPU頻率
 *   echo 120000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq
 *   a31s參數:
 *             120000    144000    168000    192000    216000    240000    264000    288000    312000    336000    360000
 *   384000    408000    432000    456000    480000    504000    528000    552000    576000    600000    624000    648000
 *   672000    696000    720000    744000    768000    792000    828000    864000    900000    936000    972000   1008000
 *   1044000   1080000   1116000   1152000   1200000   1248000   1296000   1344000   1392000   1440000
 *
 *   a64參數:
 *             480000    600000    720000    816000    1008000   1104000   1152000   1200000   1344000
 *
 *   查詢:        cat /sys/devices/system/cpu/cpufreq/all_time_in_state
 * */
void setCpuFreq(int cpu_num, int freq)
{
    int file, n, nbytes; char cfreq[64], fstr[128];
    int i;
    
    db_debug("setCpuFreq: cpu=%d freq=%d\n", cpu_num, freq);
    sprintf(cfreq, "%d\0", freq);
    nbytes = strlen(cfreq);
    
    for(i = 0; i < cpu_num; i++){
        sprintf(fstr, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq\0", i);
        if((file = open(fstr, O_RDWR)) < 0){
            db_error("setCpuFreq: scaling_max_freq. file=%s\n", fstr);
            return;
        }
        n = write(file, cfreq, nbytes);
        if(n != nbytes){
            db_error("setCpuFreq: max_freq, i=%d n=%d nbytes=%d\n", i, n, nbytes);
        }
        close(file);
        sprintf(fstr, "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_min_freq\n\0", i);
        if((file = open(fstr, O_RDWR)) < 0){
            db_error("setCpuFreq: scaling_min_freq. file=%s", fstr);
            return;
        }
        n = write(file, cfreq, nbytes);
        if(n != nbytes){
            db_error("setCpuFreq: min_freq, i=%d n=%d nbytes=%d\n", i, n, nbytes);
        }
        close(file);
    }
}

void SetCpu(int cpu_num, int freq) {
    int tool_cmd = get_TestToolCmd();
    if(tool_cmd > 0) {
        setCpuMultiCore(4);
        setCpuFreq(4, 600000);
    }
    else {
        setCpuMultiCore(cpu_num);
        setCpuFreq(cpu_num, freq);
    }
}

/*
 * weber+161005
 *   取得CPU溫度
 *   參數:
 *       none
 *   回傳:
 *       int (單位:度C)
 * */
int getCPUTemprature() {
    char buf[16];
    int fd_cpuTemp;

    fd_cpuTemp = open("/sys/devices/virtual/thermal/thermal_zone0/temp\0", O_RDONLY);
    if(fd_cpuTemp < 0){
        db_error("getCPUTemprature: open '/sys/devices/virtual/thermal/thermal_zone0/temp' Err. fd=%d\n", fd_cpuTemp);
        return -1;
    }

    memset(&buf[0], 0, 16);
    read(fd_cpuTemp, &buf[0], 16);
    close(fd_cpuTemp);

    int temprature = -1;
    sscanf(buf, "%d", &temprature);

    return temprature;
}