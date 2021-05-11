/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __SYSTEM_LOG_H__
#define __SYSTEM_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

struct System_Log_Struct_h {
	int Type;
	long Date;
	char Context[51];
	int ContextLen;
};

void addSystemLog(int type,char* context, int len);
int getSystemLogCount();
long readSystemLog(char *context, int *data);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__SYSTEM_LOG_H__