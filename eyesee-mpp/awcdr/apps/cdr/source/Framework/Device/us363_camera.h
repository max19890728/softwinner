/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __US363_CAMERA_H__
#define __US363_CAMERA_H__

#ifdef __cplusplus
extern "C" {
#endif

int initCamera();
void startPreview();

//==================== get/set =====================
void getSdPath(char *path);
void getUS363Version(char *version);
	
	
#ifdef __cplusplus
}   // extern "C"
#endif	

#endif /* __US363_CAMERA_H__ */
