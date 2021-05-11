/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Net/ux360_sock_cmd_header.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "Device/US363/System/sys_time.h"
#include "Device/US363/Util/byte_util.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::SockCmdHeader"

void sock_cmd_header_init(struct socket_cmd_header_struct *cmd_h) {
	int b2i = byte2Int(&cmd_h->keyword[0], sizeof(cmd_h->keyword));
	cmd_h->version = 0x20160311;
	cmd_h->dataLength = 0;
	cmd_h->checkSum = cmd_h->version + b2i + cmd_h->dataLength;	
}

void set_sock_cmd_header(char *key, int len, struct socket_cmd_header_struct *cmd_h) {
	int b2i=0;
	cmd_h->version = 0x20160311;
	cmd_h->dataLength = len;
	memcpy(&cmd_h->keyword[0], key, sizeof(cmd_h->keyword));
	b2i = byte2Int(&cmd_h->keyword[0], sizeof(cmd_h->keyword));
	cmd_h->checkSum = cmd_h->version + b2i + cmd_h->dataLength;
}

void sock_cmd_header_getBytes(char *buf, struct socket_cmd_header_struct *cmd_h) {
	int size;
	char *b = buf;
	size = sizeof(cmd_h->version);
	memcpy(b, &cmd_h->version, size);
	b += size;
	size = sizeof(cmd_h->keyword);
	memcpy(b, &cmd_h->keyword[0], size);
	b += size;
	size = sizeof(cmd_h->dataLength);
	memcpy(b, &cmd_h->dataLength, size);
	b += size;
	size = sizeof(cmd_h->checkSum);
	memcpy(b, &cmd_h->checkSum, size);
	b += size;
}

void sock_cmd_header_Bytes2Data(char *buf, struct socket_cmd_header_struct *cmd_h) {
	//ByteBuffer buf = ByteBuffer.wrap(b);
	int size;
	char *b = buf;
	size = sizeof(cmd_h->version);
	memcpy(&cmd_h->version, b, size);
	swap32(&cmd_h->version);
	b += size;
	
	size = sizeof(cmd_h->keyword);
	memcpy(&cmd_h->keyword[0], b, size);
	b += size;
	
	size = sizeof(cmd_h->dataLength);
	memcpy(&cmd_h->dataLength, b, size);
	swap32(&cmd_h->dataLength);
	b += size;
	
	size = sizeof(cmd_h->checkSum);
	memcpy(&cmd_h->checkSum, b, size);
	swap32(&cmd_h->checkSum);
	b += size;
}

int sock_cmd_header_checkHeader(struct socket_cmd_header_struct *cmd_h) {
	int b2i = byte2Int(&cmd_h->keyword[0], 4);
	int sum = cmd_h->version + b2i + cmd_h->dataLength;
	if(sum == cmd_h->checkSum) return 1;
	else					   return 0;
}