/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Data/system_log.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>

#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::SystemLog"

struct System_Log_Struct_h system_log[1000];
int system_log_write = 0;
int system_log_read = 0;
int system_log_count = 0;

void addSystemLog(int type,char* context, int len) {
	if(len < 50){

		long tt = (long)time(NULL);

		system_log[system_log_write].Type = type;
		system_log[system_log_write].Date = tt;
		memset(system_log[system_log_write].Context, 0, 50);
		memcpy(&system_log[system_log_write].Context, context, len);
		system_log[system_log_write].Context[len] = '\0';
		system_log[system_log_write].ContextLen = len;
		db_debug("add log time : %s\n",context);

		if(system_log_write == 999){
			system_log_write = 0;
		}else{
			system_log_write++;
		}
		if(system_log_count < 1000){
			system_log_count++;
		}else{
			system_log_read = system_log_write + 1;
			if(system_log_read > 999){
				system_log_read = 0;
			}
		}
		//db_debug("add log time : %l\n",system_log[system_log_write].Date);
	}else{
		db_error("system context len > 50\n");
	}
}

int getSystemLogCount() {
	return system_log_count;
}

long readSystemLog(char *context, int *data) {
	if(system_log_count > 0){
		memcpy(context, &system_log[system_log_read].Context, system_log[system_log_read].ContextLen);
		*data = system_log[system_log_read].Type;
		*(data + 1) = system_log[system_log_read].ContextLen;
		long time = system_log[system_log_read].Date;

		system_log_count--;
		if(system_log_read == 999){
			system_log_read = 0;
		}else{
			system_log_read++;
		}
		return time;
	}
	return 0;
}