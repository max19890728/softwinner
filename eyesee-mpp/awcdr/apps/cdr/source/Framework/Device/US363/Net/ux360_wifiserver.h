/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __UX360_WIFISERVER_H__
#define __UX360_WIFISERVER_H__


#ifdef __cplusplus
extern "C" {
#endif

struct gsensorVal_struct{
   	float pan;
   	float tilt;
   	float rotate;
   	float wide;
   	unsigned long long timestamp;
};


int start_wifi_server(int port);
void freemSendTHMListData(void);
void setSendFeedBackSTEn(int idx, int en);
void setSendFeedBackCmd(int idx, char *cmd);

void free_wifiserver_buf();
int malloc_wifiserver_buf();
void setDbtDdrCmdEn(int en);
int getDbtDdrCmdEn();
void setDbtInputDdrDataEn(int en);
int getDbtInputDdrDataEn();
void setDbtInputDdrDataFinish(int flag);
int getDbtInputDdrDataFinish();
void setDbtOutputDdrDataEn(int en);
int getDbtOutputDdrDataEn();
void setDbtRegCmdEn(int en);
int getDbtRegCmdEn();
void setDbtInputRegDataFinish(int flag);
int getDbtInputRegDataFinish();
void setDbtOutputRegDataEn(int en);
int getDbtOutputRegDataEn();

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__UX360_WIFISERVER_H__