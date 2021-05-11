/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __QSPI_H__
#define __QSPI_H__

#include <pthread.h>

#include "Device/spidev.h"
  
#ifdef __cplusplus
extern "C" {
#endif	

//extern pthread_mutex_t pthread_qspi_mutex;
	
int QSPI_Open();
void QSPI_Close();
int QSPI_Write(const char* buf, const int size);
int QSPI_IOCTL(int msg, struct spi_ioc_transfer* xfer);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__QSPI_H__