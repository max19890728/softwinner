/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/us360.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>          /* for videodev2.h */
#include <linux/videodev2.h>
//#include <linux/usbdevice_fs.h>
//#include <linux/android_alarm.h>                // rex+ 151225, ?ϥ? ANDROID_ALARM_SET_RTC
#include <dirent.h>
#include <sys/statfs.h>
#include <math.h>
#include <time.h>

#include "Device/us363_camera.h"
#include "Device/US363/us363_para.h"
#include "Device/US363/Media/recoder_buffer.h"
#include "Device/US363/Media/recoder_thread.h"
#include "Device/US363/Media/live360_thread.h"
#include "Device/US363/Media/Camera/uvc.h"
//tmp #include "live555_init.h"
#include "Device/US363/Media/Tinyalsa/asoundlib.h"
//tmp #include "mjpeg.h"
//tmp #include "gpio/oled.h"
//#include "Device/US363/Cmd/jpeg_header.h"
//tmp #include "awcodec/1639/drv_display.h"
//tmp #include "us360_log.h"
#include "Device/US363/Media/Mp4/mp4.h"
//tmp #include "awcodec/vencoder.h"
//tmp #include "aac/AacEnc.h"
//tmp #include "ts/muxer_ts.h"
//#include "ts/hls.h"
#include "Device/US363/Data/databin.h"
#include "Device/US363/Data/countrylist.h"
#include "Device/US363/Cmd/spi_cmd.h"
#include "Device/US363/Cmd/spi_cmd_s3.h"
#include "Device/US363/Cmd/us363_spi.h"
#include "Device/US363/Cmd/us360_func.h"
#include "Device/US363/Cmd/us360_define.h"
#include "Device/US363/Cmd/AletaS2_CMD_Struct.h"
#include "Device/US363/Cmd/variable.h"
#include "Device/US363/Cmd/fpga_driver.h"
#include "Device/US363/Cmd/defect.h"
#include "Device/US363/Cmd/Smooth.h"
#include "Device/US363/Test/test.h"
#include "Device/US363/Data/pcb_version.h"
#include "Device/US363/Data/customer.h"
#include "Device/US363/Data/country.h"
#include "Device/US363/Kernel/FPGA_Pipe.h"
#include "Device/US363/Kernel/k_spi_cmd.h"
#include "Device/US363/Driving/driving_mode.h"
#include "Device/US363/System/sys_time.h"
#include "Device/US363/System/sys_cpu.h"
#include "Device/US363/System/sys_power.h"
#include "Device/US363/Driver/Fan/fan.h"
#include "Device/US363/Driver/Lidar/lidar.h"
#include "Device/US363/Driver/MCU/mcu.h"
#include "Device/US363/Data/wifi_config.h"
#include "Device/US363/Util/file_util.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::US360"

//int enable_debug_flag = 0;

//int show_debug_log_message = 1;
//int show_infor_log_message = 1;
//int show_error_log_message = 1;
//int show_warni_log_message = 1;

int mic_is_alive = 0;	// weber+170623, 0:wifi 1:mic

// use android api
//#define NEW_AUDIO_RECORD 1

int mHWDecode     = 1;             // rex+ 161018, remove ./cedarx/main.c
dec_buf_struct dec_buf;            // rex+ 161018, remove ./cedarx/main.c
int uvc_err_flag = 0;

int muxer_type = MUXER_TYPE_MP4;	//???v?ʸˮ榡
int de_fd = 0, init_de_fd = 0;
int FPGA_Encode_Type = 0;		//0:JPEG 1:TimeLapse FPGA.H264, 2:TimeLapse FPGA.JPEG -> CPU.H264		setRecEn()&TimeLapse H264 ?~?]??1, ???????v?ɳ]?^0
static unsigned long long uvcWaitT=0;
int skip_frame_cnt = 0;

struct Check_Do_Dec_En_Struct {
	int rec_state;
	int HWDecode;
	int img_ok_flag;
	int dec_buf_flag;
	int size;
	int debug_flag;
	int stream;
	int res;
	int width;
	int height;
	int defect_step;
};

unsigned long long InitImageTimeS = 0;
int stream_flag = 0;
int FPGA_Ctrl_Power = 0;
int debug_cnt = 0;
int debug_h264_fs_lst = 0;
unsigned long long debug_t1=0, debug_t2=0;


int Save_Jpeg_Now_Cnt=0, Save_Jpeg_End_Cnt=0;
//media
pthread_t thread_uvc_id;        // rex+ 121523
pthread_t thread_pcm_id;
//cmd
pthread_t thread_st_id;

pthread_t thread_20ms_id;
pthread_t thread_1s_id;
pthread_t thread_5ms_id;

pthread_mutex_t mut_dec_buf;
pthread_mutex_t mut_pcm_buf;
pthread_mutex_t mut_st_buf;
pthread_mutex_t mut_touch_up;
pthread_mutex_t mut_20ms_buf;
pthread_mutex_t mut_1s_buf;
pthread_mutex_t mut_5ms_buf;

int uvc_thread_en = 1;
int pcm_thread_en = 1;
int st_thread_en = 1;
int thread_20ms_en = 1;
int thread_1s_en = 1;
int thread_5ms_en = 1;

int cnt1secUVC;
int uvc_proc_en;

//int DISP_FPS = 15;
//int IMG_WIDTH = 1920;
//int IMG_HEIGHT = 1080;
int IMG_Pixelformat = V4L2_PIX_FMT_MJPEG;       //0

unsigned int buffer_length = 0, real_length = 0;
//tmp unsigned char byte_rgb[UVC_BUF_MAX];
unsigned char *byte_rgb = NULL;

//int checkW = 0;           // 0:2M 1:10M
int img_ok_flag;            // 0:?v????????  1:?v?????`
int img_ready_flag = 0;	    // 0:?|?????쥿?`?v?? 1:?w?g?????L???`?v??

struct Cmd_Queue Cap_Cmd_Queue = {0};

//int cap_en = 0;

//extern int m_spi_file;        // rex+ 150624

doResize_buf_struc doResize_buf;

rec2thm_struct rec2thm;

char mSSID[16] = "US_0000\0";        // rex+ 151230, US_0000
char mPwd[16]  = "88888888\0";
char wifiAPssid[16] = "US_0000\0";


//char sd_path[64] = "/mnt/extsd\0";         // "/mnt/extsd\0";
unsigned jpeg_err_cnt = 0, jpeg_ok_cnt = 0;
int rec_file_cnt = 0, cap_file_cnt = 0;

int save_parameter_flag = 0;                        // rex+ 151229
extern int Block_File_idx;

int Write_File_Error = 0;

extern int BSmooth_Function;
extern int get_AEB_Frame_Cnt();

char *iconThmFile[6];       // 0:Cap, 1:Rec, 2:Lapse, 3:HDR, 4:RAW 5:Lapse H264
int iconThmSize[6]={0,0,0,0,0,0};

unsigned long long Time_Lapse_ms=0L;

//tmp unsigned char decode_buf[2][UVC_BUF_MAX];
unsigned char **decode_buf = NULL;
unsigned decode_len[2] = {0};
unsigned decode_stream[2] = {0};
int decode_idx0 = 0, decode_idx1 = 0;    //decode_idx0:UVC ??????     decode_idx1:????????

int img_debug_flag = 0;

pcm_buf_struct pcm_buf;

#define RTMP_AUDIO_BUFF_MAX             960000	/* 48000*16*1/8 */
int rtmp_audio_rate = 48000;
int rtmp_audio_channel = 1;
int rtmp_audio_bit = 16;
//tmp char rtmp_audio_buff[RTMP_AUDIO_BUFF_MAX];
char *rtmp_audio_buff = NULL;
int rtmp_audio_size = 0;
int rtmp_audio_irq_f=0;

//tmp	char aac_enc_buf[AAC_BUF_LEN];
//tmp	extern int get_rtsp_mic_alive(void);

int mSelfTimerSec = 0;
int mTakePicture = 0;
int mBurstCount = 0;

unsigned long long toutPowerT1=0, toutWifiT1=0, toutBurstT1=0;
unsigned long long toutSelfT1=0, toutLongKey=0, toutSaveT1=0, toutTakeT1=0;
unsigned long long toutPowerKey=0;

int play_sound_flag = -1;

int PowerMode = 3;

int lockRecordEnJNI = 0;
int audioTestRecording = 0;
int audioTestCount = 0;
int mMenuFlag = 0;
int isStandby = 0;
int recTime = 0;
int Send_ST_Flag = 0;
int UVCErrCount = 0;
int downloadLevel = 0;
int mBmm050Start = 0;

int check_st_cmd_ddr_flag = 0;

int adc_ratio = 1100;
int Adj_Sensor_Sync_Flag = 0;		//0:none 1:doSensorReset	2:doAdjSensorSync	3:doChooseMode
int Cmd_Idx = -1;
int Cmd_P1 = -1;

int choose_mode_flag = 0;
int isRecording = 0;

int power = 0;
int dcState = 0;
unsigned long long sleepTime;

int powerRang = 10;
int timerLidarCycle = 10;
int wifiReboot = 0;
int isNeedNewFreeCount = 0;
unsigned long long sd_freesize = 0L;
unsigned long long sd_allsize = 0L;

int doAutoStitch_flag = 0;
int WifiConnected = 0;
unsigned long long choose_mode_time;		//?????Ҧ???1???~?i????

int task5s_lock_flag = 0;
unsigned long long task5s_lock_schedule = 5000000;		//us, 5s
unsigned long long task5s_lock_time;

int rec_proc_en;

//extern unsigned long video_f_cnt;
//extern size_t riffSize, moviSize;

typedef struct C_Mode_ROM_S_H
{
    int             Picture;        // ?Ӥ??Ҧ?
    int             Single_Pic;     // ???i?Ҧ?
} C_Mode_ROM_S;
C_Mode_ROM_S C_Mode_ROM[15] =
{
    {1, 1}, // 0:Capture
    {0, 0}, // 1:Record
    {0, 0}, // 2:TimeLapse
    {1, 0}, // 3:AEB(3,5,7)
    {1, 0}, // 4:RAW
    {1, 1}, // 5:D-HDR
    {1, 1}, // 6:Night
    {1, 1}, // 7:N-HDR
    {1, 1}, // 8:Sport
    {1, 1}, // 9:S-WDR
    {0, 0}, // 10:REC-WDR
    {0, 0}, // 11:TS-WDR
    {1, 1}, // 12:M-Mode
    {1, 1}, // 13:Removal
    {1, 1}, // 14:3D-Model
};

unsigned long long Save_Jpeg_StartTime=0, Save_Jpeg_OutTime=0;
struct callback_func_entry callback_func;

#define LIVE264_MAX_BUFF    6
//tmp char live264_buff[0x400000];
char *live264_buff = NULL;
int live264_offset=0;
typedef struct live264_cmd_queue_H
{
    char             *addr;
    int                ofst;
    int                size;
    int                keyf;
} live264_cmd_queue;
live264_cmd_queue live264_cmd[LIVE264_MAX_BUFF];
int live264_idx1=0, live264_idx2=0;
int getIframe=0;

int waitIframe = 0;
int oled_proc_en = 0, dec_proc_en = 0;
int uvc_proc_count = 0, rec_proc_count = 0, oled_proc_count = 0, dec_proc_count = 0;
int need_boot = 0;
int getOneAzimuth = 999;

int sensorFlag = 0,sensorState = 0,sensorLogEnable = 1;
char sensorLog[200][128];
char sensorSavePath[128];

int HDR_Default_Parameter[3][4] = {
    		{3,15,60,0},		//?z     frameCnt/EV/strength/tone
    		{5,10,80,0},		//??
    		{7,10,60,0},		//?j
};

int doResize_flag = 0, doResize_flag_lst = 0;
char doResize_path[8][64];                                                                
	
int customerCode = 0;		//???ѽs?? 0:Ultracker	10137:Let's   2067001:?????ڤ?
int LangCode = 840;			//???a?N?X ????ISO 3166-1  158?x?W 392?饻 156???? 840???? 

int versionDate = 190507;
	
//480000    600000    720000    816000    1008000   1104000   1152000   1200000   1344000
int CpuFullSpeed     = 1152000;
int CpuAutoMoveSpeed = 600000;  	// 4?? x 600Mhz
int CpuHighSpeed     = 600000;     	// 2?? x 600Mhz        // 480000, 2MB Live?|????
int CpuMiddleSpeed   = 480000;    	// 4?? x 480Mhz
int CpuLowSpeed      = 480000;  	// 2?? x 480Mhz

int malloc_live264_buf() {
	//char live264_buff[0x400000];
	live264_buff = (char *)malloc(sizeof(char) * 0x400000);
	if(live264_buff == NULL) goto error;
	return 0;
error:
	db_error("malloc_live264_buf() malloc error!");
	return -1;
}

void free_live264_buf() {
	if(live264_buff != NULL)
		free(live264_buff);
	live264_buff = NULL;
}

int malloc_rtmp_audio_buf() {
	//char rtmp_audio_buff[RTMP_AUDIO_BUFF_MAX];
	rtmp_audio_buff = (char *)malloc(sizeof(char) * RTMP_AUDIO_BUFF_MAX);
	if(rtmp_audio_buff == NULL) goto error;
	return 0;
error:
	db_error("malloc_rtmp_audio_buf() malloc error!");
	return -1;
}

void free_rtmp_audio_buf() {
	if(rtmp_audio_buff != NULL)
		free(rtmp_audio_buff);
	rtmp_audio_buff = NULL;
}

int malloc_byte_rgb_buf() {
	//unsigned char byte_rgb[UVC_BUF_MAX];
	byte_rgb = (unsigned char *)malloc(sizeof(unsigned char) * UVC_BUF_MAX);
	if(byte_rgb == NULL) goto error;
	return 0;
error:
	db_error("malloc_byte_rgb_buf() malloc error!");
	return -1;
}

void free_byte_rgb_buf() {
	if(byte_rgb != NULL)
		free(byte_rgb);
	byte_rgb = NULL;
}

int malloc_decode_buf() {
	//unsigned char decode_buf[2][UVC_BUF_MAX];
	decode_buf = (unsigned char **)malloc(sizeof(unsigned char *) * 2);
	if(decode_buf == NULL) goto error;
	for(int i = 0; i < 2; i++) {
		decode_buf[i] = (unsigned char *)malloc(sizeof(unsigned char) * UVC_BUF_MAX);
		if(decode_buf[i] == NULL) goto error;
	}
	return 0;
error:
	db_error("malloc_decode_buf() malloc error!");
	return -1;
}

void free_decode_buf() {
	if(decode_buf != NULL) {
		for(int i = 0; i < 2; i++) {
			if(decode_buf[i] != NULL)
				free(decode_buf[i]);
		}
		free(decode_buf);
	}
	decode_buf = NULL;
}

int malloc_dec_buf() {
	//char                buf[3][DEC_BUF_MAX];
	dec_buf.buf = (char **)malloc(sizeof(char *) * 3);
	if(dec_buf.buf == NULL) goto error;
	for(int i = 0; i < 3; i++) {
		dec_buf.buf[i] = (char *)malloc(sizeof(char) * DEC_BUF_MAX);
		if(dec_buf.buf[i] == NULL) goto error;
	}
	return 0;
error:
	db_error("malloc_dec_buf() malloc error!");
	return -1;
}

void free_dec_buf() {
	if(dec_buf.buf != NULL) {
		for(int i = 0; i < 3; i++) {
			if(dec_buf.buf[i] != NULL)
				free(dec_buf.buf[i]);
		}
		free(dec_buf.buf);
	}
	dec_buf.buf = NULL;
}

int malloc_pcm_buf() {
	//unsigned char buf[PCM_BUF_MAX];
	pcm_buf.buf = (unsigned char *)malloc(sizeof(unsigned char) * PCM_BUF_MAX);
	if(pcm_buf.buf == NULL) goto error;
	return 0;
error:
	db_error("malloc_pcm_buf() malloc error!");
	return -1;
}

void free_pcm_buf() {
	if(pcm_buf.buf != NULL)
		free(pcm_buf.buf);
	pcm_buf.buf = NULL;
}

void free_us360_buf() {
	free_live264_buf();
	free_rtmp_audio_buf();
	free_byte_rgb_buf();
	free_decode_buf();
	free_rec_img_buf();
	free_dec_buf();
	free_rec_buf();
	free_pcm_buf();
}

/*
 * ?????ŧi?@???j?O????, ?{???|????,
 * ?ҥH?אּ?ʺA?t?m?O????
 */
int malloc_us360_buf() {
	if(malloc_live264_buf() < 0)	goto error;
	if(malloc_rtmp_audio_buf() < 0) goto error;
	if(malloc_byte_rgb_buf() < 0)	goto error;
	if(malloc_decode_buf() < 0)		goto error;
	if(malloc_rec_img_buf() < 0)	goto error;
	if(malloc_dec_buf() < 0)		goto error;
	if(malloc_rec_buf() < 0)		goto error;
	if(malloc_pcm_buf() < 0)		goto error;

	return 0;
error:
	db_error("malloc_us360_buf() malloc error!");
	free_us360_buf();
	return -1;
}

void SetMuxerType(int type) {
	muxer_type = type;
}
int GetMuxerType(void) {
	return muxer_type;
}

int hdmi_get_hpd_status(void){    // rex+ 161018, remove ./cedarx    // weber+s 161104
    int ret = 0;
    unsigned long arg2[4] = {0,0,0,0};
//#ifdef __CLOSE_CODE__	//tmp
    if(init_de_fd == 0){
        de_fd = open("/dev/disp", O_RDWR);
        if (de_fd < 0) {
            db_error("Open display driver failed!\n");
            return 0;
        }
        init_de_fd = 1;

        ret = ioctl(de_fd, DISP_CMD_GET_SCN_WIDTH, arg2);
        db_debug("DISP_CMD_GET_SCN_WIDTH = %d\n", ret);

        ret = ioctl(de_fd, DISP_CMD_GET_SCN_HEIGHT, arg2);
        db_debug("DISP_CMD_GET_SCN_HEIGHT = %d\n", ret);

        ret = ioctl(de_fd, DISP_CMD_GET_OUTPUT_TYPE, arg2);
        db_debug("DISP_CMD_GET_OUTPUT_TYPE = %d\n", ret);

        ret = ioctl(de_fd, DISP_CMD_LCD_GET_BRIGHTNESS, arg2);
        db_debug("DISP_CMD_LCD_GET_BRIGHTNESS = %d\n", ret);
    }

    ret = ioctl(de_fd, DISP_CMD_HDMI_GET_HPD_STATUS, arg2);
    //db_debug("ret = %d\n", ret);
//#endif	//__CLOSE_CODE__
    return ret;
}

void set_fpga_encode_type(int type) {
	FPGA_Encode_Type = type;
}
int get_fpga_encode_type() {
	return FPGA_Encode_Type;
}

int readframeonce(void)
{
    static unsigned long long inTime, outTime;
    static int retry_cnt=0;
    struct v4l2_buffer buf;
    int timeout_cnt=1;
    int c_mode = getCameraMode();
    int step=0;

    if(Check_UVC_Fd() == -1){
        db_error("readframeonce: uvc_fd=-1\n");
        return ERROR_LOCAL;
    }

    if(GetFPGASleepEn() == 1) {
    	retry_cnt = 0;
    	return TIMEOUT_LOCAL;
    }

    int ret = check_F_Com_In();
    if(ret != 1){
        //db_error("readframeonce: ret=%d\n", ret);
        return SUCCESS_LOCAL;        // command not ready
    }
    int d_cnt = read_F_Com_In_Capture_D_Cnt();
    get_current_usec(&inTime);
    for(;;) {
        int r, ret = 0;
		
        r = UVC_Select();
        if(-1 == r) {
        	db_error("readframeonce: select error\n");
            if (EINTR == errno)
                continue;
            return errnoexit ("select");
        }
		
        if(0 == r && GetDefectStep() == 0) {
        	step = Get_F_Cmd_In_Capture_Step();
            //???Ʀp?L?W?L10???A?|?y??timeout
            if(c_mode == CAMERA_MODE_REMOVAL && d_cnt != 0){					// 13:Removal
                timeout_cnt = 40;
            }
            else if(c_mode == CAMERA_MODE_M_MODE && d_cnt != 0){         // 12:M-Mode
                int bexp_1sec, bexp_gain;
                get_AEG_B_Exp(&bexp_1sec, &bexp_gain);
                if(bexp_1sec > 2){
                    timeout_cnt = (bexp_1sec + 10) / 2;
                }
            }
            else if(getTimeLapseMode() != 0 && get_fpga_encode_type() == 1){		//?Y?? FPGA.H264
                timeout_cnt = Time_Lapse_ms / 1000;
            }
            else{
                timeout_cnt = 1;
            }
            if(timeout_cnt < 1) timeout_cnt = 1;
            if(step == 2) timeout_cnt *= 2;
            retry_cnt ++;
            if(retry_cnt >= 10 * timeout_cnt){     // 2sec * 5 = 10sec
                db_error("readframeonce: select timeout! retry_cnt=%d\n", retry_cnt);
                return ERROR_LOCAL;
            }
            else return TIMEOUT_LOCAL;
        }
        else retry_cnt = 0;

        ret = readframe();
        if(ret == 1) break;
        else if(ret == -1) return ERROR_LOCAL;
    }
    get_current_usec(&outTime);
    uvcWaitT = outTime - inTime;

    return SUCCESS_LOCAL;
}

void processimage(const void *p, unsigned int length, unsigned int r_length)
{
    mjpeg((unsigned char *)p, length, r_length);
}

/*
 * ?ѳ????ٹq???A?e???ᱼ???D
 */
void Set_Skip_Frame_Cnt(int cnt) {
	skip_frame_cnt = cnt;
}

int Get_Dec_En(struct Check_Do_Dec_En_Struct ck_data, int *skip_cnt)
{
	if(ck_data.defect_step != 0) return 0;		//???a?I???w??????

	if(*skip_cnt > 0) {
    	(*skip_cnt)--;
    	return 0;
	}

	if(ck_data.rec_state != -2 || ck_data.HWDecode == 1) {
		if(ck_data.img_ok_flag == 1 && ck_data.dec_buf_flag == 0 && ck_data.size < DEC_BUF_MAX) {
			if(ck_data.debug_flag == 0) {
				if(ck_data.stream == 1) {
					if(ck_data.res == 2 || ck_data.res == 13 || ck_data.res == 14)
						return 1;
				}
				else if(ck_data.stream == 2) {
					if(ck_data.res == 1 || ck_data.res == 7 || ck_data.res == 12)
						return 1;
				}
			}
			else {
				if(ck_data.stream == 1 && ck_data.width <= 3840 && ck_data.height <= 3840)
					return 1;
			}
		}
	}
	return 0;
}

void Set_Init_Image_Time(unsigned long long time) {
	InitImageTimeS = time;
}

int readframe(void)
{
    FILE* fp;
    int fp2fd;
    char path[128];
    unsigned int i, j, jpeg_size, panorama_idx = 0, cnt = 1;
    unsigned char *tmp;
    unsigned char *tmpWhi, *tmpWlo, *tmpHhi, *tmpHlo;
    struct v4l2_buffer buf;
    struct stat sti;
    struct timeval tv;
    int cmd_p1, cmd_p2;
    int mode, res;
    int width, height, JPEG_Width, JPEG_Height;
    unsigned long long ntime;
    H_File_Head_rec *H264_Head;
	int key_f, size_tmp;
	int sps_len, pps_len;
	char sps[16], pps[16];
	int h264_buf_size = 0;
	int live_q_m = get_A2K_JPEG_Live_Quality_Mode();
	static int live_skip = 0;
    int c_mode = getCameraMode();
    int fpga_enc_type = get_fpga_encode_type();

    get_Stitching_Out(&mode, &res);

    if(Check_UVC_Fd() == -1){
        db_error("readframe: uvc_fd=-1\n");
        return ERROR_LOCAL;
    }
	
    if (UVC_DQBUF(&buf) == -1) {
        switch (errno) {
            case EAGAIN:
            	db_error("readframe: Err! (EAGAIN:%d)\n", errno);
                return 0;
            case EIO:
            default:
//tmp            	if(FPGA_Ctrl_Power == 0)
//tmp            		setErrorCode(0x002);
            	db_error("readframe: Err! (EIO:%d)\n", errno);
                return errnoexit ("VIDIOC_DQBUF");
        }
    }

    if(buf.bytesused > 0 /*&& buf.bytesused < UVC_BUF_MAX*/) {
        jpeg_size = 0;
        img_ok_flag = 0;
        tmp = Get_Buffer_Start(buf.index);
        if(fpga_enc_type == 1) {		//Timelapse	FPGA.H264
            H264_Head = Get_Buffer_Start(buf.index);
            if(H264_Head->Start_Code == Head_REC_Start_Code) {
            	JPEG_Width  = H264_Head->Size_H;
            	JPEG_Height = H264_Head->Size_V;
            	switch(res){
            	case 1:
            		if(JPEG_Width == S2_RES_12K_WIDTH && JPEG_Height == S2_RES_12K_HEIGHT)
            			img_ok_flag = 1;
            		break;
           		case 2:
           			if(JPEG_Width == S2_RES_4K_WIDTH && JPEG_Height == S2_RES_4K_HEIGHT)
           				img_ok_flag = 1;
           			break;
           		case 7:
           			if(JPEG_Width == S2_RES_8K_WIDTH && JPEG_Height == S2_RES_8K_HEIGHT)
           				img_ok_flag = 1;
           			break;
           		case 12:
           			if(JPEG_Width == S2_RES_6K_WIDTH && JPEG_Height == S2_RES_6K_HEIGHT)
           				img_ok_flag = 1;
           			break;
           		case 13:
           			if(JPEG_Width == S2_RES_3K_WIDTH && JPEG_Height == S2_RES_3K_HEIGHT)
           				img_ok_flag = 1;
           			break;
           		case 14:
           			if(JPEG_Width == S2_RES_2K_WIDTH && JPEG_Height == S2_RES_2K_HEIGHT)
           				img_ok_flag = 1;
           			break;
           		}
           		stream_flag = 1;

           		if(img_ok_flag == 1) {
           			tmp += sizeof(H_File_Head_rec);
           			if(buf.bytesused >= UVC_BUF_MAX)
           				jpeg_size = UVC_BUF_MAX - sizeof(H_File_Head_rec);
           			else
           				jpeg_size = buf.bytesused - sizeof(H_File_Head_rec);
           			key_f = !(H264_Head->IP_M);
               	    if(get_copy_h264_to_rec_en(c_mode, res, fpga_enc_type) == 1) {
               	    		size_tmp = 4;	//?h?? start code
               	    		Get_FPGA_H264_SPS_PPS(&sps_len, &sps[0], &pps_len, &pps[0]);
//tmp               	    		Set_MP4_H264_Profile_Level(VENC_H264ProfileBaseline, VENC_H264Level3);		//FPGA?T?w?Ѽ?
               	            copy_to_rec_buf(&tmp[size_tmp], (jpeg_size-size_tmp), key_f, &sps[0], sps_len, &pps[0], pps_len, fpga_enc_type, H264_Head->Frame_Stamp);
               	    }
           		}
           	}
        }
        else {
			if(tmp[0] == 0xFF && tmp[1] == 0xD8 && tmp[2] == 0xFF && tmp[3] == 0xE0) {
				tmp = Get_Buffer_Start(buf.index) + buf.bytesused - 512;
				for(i = buf.bytesused - 512; i < buf.bytesused; i++) {
					if(tmp[0] == 0xFF && tmp[1] == 0xD9) {
						//db_debug("readframe: img ok!\n");
						img_ok_flag = 1;
						jpeg_ok_cnt++;
						break;
					}
					tmp++;
				}
			}
			else
				db_error("readframe: FF D8 err!\n");

			if(img_ok_flag == 0){
				jpeg_err_cnt++;
				db_error("readframe: FF D9 err! ok=%d err=%d size=%d\n", jpeg_ok_cnt, jpeg_err_cnt, buf.bytesused);

				// debug save img
				/*sprintf(path, "/mnt/sdcard/debug%d.jpg\0", debug_cnt);
				fp = fopen(path, "wb");
				if(fp != NULL) {
					fwrite(Get_Buffer_Start(buf.index), buf.bytesused, 1, fp);
					fclose(fp);
					debug_cnt = (debug_cnt + 1) & 0x3;
				}*/
			}

			jpeg_size = i + 2;
			if(jpeg_size > buf.bytesused){
				db_error("readframe: err! jpeg_size=%d bytesused=%d\n", jpeg_size, buf.bytesused);
				jpeg_size = buf.bytesused;
				img_ok_flag = 0;
				jpeg_err_cnt++;
			}

			if(jpeg_size > UVC_BUF_MAX)
				jpeg_size = UVC_BUF_MAX;

			// ?P?_ stream2
			if(img_ok_flag == 1) {
				tmp = Get_Buffer_Start(buf.index);
				tmpWhi = tmp; tmpWlo = tmp;
				tmpHhi = tmp; tmpHlo = tmp;
				for(i = 0; i < 2048; i++) {
					if(*tmp == 0xFF && *(tmp+1) == 0xC0) {
						tmpHhi = tmp + 5;
						tmpHlo = tmp + 6;
						tmpWhi = tmp + 7;
						tmpWlo = tmp + 8;
						break;
					}
					tmp++;
				}
				JPEG_Width  = ( (*tmpWhi << 8) | *tmpWlo);
				JPEG_Height = ( (*tmpHhi << 8) | *tmpHlo);

				int tool_cmd = get_TestToolCmd();
				int dbg_jpg = get_DebugJPEGMode();
				int isp2_sen = get_ISP2_Sensor();
				if((tool_cmd&0xff) == 5) {
					img_ok_flag = (JPEG_Width == 1536 && JPEG_Height == 1152)? 1: 0;
					stream_flag = 1;
				}
				else if(dbg_jpg == 1 && isp2_sen >= 0 && isp2_sen <= 4) {
					img_ok_flag = (JPEG_Width == 1536 && JPEG_Height == 1152)? 1: 0;
					stream_flag = 1;
				}
				else if(dbg_jpg == 1 && isp2_sen == -8) {
					img_ok_flag = (JPEG_Width == 1536 && JPEG_Height == 1152)? 1: 0;
					stream_flag = 1;
				}
				else if(dbg_jpg == 1 && isp2_sen == -9) {
					img_ok_flag = (JPEG_Width == 1920 && JPEG_Height == 1080)? 1: 0;
					stream_flag = 1;
				}
				else if(dbg_jpg == 1 && (isp2_sen == -1 || isp2_sen == -2 || isp2_sen == -4 || isp2_sen == -6) ) {
					if(JPEG_Width == 1920 && JPEG_Height == 1024)
						img_ok_flag = 1;
					else if(JPEG_Width == 1024 && JPEG_Height == 1024 && (tool_cmd&0xff) == 2)  //Focus: main_cmd == 2
						img_ok_flag = 1;
					else if(JPEG_Width == 1920 && JPEG_Height == 960)
						img_ok_flag = 1;
					else
						img_ok_flag = 0;
					stream_flag = 1;
				}
				else if(dbg_jpg == 1 && isp2_sen == -3) {
					if(JPEG_Width == 3840 && JPEG_Height == 1920)
						img_ok_flag = 1;
					else if(JPEG_Width == 3840 && JPEG_Height == 1920 && ((tool_cmd&0xff) == 2 || ((tool_cmd&0xffff) == 0x0507))) // main_cmd == 7 && sub_cmd == 5
						img_ok_flag = 1;
					else
						img_ok_flag = 0;
					stream_flag = 1;
				}
				else if(dbg_jpg == 1 && (isp2_sen == -99 || isp2_sen == -5) ) {
					img_ok_flag = (JPEG_Width == 1920 && JPEG_Height == 1080)? 1: 0;
					stream_flag = 1;
				}
				else if(dbg_jpg == 2) {
					int w, h;
					get_GPadj_WidthHeight(&w, &h);
					img_ok_flag = (JPEG_Width == w && JPEG_Height == h)? 1: 0;
					stream_flag = 1;
				}
				else if(JPEG_Width == S2_RES_THM_WIDTH && JPEG_Height == S2_RES_THM_HEIGHT) {
					stream_flag = 3;
					img_ok_flag = 1;
					db_debug("01 Get THM JPEG!\n");
				}
				else {
					//0:Capture  1:Recrd  2:TimeLapse  3:AEB(3P,5P,7P)  4:RAW(5P)  5:D-HDR  6:Night 
					//7:N-HDR  8:Sport  9:S-WDR  10:RecWDR  11:TimeLapseWDR  12:M-Mode  13:Removal
					switch(c_mode) {
					case 0:				//Cap
					case 3:
					case 5:
					case 8:
					case 9:
					case 13:
						if((c_mode == 13 && JPEG_Width == (768*3*4) && JPEG_Height == (576*2)) ||
						   (c_mode == 13 && JPEG_Width == (6912) && JPEG_Height == (864)))
						{
							stream_flag = 1;
							img_ok_flag = 1;
						}
						else if(JPEG_Width == S2_RES_4K_WIDTH && JPEG_Height == S2_RES_4K_HEIGHT) {
							stream_flag = 2;
							if(live_q_m == 1 && live_skip == 1)	img_ok_flag = 0;	//skip frame
							else								img_ok_flag = 1;
							live_skip = (live_skip+1) & 0x1;
						}
						else {
							stream_flag = 1;
							if(res == 1)      { width  = S2_RES_12K_WIDTH; height = S2_RES_12K_HEIGHT; }
							else if(res == 7) { width  = S2_RES_8K_WIDTH;  height = S2_RES_8K_HEIGHT;  }
							else if(res == 12){ width  = S2_RES_6K_WIDTH;  height = S2_RES_6K_HEIGHT;  }
							if(JPEG_Width == width && JPEG_Height == height)
								img_ok_flag = 1;
							else
								img_ok_flag = 0;
						}
						break;
					case 1:				//Rec
					case 10:
						stream_flag = 1;
						if(res == 2)       { width  = S2_RES_4K_WIDTH; height = S2_RES_4K_HEIGHT; }
						else if(res == 13) { width  = S2_RES_3K_WIDTH; height = S2_RES_3K_HEIGHT; }
						else if(res == 14) { width  = S2_RES_2K_WIDTH; height = S2_RES_2K_HEIGHT; }
						if(JPEG_Width == width && JPEG_Height == height)
							img_ok_flag = 1;
						else
							img_ok_flag = 0;
						break;
					case 2:             // Timelapse
					case 11:            // Timelapse WDR
						if(JPEG_Width == S2_RES_4K_WIDTH && JPEG_Height == S2_RES_4K_HEIGHT) {
							if(res == 2) stream_flag = 1;
							else 		 stream_flag = 2;
							img_ok_flag = 1;
						}
						else if(JPEG_Width == S2_RES_3K_WIDTH && JPEG_Height == S2_RES_3K_HEIGHT) {
							stream_flag = 2;
							img_ok_flag = 1;
						}
						else {
							stream_flag = 1;
							if(res == 1)      { width  = S2_RES_12K_WIDTH; height = S2_RES_12K_HEIGHT; }
							else if(res == 7) { width  = S2_RES_8K_WIDTH;  height = S2_RES_8K_HEIGHT;  }
							else if(res == 12){ width  = S2_RES_6K_WIDTH;  height = S2_RES_6K_HEIGHT;  }
							if(JPEG_Width == width && JPEG_Height == height)
								img_ok_flag = 1;
							else
								img_ok_flag = 0;
						}
						break;

					case 4:				//RAW
						if(JPEG_Width == S2_RES_4K_WIDTH && JPEG_Height == S2_RES_4K_HEIGHT) {
							stream_flag = 2;
							if(live_q_m == 1 && live_skip == 1) img_ok_flag = 0;		//skip frame
							else								img_ok_flag = 1;
							live_skip = (live_skip+1) & 0x1;
						}
						else {
							stream_flag = 1;
							width  = S2_RES_S_FS_WIDTH - 32; height = S2_RES_S_FS_HEIGHT;
							if(JPEG_Width == width && JPEG_Height == height)
								img_ok_flag = 1;
							else
								img_ok_flag = 0;
						}
						break;

					case 6:				//Night
					case 7:
					case 12:
						if(c_mode == 12 && GetDefectStep() != 0) {
							stream_flag = 1;
							width  = S2_RES_S_FS_WIDTH - 32; height = S2_RES_S_FS_HEIGHT;
							if(JPEG_Width == width && JPEG_Height == height)
								img_ok_flag = 1;
							else
								img_ok_flag = 0;
						}
						else if(JPEG_Width == S2_RES_4K_WIDTH && JPEG_Height == S2_RES_4K_HEIGHT) {
							stream_flag = 2;
							img_ok_flag = 1;
						}
						else {
							stream_flag = 1;
							if(res == 1)      { width  = S2_RES_12K_WIDTH; height = S2_RES_12K_HEIGHT; }
							else if(res == 7) { width  = S2_RES_8K_WIDTH;  height = S2_RES_8K_HEIGHT;  }
							else if(res == 12){ width  = S2_RES_6K_WIDTH;  height = S2_RES_6K_HEIGHT;  }
							if(JPEG_Width == width && JPEG_Height == height)
								img_ok_flag = 1;
							else
								img_ok_flag = 0;
						}
						break;
					case 14:			//3D-Model
						if(JPEG_Width == S2_RES_12K_WIDTH && JPEG_Height == S2_RES_12K_HEIGHT) {
							stream_flag = 1;
							img_ok_flag = 1;
						}
						else if(JPEG_Width == S2_RES_3D1K_WIDTH && JPEG_Height == S2_RES_3D1K_HEIGHT) {
							stream_flag = 2;
							img_ok_flag = 1;
						}
						else if(JPEG_Width == S2_RES_3D4K_WIDTH && JPEG_Height == S2_RES_3D4K_HEIGHT) {
							stream_flag = 2;
							img_ok_flag = 1;
						}
						else
							img_ok_flag = 0;
						break;
					}
				}
			}
			if(Get_Init_Image_State() != 2){
				img_ok_flag = 0;
				get_current_usec(&ntime);
				if(InitImageTimeS == 0) Set_Init_Image_Time(ntime);
				else if(InitImageTimeS > ntime) Set_Init_Image_Time(ntime);
				else{
					if((ntime - InitImageTimeS) > 10000){     // 10sec timeout
						Set_Init_Image_State(2);
					}
				}
				db_debug("uvc_thread: InitImageState=%d\n", Get_Init_Image_State() );
			}
			if(img_ok_flag == 1){
				if(img_ready_flag == 0){
					db_debug("fpga download get freame\n");
					SetCpu(4, 600000);
				}
//tmp				paintOLEDDDRError(0);
				img_ready_flag = 1;
			}
			int dbg_jpg = get_DebugJPEGMode();
			int mVideoW, mVideoH;
			get_mVideo_WidthHeight(&mVideoW, &mVideoH);

			struct Check_Do_Dec_En_Struct check_data;
			check_data.rec_state    = get_rec_state();
			check_data.HWDecode     = mHWDecode;
			check_data.img_ok_flag  = img_ok_flag;
			check_data.dec_buf_flag = dec_buf.flag[dec_buf.uvc_idx];
			check_data.size         = jpeg_size;
			check_data.debug_flag   = dbg_jpg;
			check_data.stream       = stream_flag;
			check_data.res          = res;
			check_data.width        = JPEG_Width;
			check_data.height       = JPEG_Height;
			check_data.defect_step  = GetDefectStep();

			if(Get_Dec_En(check_data, &skip_frame_cnt) == 1)
			{
					dec_buf.len[dec_buf.uvc_idx] = jpeg_size;
					memcpy(&dec_buf.buf[dec_buf.uvc_idx][0], Get_Buffer_Start(buf.index), jpeg_size);
					dec_buf.size[dec_buf.uvc_idx].width  = JPEG_Width;	//mVideoWidth;
					dec_buf.size[dec_buf.uvc_idx].height = JPEG_Height;	//mVideoHeight;
					dec_buf.flag[dec_buf.uvc_idx] = 1;

					if(dec_buf.uvc_idx < 2) dec_buf.uvc_idx ++;
					else                    dec_buf.uvc_idx = 0;

					// wakeup thread_dec()
//tmp					if(jpeg_size > 0 && jpeg_size < LIVE555_BUF_MAX){
//tmp						copy_to_mjpg_buf_queue(Get_Buffer_Start(buf.index), jpeg_size);
//tmp					}
			}
			else if( (get_rec_state() != -2 || mHWDecode == 1) && img_ok_flag != 1 && dec_buf.flag[dec_buf.idx] == 0 && stream_flag == 1)
				db_error("dec: buf skip!  (img_ok_flag=%d stream_flag=%d JPEG_Width=%d JPEG_Height=%d)\n", img_ok_flag, stream_flag, JPEG_Width, JPEG_Height);
//			else if( (get_rec_state() != -2 || mHWDecode == 1) && img_ok_flag == 1 && jpeg_size >= DEC_BUF_MAX && stream_flag == 1)
//				db_error("dec: jpeg_size >= DEC_BUF_MAX  err! (%d)\n", buf.bytesused);
		}
    }
    else if(buf.bytesused <= 0 || buf.bytesused >= UVC_BUF_MAX)
        db_error("readframe: buf.bytesused err! size %d\n", buf.bytesused);

    if(jpeg_size > UVC_BUF_MAX) {
        db_error("readframe: jpeg_size >= UVC_BUF_MAX  err!(%d)\n", jpeg_size);
        if (UVC_QBUF(&buf) == -1)
            return errnoexit ("VIDIOC_QBUF");
        return -1;
    }

    processimage(Get_Buffer_Start(buf.index), jpeg_size, buf.bytesused);
    if (UVC_QBUF(&buf) == -1)
        return errnoexit ("VIDIOC_QBUF");

    return 1;
}

void mjpeg(unsigned char *src, unsigned int length, unsigned int r_length)
{
    buffer_length = length;
    real_length = r_length;
    if(buffer_length <= UVC_BUF_MAX) {
        unsigned char *brgb = NULL;
        brgb = &byte_rgb[0];
        memcpy(brgb, src, buffer_length);
        //jpeg_main(src, buffer_length);
    }
}

int Capture_Is_Finish() {
	if(Save_Jpeg_Now_Cnt == Save_Jpeg_End_Cnt)
		return 1;
	else if(Save_Jpeg_Now_Cnt > Save_Jpeg_End_Cnt)
		return -1;
	else
		return 0;
}

void SetCapInit(void)
{
	Save_Jpeg_Now_Cnt = 0;
	Save_Jpeg_End_Cnt = 0;
	memset(&Cap_Cmd_Queue, 0, sizeof(Cap_Cmd_Queue) );
}

int get_copy_jpeg_to_rec_en(int c_mode, int res, int stream, int enc_type)
{
	if( (c_mode == 2 || c_mode == 11) && enc_type == 1) {			//TimeLapse FPAG.H264
		return 0;
	}
	else if( (c_mode == 2 || c_mode == 11) && enc_type == 2) {		//TimeLapse CPU.H264
		return 0;
	}
	else {
		switch(res) {
		case 2:  if( (c_mode == 2 || c_mode == 11) && stream == 1) return 1;
				 break;
		case 1:
		case 7:
		case 12: if(stream == 1) return 1;
				 break;
		}
	}
	return 0;
}

/*
 * ?ˬd?O?_???i, ?ݭn???e
 */
void Check_Cap_State(int c_mode, int img_flag, int err_flag)
{
    unsigned long long NowTime, T1, T2;
    int err, step, page;
    int get_img[2];
    int err_cnt = 0;
    int bexp_1sec, bexp_gain;
    int smooth_en = get_A2K_Smooth_En();
    unsigned long long smooth_add_time = 4000000;
    unsigned long long lose_all_tout = 15000000;		// (NowTime - T1)
    unsigned long long lose_s_tout = 6000000;			// (NowTime - T2)
    unsigned long long lose_b_tout = 10000000;			// (NowTime - T1)

    step = Get_F_Cmd_In_Capture_Step();
    if(step == 2) {
    	err = Get_F_Cmd_In_Capture_Lose_Flag();
    	err_cnt = Get_F_Cmd_In_Capture_Img_Err_Cnt();
    	if(/*img_flag == 0 && err_flag != TIMEOUT_LOCAL &&*/ err != 0) {
    		db_error("Check_Cap_State() img_flag=%d err_flag=%d err=%d err_cnt=%d smooth_en=%d\n", img_flag, err_flag, err, err_cnt, smooth_en);
    		err_cnt++;
    	}

    	if(c_mode == 12) {										//M-Mode, ?n?????? + 20??
			get_AEG_B_Exp(&bexp_1sec, &bexp_gain);
			lose_all_tout = bexp_1sec * 1000000 + 20000000;
    	}
    	else if(c_mode == 13)									//Removal
			lose_all_tout = 60000000;
    	else if(c_mode == 6 || c_mode == 7)						//Night + NightHDR
			lose_all_tout = 30000000;
    	else
    		lose_all_tout = 20000000;

        if(smooth_en == 1) {
        	lose_all_tout += smooth_add_time;
        	lose_s_tout += smooth_add_time;
        	lose_b_tout += smooth_add_time;
        }

    	get_current_usec(&NowTime);
    	Get_F_Cmd_In_Capture_T1(&T1);
    	Get_F_Cmd_In_Capture_T2(&T2);
    	get_img[0] = Get_F_Cmd_In_Capture_Get_Img(0);
    	get_img[1] = Get_F_Cmd_In_Capture_Get_Img(1);
    	if(err_cnt >= 10) {			// ?????v?????ƿ??~, ?ɭP???A?d??, 10?????~?h???X???e????
			Set_F_Cmd_In_Capture_Lose_Flag(0);
			Set_F_Cmd_In_Capture_Step(0);
			db_error("Cap Lose File Err !!\n");
    	}
    	else if(get_img[0] == 0 && get_img[1] == 0) {
			if( (NowTime - T1) > lose_all_tout) {					// > lose_all_tout, Lose Big + Small
				Set_F_Cmd_In_Capture_Lose_Flag(3);
				Set_F_Cmd_In_Capture_T1();
				page = Get_F_Cmd_In_Capture_Lose_Page(0);
				Add_Cap_Lose(page);

				//Set_F_Cmd_In_Capture_Lose_Flag(0);
				//Set_F_Cmd_In_Capture_Step(0);
				db_error("Lose Big + Small !! NowTime=%lld T1=%lld\n", NowTime, T1);
			}
		}
		else if(get_img[0] == 1 && get_img[1] == 0) {
			if( (NowTime - T2) > lose_s_tout) {					// > 8s, Lose Small
				Set_F_Cmd_In_Capture_Lose_Flag(2);
				Set_F_Cmd_In_Capture_T2();
				page = Get_F_Cmd_In_Capture_Lose_Page(1);
				Add_Cap_Lose(page);
				db_error("Lose Small !! NowTime=%lld T2=%lld\n", NowTime, T2);
			}
		}
		else if(get_img[0] == 0 && get_img[1] == 1 && err == 0) {		//err=0, Lose Big, ?]??????e?j?ϦA?e?p??, ?p?G???????p?ϫh???ܱ??j??
			Set_F_Cmd_In_Capture_Lose_Flag(1);
			Set_F_Cmd_In_Capture_T1();
			page = Get_F_Cmd_In_Capture_Lose_Page(0);
			Add_Cap_Lose(page);
			db_error("Lose Big 01!!\n");
		}
		else if(get_img[0] == 0 && get_img[1] == 1 && err == 1) {		//err=1, Lose Big
			if( (NowTime - T1) > lose_b_tout) {
				Set_F_Cmd_In_Capture_Lose_Flag(1);
				Set_F_Cmd_In_Capture_T1();
				page = Get_F_Cmd_In_Capture_Lose_Page(0);
				Add_Cap_Lose(page);
				db_error("Lose Big 02!!\n");
			}
		}
        else if(get_img[0] == 1 && get_img[1] == 1) {
        	Set_F_Cmd_In_Capture_Lose_Flag(0);
        	Set_F_Cmd_In_Capture_Step(0);
        	db_debug("Get ALL Img !!\n");
        }

    	Set_F_Cmd_In_Capture_Img_Err_Cnt(err_cnt);
    }
}

void Save_Parameter_Tmp_Proc() {
	Set_Parameter_Tmp_RecCnt(rec_file_cnt);
	Set_Parameter_Tmp_CapCnt(cap_file_cnt);
				
	int epH, epL, gainH;
	get_AEG_EP_Value(&epH, &epL, &gainH);
	Set_Parameter_Tmp_AEG(epH, epL, gainH);
				
	int r, g, b;
	get_Temp_gain_RGB(&r, &g, &b);
	Set_Parameter_Tmp_GainRGB(r, g, b);
				
    Save_Parameter_Tmp_File();
}

void Load_Parameter_Tmp_Proc() {
	LoadParameterTmp();
	rec_file_cnt = Get_Parameter_Tmp_RecCnt();
	cap_file_cnt = Get_Parameter_Tmp_CapCnt();
	set_A2K_Cap_Cnt(cap_file_cnt);
}

void *uvc_thread(void)
{
    struct stat sti;
    struct timeval tv;
    int sub = 0;
    static unsigned long long curTime, lstTime=0, runTime, show_img_ok=1;
    int cntUVC=0, errUVC=0;
    unsigned size = 0, tmp = 0;
    unsigned p1, p2;
    int mode, res;
    unsigned char *tmp_buf = NULL;
    unsigned char *ptr = NULL;
    int c_mode;
    int err;
    int fpga_enc_type;
    nice(-6);    // ?վ?thread?u???v
    
    doResize_buf_init();
    while(uvc_thread_en)
    {
        get_current_usec(&curTime);
        if(curTime < lstTime) lstTime = curTime;    // rex+ 151229, ?????ҥ~???~
        else if((curTime - lstTime) >= 1000000){
            db_debug("uvc_thread: runTime=%lld cntUVC=%d errUVC=%d uvcWaitT=%lld size=%d\n", runTime, cntUVC, errUVC, uvcWaitT, buffer_length);
            lstTime = curTime;
            cnt1secUVC = cntUVC;
            cntUVC = 0;
            errUVC = 0;
            show_img_ok = 1;
        }

        if(FPGA_Ctrl_Power == 0 && Check_UVC_Fd() != -1) {

            if(save_parameter_flag == 1) {
				Save_Parameter_Tmp_Proc();
                save_parameter_flag = 0;
            }

            img_ok_flag = 0;
            uvc_err_flag = readframeonce();
            if(img_ok_flag == 0) errUVC++;
            else                 cntUVC++;
            if(uvc_err_flag == SUCCESS_LOCAL && img_ok_flag == 0 && show_img_ok == 1){
                show_img_ok = 0;
                db_error("uvc_thread: err! img_ok_flag=%d\n", img_ok_flag);
            }

            unsigned char *brgb;
            brgb = &byte_rgb[0];
            c_mode = getCameraMode();
            if(IMG_Pixelformat == V4L2_PIX_FMT_MJPEG && img_ok_flag == 1)
            {
                get_Stitching_Out(&mode, &res);
                fpga_enc_type = get_fpga_encode_type();
                if(( (decode_idx0 - decode_idx1) & 3) < 2 && buffer_length < UVC_BUF_MAX &&
                     (((res == 1 || res == 7 || res == 12) && mHWDecode == 1 && stream_flag == 2) ||
                     ((res == 2 || res == 13 || res == 14) && mHWDecode == 1 && stream_flag == 1) || mHWDecode == 0) )
                {
                	if(fpga_enc_type == 0) {
						memcpy(&decode_buf[decode_idx0 & 1], brgb, buffer_length);
						decode_len[decode_idx0 & 1] = buffer_length;
						decode_stream[decode_idx0 & 1] = stream_flag;
						decode_idx0++;
						decode_idx0 = decode_idx0 & 3;
                	}
                    save_jpeg_tmp_func(brgb, buffer_length);
                }
                else if(buffer_length >= UVC_BUF_MAX)
                    db_error("uvc_thread: buffer_length(%x) >= UVC_BUF_MAX err!\n", buffer_length);

                if( (stream_flag == 1 || stream_flag == 3) && img_ok_flag == 1) {
                    err = Get_F_Cmd_In_Capture_Lose_Flag();
                    save_jpeg_func(brgb, buffer_length, stream_flag, c_mode, err, real_length);
                    //if(stream_flag == 1 || stream_flag == 2)          // rex- 150515
                    //    save_jpeg_tmp_func(brgb, buffer_length);
                }

                if(callback_func.read_jpeg != NULL)
                    callback_func.read_jpeg(brgb, buffer_length, stream_flag);

                if(get_copy_jpeg_to_rec_en(c_mode, res, stream_flag, fpga_enc_type) == 1) {
                    copy_to_rec_buf(brgb, buffer_length, 1, NULL, 0, NULL, 0, fpga_enc_type, 0);
                }

            } // if(IMG_Pixelformat == V4L2_PIX_FMT_MJPEG)

            //check cap state
            Check_Cap_State(c_mode, img_ok_flag, uvc_err_flag);
        }
        else if(FPGA_Ctrl_Power == 1) {
            FPGA_Ctrl_Power = 2;

            stopCamera();
            fpgaPowerOff();

            uvc_thread_en = 0;
            pcm_thread_en = 0;
            st_thread_en  = 0;
			set_rec_thread_en(0);
            set_live360_thread_en(0);

            SetCapInit();
        }
        else{
            if(Check_UVC_Fd() == -1)
                uvc_err_flag = -1;
        }


        get_current_usec(&runTime);
        runTime -= curTime;
        if(runTime < 10000){
            usleep(10000 - runTime);
            get_current_usec(&runTime);
            runTime -= curTime;
        }
        uvc_proc_en = 1;
    }
}

void *st_thread(void)
{
    static unsigned long long curTime, lstTime=0, runTime, show_img_ok=1;
    nice(-6);    // ?վ?thread?u???v

    while(st_thread_en)
    {
        get_current_usec(&curTime);
        if(curTime < lstTime) lstTime = curTime;    // rex+ 151229, ?????ҥ~???~
        else if((curTime - lstTime) >= 1000000){
            db_debug("st_thread: runTime=%lld\n", runTime);
            lstTime = curTime;
        }

		if(getCameraMode() == CAMERA_MODE_3D_MODEL)
			SendSTCmd3DModel(0);

        get_current_usec(&runTime);
        runTime -= curTime;
        if(runTime < 10000){
            usleep(10000 - runTime);
            get_current_usec(&runTime);
            runTime -= curTime;
        }
    }
}

void Cmd_Buf_Init(void)
{
    memset(&Cap_Cmd_Queue, 0, sizeof(struct Cmd_Queue) );
    rec_cmd_queue_init();
    live360_cmd_queue_init();
}

void doResize_buf_init(void)
{
    int i;
    memset(&doResize_buf, 0, sizeof(doResize_buf_struc) );
    for(i = 0; i < DORESIZE_BUF_MAX; i++){
        doResize_buf.cmd[i].mode = -1;
    }
    return 0;
}

/*
 * extension: ???ɦW, (ex: ".jpg", ".mp4")
 */
void check_path_repeat(char *path, char *path_tmp, char *extension) {
	int i;
	struct stat sti;
	if(extension == NULL) sprintf(path, "%s\0", path_tmp);					//???Ƨ??S?????ɦW
	else				  sprintf(path, "%s%s\0", path_tmp, extension);
    for(i = 0; i < 100; i++){		//?ˬd?O?_????
        if(stat(path, &sti) != 0) break;
        else{
            sprintf(path, "%s(%d)%s\0", path_tmp, i+1, extension);
        }
    }
    if(i >= 100) db_error("check_path_repeat: err! %s\n", path);
}

/*
 * rex+ 151229
 *   mode_sel: 0 = .jpg (ex. /mnt/sdcard/DCIM/US_1234/P1234_0001.jpg)
 *             1 = .avi (ex. /mnt/sdcard/DCIM/US_1234/V1234_0002.avi)
 *             2 = .jpg / .thm (ex. /mnt/sdcard/DCIM/US_1234/V1234_0002.jpg)
 *             3 = ??dir_path, thm_path        //weber+ 151230
 *             //4 = save isp2 .jpg (ex. /mnt/sdcard/US360/ISP2_S0.jpg)                            // teat mode
 *             //5 = save focus & auto sitching  block .jpg (ex. /mnt/sdcard/US360/Block.jpg)        // teat mode
 *             6 = .mp4 .ts (ex. /mnt/sdcard/DCIM/US_1234/V1234_0002.mp4)
 *             //7 = save global phi adj  block .jpg (ex. /mnt/sdcard/US360/GP_Block_0.jpg)        // teat mode
 *             8 = ??dir_path, thm_path
 *             9 = .jpg (ex. /mnt/sdcard/DCIM/US_1234/H1234_0001.jpg)		//HDR
 *            10 = .jpg (ex. /mnt/sdcard/DCIM/US_1234/R1234_0001.jpg)		//RAW
 *            11 = .jpg (ex. /mnt/sdcard/DCIM/US_1234/T1234_0001.jpg)		//Timelapse
 *            12 = .png (ex. /mnt/sdcard/DCIM/US_1234/D1234_0001.png)		//Timelapse
 *            13 = .ts (ex. /mnt/sdcard/live/V1234_0002.ts)
 * */
void maek_save_file_path(int mod_sel, char *save_file_path, char *sd_path_in, char *ssid_str_in, int file_cnt)
{
    struct stat sti;
    char dir_path[128], thm_path[128], csid[5]={0,0,0,0,0};
    int i, cnt=0, isid=0;
    int len=0;
    int isp2_s, block_idx=-1;
    char path[128];
    static int thm_file_cnt = -1;		//???Y?ϻP???Ϥ??ǰt???D(???e???ϦA?e?Y??)

    sprintf(dir_path, "%s/DCIM\0", sd_path_in);                   // *sd_path_in = "/mnt/sdcard"
    if(stat(dir_path, &sti) != 0) {                             // dir_path = "/mnt/sdcard/DCIM"
        if(mkdir(dir_path, S_IRWXU) != 0)
            db_error("maek_save_file_path: create %s folder fail\n", dir_path);
    }

    sprintf(dir_path, "%s/DCIM/%s\0", sd_path_in, ssid_str_in); // *sd_path_in = "/mnt/sdcard"
    if(stat(dir_path, &sti) != 0) {                             // dir_path = "/mnt/sdcard/DCIM/US_1234"
        if(mkdir(dir_path, S_IRWXU) != 0)
            db_error("maek_save_file_path: create %s folder fail\n", dir_path);
    }
    sprintf(thm_path, "%s/DCIM/%s/THM\0", sd_path_in, ssid_str_in);
    if(stat(thm_path, &sti) != 0) {                             // thm_path = "/mnt/sdcard/DCIM/US_1234/THM"
        if(mkdir(thm_path, S_IRWXU) != 0)
            db_error("maek_save_file_path: create %s folder fail\n", thm_path);
    }

    //db_debug("maek_save_file_path: dir_path=%s\n", dir_path);
    csid[0] = ssid_str_in[3];                          // ?? US_0000 ?? 0000
    csid[1] = ssid_str_in[4];
    csid[2] = ssid_str_in[5];
    csid[3] = ssid_str_in[6];
    if(csid[0] >= '0' && csid[0] <= '9' &&
       csid[1] >= '0' && csid[1] <= '9' &&
       csid[2] >= '0' && csid[2] <= '9' &&
       csid[3] >= '0' && csid[3] <= '9')
    {
        isid = atoi(csid);
    }

    if(mod_sel == 0){                                           // .jpg , /mnt/sdcard/DCIM/US_1234/P1234_0000.jpg
        sprintf(save_file_path, "%s/P%04d_%04d.jpg\0", dir_path, isid, file_cnt);
        for(i = 0; i < 100; i++){
            if(stat(save_file_path, &sti) != 0) break;
            else {
                sprintf(save_file_path, "%s/P%04d_%04d(%d).jpg\0", dir_path, isid, file_cnt, i+1);
                thm_file_cnt = i+1;
            }
        }
        if(i >= 100) db_error("maek_save_file_name: err! %s\n", save_file_path);
    }
    else if(mod_sel == 1 || mod_sel == 2){                                                       // .avi , /mnt/sdcard/DCIM/US_1234/P1234_0001.avi
        sprintf(path, "%s/V%04d_%04d\0", dir_path, isid, file_cnt);
        check_path_repeat(&save_file_path[0], &path[0], ".avi\0");

        if(mod_sel == 2 && i < 100){   // ?s?@?Y?ϫe???j??
            if(i == 0) sprintf(save_file_path, "%s/V%04d_%04d.jpg\0", thm_path, isid, file_cnt);
            else       sprintf(save_file_path, "%s/V%04d_%04d(%d).jpg\0", thm_path, isid, file_cnt, i+1);
        }
    }
    else if(mod_sel == 3){		//thm P
        len = strlen(dir_path);
        if(len > 0) setDirPath(&dir_path[0]);
        len = strlen(thm_path);
        if(len > 0) setThmPath(&thm_path[0]);

        if(thm_file_cnt > 0)
        	sprintf(save_file_path, "%s/P%04d_%04d(%d).thm\0", thm_path, isid, file_cnt, thm_file_cnt);
        else
        	sprintf(save_file_path, "%s/P%04d_%04d.thm\0", thm_path, isid, file_cnt);
        thm_file_cnt = -1;
    }
    else if(mod_sel == 8){		//thm V
        len = strlen(dir_path);
        if(len > 0) setDirPath(&dir_path[0]);
        len = strlen(thm_path);
        if(len > 0) setThmPath(&thm_path[0]);

        sprintf(path, "%s/V%04d_%04d\0", thm_path, isid, file_cnt);
        check_path_repeat(&save_file_path[0], &path[0], ".thm\0");
    }
    else if(mod_sel == 4) {
        isp2_s = get_ISP2_Sensor();
        sprintf(save_file_path, "/mnt/sdcard/US360/Test/ISP2_S%d_%d.jpg\0", isp2_s, get_Sensor_Lens_Map_Cnt());
    }
    else if(mod_sel == 5) {
        memset(path, 0, sizeof(path));
        for(i = 0; i < 4; i++) {
            sprintf(path, "/mnt/sdcard/US360/Test/BlockFileIdx_%d.bin\0", i);
            if(stat(path,&sti) == 0) {
                block_idx = i;
                break;
            }
        }

        if(Block_File_idx == block_idx) Block_File_idx = block_idx + 2;
        if(Block_File_idx >= 4) Block_File_idx = Block_File_idx % 4;
        sprintf(save_file_path, "/mnt/sdcard/US360/Test/Block_%d.jpg\0", Block_File_idx);

        Block_File_idx++;
        if(Block_File_idx >= 4) Block_File_idx = Block_File_idx % 4;
    }
    else if(mod_sel == 6) {                                                       // .avi , /mnt/sdcard/DCIM/US_1234/P1234_0001.avi
        sprintf(path, "%s/V%04d_%04d\0", dir_path, isid, file_cnt);
        if(muxer_type == MUXER_TYPE_TS)
        	check_path_repeat(&save_file_path[0], &path[0], ".ts\0");
        else
        	check_path_repeat(&save_file_path[0], &path[0], ".mp4\0");
    }
    else if(mod_sel == 7) {
        sprintf(save_file_path, "/mnt/sdcard/US360/Test/GP_Block_%d.jpg\0", 0);
    }
    else if(mod_sel == 9) {                                                       // .jpg , /mnt/sdcard/DCIM/US_1234/H1234_0001.jpg
        sprintf(path, "%s/H%04d_%04d\0", dir_path, isid, file_cnt);
        check_path_repeat(&save_file_path[0], &path[0], ".jpg\0");
    }
    else if(mod_sel == 10) {                                                       // .jpg , /mnt/sdcard/DCIM/US_1234/R1234_0001.jpg
        sprintf(path, "%s/R%04d_%04d\0", dir_path, isid, file_cnt);
        check_path_repeat(&save_file_path[0], &path[0], ".jpg\0");
    }
    else if(mod_sel == 11) {                                                       // .jpg , /mnt/sdcard/DCIM/US_1234/T1234_0001.jpg
        sprintf(path, "%s/T%04d_%04d\0", dir_path, isid, file_cnt);
        check_path_repeat(&save_file_path[0], &path[0], ".mp4\0");
    }
    else if(mod_sel == 12) {                                                       // .png , /mnt/sdcard/DCIM/US_1234/T1234_0001.png
        sprintf(path, "/mnt/sdcard/US360/D%04d_%04d\0", isid, file_cnt);
        check_path_repeat(&save_file_path[0], &path[0], ".png\0");
    }
    else if(mod_sel == 13) {                                                       // .ts , /mnt/sdcard/live/V1234_0001.ts
        sprintf(path, "/mnt/sdcard/live/V%04d_%04d\0", isid, file_cnt);
        check_path_repeat(&save_file_path[0], &path[0], ".ts\0");
    }
}

int get_save_jpeg_en(int cap_en, int s_flag)
{
	unsigned long long capture_t, capture_t_lst;
	if(s_flag == 3) return 1;
	else {
		switch(cap_en) {
		case 1: // ????
		case 3: // Sensor_ISP2
		case 4: // Test_Block
		case 5: // Test_GP_Block 
		case 7: // TestFocus_GetRAW
		case 8: // Get_Defect_Img(5P)
			return 1;
			break;
		case 2: // ?s??
			get_current_usec(&capture_t);
			if( ( (capture_t - capture_t_lst) / 1000L) >= getCaptureIntervalTime()) {
				capture_t_lst = capture_t;
				return 1;
			}
			break;
		case 6: // RAW
			if(s_flag == 1)
				return 1;
			break;
		}
	}
	return 0;
}

void set_write_file_error(int err) {
	Write_File_Error = err;
}
int get_write_file_error(void) {
	return Write_File_Error;
}

int doDrivingModeDeleteFile() {
    char sd_path[128];
	char dir_path[128];
    getSdPath(&sd_path[0], sizeof(sd_path));
	sprintf(dir_path, "%s/DCIM/%s\0", sd_path, mSSID);
    return DrivingModeDeleteFile(dir_path);
}

void save_jpeg_func(unsigned char *img, int size, int s_flag, int c_mode, int err, int r_size)
{
    FILE* fp;
    int fp2fd;
    char path[128], path_tmp[128], sd_path[128];
    static char s_dir_path[128];
    unsigned int i, j, jpeg_size, len=0;
    unsigned char *tmp;
    unsigned char *tmpWhi, *tmpWlo, *tmpHhi, *tmpHlo;
    int width, height;
    int cap_en_tmp = 0;
    int cmd_p1, cmd_p2;
    int rp1, rp2;
    static int thm_cnt_tmp = -1;
    char *ptr;
    struct stat sti;
    int tool_cmd = get_TestToolCmd();
    int thmSave = 0;
    int err_cnt=0, err_len=0;
    static char thm_path[128];
    char *p1=NULL, *p2=NULL;
    int len1=0, len2=0;
    int ret = 0;
    int single_pic = get_C_Mode_Single_Pic(c_mode);
    jpeg_size = size;
    
    getSdPath(&sd_path[0], sizeof(sd_path));

    cmd_p1 = Cap_Cmd_Queue.P1;
    cmd_p2 = Cap_Cmd_Queue.P2;
    if(cmd_p2 != cmd_p1)
        cap_en_tmp = Cap_Cmd_Queue.queue[cmd_p2];

    if(get_save_jpeg_en(cap_en_tmp, s_flag) == 1)
    {
        // ?P?_?O?_?? ?j??  ???e??
        tmp = img;
        tmpHhi = tmp; tmpHlo = tmp;
        tmpWhi = tmp; tmpWlo = tmp;
        for(i = 0; i < 2048; i++) {
            if(*tmp == 0xFF && *(tmp+1) == 0xC0) {
                tmpHhi = tmp + 5;
                tmpHlo = tmp + 6;
                tmpWhi = tmp + 7;
                tmpWlo = tmp + 8;
                //db_debug("Whi %02x Wlo %02x W %d\n", *tmpWhi, *tmpWlo, (*tmpWhi << 8) | *tmpWlo);
                break;
            }
            tmp++;
        }
        width  = ( (*tmpWhi << 8) | *tmpWlo);
        height = ( (*tmpHhi << 8) | *tmpHlo);

        
        if(s_flag == 3 && width == S2_RES_THM_WIDTH && height == S2_RES_THM_HEIGHT) {
        	if(single_pic == 1) {		//Cap || WDR	+P
        		maek_save_file_path(3, path, sd_path, mSSID, thm_cnt_tmp);         //THM
            	if(thm_cnt_tmp == -1)	//???j??
            		memcpy(&thm_path[0], &path[0], sizeof(thm_path) );
            	else
            		thm_path[0] = '/0';
        	}
        	else {				//Rec	+V
        		maek_save_file_path(8, path, sd_path, mSSID, cap_file_cnt);         //THM
        		thm_path[0] = '/0';
        	}

			rp1 = doResize_buf.P1;
			rp2 = doResize_buf.P2;
			db_debug("save_jpeg_func: rp1=%d rp2=%d mode=%d thm=%d\n", rp1, rp2, doResize_buf.cmd[rp1].mode, cap_file_cnt);
			thmSave = 1;
			thm_cnt_tmp = -1;
        }
        else if(cap_en_tmp == 1 || cap_en_tmp == 2){	//1=???? / 2=?s??
            maek_save_file_path(0, path, sd_path, mSSID, cap_file_cnt);
            Save_Jpeg_Now_Cnt++;
            if(Capture_Is_Finish() == -1){
                db_error("save_jpeg_func: err-1! now=%d end=%d\n", Save_Jpeg_Now_Cnt, Save_Jpeg_End_Cnt);
            }
            SaveSmoothFile(&path[0]);

            //???j??, ???s?R?Wthm??
            if(err == 1) {
				if(stat(thm_path, &sti) == 0) {
					memset(&path_tmp[0], 0, sizeof(path_tmp) );
					p1 = strrchr(thm_path, '_');
					len1 = p1 - thm_path;
					memcpy(&path_tmp[0], &thm_path[0], len1);

					p1 = strrchr(path, '_');
					p2 = strstr(path, ".jpg");
					len2 = p2 - p1;
					memcpy(&path_tmp[len1], p1, len2);

					sprintf(path_tmp, "%s.thm\0", path_tmp);
					rename(thm_path, path_tmp);
				}
            }
        }
        else if(cap_en_tmp == 6) {                                               //AEB & RAW
                if(Save_Jpeg_Now_Cnt == 0 && (tool_cmd&0xFF) != 2 && ( (tool_cmd >> 8)&0xFF) != 101){		//???ռҦ?Focus??RAW???ݭn?ظ??Ƨ?
                    if(c_mode == 3)             // 3: AEB 3,5,7?i
                        maek_save_file_path(9, path_tmp, sd_path, mSSID, cap_file_cnt);
                    else if(c_mode == 4)
                    	maek_save_file_path(10, path_tmp, sd_path, mSSID, cap_file_cnt);
                    ptr = &path_tmp[0];
                    for(i = 0; i < 123; i++) {
                        if(*ptr == '.' && *(ptr+1) == 'j' && *(ptr+2) == 'p' && *(ptr+3) == 'g')
                            break;
                        ptr++;
                    }
                    memcpy(s_dir_path, &path_tmp[0], i);
                    s_dir_path[i] = 0;
                    if(stat(s_dir_path, &sti) != 0) {
                        if(mkdir(s_dir_path, S_IRWXU) != 0)
                            db_error("save_jpeg_func: create %s folder fail\n", s_dir_path);
                    }

                    db_debug("saveHDR: thmP1=%d path=%s\n", rec2thm.P1, s_dir_path);
                    rec2thm.en[rec2thm.P1] = 1;         // ?s?@hdr/raw???v?Y??
                    rec2thm.mode[rec2thm.P1] = (c_mode == 3)? 3: 4; // CAP=0, REC=1, Lapse=2, HDR=3, RAW=4
                    memcpy(&rec2thm.path[rec2thm.P1][0], &s_dir_path[0], 128);
                    rec2thm.P1++;
                    if(rec2thm.P1 >= REC_2_THM_BUF_MAX) rec2thm.P1 = 0;
                }

                if(c_mode == 3){                // 3: AEB 3,5,7?i
                    int fcnt = get_AEB_Frame_Cnt();
                    int fnum = (Save_Jpeg_Now_Cnt + (fcnt>>1)) % fcnt;
                    sprintf(&path[0], "%s/H%c%c%c%c_%04d_S%d.jpg\0", s_dir_path, mSSID[3], mSSID[4], mSSID[5], mSSID[6],
                        cap_file_cnt, fnum+1);
                }
                else if(c_mode == 4) {
                	if( (tool_cmd&0xFF) == 2 && ( (tool_cmd >> 8)&0xFF) == 101)		//TestTool ???j??RAW??
                		sprintf(&path[0], "/mnt/sdcard/US360/Test/RAW_S%d.jpg\0", Save_Jpeg_Now_Cnt);
                	else
                		sprintf(&path[0], "%s/R%c%c%c%c_%04d_S%d.jpg\0", s_dir_path, mSSID[3], mSSID[4], mSSID[5], mSSID[6],
                				cap_file_cnt, Save_Jpeg_Now_Cnt+1);
                }

                Save_Jpeg_Now_Cnt++;
                if(Capture_Is_Finish() == -1){  // ??3?i??????Now_Cnt=3,End_Cnt=3
                    db_error("save_jpeg_func: err-2! now=%d, end=%d\n", Save_Jpeg_Now_Cnt, Save_Jpeg_End_Cnt);
                }
        }
        else if(cap_en_tmp == 8) {                                               //Defect_Img(5P)
            if(stat("/mnt/sdcard/US360/Defect\0", &sti) != 0) {
                if(mkdir("/mnt/sdcard/US360/Defect\0", S_IRWXU) != 0) {
                	ret = 0;
                    db_error("save_jpeg_func: create /mnt/sdcard/US360/Defect folder fail\n");
                }
                else ret = 1;
            }
            else ret = 1;

            if(ret == 1) {
				if(stat("/mnt/sdcard/US360/Defect/Tmp\0", &sti) != 0) {
					if(mkdir("/mnt/sdcard/US360/Defect/Tmp\0", S_IRWXU) != 0) {
						ret = 0;
						db_error("save_jpeg_func: create /mnt/sdcard/US360/Defect/Tmp folder fail\n");
					}
					else ret = 1;
				}
				else ret = 1;
            }

            if(ret == 1) {
            	sprintf(&path[0], "/mnt/sdcard/US360/Defect/Tmp/Defect_S%d_tmp.jpg\0", Save_Jpeg_Now_Cnt);
				Save_Jpeg_Now_Cnt++;
				if(Capture_Is_Finish() == -1){  // ??3?i??????Now_Cnt=3,End_Cnt=3
					db_error("save_jpeg_func: err-2! now=%d, end=%d\n", Save_Jpeg_Now_Cnt, Save_Jpeg_End_Cnt);
				}
            }
        }
        else if(cap_en_tmp == 3)
            maek_save_file_path(4, path, sd_path, mSSID, cap_file_cnt);        //Test ISP2
        else if(cap_en_tmp == 4)
            maek_save_file_path(5, path, sd_path, mSSID, cap_file_cnt);        //Test Block
        else if(cap_en_tmp == 5)
            maek_save_file_path(7, path, sd_path, mSSID, cap_file_cnt);        //Test GP_Block

        tmp = img;
        fp = fopen(path, "wb");
        Write_JPEG_Real_Size(img, r_size);
        len = Add_Panorama_Header(width, height, img, fp);
        if(fp != NULL) {
        	err_cnt=0; err_len=0;
        	del_jpeg_error_code(img, jpeg_size, &err_cnt, &err_len);

            if(len >= 0) {
            	if(err_cnt == 0) {
            		ret = fwrite(&tmp[len], (jpeg_size - len), 1, fp);
            		sd_freesize -= ret;
            	}
            	else {
            		ret = fwrite(&tmp[len], (jpeg_size - len - err_len), 1, fp);
            		sd_freesize -= ret;
            		ret = fwrite(&tmp[jpeg_size - err_len + err_cnt], (err_len - err_cnt), 1, fp);
            		sd_freesize -= ret;
            	}
            }
            else {
            	if(err_cnt == 0) {
            		ret = fwrite(&img[0], jpeg_size, 1, fp);
            		sd_freesize -= ret;
            	}
            	else {
            		ret = fwrite(&img[0], (jpeg_size - err_len), 1, fp);
            		sd_freesize -= ret;
            		ret = fwrite(&img[jpeg_size - err_len + err_cnt], (err_len - err_cnt), 1, fp);
            		sd_freesize -= ret;
            	}
            }

            if(ret == 0) {
            	setSdState(3);
            	Write_File_Error = 1;
            }

            db_debug("save_jpeg_func: path=%s jpeg_size=%d\n", path, jpeg_size);
        }
        else errnoexit(path);

        if(fp != NULL){
            fflush(fp);
            fp2fd = fileno(fp);
            fsync(fp2fd);
            fclose(fp);
            close(fp2fd);
        }

        if(thmSave == 1){
        	if(doResize_buf.cmd[rp1].mode == -1) {
				if(single_pic == 1)
					doResize_buf.cmd[rp1].mode = 0;      // Cap Mode +P
				else
					doResize_buf.cmd[rp1].mode = 1;      // Rec Mode +V

				int len = strlen(path);
				if(len > 0 && len < sizeof(doResize_buf.cmd[rp1].doResizePath)){
					memcpy(&doResize_buf.cmd[rp1].doResizePath[0], path, len);
					doResize_buf.cmd[rp1].doResizePath[len] = 0;                // ?????줸
				}
				doResize_buf.cmd[rp1].cap_file_cnt = cap_file_cnt;

				doResize_buf.P1++;
				if(doResize_buf.P1 >= DORESIZE_BUF_MAX) doResize_buf.P1 = 0;
			}

//        	if(cap_en_tmp == 1 || cap_en_tmp == 2){
//				cap_file_cnt++;
//				if(cap_file_cnt > 9999) cap_file_cnt = 0;
//				set_A2K_Cap_Cnt(cap_file_cnt);
//
//				save_parameter_flag = 1;
//
//				Cap_Cmd_Queue.queue[cmd_p2] = 0;
//				cmd_p2++;
//				if(cmd_p2 >= CMD_QUEUE_MAX) cmd_p2 = 0;
//				Cap_Cmd_Queue.P2 = cmd_p2;
//        	}

        	Set_F_Cmd_In_Capture_Get_Img(1, 1);
        }

        if(s_flag != 3) {
        	if(cap_en_tmp == 1 || cap_en_tmp == 2) {
				if(err == 1) thm_cnt_tmp = -1;
				else		 thm_cnt_tmp = cap_file_cnt;                               // ??THM?ɦW?ϥ?
        	}
        	else
        		thm_cnt_tmp = -1;

            if(cap_en_tmp != 8) {
                if(Capture_Is_Finish() == 1){
                    cap_file_cnt++;                                     // ?ɦW?y?????[1
                    //db_debug("save_jpeg_func: debug! cnt=%d mode=%d now=%d end=%d\n", 
                    //        cap_file_cnt, c_mode, Save_Jpeg_Now_Cnt, Save_Jpeg_End_Cnt);

                    if(getDrivingRecordMode() == 1 && sd_freesize < SD_CARD_MIN_SIZE) {
                        doDrivingModeDeleteFile();
                    }
                }
				if(cap_file_cnt > 9999) cap_file_cnt = 0;
				set_A2K_Cap_Cnt(cap_file_cnt);

				save_parameter_flag = 1;
            }

			Cap_Cmd_Queue.queue[cmd_p2] = 0;
			cmd_p2++;
			if(cmd_p2 >= CMD_QUEUE_MAX) cmd_p2 = 0;
			Cap_Cmd_Queue.P2 = cmd_p2;

			Set_F_Cmd_In_Capture_Get_Img(0, 1);
			Set_F_Cmd_In_Capture_T2();
        }
        //BSmooth_Function = cap_file_cnt & 0x1;       // smooth???եΡA??2?i???ݬO?_???ﵽ
        //if(BSmooth_Function == 1) BSmooth_Function = 4;
    }

    return 0;
}

void save_jpeg_tmp_func(char *img, int size)
{
    FILE* fp;
    int fp2fd;
    char path1[128], path2[128];
    unsigned int i, j, state;
    int rp1, rp2;
    int p1, p2, en=-1, mode=-1;
    char *ptr=NULL;

    p1 = rec2thm.P1;
    p2 = rec2thm.P2;
    if(p1 != p2){
        en = rec2thm.en[p2];
        mode = rec2thm.mode[p2];
        ptr = &rec2thm.path[p2][0];
        if(en == 0){
            db_error("save_jpeg_tmp_func: err! p1=%d p2=%d en=%d\n", rec2thm.P1, rec2thm.P2, en);
            rec2thm.en[p2] = 0;
            rec2thm.P2++;
            if(rec2thm.P2 >= REC_2_THM_BUF_MAX) rec2thm.P2 = 0;
        }
        else{
            db_debug("save_jpeg_tmp_func: p1=%d p2=%d size=%d\n", rec2thm.P1, rec2thm.P2, size);
        }

        memset(path1, 0, sizeof(path1));
        memset(path2, 0, sizeof(path2));
        //make path=/storage/44C3-17F7/DCIM/US_4456/V4456_0897
        //to   path=/storage/44C3-17F7/DCIM/US_4456/THM/V4456_0897
        state = 0;
        for(j = 0; j < 128; j++){
            switch(state){
            case 0:
                if(*ptr == 'D' && *(ptr+1) == 'C' && *(ptr+2) == 'I' && *(ptr+3) == 'M' && *(ptr+4) == '/')
                    state = 1;
                ptr++;
                break;
            case 1:
                if(*ptr == '/') state = 2;
                ptr++;
                break;
            case 2:
                if(*ptr == '/'){
                    state = 3;
                    memcpy(path1, &rec2thm.path[p2][0], j);
                    strcat(path1, "/THM");
                    strcat(path1, ptr);
                    j = 128; //break;
                }
                ptr++;
                break;
            }
        }
        if(state != 3){
            db_debug("save_jpeg_tmp_func: state=%d path1=%s\n", state, path1);
            memcpy(path1, &rec2thm.path[p2][0], sizeof(path1));
        }
        
        ptr = &path1[0];
        
        for(i = 0; i < 128; i++) {
            if((*ptr == '.' && *(ptr+1) == 'a' && *(ptr+2) == 'v' && *(ptr+3) == 'i') ||
               (*ptr == '.' && *(ptr+1) == 'm' && *(ptr+2) == 'p' && *(ptr+3) == '4') ||
               (*ptr == '.' && *(ptr+1) == 'j' && *(ptr+2) == 'p' && *(ptr+3) == 'g') ||
               (*ptr == '\0'))
                break;
            ptr++;
        }
        if(i < 123){        // (128-5)
            memcpy(path2, path1, i);
            strcat(path2, ".thm");
        }
        rec2thm.en[p2] = 0;
        rec2thm.P2++;
        if(rec2thm.P2 >= REC_2_THM_BUF_MAX) rec2thm.P2 = 0;
    }

    if(en == 1 && path2[0] != 0){

        db_debug("save_jpeg_tmp_func: size=%d path2=%s\n", size, path2);

        // rex- 180515, ?Y?ϥ??s??
        fp = fopen(path2, "wb");
        if(mode >= 0 && mode < 6){
            if(iconThmFile[mode] != NULL)
                fwrite(iconThmFile[mode], iconThmSize[mode], 1, fp);
        }
        fflush(fp);
        fp2fd = fileno(fp);
        fsync(fp2fd);
        fclose(fp);
        close(fp2fd);

        // ?Ұ?Main.java?ǰewifi?p??
        rp1 = doResize_buf.P1;
        rp2 = doResize_buf.P2;
        if(doResize_buf.cmd[rp1].mode == -1) {
            doResize_buf.cmd[rp1].mode = mode;
            db_debug("save_jpeg_tmp_func: mode=%d rp1=%d\n", mode, rp1);
            
            int len = strlen(path2);
            if(len < sizeof(doResize_buf.cmd[rp1].doResizePath)){
                memcpy(&doResize_buf.cmd[rp1].doResizePath[0], path2, len);
                doResize_buf.cmd[rp1].doResizePath[len] = 0;
            }
            doResize_buf.cmd[rp1].rec_file_cnt = rec_file_cnt;
            doResize_buf.P1++;
            if(doResize_buf.P1 >= DORESIZE_BUF_MAX) doResize_buf.P1 = 0;
        }

    }
}

/*
 *    ???o?v??, value[2]
 *    video_buf: JPEG DATA
 *    value[0]: JPEG len
 *    value[1]: Stream
 */
int pixeltobmp(char *video_buf, int *value, int size_max)
{
    int len = 0;
    if(IMG_Pixelformat == V4L2_PIX_FMT_MJPEG) {
        if(decode_idx1 != decode_idx0) {
            len = decode_len[decode_idx1 & 1];
            if(len < size_max) {
				memcpy(video_buf, &decode_buf[decode_idx1 & 1][0], len);
				*value     = len;
				*(value+1) = decode_stream[decode_idx1 & 1];
            }
            else {
            	len = 0;
				*value     = 0;
				*(value+1) = 0;
            }
            decode_idx1++;
            decode_idx1 = decode_idx1 & 3;
        }
        else return 0;
    }
    return len;
}

void getSysCycleCount(int *value)
{
//tmp    extern int cnt1secDec, cnt1secEnc, cnt1secShw;
    *value     = cnt1secUVC;
//tmp    *(value+1) = cnt1secDec;
//tmp    *(value+2) = cnt1secEnc;
//tmp    *(value+3) = cnt1secShw;
}

int Time_Lapse_Init(int time_lapse, int res)
{
    switch(time_lapse) {
    case 0: Time_Lapse_ms =     0L; break;
    case 1: Time_Lapse_ms =   900L; break;
    case 2: Time_Lapse_ms =  1900L; break;
    case 3: Time_Lapse_ms =  4900L; break;
    case 4: Time_Lapse_ms =  9900L; break;
    case 5: Time_Lapse_ms = 29900L; break;
    case 6: Time_Lapse_ms = 59900L; break;
    case 7:    // Full Spees(0.1s)
		if(res == 1)       Time_Lapse_ms = 900L;	//12K
		else if(res == 7)  Time_Lapse_ms = 490L;	//8K
		else if(res == 12) Time_Lapse_ms = 290L;	//6K
		else if(res == 2)  Time_Lapse_ms =  90L;	//4K
        break;
    default: Time_Lapse_ms =    0L; break;
    }
	set_timelapse_flag(0);
    timelapse_time_init();
}

void get_timelapse_ms(unsigned long long *time) {
	*time = Time_Lapse_ms;
}

void pcm_buf_init(int freq, int mode, int a_src, int islive)
{
    //pcm_buf.en    = 0;
    pcm_buf.state = -2;
    if(freq == 0)
        pcm_buf.fps = 300;
    else
        pcm_buf.fps = 250;
    pcm_buf.P1    = 0;
    pcm_buf.P2    = 0;
    pcm_buf.P3    = 0;
    pcm_buf.jump  = 0;
    pcm_buf.size  = 0;
    switch(mode) {
    case 3:
    	if(islive == 1) {
    		if(a_src == 1) pcm_buf.delayTime = 8000000;		//8s
    		else		   pcm_buf.delayTime = 8000000;
    	}
    	else {
    		if(a_src == 1) pcm_buf.delayTime = 1000000;		//1s
    		else		   pcm_buf.delayTime = 500000;		//0.5s
    	}
    	break;
    case 4:
    case 5:
    default:
    	if(islive == 1) {
    		if(a_src == 1) pcm_buf.delayTime = 8000000;		//8s
    		else		   pcm_buf.delayTime = 8000000;
    	}
    	else {
        	if(a_src == 1) pcm_buf.delayTime = 500000;		//0.5s
        	else		   pcm_buf.delayTime = 0;
    	}
    	break;
    }
    memset(&pcm_buf.buf[0], 0, PCM_BUF_MAX);
    return;
}

void set_rtmp_audio_rate(int rate) {
	rtmp_audio_rate = rate;
}
void set_rtmp_audio_channel(int channel) {
	rtmp_audio_channel = channel;
}
void set_rtmp_audio_bit(int bit) {
	rtmp_audio_bit = bit;
}
int copy_to_rtmp_audio_buff(char *sbuf, int len)
{
    char *tbuf = rtmp_audio_buff;

    if(rtmp_audio_irq_f == 1){
        //db_error("copy_to_rtsp_audio_buff: irq_f==1\n");
        return -1;
    }
    if(len > RTMP_AUDIO_BUFF_MAX){            // 1sec
        db_error("copy_to_rtmp_audio_buff: len>96000\n");
        return 0;
    }

    rtmp_audio_irq_f = 1;
    if((rtmp_audio_size + len) > RTMP_AUDIO_BUFF_MAX){
        rtmp_audio_size = 0;
    }

    tbuf += rtmp_audio_size;
    memcpy(tbuf, sbuf, len);
    rtmp_audio_size += len;
    rtmp_audio_irq_f = 0;
    return(rtmp_audio_size);
}

int getMicRate() {
    return rtmp_audio_rate;
}

int getMicChannel() {
    return rtmp_audio_channel;
}

int getMicBit() {
    return rtmp_audio_bit;
}

//jint Java_net_ossrs_yasea_SrsPublisher_getMicBufferSize(JNIEnv* env, jobject thiz)
int getMicBufferSize() {
    return tinycap_size;
}

//jbyteArray Java_net_ossrs_yasea_SrsPublisher_getMicBufferData( JNIEnv* env, jobject thiz)
void getMicBufferData(char *buf)
{
	rtmp_audio_irq_f = 1;
	int size = tinycap_size;
	
	if(rtmp_audio_size < size || size < 1){
		rtmp_audio_irq_f = 0;
		return NULL;
	}
	memcpy(buf, rtmp_audio_buff, size);
	rtmp_audio_size = 0;
	rtmp_audio_irq_f = 0;
}

void *pcm_thread(void)
{
	int tmp, pcm_tmp;
    static unsigned long long curTime, lstTime=0, runTime;
    unsigned p1, p2;
    int ret, rtsp_audio_en=0;
	int mic_delay_size = 0;
    int size=0, aac_len=0;
    char *buf;
    //nice(-6);    // ?վ?thread?u???v


    while(pcm_thread_en)
    {
//tmp    	rtsp_audio_en = get_rtsp_mic_alive();
        get_current_usec(&curTime);
        if(curTime < lstTime) lstTime = curTime;    // rex+ 151229, ?????ҥ~???~
        else if((curTime - lstTime) >= 1000000){
            db_debug("pcm_thread: runTime=%d, state=%d mic_a=%d rtsp_a=%d\n",
                        (int)runTime, pcm_buf.state, mic_is_alive, rtsp_audio_en);
            lstTime = curTime;
        }

        if(mic_is_alive == 1 || rtsp_audio_en == 1){
            if(pcm_buf.state == -2) {
                if(tinycap_buf == NULL)
                    ret = recorderWAV(0, (pcm_buf.fps / 10) );               // keep rtsp alive
                else
                    ret = recorderWAV(1, (pcm_buf.fps / 10) );               // keep rtsp alive

                if(ret < 0 && (rtsp_audio_en == 1 || mic_is_alive == 1)){    // remove mic
                    rtsp_audio_en = 0;
                    mic_is_alive = 0;
                }
            }
            else{
				p1 = pcm_buf.P1;
				p2 = pcm_buf.P2;
				int freq = get_ISP_AEG_EP_IMX222_Freq();
				if(pcm_buf.state == 0) {
					if(freq == 0) pcm_buf.fps = 300;
					else          pcm_buf.fps = 250;
				}

				//get audio
				if(pcm_buf.fps >= 50) {
					if(pcm_buf.state == -1) {
						//recorderWAV(pcm_buf.state, (pcm_buf.fps / 10) );
						////pcm_buf.en = 0;
						pcm_buf.state = -2;
					} else {
                        if(pcm_buf.state == 0 && tinycap_buf != NULL){
                            recorderWAV(-1, (pcm_buf.fps / 10) );               // restart
                        }
						recorderWAV(pcm_buf.state, (pcm_buf.fps / 10) );
						if(pcm_buf.state == 0) {								
//tmp							if(muxer_type == MUXER_TYPE_TS || Live360_Buf.state != -2)
//tmp								AacEnc_Init(rtmp_audio_rate, rtmp_audio_channel, rtmp_audio_bit);					
							pcm_buf.state = 1;
						}
					}
				}

				//copy to buf
				buf = NULL;
#ifdef __CLOSE_CODE__	//tmp				
				if(muxer_type == MUXER_TYPE_TS || Live360_Buf.state != -2) {
					aac_len = AacEnc_Proc(&tinycap_buf[0], tinycap_size, &aac_enc_buf[0]);
					size = aac_len;
					buf = &aac_enc_buf[0];
				}
				else 
#endif	//__CLOSE_CODE__					
				{
					size = tinycap_size;
					buf = &tinycap_buf[0];
				}

				if(buf != NULL && pcm_buf.fps >= 50 && size > 0) {
					pcm_buf.size = size;
					pcm_tmp = p1 + pcm_buf.size;
					if(pcm_tmp < PCM_BUF_MAX) {
						if(p2 > p1 && pcm_tmp > p2) {
							pcm_buf.jump = 1;
							db_error("pcm_thread() 00-1 pcm_buf.jump == 1 P1 %d P2 %d size %d\n", p1, p2, pcm_buf.size);
						}
						else {
							memcpy(&pcm_buf.buf[p1], &buf[0], pcm_buf.size);
							pcm_buf.P1 = pcm_tmp & (PCM_BUF_MAX-1);
							pcm_buf.jump = 0;
						}
					}
					else {
						if(p2 > p1 && pcm_tmp > p2) {
							pcm_buf.jump = 1;
							db_error("pcm_thread() 01-1 pcm_buf.jump == 1\n");
						}
						else if(p1 > p2 && pcm_tmp >= PCM_BUF_MAX && (pcm_tmp - PCM_BUF_MAX) > p2) {
							pcm_buf.jump = 1;
							db_error("pcm_thread() 01-2 pcm_buf.jump == 1\n");
						}
						else {
							memcpy(&pcm_buf.buf[p1], &buf[0], PCM_BUF_MAX - p1);
							memcpy(&pcm_buf.buf[0], &buf[PCM_BUF_MAX - p1], pcm_tmp - PCM_BUF_MAX);
							pcm_buf.P1 = (pcm_tmp - PCM_BUF_MAX) & (PCM_BUF_MAX-1);
							pcm_buf.jump = 0;
						}
					}
				}

			}    // if(pcm_buf.state != -2)
    	}
        get_current_usec(&runTime);
        runTime -= curTime;
        if(runTime < 10000){
            usleep(10000 - runTime);
            get_current_usec(&runTime);
            runTime -= curTime;
        }
    }    //while(1)
}

//--------------------------------------------------------------------

void calSdFreeSize(unsigned long long *size)
{
    char sd_path[128];
	struct stat sti;
    struct statfs diskInfo;
    unsigned long long totalBlocks;
    unsigned long long freeDisk;

    getSdPath(&sd_path[0], sizeof(sd_path));
    if(stat(sd_path, &sti) == 0) {
		statfs(sd_path, &diskInfo);
		totalBlocks = diskInfo.f_bsize;
		freeDisk = diskInfo.f_bfree * totalBlocks;
    }
    else
    	freeDisk = 0;

	*size = freeDisk;
}

void getSDAllSize(unsigned long long *size)
{
    char sd_path[128];
    struct statfs diskInfo;
    unsigned long long totalBlocks;
    unsigned long long totalSize;

    getSdPath(&sd_path[0], sizeof(sd_path));
    statfs(sd_path, &diskInfo);
    totalBlocks = diskInfo.f_bsize;
    totalSize = diskInfo.f_blocks * totalBlocks;
	*size = totalSize;
}

int GetSelfTimerSec(void) {
	return mSelfTimerSec;
}

void SetTakePicture(int value) {
	mTakePicture = value;
}

void SetBurstCount(int cnt) {
	mBurstCount = cnt;
}

/*
 *  input
 *    sel --  0:power, 1:wifi
 *
 * */
void set_timeout_start(int sel)
{
    switch(sel){
    case 0: get_current_usec(&toutPowerT1);
//tmp      		if(getMcuVersion() > 12 && getWifiDisableTime() > 0){
//tmp      			setMCULedSetting(MCU_SLEEP_TIMER_START, 0);
//tmp      		}
      		break;    // POWER
    case 1: get_current_usec(&toutWifiT1);   break;    // WIFI
    case 2: get_current_usec(&toutBurstT1);  break;    // ?s??
    case 3: get_current_usec(&toutSelfT1);   break;    // ?۩?
    case 4: get_current_usec(&toutLongKey);  break;    // ?????ƥ?
    case 5: get_current_usec(&toutSaveT1);   break;    // ?s??
    case 6: get_current_usec(&toutTakeT1);   break;    // ????
    case 7: get_current_usec(&toutPowerKey); break;    // power key
    }
}

void get_timeout_start(int sel, unsigned long long *time)
{
    switch(sel){
    case 0: *time = toutPowerT1; break;    // POWER
    case 1: *time = toutWifiT1;   break;    // WIFI
    case 2: *time = toutBurstT1;  break;    // ?s??
    case 3: *time = toutSelfT1;   break;    // ?۩?
    case 4: *time = toutLongKey;  break;    // ?????ƥ?
    case 5: *time = toutSaveT1;   break;    // ?s??
    case 6: *time = toutTakeT1;   break;    // ????
    case 7: *time = toutPowerKey; break;    // power key
    }
}

/*
 * return -- 0:pass, 1:need doing
 * */
int check_timeout_start(int sel, unsigned long long msec)
{
    unsigned long long t1=0, t2=0;
    get_current_usec(&t2);
    switch(sel){
    case 0: t1 = toutPowerT1;     break;
    case 1: t1 = toutWifiT1;      break;
    case 2: t1 = toutBurstT1;     break;
    case 3: t1 = toutSelfT1;      break;
    case 4: t1 = toutLongKey;     break;
    case 5: t1 = toutSaveT1;      break;
    case 6: t1 = toutTakeT1;      break;
    case 7: t1 = toutPowerKey;    break;
    }
    if(t1 == 0) return -1;
    if(t2 < t1){
        switch(sel){
        case 0: toutPowerT1 = t2; break;
        case 1: toutWifiT1  = t2; break;
        case 2: toutBurstT1 = t2; break;
        case 3: toutSelfT1  = t2; break;
        case 4: toutLongKey = t2; break;
        case 5: toutSaveT1  = t2; break;
        case 6: toutTakeT1  = t2; break;
        case 7: toutPowerKey= t2; break;
        }
    }
    else{
        if( (t2 - t1) >= (msec * 1000) ){
            switch(sel){
            case 0: toutPowerT1 = 0; break;
            case 1: toutWifiT1  = 0; break;
            case 2: toutBurstT1 = 0; break;
            case 3: toutSelfT1  = 0; break;
            case 4: toutLongKey = 0; break;
            case 5: toutSaveT1  = 0; break;
            case 6: toutTakeT1  = 0; break;
            case 7: toutPowerKey= 0; break;
            }
            return 1;
        }
    }
    return 0;
}

void SetPlaySoundFlag(int flag) {
	play_sound_flag = flag;
}
int GetPlaySoundFlag(void) {
	return play_sound_flag;
}

void SetWifi_mGetEstimateEn_jni(int en) {
/*	SetWifi_mGetEstimateEn*/
	return;
}

/*void *thread_20ms(void)
{
    static unsigned long long curTime, lstTime=0, runTime, nowTime;
    static int selfTimeMode = 0;
    int selfie_time;
    //nice(-6);    // ?վ?thread?u???v

    while(thread_20ms_en)
    {
        get_current_usec(&curTime);
        if(curTime < lstTime) lstTime = curTime;
        else if((curTime - lstTime) >= 1000000){
            db_debug("thread_20ms: runTime=%d\n", (int)runTime);
            lstTime = curTime;
        }

        selfie_time = getSelfieTime();
        if(mTakePicture > 0){                    // ???i?Ҧ?
            if(check_timeout_start(6, 500) == 1){
                db_debug("mTakePucture = 0\n");
                mTakePicture = 0;
            }
        }
        else if(mBurstCount > 0){                // ?s???Ҧ?
            if(check_timeout_start(2, mBurstCount*getCaptureIntervalTime()) == 1){
                mBurstCount = 0;
            }
        }
        else if(selfie_time > 0){                    // ?۩??Ҧ?
        	mSelfTimerSec = selfie_time - 1;
            setSelfieTime(0);
//tmp            paintOLEDSelfTime(mSelfTimerSec);
            db_debug("selfmode selfTime:%d\n", mSelfTimerSec);
            set_timeout_start(3);
        }
        else if(mSelfTimerSec > 0){
            if(check_timeout_start(3, mSelfTimerSec*1000) == 1){
            	db_debug("selfmode selfFunction in, sd_state=%d\n", getSdState());
                mSelfTimerSec = 0;
                selfTimeMode = 0;
//tmp                ChangeLedMode(ledControlMode);
                if(getSdState() == 1){
                	Ctrl_Rec_Cap(getCameraMode(), getCaptureCnt(), getRecordEn());
                	SetWifi_mGetEstimateEn_jni(1);		//wifiSerThe.mGetEstimateEn = 1;
                }
                else{
                	play_sound_flag = 11;	//playSound(11);
                }

            }else{
            	get_current_usec(&nowTime);
				unsigned long long time;
				get_timeout_start(3, &time);
            	long leftTime = (mSelfTimerSec * 1000000) - (nowTime - time);
            	if(selfTimeMode == 0){
            		selfTimeMode = 1;
            	}else if(selfTimeMode == 1){
            		if(leftTime < 4000){
            			selfTimeMode = 2;
            		}else{
//tmp            			ChangeLedMode(5);
            		}
            	}else if(selfTimeMode == 2){
            		if(leftTime < 1000){
            			selfTimeMode = 3;
//tmp            			ChangeLedMode(7);
            		}else{
//tmp            			ChangeLedMode(6);
            		}
            	}
            }
        }

        get_current_usec(&runTime);
        runTime -= curTime;
        if(runTime < 20000){
            usleep(20000 - runTime);
            get_current_usec(&runTime);
            runTime -= curTime;
        }
    }    //while(1)
}*/

/*void create_thread_20ms() {
    pthread_mutex_init(&mut_20ms_buf, NULL);
    if(pthread_create(&thread_20ms_id, NULL, thread_20ms, NULL) != 0) {
        db_error("Create thread_20ms fail !\n");
    }	
}*/

/*
 * weber+170203, ?P?_?O?_???srecovery(?ɦW?O?_?@?P)
 * return:
 *         0 = no, 1 = yes
 */
int updateRecovery(char *zipName) {
	char fileStr[128] = "/cache/recovery.log\0";
	char readStr[128];
	struct stat sti;
	FILE *fp;

	db_debug("updateRecovery() 00 zipName=%s\n", zipName);
    if (-1 == stat (fileStr, &sti)) {
    	db_error("updateRecovery() 01\n");
        return 1;
    }
    else {
    	fp = fopen(fileStr, "rt");
    	if(fp != NULL) {
    		//while(!feof(fp) ) {
    			memset(&readStr[0], 0, sizeof(readStr) );
    			fgets(readStr, sizeof(readStr), fp);
    			db_debug("updateRecovery() 02 readStr=%s\n", readStr);
                if(strcmp(readStr, zipName) == 0) {
                	db_debug("updateRecovery() 03-1\n");
                	return 0;
                }
                else {
                	db_debug("updateRecovery() 03-2\n");
                	return 1;
                }
    		//}
    		fclose(fp);
    	}
    }
}
/*
 * weber+170203, ?P?_?O?_????recovery
 * return:
 *         ???Y?ɦW?? , null = no
 */
int checkDoRecovery(char *path, char *zipName) {
	DIR* pDir = NULL;
	struct dirent* pEntry = NULL;
	time_t t;
	int ret = 0;
	char empty_ver[16] = "            \0";

	pDir = opendir(path);
	if(pDir != NULL) {
		while( (pEntry = readdir(pDir) ) != NULL) {
			if(strstr(pEntry->d_name, ".zip") != 0 && strstr(pEntry->d_name, "Update") != 0) {
				sprintf(zipName, "%s\0", pEntry->d_name);
				ret = updateRecovery(zipName);
		    	if(ret == 1) {
		    		setWifiDisableTime(-2);
//tmp		            setMCULedSetting(MCU_SLEEP_TIMER_START, 0);
//tmp		            setMCULedSetting(MCU_WATCHDOG_FLAG, 1);
		            setSendMcuA64Version(&mSSID[0], &mPwd[0], &empty_ver[0]);
		        	t = time(NULL);
		            setMcuDate(t);

		    	}
				db_debug("checkDoRecovery() zipName=%s\n", zipName);
				break;
			}
		}
	}
	closedir(pDir);
	return ret;
}

int do_reboot_recovery_jni() {
	int ret;
	//do_reboot_recovery
	return ret;
}
void do_Test_Mode_Func_jni(int m_cmd, int s_cmd) {
	//do_Test_Mode_Func
	return;
}
void check_wifi_connected_jni() {
	//check_wifi_connected
	return;
}
void set_webserver_getdata_timeout_jni() {
	//set_webserver_getdata_timeout
	return;
}
void changeWifiMode_jni(int mode) {
	//changeWifiMode
	return;
}
void set_ipAddrFromAp_jni(char *addr) {
	//set_ipAddrFromAp
	return;
}
void stopWifiAP_jni() {
	//stopWifiAP
	return;
}
/*
 * ?Ȯɪ????I?sJava Code
 */
/*void FPGA_Ctrl_Power_Func_jni(int ctrl_pow, int flag) {
	//FPGA_Ctrl_Power_Func
	return;
}*/
void set_eth_connect_timeout_jni() {
	//set_eth_connect_timeout
	return;
}
/*
 * ?Ȯɪ????I?sJava Code
 */
//void FanCtrlFunc_jni() {
//	//FanCtrlFunc
//	return;
//}
void checkWifiAPLive_jni() {
	//checkWifiAPLive
	return;
}
void check_rtmp_state_jni() {
	//check_rtmp_state
	return;
}
void check_wifi_state_jni() {
	//check_wifi_state
	return;
}
void set_wifi_ctemp_tint_jni(int k, int t) {
	//set_wifi_ctemp_tint
	return;
}
void set_wifi_compass_value_jni(int val) {
	//set_wifi_compass_value
	return;
}
void set_filetool_compass_jni(int *data) {
	//set_filetool_compass
	return;
}
void set_filetool_sensor_jni() {
	//set_filetool_sensor
	return;
}
int GetDiskInfo_jni() {
	int ret;
	//GetDiskInfo
	return ret;
}
void SetTextFpgaCheckSum_jni() {
	//SetTextFpgaCheckSum
	return;
}
/*int FPGA_Download_jni() {
	//FPGA_Download
	return ret;
}*/
void Check_Bottom_File_jni(int mode, int tx_mode, int isInit) {
	//Check_Bottom_File
	return;
}

void SetPowerModeValue(int mode) {
	PowerMode = mode;
}
int GetPowerModeValue(void) {
	return PowerMode;
}

void SetLockRecordEnJNI(int lock) {
	lockRecordEnJNI = lock;
}

void SetAudioTestRecording(int en) {
	audioTestRecording = en;
}
int GetAudioTestRecording(void) {
	return audioTestRecording;
}

void SetAudioTestCount(int cnt) {
	audioTestCount = cnt;
}

void SetMenuFlag(int flag) {
	mMenuFlag = flag;
}
int GetMenuFlag(void) {
	return mMenuFlag;
}

void SetIsStandby(int en) {
	isStandby = en;
}
int GetIsStandby(void) {
	return isStandby;
}

int GetRecTime(void) {
	return recTime;
}

void SetSendSTFlag(int flag) {
	Send_ST_Flag = flag;
}
int GetSendSTFlag(void) {
	return Send_ST_Flag;
}

void SetUVCErrCount(int cnt) {
	UVCErrCount = cnt;
}
int GetUVCErrCount(void) {
	return UVCErrCount;
}

void SetDownloadLevel(int level) {
	downloadLevel = level;
}
int GetDownloadLevel(void) {
	return downloadLevel;
}

void SetBmm050Start(int en) {
	mBmm050Start = en;
}
int GetBmm050Start(void) {
	return mBmm050Start;
}

void SetCheckStCmdDdrFlag(int flag) {
	check_st_cmd_ddr_flag = flag;
}
int GetCheckStCmdDdrFlag(void) {
	return check_st_cmd_ddr_flag;
}

void SetWriteUS360DataBinFlag(int flag) {
	writeUS360DataBin_flag = flag;
}
//int GetWriteUS360DataBinFlag(void) {
//	return writeUS360DataBin_flag;
//}

void SetADCRatio(int value) {
	adc_ratio = value;
}
int GetADCRatio(void) {
	return adc_ratio;
}

void SetAdjSensorSyncFlag(int flag) {
	Adj_Sensor_Sync_Flag = flag;
}
int GetAdjSensorSyncFlag(void) {
	return Adj_Sensor_Sync_Flag;
}

/*int Defect_State  = 0; 		//-1:err 	0:none	1:ok
void SetDefectState(int state) {
	Defect_State = state;
}
int GetDefectState(void) {
	return Defect_State;
}*/

void SetCmdIdx(int idx) {
	Cmd_Idx = idx;
}
int GetCmdIdx(void) {
	return Cmd_Idx;
}

void SetCmdP1(int idx) {
	Cmd_P1 = idx;
}
int GetCmdP1(void) {
	return Cmd_P1;
}

void SetChooseModeFlag(int flag) {
	choose_mode_flag = flag;
}
int GetChooseModeFlag(void) {
	return choose_mode_flag;
}

void AletaS2Init(void)
{
   	//Sensor
    RXDelaySet(10, 0, 0x1);
    RXDelaySet(14, 0, 0x2);
    RXDelaySet(14, 0, 0x4);
    RXDelaySet(15, 0, 0x8);
    RXDelaySet(12, 1, 0xF);

    Adj_Sensor_Sync_Flag = 1;
    AS2MainCMDInit();
    SetPipeReStart();

    Send_ST_Flag = 0;
    setPowerSavingInit(0);
    SetDefectStep(0);
    SetDefectState(0);
   	setLedBrightness(Get_DataBin_LedBrightness());
   	setOledControl(Get_DataBin_OledControl());
   	SetPowerSavingInitT1();
}

int FPGA_Download(void)
{
	if(DownloadProc() < 0)
		return -1;
	setDnaCheck(dnaCheck());
    return 0;
}

int FPGA_Init()
{
    int ret=0;
    ret = FPGA_Download();
    SetTextFpgaCheckSum_jni();
    AletaS2Init();
    Cmd_Idx = -1; Cmd_P1 = -1;
    return ret;
}

void ReStart_Init_Func(int ctrl_pow)
{
	Load_Parameter_Tmp_Proc();
    FPGAdriverInit();
    Check_Bottom_File_jni(getBottomMode(), mBottomTextMode, 1);
    Cmd_P1 = 0;
    setPlayModeTmp(getPlayMode());
    choose_mode_flag = 1;
	
	int id   = Get_Camera_Id();
	int base = Get_Camera_Base();
    prepareCamera(id, base);              // openUVC, create uvc_thread & rec_thread
}

void FPGA_Ctrl_Power_Func(int ctrl_pow, int flag)
{
    int ret;
    db_debug("FPGA_Ctrl_Power_Func: ctrl=%d flag=%d\n", ctrl_pow, flag);
    if(ctrl_pow == 0) {        //open
        ret = FPGA_Init();
        if(ret == 0){
            setFPGACtrlPower(ctrl_pow);     // FPGA Power ON
            ReStart_Init_Func(ctrl_pow);
            //FPGA_Ctrl_Power = ctrl_pow;
        }
        else {
         	fpgaPowerOff();
         	setFPGACtrlPower(1);
          	//FPGA_Ctrl_Power = 1;
        }
    }
    else {                    //close
        setFPGACtrlPower(ctrl_pow);
        //FPGA_Ctrl_Power = ctrl_pow;
    }
}

/*
 * weber+170203, ????recovery command
 *         zipName : ???Y?ɦW??(?ݥ]?t.zip)
 * return:
 *         0 = success , -1 = fail
 */
int runRecoveryCmd(char *zipName) {
	char fileStr[64]  = "/cache/recovery.log\0";
	char fileStr2[64] = "/cache/recovery/command\0";
	char str_tmp[64];
	struct stat sti;
	FILE *fp;

	//?g?J?ɦW
	fp = fopen(fileStr, "wt");
	if(fp != NULL) {
		sprintf(str_tmp, "%s\r\n\0", zipName);
		fwrite(str_tmp, strlen(str_tmp), 1, fp);
		fclose(fp);
	}

	fp = fopen(fileStr2, "wt");
	if(fp != NULL) {
		sprintf(str_tmp, "boot-recovery\r\n\0");
		fwrite(str_tmp, strlen(str_tmp), 1, fp);
		sprintf(str_tmp, "--update_package=/extsd/%s\r\n\0", zipName);
		fwrite(str_tmp, strlen(str_tmp), 1, fp);
		sprintf(str_tmp, "reboot\r\n\0");
		fwrite(str_tmp, strlen(str_tmp), 1, fp);
		fclose(fp);
    }

	if(do_reboot_recovery_jni() == -1)		//if(do_reboot_recovery() == -1)
    	return -1;

	return 0;
}

void SetIsRecording(int en) {
	isRecording = en;
}
int GetIsRecording(void) {
	return isRecording;
}

int GetPower(void) {
	return power;
}

int check_power(void)
{
	if(dcState == 1)
		return 1;
	else {
		if(power > 6)		//?q?q?j?? 6%
			return 1;
		else
			return 0;
	}
}

void do_power_standby() {
	db_debug("do_power_standby %d\n", PowerMode);
    if(PowerMode == 3){
        PowerMode = 1;                    // power mem

        stopREC(9);
        get_current_usec(&sleepTime);
//tmp        setLEDLightOn(0,1);
        isStandby = 1;
//tmp        paintOLEDStandby(1);
        SetAxp81xChg(1);
		int speed = 0;
        SetFanLstLv(speed);
		SetFanSpeed(speed);
        setFanRotateSpeed(speed);

        FPGA_Ctrl_Power_Func(1, 3);
//tmp        setOledStandByMode(1);
        if(mMenuFlag == 1){
          	SetMenuFlag(0);		//mMenuFlag = 0;
//tmp            closeOLEDMenu();
        }
        stopWifiAP_jni();
        setPowerMode(PowerMode);

        if(get_live360_state() == 0)
        	set_live360_state(-1);

//        systemlog.addLog("info", System.currentTimeMillis(), "machine", "system is standby.", "---");
    }
}


void SetCapRecStartTime(unsigned long long time, int debug) {
	unsigned long long nowTime;
	if(time == 0) nowTime = 0;
	else		  get_current_usec(&nowTime);
	setPowerSavingCapRecStartTime(nowTime);
}

void SetCapRecFinishTime(unsigned long long time, unsigned long long overtime, int debug) {
	unsigned long long nowTime;
	if(time == 0) nowTime = 0;
	else		  get_current_usec(&nowTime);
	setPowerSavingCapRecFinishTime(nowTime, overtime);
}

void Set_Power_Saving_Wifi_Cmd(unsigned long long time, unsigned long long overtime, int debug) {
	if(Get_DataBin_PowerSaving() == 1) {
		if(GetFPGASleepEn() == 1)
			SetFPGASleepEn(0);
		setPowerSavingCapRecFinishTime(time, overtime);
	}
db_debug("Set_Power_Saving_Wifi_Cmd() PWS: debug=%d\n", debug);
}
void SetPowerSavingWifiCmd(unsigned long long time, unsigned long long overtime, int debug) {
	unsigned long long nowTime;
	if(time == 0) nowTime = 0;
	else		  get_current_usec(&nowTime);
	Set_Power_Saving_Wifi_Cmd(nowTime, overtime, debug);
}

/*
 * weber+17016, SDcard???A
 * return : 0 = ?????d, 1 = ???`?i??, 2 = ??, 3 = ???䴩?A??format
 */
int CheckSDcardState(char *path) {
	unsigned long long size;
//db_debug("CheckSDcardState() path=%s\n", path);
    if(strcmp(path, "/mnt/extsd") == 0) {		// ?S??SD?d / SD?d?榡???ۮe
      	if(GetDiskInfo_jni() == 1)				// SD Card Err
       		return 3;
       	else								// SD Card No
       		return 0;
    }
   	else{
		calSdFreeSize(&size);
   		if(size < 0xA00000)		// SD Card Full
            return 2;
        else								// SD Card Ok
            return 1;
    }
}

void checkPowerLog(int vol, int cur, int temp, int fan) {
	char rangStr[16];
	static int batteryLogCnt = 0;
   	if(power / 10 != powerRang && power != 0){
   		powerRang = power / 10;
   		sprintf(rangStr, "%d\%", power);
   		db_debug("Battery :%s\n", rangStr);
//   		systemlog.addLog("info", System.currentTimeMillis(), "machine", "Battery", rangStr);
   	}
   	if(batteryLogCnt >= 15){
   		batteryLogCnt = 0;
   		int ftmp =  fan * 42;
   		int pwr = power * 100;	//?ק????????K?[??
   		int tmp = temp * 100;			//?ק????????K?[??
   		int spd = GetFanSpeed() * 100;
//   		systemlog.addBattery(System.currentTimeMillis(), pwr+"", vol+"", cur+"", tmp+"", spd+"", ftmp+"");
   	}else{
   		batteryLogCnt++;
   	}
}

/*
 * ?̷Ӥ????O?_?????o?e?P?BCMD, ?ӧP?_?O?_?_?u
 */
int Check_Wifi_Connect_isAlive(void) {
	unsigned long long nowTime;
	get_current_usec(&nowTime);
  	if(Send_Data_State_t == 0)
   		return 0;

   	if(nowTime < Send_Data_State_t) nowTime = Send_Data_State_t;
   	if( (nowTime - Send_Data_State_t) > POWER_SAVING_CMD_OVERTIME_15S) {
		Send_Data_State_t = 0;
		return 0;
	}
	return 1;
}

void SetTimerLidarCycle(int time) {
	timerLidarCycle = time;
}
int GetTimerLidarCycle(void) {
	return timerLidarCycle;
}

void SetWifiReboot(int val) {
	wifiReboot = val;
}
int GetWifiReboot(void) {
	return wifiReboot;
}

void rwWifiData_jni(int rw,int wifiMode) {
	//rwWifiData
	return;
}

void timerLidar_jni() {
	//timerLidar
	return;
}

int checkTypesOfCaptureMode() {
    int c_mode = getCameraMode();
    if(c_mode == CAMERA_MODE_CAP || c_mode == CAMERA_MODE_AEB || c_mode == CAMERA_MODE_RAW || 
       c_mode == CAMERA_MODE_HDR || c_mode == CAMERA_MODE_NIGHT || c_mode == CAMERA_MODE_NIGHT_HDR ||
	   c_mode == CAMERA_MODE_SPORT || c_mode == CAMERA_MODE_SPORT_WDR || c_mode == CAMERA_MODE_M_MODE || 
       c_mode == CAMERA_MODE_REMOVAL || c_mode == CAMERA_MODE_3D_MODEL) {		//???ӼҦ?
        return 1;
	}
    else
        return 0;
}

void *thread_1s(void)
{
	int state = 0;
	int do_recovery = 0;
    int wifi_disable_time;
    int camera_position_mode;
    int sd_state;
	static int sd_state_lst = -1;
	static int keepSleep = 0;
	static int fan_ctrl_cnt = 0;
	static int kelvin_cnt = 2;
	static int cp_cnt = 0;
    static int camera_position_mode_lst = CAMERA_POSITION_0;
	char zip_name[128], sd_path[128];
	static unsigned long long curTime, lstTime=0, runTime, nowTime, live360Time;
    //nice(-6);    // ?վ?thread?u???v

    while(thread_1s_en)
    {
		get_current_usec(&curTime);
        if(curTime < lstTime) lstTime = curTime;
        else if((curTime - lstTime) >= 1000000){
            db_debug("thread_1s: runTime=%d\n", (int)runTime);
            lstTime = curTime;
        }

		if(CheckFXCnt() == -1 && GetMcuUpdateFlag() == 0) {
//    		systemlog.addLog("info", System.currentTimeMillis(), "machine", "FX Cnt Err", "---");
//tmp    	paintOLEDFxError(1);
			usleep(1000000);
//tmp    	setMCULedSetting(MCU_IS_A64_CLOSE, 1);
		}

		if(get_live360_state() == 0) {
			get_current_usec(&nowTime);
			get_live360_t1(&live360Time);
			if((nowTime - live360Time) > 60000000) {		// > 60s, ?P?_???_?u
				set_live360_state(-1);
			}
		}
		if(timerLidarCycle == 1000) timerLidar_jni();				// rex+ 201021, 1s

//    	getSaveEn();
//    	mHandler.obtainMessage(SYS_CYCLE_SHOW_NOW).sendToTarget();			// weber+170626

		Write_File_Error = get_write_file_error();
        getSdPath(&sd_path[0], sizeof(sd_path));
		setSdState(CheckSDcardState(&sd_path[0]));
        sd_state = getSdState();
		if(sd_state_lst != sd_state){
			if(sd_state_lst != -1){
				isNeedNewFreeCount = 1;
			}
			db_debug("thread1s() sd_state = %d\n", sd_state);
			sd_state_lst = sd_state;

			//setSDPathStr(sd_path.getBytes(), sd_path.getBytes().length);

			Write_File_Error = 0;

			if(sd_state == 1){
				//if(zip == null){
					do_recovery = checkDoRecovery(&sd_path[0], &zip_name[0]);
				//}
			}
		}
		if(sd_state == 1){
			do_Test_Mode_Func_jni(TestToolCmd.MainCmd, TestToolCmd.SubCmd);
			if(do_recovery == 1) {
				int mData[20];
//tmp           getMCUSendData(&mData[0]);
				db_debug("thread1s() watchdogState:%d\n", mData[8]);
				if(mData[8] == 1){
					db_debug("thread1s() runRecoveryCmd: %s\n", zip_name);
					runRecoveryCmd(&zip_name[0]);
				}
			}
		}

		calSdFreeSize(&sd_freesize);
		getSDAllSize(&sd_allsize);
//tmp   if(GetSpacePhotoNum() <= 3 && sd_state == 1){
//tmp   	isNeedNewFreeCount = 1;
//tmp   }
		if(isNeedNewFreeCount == 1 && getImgReadyFlag() == 1){
			isNeedNewFreeCount = 0;
			if(sd_state == 1){
//tmp    		getRECTimes(sd_freesize);
			}else{
//tmp    		getRECTimes(0L);
			}
//tmp    	int cnt = GetSpacePhotoNum();
			int cnt = 99;
			Set_DataBin_FreeCount(cnt);
			writeUS360DataBin_flag = 1;
		}

		if(getRecordEn() == 1 && getRecordCmdFlag() == 0){        // SD card full,????rec?ʧ@
			if(get_rec_state() == -2 && lockRecordEnJNI == 0){
				stopREC(10);
//tmp            	systemlog.addLog("info", System.currentTimeMillis(), "machine", "doRecordStop", "---");
//tmp            	if(getDebugLogSaveToSDCard() == 1){
//tmp        			systemlog.writeDebugLogFile();
//tmp            	}
			}
		}

		if(getRecordEn() == 1){
//tmp       recTime = getOLEDRecTime();
			set_timeout_start(0);                     // ???v???A???i?Jstandby

			if(check_power() == 0) {					  // ?q?q  <= 6% ???????v
//tmp        		systemlog.addLog("info", System.currentTimeMillis(), "machine", "doRecordStop", "LowPower RecordStop");
				stopREC(11);
			}
		}
		else{
			recTime = 0;
		}

		check_wifi_connected_jni();

		if(getHdmiConnected() == 1){
			set_timeout_start(0);                    // HDMI???W?A???i?Jstandby
			if(PowerMode == 3) setHdmiState(1);
			else 			   setHdmiState(0);
		}
		else setHdmiState(0);

		set_webserver_getdata_timeout_jni();

		if(getHdmiState() == 1)
			do_Test_Mode_Func_jni(TestToolCmd.MainCmd, TestToolCmd.SubCmd);

		do_Test_Mode_Func_jni(TestToolCmd.MainCmd, TestToolCmd.SubCmd);

		if(getWifiModeCmd() == 1){              // change: WifiServerThread.java
			db_debug("thread1s() mWifiModeCmd == 1\n");
			setWifiModeCmd(0);
			setWifiModeState(0);
			rwWifiData_jni(1, 0);
			set_ipAddrFromAp_jni("");		//ipAddrFromAp = "";
			changeWifiMode_jni(0);
		}else if(getWifiModeCmd() == 2){
			db_debug("thread1s() mWifiModeCmd == 2\n");
			setWifiModeCmd(0);
			rwWifiData_jni(1, wifiReboot);
			changeWifiMode_jni(1);
		}

		if(audioTestRecording == 1){
			audioTestCount++;
			if(audioTestCount >= 3){
				audioTestCount = 0;
				audioTestRecording = 0;
			}
		}

		if(mMenuFlag == 1){                            // ?i?Jmenu?A???i?Jstandby
			set_timeout_start(0);
		}

		if(PowerMode == 3){
            wifi_disable_time = getWifiDisableTime();
			if(wifi_disable_time > 0){
//tmp           if(getMcuVersion() > 12){
//tmp               setMCULedSetting(MCU_SET_SLEEP_TIMER, wifi_disable_time);
//tmp           }
//tmp           else if(check_timeout_start(0, wifi_disable_time*1000) == 1){
				if(check_timeout_start(0, wifi_disable_time*1000) == 1){
					do_power_standby();
//tmp               disableShutdown(0);
				}
			}
			else if(wifi_disable_time == -2) {
//tmp           if(getMcuVersion() > 12){
//tmp               setMCULedSetting(MCU_SET_SLEEP_TIMER, 0);
//tmp               setMCULedSetting(MCU_SLEEP_TIMER_START, 0);
//tmp           }
//tmp           disableShutdown(1);
			}
//tmp       setLEDLightOn(0,1);
			keepSleep = 0;
		}else{
//tmp       setLEDLightOn(0,1);
			keepSleep++;
			if(keepSleep > 10){
				keepSleep = 0;
				setPowerMode(PowerMode);
			}
			db_debug("a64 should sleep\n");
		}

		set_eth_connect_timeout_jni();

//    	captureDCnt = readCaptureDCnt();
		if(isStandby == 0){
			if(fan_ctrl_cnt >= 2){
				FanCtrlFunc();
				fan_ctrl_cnt = 1;
			}
			else if(fan_ctrl_cnt < 1) fan_ctrl_cnt = 1;
			else                      fan_ctrl_cnt++;
		}

		get_current_usec(&nowTime);
		if(GetTask1mLockFlag() == 1 && CheckTask1mLockTime(nowTime) == 1) {		//java: Timer1mLockTask()
			SetTask1mLockFlag(0);
			SetFanAlwaysOn(0);
		}

		checkWifiAPLive_jni();

		if(isStandby == 0){
			if(FPGA_Ctrl_Power == 0){
				if(uvc_err_flag < 0){           // MainCmdStart
					if(Send_ST_Flag == 3) UVCErrCount ++;
					else                  UVCErrCount = 0;
					db_error("00 UVCErrCount=%d ST_Flag=%d\n", UVCErrCount, Send_ST_Flag);
				}
				else UVCErrCount = 0;

				if(UVCErrCount >= 2 || (GetFPGASleepEn() == 1 && UVCErrCount != 0) ){
					db_error("01 UVCErrCount=%d FPGASleepEn=%d\n", UVCErrCount, GetFPGASleepEn() );
					FPGA_Ctrl_Power_Func(1, 1);		// Power OFF
					UVCErrCount = 0;
				}
			}
			else if(FPGA_Ctrl_Power == 1 && downloadLevel != 2){
				FPGA_Ctrl_Power_Func(0, 2);			// Power ON
				UVCErrCount = 0;
			}
		}

		check_rtmp_state_jni();

		check_wifi_state_jni();

		if(kelvin_cnt > 0){
			kelvin_cnt = 0;
			int k = 2000;
			int t = 0;

			k = GetColorTemperature();
			t = GetTint();
			set_wifi_ctemp_tint_jni(k, t);
		}else{
			kelvin_cnt++;
		}

#ifdef __CLOSE_CODE__	//tmp
		if(mBmm050Start > 0){
			if(getBmm050RegulateResult() == -1){
				db_debug("compass regulate failure\n");
				mBmm050Start = 0;
				set_wifi_compass_value_jni(-1);
			}else if(getBmm050RegulateResult() == 1){
				int data[6];
				getBmm050Data(&data[0]);
				Set_DataBin_CompassMaxx(data[0]);
				Set_DataBin_CompassMaxy(data[1]);
				Set_DataBin_CompassMaxz(data[2]);
				Set_DataBin_CompassMinx(data[3]);
				Set_DataBin_CompassMiny(data[4]);
				Set_DataBin_CompassMinz(data[5]);
				writeUS360DataBin_flag = 1;
				mBmm050Start = 0;

				set_filetool_compass_jni(&data[0]);
			}
		}
#endif	//__CLOSE_CODE__	

		if(getCtrlCameraPositionMode() == 1){			//Auto
			float sensorData2[3];
//tmp    	getBma2x2_orientation_data(&sensorData2[0], 1);
			if(mCameraPositionMode == 0) {			//???m
				if(sensorData2[2] > 135 || sensorData2[2] < -135)
					cp_cnt++;
				else
					cp_cnt = 0;
			}
			else {									//?˸m
				if(sensorData2[2] < 45 && sensorData2[2] > -45)
					cp_cnt++;
				else
					cp_cnt = 0;
			}

			if(cp_cnt > 4) {
				cp_cnt = 0;
				if(mCameraPositionMode == 0)mCameraPositionMode = 1;
				else					    mCameraPositionMode = 0;
			}
            
            camera_position_mode = getCameraPositionMode();
			if(camera_position_mode != camera_position_mode_lst) {
				get_current_usec(&nowTime);
				setCameraPositionModeChange(1);
				camera_position_mode_lst = camera_position_mode;
				Set_Power_Saving_Wifi_Cmd(nowTime, POWER_SAVING_CMD_OVERTIME_15S, 35);
				do_Test_Mode_Func_jni(TestToolCmd.MainCmd, TestToolCmd.SubCmd);
			}
		}

		set_filetool_sensor_jni();

		setWifiConnectIsAlive(Check_Wifi_Connect_isAlive());
		if(getWifiConnectIsAlive() == 0) {		// disconnect
			//wifi audio rate init
		}

		if(getPowerSavingInit() == 0) {		//?}??/???v?Ұ? ?ٹq?Ҧ?Init
			if(Get_DataBin_PowerSaving() == 1) {		//On
				get_current_usec(&nowTime);
                setPowerSavingInitTime2(nowTime);
				if(getImgReadyFlag() == 1 && get_Init_Gamma_Table_En() == 0 &&
					(getPowerSavingInitTime2() - getPowerSavingInitTime1()) > POWER_SAVING_INIT_OVERTIME) {		//?}??/???v?Ұ? ?????e????ok, ?~?i?J?ٹq???A
					setPowerSavingCapRecFinishTime(nowTime, 0);
					setPowerSavingInit(1);
				}
			}
			else								//Off
				setPowerSavingInit(1);
		}

		if(Get_DataBin_PowerSaving() == 1 && GetFPGASleepEn() == 0) {	//?ٹq?Ҧ?, ????FPGA??  (????/???v????, ?]?w?Ѽ?)
			get_current_usec(&nowTime);
			if(getPowerSavingSetingUiState() == 1 && getPowerSavingCapRecFinishTime() == 0) {		//Wifi?Ӥ[?S???n?D?P?B????, ???P?_?u, ?h?i?J?ٹq???A, ?w??UI?S?????^????????????APP
				if(getWifiConnectIsAlive() == 0) {
					setPowerSavingCapRecFinishTime(nowTime, 0);
					setPowerSavingSetingUiState(0);
				}
			}

			if(getPowerSavingCapRecStartTime() != 0 || getPowerSavingCapRecFinishTime() != 0) {
				if(checkTypesOfCaptureMode()) {		//???ӼҦ?
					if(getPowerSavingCapRecStartTime() != 0 && getPowerSavingCapRecFinishTime() == 0 && CheckSaveJpegCnt() == 0) {		//???ӵ???
						setPowerSavingCapRecFinishTime(nowTime, POWER_SAVING_CMD_OVERTIME_5S);
					}
				}

				if(nowTime < getPowerSavingCapRecFinishTime()) setPowerSavingCapRecFinishTime(nowTime, POWER_SAVING_CMD_OVERTIME_5S);
//tmp        	if(getPowerSavingCapRecFinishTime() != 0 && Send_ST_Flag == 3 && getCameraPositionModeChange() == 0 && GetDecBottomStep() == 0 &&
//tmp        			getHdmiState() == 0 && check_st_cmd_ddr_flag == 1 && (nowTime - getPowerSavingCapRecFinishTime()) > getPowerSavingOvertime()) {
				if(getPowerSavingCapRecFinishTime() != 0 && Send_ST_Flag == 3 && getCameraPositionModeChange() == 0 /*&& GetDecBottomStep() == 0*/ &&
						getHdmiState() == 0 && check_st_cmd_ddr_flag == 1 && (nowTime - getPowerSavingCapRecFinishTime()) > getPowerSavingOvertime()) {
					setPowerSavingCapRecStartTime(0);
					setPowerSavingCapRecFinishTime(0, POWER_SAVING_CMD_OVERTIME_5S);
					SetFPGASleepEn(1);
					usleep(500000);
				}
			}
		}

		pollWatchDog();
	
	    get_current_usec(&runTime);
        runTime -= curTime;
        if(runTime < 1000000){
            usleep(1000000 - runTime);
            get_current_usec(&runTime);
            runTime -= curTime;
        }
	}	//while(1)
}

void create_thread_1s() {
    pthread_mutex_init(&mut_1s_buf, NULL);
    if(pthread_create(&thread_1s_id, NULL, thread_1s, NULL) != 0) {
        db_error("Create thread_1s fail !\n");
    }	
}


/*void wifiSerThe_proc_jni() {
	//wifiSerThe_proc
}*/
void System_Exit_jni() {
	//System_Exit
	return;
}
int Make_Defect_3x3_Table_jni(int type) {
	int ret;
	//Make_Defect_3x3_Table
	return ret;
}
void audio_record_thread_release_jni() {
	//audio_record_thread_release
	return;
}
void audio_record_thread_start_jni() {
	//audio_record_thread_start
	return;
}

void SetWifiConnected(int en) {
	WifiConnected = en;
}

void deleteSkipWatchDogFile(void) {
	char fileStr[64] = "/mnt/sdcard/US360/skipWatchDog.bin\0";
	struct stat sti;
    if(stat(fileStr, &sti) == 0) {
        remove(fileStr);
    }
}

void createSkipWatchDogFile(){
	char dirStr[64]  = "/mnt/sdcard/US360\0";
	char fileStr[64] = "/mnt/sdcard/US360/skipWatchDog.bin\0";
	struct stat sti;
	FILE *fp;
	int value = 1;

	if(stat(dirStr, &sti) != 0) {
        if(mkdir(dirStr, S_IRWXU) != 0)
            db_error("createSkipWatchDogFile() create %s folder fail\n", dirStr);
	}

	if(stat(fileStr, &sti) != 0) {
		fp = fopen(fileStr, "wb");
		fwrite(&value, sizeof(value), 1, fp);
		fclose(fp);
	}
}

int ResolutionModeToKPixel(int res_mode)
{
    int kpixel = 1000;
    switch(res_mode){
    case 1:  kpixel = 12000; break;		//12K
    case 2:  kpixel =  4000; break;		//4K
    case 7:  kpixel =  8000; break;		//8K
    case 8:  kpixel = 10000; break;		//10K
    case 12: kpixel =  6000; break;		//6K
    case 13: kpixel =  3000; break;		//3K
    case 14: kpixel =  2000; break;		//2K
    default: kpixel = 99000; break;
    }
    return kpixel;
}

void ModeTypeSelectS2(int play_mode, int resolution, int hdmi_state, int camera_mdoe)
{
    unsigned long long now_time;
    
    if(get_rec_state() != -2) {
     	setRecordEn(0);
//tmp        systemlog.addLog("info", System.currentTimeMillis(), "machine", "doRecordStop", "---");
//tmp        if(getDebugLogSaveToSDCard() == 1){
//tmp            systemlog.writeDebugLogFile();
//tmp        }
        doRecordVideo(0, getPlayMode(), getResolutionMode(), getTimeLapseMode(), hdmi_state);
    }

    //?j???]??Global?Ҧ?
    if(play_mode != PLAY_MODE_GLOBAL) {
        setPlayMode(PLAY_MODE_GLOBAL);
        setPlayModeTmp(PLAY_MODE_GLOBAL);
    }

    //?̷?CameraMode?????ѪR??
    if(camera_mdoe == CAMERA_MODE_CAP || camera_mdoe == CAMERA_MODE_HDR || 
       camera_mdoe == CAMERA_MODE_SPORT || camera_mdoe == CAMERA_MODE_SPORT_WDR) {
    	switch(resolution) {
        case RESOLUTION_MODE_12K:
        case RESOLUTION_MODE_8K:
        case RESOLUTION_MODE_6K:
          	setResolutionMode(resolution);
            break;
        default:
         	setResolutionMode(RESOLUTION_MODE_12K);
            break;
        }
        setFPS(100);
    }
    else if(camera_mdoe == CAMERA_MODE_NIGHT || camera_mdoe == CAMERA_MODE_NIGHT_HDR || 
            camera_mdoe == CAMERA_MODE_M_MODE) {
    	switch(resolution) {
        case RESOLUTION_MODE_12K:
        case RESOLUTION_MODE_8K:
        case RESOLUTION_MODE_6K:
          	setResolutionMode(resolution);
            break;
        default:
         	setResolutionMode(RESOLUTION_MODE_12K);
            break;
        }
        setFPS(50);
    }
    else if(camera_mdoe == CAMERA_MODE_AEB || camera_mdoe == CAMERA_MODE_RAW) {
      	setResolutionMode(RESOLUTION_MODE_12K);
        setFPS(100);
    }
    else if(camera_mdoe == CAMERA_MODE_REC || camera_mdoe == CAMERA_MODE_REC_WDR) {
        switch(resolution) {
        case RESOLUTION_MODE_4K:
        case RESOLUTION_MODE_3K:
        case RESOLUTION_MODE_2K:
          	setResolutionMode(resolution);
            if(resolution == RESOLUTION_MODE_4K) {
                setFPS(100);
            }
            else if(resolution == RESOLUTION_MODE_3K) {
                if(getAegEpFreq() == 60) setFPS(240);
                else                     setFPS(200);   
            }
            else if(resolution == RESOLUTION_MODE_2K) {
                if(getAegEpFreq() == 60) setFPS(300);
                else                     setFPS(250);   
            }
            break;
        default:
          	setResolutionMode(RESOLUTION_MODE_4K);
            setFPS(100);
            break;
        }
    }
    else if(camera_mdoe == CAMERA_MODE_TIMELAPSE || camera_mdoe == CAMERA_MODE_TIMELAPSE_WDR) {
        switch(resolution) {
        case RESOLUTION_MODE_12K:
        case RESOLUTION_MODE_4K:
        case RESOLUTION_MODE_8K:
        case RESOLUTION_MODE_6K:
           	setResolutionMode(resolution);
            break;
        default:
          	setResolutionMode(RESOLUTION_MODE_6K);
            break;
        }
        setFPS(100);
    }
    else if(camera_mdoe == CAMERA_MODE_REMOVAL) {
    	setResolutionMode(RESOLUTION_MODE_12K);
        setFPS(100);
    }
    else if(camera_mdoe == CAMERA_MODE_3D_MODEL) {
    	setResolutionMode(RESOLUTION_MODE_12K);
        setFPS(100);
    }

    setResolutionWidthHeight(getResolutionMode());
    get_current_usec(&now_time);
    setChooseModeTime(now_time);
}

void choose_mode(int c_mode, int mode, int res_mode, int fps)
{
	int i;
	int mode_tmp, res_tmp;
    int kpixel = ResolutionModeToKPixel(res_mode);
//tmp    setOLEDMainModeResolu(mode, kpixel);

    //DebugJPEGMode = 0;
    //ISP2_Sensor = 0;
    setJPEGaddr(0, 0, 0);
    setPlayMode(mode);
    ShowFocusVisibilityMode = 0;

    db_debug("choose_mode() res_mode=%d fps=%d\n", res_mode, fps);
    setStitchingOut(c_mode, 0, res_mode, fps);
    MakeH264DataHeaderProc();
    Adj_Sensor_Sync_Flag = 3;
    get_Stitching_Out(&mode_tmp, &res_tmp);
    for(i = 0; i < 8; i++)
        writeCmdTable(i, res_tmp, fps, 0, 0, 0);
    //PlayMode = 0;

    if(c_mode == CAMERA_MODE_M_MODE && GetDefectState() == 1)
    	set_A2K_ISP2_Defect_En(1);
    else
    	set_A2K_ISP2_Defect_En(0);
}

//int DebugJPEGMode = 0, ISP2_Sensor = 0;
/*
 * sensor:
 * 0~4: ????4K???@ Sensor
 * -1:
 * -2:	????4K?C?? Sensor ???????_?X??
 * -3:
 * -4:	????12K ?E?c??, ??Focus??
 * -5:
 * -6:	????12K 4????, ??Focus??
 * -8:	????FX DDR???}
 * -9:
 * -99:	????F2 DDR???}
 */
void Set_ISP2_Addr(int mdoe, int addr, int sensor)
{
//    DebugJPEGMode = mdoe;
//    ISP2_Sensor = sensor;
    setJPEGaddr(mdoe, addr, sensor);
    if(sensor == -1)
    	S2AddSTTableProcTest();
	else if(sensor == -3)
       	STTableTestS2AllSensor();
}

/*
 * idx  0:none 1:1s 2:2s 3:5s 4:10s 5:30s 6:60s 7:0.166s
 * */
int LapseModeToSec(int idx)
{
    switch(idx){
        case 0: return 0;
        case 1: return 1;
        case 2: return 2;
        case 3: return 5;
        case 4: return 10;
        case 5: return 30;
        case 6: return 60;
        case 7: return 166;
    }
    return 0;
}

/*
 * -1: ?s??        0: ?@??????    2:2???۩?    10:10???۩?
 * */
int CaptureModeToSec(int idx)
{
    switch(idx){
        case 2: return 2;
        case 10: return 10;
        case 30: return 30;
        default:
          	return idx;
    }
}

void Set_OLED_MainType(int c_mode, int cap_cnt, int cap_mode, int tl_mode, int debug) {
#ifdef __CLOSE_CODE__	//tmp	
   	switch(c_mode) {    // ?۾??Ҧ?, 0:Cap 1:Rec 2:TimeLapse 3:HDR(3P) 4:RAW(5P) 5:WDR 6:Night 7:NightWDR
   	case 0:                             // capture
       	if(cap_cnt != 0)       setOLEDMainType(3);    // ?s??
       	else if(cap_mode == 0) setOLEDMainType(2);    // ???`            // photo
       	else                   setOLEDMainType(2);    // ?۩?
       	break;
   	case 1:                             // record
   	case 2:                             // time lapse
       	if(tl_mode == 0) setOLEDMainType(0);       // rex+ 160302
       	else             setOLEDMainType(1);
       	break;
   	case 3: setOLEDMainType(5); break;  // AEB
   	case 4: setOLEDMainType(6); break;  // RAW(5P)
   	case 5: setOLEDMainType(7); break;  // HDR
    case 6: setOLEDMainTypesetOLEDMainType(8); break;  // Night, ?Ȯ?181106
    case 7: setOLEDMainType(9); break;  // NightHDR, ?Ȯ?181106
    case 8:  setOLEDMainType(10); break;  // Sport
    case 9:  setOLEDMainType(11); break;  // SportWDR
    case 10: setOLEDMainType(12); break;  // RecWDR
    case 11: setOLEDMainType(13); break;  // TimeLapseWDR
    case 12: setOLEDMainType(14); break;  // B?֪?
    case 13: setOLEDMainType(15); break;  // remove
    case 14: setOLEDMainType(15); break;  // 3D Model
   	}
    showTimeLapse(LapseModeToSec(Get_DataBin_TimeLapseMode() ), CaptureModeToSec(Get_DataBin_CaptureMode() ), Get_DataBin_CaptureCnt() );
#endif	//__CLOSE_CODE__	
}

void Change_Cap_12K_Mode()
{
    int c_mode = getCameraMode();
    int resolution = getResolutionMode();
    if(c_mode != 0 || resolution != RESOLUTION_MODE_12K || getPlayModeTmp() !=0) {	//Change Mode: Cap 12K Mode
        c_mode = 0;
        resolution = 1;
        setCameraMode(c_mode);
        setResolutionMode(resolution);
        Set_OLED_MainType(c_mode, getCaptureCnt(), getCaptureMode(), getTimeLapseMode(), 13);
        setPlayModeTmp(0);
        ModeTypeSelectS2(getPlayModeTmp(), resolution, getHdmiState(), c_mode);
        choose_mode(c_mode, getPlayModeTmp(), resolution, getFPS());
    }
}

void Change_Raw_12K_Mode()
{
    int c_mode = getCameraMode();
    int resolution = getResolutionMode();
    if(c_mode != 4 || resolution != RESOLUTION_MODE_12K || getPlayModeTmp() !=0) {	//Change Mode: Raw 12K Mode
        c_mode = 4;
        resolution = 1;
        setCameraMode(c_mode);
        setResolutionMode(resolution);
        Set_OLED_MainType(c_mode, getCaptureCnt(), getCaptureMode(), getTimeLapseMode(), 14);
        setPlayModeTmp(0);
        ModeTypeSelectS2(getPlayModeTmp(), resolution, getHdmiState(), c_mode);
        choose_mode(c_mode, getPlayModeTmp(), resolution, getFPS());
    }
}

void Send_ST_Cmd_Proc(void) {
	SendSTCmd();
	check_st_cmd_ddr_flag = 0;
}

int do_Auto_Stitch_Proc(int debug)
{
   	int ret = -1;

   	db_debug("do_Auto_Stitch() flag=%d\n", doAutoStitch_flag);
   	if(doAutoStitch_flag == 1) {
   		ret = doAutoStitch();
		if(ret >= 0) {
			WriteSensorAdjFile();
			ReadSensorAdjFile();

			// write cmd to fpga
			AdjFunction();
			Send_ST_Cmd_Proc();

			WriteTestResult(0, debug);
		}
//tmp		paintOLEDStitching(0);
		doAutoStitch_flag = 0;
   	}
   	else ret = -1;

   	return ret;
}

void *thread_5ms(void)
{
	int ret, state;
    int skip_watchdog;
    int focus_sensor2;
    unsigned long long now_time;
    static int skip_watchdog_lst = -1;
	static int saveBinStep=0;
	static int dna_check_lst = -1;
	static int hdmi_state_lst = -1;
	static int Sensor_State_Cnt = 0;
	static int ledFirstConnect = 0;
	static unsigned long long SendMainCmdPipeT1=0, SendMainCmdPipeT2=0;
	static unsigned long long SensorStateT1=0, SensorStateT2=0;
	static unsigned long long AdjSensorSyncT1=0, AdjSensorSyncT2=0;
	static unsigned long long curTime, lstTime=0, runTime, nowTime;
    //nice(-6);    // ?վ?thread?u???v

    while(thread_5ms_en)
    {
		get_current_usec(&curTime);
        if(curTime < lstTime) lstTime = curTime;
        else if((curTime - lstTime) >= 1000000){
			db_debug("task5ms: runTime=lld rtsp: fps=%d sand=%dkb mjpeg: fps=%d send=%dkb\n",
        		runTime, getRtspFps(), getRtspSendLength()/1024, getMjpegFps(), getMjpegSendLength()/1024);
			lstTime = curTime;
			setMjpegFps(0);
			setMjpegSendLength(0);
			setRtspFps(0);
			setRtspSendLength(0);
        }

		if(writeUS360DataBin_flag == 1){
			writeUS360DataBin_flag = 0;
			saveBinStep = 1;
			set_timeout_start(5);
		}
		else if(saveBinStep == 1){
			if(check_timeout_start(5, 1000) == 1){    // ?]?w1sec???x?s
				saveBinStep = 0;
				WriteUS360DataBin();
			}
		}
		//wifiSerThe_proc_jni();

        skip_watchdog = getSkipWatchDog();
		if(skip_watchdog_lst != skip_watchdog){
			skip_watchdog_lst = skip_watchdog;

			if(skip_watchdog == 0)
				deleteSkipWatchDogFile();
			else if(skip_watchdog == 1)
				createSkipWatchDogFile();
			else if(skip_watchdog == 2)
				System_Exit_jni();
		}

		if(dna_check_lst != getDnaCheck() ){
			dna_check_lst = getDnaCheck();
//        	mHandler.obtainMessage(FPGA_SHOW_NOW).sendToTarget();
		}

		//??HDMI???A, ????CPU?W?v?MFPS
		if(getHdmiState() != hdmi_state_lst) {
//tmp       SetRenderEn(getHdmiState());

			if(getHdmiState() == 1) {				//HDMI???W, ?????ٹq???A
				if(getPowerSavingMode() == 1) {
					if(GetFPGASleepEn() == 1) {
						SetFPGASleepEn(0);
						Adj_Sensor_Sync_Flag = 2;
					}
					setPowerSavingCapRecStartTime(0);
					setPowerSavingCapRecFinishTime(0, POWER_SAVING_CMD_OVERTIME_5S);
				}
			}
			else {								//HDMI?ޥX, ?i?J?ٹq???A
				if(getPowerSavingMode() == 1 && GetFPGASleepEn() == 0)
					setPowerSavingCapRecFinishTime(curTime, 0);
			}

			hdmi_state_lst = getHdmiState();
			setHdmiStateChange(1);
		}

		if(FPGA_Ctrl_Power == 0) {
			do_Defect_Func();
			Cmd_Idx = readCmdIdx();
			if(Cmd_P1 != Cmd_Idx) {

				if(choose_mode_flag == 1) {        // ?????Ҧ?
					choose_mode(getCameraMode(), getPlayModeTmp(), getResolutionMode(), getFPS());
					isNeedNewFreeCount = 1;
					choose_mode_flag = 0;
//                	mHandler.obtainMessage(REC_SHOW_NOW_MODE).sendToTarget();
					check_st_cmd_ddr_flag = 0;
				}
				else if(TestToolCmd.State == 1) {
					switch(TestToolCmd.MainCmd) {
					case 0:
						break;
					case 1:        //Focus
						if(TestToolCmd.Step == 1) {
							Set_ISP2_Addr(1, 0, 5);
							TestToolCmd.Step = 2;		//testtool.SetStep(2);
						}
						break;
					case 2:        //Focus
						if(TestToolCmd.SubCmd >= 0 && TestToolCmd.SubCmd <= 4) {			//Show Focus
							if(TestToolCmd.Step == 1) {
                                focus_sensor2 = TestToolCmd.SubCmd;
								setFocusSensor2(focus_sensor2);
								Change_Cap_12K_Mode();
								FocusScanTableTranInit();
								FocusResultInit();
								ReadTestToolFocusAdjFile(focus_sensor2);
//tmp								if(getFocusVisibilityMode() == 0) {
//tmp									setFocusVisibilityMode(1);
//tmp									mHandler.obtainMessage(SHOW_FOCUS_VISIBILITY).sendToTarget();
//tmp								}
								set_A2K_Debug_Focus(focus_sensor2);
								Set_ISP2_Addr(1, 0, -6);
								do2DNR(1);		//?U?Q?׽զ̧ܳC

								TestToolCmd.Step = 2;		//testtool.SetStep(2);
							}
						}
						else if(TestToolCmd.SubCmd == 99) {							//Save Focus File
							if(TestToolCmd.Step == 1) {
                                focus_sensor2 = getFocusSensor2();
								if(focus_sensor2 >= 0 && focus_sensor2 <= 4)
									WriteFocusResult(focus_sensor2);
								TestToolCmd.Step = 2;		//testtool.SetStep(2);
							}
						}
						else if(TestToolCmd.SubCmd == 100) {							//Exit Focus
							focus_sensor2 = -1;
                            setFocusSensor2(focus_sensor2);
							set_A2K_Debug_Focus(focus_sensor2);
							do2DNR(0);
							setFocusVisibilityMode(0);
							choose_mode_flag = 1;
							TestToolCmdInit();
						}
						else if(TestToolCmd.SubCmd == 101) {							//Get 12K Raw File & Get Foucs Posi
							if(TestToolCmd.Step == 1) {
								Change_Raw_12K_Mode();
								TestToolCmd.Step = 2;		//testtool.SetStep(2);
							}
							else if(TestToolCmd.Step == 2) {
								doTakePicture(7);
								TestToolCmd.Step = 3;		//testtool.SetStep(3);
							}
						}

						break;
					case 3: break;
					case 4: break;
					case 5:
						if(TestToolCmd.SubCmd == 0) {
							Change_Cap_12K_Mode();
						}
						else if(TestToolCmd.SubCmd == 1 || TestToolCmd.SubCmd == 2 || TestToolCmd.SubCmd == 3 || TestToolCmd.SubCmd == 4 || TestToolCmd.SubCmd == 5) {
							get_current_usec(&now_time);
                            setTesttoolDelayTime(2, now_time);
							if(TestToolCmd.Step == 1) {        //delay 1s
								if(getTesttoolDelayTime(2) < getTesttoolDelayTime(1))   //?w?????~
                                    setTesttoolDelayTime(2, getTesttoolDelayTime(1));
								if((getTesttoolDelayTime(2) - getTesttoolDelayTime(1)) > 2000000)
									TestToolCmd.Step = 2;		//testtool.SetStep(2);
							}
							else if(TestToolCmd.Step == 2) {    //take picture
								doTakePicture(3);
								TestToolCmd.Step = 3;		//testtool.SetStep(3);
							}
						}
						else if(TestToolCmd.SubCmd == 7) {
							ReadSensorAdjFile();
							WriteLensRateTable();
							TestToolCmdInit();
							WriteTestResult(1, 0);

							//Write Finish File -> TestTool Restart App
							createAutoStitchRestartFile();
						}
						break;
					case 6:
						if(TestToolCmd.SubCmd == 1 || TestToolCmd.SubCmd == 2 || TestToolCmd.SubCmd == 3 || TestToolCmd.SubCmd == 4) {
							get_current_usec(&now_time);
                            setTesttoolDelayTime(2, now_time);
							if(TestToolCmd.Step == 1) {        //delay 1s
								if(getTesttoolDelayTime(2) < getTesttoolDelayTime(1))   //?w?????~ 
                                    setTesttoolDelayTime(2, getTesttoolDelayTime(1));
								if((getTesttoolDelayTime(2) - getTesttoolDelayTime(1)) > 1000000)
									TestToolCmd.Step = 2;		//testtool.SetStep(2);
							}
							else if(TestToolCmd.Step == 2) {    //take picture
								doTakePicture(3);
								TestToolCmd.Step = 3;		//testtool.SetStep(3);
							}
						}
						else if(TestToolCmd.SubCmd == 7) {
							db_debug("adj sensor lens End!\n");
							ReadAdjSensorLensFile();
							TestToolCmdInit();
						}
						break;
					case 7:        //test mode
						do_Test_Mode_Func_jni(TestToolCmd.MainCmd, TestToolCmd.SubCmd);
						break;
					case 8:
						if(TestToolCmd.SubCmd == 0) {
							if(TestToolCmd.Step == 1) {
								Set_ISP2_Addr(2, 0, -1);
								writeCmdTable(4, getResolutionMode, getFPS(), 3, 0, 1);
//                            	AutoGlobalPhiInit(1);
								TestToolCmd.Step = 2;		//testtool.SetStep(2);
							}
//                          AutoGlobalPhiAdj(1, 1);
						}
						else if(TestToolCmd.SubCmd == 1) {
							get_current_usec(&now_time);
                            setTesttoolDelayTime(2, now_time);
							if(TestToolCmd.Step == 1) {        //delay 1s
//                              AutoGlobalPhiAdj(1, 1);
								if(getTesttoolDelayTime(2) < getTesttoolDelayTime(1))   //?w?????~ 
                                    setTesttoolDelayTime(2, getTesttoolDelayTime(1));
								if((getTesttoolDelayTime(2) - getTesttoolDelayTime(1)) > 1000000)
									TestToolCmd.Step = 2;		//testtool.SetStep(2);
							}
							else if(TestToolCmd.Step == 2) {    //take picture
								doTakePicture(5);
								TestToolCmd.Step = 3;		//testtool.SetStep(3);
							}
						}
						else if(TestToolCmd.SubCmd == 2) {
							//
						}
						break;
					case 100: 	// S2 sensor*5 auto st
						if(TestToolCmd.SubCmd == 1 || TestToolCmd.SubCmd == 2 || TestToolCmd.SubCmd == 3 || TestToolCmd.SubCmd == 4 || TestToolCmd.SubCmd == 5) {
							get_current_usec(&now_time);
                            setTesttoolDelayTime(2, now_time);
							if(TestToolCmd.Step == 1) {		//delay 1s
								if(getTesttoolDelayTime(2) < getTesttoolDelayTime(1))   //?w?????~ 
                                    setTesttoolDelayTime(2, getTesttoolDelayTime(1));
								if((getTesttoolDelayTime(2) - getTesttoolDelayTime(1)) > 1000000)
									TestToolCmd.Step = 2;		//testtool.SetStep(2);
							}
							else if(TestToolCmd.Step == 2) {	//take picture
								doTakePicture(3);
								TestToolCmd.Step = 3;		//testtool.SetStep(3);
							}
						}
						else if(TestToolCmd.SubCmd == 7) {
							ReadSensorAdjFile();

							// write cmd to fpga
							AdjFunction();
							//writeSensorLine();
							choose_mode_flag = 1;
							TestToolCmdInit();

							WriteTestResult(1, 0);
						}
						//testtool.State = 0;
						break;
					}
				}
				else if(getCameraPositionModeChange() == 1) {
                    set_A2K_DMA_CameraPosition(mCameraPositionMode);
					AdjFunction();
					Send_ST_Cmd_Proc();
					setCameraPositionModeChange(0);
				}
				else {
					if(get_ISP2_Sensor() == -5) {
						STTableTestS2ShowSTLine();
					}
				}
				Cmd_P1 = Cmd_Idx;
			}

			if(getRecordCmdFlag() == 1) {
				if(getRecordEn() == 1){
					lockRecordEnJNI = 1;
					get_current_usec(&task5s_lock_time);
					task5s_lock_flag = 1;
				}
				doRecordVideo(getRecordEn(), getPlayMode(), getResolutionMode, getTimeLapseMode(), getHdmiState());
				setRecordCmdFlag(2);
			}

			if(getRecordCmdFlag() == 2 || get_live360_cmd() == 2) {
				if(isRecording == 1)
					audio_record_thread_start_jni();
				else
					audio_record_thread_release_jni();
				if(getRecordCmdFlag() == 2) setRecordCmdFlag(3);
				if(get_live360_cmd() == 2) set_live360_cmd(3);
			}

			switch(Send_ST_Flag) {
			case 0:
//        		SetSendSTTime(1);		//Send_ST_T1 = System.currentTimeMillis();
//        		SetSendSTTime(2);		//Send_ST_T2 = System.currentTimeMillis();
				Send_ST_Flag = 1;
				break;
			case 1:
//        		SetSendSTTime(1);		//Send_ST_T1 = System.currentTimeMillis();
				if(GetMcuUpdateFlag() == 0) {
					if(do_Auto_Stitch_Proc(0) < 0)
						Send_ST_Cmd_Proc();
				}
				else
					Send_ST_Cmd_Proc();
				Send_ST_Flag = 2;
				break;
			case 2:
				get_current_usec(&SendMainCmdPipeT1);
				if(SendMainCmdPipeT1 < SendMainCmdPipeT2) SendMainCmdPipeT1 = SendMainCmdPipeT2;
				if( (SendMainCmdPipeT1 - SendMainCmdPipeT2) > 50000) {
					SendMainCmdPipeT2 = SendMainCmdPipeT1;
					choose_mode(getCameraMode(), getPlayModeTmp(), getResolutionMode(), getFPS());
					SendMainCmdPipe(getCameraMode(), getTimeLapseMode(), 1);
					MainCmdStart();

					SendH264EncodeTable();
					setFPGASpeed(0, 0xB80);
					setFPGASpeed(1, 0xB80);
					setFPGASpeed(2, 0xD00);

					//Defect Init
					set_A2K_ISP2_Defect_En(0);
					SetDefectState(0);
					ret = DefectInit();
					if(ret == 0 && getCameraMode() == CAMERA_MODE_M_MODE) {
						set_A2K_ISP2_Defect_En(1);
						SetDefectState(1);
					}

					setSaturationInitFlag(1);
					Send_ST_Flag = 3;
					UVCErrCount = 0;
					Adj_Sensor_Sync_Flag = 0;
				}
				break;
			case 3:
				if(GetFPGASleepEn() != 0) break;

				if(check_st_cmd_ddr_flag == 0) {
					CheckSTCmdDDRCheckSum();
					check_st_cmd_ddr_flag = 1;
				}

				get_current_usec(&SensorStateT1);
				if(SensorStateT1 < SensorStateT2) SensorStateT1 = SensorStateT2;
				if( (SensorStateT1 - SensorStateT2) > 1000000) {
					SensorStateT2 = SensorStateT1;
					state = readSensorState();
					if(state == 1) {
						Sensor_State_Cnt++;
						db_error("Cnt=%d ST=%d Flag=%d\n", Sensor_State_Cnt, Send_ST_Flag, Adj_Sensor_Sync_Flag);
						if(Sensor_State_Cnt >= 3) {
							Adj_Sensor_Sync_Flag = 1;
							Sensor_State_Cnt = 0;
						}
					}
					else{
						if(Sensor_State_Cnt > 0){
							Sensor_State_Cnt--;
						}
					}
				}

				get_current_usec(&AdjSensorSyncT1);
				if(AdjSensorSyncT1 < AdjSensorSyncT2) AdjSensorSyncT1 = AdjSensorSyncT2;
				if(Adj_Sensor_Sync_Flag == 0 && (AdjSensorSyncT1 - AdjSensorSyncT2) > 3000000) {
					AdjSensorSyncT2 = AdjSensorSyncT1;
					Adj_Sensor_Sync_Flag = 2;
				}

				if(getSaturationInitFlag() == 1) {
					setSaturationInitFlag(0);
					SetSaturationValue(0, getSaturation());
				}

				get_current_usec(&SendMainCmdPipeT1);
				if(SendMainCmdPipeT1 < SendMainCmdPipeT2) SendMainCmdPipeT1 = SendMainCmdPipeT2;
				if( (SendMainCmdPipeT1 - SendMainCmdPipeT2) > 50000) {
					SendMainCmdPipeT2 = SendMainCmdPipeT1;
					if(Adj_Sensor_Sync_Flag != 0) {
						SendMainCmdPipe(getCameraMode(), getTimeLapseMode(), Adj_Sensor_Sync_Flag);
						Adj_Sensor_Sync_Flag = 0;
					}
					else {
						SendMainCmdPipe(getCameraMode(), getTimeLapseMode(), 0);
					}
				}
				break;
			}

		}

		get_current_usec(&nowTime);
		if(task5s_lock_flag == 1 && (nowTime - task5s_lock_time) > task5s_lock_schedule) {		//java: Timer5sLockTask()
			task5s_lock_flag = 0;
			lockRecordEnJNI = 0;
		}

		if(WifiConnected == 1){
			if(ledFirstConnect == 0){
				ledFirstConnect = 1;
//tmp       	ChangeLedByTime(0,2);
//tmp       	SetLedControlMode(3);
			}
		}else{
			if(ledFirstConnect == 1){
				ledFirstConnect = 0;
//tmp       	ChangeLedMode(0);
//tmp           SetLedControlMode(0);
			}
		}
	
		get_current_usec(&runTime);
        runTime -= curTime;
        if(runTime < 5000){
            usleep(5000 - runTime);
            get_current_usec(&runTime);
            runTime -= curTime;
        }
	}	//while(1)
}

void create_thread_5ms() {
    pthread_mutex_init(&mut_5ms_buf, NULL);
    if(pthread_create(&thread_5ms_id, NULL, thread_5ms, NULL) != 0) {
        db_error("Create thread_5ms fail !\n");
    }	
}

int get_RecFPS(void)
{
    int mode, res;
    int fps = 100;
    int freq = get_ISP_AEG_EP_IMX222_Freq();

    if(getTimeLapseMode() != 0) {
        if(freq == 0) fps = 300;
        else          fps = 250;
        return fps;
    }

    get_Stitching_Out(&mode, &res);
    switch(res){
    case 1:         //12K
    	if(freq == 0) fps = 10;
    	else		  fps = 10;
        break;
    case 2:         //4K
    	if(freq == 0) fps = 100;
    	else		  fps = 100;
        break;
    case 7:         //8K
    	if(freq == 0) fps = 20;
    	else		  fps = 20;
        break;
    case 12:        //6K
    	if(freq == 0) fps = 30;
    	else		  fps = 30;
        break;
    case 13:        //3K
    	if(freq == 0) fps = 240;
    	else		  fps = 200;
        break;
    case 14:        //2K
    	if(freq == 0) fps = 300;
    	else		  fps = 250;
        break;
    }
    int ep_fps = get_AEG_EP_FPS();
    if(fps >= ep_fps) fps = ep_fps;

    return fps;
}
int get_LiveFPS(void)
{
    int mode, res;
    int fps = 100;
    int freq = get_ISP_AEG_EP_IMX222_Freq();

    get_Stitching_Out(&mode, &res);
    switch(res){
    case 1:         //12K
    	if(getTimeLapseMode() != 0)
    		fps = 10;
    	else {
			if(freq == 0) fps = 100;
			else		  fps = 100;
    	}
        break;
    case 2:         //4K
    	if(freq == 0) fps = 100;
    	else		  fps = 100;
        break;
    case 7:         //8K
    	if(getTimeLapseMode() != 0)
    		fps = 20;
    	else {
			if(freq == 0) fps = 100;
			else		  fps = 100;
    	}
        break;
    case 12:        //6K
    	if(getTimeLapseMode() != 0)
    		fps = 30;
    	else {
			if(freq == 0) fps = 100;
			else		  fps = 100;
    	}
        break;
    case 13:        //3K
    	if(freq == 0) fps = 240;
    	else		  fps = 200;
        break;
    case 14:        //2K
    	if(freq == 0) fps = 300;
    	else		  fps = 250;
        break;
    }
    int ep_fps = get_AEG_EP_FPS();
    if(fps >= ep_fps) fps = ep_fps;

    return fps;
}

void check_tl_path_repeat(char *tl_path, char *dir_path, char *ssid, int f_cnt, int tl_cnt) {
	char path_tmp[128];
    sprintf(path_tmp, "%s/T%c%c%c%c_%04d_%04d\0", dir_path, ssid[3], ssid[4], ssid[5], ssid[6], f_cnt, tl_cnt);
    check_path_repeat(tl_path, &path_tmp[0], ".jpg\0");
}

int read_pcm_buf(pcm_buf_struct *buf, unsigned char *a_buf, int size)
{
    int a_p1, a_p2;
    int pcm_size, pcm_p2_tmp;
    int is_audio = 0;

    a_p1 = buf->P1;
    a_p2 = buf->P2;
    if(a_p1 != a_p2) {
        pcm_size = size;    //buf->size;
        pcm_p2_tmp = a_p2 + pcm_size;
        if(pcm_p2_tmp < PCM_BUF_MAX) {
            if( (a_p1 > a_p2 && pcm_p2_tmp <= a_p1) ||
                    a_p1 < a_p2) {
                memcpy(&a_buf[0], &buf->buf[a_p2], pcm_size);
                buf->P2 = pcm_p2_tmp & (PCM_BUF_MAX-1);
                is_audio = 1;
            }
        }
        else {
            if(a_p1 < a_p2 && (pcm_p2_tmp - PCM_BUF_MAX) <= a_p1) {
                memcpy(&a_buf[0], &buf->buf[a_p2], (PCM_BUF_MAX - a_p2) );
                memcpy(&a_buf[PCM_BUF_MAX - a_p2], &buf->buf[0], (pcm_p2_tmp - PCM_BUF_MAX) );
                buf->P2 = (pcm_p2_tmp - PCM_BUF_MAX) & (PCM_BUF_MAX-1);
                is_audio = 1;
            }
        }
    }
    return is_audio;
}

#ifdef __CLOSE_CODE__	//tmp
int read_aac_buf(pcm_buf_struct *buf, unsigned char *a_buf,
		LinkADTSFixheader *fix, LinkADTSVariableHeader *var)
{
    int a_p1, a_p2;
    int size=0, size_tmp=0;
    int is_audio = 0;
    unsigned char header[8];

    a_p1 = buf->P1;
    a_p2 = buf->P2;
    if(a_p1 != a_p2) {
    	// AAC?e??7??Byte?????Y(LinkADTSFixheader + LinkADTSVariableHeader)
    	size_tmp = (a_p2+sizeof(header));
    	if(size_tmp < PCM_BUF_MAX) {
    		memcpy(&header[0], &buf->buf[a_p2], sizeof(header));
    	}
    	else {
    		memcpy(&header[0], &buf->buf[a_p2], (PCM_BUF_MAX - a_p2) );
    		memcpy(&header[PCM_BUF_MAX - a_p2], &buf->buf[0], (size_tmp - PCM_BUF_MAX) );
    	}
        LinkParseAdtsfixedHeader(&header[0], fix);
        LinkParseAdtsVariableHeader(&header[0], var);

        size = var->aac_frame_length;
        size_tmp = a_p2 + size;
        if(size_tmp < PCM_BUF_MAX) {
            if( (a_p1 > a_p2 && size_tmp <= a_p1) ||
                    a_p1 < a_p2) {
                memcpy(&a_buf[0], &buf->buf[a_p2], size);
                buf->P2 = size_tmp & (PCM_BUF_MAX-1);
                is_audio = 1;
            }
        }
        else {
            if(a_p1 < a_p2 && (size_tmp - PCM_BUF_MAX) <= a_p1) {
                memcpy(&a_buf[0], &buf->buf[a_p2], (PCM_BUF_MAX - a_p2) );
                memcpy(&a_buf[PCM_BUF_MAX - a_p2], &buf->buf[0], (size_tmp - PCM_BUF_MAX) );
                buf->P2 = (size_tmp - PCM_BUF_MAX) & (PCM_BUF_MAX-1);
                is_audio = 1;
            }
        }
    }
    return is_audio;
}
#endif	//__CLOSE_CODE__

int prepareCamera(int videoid, int videobase)
{
	int ret, freq;
	int M_Mode, S_Mode;

    doResize_buf_init();
    ret = openUVC(videoid, videobase);

	//create thread ---------------------------------------------------------
    pthread_mutex_init(&mut_dec_buf, NULL);
    if(pthread_create(&thread_uvc_id, NULL, uvc_thread, NULL) != 0)
    {
        db_error("Create uvc_thread fail !\n");
    }

    freq = get_ISP_AEG_EP_IMX222_Freq();
    Get_M_Mode(&M_Mode, &S_Mode);
    pcm_buf_init(freq, M_Mode, mic_is_alive, 0);
//#ifndef NEW_AUDIO_RECORD
    pthread_mutex_init(&mut_pcm_buf, NULL);
    if(pthread_create(&thread_pcm_id, NULL, pcm_thread, NULL) != 0)
    {
        db_error("Create pcm_thread fail !\n");
    }
//#endif

    pthread_mutex_init(&mut_st_buf, NULL);
    if(pthread_create(&thread_st_id, NULL, st_thread, NULL) != 0)
    {
        db_error("Create st_thread fail !\n");
    }

    pthread_mutex_init(&mut_touch_up, NULL);
    return 0;
}

/*
 *    ?}?? uvc driver & init thread
 */
//int prepareCameraWithBase(int videoid, int videobase, int pixelformat, int width, int height)
//{
//        //cameraBase = videobase;
//        IMG_Pixelformat = pixelformat;
//        IMG_WIDTH  = width;
//        IMG_HEIGHT = height;
//        return prepareCamera(videoid, videobase);
//}

/*
 * rex+ 151221
 *   public function
 * */
/*char *get_storage_path(void)
{
    return sd_path;
}*/

/*
 * rex+ 151221
 *   public function
 *
 * ???????󪬺A
 *   S_IRGRP 00040 ?Τ??ը??iŪ???v??
 *   S_IWGRP 00020 ?Τ??ը??i?g?J?v??
 *   S_IXGRP 00010 ?Τ??ը??i?????v??
 *   ?Ѧ? http://c.biancheng.net/cpp/html/326.html
 *   return 0:normal
 *         -1:error
 * */
int checksd(void)
{
    char sd_path[128];
    struct stat st;
    
    getSdPath(&sd_path[0], sizeof(sd_path));
    stat(sd_path, &st);
    if((st.st_mode & 0x00020) == 0x00020)
    	return 0;
    else
    	return -1;
}

/*
 * ?T?{UVC?O?_???إ߳s?u (video0)
 */
int checkUVC(void)
{
    struct stat st;

    if (-1 == stat ("/dev/video0", &st)) {
        db_error("Cannot find video0\n");
        return -1;
    }
    return 0;
}

// c_mode=0,3,4,5,6,7,8,9,12,13    return=1
int get_C_Mode_Picture(int c_mode)
{
    return(C_Mode_ROM[c_mode].Picture);
}
// c_mode=0,5,6,7,8,9,12,13    return=1
int get_C_Mode_Single_Pic(int c_mode)
{
    return(C_Mode_ROM[c_mode].Single_Pic);
}

int CheckSaveJpegCnt(void)
{
	unsigned long long curTime;
    get_current_usec(&curTime);
    if(Capture_Is_Finish() == 0 && Get_F_Cmd_In_Capture_Step() == 0){     // ?s???٨S???槹
        if(curTime < Save_Jpeg_StartTime) Save_Jpeg_StartTime = curTime;    // ?????ҥ~???~
        else if((curTime - Save_Jpeg_StartTime) >= Save_Jpeg_OutTime){
        	db_error("setCapEn.E: FpgaPS: timeout! now=%d end=%d tout=%lld outTime=%lld\n", Save_Jpeg_Now_Cnt, Save_Jpeg_End_Cnt, curTime-Save_Jpeg_StartTime, Save_Jpeg_OutTime);
            Save_Jpeg_Now_Cnt = Save_Jpeg_End_Cnt;
        }
    }

    if(Capture_Is_Finish() == 1)
    	return 0;
    else
    	return 1;
}

//tmp	extern int Dec_Bottom_step;         // awcodec.c
/*
 *    ?]?w???ӼҦ?
 *    capEn: 0:none 1:????  2:?s??  3:Sensor_ISP2 4:Test_Block 5:Test_GP_Block 6:5Sensor 7:TestFocus_GetRAW 8:Defect_Img(5P)
 *    capCnt: ?s?禸??
 *    capStime: ?s?綡?j?ɶ?(?ثe?T?w160ms)
 *    freesize: ?x?s?Ŷ??Ѿl?e?q
 */
int setCapEn(int capEn, int capCnt, int capStime, unsigned long long freesize)
{
    static unsigned long long curTime;
    int i, ret=-1, cmd_p1, cmd_p2, step=0;
    int c_mode = getCameraMode();
    setCaptureIntervalTime(160);    //capStime;
    sd_freesize = freesize;
    char sd_path[128];

    if(get_Init_Gamma_Table_En() != 0) return -1;    // Gamma, 0:ready, 1:waiting
//tmp    if(Dec_Bottom_step != 0) return -1;        // ????, 0:ready

    get_current_usec(&curTime);

    CheckSaveJpegCnt();
    getSdPath(&sd_path[0], sizeof(sd_path));

    if(capEn == 1 || capEn == 2 || capEn == 7 || capEn == 8) {
        step = Get_F_Cmd_In_Capture_Step();
        if(Capture_Is_Finish() == 0 || step != 0)     // ?e?@?D?B?z???~?i?H?[?s?????O
            ret = -2;
        else
        	ret = FPGA_Com_In_Add_Capture(1);               // FPGA_Pipe.c, setting
        db_debug("setCapEn.1: en=%d cnt=%d ret=%d freesize=%lldmb\n", capEn, capCnt, ret, freesize/1000000);
      
        if(ret == 1){
            if(c_mode == CAMERA_MODE_AEB || c_mode == CAMERA_MODE_HDR || c_mode == CAMERA_MODE_NIGHT_HDR || BSmooth_Function != 0)   // 3:HDR 5:WDR 6:Night 7:Night+WDR, ?ϥΤj??YUV?_?X
                set_run_big_smooth(1);
            else
                set_run_big_smooth(0);
            save_sensor_data();
            
            switch(c_mode){
            case CAMERA_MODE_AEB:                                             // 3:AEB(3p,5p,7p)
                    Save_Jpeg_End_Cnt = get_AEB_Frame_Cnt();
                    capEn = 6;
                    break;
            case CAMERA_MODE_RAW: Save_Jpeg_End_Cnt = 5; capEn = 6; break;    // 4:RAW
            case CAMERA_MODE_M_MODE:
            	if(GetDefectStep() != 0)
            		Save_Jpeg_End_Cnt = 5;
            	else {
                    if(capEn == 2 && capCnt > 0)                    // ?s??
                        Save_Jpeg_End_Cnt = capCnt;                 // capCnt=3,5,10
                    else
                        Save_Jpeg_End_Cnt = 1;                      // Capture=0, WDR=5
            	}
            	break;
            default:
                if(capEn == 2 && capCnt > 0)                    // ?s??
                    Save_Jpeg_End_Cnt = capCnt;                 // capCnt=3,5,10
                else
                    Save_Jpeg_End_Cnt = 1;                      // Capture=0, WDR=5
                break;
            }
            Save_Jpeg_Now_Cnt = 0;
            Save_Jpeg_StartTime = curTime;
            //Save_Jpeg_OutTime = 6000000*Save_Jpeg_End_Cnt;            // ?]?w???i6s timeout, 4s?n??+2s?s??
            if(c_mode == CAMERA_MODE_M_MODE){         // 12:M-Mode
            	int bexp_1sec, bexp_gain;
                get_AEG_B_Exp(&bexp_1sec, &bexp_gain);
                Save_Jpeg_OutTime = bexp_1sec * 1000000 + 30000000;
            }
            else if(c_mode == CAMERA_MODE_REMOVAL)
            	Save_Jpeg_OutTime = 60000000;
            else
            	Save_Jpeg_OutTime = 30000000;            // ?]?w???i30s timeout


            F_Cmd_In_Capture_Start();	//Init
            Set_F_Cmd_In_Capture_T1();
			Set_F_Cmd_In_Capture_Lose_Flag(0);
        }
    }
    else {
        Save_Jpeg_Now_Cnt = 0;
        Save_Jpeg_End_Cnt = 1;                          // ?]?w?x?s1?i
        Save_Jpeg_StartTime = curTime;
        //Save_Jpeg_OutTime = 6000000*Save_Jpeg_End_Cnt;            // ?]?w???i6s timeout
        Save_Jpeg_OutTime = 30000000;            // ?]?w???i30s timeout
        db_debug("setCapEn.2: en=%d cnt=%d ret=%d freesize=%lldmb\n", capEn, capCnt, ret, freesize/1000000);
        ret = 1;
    }
    if(ret == 1){
        if(Cap_Cmd_Queue.P1 != Cap_Cmd_Queue.P2){
            db_debug("Cap_Cmd_Queue: P1=%d P2=%d capEn=%d\n", Cap_Cmd_Queue.P1, Cap_Cmd_Queue.P2, capEn);
            Cap_Cmd_Queue.P1 = Cap_Cmd_Queue.P2;        // ?Y?S???槹?A?h???L?A?קK?s???????X??
        }
        cmd_p1 = Cap_Cmd_Queue.P1;
        cmd_p2 = Cap_Cmd_Queue.P2;
        for(i = 0; i < Save_Jpeg_End_Cnt; i++){
            //db_debug("Cap_Cmd_Queue: cmd_p1=%d capEn=%d\n", cmd_p1, capEn);
            Cap_Cmd_Queue.queue[cmd_p1] = capEn;
            cmd_p1++;
            if(cmd_p1 >= CMD_QUEUE_MAX) cmd_p1 = 0;
            Cap_Cmd_Queue.P1 = cmd_p1;
        }
    }
    setLidarSavePath(sd_path,mSSID,cap_file_cnt);

    return ret;
}

/*
 *    ?]?w???v?Ҧ?
 *    recState: 0:?}?l???v -1:???????v
 *    time_lapse:    ?Y?ɿ??v???j?ɶ?(0:???`???v 1:1s 2:2s 5:5s 10:10s 30:30s 60:60s)
 *    freesize: ?x?s?Ŷ??Ѿl?e?q
 *    driving_mode: ?樮?????Ҧ?(?`?????v) 0:close 1:open
 */
void setRecEn(int recState, int time_lapse, unsigned long long freesize, int timelapse_enc)
{
    int cmd_p1, cmd_p2, exp_max;
    int mode, res;
    int c_mode = getCameraMode();

    get_Stitching_Out(&mode, &res);
    cmd_p1 = get_rec_cmd_queue_p1();
    cmd_p2 = get_rec_cmd_queue_p2();
    set_rec_cmd_queue_state(cmd_p1, recState);
    set_rec_cmd_queue_p1(++cmd_p1);

    Set_Smooth_Speed_Mode(getTimeLapseMode());
    sd_freesize = freesize;
    db_debug("setRecEn: state=%d lapse=%d freesize=%d\n", recState, time_lapse, freesize);

    if(recState == 0) {
        if(c_mode == CAMERA_MODE_TIMELAPSE || c_mode == CAMERA_MODE_TIMELAPSE_WDR) {			//TimeLapse
        	if(timelapse_enc == 1) {
            	if(res == 2) {
            		set_fpga_encode_type(2);					//CPU H264
            	}
            	else if(res == 7 || res == 12) {
            		set_fpga_encode_type(1);					//FPGA H264
        		    set_A2K_H264_Init(1);
            	}
        	}
        	else{
        		set_fpga_encode_type(0);
        		startSensorLog();
        	}
        }
        else {
        	set_A2K_do_Rec_Thm_CMD(1);
        	set_fpga_encode_type(0);
        }
    }else{
    	stopSensorLog();
    }
}

void setLidarStart_jni() {
	//setLidarStart
	return;
}

int checkCanTakePicture()
{
    unsigned long long now_time, choose_time;
    getChooseModeTime(&choose_time);
    get_current_usec(&now_time);
    if(now_time < choose_time) {                        //???????~
        setChooseModeTime(now_time);
        return 1;
    }
    else if((now_time - choose_time) < 1000000) {       //?????Ҧ???1???~?i????
        return 0;
    }
    else {
        return 1;
    }
}

int doTakePicture(int enable)
{
  	int ret = 1, num;
   	unsigned long long free = 0L, nowTime = 0L;
    char sd_path[128];

    //????DNA???~, ????????
   	/*if(GetDNACheckOk() != 1 && GetTestToolState() == -1) {
   		db_error("doTakePicture() dna check err!\n");
   		paintOLEDCheckError(1);
   		play_sound_flag = 11;	//playSound(11);
   		return -1;
   	}*/

    if(get_live360_state() == 0)
    	set_live360_state(-1);

   	if(Write_File_Error == 1) {
   		play_sound_flag = 11;	//playSound(11);
   		return -1;
   	}

    if(checksd() == 0) {
        getSdPath(&sd_path[0], sizeof(sd_path));
		setSdState(CheckSDcardState(&sd_path[0]));
        calSdFreeSize(&free);
//tmp        doPictureFinish(1);
        setPhotoLuxValue();
//tmp        num = GetSpacePhotoNum();
        Set_DataBin_FreeCount(num);
        writeUS360DataBin_flag = 1;
        if(getSdState() == 2)
            isNeedNewFreeCount = 1;
    } else {
        setSdState(0);
        db_debug("no SD Card\n");
    }
    set_timeout_start(0);                // ???s?p??
    if(getSdState() == 1 || enable == 3 || enable == 4 || enable == 5 || enable == 7 || enable == 8) {
//tmp    	if(getDebugLogSaveToSDCard() == 1){
//tmp    		systemlog.saveDebugLog();
//tmp        }
//tmp    	systemlog.addLog("info", System.currentTimeMillis(), "machine", "doTakePicture", "---");

        if(checkCanTakePicture())
        	ret = setCapEn(enable, getCaptureCnt(), getCaptureIntervalTime(), free);
        else
        	ret = -1;

        get_current_usec(&nowTime);
        if(ret < 0){
//tmp    		paintOLEDSnapshot(1);
    		play_sound_flag = 11;	//playSound(11);
        	setPowerSavingCapRecFinishTime(nowTime, POWER_SAVING_CMD_OVERTIME_5S);
        }else{
//tmp    		paintOLEDSnapshot(1);
    		play_sound_flag = 0;	//playSound(0);
        	setPowerSavingCapRecStartTime(nowTime);
        	setPowerSavingCapRecFinishTime(0, POWER_SAVING_CMD_OVERTIME_5S);
        	setLidarStart_jni();
        }

//        systemlog.addLog("info", System.currentTimeMillis(), "machine", "do Picture.", String.valueOf(enable));
    }

    return ret;
}

void startREC(void)
{
    /*if(GetDNACheckOk() != 1 && GetTestToolState() == -1) {
    	db_error("startREC() dna check err!\n");
    	paintOLEDCheckError(1);
    	play_sound_flag = 11;	//playSound(11);
    	return;
    }*/

    if(Write_File_Error == 1) {
    	play_sound_flag = 11;	//playSound(11);
    	return;
    }

    if(getRecordEn() == 0){                                    // ?}?lrec?ʧ@
        setRecordEn(1);
        setRecordCmdFlag(1);
        play_sound_flag = 4;	//playSound(4);

//        systemlog.addLog("info", System.currentTimeMillis(), "machine", "doRecordStart", "---");
    }
}

int stopREC(int debug)
{
	unsigned long long nowTime;

    if(getRecordEn() == 1){                                    // ????rec?ʧ@
        setRecordEn(0);
        setRecordCmdFlag(1);
        play_sound_flag = 5;	//playSound(5);

//tmp        systemlog.addLog("info", System.currentTimeMillis(), "machine", "doRecordStop", "---");
//tmp        if(getDebugLogSaveToSDCard() == 1){
//tmp      		systemlog.writeDebugLogFile();
//tmp        }
        get_current_usec(&nowTime);
        setPowerSavingCapRecFinishTime(nowTime, POWER_SAVING_CMD_OVERTIME_5S);

//tmp        ls_audioBuf.clear();
//tmp        ls_audioTS.clear();
//tmp        ls_readBufSize.clear();
        return 1;
    }
    else
    	return 0;
}

void doRecordVideo(int enable, int mode, int res, int time_lapse, int hdmi_state)
{
    int fps_tmp = 0;
    unsigned long long free = 0L, nowTime;
    char sd_path[128];

    if(checksd() == 0) {
        getSdPath(&sd_path[0], sizeof(sd_path));
		setSdState(CheckSDcardState(&sd_path[0]));
        calSdFreeSize(&free);
        isNeedNewFreeCount = 1;
    } else {
        setSdState(0);
        db_debug("doRecordVideo: no SD Card\n");
    }
    set_timeout_start(0);                // ???s?p??
    if(enable == 1 && check_power() == 1){
        // ?}?l???v

        if(get_live360_state() == 0)
        	set_live360_state(-1);

        if(getSdState() == 1) {
//            systemlog.addLog("info", System.currentTimeMillis(), "machine", "start REC.", "---");
            if(time_lapse == 0) {
//tmp                ChangeLedMode(1);
                //checkCpuSpeed();
                if(checkMicInterface() == 0){
                  	isRecording = 1;
                   	//micSource = 0;
                	//audio_record_thread_start_jni();
                }else{
                   	//micSource = 1;
                   	//new AudioRecordLineInThread().start();
                }
            }else{
                //changeLedByTime(1,time_lapse);
//tmp             	ChangeLedMode(1);
	        	//audioRate    = getMicRate();
        		//audioChannel = getMicChannel();
        		//audioBit     = getMicBit();
            }
            setRecEn(0, time_lapse, free, getTimelapseEncodeType());
//tmp            paintOLEDRecording(1);                    // rex+ 151221
            get_current_usec(&nowTime);
            setPowerSavingCapRecStartTime(nowTime);
            setPowerSavingCapRecFinishTime(0, 0);
        }
        else {
            usleep(200000);
            stopREC(14);
        }
    }
    else{
    	setRecEn(-1, time_lapse, free, getTimelapseEncodeType());
//        systemlog.addLog("info", System.currentTimeMillis(), "machine", "stop REC.", "---");
        if(time_lapse == 0 || isRecording == 1){
            isRecording = 0;
            //audio_record_thread_release_jni();
//tmp            int led_mode = GetLedControlMode();
//tmp            ChangeLedMode(led_mode);
            //checkCpuSpeed();
        }else{
            //isLedControl = false;
//tmp            SetIsLedControl(0);
        }
//tmp        paintOLEDRecording(0);                        // rex+ 151221
        get_current_usec(&nowTime);
        setPowerSavingCapRecFinishTime(nowTime, POWER_SAVING_CMD_OVERTIME_5S);
    }
}

void Ctrl_Rec_Cap(int c_mode, int cap_cnt, int rec_en) {
   	int ret = 1;
   	switch(c_mode) {
   	case 0: // Cap
   	case 3: // HDR
   	case 4: // RAW
   	case 5: // WDR
    case 6: // Night
    case 7: // Night WDR
    case 8:	// Sport
    case 9:	// Sport WDR
    case 12:	// Sport WDR
    case 13:	// remove
    case 14: // 3D Model
    	if(cap_cnt > 0){
    		SetBurstCount(cap_cnt);		//mBurstCount = cap_cnt;
            setCaptureIntervalTime(200);        // ?C??5?i
            set_timeout_start(2);
            ret = doTakePicture(2);
            if(ret < 0)
            	play_sound_flag = 11;	//playSound(11);
            else {
	            if     (cap_cnt ==  3) play_sound_flag = 1;	//playSound(1);        // mp_shutter_03.start();    //sound.play(au_shutter_03, 1, 1, 0, 0, 1);
	            else if(cap_cnt ==  5) play_sound_flag = 2;	//playSound(2);        // mp_shutter_05.start();    //sound.play(au_shutter_05, 1, 1, 0, 0, 1);
	            else if(cap_cnt == 10) play_sound_flag = 3;	//playSound(3);        // mp_shutter_10.start();    //sound.play(au_shutter_10, 1, 1, 0, 0, 1);
            }
        }
        else{
        	set_timeout_start(6);
            SetTakePicture(1);		//mTakePicture = 1;
            doTakePicture(1);
        }
    	break;
    case 1:	 //Rec
    case 2:	 //TimeLapse
    case 10: //Rec WDR
    case 11: //TimeLapse WDR
        if(rec_en == 0) startREC();
        else            stopREC(15);
        break;
    }
    //saveImageTimer = 100;// rex+ 181016,
}

/*
 * ???o???ӻP???v?????A
 * en[0]: cap_en        ???Ӫ??A
 * en[1]: rec_state        ???v???A
 */
void getSaveEnJNI(int *en)
{
    int cap_en;
    int cmd_p1 = Cap_Cmd_Queue.P1;
    int cmd_p2 = Cap_Cmd_Queue.P2;
    if(cmd_p1 == cap_en) cap_en = -2;           // normal
    else                 cap_en = 1;            // save
    *en     = cap_en;
    *(en+1) = get_rec_state();
}

/*void setSDPathStr(char* path, int len)
{
    memset(sd_path, 0, sizeof(sd_path) );
    memcpy(&sd_path[0], path, len);
    sd_path[len] = '\0';

//    set_dev_storage(sd_path);
}*/

/*
 *     mode: 0:?]?w?v??buf??idx 1:???o?{?b?v??buf??idx
 *     data: 0~3, ???Ӭ??{?b??idx+1
 */
int DecodeIdxRW(int mode, int data)
{
    if(mode == 0) decode_idx1 = data & 3;
    else            return decode_idx1;
    return -1;
}

/*
 *     int getdoResize(int *en, char *resize_path)
 *     jint Java_com_camera_simplewebcam_Main_getdoResize(JNIEnv* env, jobject thiz, jintArray en, jbyteArray resize_path)
 *     en: ???oResize?????T??
 *     en[0]: mode              Resize?Ҧ?(-1:None 0:Cap 1:Rec 2:Timelaspe, 3:HDR, 4:RAW)
 *     en[1]: cap_file_cnt      ???o?s?ɪ??p?ƭ?
 *     en[2]: rsv1              not use
 *     en[3]: rec_file_cnt
 *     en[4]: rsv2              not use
 *     en[5]: len               resize_path ????
 *
 *     resize_path: ???o?n??.thm???ɦW
 *
 * rex+ 180515, ?ǰewifi?p??
 */
void getdoResize(int *en, char *resize_path)
{
    int i;
    int len;
    int rp1, rp2;

    rp1 = doResize_buf.P1;
    rp2 = doResize_buf.P2;
    if(rp1 != rp2) {
        len = strlen(doResize_buf.cmd[rp2].doResizePath);
        if(len > 0){
            memcpy(resize_path, &doResize_buf.cmd[rp2].doResizePath[0], len);
        }
        //db_debug("getdoResize: rp2=%d mode=%d path=%s\n", rp2, doResize_buf.cmd[rp2].mode, resize_path);

        *en     = doResize_buf.cmd[rp2].mode;
        *(en+1) = doResize_buf.cmd[rp2].cap_file_cnt;
        *(en+2) = doResize_buf.cmd[rp2].rsv1;
        *(en+3) = doResize_buf.cmd[rp2].rec_file_cnt;
        *(en+4) = doResize_buf.cmd[rp2].rsv2;
        *(en+5) = len;

        memset(&doResize_buf.cmd[rp2], 0, sizeof(doResize_struc) );
        doResize_buf.cmd[rp2].mode = -1;
        doResize_buf.P2++;
        if(doResize_buf.P2 >= DORESIZE_BUF_MAX) doResize_buf.P2 = 0;
    }
    else {
        *en     = -1;
        *(en+1) = 0;
        *(en+2) = 0;
        *(en+3) = 0;
        *(en+4) = 0;
        *(en+5) = 0;
    }
}

/*
 * rex+ 180515
 */
void writeThmFile(int mode, char *buf, int size)
{
    if(mode >= 0 && mode < 6){
        if(iconThmFile[mode] == NULL){
            iconThmFile[mode] = malloc(size);
            if(iconThmFile[mode] != NULL){
                memcpy(iconThmFile[mode], buf, size);
                iconThmSize[mode] = size;
            }
        }
    }
}

/*
 *     void getPath()
 *     void Java_com_camera_simplewebcam_Main_getPath( JNIEnv* env, jobject thiz)
 *    ?]?w THMPath & DirPath
 */
void getPath()
{
    char sd_path[128];
    char path[128];
    getSdPath(&sd_path[0], sizeof(sd_path));
    maek_save_file_path(3, path, sd_path, mSSID, 0);
}

int getPNGPath(char *buf)
{
    char sd_path[128];
    char path[128];
    getSdPath(&sd_path[0], sizeof(sd_path));
    maek_save_file_path(12, path, sd_path, mSSID, cap_file_cnt-1);
    db_debug("getPNGPath: path=%s\n", path);
    int len = strlen(path);
    memcpy(buf, path, len);
}

int getCaptureStep() {
	return Get_F_Cmd_In_Capture_Step();
}

int getCaptureFileCnt() {
	return cap_file_cnt;
}

/*
 * rex+ 151229
 *   ?]?w?ɦWmSSID?Ѽ?
 * */
/*void setFileSSID(char *ssid, char *pwd)
{
    int slen = 7;    // US_0000
    int plen = 8;    // 88888888
    memcpy(&mSSID[0], ssid, slen);
    mSSID[slen] = '\0';
    memcpy(&mPwd[0], pwd, plen);
    mSSID[plen] = '\0';
    set_T_mSSID(mSSID);             // test.c
}*/

/*
 * rex+ 151231
 *   ?]?wAndroid GUI?O?_????
 *   enable: 0:off 1:on
 * */
void setGUI(char *enable)
{

    int status;
    int fd = open("/dev/us360_gui", O_RDWR);
    if(fd >= 0){
        if(*enable == 0) status = ioctl(fd, IOCTL_GUI_OFF, 0);
        else            status = ioctl(fd, IOCTL_GUI_ON, 0);
        close(fd);
        db_debug("setGUI: status=%d\n", status);
    }

    /*
    FILE* fp;
    struct stat sti;
    if(enable == 0){
        if(stat("/mnt/sdcard/show_gui", &sti) == 0) {
            remove("/mnt/sdcard/show_gui");             // ?R???ɮ?
        }
    }
    else{
        if(stat("/mnt/sdcard/show_gui", &sti) != 0) {
            fp = fopen("/mnt/sdcard/show_gui", "wb");   // ?إ??ɮ?
            if(fp >= 0){
                fwrite(&fp, 4, 1, fp);
                fclose(fp);
            }
        }
    }
    */
}

//extern FILE *out;
void SDCardEject(void)
{
	int state = get_rec_state();
    if(state != -2) {
		state = -2;
        set_rec_state(state);
        set_A2K_Rec_State_CMD(state);
        if(pcm_buf.state == 1)
            pcm_buf.state = -1;
//tmp        if(out) {
//tmp            fclose(out);
//tmp            out = NULL;
//tmp        }
    }
}

void set_callback_func(char *jpeg_func, char *h264_func)
{
    if(callback_func.read_jpeg == NULL)
        callback_func.read_jpeg = jpeg_func;
    if(callback_func.read_h264 == NULL)
        callback_func.read_h264 = h264_func;
}

/*void enable_debug_message(int en)
{
    enable_debug_flag = en;
}*/

/*
 * check ethernet connect ?
 */
int CheckEthernetConnect(void)
{
    char path[128];
    struct stat sti;
    sprintf(path, "/sys/class/net/eth0\0");
    if(stat(path, &sti) != 0) {
        //db_error("Ethernet Desconnect !\n");
        return -1;
    }
    else
        //db_debug("Ethernet Connect !\n");
    return 0;
}

unsigned long long get_sd_free_size(char *path)
{
    struct statfs sfs;
    unsigned long long size_tmp;
    if(statfs(path, &sfs) == 0) {
        size_tmp = sfs.f_bavail * sfs.f_bsize;
        db_debug("get_sd_free_size() %lld\n", size_tmp);
    }
    else
        db_error("get_sd_free_size() statfs err\n");
    return size_tmp;
}

/*
 * ?]?w wifi channel
 */
void WriteWifiChannel(int channel)
{
    FILE* fp=NULL;
    fp = fopen("/proc/spi/wifichannel", "wb");
    if(fp != NULL){
        char str[2];
        sprintf(str, "%02d", channel);
        int len = strlen(str);
        db_debug("WriteWifiChannel : len = %d, str = %s\n", len, str);
        fwrite(str, len, 1, fp);
        //db_debug("WriteWifiChannel = %d\n", channel);
        fclose(fp);
    }
    else{
        db_error("WriteWifiChannel fopen error!\n");
    }
}

/*
 * ?]?w wifi?̤j?s?u??
 */
void writeWifiMaxLink(int maxLink)
{
    FILE* fp=NULL;
    fp = fopen("/proc/spi/wifimaxnumsta", "wb");
    if(fp != NULL){
        char str[2];
        sprintf(str, "%02d", maxLink);
        int len = strlen(str);
        db_debug("WriteWifiMaxLink : len = %d, str = %s\n", len, str);
        fwrite(str, len, 1, fp);
        //db_debug("WriteWifiChannel = %d\n", channel);
        fclose(fp);
    }
    else{
        db_error("WriteWifiMaxLink fopen error!\n");
    }
}

unsigned long GetFileLength(char *fileName)
{
    struct stat sti;
    int i = stat(fileName, &sti);

    if(i != 0) return 0;
    else       return sti.st_size;
}

/*
 * ?]?w?w???٬O?n??????
 * mode: 0:?]???n??  1:?]???w??
 */
//void setViewMode(int mode)
//{
//    int i;
//
//    mHWDecode = mode;
//    decode_idx0 = 0;
//    decode_idx1 = 0;
//    memset(&dec_buf, 0, sizeof(dec_buf));    // max+ ?Ѥ??^?w?? ?e???????s
//}

int initCedarxResolution_flag;
/*
 * Init Cedarx
 * ?]?winitCedarxResolution_flag??1, thread_dec()?~??initCedarxResolution()
 */
void setInitCedarxResolutionFlag(void)
{
    initCedarxResolution_flag = 1;
}

//int mHdmiConnected=1;
/*
 *     ???o HDMI ?s?????A
 *     0: ???s?? 1:?w?s??
 */
int getHdmiConnected(void)
{
    return hdmi_get_hpd_status();
}

int CopyAudioRecord(char *abuf, int alen, int afreq, int abit, int achannel)
{
//#ifdef NEW_AUDIO_RECORD
    int tmp, pcm_tmp;
    static unsigned long long curTime, lstTime=0, runTime;
    unsigned p1, p2;
    int freq = get_ISP_AEG_EP_IMX222_Freq();
    int size=0, aac_len=0;
    char *buf;

    if(mic_is_alive == 0){
		if(pcm_buf.state != -2) {
			p1 = pcm_buf.P1;
			p2 = pcm_buf.P2;
			if(pcm_buf.state == 0) {
				if(freq == 0) pcm_buf.fps = 300;
				else          pcm_buf.fps = 250;
				pcm_buf.size = afreq * abit * achannel / 8 / (pcm_buf.fps/10);
				audio_rate = afreq;

//tmp				if(muxer_type == MUXER_TYPE_TS || Live360_Buf.state != -2) {
//tmp					AacEnc_Init(afreq, achannel, abit);
//tmp				}
			}

//tmp			if(muxer_type == MUXER_TYPE_TS || Live360_Buf.state != -2) {
//tmp				aac_len = AacEnc_Proc(abuf, alen, &aac_enc_buf[0]);
//tmp				size = aac_len;
//tmp				buf = &aac_enc_buf[0];
//tmp			}
//tmp			else 
			{
				size = alen;
				buf = abuf;
			}

			if(pcm_buf.fps >= 50) {
				if(pcm_buf.state == -1) {
					pcm_buf.state = -2;
				} else {
					if(pcm_buf.state == 0) pcm_buf.state = 1;
				}
			}

			if(buf != NULL && pcm_buf.fps >= 50 && size > 0) {
				//pcm_buf.size = size;
//				if(freq == 0) pcm_buf.size = 2940;
//				else          pcm_buf.size = 3528;
				pcm_tmp = p1 + size;    //pcm_buf.size;
				if(pcm_tmp < PCM_BUF_MAX) {
					if(p2 > p1 && pcm_tmp > p2) {
						pcm_buf.jump = 1;
						db_error("pcm_thread() 00-1 pcm_buf.jump == 1 P1 %d P2 %d size %d\n", p1, p2, pcm_buf.size);
					}
					//else if(pcm_buf.P1 > pcm_buf.P2 && tmp >= PCM_BUF_MAX && (tmp - PCM_BUF_MAX) > pcm_buf.P2) {
					//    pcm_buf.jump = 1;
					//    db_error("pcm_thread() 00-2 pcm_buf.jump == 1\n");
					//}
					else {
						memcpy(&pcm_buf.buf[p1], &buf[0], size);
						pcm_buf.P1 = pcm_tmp;
						pcm_buf.jump = 0;
					}
				}
				else {
					if(p2 > p1 && pcm_tmp > p2) {
						pcm_buf.jump = 1;
						db_error("pcm_thread() 01-1 pcm_buf.jump == 1\n");
					}
					else if(p1 > p2 && pcm_tmp >= PCM_BUF_MAX && (pcm_tmp - PCM_BUF_MAX) > p2) {
						pcm_buf.jump = 1;
						db_error("pcm_thread() 01-2 pcm_buf.jump == 1\n");
					}
					else {
						memcpy(&pcm_buf.buf[p1], &buf[0], PCM_BUF_MAX - p1);
						memcpy(&pcm_buf.buf[0], &buf[PCM_BUF_MAX - p1], pcm_tmp - PCM_BUF_MAX);
						pcm_buf.P1 = (pcm_tmp - PCM_BUF_MAX);
						pcm_buf.jump = 0;
					}
				}
			}

		}    // if(pcm_buf.state != -2)
		else return -1;
	}
//#endif
    return 0;
}

void copy_to_live_264_buff(char *buf, int size, int keyf)
{
	int idx;
    char *addr;

    if((live264_offset + size) > sizeof(live264_buff))
        live264_offset = 0;

    if(live264_idx1 < (LIVE264_MAX_BUFF-1)) idx = live264_idx1 + 1;
    else                                    idx = 0;
    if(getIframe == 1 && keyf == 1){
        getIframe = 0;
    }
    if(getIframe == 0){
        if(idx != live264_idx2){
            addr = (char *)&live264_buff[live264_offset];
            memcpy(addr, buf, size);
            live264_cmd[live264_idx1].addr = addr;
            live264_cmd[live264_idx1].ofst = live264_offset;
            live264_cmd[live264_idx1].size = size;
            live264_cmd[live264_idx1].keyf = keyf;
            live264_offset += size;

            usleep(1);		//?קK?g?J?PŪ?????Ʈɶ??ӱ????ɭPŪ?????~
            if(live264_idx1 < (LIVE264_MAX_BUFF-1)) live264_idx1 ++;
            else                                     live264_idx1 = 0;
        }
        else{
            getIframe = 1;
            //db_debug("copy_to_live_264_buff: idx1=%d idx2=%d\n", live264_idx1, live264_idx2);
            live264_idx1 = live264_idx2;
        }
    }
    //db_debug("copy_to_live_264_buff: idx1=%d idx2=%d key=%d\n", live264_idx1, live264_idx2, keyf);
}

int getLive264buff(char *buf, int max, char *key)
{
    int size=0, ofst=0; char *addr;

    if(live264_idx1 != live264_idx2){
        addr = live264_cmd[live264_idx2].addr;
        ofst = live264_cmd[live264_idx2].ofst;
        size = live264_cmd[live264_idx2].size;
        *key = live264_cmd[live264_idx2].keyf;
        if(max >= size){
            memcpy(buf, addr, size);
        }
        if(live264_idx2 < (LIVE264_MAX_BUFF-1)) live264_idx2 ++;
        else                                     live264_idx2 = 0;
    }

    return size;
}

int get_copy_h264_to_rec_en(int c_mode, int res, int enc_type)
{
	if(enc_type == FPGA_VIDEO_ENCODE_TYPE_JPEG) {				//JPEG
		if(c_mode == CAMERA_MODE_TIMELAPSE || c_mode == CAMERA_MODE_TIMELAPSE_WDR)
			return 0;
		else
			return 1;
	}
	else if(enc_type == FPGA_VIDEO_ENCODE_TYPE_FPGA_H264) {		//TimeLaps FPGA.H264
		if(c_mode == CAMERA_MODE_TIMELAPSE || c_mode == CAMERA_MODE_TIMELAPSE_WDR)
			return 1;
	}
	else if(enc_type == FPGA_VIDEO_ENCODE_TYPE_CPU_H264) {		//TimeLaps CPU.H264
		if(c_mode == CAMERA_MODE_TIMELAPSE || c_mode == CAMERA_MODE_TIMELAPSE_WDR)
			return 1;
	}
	return 0;
}

void copy_H264_buf(char* buf, int size, int key_f, int width, int height,
        char *sps, int sps_len, char *pps, int pps_len, int offset, int rtmp_sw)
{
    int mode, res;
    int size_tmp=0;
    int fpga_enc_type = get_fpga_encode_type();

    get_Stitching_Out(&mode, &res);

    //db_debug("copy_H264_buf: size=%d w=%d h=%d\n", size, width, height);

    // live555 process..
    if(key_f == 1)    waitIframe = 0;
    if(waitIframe == 0){
//tmp        set_h264_rtsp_parameter(key_f, width, height);
//tmp        if(copy_to_h264_buf_queue((char *)(buf+offset), size-offset) < 0){
//tmp            waitIframe = 1;
//tmp            clean_h264_buf_queue();
//tmp        }
    }
    copy_to_live_264_buff(buf, size, key_f);

    // max+ ?sH.264  ???N???Ʀs??BUF??, ?? rec_thread ?s??
    if(get_copy_h264_to_rec_en(getCameraMode(), res, fpga_enc_type) == 1) {
		if(key_f == 1 || rtmp_sw == 0) size_tmp = sps_len+pps_len+12;
		else           				   size_tmp = 4;
		//size_tmp = sps_len+pps_len+12;	//ios mp4 ?ݥh?? sps & pps & start code
		copy_to_rec_buf(&buf[size_tmp], (size-size_tmp), key_f, sps, sps_len, pps, pps_len, fpga_enc_type, 0);
    }

    if(callback_func.read_h264 != NULL)
        callback_func.read_h264(buf, size, key_f);
}

/*
 * ???oUVC???A
 * return 0=???`?A1=???`
 */
int GetUVCfd()
{
    return uvc_err_flag;
}

/*
 * check wifi interface, weber+170428
 */
int checkWifiInterface(void)
{
    char path[128];
    struct stat sti;
    sprintf(path, "/sys/class/net/wlan0\0");
    if(stat(path, &sti) != 0) {
        //db_error("Ethernet Desconnect !\n");
        return 0;
    }
    else
        //db_debug("Ethernet Connect !\n");
    return 1;
}

/*
 * weber+170623
 * check mic interface (pcmC2D0c)
 */
int checkMicInterface(){
    char path[128];
    struct stat sti;
    int freq = get_ISP_AEG_EP_IMX222_Freq();
    int M_Mode, S_Mode;
    sprintf(path, "/dev/snd/pcmC2D0c\0");
    if(stat(path, &sti) != 0) {
          mic_is_alive = 0;
//          audio_rate = 44100;
    }
    else
          mic_is_alive = 1;

    Get_M_Mode(&M_Mode, &S_Mode);
    pcm_buf_init(freq, M_Mode, mic_is_alive, 0);
    db_debug("checkMicInterface() mic_is_alive=%d\n", mic_is_alive);
    return mic_is_alive;
}
int getMicInterface(){
    char path[128];
    struct stat sti;
    sprintf(path, "/dev/snd/pcmC2D0c\0");
    if(stat(path, &sti) != 0) {
          return 0;
    }
    return 1;
}

int get_mic_is_alive() {
	return mic_is_alive;
}

//extern void reset_JPEG_Quality(int quality);       // spi_cmd.c
/*
 *    FPGA ?}??
 *    ctrl_pow: 0:?}??  1:????
 */
void setFPGACtrlPower(int ctrl_pow)
{
    FPGA_Ctrl_Power = ctrl_pow;
    if(FPGA_Ctrl_Power == 0) {
        uvc_thread_en = 1;
        pcm_thread_en = 1;
        st_thread_en  = 1;
		set_rec_thread_en(1);
        set_live360_thread_en(1);
        reset_JPEG_Quality(F2_J_QUALITY_DEFAULT);
    }
    if(ctrl_pow == 0) db_debug("setFPGACtrlPower: Power ON\n");
    else              db_debug("setFPGACtrlPower: Power OFF\n");
}

void set_rec_proc_en(int en) {
	rec_proc_en = en;
}

void pollWatchDog(){
    unsigned addr = 0x004;
    unsigned Data[2];
    int ret,i;


    if(uvc_thread_en == 0)    // standby
        uvc_proc_en = 1;
    if(get_rec_thread_en() == 0)    // standby
        rec_proc_en = 1;

    if(uvc_proc_en == 1){
        uvc_proc_en = 0;
        uvc_proc_count = 0;
    }
    else{
        uvc_proc_count++;
        if(uvc_proc_count > 30){
            if(need_boot == 0)
                need_boot = 1;
        }
    }
    if(rec_proc_en == 1){
        rec_proc_en = 0;
        rec_proc_count = 0;
    }
    else{
        rec_proc_count++;
        if(rec_proc_count > 30){
            if(need_boot == 0)
                need_boot = 1;
        }
    }
    if(oled_proc_en == 1){
        oled_proc_en = 0;
        oled_proc_count = 0;
    }
    else{
        oled_proc_count++;
        if(oled_proc_count > 30){
            if(need_boot == 0)
                need_boot = 1;
        }
    }
    if(dec_proc_en == 1){
        dec_proc_en = 0;
        dec_proc_count = 0;
    }
    else{
        dec_proc_count++;
        if(dec_proc_count > 30){
            if(need_boot == 0)
                need_boot = 1;
        }
    }

    db_debug("pollWatchDog: %d - %d %d %d %d\n", need_boot, uvc_proc_count, rec_proc_count, oled_proc_count, dec_proc_count);

    //if(need_boot == 0){
        //spi_read_io_porcess(addr);
        //Data[0] = 0x004;
        //Data[1] = 0;
        //ret = SPI_Write_IO(0x9, &Data[0], 8);       // watchdog
    //}
}

void save_sensor_data(void){

	int yaw = 0;
//tmp	int azimuth = getBmm050_azimuth();
	int azimuth = 0;

	float bmaData[3];
//tmp	getBma2x2_orientation_data(bmaData, 0);

	if(getOneAzimuth == 999){
//tmp		yaw = getBmg160_yaw() / 100;
		getOneAzimuth = azimuth - yaw;
		Set_Jpeg_Sensor_Head(0, azimuth);
	}else{
//tmp		yaw = getBmg160_yaw() / 100 + getOneAzimuth;
		if(yaw < 0){
			yaw = 360 + yaw;
		}else{
			yaw = yaw % 360;
		}
		Set_Jpeg_Sensor_Head(0, yaw);
	}

	Set_Jpeg_Sensor_Pitch(0, bmaData[1]);
//tmp	if(get_CameraPositionMode() == 1 && getBma2x2_enable() == 1) {			//?˸m
	if(get_CameraPositionMode() == 1 /*&& getBma2x2_enable() == 1*/) {			//?˸m
		if(bmaData[2] >= 0)
			Set_Jpeg_Sensor_Roll(0, (bmaData[2] - 180) );
		else
			Set_Jpeg_Sensor_Roll(0, (bmaData[2] + 180) );
	}
	else					//???m
		Set_Jpeg_Sensor_Roll(0, bmaData[2]);
}

void set_first_Azimuth(int val) {
	getOneAzimuth = val;
}

int read_sensor_data(void) {
	if(getCtrlCameraPositionMode() == 0){		//????
		return -1;
	}else{										//Auto
		return 0;
	}
	return 0;
}

/*void setSoftVersion(char *ver)
{
	set_A2K_Softwave_Version(ver);
}*/

void setModelName(char *ver)
{
	set_A2K_Model(ver);
}

void setImgReadyFlag(int val){
	img_ready_flag = val;
}

int getImgReadyFlag() {
	return img_ready_flag;
}

/*
 * ?j?? FPGA ?e USB, ?] FPGA ???????b?e USB ???e?ݭn???? JPEG
 */
void Add_Cap_Lose(int i_page)
{
	unsigned jpeg_sel, addr, Data[2];
	switch(i_page % 10) {
	case 0: addr = (USB_STM1_P0_S_BUF_ADDR >> 5); jpeg_sel = 2; break;		//JPEG0
	case 1: addr = (USB_STM1_P0_S_BUF_ADDR >> 5); jpeg_sel = 2; break;		//JPEG0
	case 2: addr = (USB_STM1_P1_S_BUF_ADDR >> 5); jpeg_sel = 6; break;		//JPEG1
	case 3: addr = (USB_STM1_P1_S_BUF_ADDR >> 5); jpeg_sel = 6; break;		//JPEG1
	}
    Data[0] = 0xB00;
    Data[1] = (0xF << 28) | (jpeg_sel << 24) | addr;
    SPI_Write_IO_S2(0x9, &Data[0], 8);
db_debug("Add_Cap_Lose() 00 i_page=%d jpeg_sel=%d addr=0x%x Data[0]=0x%x Data[1]=0x%x\n", i_page, jpeg_sel, (addr << 5), Data[0], Data[1]);
}

/*
 * ?P?_SDCard???A?O?_?????ۮe
 */
int GetSDState(void)
{
    DIR *dir = opendir("/dev/block/vold");						//SD?d?榡???ۮe, ?|???v?????D
    struct dirent *ent;
    int disk_cnt=0, public_cnt=0;

    if(dir == NULL) {
    	db_error("GetSDState() dir == NULL\n");
        return -1;
    }

    while(1)
    {
        ent = readdir(dir);
        if(ent <= 0)
            break;

        if( (strcmp(".", ent->d_name) == 0) || (strcmp("..", ent->d_name) == 0) )
            continue;

        if(strstr(ent->d_name, "disk") != NULL)
        	disk_cnt++;

        if(strstr(ent->d_name, "public") != NULL)
        	public_cnt++;
    }

    if(disk_cnt >= 1)
    	return 3;

    return 0;
}

void settingHDR7P(int manual, int frame_cnt, int ae_scale, int strength)
{
	setting_HDR7P(manual, frame_cnt,ae_scale,strength);
}
void settingRemovalHDR(int manual, int ae_scale, int strength)
{
	setting_Removal_HDR(manual, ae_scale, strength);
}
void settingAEB(int frame_cnt, int ae_scale)
{
	setting_AEB(frame_cnt,ae_scale);
}
int getCaptureEstimatedTime()
{
	return getCaptureAddTime();
}
int doGetCaptureEpTime()
{
	return getCaptureEpTime();
}

void save_Removal_bin_file(char fname, char *fptr, int size)
{
        FILE *file = NULL;
        int fp2fd = 0; char fstr[128];
        
        sprintf(fstr,"/mnt/sdcard/US360/%s\0",fname);
        file = fopen(fstr, "wb");
        if(file != NULL) {
            fwrite(fptr, size, 1, file);
            fflush(file);
            fp2fd = fileno(file);
            fsync(fp2fd);

            fclose(file);
            close(fp2fd);
            db_debug("save_Removal_bin_file: size=%d\n", size);
        }
}
int readCaptureDCnt(void) {
	return read_F_Com_In_Capture_D_Cnt();
}

int readCapturePrepareTime(void){
	return getCapturePrepareTime();
}

void startSensorLog(){
    char sd_path[128];
	if(sensorLogEnable == 0){
		return;
	}
	int isid = 0;
	char csid[5]={0,0,0,0,0};
	csid[0] = mSSID[3];                          // ?? US_0000 ?? 0000
	csid[1] = mSSID[4];
	csid[2] = mSSID[5];
	csid[3] = mSSID[6];
	if(csid[0] >= '0' && csid[0] <= '9' &&
	   csid[1] >= '0' && csid[1] <= '9' &&
	   csid[2] >= '0' && csid[2] <= '9' &&
	   csid[3] >= '0' && csid[3] <= '9')
	{
		isid = atoi(csid);
	}
    getSdPath(&sd_path[0], sizeof(sd_path));
	sprintf(sensorSavePath, "%s/DCIM/%s/T%04d_%04d/sensorLog.txt\0", sd_path, mSSID, isid, cap_file_cnt);
	db_debug("bmx055Log : %s\n",sensorSavePath);
	sensorState = 1;
}
void stopSensorLog(){
	sensorState = 0;
	if(sensorFlag > 0){
		saveSensorData();
	}
}
int getSensorState(){
	return sensorState;
}

void addSensorData(unsigned long long curTime){
	static double bmaData[3],bmgData[3];
	static int bmmData[3];
//tmp	getBma2x2_accel_speed(bmaData);
//tmp	getBmg160_gyroscope_rad(bmgData);
//tmp	getBmm050_magnetometer(bmmData);
	unsigned long long time;
	Get_F_JPEG_Header_Now_Time(&time);
	db_debug("bmx055: %llu,%lld,%lf,%lf,%lf,%lf,%lf,%lf,%d,%d,%d\n",
		curTime / 1000,time,
		bmgData[0],bmgData[1],bmgData[2],
		bmaData[0],bmaData[1],bmaData[2],
		bmmData[0],bmmData[1],bmmData[2]);
	sprintf(sensorLog[sensorFlag], "%llu,%lld,%lf,%lf,%lf,%lf,%lf,%lf,%d,%d,%d\r\n\0",
			curTime / 1000,time,
			bmgData[0],bmgData[1],bmgData[2],
			bmaData[0],bmaData[1],bmaData[2],
			bmmData[0],bmmData[1],bmmData[2]);
	//db_debug("bmx055Log len: %d\n",strlen(sensorLog[sensorFlag]));
	sensorFlag++;
	if(sensorFlag >= 200){
		saveSensorData();
	}
}

int saveSensorData()
{

    int i,x;
    int size;
    FILE *SaveFile = NULL;
    int fp2fd = 0;

    SaveFile = fopen(sensorSavePath, "a+b");
    if(SaveFile == NULL) {
        db_error("Save US360Config.bin err!\n");
        return -1;
    }

    for(x = 0; x < sensorFlag;x++){
    	size = strlen(sensorLog[x]);
    	fwrite(&sensorLog[x], size, 1, SaveFile);
    }

    fflush(SaveFile);
    fp2fd = fileno(SaveFile);
    fsync(fp2fd);

    //if(SaveFile)
    {
        fclose(SaveFile);
        close(fp2fd);
        SaveFile = NULL;
    }

	sensorFlag = 0;
    return 0;
}
void setSensorLogEnable(int enable){
	sensorLogEnable = enable;
//tmp	setBmg160_radCal(sensorLogEnable);
}

int GetSaturationValue(int value) {
	return ( (value + 7) * 1000 / 7);
}

int GetHDRDefaultParameter(int mode, int idx) {
	return HDR_Default_Parameter[mode][idx];
}

void Setting_HDR7P_Proc(int manual, int ev_mode)
{
	int number = 5, increment = 10, strength = 60;
    if(manual == 2) {				//Auto
      	number    = Get_DataBin_HdrNumber();
       	increment = Get_DataBin_HdrIncrement();
       	strength  = Get_DataBin_HdrAutoStrength();
    }
    else if(manual == 1) {			//???ʶ}
       	number    = Get_DataBin_HdrNumber();		//wifiSerThe.mHdrEvModeNumber;
       	increment = Get_DataBin_HdrIncrement();	//wifiSerThe.mHdrEvModeIncrement;
       	strength  = Get_DataBin_HdrStrength();	//wifiSerThe.mHdrEvModeStrength;
    }
    else if(manual == 0) {		//??????
       	if(ev_mode == 2) {
       		number    = HDR_Default_Parameter[0][0];
       		increment = HDR_Default_Parameter[0][1];
       		strength  = HDR_Default_Parameter[0][2];
       	}
       	else if(ev_mode == 4) {
       		number    = HDR_Default_Parameter[1][0];
       		increment = HDR_Default_Parameter[1][1];
       		strength  = HDR_Default_Parameter[1][2];
       	}
       	else if(ev_mode == 8) {
       		number    = HDR_Default_Parameter[2][0];
       		increment = HDR_Default_Parameter[2][1];
       		strength  = HDR_Default_Parameter[2][2];
       	}
    }
    setting_HDR7P(manual, number, increment * 2, strength);
}

void Setting_RemovalHDR_Proc(int mode)
{
   	int increment = 10, strength = 60;
   	if(mode == 1) {			//Auto
   		increment = Get_DataBin_RemoveHdrIncrement();
   		strength  = Get_DataBin_RemoveHdrAutoStrength();
   	}
   	else {
   		increment = Get_DataBin_RemoveHdrIncrement();
   		strength  = Get_DataBin_RemoveHdrStrength();
   	}
   	setting_Removal_HDR(mode, increment * 2, strength);
}


//--------------------------------------------------------------------------

void Show_Now_Mode_Message(int mode, int res, int fps, int live_rec) {
    char message[64];
    char mode_st[16]; 
    char res_st[16];
    char fps_st[16];
    char live_rec_st[16];
        
    switch(mode) {
    case 0: sprintf(mode_st, "Global\0"); break;
    case 1: sprintf(mode_st, "Front\0");  break;
    case 2: sprintf(mode_st, "360\0");    break;
    case 3: sprintf(mode_st, "240\0");    break;
    case 4: sprintf(mode_st, "180\0");    break;
    case 5: sprintf(mode_st, "Split4\0"); break;
    case 6: sprintf(mode_st, "PIP\0");    break;
    }
        
    switch(res) {
    case 0: break;
    case 1: sprintf(res_st, "12K\0");      break;
    case 2: sprintf(res_st, "4K\0");       break;
    case 7: sprintf(res_st, "8K\0");       break;
    case 8: sprintf(res_st, "10K\0");      break;
    case 12: sprintf(res_st, "6K\0");      break;
    case 13: sprintf(res_st, "3K\0");      break;
    case 14: sprintf(res_st, "2K\0");      break;
    }
        
    sprintf(fps_st, "%d\0", (fps/10) );
        
    if(live_rec == 0) sprintf(live_rec_st, "live\0");
    else              sprintf(live_rec_st, "rec\0");
          
	sprintf(message, "%s, %s, %s fps, %s", mode_st, res_st, fps_st, live_rec_st);
//tmp    textModeMessage.setText(message);
}

int errnoexit(const char *s)
{
    db_error("%s error %d, %s\n", s, errno, strerror (errno));
    return ERROR_LOCAL;
}

//#define TEST_SPI_IO_RW		1
//#define TEST_SPI_DDR_RW		1
//#define TEST_QSPI			1
#define TEST_MIPI			1

pthread_t thread_test_id;
pthread_mutex_t mut_test_buf;
int test_thread_en = 1;

void Write_Test_SPI_Cmd() {
	int i, Addr = 0;
	unsigned Data[896], rBuf[128];		//QSPI?ܤֻݭn?W?L&????512Byte 
	unsigned short Data_c[1792];
	
#if defined(TEST_SPI_IO_RW)
	Addr = 0x3F0;
	//spi write io
    Data[0] = Addr;
    Data[1] = 0xABCD;
    SPI_Write_IO_S2(0x9, (int *) &Data[0], 8);
	//spi read io
	memset(&Data[0], 0, sizeof(Data) );
	spi_read_io_porcess_S2(Addr, (int *) &Data[0], 4);
	db_debug("Write_Test_SPI_Cmd: value=0x%x", Data[0]);
#else
	//qspi write ddr
	memset(&Data[0], 0, sizeof(Data) );
	memset(&rBuf[0], 0, sizeof(rBuf) );
	memset(&Data_c[0], 0, sizeof(Data_c) );
	for(i = 0; i < 1792; i++) {
		Data_c[i] = i;
	}

  #if defined(TEST_MIPI)	 
	for(i = 0; i < 1920; i++) {	
		Addr = MTX_S_ADDR + (i << 16);	// 0x10000: DDR?@?C
		Data_c[0] = i;
		//ua360_qspi_ddr_write(Addr, (int *) &Data_c[0], sizeof(Data_c) );
		ua360_spi_ddr_write(Addr, (int *) &Data_c[0], sizeof(Data_c) );
	}
  #else
	Addr = 0x10000000;
   #if defined(TEST_QSPI)
	ua360_qspi_ddr_write(Addr, (int *) &Data[0], sizeof(Data) );
   #else
	ua360_spi_ddr_write(Addr, (int *) &Data[0], sizeof(Data) );
   #endif	// defined(TEST_QSPI)
   
//	ua360_spi_ddr_read(Addr, (int *) &rBuf[0],  sizeof(rBuf), 2, 0);
//	for(i = 0; i < 8; i++)
//		db_debug("Write_Test_SPI_Cmd: rBuf[%d]=0x%x", i, rBuf[i]);
  #endif	// defined(TEST_MIPI)
  
#endif	// defined(TEST_SPI_IO_RW)
}

enum time_state {
	TIME_STATE_STOP = -1,
    TIME_STATE_NONE = 0,
    TIME_STATE_SEC  = 3		//delay 3s ?~???? QSPI, ?קK?}???????ɭP flash error
};

void *test_thread(void)
{
    static unsigned long long curTime, lstTime=0, runTime, show_img_ok=1;
    nice(-6);    // ?վ?thread?u???v

	int testFlag = TIME_STATE_NONE;
	
    while(test_thread_en)
    {
        get_current_usec(&curTime);
        if(curTime < lstTime) lstTime = curTime;    // rex+ 151229, ?????ҥ~???~
        else if((curTime - lstTime) >= 1000000){
            db_debug("test_thread: runTime=%lld", runTime);
            lstTime = curTime;
			if(testFlag >= TIME_STATE_NONE) testFlag++;
        }

		if(testFlag >= TIME_STATE_SEC) {
			Write_Test_SPI_Cmd();
#if defined(TEST_MIPI)
			as3_reg_addr_init();
			testFlag = TIME_STATE_STOP;
db_debug("max+ test_thread: do mipi cmd!");			
			break;
#else
			testFlag = TIME_STATE_SEC;
#endif			
		}

        get_current_usec(&runTime);
        runTime -= curTime;
        if(runTime < 1000000){
            usleep(1000000 - runTime);
            get_current_usec(&runTime);
            runTime -= curTime;
        }
    }	
}

void create_test_thread() {
    pthread_mutex_init(&mut_test_buf, NULL);
    if(pthread_create(&thread_test_id, NULL, test_thread, NULL) != 0) {
        db_error("Create test_thread fail !");
    }	
}

void us360_init() {
	DDR_Reset();
	recoder_thread_init();
	create_test_thread();
}
