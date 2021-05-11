/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __SPI_H__
#define __SPI_H__

#include <pthread.h>

#include "Device/spidev.h"
  
#ifdef __cplusplus
extern "C" {
#endif	

extern pthread_mutex_t pthread_spi_mutex;
	
int SPI_Open();
void SPI_Close();
int SPI_Write(const char* buf, const int size);
int SPI_IOCTL(int msg, struct spi_ioc_transfer* xfer);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__SPI_H__