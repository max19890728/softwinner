/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

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

#include "Device/US363/Data/pcb_version.h"
#include "Device/US363/Util/file_util.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::PCBVer"

#define PCB_VERSION_PATH    "/mnt/extsd/US360/PCB_Ver.txt\0"

void readPcbVersion(char *pcbVersion) {
	char readStr[64];
	FILE *fp;

    if (!checkFileExist(PCB_VERSION_PATH)) {			//not exist
        sprintf(pcbVersion, "V0.0\0");
    }
    else {									            //exist
    	fp = fopen(PCB_VERSION_PATH, "rt");
    	if(fp != NULL) {
    		memset(&readStr[0], 0, sizeof(readStr) );
    		fgets(readStr, sizeof(readStr), fp);
    		sprintf(pcbVersion, "%s\0", readStr);
    		fclose(fp);
    	}
    	else {
    		sprintf(pcbVersion, "V0.0\0");
    	}
    }

   	db_debug("readPcbVersion() version= %s\n", pcbVersion);
    if(strcmp(pcbVersion, "V1.3") == 0) {
//tmp		setMCULedSetting(MCU_ADC_RATIO_L, adc_ratio % 256);
//tmp		setMCULedSetting(MCU_ADC_RATIO_H, adc_ratio / 256);
    }
}