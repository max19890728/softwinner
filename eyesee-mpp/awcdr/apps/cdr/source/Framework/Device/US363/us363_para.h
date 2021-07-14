/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __US363_PARA_H__
#define __US363_PARA_H__

#ifdef __cplusplus
extern "C" {
#endif

extern int DebugJPEGMode;	// 0:normal 1:debug
extern int DebugJPEGaddr;
extern int ISP2_Sensor;		// 0~3:sensor0~3 4:all

//===============================================================
void setALIGlobalPhiWifiCmd(int phi_idx);
void setALIGlobalPhi2WifiCmd(int phi_idx);
void setStitchingOut(int c_mode, int mode, int res_mode, int fps);
int writeCmdTable(int sub_state, int res_mode, int fps, int flag, int isInit, int usb_en);
int get_ISP2_Sensor(void);
void SendMainCmdPipe(int c_mode, int t_mode, int sync_mode);
void reset_JPEG_Quality(int quality);
void setJPEGaddr(int mode, int addr, int sensor);
void setGPSLocation(int en, double latitude, double longitude, double altitude);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__US363_PARA_H__