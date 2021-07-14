/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __US360_H__
#define __US360_H__

#include <pthread.h>

#include "Device/US363/Debug/fpga_dbt.h"

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

//#define CUSTOMER_CODE_ULTRACKER 	0
//#define CUSTOMER_CODE_LETS      	10137
//#define CUSTOMER_CODE_ALIBABA   	2067001
//#define CUSTOMER_CODE_PIIQ   		20141

//#define  BOTTOM_FILE_NAME_DEFAULT	"background_bottom"
//#define  BOTTOM_FILE_NAME_USER		"background_bottom_user"
//#define  BOTTOM_FILE_NAME_ORG		"background_bottom_org"

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

#define REC_2_THM_BUF_MAX	8
typedef struct rec2thm_struct_h {
    int P1;
    int P2;
    int en[REC_2_THM_BUF_MAX];
    char path[REC_2_THM_BUF_MAX][128];
    int mode[REC_2_THM_BUF_MAX];            // CAP=0, REC=1, Lapse=2, HDR=3, RAW=4
}rec2thm_struct;
extern rec2thm_struct rec2thm;

// ====================================================================================
void doResize_buf_init(void);       //***
void maek_save_file_path(int mod_sel, char *save_file_path, char *sd_path_in, char *ssid_str_in, int file_cnt);
int readframe(void);    //***
void save_jpeg_func(unsigned char *img, int size, int s_flag, int c_mode, int err, int r_size);     //***
void save_jpeg_tmp_func(char *img, int size);   //***
void mjpeg(unsigned char *src, unsigned int length, unsigned int r_length);     //***
int Time_Lapse_Init(int time_lapse, int res);
void pcm_buf_init(int freq, int mode, int a_src, int islive);
int read_pcm_buf(pcm_buf_struct *buf, unsigned char *a_buf, int size);
unsigned long long get_sd_free_size(char *path);
unsigned long GetFileLength(char *fileName);
void save_sensor_data(void);    //***
void Set_Skip_Frame_Cnt(int cnt);
void fpgaCtrlPowerService(int ctrl_pow);
int prepareCamera(int videoid, int videobase);
void set_fpga_encode_type(int type);
int get_fpga_encode_type();
int getImgReadyFlag();
int Capture_Is_Finish();
void Set_Init_Image_Time(unsigned long long time);
void free_us360_buf();
int malloc_us360_buf();
int errnoexit(const char *s);
void get_timelapse_ms(unsigned long long *time);
int get_mic_is_alive();
void set_rec_proc_en(int en);
void setRecEn(int recState, int time_lapse, int timelapse_enc);

//-----------------------------------------------------
void us360_init();

void calSdFreeSize(unsigned long long *size);

void setDbtDdrRWCmd(fpga_dbt_rw_cmd_struct *ddr_cmd);
void copyDataToDbtDdrRWBuf(char *buf, int size, int offset);
void getDbtDdrRWStruct(fpga_ddr_rw_struct *ddr_p);
void setDbtRegRWCmd(fpga_reg_rw_struct *reg_p);
void getDbtRegRWStruct(fpga_reg_rw_struct *reg_p);

void setModelName(char *ver);
void writeWifiMaxLink(int maxLink);
void setBmxSensorLogEnable(int enable);
void WriteWifiChannel(int channel);
int GetSaturationValue(int value);
void FPGA_Ctrl_Power_Func(int ctrl_pow, int flag);
void getPath();
void Show_Now_Mode_Message(int mode, int res, int fps, int live_rec);
void set_timeout_start(int sel);
int checksd();
int CheckSDcardState(char *path);
int setCapEn(int capEn, int capCnt, int capStime);
int checkMicInterface();
int getHdmiConnected();
int GetUVCfd();
void Load_Parameter_Tmp_Proc();
int CheckSaveJpegCnt();
void pollWatchDog();
int checkWifiInterface();
int getImgPixelformat();
void setCapFileCnt(int cnt);
int getCapFileCnt();
void setSaveParameterFlag(int flag);
int readCapturePrepareTime();

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__US360_H__
