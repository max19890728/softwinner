/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/qspi.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <malloc.h>
#include <pthread.h>
#include <sys/ioctl.h>

#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "Device::QSPI"

const char* _qspi_path_ = "/dev/spidev0.1\0";

int qspi_fd = -1;

//pthread_mutex_t pthread_qspi_mutex;

int QSPI_Open() {
    __u8  lsb, bits;
    __u32 speed = 100000000;
	__u32 mode;

    if(qspi_fd > 0) return 0;

    db_debug("QSPI_Open: ~\n");
    if ((qspi_fd = open(_qspi_path_, O_RDWR)) < 0) {
        db_error("QSPI_Open: err.-1");
        return -1;
    }

    if (ioctl(qspi_fd, SPI_IOC_RD_MODE32, &mode) < 0) {
        db_error("QSPI_Open: err.-2.1");
        return -2;
    }
	
/*	mode |= (SPI_TX_QUAD | SPI_RX_QUAD);
	if (ioctl(qspi_fd, SPI_IOC_WR_MODE32, &mode) < 0) {
        db_error("QSPI_Open: err.-2.2");
        return -2;
    }*/

    if (ioctl(qspi_fd, SPI_IOC_RD_LSB_FIRST, &lsb) < 0) {
        db_error("QSPI_Open: err.-3.1");
        return -3;
    }

    if (ioctl(qspi_fd, SPI_IOC_RD_BITS_PER_WORD, &bits) < 0) {
        db_error("QSPI_Open: err.-4");
        return -4;
    }

    if (ioctl(qspi_fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed) < 0) {
        db_error("QSPI_Open: err.-5");
        return -5;
    }

    return 0;
}

void QSPI_Close() {
	if(qspi_fd > 0) {
		close(qspi_fd);
		qspi_fd = -1;
	}
}

int QSPI_Write(const char* buf, const int size) {
	int ret=0;
	
	if(qspi_fd < 0) {
		db_error("QSPI_Write() qspi_fd < 0");
		return -1;
	}
	
	ret = write(qspi_fd, buf, size);
    if (ret < 0) {
        db_error("qspi write error!  size=%d", size);
        return -1;
    }
	
	return ret;
}

int QSPI_IOCTL(int msg, struct spi_ioc_transfer* xfer) {
	int ret=0;
	
	if(qspi_fd < 0) {
		db_error("QSPI_IOCTL() qspi_fd < 0");
		return -1;
	}
	
	ret = ioctl(qspi_fd, msg, xfer);
    if (ret < 0) {
        db_error("qspi ioctl error!");
        return -1;
    }
	
	return ret;
}

