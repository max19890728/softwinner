/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/
 
#include "Device/US363/Net/ux360_sock_cmd_sta_header.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "Device/US363/Util/byte_util.h"
#include "Device/US363/System/sys_time.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::SockCmdStaHeader"


void sock_cmd_sta_header_init(struct sock_cmd_sta_header_struct *cmd_h) {
	int b2i = byte2Int(&cmd_h->keyword[0], sizeof(cmd_h->keyword));
	cmd_h->version = 0x20170316;
	cmd_h->dataLength = 0;
	cmd_h->sourceId = 0;
	cmd_h->targetId = 0;
	cmd_h->reserve1 = 0;
	cmd_h->reserve2 = 0;
	cmd_h->checkSum = cmd_h->version + b2i + cmd_h->dataLength + cmd_h->sourceId +
						cmd_h->targetId + cmd_h->reserve1 + cmd_h->reserve2;
}

void set_sock_cmd_sta_header(char *key, int len, int srcId, int trgId, 
		struct sock_cmd_sta_header_struct *cmd_h) {
	int b2i=0;
	cmd_h->version = 0x20170316;
	cmd_h->dataLength = len;
	cmd_h->sourceId = srcId;
	cmd_h->targetId = trgId;
	cmd_h->reserve1 = 0;
	cmd_h->reserve2 = 0;
	memcpy(&cmd_h->keyword[0], key, sizeof(cmd_h->keyword));
	b2i = byte2Int(&cmd_h->keyword[0], sizeof(cmd_h->keyword));
	cmd_h->checkSum = cmd_h->version + b2i + cmd_h->dataLength + cmd_h->sourceId + 
						cmd_h->targetId + cmd_h->reserve1 + cmd_h->reserve2;
}

void sock_cmd_sta_header_getBytes(char *buf, 
		struct sock_cmd_sta_header_struct *cmd_h) {
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
	size = sizeof(cmd_h->sourceId);
	memcpy(b, &cmd_h->sourceId, size);
	b += size;
	size = sizeof(cmd_h->targetId);
	memcpy(b, &cmd_h->targetId, size);
	b += size;
	size = sizeof(cmd_h->reserve1);
	memcpy(b, &cmd_h->reserve1, size);
	b += size;
	size = sizeof(cmd_h->reserve2);
	memcpy(b, &cmd_h->reserve2, size);
	b += size;
	size = sizeof(cmd_h->checkSum);
	memcpy(b, &cmd_h->checkSum, size);
	b += size;
}

void sock_cmd_sta_header_Bytes2Data(char *buf, 
		struct sock_cmd_sta_header_struct *cmd_h) {	
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
	size = sizeof(cmd_h->sourceId);
	memcpy(&cmd_h->sourceId, b, size);
	swap32(&cmd_h->sourceId);
	b += size;
	size = sizeof(cmd_h->targetId);
	memcpy(&cmd_h->targetId, b, size);
	swap32(&cmd_h->targetId);
	b += size;
	size = sizeof(cmd_h->reserve1);
	memcpy(&cmd_h->reserve1, b, size);
	swap32(&cmd_h->reserve1);
	b += size;
	size = sizeof(cmd_h->reserve2);
	memcpy(&cmd_h->reserve2, b, size);
	swap32(&cmd_h->reserve2);
	b += size;
	size = sizeof(cmd_h->checkSum);
	memcpy(&cmd_h->checkSum, b, size);
	swap32(&cmd_h->checkSum);
	b += size;
	
	printf("sock_cmd_sta_header_Bytes2Data() ver=0x%x\n", cmd_h->version);
	printf("sock_cmd_sta_header_Bytes2Data() len=%d\n", cmd_h->dataLength);
	printf("sock_cmd_sta_header_Bytes2Data() sum=%d\n", cmd_h->checkSum);
	printf("sock_cmd_sta_header_Bytes2Data() key=%c.%c.%c.%c\n", 
		cmd_h->keyword[0], cmd_h->keyword[1], cmd_h->keyword[2], cmd_h->keyword[3]);
}

int sock_cmd_sta_header_checkHeader(struct sock_cmd_sta_header_struct *cmd_h) {
	int b2i = byte2Int(&cmd_h->keyword[0], 4);
	int sum = cmd_h->version + b2i + cmd_h->dataLength + cmd_h->sourceId + 
				cmd_h->targetId + cmd_h->reserve1 + cmd_h->reserve2;
	if(sum == cmd_h->checkSum) return 1;
	else					   return 0;
}
	