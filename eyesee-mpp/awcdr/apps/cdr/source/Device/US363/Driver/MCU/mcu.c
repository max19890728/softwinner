/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Driver/MCU/mcu.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <time.h>

#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::MCU"

const int mMcuVersion = 44;
int mcuUpdate_flag = 0;

void setSendMcuA64Version(char *ssid, char *pwd, char *version) {
	char ssidNum[16];
	int verLen = strlen(version);
    int id;
    //memcpy(&ssidNum[0], &ssid[3], strlen(ssid)-3);
    sprintf(ssidNum, "%s\0", &ssid[3]);
    id = atoi(ssidNum);
db_debug("setSendMcuA64Version() len=%d ssid=%s\n", strlen(ssid), ssid);
db_debug("setSendMcuA64Version() id=%d ssidNum=%s\n", id, ssidNum);
//tmp    setMCULedSetting(MCU_A64_SSID_L, id % 256);
//tmp    setMCULedSetting(MCU_A64_SSID_H, id / 256);

    int x;
    int pw = atoi(pwd);
    int pwl = pw % 10000;
    int pwh = pw / 10000;
db_debug("setSendMcuA64Version() pw=%d pwd=%s\n", pw, pwd);
//tmp    setMCULedSetting(MCU_A64_PWD_LL, pwl % 256);
//tmp    setMCULedSetting(MCU_A64_PWD_LH, pwl / 256);
//tmp    setMCULedSetting(MCU_A64_PWD_HL, pwh % 256);
//tmp    setMCULedSetting(MCU_A64_PWD_HH, pwh / 256);

    if(verLen > 12){
    	verLen = 12;
    }
    for(x = 0;x < 6; x++){
//tmp    	setMCULedSetting(MCU_A64_VER_1 + x, 255);
//tmp    	setMCULedSetting(MCU_A64_VER_7 + x, 255);
    }
    for(x = 0; x < verLen; x++){
//tmp    	if(x < 6)
//tmp    		setMCULedSetting(MCU_A64_VER_1 + x, version[x]);
//tmp    	else
//tmp    		setMCULedSetting(MCU_A64_VER_7 + x - 6, version[x]);
    }
//tmp    sendMessageForMCU();
}

/*void doSetSendMcuA64Version(char *ssid, int slen, char *pwd, int plen, char *version, int vlen) {
	int slen_tmp, plen_tmp, vlen_tmp;
	char ssid_tmp[16], pwd_tmp[16], ver_tmp[64];

    if(slen > sizeof(ssid_tmp) ) slen_tmp = sizeof(ssid_tmp);
    else						 slen_tmp = slen;
    memcpy(&ssid_tmp[0], ssid, slen_tmp);
    ssid_tmp[slen_tmp] = '\0';

    if(plen > sizeof(pwd_tmp) ) plen_tmp = sizeof(pwd_tmp);
    else						plen_tmp = plen;
    memcpy(&pwd_tmp[0], pwd, plen_tmp);
    pwd_tmp[plen_tmp] = '\0';

    if(vlen > sizeof(ver_tmp) ) vlen_tmp = sizeof(ver_tmp);
    else						vlen_tmp = vlen;
    memcpy(&ver_tmp[0], version, vlen_tmp);
    ver_tmp[vlen_tmp] = '\0';

    setSendMcuA64Version(&ssid_tmp[0], &pwd_tmp[0], &ver_tmp[0]);
}*/

void setMcuDate(time_t time) {
	struct tm *tmf;
	tmf = localtime(&time);
//tmp	setMCULedSetting(MCU_A64_YEAE,  tmf->tm_year + 1900);
//tmp	setMCULedSetting(MCU_A64_MONTH, tmf->tm_mon + 1);
//tmp	setMCULedSetting(MCU_A64_DAY,   tmf->tm_mday);
//tmp	setMCULedSetting(MCU_A64_HOUR,  tmf->tm_hour);
//tmp	setMCULedSetting(MCU_A64_MIN,   tmf->tm_min);
//tmp	setMCULedSetting(MCU_A64_SEC,   tmf->tm_sec);
db_debug("setMcuDate() %d:%d:%d  %d:%d:%d\n",
		tmf->tm_year + 1900, tmf->tm_mon + 1, tmf->tm_mday, tmf->tm_hour, tmf->tm_min, tmf->tm_sec);
}

void SetMcuUpdateFlag(int flag) {
	mcuUpdate_flag = flag;
}

int GetMcuUpdateFlag() {
	return mcuUpdate_flag;
}

//void SetmMcuVersion(int ver) {
//	mMcuVersion = ver;
//}

//int GetmMcuVersion() {
//	return mMcuVersion;
//}

int useMcuTime() {
	int date[20];
//tmp	getMCUSendData(&date[0]);
	db_debug("useMcuTime() get Y/M/D:%d/%d/%d\n", date[0], date[1], date[2]);
   	if(date[0] > 15){
		time_t lTime;
		time_t syst;
		struct tm tmf;
		tmf.tm_year = date[0];
		tmf.tm_mon  = date[1];
		tmf.tm_mday = date[2];
		tmf.tm_hour = date[3];
		tmf.tm_min  = date[4];
		tmf.tm_sec  = date[5];
		lTime = mktime(&tmf);
		syst = time(NULL);		//sec
		if(lTime > syst){
			setSysTime(lTime * 1000);
		}
		return 1;
   	}
   	return 0;
}

void SetMCUData(int cpuNowTemp) {
#ifdef __CLOSE_CODE__	//tmp	
	int x;
   	int fanHigh = 0;
    int fanLow = 0;
    int speed = GetFanSpeed();
    int base_intensity;
    static int delayCheck = 0;
    static int adcValue = 1000;
    static int mcuTimeCheck = 1;
    static int isStartReset = 0;
    static int resetTime = -1;

    if(speed > 0){
       	fanHigh = (3000 + speed * 50) / 256;
        fanLow = (3000 + speed * 50) % 256;
    }
//tmp    setMCULedSetting(MCU_LED_FAN_H, fanHigh);
//tmp    setMCULedSetting(MCU_LED_FAN_L, fanLow);

    base_intensity = 240 - getLedIntensity();
	if(base_intensity > 160) base_intensity = 160;
	if(base_intensity < 40)  base_intensity = 40;
//tmp	setMCULedSetting(MCU_LED_BASE_INTENSITY, base_intensity);
	if(mSelfTimerSec == 0){
//tmp		setMCULedSetting(MCU_LED_NO_CHANGE, 1);
		//for(x = 0; x < 70; x++){
		//for(x = 20; x < 70; x++){
        //	setMCULedSetting(x,ledStateArray[x]);
        //}
//tmp        sendMessageForMCU();
        db_debug("CpuNowTemp = %d , setFanRotateSpeed = %d\n", cpuNowTemp, speed);
	}

	int data[20];
//tmp	getMCUSendData(&data[0]);

	if(data[0] > 15){
		time_t lTime;
		time_t syst;
		struct tm tmf;
		tmf.tm_year = data[0];
		tmf.tm_mon  = data[1];
		tmf.tm_mday = data[2];
		tmf.tm_hour = data[3];
		tmf.tm_min  = data[4];
		tmf.tm_sec  = data[5];
		lTime = mktime(&tmf);
		syst = time(NULL);		//sec
		if(syst > (lTime + 10)){
			setMcuDate(syst);
		}
   	}

    power = data[9];
    dcState = data[11];
    int batteryState = 3;
    if(power == 0){
      	batteryState = 1;
    } else if(dcState == 1){
       	batteryState = 2;
    }
    setBatteryOLED(batteryState, power, cpuNowTemp, 4000);

    checkPowerLog(data[6], data[7], cpuNowTemp, data[12]);

    int ver = data[10];
    if(ver != 0){
      	if(ver > 12 && getWifiDisableTime() > 0){
//tmp       		setMCULedSetting(MCU_SLEEP_TIMER_START, 1);
       	}
       	if(ver != mMcuVersion && mMcuVersion != 0){
       		paintOLEDMcuUpdate();
       		mcuUpdate_flag = 1;
       	}
       	//else { do_Auto_Stitch(); }
       	if(mcuTimeCheck == 1) {
       		mcuTimeCheck = 0;
           	useMcuTime();
        }
       	if(data[13] == 1 && isStartReset == 0){
       		db_debug("a64 reset Start\n");
       		isStartReset = 1;
       		int ran;
       		char newSSID[16];
       		srand(time(NULL));
       		ran = rand() % 10000;
       		sprintf(newSSID, "US_%04d\0", ran);
       		db_debug("a64 reset delete databin Start\n");
       		DeleteUS360DataBin();
       		db_debug("a64 reset delete databin End\n");
       		WriteUS360SystemConfig(&newSSID[0], "88888888\0");
       		db_debug("a64 reset new SSID: %s\n", newSSID);
       		//for(x = 0; x < 70; x++){
       		//for(x = 20; x < 70; x++){
   	        //	setMCULedSetting(x,ledStateArray[x]);
   	        //}
//tmp       		sendMessageForMCU();
       		resetTime = 1;
       		db_debug("a64 reset End\n");
       	}else if(isStartReset == 1){
       		if(resetTime == 1){
       			resetTime = -1;
//tmp       			setMCULedSetting(MCU_IS_A64_CLOSE, 2);
       			db_debug("a64 reset cmd to mcu\n");
//tmp       			sendMessageForMCU();
       		}else{
       			resetTime--;
       		}
       	}

    }
    if(strcmp(PCB_Ver, "V1.3") == 0) {
   		if(adcValue != adc_ratio) {
   			adcValue = adc_ratio;
//tmp   			setMCULedSetting(MCU_ADC_RATIO_L, adcValue % 256);
//tmp   			setMCULedSetting(MCU_ADC_RATIO_H, adcValue / 256);
//tmp   			sendMessageForMCU();
   		}
   	}
    if(delayCheck < 20){
       	delayCheck++;
    }else if(delayCheck == 20){
       	//db_debug("set mcu version: %d\n", mMcuVersion);
       	setTrgMcuVersion(mMcuVersion);
    }
#endif	//__CLOSE_CODE__	
}