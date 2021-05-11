/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/gpio.h"

#include <fcntl.h>
#include <sys/ioctl.h>

#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "Device::GPIO"

const char* _gpio_path_ = "/dev/us360_gpio\0";

int gpio_fd = -1;

int GPIO_Open() {
	if(gpio_fd >= 0)
		return 0;
	else {
		gpio_fd = open(_gpio_path_, O_RDWR);
		if(gpio_fd < 0)
			return -1;
	}
	return 0;
}

void GPIO_Close() {
	if(gpio_fd >= 0) {
		close(gpio_fd);
		gpio_fd = -1;
	}
}

int GPIO_IOCTL(int type, int index) {
	int ret = 0;
	if(gpio_fd >= 0)
		ret = ioctl(gpio_fd, type, index);
	else
		return -1;
	return ret;
}
