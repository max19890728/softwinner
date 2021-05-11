/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __US363_FOLDER_H__
#define __US363_FOLDER_H__


#ifdef __cplusplus
extern "C" {
#endif

#define US360_FOLDER_PATH		"/mnt/extsd/US360\0"
#define TEST_FOLDER_PATH		"/mnt/extsd/US360/Test\0"

int makeUS360Folder();
int makeTestFolder();

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__US363_FOLDER_H__