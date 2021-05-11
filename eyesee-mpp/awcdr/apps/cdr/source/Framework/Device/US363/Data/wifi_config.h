/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __WIFI_CONFIG_H__
#define __WIFI_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

enum check_password_result {
	PASSWORK_OK = 0,
	PASSWORK_ERROR
};

enum wifi_config_items_index {
	WIFI_CONFIG_ITEMS_SSID = 0,
	WIFI_CONFIG_ITEMS_PASSWORD,
	WIFI_CONFIG_ITEMS_ENCRYPTION,
	WIFI_CONFIG_ITEMS_MAX
};

typedef struct Wifi_Config_Items_Struct_H {
	char items[32];
	char value[32];
}Wifi_Config_Items_Struct;

int checkWifiApPassword(char *password);
void writeWifiConfig(char *ssid, char *password);
void readWifiConfig(char *ssid, char *password);
void getNewSsidPassword(char *ssid, char *password);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__WIFI_CONFIG_H__