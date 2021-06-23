/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __UX360_WIFIMANAGE_H__
#define __UX360_WIFIMANAGE_H__

#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

int stratWifiAp(char *ssid, char *password, int channel, int type);
void closeWifiAp();
bool isWifiApEnabled();

void stratWifiSta(char *ssid, char *password, int linkType);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__UX360_WIFIMANAGE_H__
