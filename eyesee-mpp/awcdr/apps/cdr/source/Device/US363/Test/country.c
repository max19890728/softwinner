/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Test/country.h"

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
#define LOG_TAG "US363::Country"

const char countryPath[64] = "/mnt/sdcard/US360/Country.bin\0";

int getCountryNum() {
	FILE *fp;
	int num = 0;

	fp = fopen(countryPath, "rb");
	if(fp != NULL) {
		fread(&num, sizeof(num), 1, fp);
		fclose(fp);
	}
	else
		num = 840;		//Default	840

	return num;
}