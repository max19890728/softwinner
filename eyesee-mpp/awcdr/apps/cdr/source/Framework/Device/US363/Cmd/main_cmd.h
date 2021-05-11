/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __MAIN_CMD_H__
#define __MAIN_CMD_H__

#include "Device/US363/Cmd/AletaS2_CMD_Struct.h"

#ifdef __cplusplus
extern "C" {
#endif

void setISP1RGBOffset(int idx, int value);
void getISP1RGBOffset(int *val);
void AS2MainCMDInit();
void MainCmdStart();

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__MAIN_CMD_H__
