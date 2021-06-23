/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Test/pcb_ver.h"

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

#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::PCBVer"

const char pcbVerPath[64] = "/mnt/sdcard/US360/PCB_Ver.txt\0";
char PCB_Ver[8] = "V0.0\0";

void Read_PCB_Ver() {
	char readStr[64];
	struct stat sti;
	FILE *fp;

    if (-1 == stat (pcbVerPath, &sti)) {			//不存在
        sprintf(PCB_Ver, "V0.0\0");
    }
    else {									//存在
    	fp = fopen(pcbVerPath, "rt");
    	if(fp != NULL) {
    		memset(&readStr[0], 0, sizeof(readStr) );
    		fgets(readStr, sizeof(readStr), fp);
    		db_debug("Read_PCB_Ver() readStr=%s\n", readStr);
    		sprintf(PCB_Ver, "%s\0", readStr);
    		fclose(fp);
    	}
    	else {
    		sprintf(PCB_Ver, "V0.0\0");
    	}
    }

   	db_debug("Read_PCB_Ver() PCB_Ver= %s\n", PCB_Ver);
    if(strcmp(PCB_Ver, "V1.3") == 0) {
//tmp		setMCULedSetting(MCU_ADC_RATIO_L, adc_ratio % 256);
//tmp		setMCULedSetting(MCU_ADC_RATIO_H, adc_ratio / 256);
    }
}