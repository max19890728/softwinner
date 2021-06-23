/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __US363_CAMERA_H__
#define __US363_CAMERA_H__

#ifdef __cplusplus
extern "C" {
#endif

void initCamera();
void startPreview();
void destroyCamera();	

void getWifiApSsid(char *ssid);
	
#ifdef __cplusplus
}   // extern "C"
#endif	

#endif /* __US363_CAMERA_H__ */
