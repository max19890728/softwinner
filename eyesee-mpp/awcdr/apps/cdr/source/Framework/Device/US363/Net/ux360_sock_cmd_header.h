/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __UX360_SOCK_CMD_HEADER_H__
#define __UX360_SOCK_CMD_HEADER_H__


#ifdef __cplusplus
extern "C" {
#endif

struct socket_cmd_header_struct {
	int 	version;
	char 	keyword[4];
	int 	dataLength;
	int 	checkSum;
};


void sock_cmd_header_init(struct socket_cmd_header_struct *cmd_h);
void set_sock_cmd_header(char *key, int len, struct socket_cmd_header_struct *cmd_h);
void sock_cmd_header_getBytes(char *buf, struct socket_cmd_header_struct *cmd_h);
void sock_cmd_header_Bytes2Data(char *buf, struct socket_cmd_header_struct *cmd_h);
int sock_cmd_header_checkHeader(struct socket_cmd_header_struct *cmd_h);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__UX360_SOCK_CMD_HEADER_H__