/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __FPGA_DOWNLOAD_H__
#define __FPGA_DOWNLOAD_H__
	
#ifdef __cplusplus
extern "C" {
#endif
	
int GetFpgaCheckSum();
int DownloadProc();
int fpgaPowerOff();

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__FPGA_DOWNLOAD_H__
