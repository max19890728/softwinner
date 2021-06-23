/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __US360_SYSTEM_CONFIG_H__
#define __US360_SYSTEM_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

#define US360_SYSTEM_CONFIG_ITEM_MAX	6

void WriteUS360SystemConfig(char *ssid, char *pwd);
void ReadUS360SystemConfig(char *ssid, char *pwd);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__US360_SYSTEM_CONFIG_H__