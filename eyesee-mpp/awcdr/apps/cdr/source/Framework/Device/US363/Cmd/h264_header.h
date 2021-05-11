/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __H264_HEADER_H__
#define __H264_HEADER_H__

#ifdef __cplusplus
extern "C" {
#endif

#define swap4(x) ((x &0xFF) << 24) | ((x &0xFF00) << 8) | ((x >> 8) &0xFF00) | ((x >> 24) &0xFF)
#define swap2(x) ((x &0xFF) << 8) | ((x >> 8) &0xFF)

typedef struct FPGA_H264_SPS_PPS_Struct_h {
	int sps_len;
	char sps[16];
	int pps_len;
	char pps[16];
}FPGA_H264_SPS_PPS_Struct;

void Get_FPGA_H264_SPS_PPS(int *sps_len, char *sps, int *pps_len, char *pps);
void MakeH264DataHeaderProc();
void SendH264EncodeTable(void);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__H264_HEADER_H__