/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __DRIVING_MODE_H__
#define __DRIVING_MODE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* 1GB, Driving Mode 一次刪除的容量 */
#define DRIVING_FREE_SIZE 0x40000000

#define DRIVING_MODE_INFO_MAX	10000

int DrivingModeDeleteFile(char *dir_path);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__DRIVING_MODE_H__