/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Media/recoder_thread.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#include "Device/US363/us360.h"
#include "Device/US363/Media/recoder_buffer.h"
#include "Device/US363/Media/Mp4/mp4.h"
#include "Device/US363/Kernel/FPGA_Pipe.h"
#include "Device/US363/Kernel/k_spi_cmd.h"
#include "Device/US363/System/sys_time.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::RecTh"


int rec_thread_en = 1;
pthread_t thread_rec_id;
pthread_mutex_t mut_rec_buf;

int rec_state = -2;                            // 0:star  1:rec  -1:end  -2:not do  -3:close+start
char save_path[128];
struct Cmd_Queue Rec_Cmd_Queue = {0};

void rec_cmd_queue_init() {
	memset(&Rec_Cmd_Queue, 0, sizeof(struct Cmd_Queue) );
}

void set_rec_cmd_queue_p1(int p1) {	
	if(p1 < 0 || p1 >= CMD_QUEUE_MAX) p1 = 0;
	Rec_Cmd_Queue.P1 = p1;
db_debug("set_rec_cmd_queue_p1() p1=%d\n", Rec_Cmd_Queue.P1);	
}

int get_rec_cmd_queue_p1() {
	return Rec_Cmd_Queue.P1;
}

void set_rec_cmd_queue_p2(int p2) {
	if(p2 < 0 || p2 >= CMD_QUEUE_MAX) p2 = 0;
	Rec_Cmd_Queue.P2 = p2;
db_debug("set_rec_cmd_queue_p2() p2=%d\n", Rec_Cmd_Queue.P2);	
}

int get_rec_cmd_queue_p2() {
	return Rec_Cmd_Queue.P2;
}

void set_rec_cmd_queue_state(int p, int state) {
	if(p < 0 || p >= CMD_QUEUE_MAX) return;
	Rec_Cmd_Queue.queue[p] = state;
db_debug("set_rec_cmd_queue_state() state[%d]=%d\n", p, Rec_Cmd_Queue.queue[p]);	
}

int get_rec_cmd_queue_state(int p) {
	if(p < 0 || p >= CMD_QUEUE_MAX) return;
	return Rec_Cmd_Queue.queue[p];
}

void rec_start_init(int res, int time_lapse, int freq, int fpga_enc)
{
    rec_buf.P1   = 0;
    rec_buf.P2   = 0;
    rec_buf.P3   = 0;
    rec_buf.jump = 0;
    rec_buf.f_frame = 1;
    if(time_lapse != 0) {
    	//if(fpga_enc == 1)	//H264
    		rec_buf.enc_mode = 2;
    	//else				//JPEG
    	//	rec_buf.enc_mode = 0;
    }
    else {
    	//if(res == 2 || res == 13 || res == 14)
			rec_buf.enc_mode = 1;
		//else
		//	rec_buf.enc_mode = 0;
    }
    memset(&rec_buf.buf[0], 0, REC_BUF_MAX);

    //pcm_buf.fps = fps;
    if(freq == 0)
        pcm_buf.fps = 300;
    else
        pcm_buf.fps = 250;
    pcm_buf.P1 = 0;
    pcm_buf.P2 = 0;
    pcm_buf.P3 = 0;
    pcm_buf.jump = 0;
//    if(freq == 0)
//        pcm_buf.size = 2940;    //300 * 2940 / fps;
//    else
//        pcm_buf.size = 3528;    //250 * 2940 / fps;
    memset(&pcm_buf.buf[0], 0, PCM_BUF_MAX);

    Time_Lapse_Init(time_lapse, res);
    return;
}

/*
 * 一般錄影時, Buffer內超過1M的資料未寫入檔案, 則增加for迴圈次數, 以加速寫入檔案
 */
int get_rec_times(int p1, int p2, int tl_m) {
	int sub = 0, times = 1;

	if(tl_m != 0) {
		times = 1;
	}
	else {
		if(p1 >= p2) {
			sub = p1 - p2;
		}
		else {
			sub = REC_BUF_MAX - p2;
			sub += p1;
		}
		if(sub > 0x100000) times = 7; 		// > 1MB
		else			   times = 1;
	}
	return times;
}

void set_rec_state(int state) {
	rec_state = state;
}

int get_rec_state() {
	return rec_state;
}

void set_rec_thread_en(int en) {
	rec_thread_en = en;
}

int get_rec_thread_en() {
	return rec_thread_en;
}

#ifdef __CLOSE_CODE__ 	//tmp
int save_avi_file(unsigned char *video_buf, unsigned char *audio_buf, int len, int audio_len,
        int en, int lose, unsigned long long *freesize, int resWidth, int resHeight, int fps, int enc_mode)
{
    int i = 0, j = 0, ret = 0;

    if(en == 0){
        maek_save_file_path(1, save_path, sd_path, mSSID, cap_file_cnt);

/*        rec2thm.en[rec2thm.P1] = 1;         // 製作avi錄影縮圖
        rec2thm.mode[rec2thm.P1] = 1;       // CAP=0, REC=1, Lapse=2, HDR=3, RAW=4
        memcpy(&rec2thm.path[rec2thm.P1][0], &save_path[0], 128);
        rec2thm.P1++;
        if(rec2thm.P1 >= REC_2_THM_BUF_MAX) rec2thm.P1 = 0;*/

//        cap_file_cnt++;                                // max+s 160223, saveAVI
//        if(cap_file_cnt > 9999) cap_file_cnt = 0;
//        save_parameter_flag = 1;
//        set_A2K_Cap_Cnt(cap_file_cnt);
    }

    if(lose == 1){
        ret = jpeg2avi(fps, save_path, NULL, len, audio_buf, audio_len, en, freesize, resWidth, resHeight, enc_mode);
    }
    else{
        ret = jpeg2avi(fps, save_path, video_buf, len, audio_buf, audio_len, en, freesize, resWidth, resHeight, enc_mode);
    }

    if((video_f_cnt / (fps / 10) ) >= REC_PER_FILE_TIME) {
        db_debug("saveAVI() REC_PER_FILE_TIME\n\0");
        return -1;
    }
    else if(ret == -1 || ret == -2) {
        db_debug("saveAVI over free size!\n\0");
        return -2;
    }
    else if(ret == -3) {
        db_debug("saveAVI rec file size > REC_PER_FILE_SIZE\n\0");
        return -3;
    }

    return 0;
}
#endif	//__CLOSE_CODE__

int save_mp4_file(unsigned char *video_buf, unsigned char *audio_buf, int len, int audio_len, int en, int lose, unsigned long long *freesize,
        int resWidth, int resHeight, int fps, int enc_mode, int ip_f, int play_mode, char *sps, int sps_len, char *pps, int pps_len,
        int tl_mode, int a_delay_t, int a_src)
{
    int i = 0, j = 0, ret = 0;
    static int v_cnt = 0;
    char *ptr;
    char path[128];
    int enc_type=0;

    if(en == 0){
        v_cnt = 0;

        maek_save_file_path(6, save_path, sd_path, mSSID, cap_file_cnt);

        ptr = &save_path[0];
        for(i = 0; i < 128; i++) {
            if(*ptr == '.' && *(ptr+1) == 'm' && *(ptr+2) == 'p' && *(ptr+3) == '4')
                break;
            ptr++;
        }
        memset(path, 0, 128);
        memcpy(path, &save_path[0], i);

        if(tl_mode != 0) {
            rec2thm.en[rec2thm.P1] = 1;         // 製作縮時錄影縮圖
            rec2thm.mode[rec2thm.P1] = 5;       // CAP=0, REC=1, Lapse=2, HDR=3, RAW=4, Lapse H264=5
            memcpy(&rec2thm.path[rec2thm.P1][0], &path[0], 128);
            rec2thm.P1++;
            if(rec2thm.P1 >= REC_2_THM_BUF_MAX) rec2thm.P1 = 0;
        }

        db_debug("save_mp4_file: thmP1=%d path=%s  ip_f=%d buf[0]=0x%02x buf[1]=0x%02x buf[2]=0x%02x buf[3]=0x%02x buf[4]=0x%02x buf[5]=0x%02x buf[6]=0x%02x buf[7]=0x%02x\n",
        		rec2thm.P1, path, ip_f, *video_buf, *(video_buf+1), *(video_buf+2), *(video_buf+3), *(video_buf+4), *(video_buf+5), *(video_buf+6), *(video_buf+7) );
    }

    //if(enc_mode == 1 || enc_mode == 2)	//H264
    	enc_type = 1;
    //else									//JPEG
    //	enc_type = 0;
    if(lose == 1){
        ret = save_mp4_proc(save_path,      NULL, len, audio_buf, audio_len, resWidth, resHeight, fps, ip_f, enc_type, en, freesize,	\
        		play_mode, &v_cnt, sps, sps_len, pps, pps_len, a_delay_t, a_src);
    }
    else{
        ret = save_mp4_proc(save_path, video_buf, len, audio_buf, audio_len, resWidth, resHeight, fps, ip_f, enc_type, en, freesize, 	\
        		play_mode, &v_cnt, sps, sps_len, pps, pps_len, a_delay_t, a_src);
    }

    if((v_cnt / (fps / 10) ) >= REC_PER_FILE_TIME) {	//Over File Time
        db_debug("save_mp4_file: REC_PER_FILE_TIME\n\0");
        return -1;
    }
    else if(ret == -1 || ret == -2) {					//-1:Snd -2:Img, Save Snd/Img Over Free Size
        db_debug("save_mp4_file: over free size!\n\0");
        return -2;
    }
    else if(ret == -3) {								//Over File Size
        db_debug("save_mp4_file: rec file size > REC_PER_FILE_SIZE\n\0");
        return -3;
    }
    else if(ret == -4) {								//Write File Error
    	db_error("save_mp4_file: Write File Error!\n\0");
    	return -4;
    }

    return 0;
}

void *rec_thread(void)
{
    int i, j;
    static unsigned long long curTime, lstTime=0, runTime, defTime;
    static int state_lst = -2;		//解沒有開始寫檔, 卻結束錄影, 導致寫檔錯誤問題
    unsigned size;
    unsigned v_p1, v_p2;
    unsigned a_p1, a_p2;
    int save_flag;
    unsigned checksum = 0, P2_tmp = 0, timestamp = -1;
    unsigned char *ptr=NULL, *img_ptr=NULL, *buf=NULL, *abuf=NULL;
    unsigned char buf_tmp;
    img_buf_struct img_tmp;
    int pcm_size, pcm_cnt, pcm_fps, pcm_p2_tmp;
    int isVideo = 0, isAudio = 0;
    int cmd_p1, cmd_p2, cmd_d;
    int close_run = 0, saveSize = 0, saveFPS = 0;
    int ip_frame;
    int mode, res;
    int mSPS_len, mPPS_len;
    char mSPS_buf[16], mPPS_buf[16];
    FILE *fp;
    int fp2fd;
    struct stat sti;
    static char tl_dir_path[128], tl_path[128];
    static int time_lapse_cnt=0;
    int freq = get_ISP_AEG_EP_IMX222_Freq();
    int frame_s, wait_i_frame = 0;		//wait_i_frame: FPGA H264 掉張, 須等下一張I Frame
    static int frame_s_lst = -1;
    int err_cnt=0, err_len=0;
    int resWidth, resHeight;
    int len, ret;
    static int cap_file_cnt_tmp=0;
    int M_Mode, S_Mode;
//tmp    LinkADTSFixheader fix;
//tmp    LinkADTSVariableHeader var;
    char path[128];
    int rec_times = 1;
	static int rec_fps = 60;
	int tl_mode;
	
    //Cmd_Buf_Init();
    while(rec_thread_en)
    {
		tl_mode = getTimeLapseMode();
        get_current_usec(&curTime);
        if(curTime < lstTime) lstTime = curTime;    // rex+ 151229, 防止例外錯誤
        else if((curTime - lstTime) >= 1000000){
        	//db_debug("rec_thread: runTime=%d rec_state=%d saveSize=%d saveFPS=%d\n", (int)runTime, rec_state, saveSize, saveFPS);
            lstTime = curTime;
            runTime = defTime;
            saveSize = 0;
            saveFPS = 0;
            set_AE_adj_tout(rec_state, tl_mode);
        }

        get_Stitching_Out(&mode, &res);
        rec_times = get_rec_times(rec_buf.P1, rec_buf.P2, tl_mode);
        for(j = 0; j < rec_times; j++) {
			cmd_p1 = get_rec_cmd_queue_p1();
			cmd_p2 = get_rec_cmd_queue_p2();
			if(cmd_p1 != cmd_p2){
				cmd_d = get_rec_cmd_queue_state(cmd_p2);
db_debug("rec_thread: cmd change (cmd=%d)\n", cmd_d);				
				if(cmd_d == 0){                                    // 0:start
					if(rec_state == -2){                        // rec_state - 0:star 1:ing -1:end -2:not do
						rec_start_init(res, tl_mode, freq, get_fpga_encode_type());
						Get_M_Mode(&M_Mode, &S_Mode);
						pcm_buf_init(freq, M_Mode, get_mic_is_alive(), 0);
						rec_state = cmd_d;
						if(tl_mode == 0) pcm_buf.state = cmd_d;
					}
					set_rec_cmd_queue_p2(++cmd_p2);
				}
				else if(cmd_d == -1){                            // stop
					if(rec_state == 0 || rec_state == 1){
						rec_state = cmd_d;
						if(tl_mode == 0) pcm_buf.state = cmd_d;
					}
					set_rec_cmd_queue_p2(++cmd_p2);
				}
			}

			if(rec_state != -2)
			{			
				if(rec_state == 0)
					freq = get_ISP_AEG_EP_IMX222_Freq();

				buf = NULL;
				isVideo = 0; isAudio = 0;
				size = 0; pcm_size = 0; close_run = 0;
				v_p1 = rec_buf.P1; v_p2 = rec_buf.P2;
				a_p1 = pcm_buf.P1; a_p2 = pcm_buf.P2;

				if(rec_state == 0) {
					rec_fps = get_RecFPS();
				}

				if(v_p1 != v_p2) {
					ptr = &checksum;
					if( (v_p2 + 4) < REC_BUF_MAX)
						memcpy(&ptr[0], &rec_buf.buf[v_p2], 4);
					else {
						memcpy(&ptr[0], &rec_buf.buf[v_p2], (REC_BUF_MAX - v_p2) );
						memcpy(&ptr[REC_BUF_MAX - v_p2], &rec_buf.buf[0], (v_p2 + 4 - REC_BUF_MAX) );
					}
					if(checksum == 0x20150819) isVideo = 1;
				}

				/*if(v_p1 != v_p2) {
					ptr = &rec_buf.buf[v_p2];
					memcpy(&checksum, ptr, 4);
					if(checksum == 0x20150819) isVideo = 1;
				}*/

				if(tl_mode == 0 && pcm_buf.fps >= 50) {
#ifdef __CLOSE_CODE__	//tmp					
					if(muxer_type == MUXER_TYPE_TS) {
						abuf = malloc(AAC_BUF_LEN);
						if(abuf != NULL) {
							isAudio = read_aac_buf(&pcm_buf, abuf, &fix, &var);
							pcm_size = var.aac_frame_length;
						}
						else
							db_error("rec_thread() abuf malloc err!\n");
					}
					else
#endif	//__CLOSE_CODE__						
					{
						if(freq == 0) pcm_size = (pcm_buf.size * 300 / rec_fps) & 0xFFFE;    //pcm_buf.fps;
						else          pcm_size = (pcm_buf.size * 250 / rec_fps) & 0xFFFE;    //pcm_buf.fps;
						abuf = malloc(pcm_size);
						if(abuf != NULL)
							isAudio = read_pcm_buf(&pcm_buf, abuf, pcm_size);
						else
							db_error("rec_thread() abuf malloc err!\n");
					}
				}

				if(rec_state == -1 || rec_state == -3) close_run = 1;
				if(isAudio == 1 || ( (tl_mode != 0 || pcm_buf.fps < 50 || audio_rate == -1) && isVideo == 1) || close_run == 1)
				{
					switch(rec_state){
					case 0:        // start
db_error("max+ rec_thread() rec start\n");					
						rec_buf.f_frame = 1;
						if(isVideo == 1) {
							img_ptr = &img_tmp;
							memset(img_ptr, 0, sizeof(img_buf_struct) );
							if( (v_p2 + sizeof(img_buf_struct) ) < REC_BUF_MAX) {
								memcpy(img_ptr, &rec_buf.buf[v_p2], sizeof(img_buf_struct) );
								size = img_tmp.size;
								ip_frame = img_tmp.ip_frame;
								frame_s = img_tmp.frame_stamp;
								mSPS_len = img_tmp.sps_len;
								memcpy(&mSPS_buf[0], &img_tmp.sps_buf[0], img_tmp.sps_len);
								mPPS_len = img_tmp.pps_len;
								memcpy(&mPPS_buf[0], &img_tmp.pps_buf[0], img_tmp.pps_len);
								P2_tmp = v_p2 + sizeof(img_buf_struct);
							}
							else {
								memcpy(img_ptr, &rec_buf.buf[v_p2], REC_BUF_MAX - v_p2);
								memcpy(img_ptr + (REC_BUF_MAX - v_p2), &rec_buf.buf[0] , v_p2 + sizeof(img_buf_struct) - REC_BUF_MAX);
								size = img_tmp.size;
								ip_frame = img_tmp.ip_frame;
								frame_s = img_tmp.frame_stamp;
								mSPS_len = img_tmp.sps_len;
								memcpy(&mSPS_buf[0], &img_tmp.sps_buf[0], img_tmp.sps_len);
								mPPS_len = img_tmp.pps_len;
								memcpy(&mPPS_buf[0], &img_tmp.pps_buf[0], img_tmp.pps_len);
								P2_tmp = v_p2 + sizeof(img_buf_struct) - REC_BUF_MAX;
							}

							if(rec_buf.f_frame == 0 || img_tmp.enc_mode == 0 ||
									(rec_buf.f_frame == 1 && img_tmp.enc_mode != 0 && img_tmp.ip_frame == 1) )
							{
								rec_buf.f_frame = 0;
								buf = &rec_img_buf[0];    //malloc(size);
								if( (P2_tmp + size) < REC_BUF_MAX)
									memcpy(&buf[0], &rec_buf.buf[P2_tmp], size);
								else {
									memcpy(&buf[0], &rec_buf.buf[P2_tmp], REC_BUF_MAX - P2_tmp);
									memcpy(&buf[REC_BUF_MAX - P2_tmp], &rec_buf.buf[0], P2_tmp + size - REC_BUF_MAX);
								}
							}
						}

						get_Resolution_WidthHeight(&resWidth, &resHeight);
						if(rec_buf.f_frame == 0) {
							if(tl_mode == 0 || (tl_mode != 0 && (get_fpga_encode_type() == 1 || get_fpga_encode_type() == 2) ) ) {
								wait_i_frame = 0;
								frame_s_lst = frame_s;
#ifdef __CLOSE_CODE__	//tmp								
								if(muxer_type == MUXER_TYPE_TS) {
									maek_save_file_path(6, save_path, sd_path, mSSID, cap_file_cnt);
									ptr = &save_path[0];
									for(i = 0; i < 128; i++) {
										if(*ptr == '.' && *(ptr+1) == 't' && *(ptr+2) == 's')
											break;
										ptr++;
									}

									memset(path, 0, 128);
									memcpy(path, &save_path[0], i);
									if(tl_mode != 0) {
										rec2thm.en[rec2thm.P1] = 1;         // 製作縮時錄影縮圖
										rec2thm.mode[rec2thm.P1] = 5;       // CAP=0, REC=1, Lapse=2, HDR=3, RAW=4, Lapse H264=5
										memcpy(&rec2thm.path[rec2thm.P1][0], &path[0], 128);
										rec2thm.P1++;
										if(rec2thm.P1 >= REC_2_THM_BUF_MAX) rec2thm.P1 = 0;
									}

									timestamp = -1;
									save_flag = muxer_ts_proc(rec_state, buf, size, abuf, pcm_size, &mSPS_buf[0], mSPS_len, &mPPS_buf[0],
													mPPS_len, ip_frame, rec_fps, tl_mode, pcm_buf.delayTime/1000, &save_path[0],
													&sd_freesize, &timestamp, 0, NULL, &fix, &var);
								}
								else 
#endif	//__CLOSE_CODE__									
								{
									save_flag = save_mp4_file(buf, abuf, size, pcm_size, rec_state, 0, &sd_freesize, resWidth, resHeight,
													rec_fps, rec_buf.enc_mode, ip_frame, mode, &mSPS_buf[0], mSPS_len, &mPPS_buf[0], mPPS_len,
													tl_mode, pcm_buf.delayTime/1000, get_mic_is_alive());
								}
								saveSize += size;
								saveFPS++;
							}
							else {
								db_debug("rec_thread: Time Lapse !\n");
								time_lapse_cnt = 0;

								maek_save_file_path(11, save_path, sd_path, mSSID, cap_file_cnt);

								ptr = &save_path[0];
								for(i = 0; i < 128; i++) {
									if(*ptr == '.' && *(ptr+1) == 'm' && *(ptr+2) == 'p' && *(ptr+3) == '4')
										break;
									ptr++;
								}
								memset(tl_dir_path, 0, 128);
								memcpy(tl_dir_path, &save_path[0], i);

								rec2thm.en[rec2thm.P1] = 1;         // 製作縮時錄影縮圖
								rec2thm.mode[rec2thm.P1] = 2;       // CAP=0, REC=1, Lapse=2, HDR=3, RAW=4, Lapse H264=5
								memcpy(&rec2thm.path[rec2thm.P1][0], &tl_dir_path[0], 128);
								rec2thm.P1++;
								if(rec2thm.P1 >= REC_2_THM_BUF_MAX) rec2thm.P1 = 0;

								if(stat(tl_dir_path, &sti) != 0) {
									if(mkdir(tl_dir_path, S_IRWXU) != 0)
										db_error("rec_thread: create %s folder fail\n", tl_dir_path);
								}
								memset(&tl_path[0], 0, 128);
								check_tl_path_repeat(&tl_path[0], &tl_dir_path[0], &mSSID[0], cap_file_cnt, time_lapse_cnt);
								fp = fopen(tl_path, "wb");
								len = Add_Panorama_Header(resWidth, resHeight, &buf[4], fp);
								if(fp != NULL) {
									err_cnt=0; err_len=0;
									del_jpeg_error_code(&buf[4], size-4, &err_cnt, &err_len);

									if(len >= 0) {
										if(err_cnt == 0)
											ret = fwrite(&buf[4+len], (size - 4 - len), 1, fp);
										else {
											ret = fwrite(&buf[4+len], (size - 4 - len - err_len), 1, fp);
											ret = fwrite(&buf[size - err_len + err_cnt], (err_len - err_cnt), 1, fp);
										}
									}
									else {
										if(err_cnt == 0)
											ret = fwrite(&buf[4], (size - 4), 1, fp);
										else {
											ret = fwrite(&buf[4], (size - 4 - err_len), 1, fp);
											ret = fwrite(&buf[size - err_len + err_cnt], (err_len - err_cnt), 1, fp);
										}
									}
									time_lapse_cnt++;
									saveSize += size;

									if(ret == 0)
										save_flag = -4;
									else if(sd_freesize < (size-4) || time_lapse_cnt >= 9999)
										save_flag = -2;
									else {
										sd_freesize -= (size-4);
										save_flag = 0;
									}

									fflush(fp);
									fp2fd = fileno(fp);
									fsync(fp2fd);
									fclose(fp);
									close(fp2fd);
								}

								cap_file_cnt_tmp = cap_file_cnt;
	/*                            cap_file_cnt++;                                // max+s 160223, saveTimeLapse
								if(cap_file_cnt > 9999) cap_file_cnt = 0;
								save_parameter_flag = 1;
								set_A2K_Cap_Cnt(cap_file_cnt);*/
							}
						}

						if(isVideo == 1) {
							size += sizeof(img_buf_struct);        // += 64 bytes
							if( (v_p2 + size) >= REC_BUF_MAX)
								rec_buf.P2 = v_p2 + size - REC_BUF_MAX;
							else
								rec_buf.P2 = v_p2 + size;
						}

						if(rec_buf.f_frame == 0) {
							rec_state = 1;
						}
						state_lst = rec_state;
						break;
					case 1:        // rec
						if(isVideo == 1) {
							img_ptr = &img_tmp;
							memset(img_ptr, 0, sizeof(img_buf_struct) );
							if( (v_p2 + sizeof(img_buf_struct) ) < REC_BUF_MAX) {
								memcpy(img_ptr, &rec_buf.buf[v_p2], sizeof(img_buf_struct) );
								size = img_tmp.size;
								ip_frame = img_tmp.ip_frame;
								frame_s = img_tmp.frame_stamp;
								if(get_fpga_encode_type() == 1 && img_tmp.enc_mode == 2) {			//FPGA H264
									if(abs(frame_s - frame_s_lst) != 1) {
										wait_i_frame = 1;
										db_error("rec_thread() rec01: fs=%d fs_lst=%d\n", frame_s, frame_s_lst);
									}
									frame_s_lst = frame_s;
								}
								else
									wait_i_frame = 0;
								mSPS_len = img_tmp.sps_len;
								memcpy(&mSPS_buf[0], &img_tmp.sps_buf[0], img_tmp.sps_len);
								mPPS_len = img_tmp.pps_len;
								memcpy(&mPPS_buf[0], &img_tmp.pps_buf[0], img_tmp.pps_len);
								P2_tmp = v_p2 + sizeof(img_buf_struct);
							}
							else {
								memcpy(img_ptr, &rec_buf.buf[v_p2], REC_BUF_MAX - v_p2);
								memcpy(img_ptr + (REC_BUF_MAX - v_p2), &rec_buf.buf[0] , v_p2 + sizeof(img_buf_struct) - REC_BUF_MAX);
								size = img_tmp.size;
								ip_frame = img_tmp.ip_frame;
								frame_s = img_tmp.frame_stamp;
								if(get_fpga_encode_type() == 1 && img_tmp.enc_mode == 2) {			//FPGA H264
									if(abs(frame_s - frame_s_lst) != 1) {
										wait_i_frame = 1;
										db_error("rec_thread() rec02: fs=%d fs_lst=%d\n", frame_s, frame_s_lst);
									}
									frame_s_lst = frame_s;
								}
								else
									wait_i_frame = 0;
								mSPS_len = img_tmp.sps_len;
								memcpy(&mSPS_buf[0], &img_tmp.sps_buf[0], img_tmp.sps_len);
								mPPS_len = img_tmp.pps_len;
								memcpy(&mPPS_buf[0], &img_tmp.pps_buf[0], img_tmp.pps_len);
								P2_tmp = v_p2 + sizeof(img_buf_struct) - REC_BUF_MAX;
							}

							if(rec_buf.f_frame == 0 || img_tmp.enc_mode == 0 ||
									(rec_buf.f_frame == 1 && img_tmp.enc_mode != 0 && img_tmp.ip_frame == 1) )
							{
								if(rec_buf.f_frame == 1)
									db_debug("rec_thread() run ip_f %d size %d\n", img_tmp.ip_frame, size);

								rec_buf.f_frame = 0;
								buf = &rec_img_buf[0];
								if( (P2_tmp + size) < REC_BUF_MAX)
									memcpy(&buf[0], &rec_buf.buf[P2_tmp], size);
								else {
									memcpy(&buf[0], &rec_buf.buf[P2_tmp], REC_BUF_MAX - P2_tmp);
									memcpy(&buf[REC_BUF_MAX - P2_tmp], &rec_buf.buf[0], P2_tmp + size - REC_BUF_MAX);
								}
							}
						}

						get_Resolution_WidthHeight(&resWidth, &resHeight);
						if(tl_mode == 0 || (tl_mode != 0 && (get_fpga_encode_type() == 1 || get_fpga_encode_type() == 2) ) ) {
							if(wait_i_frame == 1 && ip_frame == 1)
								wait_i_frame = 0;
							if(wait_i_frame == 0) {
#ifdef __CLOSE_CODE__	//tmp								
								if(muxer_type == MUXER_TYPE_TS) {
									timestamp = -1;
									save_flag = muxer_ts_proc(rec_state, buf, size, abuf, pcm_size, &mSPS_buf[0], mSPS_len, &mPPS_buf[0],
													mPPS_len, ip_frame, rec_fps, tl_mode, pcm_buf.delayTime/1000, &save_path[0],
													&sd_freesize, &timestamp, 0, NULL, &fix, &var);
								}
								else 
#endif	//__CLOSE_CODE__									
								{
									save_flag = save_mp4_file(buf, abuf, size, pcm_size, rec_state, rec_buf.jump, &sd_freesize, resWidth, resHeight,
													rec_fps, rec_buf.enc_mode, ip_frame, mode, &mSPS_buf[0], mSPS_len, &mPPS_buf[0], mPPS_len,
													tl_mode, pcm_buf.delayTime/1000, get_mic_is_alive());												
								}
								saveSize += size;
								saveFPS++;
							}
						}
						else {
							memset(&tl_path[0], 0, 128);
							check_tl_path_repeat(&tl_path[0], &tl_dir_path[0], &mSSID[0], cap_file_cnt, time_lapse_cnt);
							fp = fopen(tl_path, "wb");
							len = Add_Panorama_Header(resWidth, resHeight, &buf[4], fp);
							if(fp != NULL) {
								err_cnt=0; err_len=0;
								del_jpeg_error_code(&buf[4], size-4, &err_cnt, &err_len);

								if(len >= 0) {
									if(err_cnt == 0)
										ret = fwrite(&buf[4+len], (size - 4 - len), 1, fp);
									else {
										ret = fwrite(&buf[4+len], (size - 4 - len - err_len), 1, fp);
										ret = fwrite(&buf[size - err_len + err_cnt], (err_len - err_cnt), 1, fp);
									}
								}
								else {
									if(err_cnt == 0)
										ret = fwrite(&buf[4], (size - 4), 1, fp);
									else {
										ret = fwrite(&buf[4], (size - 4 - err_len), 1, fp);
										ret = fwrite(&buf[size - err_len + err_cnt], (err_len - err_cnt), 1, fp);
									}
								}
								time_lapse_cnt++;
								saveSize += size;

								if(ret == 0)
									save_flag = -4;
								else if(sd_freesize < (size-4) || time_lapse_cnt >= 9999)
									save_flag = -2;
								else {
									sd_freesize -= (size-4);
									save_flag = 0;
								}

								fflush(fp);
								fp2fd = fileno(fp);
								fsync(fp2fd);
								fclose(fp);
								close(fp2fd);
							}
						}

						if(isVideo == 1) {
							size += sizeof(img_buf_struct);
							if( (v_p2 + size) >= REC_BUF_MAX)
								rec_buf.P2 = v_p2 + size - REC_BUF_MAX;
							else
								rec_buf.P2 = v_p2 + size;

							v_p2 = rec_buf.P2;
							if(rec_buf.jump == 1 && tl_mode == 0)  {
								ptr = &rec_buf.buf[v_p2];
								memcpy(&checksum, ptr, 4);
								if(checksum != 0x20150819) {
									db_error("rec_thread: checksum=%x\n", checksum);
								}
								else {
									isAudio = read_pcm_buf(&pcm_buf, abuf, pcm_size);
									if(isAudio == 1) {
#ifdef __CLOSE_CODE__	//tmp											
										if(muxer_type == MUXER_TYPE_TS) {
											timestamp = -1;
											save_flag = muxer_ts_proc(rec_state, NULL, size, abuf, pcm_size, &mSPS_buf[0], mSPS_len, &mPPS_buf[0],
															mPPS_len, ip_frame, rec_fps, tl_mode, pcm_buf.delayTime/1000, &save_path[0],
															&sd_freesize, &timestamp, 1, NULL, &fix, &var);
										}
										else 
#endif	//__CLOSE_CODE__										
										{
											save_flag = save_mp4_file(NULL, abuf, size, pcm_size, rec_state, rec_buf.jump, &sd_freesize, resWidth, resHeight,
															rec_fps, rec_buf.enc_mode, ip_frame, mode, &mSPS_buf[0], mSPS_len, &mPPS_buf[0], mPPS_len,
															tl_mode, pcm_buf.delayTime/1000, get_mic_is_alive());														
										}
										saveSize += size;
										saveFPS++;
									}

									size = ((img_buf_struct*)ptr)->size;
									size += sizeof(img_buf_struct);
									if( (v_p2 + size) >= REC_BUF_MAX)
										rec_buf.P2 = v_p2 + size - REC_BUF_MAX;
									else
										rec_buf.P2 = v_p2 + size;
								}
								rec_buf.jump = 0;
							}
						}

						if(save_flag == -1 || save_flag == -3) {	//Over File Time / Over File Size
							rec_state = -3;
						}
						else if(save_flag == -2) {					//Over Free Size
							if(DrivingRecord_Mode == 0) {	//DrivingRecord_Mode: Off
								rec_state = -1;
								if(pcm_buf.state == 1)
									pcm_buf.state = -1;
							}
							else							//DrivingRecord_Mode: On
								rec_state = -3;
						}
						else if(save_flag == -4) {					//Write File Error
							rec_state = -1;
							set_sd_card_state(3);
							set_write_file_error(1);
						}
						break;
					case -1:    // stop
					case -3:    // stop + restart
db_error("max+ rec_thread() rec stop\n");						
						if(isVideo == 1) {
							img_ptr = &img_tmp;
							memset(img_ptr, 0, sizeof(img_buf_struct) );
							if( (v_p2 + sizeof(img_buf_struct) ) < REC_BUF_MAX) {
								//size = ((img_buf_struct*)ptr)->size;
								memcpy(img_ptr, &rec_buf.buf[v_p2], sizeof(img_buf_struct) );
								size = img_tmp.size;
								ip_frame = img_tmp.ip_frame;
								frame_s = img_tmp.frame_stamp;
								mSPS_len = img_tmp.sps_len;
								memcpy(&mSPS_buf[0], &img_tmp.sps_buf[0], img_tmp.sps_len);
								mPPS_len = img_tmp.pps_len;
								memcpy(&mPPS_buf[0], &img_tmp.pps_buf[0], img_tmp.pps_len);
								P2_tmp = v_p2 + sizeof(img_buf_struct);
							}
							else {
								memcpy(img_ptr, &rec_buf.buf[v_p2], REC_BUF_MAX - v_p2);
								memcpy(img_ptr + (REC_BUF_MAX - v_p2), &rec_buf.buf[0] , v_p2 + sizeof(img_buf_struct) - REC_BUF_MAX);
								size = img_tmp.size;
								ip_frame = img_tmp.ip_frame;
								frame_s = img_tmp.frame_stamp;
								mSPS_len = img_tmp.sps_len;
								memcpy(&mSPS_buf[0], &img_tmp.sps_buf[0], img_tmp.sps_len);
								mPPS_len = img_tmp.pps_len;
								memcpy(&mPPS_buf[0], &img_tmp.pps_buf[0], img_tmp.pps_len);
								P2_tmp = v_p2 + sizeof(img_buf_struct) - REC_BUF_MAX;
							}

							if(rec_buf.f_frame == 0 || img_tmp.enc_mode == 0 ||
									(rec_buf.f_frame == 1 && img_tmp.enc_mode != 0 && img_tmp.ip_frame == 1) )
							{
								rec_buf.f_frame = 0;
								buf = &rec_img_buf[0];
								if( (P2_tmp + size) < REC_BUF_MAX)
									memcpy(&buf[0], &rec_buf.buf[P2_tmp], size);
								else {
									memcpy(&buf[0], &rec_buf.buf[P2_tmp], REC_BUF_MAX - P2_tmp);
									memcpy(&buf[REC_BUF_MAX - P2_tmp], &rec_buf.buf[0], P2_tmp + size - REC_BUF_MAX);
								}
							}
						}

						get_Resolution_WidthHeight(&resWidth, &resHeight);
						if( (tl_mode == 0 || (tl_mode != 0 && (get_fpga_encode_type() == 1 || get_fpga_encode_type() == 2) ) ) && state_lst == 1) {
#ifdef __CLOSE_CODE__	//tmp							
							if(muxer_type == MUXER_TYPE_TS) {
								timestamp = -1;
								save_flag = muxer_ts_proc(rec_state, buf, size, abuf, pcm_size, &mSPS_buf[0], mSPS_len, &mPPS_buf[0],
												mPPS_len, ip_frame, rec_fps, tl_mode, pcm_buf.delayTime/1000, &save_path[0],
												&sd_freesize, &timestamp, 0, NULL, &fix, &var);
							}
							else 
#endif	//__CLOSE_CODE__							
							{
								save_flag = save_mp4_file(buf, abuf, size, pcm_size, -1, 0, &sd_freesize, resWidth, resHeight,
												rec_fps, rec_buf.enc_mode, ip_frame, mode, &mSPS_buf[0], mSPS_len, &mPPS_buf[0], mPPS_len,
												tl_mode, pcm_buf.delayTime/1000, get_mic_is_alive());											
							}
							saveSize += size;
							saveFPS++;
						}
						else {
							//Time Lapse JPEG
						}

						if(isVideo == 1) {
							size += sizeof(img_buf_struct);
							if( (v_p2 + size) >= REC_BUF_MAX)
								rec_buf.P2 = v_p2 + size - REC_BUF_MAX;
							else
								rec_buf.P2 = v_p2 + size;
						}

						if(rec_state == -1) {
							set_rec_delay_timer(0);
							timelapse_time_init();
							rec_state = -2;
							set_fpga_encode_type(0);
						}
						else {
							if(DrivingRecord_Mode == 1 && sd_freesize < SD_CARD_MIN_SIZE) {
								doDrivingModeDeleteFile();
							}

							//取得 SD 卡剩餘空間
							getSDFreeSize(&sd_freesize);		//get_sd_free_size(sd_path);
							if(sd_freesize < SD_CARD_MIN_SIZE) {
								rec_state = -2;
								set_fpga_encode_type(0);
							}
							else {
								rec_state = 0;
								if(tl_mode == 0)	//解錄影檔案循環, 縮圖只有一張問題
									set_A2K_do_Rec_Thm_CMD(1);
							}
						}

						cap_file_cnt++;                                // max+s 160223, saveTimeLapse
						if(cap_file_cnt > 9999) cap_file_cnt = 0;
						save_parameter_flag = 1;
						set_A2K_Cap_Cnt(cap_file_cnt);

						state_lst = rec_state;
						break;
					case -2:    // n/a
						break;
					}
				}

			} // if(rec_state != -2)
#ifdef __CLOSE_CODE__	//tmp				
			else if(get_convert_state() != -2) {
				sprintf(path, "%s/DCIM/%s/V2986_1057.mp4\0", sd_path, mSSID);		//暫時使用固定檔名, 未來由連接使用者選擇檔名
				convert_mp4_to_ts(&path[0]);
			}
#endif	//__CLOSE_CODE__			

			if(abuf != NULL) {
				free(abuf);
				abuf = NULL;
			}
        }
        get_current_usec(&defTime);
        defTime -= curTime;
        if(defTime < 5000){
            usleep(5000 - defTime);
            //get_current_usec(&runTime);
            //runTime -= curTime;
        }
        if(defTime > runTime) runTime = defTime;

        set_rec_proc_en(1);
        set_A2K_Rec_State_CMD(rec_state);
    }
}

void recoder_thread_init() {
	pthread_mutex_init(&mut_rec_buf, NULL);
    if(pthread_create(&thread_rec_id, NULL, rec_thread, NULL) != 0)
    {
        db_error("Create rec_thread fail !\n");
    }
}
