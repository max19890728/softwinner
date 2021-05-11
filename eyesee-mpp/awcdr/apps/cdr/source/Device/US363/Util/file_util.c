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