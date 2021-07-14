#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <dirent.h>

#include "Device/US363/Util/file_util.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::FileUtil"


int makeFolder(char *folderPath)
{
	struct dirent *dir = NULL;    
	
	dir = opendir(folderPath);
	if(dir == NULL) {
		db_debug("makeFolder() not find dir");
		if(mkdir(folderPath, S_IRWXU) == 0) {
			dir = opendir(folderPath);
			if(dir == NULL) 
				return OPEN_FOLDER_ERROR;
		}
		else {
			db_error("makeFolder() make dir error");
			return MAKE_FOLDER_ERROR;
		}
	}
	if(dir != NULL) closedir(dir);
	
	return MAKE_FOLDER_OK;
}

int checkFileExist(char *filePath)
{
    struct stat sti;
    if (-1 == stat (filePath, &sti))			//not exist
        return 0;
    else
        return 1;
}

int checkIsFolder(char *path) {
    struct stat sti;
    if(stat(path,&sti) == 0) {
    	if(sti.st_mode & S_IFDIR)
    		return 1;
    	else
            return 0;
    }
    else
        return -1;
}

long getFolderSize(char *dirname) {
	DIR *dir;
	struct dirent *ptr;
	long total_size = 0;
	char path[PATH_MAX] = {0};
	struct stat sti;

	dir = opendir(dirname);
	if(dir == NULL) {
		db_error("getFolderSize() open dir: %s failed.\n", path);
		return -1;
	}

	while( (ptr=readdir(dir)) != NULL) {
		sprintf(path, "%s/%s\0", dirname, ptr->d_name);

		if(lstat(path, &sti) < 0) {
			db_error("getFolderSize() lstat %s error.\n", path);
		}

		if(strcmp(ptr->d_name,".") == 0) {
			total_size += sti.st_size;
			continue;
		}
		if(strcmp(ptr->d_name,"..") == 0) {
			continue;
		}

		if(ptr->d_type == DT_DIR)
			total_size += getFolderSize(path);
		else
			total_size += sti.st_size;
	}
	closedir(dir);
	return total_size;
}

void deleteFolder(char *dir_path) {
	DIR *dir;
	struct dirent *dir_info = NULL;
	struct stat sti;
	char path[128];

	dir = opendir(dir_path);
	if(dir == NULL) {
		db_error("deleteFolder() open dir: %s failed.\n", path);
		return;
	}

    while( (dir_info=readdir(dir)) != NULL) {
        if(strcmp(dir_info->d_name,"..") != 0 && strcmp(dir_info->d_name,".") != 0) {
            sprintf(path, "%s/%s\0", dir_path, dir_info->d_name);
            if(stat(path,&sti) == 0) {
            	if((sti.st_mode & S_IFDIR) != 0)
              		deleteFolder(&path[0]);
            	else
            		unlink(path);
            }
        }
    }
	rmdir(dir_path);
    closedir(dir);
    return;
}

void deleteFile(char *path) {
    struct stat sti;
    if(stat(path, &sti) == 0)
       	unlink(path);
}