/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Data/us360_parameter_tmp.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>

#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::US360ParaTmp"

struct Tmp_Parameter_struct_H tmp_parameter;

const char paraTmpPath[64] = "/mnt/sdcard/US360ParameterTmp.bin\0";
const char loadParaTmpErr[64] = "/mnt/sdcard/US360/LoadParameterTmp_err.txt\0";

int Save_Parameter_Tmp_File() {
	int size;
	unsigned char buf[sizeof(struct Tmp_Parameter_struct_H)];
	FILE *file = NULL;
	int fp2fd = 0;

	tmp_parameter.CheckSum = PARAMETERTMP_CHECKSUM;

	size = sizeof(struct Tmp_Parameter_struct_H);
	memset(&buf[0], 0, size);
	memcpy(&buf[0], &tmp_parameter, size);

	file = fopen(paraTmpPath, "w+b");
	if(file == NULL) {
		db_error("Save AWB_Parameter.bin err!\n");
		return -1;
	}
	fwrite(&buf[0], size, 1, file);

	fflush(file);
	fp2fd = fileno(file);
	fsync(fp2fd);

	//if(file)
	{
		fclose(file);
		close(fp2fd);
		file = NULL;
	}
	return 0;
}

/*
 * 	讀取US360ParameterTmp.bin, 取得紀錄的值(存檔序號, AE, AWB)
 */
int LoadParameterTmp() {
	FILE *fp;
	int size, check_sum=0, change=0;
	struct stat stFileInfo;
	unsigned char buf[sizeof(struct Tmp_Parameter_struct_H)];
	memset(&buf[0], 0, size);

	size = sizeof(struct Tmp_Parameter_struct_H);

	int intStat;
	intStat = stat(paraTmpPath, &stFileInfo);
	if(intStat != 0) {
		db_error("US360ParameterTmp.bin not exist!\n");
		return -1;
	}
	else if(size != stFileInfo.st_size) {
		db_error("US360ParameterTmp.bin size not same! %d\n", stFileInfo.st_size);
		return stFileInfo.st_size;
	}

	fp = fopen(paraTmpPath, "r+b");
	if(fp == NULL) {
		db_error("Load US360ParameterTmp.bin err!\n");
		return -3;
	}
	fread(&buf[0], size, 1, fp);

	memset(&tmp_parameter, 0, sizeof(struct Tmp_Parameter_struct_H));
	memcpy(&tmp_parameter, &buf[0], size);

	check_sum = tmp_parameter.CheckSum;
	if(check_sum != PARAMETERTMP_CHECKSUM) change = 1;

	db_debug("LoadParameterTmp() rec_file_cnt %d cap_file_cnt %d\n", tmp_parameter.rec_cnt, tmp_parameter.cap_cnt);

	if(fp) fclose(fp);
	//BSmooth_Function = cap_file_cnt & 0x1;        // 測試用，拍2張後看是否有改善
	//if(BSmooth_Function == 1) BSmooth_Function = 4;

	return 1;
}

void Set_Parameter_Tmp_RecCnt(int cnt) {
	tmp_parameter.rec_cnt = cnt;
}

int Get_Parameter_Tmp_RecCnt() {
	return tmp_parameter.rec_cnt;
}

void Set_Parameter_Tmp_CapCnt(int cnt) {
	tmp_parameter.cap_cnt = cnt;
}

int Get_Parameter_Tmp_CapCnt() {
	return tmp_parameter.cap_cnt;
}

void Set_Parameter_Tmp_AEG(int ep_h, int ep_l, int gain) {
	tmp_parameter.AEG_EP_H_Init   = ep_h;
	tmp_parameter.AEG_EP_L_Init   = ep_l;
	tmp_parameter.AEG_gain_H_Init = gain;
}

void Set_Parameter_Tmp_GainRGB(int r, int g, int b) {
	tmp_parameter.Temp_gain_R_Init = r;
	tmp_parameter.Temp_gain_G_Init = g;
	tmp_parameter.Temp_gain_B_Init = b;
}

void WriteLoadParameterTmpErr(int ret) {
	char dirStr[64]  = "/mnt/sdcard/US360\0";
    struct dirent *dir = NULL;    
    FILE *file = NULL;  
	dir = opendir(dirStr);
	if(dir == NULL) {
		if(mkdir(dirStr, S_IRWXU) == 0) {
			dir = opendir(dirStr);
			if(dir == NULL)
				db_error("open LoadParameterTmp dir error\n");
		}
		else
			db_error("make LoadParameterTmp dir error\n");
	}
		
	if(dir != NULL) {
		file = fopen(loadParaTmpErr, "wb");
		if(file != NULL) {
			fwrite(&ret, sizeof(fwrite), 1, file);
			fclose(file);
		}
		closedir(dir);
	}
}