/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __UX360_SOCK_CMD_STA_HEADER_H__
#define __UX360_SOCK_CMD_STA_HEADER_H__


#ifdef __cplusplus
extern "C" {
#endif

struct sock_cmd_sta_header_struct {
	int 	version;
	char 	keyword[4];
	int 	dataLength;
	int 	sourceId;
	int 	targetId;
	int		reserve1;
	int		reserve2;
	int 	checkSum;
};

void sock_cmd_sta_header_init(struct sock_cmd_sta_header_struct *cmd_h);
void set_sock_cmd_sta_header(char *key, int len, int srcId, int trgId, 
		struct sock_cmd_sta_header_struct *cmd_h);
void sock_cmd_sta_header_getBytes(char *buf, 
		struct sock_cmd_sta_header_struct *cmd_h);
void sock_cmd_sta_header_Bytes2Data(char *buf, 
		struct sock_cmd_sta_header_struct *cmd_h);
int sock_cmd_sta_header_checkHeader(struct sock_cmd_sta_header_struct *cmd_h);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__UX360_SOCK_CMD_STA_HEADER_H__