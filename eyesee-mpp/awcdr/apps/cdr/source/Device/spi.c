/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/spi.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <malloc.h>
#include <pthread.h>
#include <sys/ioctl.h>

#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "Device::SPI"

const char* _spi_path_ = "/dev/spidev3.0\0";

int spi_fd = -1;

pthread_mutex_t pthread_spi_mutex;

int SPI_Open() {
    __u8  mode, lsb, bits;
    __u32 speed = 48000000;

    if(spi_fd > 0) return 0;

    db_debug("spi_init: ~\n");
    if ((spi_fd = open(_spi_path_, O_RDWR)) < 0) {
        db_error("spi_init: err.-1");
        return -1;
    }

    if (ioctl(spi_fd, SPI_IOC_RD_MODE, &mode) < 0) {
        db_error("spi_init: err.-2");
        return -2;
    }

    if (ioctl(spi_fd, SPI_IOC_RD_LSB_FIRST, &lsb) < 0) {
        db_error("spi_init: err.-3");
        return -3;
    }

    if (ioctl(spi_fd, SPI_IOC_RD_BITS_PER_WORD, &bits) < 0) {
        db_error("spi_init: err.-4");
        return -4;
    }

    if (ioctl(spi_fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed) < 0) {
        db_error("spi_init: err.-5");
        return -5;
    }

    return 0;
}

void SPI_Close() {
	if(spi_fd > 0) {
		close(spi_fd);
		spi_fd = -1;
	}
}

int SPI_Write(const char* buf, const int size) {
	int ret=0;
	
	if(spi_fd < 0) {
		db_error("SPI_Write() spi_fd < 0");
		return -1;
	}
	
	ret = write(spi_fd, buf, size);
    if (ret < 0) {
        db_error("spi write error!  size=%d", size);
        return -1;
    }
	
	return ret;
}

int SPI_IOCTL(int msg, struct spi_ioc_transfer* xfer) {
	int ret=0;
	
	if(spi_fd < 0) {
		db_error("SPI_IOCTL() spi_fd < 0");
		return -1;
	}
	
	ret = ioctl(spi_fd, msg, xfer);
    if (ret < 0) {
        db_error("spi ioctl error!");
        return -1;
    }
	
	return ret;
}

