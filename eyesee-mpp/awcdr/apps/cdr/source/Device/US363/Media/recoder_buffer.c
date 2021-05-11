/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Media/recoder_buffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

#include "Device/US363/us360.h"
#include "Device/US363/Media/Mp4/mp4.h"
#include "Device/US363/System/sys_time.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::RecBuf"

rec_buf_struct rec_buf;
img_buf_struct img_buf;

//tmp unsigned char rec_img_buf[UVC_BUF_MAX];
unsigned char *rec_img_buf = NULL;

int rec_delay_flag = 0;
unsigned long long rec_delay_timer = 0;        // delay 1s 再開始錄影, 避免錄到切換fpga頻率時的垃圾畫面

int Time_Lapse_flag = 0;
unsigned long long Time_Lapse_time = 0L, Time_Lapse_time_lst = 0L;
unsigned long long Time_Lapse_time_render_lst = 0L, Time_Lapse_time_encoder_lst = 0L;


int malloc_rec_buf() {
	//unsigned char buf[REC_BUF_MAX];
	rec_buf.buf = (unsigned char *)malloc(sizeof(unsigned char) * REC_BUF_MAX);
	if(rec_buf.buf == NULL) goto error;
	return 0;
error:
	db_error("malloc_rec_buf() malloc error!");
	return -1;
}

int malloc_rec_img_buf() {
	//unsigned char rec_img_buf[UVC_BUF_MAX];
	rec_img_buf = (unsigned char *)malloc(sizeof(unsigned char) * UVC_BUF_MAX);
	if(rec_img_buf == NULL) goto error;
	return 0;
error:
	db_error("malloc_rec_img_buf() malloc error!");
	return -1;
}

void free_rec_buf() {
	if(rec_buf.buf != NULL)
		free(rec_buf.buf);
	rec_buf.buf = NULL;
}

void free_rec_img_buf() {
	if(rec_img_buf != NULL)
		free(rec_img_buf);
	rec_img_buf = NULL;
}

int check_rec_time(int state, int ip_f, int type, int tl_mode)
{
    int flag=0;
	unsigned long long tl_ms;

    get_current_usec(&Time_Lapse_time);
    if(state != -2) {
        if(tl_mode == 0) {
            if(rec_delay_timer == 0) {
                rec_delay_flag = 0;
                rec_delay_timer = Time_Lapse_time;
            }
            else if( ( ( (Time_Lapse_time - rec_delay_timer) / 1000) > 1000 && ip_f == 1) || rec_delay_flag == 1) {
                rec_delay_flag = 1;
				get_timelapse_ms(&tl_ms);
                if( ( (Time_Lapse_time - Time_Lapse_time_lst) / 1000) > tl_ms) {
                    flag = 1;
                    Time_Lapse_time_lst = Time_Lapse_time;
                }
            }
        }
        else {    //縮時拿掉delay 1s才寫檔, 避免縮時錄影等第一張I frame
        	if(type == 1)		//H264
				flag = 1;
        	else {				//JPEG
				if(rec_delay_timer == 0) {
					rec_delay_flag = 0;
					rec_delay_timer = Time_Lapse_time;
				}
				else if( (rec_delay_flag == 0 && ip_f == 1) || rec_delay_flag == 1) {
					rec_delay_flag = 1;
					get_timelapse_ms(&tl_ms);
					if( ( (Time_Lapse_time - Time_Lapse_time_lst) / 1000) > tl_ms || tl_ms < 1000) {		//Time_Lapse_ms < 1s: 搭配pipe line安排, 解縮時JPEG掉張問題
						flag = 1;
						Time_Lapse_time_lst = Time_Lapse_time;
					}
				}
			}
        }
    }
    return flag;
}

void set_rec_delay_timer(unsigned long long time) {
	rec_delay_timer = time;
}

void set_timelapse_flag(int flag) {
	Time_Lapse_flag = flag;
}

void timelapse_time_init() {
	Time_Lapse_time = 0;
	Time_Lapse_time_lst = 0;
	Time_Lapse_time_render_lst = 0;
	Time_Lapse_time_encoder_lst = 0;
}

/*
 * idx: 0=encoder 1=render
 */
int check_rec_time2(int idx)
{
    int flag=0;
    unsigned long long now_time=0, tl_ms;
    unsigned long long *lst_time;

    if(idx == 0) lst_time = &Time_Lapse_time_encoder_lst;
    else		 lst_time = &Time_Lapse_time_render_lst;
    get_current_usec(&now_time);
	get_timelapse_ms(&tl_ms);
    if( ( (now_time - *lst_time) / 1000) > tl_ms) {
        flag = 1;
        *lst_time = now_time;
    }
    return flag;
}

void copy_to_rec_buf(unsigned char *img, int len, int ip_frame, char *sps_buf, int sps_len, char *pps_buf, int pps_len, int fenc_type, int fs)
{
    unsigned size = 0, tmp = 0;
    unsigned p1, p2;
    unsigned char *img_p = NULL;
    unsigned char *ptr = NULL;
    unsigned char *size_p = NULL;
	int state = get_rec_state();
db_error("max+ copy_to_rec_buf() 00 state=%d len=%d ip_f=%d\n", state, len, ip_frame);
    if(sps_buf == NULL)
        Time_Lapse_flag = check_rec_time(state, ip_frame, 0, getTimeLapseMode());    //jpeg
    else
        Time_Lapse_flag = check_rec_time(state, ip_frame, 1, getTimeLapseMode());    //h264

    //錄影
//tmp    if( ( (state != -2 && Time_Lapse_flag == 1) || Live360_Buf.state != -2)
//tmp    		&& len < (UVC_BUF_MAX - sizeof(img_buf_struct) ))
    if( ( (state != -2 && Time_Lapse_flag == 1) /*|| Live360_Buf.state != -2*/)
    		&& len < (UVC_BUF_MAX - sizeof(img_buf_struct) ))
    {
        img_p = img;
        img_buf.check_sum = 0x20150819;
        img_buf.size = len + 4;        // +4: data 前 +4byte size
        img_buf.enc_mode = rec_buf.enc_mode;
        img_buf.ip_frame = ip_frame;
        if(img_buf.enc_mode != 0) {
            img_buf.sps_len = sps_len;
            if(sps_len < 32) memcpy(&img_buf.sps_buf[0], sps_buf, sps_len);
            else             db_error("copy_to_rec_buf() sps err! sps_len=%d\n", sps_len);
            img_buf.pps_len = pps_len;
            if(pps_len < 32) memcpy(&img_buf.pps_buf[0], pps_buf, pps_len);
            else             db_error("copy_to_rec_buf() pps err! pps_len=%d\n", pps_len);
        }
        img_buf.frame_stamp = fs;
        ptr = &img_buf;

        p1 = rec_buf.P1;
        p2 = rec_buf.P2;
        tmp = p1 + sizeof(img_buf_struct);
        if(tmp < REC_BUF_MAX) {
            if(p2 > p1 && tmp >= p2) {
                rec_buf.jump = 1;
                db_error("copy_to_rec_buf() 0-00-1 rec_buf.jump == 1 P1 %d P2 %d size %d\n", p1, p2, len);
            }
            //else if(p1 > p2 && tmp >= REC_BUF_MAX && (tmp - REC_BUF_MAX) >= p2) {
            //    rec_buf.jump = 1;
            //    db_error("uvc_thread: 00-2 rec_buf.jump == 1\n");
            //}
            else {
                memcpy(&rec_buf.buf[p1], &ptr[0], sizeof(img_buf_struct));

                //rec_buf.P1 = tmp;
                p1 = tmp;
                rec_buf.jump = 0;
                //rec_buf.enc_mode = 0;

                Time_Lapse_flag = 0;
            }
        }
        else {
            if(p2 > p1 && tmp >= p2) {
                rec_buf.jump = 1;
                db_error("copy_to_rec_buf() 0-01-1 rec_buf.jump == 1\n");
            }
            else if(p1 > p2 && tmp >= REC_BUF_MAX && (tmp - REC_BUF_MAX) >= p2) {
                rec_buf.jump = 1;
                db_error("copy_to_rec_buf() 0-01-2 rec_buf.jump == 1\n");
            }
            else {
                db_debug("p1=%d p2=%d tmp=%d\n", p1, p2, tmp);
                memcpy(&rec_buf.buf[p1], &ptr[0], REC_BUF_MAX - p1);
                memcpy(&rec_buf.buf[0], &ptr[REC_BUF_MAX - p1], p1 + sizeof(img_buf_struct) - REC_BUF_MAX);

                //rec_buf.P1 = (tmp - REC_BUF_MAX);
                p1 = (tmp - REC_BUF_MAX);
                rec_buf.jump = 0;
                //rec_buf.enc_mode = 0;

                Time_Lapse_flag = 0;
            }
        }

        //p1 = rec_buf.P1;
        //p2 = rec_buf.P2;
        tmp = p1 + len + 4;
        if(tmp < REC_BUF_MAX) {
            if(p2 > p1 && tmp >= p2) {
                rec_buf.jump = 1;
                db_error("copy_to_rec_buf() 1-00-1 rec_buf.jump == 1 P1 %d P2 %d size %d\n", p1, p2, len);
            }
            //else if(p1 > p2 && tmp >= REC_BUF_MAX && (tmp - REC_BUF_MAX) >= p2) {
            //    rec_buf.jump = 1;
            //    db_error("uvc_thread: 00-2 rec_buf.jump == 1\n");
            //}
            else {
                size = SWAP32(len);
                //db_debug("[tmp=p1+len+4if]p1=%d p2=%d tmp=%d size=%d len=%d\n", p1, p2, tmp, size, len);
                memcpy(&rec_buf.buf[p1], &size, 4);
                memcpy(&rec_buf.buf[p1+4], &img_p[0], len);

                rec_buf.P1 = tmp;
                rec_buf.jump = 0;
                //rec_buf.enc_mode = 0;

                Time_Lapse_flag = 0;
            }
        }
        else {
            if(p2 > p1 && tmp >= p2) {
                rec_buf.jump = 1;
                db_error("copy_to_rec_buf() 1-01-1 rec_buf.jump == 1\n");
            }
            else if(p1 > p2 && tmp >= REC_BUF_MAX && (tmp - REC_BUF_MAX) >= p2) {
                rec_buf.jump = 1;
                db_error("copy_to_rec_buf() 1-01-2 rec_buf.jump == 1\n");
            }
            else {
                size = SWAP32(len);
                db_debug("[tmp=p1+len+4el]p1=%d p2=%d tmp=%d size=%d len=%d\n", p1, p2, tmp, size, len);
                if( (p1 + 4) < REC_BUF_MAX) {
                    memcpy(&rec_buf.buf[p1], &size, 4);

                    p1 += 4;
                    db_debug("[el]p1=%d w1=%d w2=%d\n", p1, (REC_BUF_MAX - p1), (p1 + len - REC_BUF_MAX));
                    memcpy(&rec_buf.buf[p1], &img_p[0], REC_BUF_MAX - p1);
                    memcpy(&rec_buf.buf[0], &img_p[REC_BUF_MAX - p1], p1 + len - REC_BUF_MAX);
                }
                else {
                    size_p = &size;
                    memcpy(&rec_buf.buf[p1], size_p, REC_BUF_MAX - p1);
                    size_p += (REC_BUF_MAX - p1);
                    memcpy(&rec_buf.buf[0], size_p, p1 + 4 - REC_BUF_MAX);

                    p1 = p1 + 4 - REC_BUF_MAX;
                    memcpy(&rec_buf.buf[p1], &img_p[0], len);
                    //memcpy(&rec_buf.buf[p1], &img_p[0], REC_BUF_MAX - p1);
                    //memcpy(&rec_buf.buf[0], &img_p[REC_BUF_MAX - p1], p1 + len - REC_BUF_MAX);
                }

                rec_buf.P1 = (tmp - REC_BUF_MAX);
                rec_buf.jump = 0;
                //rec_buf.enc_mode = 0;

                Time_Lapse_flag = 0;
            }
        }

    }

    return 0;
}
