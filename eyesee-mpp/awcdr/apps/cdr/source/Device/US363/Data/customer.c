/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Data/customer.h"

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

#define CUSTOMER_BIN_PATH   "/mnt/extsd/US360/Customer.bin\0"

int readCustomerCode() {
	FILE *fp;
	int code = 0;

	fp = fopen(CUSTOMER_BIN_PATH, "rb");
	if(fp != NULL) {
		fread(&code, sizeof(code), 1, fp);
		fclose(fp);
	}
	else
		code = 0;       //0:Ultracker

	return code;
}