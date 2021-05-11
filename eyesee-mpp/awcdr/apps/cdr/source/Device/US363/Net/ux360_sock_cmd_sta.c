/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Net/ux360_sock_cmd_sta.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "Device/US363/Net/ux360_wifiserver.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::SockCmdSta"
	
//recStatus
int sId_Record = 0;
int tId_Record = 0;
int recStatus = 1; 		// 0:正常發送  1:重複發送
int f_recStatus = 0; 	//前置狀態 0:無錄影  1:錄影中
int p_recStatus = 0; 	//後置狀態 0:無錄影  1:錄影中
int recLock = 0;
	
//thmStatus
int sId_Thumbnail = 0;
int tId_Thumbnail = 0;
int thmStatus = 1; 		// 0:正常發送  1:重複發送
	
//imgStatus
int sId_Image = 0;
int tId_Image = 0;
int imgStatus = 0; 		// 0:正常發送  1:重複發送
	
//dataStatus
int sId_Data = 0;
int tId_Data = 0;
int dataStatus = 0; 	// 0:正常發送  1:重複發送
	
//rtmpStatus
int sId_Rtmp = 0;
int tId_Rtmp = 0;
int rtmpStatus = 0; 	// 0:正常發送  1:重複發送


void setSockCmdSta_sId_Record(int val) { 
	sId_Record = val; 
}
int getSockCmdSta_sId_Record(void) { 
	return sId_Record; 
}

void setSockCmdSta_tId_Record(int val) {
	tId_Record = val;
}
int getSockCmdSta_tId_Record(void) { 
	return tId_Record; 
}

void setSockCmdSta_recStatus(int val) {
	recStatus = val;
}
int getSockCmdSta_recStatus(void) { 
	return recStatus; 
}

void setSockCmdSta_f_recStatus(int val) {
	f_recStatus = val;
}
int getSockCmdSta_f_recStatus(void) { 
	return f_recStatus; 
}

void setSockCmdSta_p_recStatus(int val) {
	p_recStatus = val;
}
int getSockCmdSta_p_recStatus(void) { 
	return p_recStatus; 
}

void setSockCmdSta_recLock(int val) {
	recLock = val;
}
int getSockCmdSta_recLock(void) { 
	return recLock; 
}

void setSockCmdSta_sId_Thumbnail(int val) { 
	sId_Thumbnail = val; 
}
int getSockCmdSta_sId_Thumbnail(void) { 
	return sId_Thumbnail; 
}

void setSockCmdSta_tId_Thumbnail(int val) { 
	tId_Thumbnail = val; 
}
int getSockCmdSta_tId_Thumbnail(void) { 
	return tId_Thumbnail; 
}

void setSockCmdSta_thmStatus(int val) { 
	thmStatus = val; 
}
int getSockCmdSta_thmStatus(void) { 
	return thmStatus; 
}

void setSockCmdSta_sId_Image(int val) { 
	sId_Image = val; 
}
int getSockCmdSta_sId_Image(void) { 
	return sId_Image; 
}

void setSockCmdSta_tId_Image(int val) { 
	tId_Image = val; 
}
int getSockCmdSta_tId_Image(void) { 
	return tId_Image; 
}

void setSockCmdSta_imgStatus(int val) { 
	imgStatus = val; 
}
int getSockCmdSta_imgStatus(void) { 
	return imgStatus; 
}

void setSockCmdSta_sId_Data(int val) { 
	sId_Data = val; 
}
int getSockCmdSta_sId_Data(void) { 
	return sId_Data; 
}

void setSockCmdSta_tId_Data(int val) { 
	tId_Data = val; 
}
int getSockCmdSta_tId_Data(void) { 
	return tId_Data; 
}

void setSockCmdSta_dataStatus(int val) { 
	dataStatus = val; 
}
int getSockCmdSta_dataStatus(void) { 
	return dataStatus; 
}

void setSockCmdSta_sId_Rtmp(int val) { 
	sId_Rtmp = val; 
}
int getSockCmdSta_sId_Rtmp(void) { 
	return sId_Rtmp; 
}

void setSockCmdSta_tId_Rtmp(int val) { 
	tId_Rtmp = val; 
}
int getSockCmdSta_tId_Rtmp(void) { 
	return tId_Rtmp; 
}

void setSockCmdSta_rtmpStatus(int val) { 
	rtmpStatus = val; 
}
int getSockCmdSta_rtmpStatus(void) { 
	return rtmpStatus; 
}
	
void sock_cmd_sta_initVal(void) {
	//0:recStatus
	setSendFeedBackSTEn(0, 0);
	setSendFeedBackCmd(0, "");
		
	//1:thmStatus
	setSendFeedBackSTEn(1, 0);
	setSendFeedBackCmd(1, "");
		
	//2:imgStatus
	setSendFeedBackSTEn(2, 0);
	setSendFeedBackCmd(2, "");
}

