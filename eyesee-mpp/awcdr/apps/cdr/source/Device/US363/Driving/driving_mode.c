/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Driving/driving_mode.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <dirent.h>

#include "Device/US363/Util/file_util.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::DrivingMode"

struct dirent DrivingMode_Info[DRIVING_MODE_INFO_MAX];

void get_thm_path(char *dir_path, char *name, struct stat *sti, char *thm_path) {
	char thm_name[128];
	char *str_p = NULL;
    int len;

	if( sti->st_mode & S_IFDIR) {
		sprintf(thm_path, "%s/THM/%s.thm\0", dir_path, name);
	}
	else {
		str_p = strstr(name, ".jpg");
		if(str_p != NULL) {			// .jpg
			len = (str_p-name);
			memcpy(thm_name, name, len);
			thm_name[len] = '\0';
		}
		else {
			str_p = strstr(name, ".mp4");
			if(str_p != NULL) {		// .mp4
				len = (str_p-name);
				memcpy(thm_name, name, len);
				thm_name[len] = '\0';
			}
			else {
				str_p = strstr(name, ".ts");
				if(str_p != NULL) {		// .ts
					len = (str_p-name);
					memcpy(thm_name, name, len);
					thm_name[len] = '\0';
				}
				else					// error
					sprintf(thm_name, "%s\0", name);
			}
		}
		sprintf(thm_path, "%s/THM/%s.thm\0", dir_path, thm_name);
	}
}

int DrivingMode_Info_Sort(char *dir_path) {
	int i, j;
	int info_idx = 0;
	DIR *dir = NULL;
	struct dirent *dir_info = NULL;
	struct dirent info_tmp;
	struct stat sti1, sti2;
	char path1[128], path2[128];

	dir = opendir(dir_path);
    if(dir != NULL) {
    	memset(&DrivingMode_Info[0], 0, sizeof(DrivingMode_Info));
		while( (dir_info=readdir(dir)) != NULL) {
			if(strcmp(dir_info->d_name,"..") != 0 && strcmp(dir_info->d_name,".") != 0) {
				sprintf(path1, "%s/%s\0", dir_path, dir_info->d_name);
				if(stat(path1,&sti1) == 0 && strcmp(dir_info->d_name, "THM") != 0 &&
				   (strstr(dir_info->d_name, ".mp4") != NULL || strstr(dir_info->d_name, ".jpg") != NULL ||
				    strstr(dir_info->d_name, ".ts") != NULL || (sti1.st_mode & S_IFDIR) != 0)
				) {
					if(info_idx < DRIVING_MODE_INFO_MAX) {
						DrivingMode_Info[info_idx] = *dir_info;
						info_idx++;
					}
					else
						break;
				}
			}
		}
		closedir(dir);
    }

	for(i = 0; i < info_idx; i++) {
		for(j = 0; j < i; j++) {
			sprintf(path1, "%s/%s\0", dir_path, DrivingMode_Info[j].d_name);
			sprintf(path2, "%s/%s\0", dir_path, DrivingMode_Info[i].d_name);
			stat(path1, &sti1);		//DrivingMode_Info[j]
			stat(path2, &sti2);		//DrivingMode_Info[i]
			if(sti1.st_mtime > sti2.st_mtime) {
				info_tmp = DrivingMode_Info[j];
				DrivingMode_Info[j] = DrivingMode_Info[i];
				DrivingMode_Info[i] = info_tmp;
			}
		}
	}
	return info_idx;
}

int DrivingModeDeleteFile(char *dir_path) {
    long del_size=0, dir_size=0;
    struct stat sti, thm_sti;
    char path[128], thm_path[128];
    char thm_name[128];
    char *str_p = NULL;
    int i, len, cnt=0, info_cnt=0;

db_debug("DrivingModeDeleteFile() dir=%s\n", dir_path);

	//依時間排序
	info_cnt = DrivingMode_Info_Sort(dir_path);

    for(i = 0; i < info_cnt; i++) {
      	if(del_size > DRIVING_FREE_SIZE) break;

        sprintf(path, "%s/%s\0", dir_path, DrivingMode_Info[i].d_name);
        if(stat(path,&sti) == 0) {
      		if( sti.st_mode & S_IFDIR) {
      			get_thm_path(dir_path, &DrivingMode_Info[i].d_name, &sti, &thm_path[0]);
       			if(stat(thm_path, &thm_sti) == 0)
       				unlink(thm_path);

       			del_size += getFolderSize(path);
       			deleteFolder(&path[0]);
       		}
       		else {
       			get_thm_path(dir_path, &DrivingMode_Info[i].d_name, &sti, &thm_path[0]);
       			if(stat(thm_path, &thm_sti) == 0)
       				unlink(thm_path);

       			del_size += sti.st_size;
       			unlink(path);
       		}
       		db_debug("DrivingModeDeleteFile() path=%s mt=%ld size=%d\n", path, sti.st_mtime, del_size);
       		db_debug("DrivingModeDeleteFile() thm_path=%s\n", thm_path);
       		cnt++;
        }
     }

db_debug("DrivingModeDeleteFile() finish! (cnt=%d, del=%ld, info=%d)\n", cnt, del_size, info_cnt);
	return cnt;
}