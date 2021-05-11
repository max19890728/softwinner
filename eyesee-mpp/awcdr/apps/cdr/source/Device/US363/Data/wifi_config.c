/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>

#include "Device/US363/Data/wifi_config.h"
#include "Device/US363/Util/string_util.h"
#include "Device/US363/Util/file_util.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::WifiConfig"

#define WIFI_CONFIG_PATH		"/mnt/extsd/US360/WifiConfig.txt\0"

Wifi_Config_Items_Struct WifiConfigItems[WIFI_CONFIG_ITEMS_MAX] = {
	{ "Wifi_ID",       "" },
	{ "Wifi_Password", "" },
	{ "Encryption",    "" }
};

int checkWifiApPassword(char *password) 
{
	if(stringIsNumber(password) == STRING_NOT_NUMBER || strlen(password) != 8) {
   		db_error("checkWifiApPassword() Err! password=%s len=%d", password, strlen(password) );
   		sprintf(password, "88888888\0");
		return PASSWORK_ERROR;
   	}
	return PASSWORK_OK;
}

void writeWifiConfig(char *ssid, char *password) 
{
	char version[32];
	
	checkWifiApPassword(password);
    db_debug("ssid = %s, password = %s", ssid, password);
    
    get_A2K_Softwave_Version(&version[0]);
    setSendMcuA64Version(&ssid[0], &password[0], &version[0]);

    //Write us360 system config
    FILE *fp;
    char wifi_id[64], wifi_password[64], encryption[64];
    fp = fopen(WIFI_CONFIG_PATH, "wt");
    if(fp != NULL) {
       	sprintf(wifi_id,       "Wifi_ID: %s \r\n\0",        ssid);
       	sprintf(wifi_password, "Wifi_Password: %s \r\n\0",  password);
       	sprintf(encryption,    "Encryption:  \r\n\0");

       	fwrite(&wifi_id[0],       strlen(wifi_id),       1, fp);
       	fwrite(&wifi_password[0], strlen(wifi_password), 1, fp);
       	fwrite(&encryption[0],    strlen(encryption),    1, fp);
       	fflush(fp);
       	fclose(fp);
    }
}

void getNewSsidPassword(char *ssid, char *password)
{
	srand( time(NULL) );
	//double ran = (double)rand() / (RAND_MAX + 1.0) * 10000;
	int ran = (rand() % 10000);
	sprintf(ssid,     "US_%04d\0", (int)ran);
	sprintf(password, "88888888\0");
}

void makeWifiConfigDefault(char *ssid, char *password) 
{
	getNewSsidPassword(ssid, password);
	writeWifiConfig(ssid, password);
}

void getLineItemsValue(char *line, char *value, int count)
{
	int i, size;
	char *tmp_p = NULL;
	
	if(line == NULL)  db_debug("getLineItemsValue() line == NULL");
	if(value == NULL) db_debug("getLineItemsValue() value == NULL");

	tmp_p = line;	
	tmp_p += (strlen(WifiConfigItems[count].items) + 2);	
	size = strlen(tmp_p);
	for(i = 0; i < size; i++) {		
		if(tmp_p[i] == '\r' || tmp_p[i] == '\n') {
			tmp_p[i] = '\0';
		}
		if(tmp_p[i] == '\0') break;
	}	
	memcpy(value, tmp_p, (i-1));
}

void readWifiConfig(char *ssid, char *password) 
{
	int itemsCount = 0;
	char lineString[64];
	char *start_p = NULL;
	FILE *file = NULL;  
	struct stat sti;
           
	if(stat(WIFI_CONFIG_PATH, &sti) != 0) {        	//WifiConfig.txt 不存在, 則產生新檔案    
		makeWifiConfigDefault(ssid, password);
    } 
    else {                        					//WifiConfig.txt 存在, 則讀取檔案
		stat(WIFI_CONFIG_PATH, &sti);
        if(sti.st_size <= 10) {						//檔案存在, 但損毀, 則產生新檔案   
            makeWifiConfigDefault(ssid, password);			
        }
        else{
			file = fopen(WIFI_CONFIG_PATH, "rt");
			if(file != NULL) {
				itemsCount = 0;				
				while(!feof(file) ) {
					if(itemsCount < WIFI_CONFIG_ITEMS_MAX) {							
						memset(&lineString[0], 0, sizeof(lineString) );
						fgets(lineString, sizeof(lineString), file);	
						
						start_p = strstr(&lineString[0], &WifiConfigItems[itemsCount].items[0]);
						if(lineString[0] != 0 && start_p != NULL) {
							getLineItemsValue(&lineString[0], &WifiConfigItems[itemsCount].value[0], itemsCount);
							db_debug("readWifiConfig() 02 count:%d value:%s", itemsCount, WifiConfigItems[itemsCount].value);
							itemsCount++;
						}
						else break;
					}
					else break;
				}
				fclose(file);

				if(itemsCount != WIFI_CONFIG_ITEMS_MAX) {		//有新增內容, 則產生新檔案   
					makeWifiConfigDefault(ssid, password);
				}
			}  
			sprintf(ssid,     "%s\0", WifiConfigItems[WIFI_CONFIG_ITEMS_SSID].value);
			sprintf(password, "%s\0", WifiConfigItems[WIFI_CONFIG_ITEMS_PASSWORD].value);
        }
                
		if(checkWifiApPassword(password) == PASSWORK_ERROR) {
			writeWifiConfig(ssid, password);
		}
		
        //setFileSSID(ssid.getBytes(), pwd.getBytes());		//原本為傳給下層 ssid, password
    }
}