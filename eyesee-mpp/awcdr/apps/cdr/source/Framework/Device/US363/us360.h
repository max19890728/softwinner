/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __US360_H__
#define __US360_H__

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CLEAR(x) memset (&(x), 0, sizeof (x))

#define ERROR_LOCAL -1
#define SUCCESS_LOCAL 0
#define TIMEOUT_LOCAL 1

#define UVC_BUF_MAX 0x1800000

/* 25min, 錄影單支檔案最長播放時間 */
#define REC_PER_FILE_TIME 1500

#define IOCTL_GUI_OFF       0
#define IOCTL_GUI_ON        1
#define IOCTL_GUI_STATUS    2

/* 30MB, SD Card 最小剩餘空間 */
#define SD_CARD_MIN_SIZE 0x1E00000

// rex+ 161018, remove ./cedarx
//#define IMG_BUF_MAX 		0x1800000
//DEC_BUF_MAX: 存放給硬解的影像, 最大4k解析度
//#define DEC_BUF_MAX			0x1800000
#define DEC_BUF_MAX			0x800000
//錄影緩存BUF
#define REC_BUF_MAX 		0x2800000

/* Muxer Type */
#define MUXER_TYPE_MP4		0
#define MUXER_TYPE_TS		1
#define MUXER_TYPE_AVI		2

#define CUSTOMER_CODE_ULTRACKER 	0
#define CUSTOMER_CODE_LETS      	10137
#define CUSTOMER_CODE_ALIBABA   	2067001
#define CUSTOMER_CODE_PIIQ   		20141

#define POWER_SAVING_INIT_OVERTIME    	35000000
#define POWER_SAVING_CMD_OVERTIME_5S  	 5000000
#define POWER_SAVING_CMD_OVERTIME_15S 	15000000

#define  BOTTOM_FILE_NAME_DEFAULT	"background_bottom"
#define  BOTTOM_FILE_NAME_USER		"background_bottom_user"
#define  BOTTOM_FILE_NAME_ORG		"background_bottom_org"

typedef struct image_size_struct_H
{
    int width;
    int height;
}image_size_struct;

typedef struct dec_buf_struct_H
{
	int                 idx;
	int                 uvc_idx;
	int                 flag[3];
	int                 len[3];
//tmp	char                buf[3][DEC_BUF_MAX];
	char                **buf;
	image_size_struct   size[3];                // rex+ 161107
} dec_buf_struct;
extern dec_buf_struct dec_buf;



extern int hdmi_get_hpd_status();
extern int checkWifiInterface();
extern int getMicInterface();

// ====================================================================================

#define PCM_BUF_MAX 0x100000
typedef struct pcm_buf_struct_H
{
	//int en;
	int state;
	int fps;
	unsigned P1;	// TINYCAP to BUF
	unsigned P2;	// BUF to FILE
	unsigned P3;	// not use
	unsigned jump;
	unsigned size;
//tmp	unsigned char buf[PCM_BUF_MAX];
	unsigned char *buf;
	unsigned long long delayTime;
}pcm_buf_struct;
extern pcm_buf_struct pcm_buf;

#define CMD_QUEUE_MAX 20
struct Cmd_Queue {
  int P1;	//最後一個CMD IDX
  int P2;	//執行到哪一個CMD IDX
  int queue[CMD_QUEUE_MAX];
};
extern struct Cmd_Queue Cap_Cmd_Queue;

#define DORESIZE_BUF_MAX 8
typedef struct doResize_struct_h 
{
	int mode;
	int cap_file_cnt;
	int rsv1;
	int rec_file_cnt;
	int rsv2;
	char doResizePath[128];
	char THMPath[128];
	char DirPath[128];
}doResize_struc;

typedef struct doResize_buf_struct_h 
{
	int P1;
	int P2;
	doResize_struc cmd[DORESIZE_BUF_MAX];
}doResize_buf_struc;
extern doResize_buf_struc doResize_buf;

#define REC_2_THM_BUF_MAX	8
typedef struct rec2thm_struct_h {
    int P1;
    int P2;
    int en[REC_2_THM_BUF_MAX];
    char path[REC_2_THM_BUF_MAX][128];
    int mode[REC_2_THM_BUF_MAX];            // CAP=0, REC=1, Lapse=2, HDR=3, RAW=4
}rec2thm_struct;
extern rec2thm_struct rec2thm;

typedef	int (read_jpeg_t)(unsigned char *buf, int size, int stream);
typedef	int (read_h264_t)(unsigned char *buf, int size, int ip_f);
struct callback_func_entry {
	read_jpeg_t *read_jpeg;
	read_h264_t *read_h264;
};
extern struct callback_func_entry callback_func;

typedef struct US360_Debug_Struct_h {
	int CheckSum;					//0x20180221
	unsigned long long Time;
	int Tag;						//0:non 1:err
	int MainCode;
	int SubCode;
	int Data;

	int rev[8];
}US360_Debug_Struct;

// ====================================================================================

//extern int enable_debug_flag;

extern pthread_t thread_uvc_id;
extern pthread_t thread_rec_id;
extern pthread_t thread_pcm_id;

extern int camerabase;

//extern int DISP_FPS;
//extern int IMG_WIDTH;
//extern int IMG_HEIGHT;
extern int IMG_Pixelformat;

extern unsigned int buffer_length, real_length;

extern unsigned char *byte_rgb;

//extern int checkW;
extern int img_ok_flag;
extern int img_ready_flag;
//extern int cap_en;
//extern int CaptureCnt;
//extern unsigned long long CaptureSpaceTime;
extern unsigned long long capture_t, capture_t_lst;

extern char THMPath[128];
extern char DirPath[128];

extern char mSSID[16];

extern char panorama[0x69E];
extern char panorama_head_str[0x69E];

extern char TimeString[80];

extern char sd_path[64];
extern unsigned jpeg_err_cnt, jpeg_ok_cnt;
extern int debug_cnt, debug_en;
extern int rec_file_cnt, cap_file_cnt;

extern int save_parameter_flag;

extern int stream_flag;
//extern char *panoramabuf;

extern unsigned char **decode_buf;
extern unsigned decode_len[2];
extern unsigned decode_stream[2];
extern pthread_mutex_t mut_dec_buf;
extern pthread_mutex_t mut_rec_buf;
extern pthread_mutex_t mut_pcm_buf;
extern int decode_idx0, decode_idx1;

extern int img_debug_flag;

extern unsigned long long save_parameter_tmp_t, save_parameter_lst_t;
extern int uvc_thread_en, pcm_thread_en;

extern unsigned long long sd_freesize;
extern unsigned char *rec_img_buf;

extern int DrivingRecord_Mode;

// ====================================================================================
void Cmd_Buf_Init(void);
void doResize_buf_init(void);
int xioctl(int fd, int request, void *arg);
int readframeonce(void);
void processimage (const void *p, unsigned int length, unsigned int r_length);
void set_panorama_head(int width, int height, int Top);
void maek_save_file_path(int mod_sel, char *save_file_path, char *sd_path_in, char *ssid_str_in, int file_cnt);
int readframe(void);
void save_jpeg_func(unsigned char *img, int size, int s_flag, int c_mode, int err, int r_size);
void save_jpeg_tmp_func(char *img, int size);
int stopcapturing(void);
int uninitdevice(void);
int closedevice(void);
void mjpeg(unsigned char *src, unsigned int length, unsigned int r_length);
int Time_Lapse_Init(int time_lapse, int res);
void pcm_buf_init(int freq, int mode, int a_src, int islive);
void rec_start_init(int res, int time_lapse, int freq, int fpag_enc);
void *uvc_thread(void);
void *pcm_thread(void);
void *rec_thread(void);
int read_pcm_buf(pcm_buf_struct *buf, unsigned char *a_buf, int size);
//int RecoderFillBlack(unsigned char *vbuf, int len, unsigned char *vbuf_tmp, int cnt);
char *get_storage_path(void);
//void set_read_proc(char *jpeg_proc, char *h264_proc);
//void enable_debug_message(void);
unsigned long long get_sd_free_size(char *path);
unsigned long GetFileLength(char *fileName);
int check_rec_time(int state, int ip_f, int type, int tl_mode);
int check_rec_time2(int idx);
void save_sensor_data(void);
int read_sensor_data(void);
void Get_FPGA_H264_SPS_PPS(int *sps_len, char *sps, int *pps_len, char *pps);
void Set_Skip_Frame_Cnt(int cnt);
void wrtieLidar2File();
void MakeH264DataHeaderProc(void);
void SendH264EncodeTable(void);
void setFanRotateSpeed(int speed);
void setFPGACtrlPower(int ctrl_pow);
int LoadParameterTmp(void);
int prepareCamera(int videoid, int videobase);
void SetWriteUS360DataBinFlag(int flag);
void set_fpga_encode_type(int type);
int get_fpga_encode_type();
int getImgReadyFlag();
int Capture_Is_Finish();
int getCaptureFileCnt();
void Set_Init_Image_Time(unsigned long long time);
void do_Test_Mode_Func_jni(int m_cmd, int s_cmd);
int stopREC(int debug);
void SetPlaySoundFlag(int flag);
void SetMCUData(int cpuNowTemp);
void free_us360_buf();
int malloc_us360_buf();
int errnoexit(const char *s);
void get_timelapse_ms(unsigned long long *time);
int getTimeLapseMode(void);
int get_mic_is_alive();
void set_rec_proc_en(int en);
void set_sd_card_state(int state);
int get_sd_card_state();
void set_write_file_error(int err);
void setRecEn(int recState, int time_lapse, unsigned long long freesize, int fpga_enc);

void us360_init();

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__US360_H__
