/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __UX360_SOCK_CMD_STA_H__
#define __UX360_SOCK_CMD_STA_H__


#ifdef __cplusplus
extern "C" {
#endif

void sock_cmd_sta_initVal(void);

void setSockCmdSta_sId_Record(int val);
int getSockCmdSta_sId_Record(void);

void setSockCmdSta_tId_Record(int val);
int getSockCmdSta_tId_Record(void);

void setSockCmdSta_recStatus(int val);
int getSockCmdSta_recStatus(void);

void setSockCmdSta_f_recStatus(int val);
int getSockCmdSta_f_recStatus(void);

void setSockCmdSta_p_recStatus(int val);
int getSockCmdSta_p_recStatus(void);

void setSockCmdSta_recLock(int val);
int getSockCmdSta_recLock(void);

void setSockCmdSta_sId_Thumbnail(int val);
int getSockCmdSta_sId_Thumbnail(void);

void setSockCmdSta_tId_Thumbnail(int val);
int getSockCmdSta_tId_Thumbnail(void);

void setSockCmdSta_thmStatus(int val);
int getSockCmdSta_thmStatus(void);

void setSockCmdSta_sId_Image(int val);
int getSockCmdSta_sId_Image(void);

void setSockCmdSta_tId_Image(int val);
int getSockCmdSta_tId_Image(void);

void setSockCmdSta_imgStatus(int val);
int getSockCmdSta_imgStatus(void);

void setSockCmdSta_sId_Data(int val);
int getSockCmdSta_sId_Data(void);

void setSockCmdSta_tId_Data(int val);
int getSockCmdSta_tId_Data(void);

void setSockCmdSta_dataStatus(int val);
int getSockCmdSta_dataStatus(void);

void setSockCmdSta_sId_Rtmp(int val);
int getSockCmdSta_sId_Rtmp(void);

void setSockCmdSta_tId_Rtmp(int val);
int getSockCmdSta_tId_Rtmp(void);

void setSockCmdSta_rtmpStatus(int val);
int getSockCmdSta_rtmpStatus(void);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__UX360_SOCK_CMD_STA_H__