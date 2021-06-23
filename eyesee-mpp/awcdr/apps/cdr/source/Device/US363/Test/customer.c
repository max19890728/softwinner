/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Test/customer.h"

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
#define LOG_TAG "US363::Customer"

const char customerPath[64] = "/mnt/sdcard/US360/Customer.bin\0";

int getCustomerNum() {
	FILE *fp;
	int num = 0;

	fp = fopen(customerPath, "rb");
	if(fp != NULL) {
		fread(&num, sizeof(num), 1, fp);
		fclose(fp);
	}
	else
		num = 0;
	db_debug("getCustomerNum() num=%d\n", num);
	return num;
}