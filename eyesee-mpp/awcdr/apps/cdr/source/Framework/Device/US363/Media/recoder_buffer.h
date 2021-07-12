/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __RECODER_BUFFER_H__
#define __RECODER_BUFFER_H__

#ifdef __cplusplus
extern "C" {
#endif

#define REC_BUF_MAX 		0x2800000

typedef struct rec_buf_struct_H
{
	unsigned P1;	// UVC to BUF
	unsigned P2;	// BUF to FILE
	unsigned P3;	// not use
	unsigned jump;
	unsigned enc_mode;	// 0:JPEG 1:A64_H264 2:FPGA_H264
	unsigned f_frame;	// first frame
//tmp	unsigned char buf[REC_BUF_MAX];
	unsigned char *buf;
}rec_buf_struct;
extern rec_buf_struct rec_buf;

typedef struct img_buf_struct_H
{
	unsigned check_sum;		// 0x20150819
	unsigned size;
	unsigned enc_mode;		// 0:MJPEG 1:H264
	unsigned ip_frame;		// H264 i_frame:1 p_frame:0 / MJPEG:1
	unsigned sps_len;
	char     sps_buf[32];
	unsigned pps_len;
	char     pps_buf[32];
	unsigned frame_stamp;
	
	unsigned rev;
}img_buf_struct;
//extern img_buf_struct img_buf;

extern unsigned char *rec_img_buf;


int malloc_rec_buf();
int malloc_rec_img_buf();
void free_rec_buf();
void free_rec_img_buf();
void set_rec_delay_timer(unsigned long long time);
void set_timelapse_flag(int flag);
void timelapse_time_init();
//int check_rec_time2(int idx);
void copy_to_rec_buf(unsigned char *img, int len, int ip_frame, char *sps_buf, int sps_len, char *pps_buf, int pps_len, int fenc_type, int fs);


#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__RECODER_BUFFER_H__