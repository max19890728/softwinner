/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __FILE_UTIL_H__
#define __FILE_UTIL_H__


#ifdef __cplusplus
extern "C" {
#endif

enum make_folder_result {
	MAKE_FOLDER_OK = 0,
	OPEN_FOLDER_ERROR,
	MAKE_FOLDER_ERROR
};

int makeFolder(char *folderPath);
int checkFileExist(char *filePath);
int checkIsFolder(char *path);
long getFolderSize(char *dirname);
void deleteFolder(char *dir_path);
void deleteFile(char *path);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__FILE_UTIL_H__