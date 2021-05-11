/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __SPI_CMD_S3_H__
#define __SPI_CMD_S3_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MTX_S_ADDR		    0x10000000

void as3_reg_addr_init();
void DDR_Reset();

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__SPI_CMD_S3_H__