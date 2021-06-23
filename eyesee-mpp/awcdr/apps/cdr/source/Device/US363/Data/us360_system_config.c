/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Data/us360_system_config.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <dirent.h>

#include "Device/US363/us360.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::US360SysConfig"

const char sysCfgPath[64] = "/mnt/sdcard/US360/US360SystemConfig.txt\0";

void WriteUS360SystemConfig(char *ssid, char *pwd) {

  	if(isNumeric(&pwd[0]) == 0 || strlen(pwd) != 8) {
   		db_error("WriteUS360SystemConfig() Err! pwd=%s len=%d", pwd, strlen(pwd) );
   		sprintf(pwd, "88888888\0");
   	}

    db_debug("ssid = %s, pwd = %s", ssid, pwd);
    char ver[32];
    get_A2K_Softwave_Version(&ver[0]);
    setSendMcuA64Version(&ssid[0], &pwd[0], &ver[0]);

    //Write us360 system config
    FILE *fp;
    char serial[64], wifi_id[64], wifi_pwd[64], wifi_mac[64], us360_mac[64], encryption[64];
    fp = fopen(sysCfgPath, "wt");
    if(fp != NULL) {
      	sprintf(serial,     "Serial_Number:  \r\n\0");
       	sprintf(wifi_id,    "Wifi_ID: %s \r\n\0",        ssid);
       	sprintf(wifi_pwd,   "Wifi_Password: %s \r\n\0",  pwd);
       	sprintf(wifi_mac,   "Wifi_MAC:  \r\n\0");
       	sprintf(us360_mac,  "US360_MAC:  \r\n\0");
       	sprintf(encryption, "Encryption:  \r\n\0");

       	fwrite(&serial[0],     strlen(serial),     1, fp);
       	fwrite(&wifi_id[0],    strlen(wifi_id),    1, fp);
       	fwrite(&wifi_pwd[0],   strlen(wifi_pwd),   1, fp);
       	fwrite(&wifi_mac[0],   strlen(wifi_mac),   1, fp);
       	fwrite(&us360_mac[0],  strlen(us360_mac),  1, fp);
       	fwrite(&encryption[0], strlen(encryption), 1, fp);
       	fflush(fp);
       	fclose(fp);
    }
}

void MakeUS360SystemConfigDefault(char *ssid, char *pwd) {
	FILE *file = NULL;
	char idStr[32] = "", pwStr[32] = ""; 
	char str_w[64];	
	
	file = fopen(sysCfgPath, "wt");
	if(file != NULL) {  
		srand( time(NULL) );
		double ran = (double)rand() / (RAND_MAX + 1.0) * 9999;
		sprintf(idStr, "US_%04d\0", (int)ran);
		sprintf(pwStr, "88888888\0");
                
		sprintf(ssid, "%s\0", idStr);
		sprintf(pwd,  "%s\0", pwStr);
		//setFileSSID(ssid.getBytes(), pwd.getBytes() );
                
		sprintf(str_w, "Serial_Number:  \r\n\0");
		fwrite(str_w, strlen(str_w), 1, file);
		sprintf(str_w, "Wifi_ID: %s \r\n\0", idStr);
		fwrite(str_w, strlen(str_w), 1, file);
		sprintf(str_w, "Wifi_Password: %s \r\n\0", pwStr);
		fwrite(str_w, strlen(str_w), 1, file);
		sprintf(str_w, "Wifi_MAC:  \r\n\0");
		fwrite(str_w, strlen(str_w), 1, file);
		sprintf(str_w, "US360_MAC:  \r\n\0");
		fwrite(str_w, strlen(str_w), 1, file);
		sprintf(str_w, "Encryption:  \r\n\0");
		fwrite(str_w, strlen(str_w), 1, file);
		fclose(file);  	
	}
	else
		db_error("MakeUS360SystemConfigDefault() create US360SystemConfig error");
}

void ReadUS360SystemConfig(char *ssid, char *pwd) {
    int ser, id, pw, wmac, usmac, enc;
    char sbStr[32] = "";
    char serStr[32] = "", idStr[32] = "", pwStr[32] = "", wmacStr[32] = "", usmacStr[32] = "", encStr[32] = "";  
	char str_r[64];	
        
    char dirStr[64]  = "/mnt/sdcard/US360\0";
	char dirTestStr[64]  = "/mnt/sdcard/US360/Test\0";
    struct dirent *dir = NULL;
	struct dirent *dirTest = NULL;     
    FILE *file = NULL;  
	struct stat sti;
	
	int count = 0;
	char *tmp = NULL, *start = NULL;
	int posStrLen = US360_SYSTEM_CONFIG_ITEM_MAX;
	char posStr[US360_SYSTEM_CONFIG_ITEM_MAX][32] = {
		"Serial_Number", "Wifi_ID", "Wifi_Password", "Wifi_MAC", "US360_MAC", "Encryption"
	};

	dir = opendir(dirStr);
	if(dir == NULL) {
		db_debug("ReadUS360SystemConfig() not find dir");
		if(mkdir(dirStr, S_IRWXU) == 0) {
			dir = opendir(dirStr);
			if(dir == NULL) return;
		}
		else {
			db_error("ReadUS360SystemConfig() make dir error");
			return;
		}
	}
	if(dir != NULL) closedir(dir);
           
	dirTest = opendir(dirTestStr);
	if(dirTest == NULL) {
		db_debug("ReadUS360SystemConfig() not find dirTest");
		if(mkdir(dirTestStr, S_IRWXU) == 0) {
			dirTest = opendir(dirTestStr);
			if(dirTest == NULL) return;
		}
		else {
			db_error("ReadUS360SystemConfig() make dirTest error");
			return;
		}
	}
	if(dirTest != NULL) closedir(dirTest);
           
	if(stat(sysCfgPath, &sti) != 0) {        //US360SystemFile.txt 不存在    
        //產生檔案
		MakeUS360SystemConfigDefault(ssid, pwd);
    } 
    else {                        //US360SystemFile.txt 存在
        //讀取檔案
		stat(sysCfgPath, &sti);
        if(sti.st_size <= 10) {		//檔案存在, 但損毀
            MakeUS360SystemConfigDefault(ssid, pwd);
        }
        else{
			file = fopen(sysCfgPath, "rt");
			if(file != NULL) {
				count = 0;
				while(!feof(file) ) {
					if(count < posStrLen){
						memset(&str_r[0], 0, sizeof(str_r) );
						fgets(str_r, sizeof(str_r), file);
						start = strstr(&str_r[0], &posStr[count][0]);
						if(str_r[0] != 0 && start != NULL) {
							tmp = &str_r[0];
							tmp += (strlen(posStr[count]) + 2);
							for(int i = 0; i < strlen(tmp); i++) {
								if(tmp[i] == '\r' || tmp[i] == '\n') {
									tmp[i] = '\0';
								}
								if(tmp[i] == '\0') break;
							}

							switch(count) {
							case 0: sprintf(serStr,   "%s\0", tmp); break;	
							case 1: sprintf(idStr,    "%s\0", tmp); break;
							case 2: sprintf(pwStr,    "%s\0", tmp); break;
							case 3: sprintf(wmacStr,  "%s\0", tmp); break;
							case 4: sprintf(usmacStr, "%s\0", tmp); break;
							case 5: sprintf(encStr,   "%s\0", tmp); break;
							}

							db_debug("readDatabinFromPath() 02 count:%d str:%s", count, tmp);
							count++;
						}
						else break;
					}
					else break;
				}
				fclose(file);

				if(count != posStrLen) {		// 有新增內容
					MakeUS360SystemConfigDefault(ssid, pwd);
				}
			}  
			sprintf(ssid, "%s\0", idStr);
			sprintf(pwd,  "%s\0", pwStr);
        }
                
        if(isNumeric(pwd) == 0 || strlen(pwd) != 8) {
            db_error("ReadUS360SystemConfig() Err! pwd=%s len=%d", pwd, strlen(pwd) );
			sprintf(pwd,  "88888888\0");
            WriteUS360SystemConfig(ssid, pwd);
        }
        //setFileSSID(ssid.getBytes(), pwd.getBytes());
    }
}