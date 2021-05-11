/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Driver/Fan/fan.h"

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
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <time.h>

#include "Device/US363/us360.h"
#include "Device/US363/Cmd/fpga_driver.h"
#include "Device/US363/Test/test.h"
#include "Device/US363/System/sys_time.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::Fan"

int fanAlwaysOn = 0;
int init_fd_fanSpeed = 0, fd_fanSpeed = -1;

int FanCtrl = 2;
int CpuTempThreshold = 70;            // Cpu溫度門檻值, 單位:度C ,隨風扇轉速調整
int CpuTempDownClose = -5;            // Cpu降幾度關風扇
int FanSpeed = 0;
int FanLstLv = -1;

int task1m_lock_flag = 0;
unsigned long long task1m_lock_schedule = 40000000;		//us, 40s
unsigned long long task1m_lock_time;

void SetFanAlwaysOn(int en) {
	fanAlwaysOn = en;
}

int GetFanAlwaysOn() {
	return fanAlwaysOn;
}

void SetTask1mLockFlag(int flag) {
	task1m_lock_flag = flag;
}

int GetTask1mLockFlag() {
	return task1m_lock_flag;
}

int CheckTask1mLockTime(unsigned long long now_t) {
	if( (now_t - task1m_lock_time) > task1m_lock_schedule)
		return 1;
	else
		return 0;
}

void SetFanCtrl(int ctrl) {
	FanCtrl = ctrl;
}

int GetFanCtrl() {
	return FanCtrl;
}

void SetFanSpeed(int speed) {
	FanSpeed = speed;
}

int GetFanSpeed() {
	return FanSpeed;
}

void SetFanLstLv(int speed) {
	FanLstLv = speed;
}

int GetFanLstLv() {
	return FanLstLv;
}

int FanLevel2Speed(int level) {
    int ret = 0;
    switch(level){
    case  0:    ret =  0;     break;
    case  1:    ret =  1;     break;
    case  2:    ret =  2;     break;
    case  3:    ret =  3;     break;
    case  4:    ret =  5;     break;
    case  5:    ret =  7;     break;
    case  6:    ret = 10;     break;
    case  7:    ret = 14;     break;
    case  8:    ret = 18;     break;
    case  9:    ret = 23;     break;
    case 10:    ret = 28;     break;
    case 11:    ret = 34;     break;
    case 12:    ret = 40;     break;
    case 13:    ret = 47;     break;
    case 14:    ret = 54;     break;
    case 15:    ret = 62;     break;
    case 16:    ret = 71;     break;
    case 17:    ret = 80;     break;
    case 18:    ret = 90;     break;
    case 19:    ret = 99;     break;
    default:    ret =  0;     break;
    }
    return ret;
}

int FanSpeed2Level(int speed) {
    int ret = 0;
    switch(speed){
    case  0:    ret =  0;     break;
    case  1:    ret =  1;     break;
    case  2:    ret =  2;     break;
    case  3:    ret =  3;     break;
    case  5:    ret =  4;     break;
    case  7:    ret =  5;     break;
    case 10:    ret =  6;     break;
    case 14:    ret =  7;     break;
    case 18:    ret =  8;     break;
    case 23:    ret =  9;     break;
    case 28:    ret = 10;     break;
    case 34:    ret = 11;     break;
    case 40:    ret = 12;     break;
    case 47:    ret = 13;     break;
    case 54:    ret = 14;     break;
    case 62:    ret = 15;     break;
    case 71:    ret = 16;     break;
    case 80:    ret = 17;     break;
    case 90:    ret = 18;     break;
    case 99:    ret = 19;     break;
    default:    ret =  0;     break;
    }
    return ret;
}

int calculateFanLevel(int nowSpeed, int cpuTemprature) {
    int lv = FanSpeed2Level(nowSpeed);

    if(FanCtrl == 1) 	  CpuTempThreshold = 60;
    else if(FanCtrl == 2) CpuTempThreshold = 70;
    else				  CpuTempThreshold = 75;

    int diffTemp = cpuTemprature - CpuTempThreshold;
    lv += diffTemp;

    if(FanSpeed2Level(nowSpeed) != 0 && lv < 1) {
        if(diffTemp <= CpuTempDownClose)
            lv = 0;
        else
            lv = 1;
    }
    if(lv < 0)		 lv = 0;
    else if(lv > 19) lv = 19;

    return lv;
}

void FanCtrlFunc() {
	int fanPreRange = 10;
	static int fanPreData = 0;
	static int fanAvgFlag = 0;

//tmp   	if(GetLedByTimeFlag() == 1) {
//tmp   		return;
//tmp   	}
    int CpuNowTemp = -1;

    CpuNowTemp = getCPUTemprature();
    if(CpuNowTemp < 0) return;

    int State, MainCmd, SubCmd;
    State   = TestToolCmd.State;
	MainCmd = TestToolCmd.MainCmd;
	SubCmd  = TestToolCmd.SubCmd;

    if(FanCtrl == 0)        FanSpeed = 0;
    else if(MainCmd == 7 && SubCmd == 7){
//tmp      	do_Test_Mode_Func_jni(MainCmd, SubCmd);
    }
    else{
    	int level = calculateFanLevel(FanSpeed, CpuNowTemp);
        int speed = FanLevel2Speed(level);
        FanSpeed = speed;
    }

    int fpgatemp = Read_FPGA_Pmw();
    if(fanPreData == 0 || fpgatemp < fanPreData){
      	fanPreData = fpgatemp;
    }

    if(GetFanAlwaysOn() == 1){
      	if(FanSpeed < 40){
      		FanSpeed = 40;
       	}
    }else{
      	fanAvgFlag++;
        if(fanAvgFlag >= 5){
          	if(fpgatemp > (fanPreData + fanPreRange)){
           		db_debug("fpga temperature start :%d\n", (fpgatemp - fanPreData) );
           		SetFanAlwaysOn(1);
           		fanPreData = 255;
           		get_current_usec(&task1m_lock_time);
           		task1m_lock_flag = 1;
           	}
           	fanAvgFlag = 0;
        }
    }

    if(CpuNowTemp >= CPU_TEMP_MAX){
//      	systemlog.addLog("error", System.currentTimeMillis(), "machine", "OverHeat!", String.valueOf(CpuNowTemp));
//tmp        stopREC(7);
//tmp        SetPlaySoundFlag(8);
        usleep(1000000);
//tmp        paintCpuStatus(CpuNowTemp, FanSpeed);
    }

    if(FanLstLv == -1 || (FanLstLv != FanSpeed)){
    	FanLstLv = FanSpeed;
        setFanRotateSpeed(FanSpeed);            // 設定風扇轉速
        db_debug("CpuNowTemp = %d , setFanRotateSpeed = %d\n", CpuNowTemp, FanSpeed);
    }
//tmp    SetMCUData(CpuNowTemp);
}

/*
 * weber+160826
 *   設定風扇轉速
 *   參數:
 *       0 ~ 99
 * */
void setFanRotateSpeed(int speed) {
	int ret;
	
    if(speed < 0)       speed = 0;
    else if(speed > 99) speed = 99;
    db_debug("setFanRotateSpeed: speed=%d\n", speed);

    if(init_fd_fanSpeed == 0) {
        fd_fanSpeed = open("/dev/pwm_ctrl\0", O_RDWR);
        if(fd_fanSpeed < 0){
            db_error("setFanRotateSpeed: open '/dev/pwm_ctrl' Err. fd=%d\n", fd_fanSpeed);
            return;
        }
        init_fd_fanSpeed = 1;
    }
    
    ret = ioctl(fd_fanSpeed, 1, speed);
    if(ret < 0) {
        db_error("setFanRotateSpeed: ioctl '/dev/pwm_ctrl' Err. ret=%d\n", ret);
        return;
    }
}