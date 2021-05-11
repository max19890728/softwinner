/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __MCU_H__
#define __MCU_H__

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

void setSendMcuA64Version(char *ssid, char *pwd, char *version);
void setMcuDate(time_t time);
void SetMCUData(int cpuNowTemp);
void SetMcuUpdateFlag(int flag);
int GetMcuUpdateFlag();

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__MCU_H__
