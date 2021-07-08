/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Media/live360_thread.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "Device/us363_camera.h"
#include "Device/US363/us360.h"
#include "Device/US363/Media/recoder_buffer.h"
#include "Device/US363/Media/recoder_thread.h"
#include "Device/US363/Media/Mp4/mp4.h"
#include "Device/US363/System/sys_time.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::Live360Th"


int live360_thread_en = 1;
pthread_t thread_live360_id;
pthread_mutex_t mut_live360_buf;

int mLive360En = 0;
int mLive360Cmd = 0;
int Live360_State = -1;		// 0:start -1:finish
char live360_path[128];
unsigned long long Live360_t1 = 0;
struct Cmd_Queue Live360_Cmd_Queue = {0};

void live360_cmd_queue_init() {
	memset(&Live360_Cmd_Queue, 0, sizeof(struct Cmd_Queue) );
}

void set_live360_t1() {
	get_current_usec(&Live360_t1);
}

void get_live360_t1(unsigned long long *time) {
	*time = Live360_t1;
}

void set_live360_thread_en(int en) {
	live360_thread_en = en;
}

void set_live360_state(int en) {
	if(Live360_State != en) {
		if(en == 0) {
			if(get_rec_state() != -2) return;
            //changeLedMode(1);
            //checkCpuSpeed();
            if(checkMicInterface() == 0){
               	setAudioRecThreadEn(1);
               	//mic_is_alive = 0;
            }else{
            	//mic_is_alive = 1;
               	//new AudioRecordLineInThread().start();
            }
            mLive360En = 1;
            mLive360Cmd = 2;
//            systemlog.addLog("info", System.currentTimeMillis(), "machine", "live360", "live360 start");
		}
		else {
			if(get_mic_is_alive() == 0 && getAudioRecThreadEn() == 1) {
				setAudioRecThreadEn(0);
			}
			mLive360En = 0;
			mLive360Cmd = 2;
//			systemlog.addLog("info", System.currentTimeMillis(), "machine", "live360", "live360 finish");
		}
		Live360_State = en;
//tmp		Set_Live360_State_Cmd(Live360_State);
	}
}

int get_live360_state() {
	return Live360_State;
}

void set_live360_cmd(int cmd) {
	mLive360Cmd = cmd;
}

int get_live360_cmd() {
	return mLive360Cmd;
}

void *live360_thread(void)
{
#ifdef __CLOSE_CODE__ //tmp	
    int i;
    static unsigned long long curTime, lstTime=0, runTime, defTime;
    unsigned size;
    unsigned v_p1, v_p2;
    unsigned a_p1, a_p2;
    int save_flag;
    unsigned checksum = 0, P2_tmp = 0, timestamp = 0;
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
    int freq = get_ISP_AEG_EP_IMX222_Freq();
    int frame_s, wait_i_frame = 0;		//wait_i_frame: FPGA H264 掉張, 須等下一張I Frame
    static int frame_s_lst = -1;
    int resWidth, resHeight;
    int len, ret;
    int M_Mode, S_Mode;
    LinkADTSFixheader fix;
    LinkADTSVariableHeader var;
    char path[128], ssid[0], sd_path[128];
    int live_p1, live_p2, live_p_tmp;
    struct stat sti;
	static int rec_fps = 60;

    while(live360_thread_en)
    {
        get_current_usec(&curTime);
        if(curTime < lstTime) lstTime = curTime;    // rex+ 151229, 防止例外錯誤
        else if((curTime - lstTime) >= 1000000){
        	db_debug("live360_thread: runTime=%d state=%d saveSize=%d saveFPS=%d\n", (int)runTime, Live360_Buf.state, saveSize, saveFPS);
            lstTime = curTime;
            runTime = defTime;
            saveSize = 0;
            saveFPS = 0;
        }

        get_Stitching_Out(&mode, &res);

        cmd_p1 = Live360_Cmd_Queue.P1;
        cmd_p2 = Live360_Cmd_Queue.P2;
        if(cmd_p1 != cmd_p2){
            cmd_d = Live360_Cmd_Queue.queue[cmd_p2];
            if(cmd_d == 0){                                    // 0:start
                if(Live360_Buf.state == -2){                        // Live360_Buf.state - 0:star 1:ing -1:end -2:not do
                	Live360_Buf_Init(&Live360_Buf);
                    rec_start_init(res, 0, freq, 0);
                    Get_M_Mode(&M_Mode, &S_Mode);
                    pcm_buf_init(freq, M_Mode, get_mic_is_alive(), 1);
                    Live360_Buf.state = cmd_d;
                    if(getTimeLapseMode() == 0) pcm_buf.state = cmd_d;
                }

                cmd_p2++;
                if(cmd_p2 >= CMD_QUEUE_MAX) cmd_p2 = 0;
                Live360_Cmd_Queue.P2 = cmd_p2;
            }
            else if(cmd_d == -1){                            // stop
                if(Live360_Buf.state == 0 || Live360_Buf.state == 1){
                	Live360_Buf.state = cmd_d;
                    if(getTimeLapseMode() == 0) pcm_buf.state = cmd_d;
                }

                cmd_p2++;
                if(cmd_p2 >= CMD_QUEUE_MAX) cmd_p2 = 0;
                Live360_Cmd_Queue.P2 = cmd_p2;
            }
        }

        if(Live360_Buf.state != -2)
        {
        	buf = NULL;
            isVideo = 0; isAudio = 0;
            size = 0; pcm_size = 0; close_run = 0;
            v_p1 = rec_buf.P1; v_p2 = rec_buf.P2;
            a_p1 = pcm_buf.P1; a_p2 = pcm_buf.P2;

            if(Live360_Buf.state == 0) {
            	freq = get_ISP_AEG_EP_IMX222_Freq();
                rec_fps = get_LiveFPS();
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

            if(pcm_buf.fps >= 50) {
            	abuf = malloc(AAC_BUF_LEN);
            	if(abuf != NULL) {
            		isAudio = read_aac_buf(&pcm_buf, abuf, &fix, &var);
            		pcm_size = var.aac_frame_length;
            	}
				else
					db_error("live360_thread() abuf malloc err!\n");
            }

            if(Live360_Buf.state == -1 || Live360_Buf.state == -3) close_run = 1;
            if(isAudio == 1 || ( (pcm_buf.fps < 50 || audio_rate == -1) && isVideo == 1) || close_run == 1)
            {
                switch(Live360_Buf.state){
                case 0:        // start
                    rec_buf.f_frame = 1;
                    if(isVideo == 1) {
                    	//Get Header -> Get Size
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

                        //Get H.264 Data
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

                    get_mVideo_WidthHeight(&resWidth, &resHeight);
                    if(rec_buf.f_frame == 0) {
						wait_i_frame = 0;
						frame_s_lst = frame_s;
                        getWifiSsid(&ssid[0]);
                        getSdPath(&sd_path[0], sizeof(sd_path));
						maek_save_file_path(13, live360_path, sd_path, mSSID, Live360_Buf.cnt);
						ptr = &live360_path[0];
						for(i = 0; i < 128; i++) {
							if(*ptr == '.' && *(ptr+1) == 't' && *(ptr+2) == 's')
								break;
							ptr++;
						}
						memset(path, 0, 128);
						memcpy(path, &live360_path[0], i);
						save_flag = muxer_ts_proc(Live360_Buf.state, buf, size, abuf, pcm_size, &mSPS_buf[0], mSPS_len, &mPPS_buf[0],
										mPPS_len, ip_frame, rec_fps, 0, pcm_buf.delayTime/1000, &live360_path[0],
										NULL, &timestamp, 0, &Live360_Buf, &fix, &var);

						saveSize += size;
						saveFPS++;
                    }

                    if(isVideo == 1) {
                        size += sizeof(img_buf_struct);        // += 64 bytes
                        if( (v_p2 + size) >= REC_BUF_MAX)
                            rec_buf.P2 = v_p2 + size - REC_BUF_MAX;
                        else
                            rec_buf.P2 = v_p2 + size;
                    }

                    if(rec_buf.f_frame == 0) {
                    	Live360_Buf.state = 1;
                    }

                    break;
                case 1:        // rec
                    if(isVideo == 1) {
                    	//Get Header -> Get Size
                        img_ptr = &img_tmp;
                        memset(img_ptr, 0, sizeof(img_buf_struct) );
                        if( (v_p2 + sizeof(img_buf_struct) ) < REC_BUF_MAX) {
                            memcpy(img_ptr, &rec_buf.buf[v_p2], sizeof(img_buf_struct) );
                            size = img_tmp.size;
                            ip_frame = img_tmp.ip_frame;
                            frame_s = img_tmp.frame_stamp;
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
                            wait_i_frame = 0;
                            mSPS_len = img_tmp.sps_len;
                            memcpy(&mSPS_buf[0], &img_tmp.sps_buf[0], img_tmp.sps_len);
                            mPPS_len = img_tmp.pps_len;
                            memcpy(&mPPS_buf[0], &img_tmp.pps_buf[0], img_tmp.pps_len);
                            P2_tmp = v_p2 + sizeof(img_buf_struct) - REC_BUF_MAX;
                        }

                        //Get H.264 Data
                        if(rec_buf.f_frame == 0 || img_tmp.enc_mode == 0 ||
                                (rec_buf.f_frame == 1 && img_tmp.enc_mode != 0 && img_tmp.ip_frame == 1) )
                        {
                            if(rec_buf.f_frame == 1)
                                db_debug("live360_thread() run ip_f %d size %d\n", img_tmp.ip_frame, size);

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

                    get_mVideo_WidthHeight(&resWidth, &resHeight);
					if(wait_i_frame == 1 && ip_frame == 1)
						wait_i_frame = 0;
					if(wait_i_frame == 0) {
						save_flag = muxer_ts_proc(Live360_Buf.state, buf, size, abuf, pcm_size, &mSPS_buf[0], mSPS_len, &mPPS_buf[0],
										mPPS_len, ip_frame, rec_fps, 0, pcm_buf.delayTime/1000, &live360_path[0],
										NULL, &timestamp, 0, &Live360_Buf, &fix, &var);
						saveSize += size;
						saveFPS++;
					}

                    if(isVideo == 1) {
                        size += sizeof(img_buf_struct);
                        if( (v_p2 + size) >= REC_BUF_MAX)
                            rec_buf.P2 = v_p2 + size - REC_BUF_MAX;
                        else
                            rec_buf.P2 = v_p2 + size;

                        v_p2 = rec_buf.P2;
                        if(rec_buf.jump == 1)  {
                            ptr = &rec_buf.buf[v_p2];
                            memcpy(&checksum, ptr, 4);
                            if(checksum != 0x20150819) {
                                db_error("live360_thread: checksum=%x\n", checksum);
                            }
                            else {		//jump 只寫入聲音, 影像只改變p2
                                isAudio = read_pcm_buf(&pcm_buf, abuf, pcm_size);
                                if(isAudio == 1) {
        							save_flag = muxer_ts_proc(Live360_Buf.state, NULL, size, abuf, pcm_size, &mSPS_buf[0], mSPS_len, &mPPS_buf[0],
        											mPPS_len, ip_frame, rec_fps, 0, pcm_buf.delayTime/1000, &live360_path[0],
        											NULL, &timestamp, 1, &Live360_Buf, &fix, &var);
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

                    if(save_flag == -1)		//Over File Time
                    	Live360_Buf.state = -3;

                    break;
                case -1:    // stop
                case -3:    // stop + restart
                    if(isVideo == 1) {
                    	//Get Header -> Get Size
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

                        //Get H.264 Data
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

                    get_mVideo_WidthHeight(&resWidth, &resHeight);
					save_flag = muxer_ts_proc(Live360_Buf.state, buf, size, abuf, pcm_size, &mSPS_buf[0], mSPS_len, &mPPS_buf[0],
									mPPS_len, ip_frame, rec_fps, 0, pcm_buf.delayTime/1000, &live360_path[0],
									NULL, &timestamp, 0, &Live360_Buf, &fix, &var);
					saveSize += size;
					saveFPS++;

                    if(isVideo == 1) {
                        size += sizeof(img_buf_struct);
                        if( (v_p2 + size) >= REC_BUF_MAX)
                            rec_buf.P2 = v_p2 + size - REC_BUF_MAX;
                        else
                            rec_buf.P2 = v_p2 + size;
                    }

                    if(Live360_Buf.state == -1) {		//-1, finish
                    	Delete_All_File(&live360_path[0]);
                        Live360_Buf.state = -2;
                    }
                    else {								//-3, restart
                        live_p1 = Live360_Buf.p1;
                        live_p2 = Live360_Buf.p2;
                    	ptr = strstr(live360_path, "/live");
                    	sprintf(&Live360_Buf.name[live_p1][0], "%s\0", (ptr+6) );
                    	Live360_Buf.duration[live_p1] = timestamp;	//LIVE360_PER_FILE_TIME;

                    	//delete file
                		live_p_tmp = live_p1+1;
                		if(live_p_tmp >= LIVE360_BUF_MAX)
                			live_p_tmp = 0;
                		sprintf(path, "%s/%s\0", live360_path, Live360_Buf.name[live_p_tmp]);
        				if(stat(path,&sti) == 0) {
        					unlink(path);
        					Live360_Buf.duration[live_p_tmp] = 0;
        				}

                    	Live360_Buf.p1++;
                    	if(Live360_Buf.p1 >= LIVE360_BUF_MAX)
                    		Live360_Buf.p1 = 0;

                    	if(Live360_Buf.delay_time < LIVE360_DELAY_TIME) {
                    		Live360_Buf.delay_time += LIVE360_PER_FILE_TIME;
                    	}

                    	if(Live360_Buf.delay_time >= LIVE360_DELAY_TIME) {
                    		//Make .m3u8
                    		Write_Live360_PlayList(&Live360_Buf);

                    		//delete file
                    		/*live_p_tmp = live_p2-1;
                    		if(live_p_tmp < 0)
                    			live_p_tmp = (LIVE360_BUF_MAX-1);
                    		sprintf(path, "%s/%s\0", live360_path, Live360_Buf.name[live_p_tmp]);
            				if(stat(path,&sti) == 0) {
            					unlink(path);
            					Live360_Buf.duration[live_p_tmp] = 0;
            				}*/

                    		Live360_Buf.p2++;
                        	if(Live360_Buf.p2 >= LIVE360_BUF_MAX)
                        		Live360_Buf.p2 = 0;
                    	}
                    	Live360_Buf.cnt++;
                    	Live360_Buf.state = 0;
                    }

                    break;
                case -2:    // n/a
                    break;
                }
            }

        } // if(Live360_Buf.state != -2)

        if(abuf != NULL) {
        	free(abuf);
        	abuf = NULL;
        }

        get_current_usec(&defTime);
        defTime -= curTime;
        if(defTime < 5000){
            usleep(5000 - defTime);
        }
        if(defTime > runTime) runTime = defTime;
    }
#endif	//__CLOSE_CODE__	
}

void live360_thread_init() {
	pthread_mutex_init(&mut_live360_buf, NULL);
    if(pthread_create(&thread_live360_id, NULL, live360_thread, NULL) != 0)
    {
        db_error("Create live360_thread fail !\n");
    }
}
