/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __US363_CAMERA_H__
#define __US363_CAMERA_H__

#ifdef __cplusplus
extern "C" {
#endif

//Country
#define COUNTRY_CODE_TAIWAN         158
#define COUNTRY_CODE_JAPAN          392
#define COUNTRY_CODE_CHINA          156
#define COUNTRY_CODE_USA            840

//Customer
#define CUSTOMER_CODE_ULTRACKER     0
#define CUSTOMER_CODE_LETS          10137
#define CUSTOMER_CODE_ALIBABA       2067001   
#define CUSTOMER_CODE_PIIQ   	    20141	    /* 客戶後來更名為PEEK */

//Resolution Width
#define RESOLUTION_WIDTH_12K    11520
#define RESOLUTION_WIDTH_4K     3840
#define RESOLUTION_WIDTH_8K     7680
#define RESOLUTION_WIDTH_10K    10240
#define RESOLUTION_WIDTH_6K     6144
#define RESOLUTION_WIDTH_3K     3072
#define RESOLUTION_WIDTH_2K     2048

//Resolution Height
#define RESOLUTION_HEIGHT_12K   5760
#define RESOLUTION_HEIGHT_4K    1920
#define RESOLUTION_HEIGHT_8K    3840
#define RESOLUTION_HEIGHT_10K   5120
#define RESOLUTION_HEIGHT_6K    3072
#define RESOLUTION_HEIGHT_3K    1536
#define RESOLUTION_HEIGHT_2K    1024

//Power Saving OverTime
#define POWER_SAVING_INIT_OVERTIME      35000   /* 35s */
#define POWER_SAVING_CMD_OVERTIME_5S    5000    /* 5s */
#define POWER_SAVING_CMD_OVERTIME_15S   15000  	/* 15s */

//Bottom File Name
#define BOTTOM_FILE_NAME_DEFAULT    "background_bottom"                           
#define BOTTOM_FILE_NAME_USER       "background_bottom_user"  
#define BOTTOM_FILE_NAME_ORG        "background_bottom_org" 

//SD Card Size
#define SD_CARD_FREE_SIZE_MIN       0x1E00000   /* 30MB */  /** JAVA_SD_CARD_MIN_SIZE */

//Play Mode
enum {
    PLAY_MODE_GLOBAL = 0,
    PLAY_MODE_FRONT  = 1,
    PLAY_MODE_360    = 2,
    PLAY_MODE_240    = 3,
    PLAY_MODE_180X2  = 4,
    PLAY_MODE_4SPLIT = 5,
    PLAY_MODE_PIP    = 6
};

//Resolution
enum {
    RESOLUTION_MODE_FIX = 0,
    RESOLUTION_MODE_12K = 1,
    RESOLUTION_MODE_4K  = 2,
    RESOLUTION_MODE_8K  = 7,
    RESOLUTION_MODE_10K = 8,
    RESOLUTION_MODE_6K  = 12,
    RESOLUTION_MODE_3K  = 13,
    RESOLUTION_MODE_2K  = 14
};

//Camera Mode
enum {
    CAMERA_MODE_CAP           = 0,
    CAMERA_MODE_REC           = 1,
    CAMERA_MODE_TIMELAPSE     = 2,
    CAMERA_MODE_AEB           = 3,
    CAMERA_MODE_RAW           = 4,
    CAMERA_MODE_HDR           = 5,
    CAMERA_MODE_NIGHT         = 6,
    CAMERA_MODE_NIGHT_HDR     = 7,
    CAMERA_MODE_SPORT         = 8,
    CAMERA_MODE_SPORT_WDR     = 9,
    CAMERA_MODE_REC_WDR       = 10,
    CAMERA_MODE_TIMELAPSE_WDR = 11,
    CAMERA_MODE_M_MODE        = 12,
    CAMERA_MODE_REMOVAL       = 13,
    CAMERA_MODE_3D_MODEL      = 14
};

//Capture Mode
enum {
    CAPTURE_MODE_BURST      = -1,               //連拍
    CAPTURE_MODE_NORMAL     = 0,                //一般拍照
    CAPTURE_MODE_SELFIE_2S  = 2,                //2S自拍
    CAPTURE_MODE_SELFIE_10S =10                 //10S自拍
};

//Timelapse Mode
enum {
    TIMELAPSE_MODE_NONE  = 0,
    TIMELAPSE_MODE_1S    = 1,
    TIMELAPSE_MODE_2S    = 2,
    TIMELAPSE_MODE_5S    = 3,
    TIMELAPSE_MODE_10S   = 4,
    TIMELAPSE_MODE_30S   = 5,
    TIMELAPSE_MODE_60S   = 6,
    TIMELAPSE_MODE_166MS = 7
};

//Camera Position Ctrl Mode
enum {
    CAMERA_POSITION_CTRL_MODE_MANUAL = 0,
    CAMERA_POSITION_CTRL_MODE_AUTO
};

//Camera Position
enum {
    CAMERA_POSITION_0   = 0,
    CAMERA_POSITION_180 = 1,
    CAMERA_POSITION_90  = 2
};

//White Balance Mode
enum {
    WHITE_BALANCE_MODE_AUTO          = 0,         //自動
    WHITE_BALANCE_MODE_FILAMENT_LAMP = 1,         //鎢絲燈
    WHITE_BALANCE_MODE_DAYLIGHT_LAMP = 2,         //日光燈
    WHITE_BALANCE_MODE_SUN           = 3,         //陽光
    WHITE_BALANCE_MODE_CLOUDY        = 4,         //陰天
    WHITE_BALANCE_MODE_RGB           = 5
};

//Fan Ctrl
enum {
    FAN_CTRL_OFF    = 0,
    FAN_CTRL_FAST   = 1,
    FAN_CTRL_MEDIAN = 2,
    FAN_CTRL_SLOW   = 3
};

//Color Stitching Mode
enum {
    COLOR_STITCHING_MODE_OFF  = 0,
    COLOR_STITCHING_MODE_ON   = 1,
    COLOR_STITCHING_MODE_AUTO = 2
};

//Translucent Mode
enum {
    TRANSLUCENT_MODE_OFF  = 0,
    TRANSLUCENT_MODE_ON   = 1,
    TRANSLUCENT_MODE_AUTO = 2
};

//Jpeg Quality Mode
enum {
    JPEG_QUALITY_MODE_HIGH   = 0,
    JPEG_QUALITY_MODE_MIDDLE = 1,
    JPEG_QUALITY_MODE_LOW    = 2
};

//Jpeg Live Quality Mode
enum {
    JPEG_LIVE_QUALITY_MODE_HIGH = 0,                    //每秒全張數送給遠端, high quality, ex:4K = 10fps
    JPEG_LIVE_QUALITY_MODE_LOW                          //每秒半張數送給遠端,  low quality, ex:4K =  5fps, 達到降低WIFI傳輸量
};

//Speaker Mode
enum {
    SPEAKER_MODE_ON = 0,
    SPEAKER_MODE_OFF
};

//Oled Control
enum {
    OLED_CONTROL_ON = 0,
    OLED_CONTROL_OFF
};

//HDR Interval Ev Mode
enum {
    HDR_INTERVAL_EV_MODE_LOW    = 2,                    //Ev:  0,-1,-2
    HDR_INTERVAL_EV_MODE_MIDDLE = 4,                    //Ev: +1,-1,-3
    HDR_INTERVAL_EV_MODE_HIGH   = 8                     //Ev: +2,-1,-4
};

//WDR Mode
enum {
    WDR_MODE_HIGH   = 0,
    WDR_MODE_MIDDLE = 1,
    WDR_MODE_LOW    = 2
};

//Bottom Mode
enum {
    BOTTOM_MODE_OFF            = 0,                     //關閉
    BOTTOM_MODE_EXTRND         = 1,                     //延伸
    BOTTOM_MODE_IMAGE_DEFAULT  = 2,                     //加底圖(default)
    BOTTOM_MODE_MIRROR         = 3,                     //鏡像
    BOTTOM_MODE_IMAGE_USER     = 4                      //加底圖(user)
};

//Bottom Text Mode
enum {
    BOTTOM_TEXT_MODE_OFF = 0,
    BOTTOM_TEXT_MODE_ON
};

//Fpga Video Encode Tyep
enum {
    FPGA_VIDEO_ENCODE_TYPE_JPEG      = 0,
    FPGA_VIDEO_ENCODE_TYPE_FPGA_H264 = 1,
    FPGA_VIDEO_ENCODE_TYPE_CPU_H264  = 2
};

//Power Saving Mode
enum {
    POWER_SAVING_MODE_OFF = 0,
    POWER_SAVING_MODE_ON
};




void initCamera();
void destroyCamera();
void startPreview();
void create_thread_1s();
void create_thread_5ms();
void create_thread_20ms();
void do_Test_Mode_Func(int m_cmd, int s_cmd);
int GetDiskInfo();
int stopREC();

//==================== get/set =====================
void getUS363Version(char *version);
void getSdPath(char *path, int size);
void setWriteFileError(int err);
int getWriteFileError();
int getSdState();
void getWifiSsid(char *ssid);
void getWifiApSsid(char *ssid);
void getWifiPassword(char *password);
void setFpgaCtrlPower(int ctrl);
int getFpgaCtrlPower();
int getFpgaStandbyEn();
int getPlayMode();
void setCameraMode(int camrea_mode);
int getCameraMode();
int getCameraPositionCtrlMode();
int getCameraPositionMode();
int getHdrIntervalEvMode();
int getFPS();
int getDrivingRecordMode();
void getResolutionWidthHeight(int *width, int *height);
void setSdState(int state);
void setDirPath(char *path);
void setThmPath(char *path);
int getCaptureIntervalTime();
void setCaptureIntervalTime(int time);
void setFpgaStandbyEn(int en);
void setWhiteBalanceMode(int wb_mode);
int getWhiteBalanceMode();
void setLedBrightness(int led_brightness);
void setOledControl(int oled_ctrl);
void setWifiModeCmd(int mode);
void setPowerSavingSendDataStateTime(unsigned long long time);
int getFreeCount();	
void setAudioRecThreadEn(int en);
int getAudioRecThreadEn();
void setDcState(int state);
int getDcState();
void setSdFreeSize(unsigned long long size);
unsigned long long getSdFreeSize();
void subSdFreeSize(int size);
int getTimeLapseMode();
void wifiSerThe_proc();
    
	
#ifdef __cplusplus
}   // extern "C"
#endif	

#endif /* __US363_CAMERA_H__ */
