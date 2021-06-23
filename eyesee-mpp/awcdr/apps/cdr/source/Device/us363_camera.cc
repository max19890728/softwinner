/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/us363_camera.h"

#include <mpi_sys.h>
#include <mpi_venc.h>

#include "Device/spi.h"
#include "Device/qspi.h"
#include "Device/US363/us360.h"
#include "Device/US363/Kernel/k_spi_cmd.h"
#include "Device/US363/Driver/Lidar/lidar.h"
#include "Device/US363/Net/ux363_network_manager.h"
#include "Device/US363/Net/ux360_wifiserver.h"
#include "Device/US363/Data/databin.h"
#include "Device/US363/Data/wifi_config.h"
#include "Device/US363/Data/pcb_version.h"
#include "Device/US363/Data/customer.h"
#include "Device/US363/Data/country.h"
#include "Device/US363/Data/us363_folder.h"
#include "Device/US363/System/sys_time.h"
#include "Device/US363/System/sys_cpu.h"
#include "Device/US363/System/sys_power.h"
#include "Device/US363/Test/test.h"

#include "device_model/display.h"
#include "device_model/media/camera/camera.h"
#include "device_model/media/camera/camera_factory.h"
#include "device_model/storage_manager.h"
#include "device_model/system/uevent_manager.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363Camera"

//==================== main parameter ====================
char mUS363Version[64] = "v1.00.00\0";          /** VersionStr */

char mWifiApSsid[32] = "US_0000\0";             /** ssid */
char mWifiApPassword[16] = "88888888\0";        /** pwd */

char mWifpStaIpFromAp[32];                      /** ipAddrFromAp */
char mWifiStaSsid[32], mWifiStaPassword[16];    /** wifiSSID, wifiPassword */
int mWifiStaType = 0, mWifiStaLinkType = 0, mWifiStaReboot = 0;     /** wifiType, wifiLinkType, wifiReboot */
char mWifiStaIP[32], mWifiStaGateway[32], mWifiStaPrefix[32];       /** wifiIP, wifiGateway, wifiPrefix */
char mWifiStaDns1[32], mWifiStaDns2[32];        /** wifiDns1, wifiDns2 */

int mCountryCode = COUNTRY_CODE_USA;            /** LangCode */
int mCustomerCode = CUSTOMER_CODE_ULTRACKER;    /** customerCode */

//int mMcuVersion = 45;                         /** mcuVersion */
int mUserCtrl = 1, mUserCtrlLst = 0;            //使用者控制FPS,  0: auto 1: 30fps   /** User_Ctrl, User_Ctrl_lst */
int mWifiDisableTime = 180;                     /** wifiDisableTime */
int mFreeCount = 0;                             //DataBin, 剩餘拍攝張數/錄影時間 /** freeCount */

int mPlayMode    = PLAY_MODE_GLOBAL;            /** PlayMode2 */                              
int mPlayModeTmp = PLAY_MODE_GLOBAL;            /** PlayMode2_tmp */   

int mResolutionWidthHeight[15][2] = {
/* 0*/    { 0, 0 },
/* 1*/    { RESOLUTION_WIDTH_12K, RESOLUTION_HEIGHT_12K },
/* 2*/    { RESOLUTION_WIDTH_4K,  RESOLUTION_HEIGHT_4K  },
/* 3*/    { 0, 0 },
/* 4*/    { 0, 0 },
/* 5*/    { 0, 0 },
/* 6*/    { 0, 0 },
/* 7*/    { RESOLUTION_WIDTH_8K,  RESOLUTION_HEIGHT_8K  },
/* 8*/    { RESOLUTION_WIDTH_10K, RESOLUTION_HEIGHT_10K },
/* 9*/    { 0, 0 },
/*10*/    { 0, 0 },
/*11*/    { 0, 0 },
/*12*/    { RESOLUTION_WIDTH_6K,  RESOLUTION_HEIGHT_6K  },
/*13*/    { RESOLUTION_WIDTH_3K,  RESOLUTION_HEIGHT_3K  },
/*14*/    { RESOLUTION_WIDTH_2K,  RESOLUTION_HEIGHT_2K  }
};
int mResolutionMode    = RESOLUTION_MODE_4K;    /** ResolutionMode */
//int mResolutionModeLst = RESOLUTION_MODE_4K;    /** ResolutionMode_lst */
int mResolutionWidth   = mResolutionWidthHeight[RESOLUTION_MODE_4K][0];     /** ResolutionWidth */
int mResolutionHeight  = mResolutionWidthHeight[RESOLUTION_MODE_4K][1];     /** ResolutionHeight */

int mCameraMode = CAMERA_MODE_HDR;              /** CameraMode */       	
int mFPS = 100;                                 /** FPS */

int mCaptureMode = CAPTURE_MODE_NORMAL;         /** CaptureMode */
int mCaptureCnt = 3;            	            // 連拍: 1 3 5 10 ...                 /** CaptureCnt */
int mCaptureIntervalTime = 160;    	            // 連拍間隔時間: 500ms 1000ms ...     /** CaptureSpaceTime */
int mSelfieTime = 0;            	            // 0: none 2: 2秒自拍 10: 10秒自拍    /** SelfTimer */

int mTimeLapseMode = TIMELAPSE_MODE_NONE;        /** Time_Lapse_Mode */

#define CAMERA_POSITION_OVER_COUNT      4
int mCameraPositionCtrlMode = CAMERA_POSITION_CTRL_MODE_AUTO;   /** CtrlCameraPositionMode */    
int mCameraPositionMode     = CAMERA_POSITION_0;                /** CameraPositionMode */  
int mCameraPositionModeChange = 0;               /** CameraPositionModeChange */
int mCameraPositionCnt = 0;                      //連續幾次才變化, 解震盪現象 /** CameraPositionCnt */

int mWhiteBalanceMode = WHITE_BALANCE_MODE_AUTO;    /** WB_Mode */

int mWifiChannel = 6;                           /** WifiChannel */
int mAegEpFreq = 60;                            // 60:60Hz 50:50Hz  /** AEG_EP_Freq */

int mFanCtrl = FAN_CTRL_MEDIAN;                 /** FanCtrl */     
//#define FAN_CTRL_TIME   2                     /* 幾秒做一次FanCtrl, 單位:秒 */    /** FanCtrlCounter */      
//int fanCtrlCount = 0;                         /** FanCtrlCount */                    

int mColorStitchingMode = COLOR_STITCHING_MODE_ON;      /** Color_ST_Mode */
int mTranslucentMode = TRANSLUCENT_MODE_ON;             /** Translucent_Mode */

int mAutoGlobalPhiAdjMode = 0;                          //0~100  /** doAutoGlobalPhiAdjMode */ 
int mHDMITextVisibilityMode = 1;                        /** HDMITextVisibilityMode */

int mJpegQualityMode = JPEG_QUALITY_MODE_HIGH;			//照片的Quality    /** JPEGQualityMode */
int mJpegLiveQualityMode = JPEG_LIVE_QUALITY_MODE_HIGH; //Live時JPEG的Quality    /** LiveQualityMode */

#define LED_BRIGHTNESS_AUTO     -1
int mLedBrightness = LED_BRIGHTNESS_AUTO;			    //-1:自動,亮度:0~100   /** ledBrightness */
int mSpeakerMode = SPEAKER_MODE_ON;				        /** speakerMode */
int mOledControl = OLED_CONTROL_ON;				        /** oledControl */

int mHdrIntervalEvMode = HDR_INTERVAL_EV_MODE_MIDDLE;   /** HdrEvMode */
int mWdrMode = WDR_MODE_MIDDLE;	 			            /** WdrMode */

int mBottomMode = BOTTOM_MODE_IMAGE_DEFAULT;			/** BottomMode */
int mBottomSize = 50;	                                //10~100 /** BottomSize */
int mBottomTextMode = BOTTOM_TEXT_MODE_OFF;		        /** BottomTextMode */
    
int mHttpPort = 8080;                                   /** definePort */
char mHttpAccount[32] = "admin\0";                      /** httpAccount */
char mHttpPassword[32] = "admin\0";                     /** httpPassword */
    
int mTimelapseEncodeType = TIMELAPSE_ENCODE_TYPE_H264;		/** FPGA_Encode_Type */   
    
int mPowerSavingMode = POWER_SAVING_MODE_OFF;							            //省電模式, 0:Off 1:On  /** Power_Saving_Mode */
unsigned long long powerSavingInitTime1 = 0, powerSavingInitTime2 = 0;                   /** Power_Saving_Init_t1, Power_Saving_Init_t2*/
unsigned long long powerSavingCapRecStartTime = 0, powerSavingCapRecFinishTime = 0;      /** Cap_Rec_Start_t, Cap_Rec_Finish_t*/
unsigned long long powerSavingSendDataStateTime = 0;								     /** Send_Data_State_t */  //預防手機忘記關閉Seting UI
unsigned long long powerSavingOvertime = 0;                                              /** Power_Saving_Overtime */
unsigned long long powerSavingSetingUiTime1 = 0, powerSavingSetingUiTime2 = 0;           /** Seting_UI_t1, Seting_UI_t2 */
int fpgaStandbyEn = 0;								                                //FPGA休眠狀態, 0:Off 1:On  /** FPGA_Sleep_En */
int powerSavingInit = 0;							                                //開機 / 休眠起來一段時間FPGA才進入休眠  /** Power_Saving_Init */
int powerSavingSetingUiState = 0;								                    //手機Seting列的狀態, 0:close 	1:open  /** Seting_UI_State */

char mPcbVersion[8] = "V0.0\0";                                                     /** PCB_Ver */

int mSaturation = 0;                                                                /** Saturation */
int saturationInitFlag = 0;                                                         /** Saturation_Init_Flag */

char mEthernetIP[64];
char mEthernetMask[64];
char mEthernetGway[64];
char mEthernetDns[64];

int mDrivingRecordMode = 1;                     // 行車紀錄模式 0:off 1:on   /** DrivingRecord_Mode */
int mDebugLogSaveToSDCard = 0;	 	            // 系統資料是否存入SD卡 0:off 1:on   /** DebugLog_Mode */

//==================== variable ====================                               
char bottomFileNameSource[32] = "background_bottom\0";   /** BOTTOM_FILE_NAME_SOURCE */

int doResizeParameter[10];                      /** doResize */
int doResizeIndex = 0, doResizeIndexLst = 0;    /** doResize_flag, doResize_flag_lst*/
char doResizePath[8][128];                      /** doResize_path */
int doResizeMode[8];                            /** doResize_mode */

char webServiceData[0x10000];
int webServiceDataLen = 0;

int recordEn = 0;                               /** mRecordEn */    //0:stop 1:start
int recordCmdFlag = 0;                          /** mRecordCmd */   //0:none 1:doRecordVideo 2:audio record start 3:send state to wifi
int recordEnReal = -2;                          // read jni rec_state   /** mRecordEnJNI */ //-2:none 0:start 1:recording -1/-3:stop

int wifiModeCmd = 0;                            // change Wifi or WifiAP    /** mWifiModeCmd */
enum {
    WIFI_AP = 0,
    WIFI_STA,
    WIFI_STA_DISCONNECT,
    WIFI_STA_CHANGE
};
int wifiModeState = WIFI_AP;                    /** mWifiModeState */
int wifiApEn=0;                                 /** wifi_connected */
//int wifiApRestartCount = 0;                   /** systemRestartCount */   //checkWifiAPLive() 可改靜態即可
int wifiConnectIsAlive = 0;                     /** Wifi_Connect_isAlive */
int wifiChangeStop = 0;                         //changeWifiMode() 可改靜態即可

int hdmiState = 0/*, hdmiStateLst = -1*/;       //0:off 1:on    /** HDMI_State, HDMI_State_lst */
int hdmiStateChange = 0;                        /** HDMI_State_Change */
    
int fpgaCheckSum = 0;
int skipWatchDog = 0/*, skipWatchDogLst = -1*/;     /**  , skipWatchDog_lst */
int dnaCheck = 0/*, dnaCheckLst = -1*/;             //0:err 1:ok  /** dna_check_ok, dna_check_ok_lst */

char thmPath[128];                              /** THM_path */
char dirPath[128];                              /** DIR_path */
char sdPath[128] = "/mnt/extsd";                /** sd_path */
int sdState = 0/*, sdStateLst = -1*/;               /** sd_state, lst_sd_state */
unsigned long long sdFreeSize = 0;                   /** sd_freesize */
unsigned long long sdAllSize = 0;                    /** sd_allsize */

char capNumStr[64];                             //還可以拍幾張    /** cap_num */
char recTimeStr[64];                            //還可以錄多久    /** rec_time */
int recTime = 0;                                //已經錄了多久
int isNeedNewFreeCount = 0;                     //需要重新計算還可以拍幾張/路多久, 則設成1
//int captureDCnt = 0;                          //取得拍照時Pipe Line Cmd, readCaptureDCnt(), 可能Cmd沒有成功加入會歸零, 拍完照也會歸零

int batteryPower = 0;                           /** power */
int batteryPowerLst = 10;                       /** powerRang */
int batteryVoltage = 0;                         /** voltage */
int dcState = 0;
int batteryLogCnt = 0;

unsigned long long systemTime = 0;
int writeUS360DataBinFlag = 0;                  /** writeUS360DataBin_flag */
int writeUS360DataBinStep = 0;                  /** saveBinStep */

int lidarState = 0;
int lidarCode = 0;
char lidarVersion[8];
long lidarDelay = -1;

#define LIDAR_BUFFER_MAX    0x400000            /* 4MB */
int lidarCanBeFind = 1;                         // 0禁止使用 1:正常
int lidarCycleTime = 10;                        /** timerLidarCycle */
int lidarPtrEnable = 0;                         /** LidarPtrEnable */
int lidarBufMod = 1;	                        /** LidarBufMod */   // 0:512x256, 1:1Kx512, 2:2Kx1K
int lidarBufEnd = 2000000;			            /** LidarBufEnd */   // 設定讀取的總數量
int lidarBufPtr = 0;				            /** LidarBufPtr */   // 目前已讀取的數量
int lidarScanCnt = 0;                           /** lidarScanCnt */
int lidarBuffer[LIDAR_BUFFER_MAX];		        /** LidarBuffer */   // 最大 2M 筆資料 (1K x 2K) x 2 (sin & cos)
unsigned long long lidarTime1;                       /** lidarT1 */
int lidarError = 0;                             /** lidarErr */

int chooseModeFlag = 0;                         /** choose_mode_flag */
int fpgaCtrlPower = 0;                          /** FPGA_Ctrl_Power */
int fpgaCmdIdx = -1, fpgaCmdP1 = -1;            /** Cmd_Idx, Cmd_P1 */

enum {
    DEFECT_TYPE_TESTTOOL = 0,
    DEFECT_TYPE_USER
};
enum {
    DEFECT_STATE_ERROR = -1,
    DEFECT_STATE_NONE  = 0,
    DEFECT_STATE_OK    = 1,
};
int defectType = DEFECT_TYPE_TESTTOOL;          /** Defect_Type */       
int defectStep = 0;                             /** Defect_Step */       
int defectState = DEFECT_STATE_NONE; 		    /** Defect_State */       
int defectEp = 30;                              /** Defect_Ep */       
//紀錄抓壞點前的參數
int defectCameraModeLst = 0;                    /** Defect_CMode_lst */       
int defectResolutionModeLst   = 0;              /** Defect_Res_lst */       
int defectEpLst    = 30;                        /** Defect_Ep_lst */       
int defectGaniLst  = 0;                         /** Defect_Gani_lst */    
unsigned long long defectDelayTime1=0, defectDelayTime2=0;       /** Defect_T1, Defect_T2 */   

#define CPU_FULL_SPEED      1152000
#define CPU_HIGH_SPEED      600000     	        /* 2核 x 600Mhz*/
#define CPU_MIDDLE_SPEED    480000    	        /* 4核 x 480Mhz*/
#define CPU_LOW_Speed       480000  	        /* 2核 x 480Mhz*/

#define ETHERNET_CONNECT_MAX    3               /** EthCounter */
int ethernetConnectCount = 0;                   /** EthCount */
int ethernetConnectFlag;                        /** ethConnect */

int isStandby = 0;
//int uvcErrCount = 0;                          /** UVCErrCount */

int showFpgaDdrEn = 0;                          /** Show_DDR_En */
int showFpgaDdrAddr = 0x00080000;               /** Show_DDR_Addr */

int paintVisibilityMode = 0;                    /** ShowPaintVisibilityMode */
int stitchingVisibilityMode = 0;                /** ShowStitchVisibilityMode */
int stitchingVisibilityType = 3;                /** ShowStitchVisibilityType */
int focusVisibilityMode = 0;                    /** ShowFocusVisibilityMode */

int adjSensorIdx = 0;                           /** AdjSensorIdx */
int smoothOIdx = 0;                             //debug /** Smooth_O_Idx */

int gpsState = 0;					            /** mGps */
int bmm050Start = 0;					        //手機校正電子羅盤  /** mBmm050Start */
unsigned long long standbyTime = 0;                  /** sleepTime */

int smoothShow = -1;                            //debug
int stitchShow = -1;                            //debug

char updateFileName[128];                       /** zip */
int sendColorTemperatureTime = 2;               // 幾秒送一次色溫, 單位:秒    /** KelvinCounter */

enum {
    RTMP_SWITCH_ON = 0,
    RTMP_SWITCH_OFF
};
int sendRtmpEn = 0;                             /** canSendRtmp */
char rtmpUrl[128] = "rtmp://localhost/live/stream";
int liveStreamWidth  = mResolutionWidthHeight[RESOLUTION_MODE_4K][0];       /** outWidth */
int liveStreamHeight = mResolutionWidthHeight[RESOLUTION_MODE_4K][1];       /** outHeight */
int rtmpSwitch = RTMP_SWITCH_OFF;               /** rtmp_switch */
char oledRtmpBitrate[32] = "0 kbps";            /** oled_rtmp_bitrate */
int rtmpConnectCount = 0;
unsigned long long sendRtmpVideoTime = 0;            /** rtmpVideoPushTimer */   //用來計算距離前一次送資料多久
unsigned long long sendRtmpAudioTime = 0;            /** rtmpAudioPushTimer */
int rtmpVideoPushReady = 0;
int rtmpAudioPushReady = 0;

int doAutoStitchingFlag = 0;                    /** doAutoStitch_flag */
//int mcuUpdateFlag = 0;                          /** mcuUpdate_flag */
//int mcuDelayCheck = 0;                          /** delayCheck */
//int mcuAdcValue = 1000;                         /** adcValue */

int focusSensor2 = -1;                          /** Focus2_Sensor */
//int focusSensor = 0;                            /** Focus_Sensor */
int focusIdx = 0;                               /** Focus_Idx */
int focusToolNum = 0;                           /** Focus_Tool_Num */

int downloadFileState = 0;                      /** downloadLevel */    //download 狀態
int playSoundId = 0;                            /** soundId */  //播放音效ID
int captureWaitTime = 30;                       /** captureWaitTime */  //拍照預估等待時間(最大值)

#define CHOOSE_MODE_AFTER_TIME   1000           /* 切換模式後1秒才可拍照 */
unsigned long long chooseModeTime = 0;               /** choose_mode_time */ 

int doCheckStitchingCmdDdrFlag = 0;             /** check_st_cmd_ddr_flag */
int test3DHorizontalFlag = 0;                   /** testHonz */ 

#define RTSP_BUFFER_MAX     0x100000            /* 1MB */
char rtspBuffer[RTSP_BUFFER_MAX];               /** rtsp_buff */
int rtspFps = 0, rtspSendLength = 0;            /** rtsp_fps, rtsp_send */

unsigned long long checkTimeoutPowerTime1 = 0;       /** toutPowerT1*/
unsigned long long checkTimeoutBurstTime1 = 0;       /** toutBurstT1*/
unsigned long long checkTimeoutSelfieTime1 = 0;      /** toutSelfT1*/
unsigned long long checkTimeoutLongKeyTime = 0;      /** toutLongKey*/
unsigned long long checkTimeoutSaveTime1 = 0;        /** toutSaveT1*/
unsigned long long checkTimeoutTakeTime1 = 0;        /** toutTakeT1*/

int checkTimeoutSelfieSec = 0;                  /** mSelfTimerSec */
int checkTimeoutBurstCount = 0;                 /** mBurstCount */
int checkTimeoutTakePicture = 0;                /** mTakePicture */

unsigned long long curTimeThread1s = 0, lstTimeThread1s = 0;         /** curTime_run1s, lstTime_run1s */
unsigned long long curTimeThreadSt = 0, lstTimeThreadSt = 0;         /** curTime_runSt, lstTime_runSt */
unsigned long long curTimeThread10ms = 0, lstTimeThread10ms = 0;     /** curTime_run10ms, lstTime_run10ms */
unsigned long long curTimeThread20ms = 0, lstTimeThread20ms = 0;     /** curTime_run20ms, lstTime_run20ms */
unsigned long long curTimeThread100ms = 0, lstTimeThread100ms = 0;   /** curTime_run100ms, lstTime_run100ms */
unsigned long long curTimeThread5ms = 0, lstTimeThread5ms = 0, runTimeThread5ms = 0;      /** curTime, lstTime, runTime*/
unsigned long long curTimeThread1 = 0, lstTimeThread1 = 0;           /** curTime_run1, lstTime_run1 */

int mjpegFps = 0, mjpegSendLength = 0;          /** int mjpeg_fps, mjpeg_send */

#define SEND_MAIN_CMD_PIPE_DELAY_TIME       50                      /* 50ms */
unsigned long long sendMainCmdPipeTime1 = 0, sendMainCmdPipeTime2 = 0;   /** SendMainCmdPipeT1, SendMainCmdPipeT2 */ //send main cmd delay time
#define ADJUST_SENSOR_SYNC_INTERVAL_TIME    3000                    /* 3000ms */
unsigned long long adjSensorSyncTime1 = 0, adjSensorSyncTime2 = 0;       /** AdjSensorSyncT1, AdjSensorSyncT2 */
#define READ_SENSOR_STATE_INTERVAL_TIME     1000                    /* 1000ms */
unsigned long long sensorStateTime1 = 0, sensorStateTime2 = 0;           /** SensorStateT1, SensorStateT2 */
unsigned long long testtoolDelayTime1 = 0, testtoolDelayTime2 = 0;       /** TestTool_D_t1, TestTool_D_t2 */

int sendFpgaCmdStep = 0;                        /** Send_ST_Flag */
#define READ_SENSOR_STATE_ERROR_COUNT_MAX   3
int sensorState = 0, sensorStateErrorCnt = 0;   /** Sensor_State, Sensor_State_Cnt */
enum {
    ADJUST_SENSOR_FLAG_NONE        = 0,
    ADJUST_SENSOR_FLAG_RESET       = 1,
    ADJUST_SENSOR_FLAG_SYNC        = 2,
    ADJUST_SENSOR_FLAG_CHOOSE_MODE = 3
};
int adjustSensorFlag = 0;		                    /** Adj_Sensor_Sync_Flag */

#define  SHOW_SENSOR                0
#define  SHOW_Adj_C_New             1
#define  SHOW_Adj                   2
#define  REC_SHOW_NOW_MODE          3
#define  RADIO_SHOW_NOW_MODE        4
#define  HDMI_SHOW_NOW_MODE         5
#define  FPGA_SHOW_NOW              6
#define  HDMI_TEXT_VISIBILITY       7
#define  SHOW_PAINT_VISIBILITY      8
#define  SHOW_STITCH_VISIBILITY     9
#define  SYS_CYCLE_SHOW_NOW	        10
#define  SHOW_FOCUS_VISIBILITY      11

//int menuFlag = 0;                             /** mMenuFlag */    //長按拍照鍵切換模式
//float menuGsensorMaxX, menuGsensorMaxY;       /** Gsensor_Max_X Gsensor_Max_Y */  // 搖晃US360，進入menu模式                  

int captureKeyLongPressFlag = 0;                /** mSYSRQ_LongPress_Flag */
int powerKeyLongPressFlag = 0;                  /** mPOWER_LongPress_Flag */
int powerKeyRepeatCount = -1;                   /** mPower_RepeatCount */
enum {
    POWER_MODE_STANDBY  = 0,
    POWER_MODE_MEM      = 1,
    POWER_MODE_BOOTFAST = 2,
    POWER_MODE_ON       = 3
};
int powerMode = POWER_MODE_ON, powerOFF = 0;    /** PowerMode, PowerOFF */

#define AUDIO_REC_EMPTY_BYTES_SIZE   10240
char audioRecEmptyBytes[AUDIO_REC_EMPTY_BYTES_SIZE];

vector<long>::iterator audioTimeStamp;          /** ls_audioTS */
vector<int>::iterator audioSize;                /** ls_readBufSize */
vector<char[]>::iterator audioBuffer;           /** ls_audioBuf */

int micSource = 0;			                    // 0:內部  1:外部
int localAudioRate = 44100, localAudioBits = 16, localAudioChannel = 1;     /** audioRate, audioBit, audioChannel */
int wifiAudioRate = 44100, wifiAudioBits = 16, wifiAudioChannel = 1;        /** wifi_audioRate, wifi_audioBit, wifi_audioChannel */

int audioTestCount = 0;                         //產線測試喇叭用, 播放一段聲音並錄下

enum {
    LIVE360_STATE_STOP  = -1,
    LIVE360_STATE_START = 0
};
int live360En = 0;                              /** mLive360En */
int live360Cmd = 0;                             /** mLive360Cmd */
int live360State = LIVE360_STATE_STOP;          /** Live360_State */    // 0:start -1:stop
long live360SendHttpCmdTime1 = 0;               /** Live360_t1 */


//==================== get/set =====================

void setDoAutoStitchFlag(int flag) { doAutoStitchingFlag = flag; }
int getDoAutoStitchFlag() { return doAutoStitchingFlag; }

void setResolutionMode(int resolution) { mResolutionMode = resolution; }
int getResolutionMode() { return mResolutionMode; }

void setResolutionWidthHeight(int resolution) {
    mResolutionWidth   = mResolutionWidthHeight[resolution][0];
    mResolutionHeight  = mResolutionWidthHeight[resolution][1];
}
void getResolutionWidthHeight(int *width, int *height) { 
    *width  = mResolutionWidth;
    *height = mResolutionHeight;
}

void setPlayMode(int play_mode) { mPlayMode = play_mode; }
int getPlayMode() { return mPlayMode; }

void setPlayModeTmp(int play_mode) { mPlayModeTmp = play_mode; }
int getPlayModeTmp() { return mPlayModeTmp; }

void setFPS(int fps) { mFPS = fps; }
int getFPS() { return mFPS; }

void setAegEpFreq(int freq) {
	mAegEpFreq = freq;
	setFpgaEpFreq(freq);
}
int getAegEpFreq() { return mAegEpFreq; }

void setChooseModeTime(unsigned long long time) { chooseModeTime = time; }
void getChooseModeTime(unsigned long long *time) { *time = chooseModeTime; }

void setHdmiState(int state) { hdmiState = state; }
int getHdmiState() { return hdmiState; }

void setHdmiStateChange(int en) { hdmiStateChange = en; }
int getHdmiStateChange() { return hdmiStateChange; }

void setCameraMode(int camrea_mode) { mCameraMode = camrea_mode; }
int getCameraMode() { return mCameraMode; }

void setMjpegFps(int fps) { mjpegFps = fps; }
int getMjpegFps() { return mjpegFps; }

void setMjpegSendLength(int length) { mjpegSendLength = length; }
int getMjpegSendLength() { return mjpegSendLength; }

void setRtspFps(int fps) { rtspFps = fps; }
int getRtspFps() { return rtspFps; }

void setRtspSendLength(int value) { rtspSendLength = value; }
int getRtspSendLength() { return rtspSendLength; }

void setSkipWatchDog(int flag) { skipWatchDog = flag; }
int getSkipWatchDog() { return skipWatchDog; }

void setFocusVisibilityMode(int visibility_mode) { focusVisibilityMode = visibility_mode; }
int getFocusVisibilityMode() { return focusVisibilityMode; }

void setPowerSavingMode(int pw_save_mode) { mPowerSavingMode = pw_save_mode; }
int getPowerSavingMode() { return mPowerSavingMode; }

void setUserCtrl(int user_ctrl) { mUserCtrl = user_ctrl; }
int getUserCtrl() { return mUserCtrl; }

void setFocusSensor2(int sensor) { focusSensor2 = sensor; }
int getFocusSensor2() { return focusSensor2; }

void setCaptureMode(int capture_mode) { mCaptureMode = capture_mode; }
int getCaptureMode() { return mCaptureMode; }

void setSaturation(int saturation) { mSaturation = saturation; }
int getSaturation() { return mSaturation; }

void setSaturationInitFlag(int flag) { saturationInitFlag = flag; }
int getSaturationInitFlag() { return SaturationInitFlag; }

void setTesttoolDelayTime(int idx, unsigned long long time) {
	if(idx == 1)      testtoolDelayTime1 = time;
	else if(idx == 2) testtoolDelayTime2 = time;
}
unsigned long long getTesttoolDelayTime(int idx) {
	unsigned long long time;
	if(idx == 1)      time = testtoolDelayTime1;
	else if(idx == 2) time = testtoolDelayTime2;
	return time;
}

void getUS363Version(char *version) { sprintf(version, "%s\0", mUS363Version); }

void setWifiDisableTime(int time) { mWifiDisableTime = time; }
int getWifiDisableTime() { return mWifiDisableTime; }

void setFreeCount(int cnt) { mFreeCount = cnt; }
int getFreeCount() { return mFreeCount; }

void setCaptureCnt(int cnt) { mCaptureCnt = cnt; }
int getCaptureCnt() { return mCaptureCnt; }

void setCaptureIntervalTime(int time) { mCaptureIntervalTime = time; }
int getCaptureIntervalTime() { return mCaptureIntervalTime; }

void setSelfieTime(int time) { mSelfieTime = time; }
int getSelfieTime() { return mSelfieTime; }

void setTimeLapseMode(int tl_mode) { mTimeLapseMode = tl_mode; }
int getTimeLapseMode() { return mTimeLapseMode; }

void setCtrlCameraPositionMode(int ctrl_mode) { mCtrlCameraPositionMode = ctrl_mode; }
int getCtrlCameraPositionMode() { return mCtrlCameraPositionMode; }

void setCameraPositionMode(int position_mode) { mCameraPositionMode = position_mode; }
int getCameraPositionMode() { return mCameraPositionMode; }

void setCameraPositionModeChange(int en) { mCameraPositionModeChange = en; }
int getCameraPositionModeChange() { return mCameraPositionModeChange; }

void setWhiteBalanceMode(int wb_mode) { mWhiteBalanceMode = wb_mode; }
int getWhiteBalanceMode() { return mWhiteBalanceMode; }

void setFanCtrl(int ctrl) { mFanCtrl = ctrl; }
int getFanCtrl() { return mFanCtrl; }

void setColorStitchingMode(int color_st_mode) { mColorStitchingMode = color_st_mode; }
int getColorStitchingMode() { return mColorStitchingMode; }

void setTranslucentMode(int tran_mode) { mTranslucentMode = tran_mode; }
int getTranslucentMode() { return mTranslucentMode; }

void setAutoGlobalPhiAdjMode(int adj_mode) { mAutoGlobalPhiAdjMode = adj_mode; }
int getAutoGlobalPhiAdjMode() { return mAutoGlobalPhiAdjMode; }

void setHDMITextVisibilityMode(int visibility_mode) { mHDMITextVisibilityMode = visibility_mode; }
int getHDMITextVisibilityMode() { return mHDMITextVisibilityMode; }

void setJpegQualityMode(int quality_mode) { 
    mJpegQualityMode = quality_mode; 
    set_A2K_JPEG_Quality_Mode(quality_mode);
}
int getJpegQualityMode() { return mJpegQualityMode; }

void setJpegLiveQualityMode(int quality_mode) { mJpegLiveQualityMode = quality_mode; }
int getJpegLiveQualityMode() { return mJpegLiveQualityMode; }

void setLedBrightness(int led_brightness) { mLedBrightness = led_brightness; }
int getLedBrightness() { return mLedBrightness; }

void setSpeakerMode(int speaker_mode) { mSpeakerMode = speaker_mode; }
int getSpeakerMode() { return mSpeakerMode; }

void setOledControl(int oled_ctrl) { mOledControl = oled_ctrl; }
int getOledControl() { return mOledControl; }

void setHdrIntervalEvMode(int hdr_ev_mode) { mHdrIntervalEvMode = hdr_ev_mode; }
int getHdrIntervalEvMode() { return mHdrIntervalEvMode; }

void setWdrMode(int wdr_mode) { mWdrMode = mode; }
int getWdrMode() { return mWdrMode; }

void setBottomMode(int btm_mode) { mBottomMode = btm_mode; }
int getBottomMode() { return mBottomMode; }

void setBottomSize(int size) { mBottomSize = size; }
int getBottomSize() { return mBottomSize; }

void setBottomTextMode(int btm_text_mode) { 
    mBottomTextMode = btm_text_mode; 
    set_A2K_DMA_BottomTextMode(btm_text_mode);
}
int getBottomTextMode() { return mBottomTextMode; }

void setTimelapseEncodeType(int enc_type) { mTimelapseEncodeType = enc_type; }
int getTimelapseEncodeType() { return mTimelapseEncodeType; }

void setFpgaStandbyEn(int en) { 
    fpgaStandbyEn = en; 
    SetFPGASleepEn(fpgaStandbyEn);
}
int getFpgaStandbyEn() { return fpgaStandbyEn; }

void setPowerSavingInit(int flag) { powerSavingInit = flag; }
int getPowerSavingInit() { return powerSavingInit; }

void setPowerSavingSetingUiState(int state) { powerSavingSetingUiState = state; }
int getPowerSavingSetingUiState() { return powerSavingSetingUiState; }

void setDrivingRecordMode(int mode) { mDrivingRecordMode = mode; }
int getDrivingRecordMode() { return mDrivingRecordMode; }

void setDebugLogSaveToSDCard(int en) { mDebugLogSaveToSDCard = en; }
int getDebugLogSaveToSDCard() { return mDebugLogSaveToSDCard; }

void setPowerSavingInitTime1(unsigned long long time) { powerSavingInitTime1 = time; }
unsigned long long getPowerSavingInitTime1() { return powerSavingInitTime1; }

void setPowerSavingInitTime2(unsigned long long time) { powerSavingInitTime2 = time; }
unsigned long long getPowerSavingInitTime2() { return powerSavingInitTime2; }

void setPowerSavingCapRecStartTime(unsigned long long time) {
		powerSavingCapRecStartTime = time;
		setPowerSavingInit(1);		//預防開機先拍照
}
unsigned long long getPowerSavingCapRecStartTime() { return powerSavingCapRecStartTime; }

void setPowerSavingOvertime(unsigned long long time) { powerSavingOvertime = time; }
unsigned long long getPowerSavingOvertime() { return powerSavingOvertime; }

void setPowerSavingCapRecFinishTime(unsigned long long time, unsigned long long overtime) {
	powerSavingCapRecFinishTime = time;
	setPowerSavingOvertime(overtime);
	setPowerSavingInit(1);		//預防開機先拍照
}
unsigned long long getPowerSavingCapRecFinishTime() { return powerSavingCapRecFinishTime; }

void setPowerSavingSendDataStateTime(unsigned long long time) { powerSavingSendDataStateTime = time; }
unsigned long long getPowerSavingSendDataStateTime() { return powerSavingSendDataStateTime; }

void setPowerSavingSetingUiTime1(unsigned long long time) { powerSavingSetingUiTime1 = time; }
unsigned long long getPowerSavingSetingUiTime1() { return powerSavingSetingUiTime1; }

void setPowerSavingSetingUiTime2(unsigned long long time) { powerSavingSetingUiTime2 = time; }
unsigned long long getPowerSavingSetingUiTime2() { return powerSavingSetingUiTime2; }

void setWebServiceDataLen(int len) { webServiceDataLen = len; }
int getWebServiceDataLen() { return webServiceDataLen; }

void setWebServiceData(char *data, int offset, int data_len) { memcpy(&webServiceData[offset], data, data_len); }
void getWebServiceData(char *buf, int web_len) { memcpy(buf, &webServiceData[0], web_len); }

void pushWebServiceData(char *data, int offset, int data_len) { 
    int web_len = getWebServiceDataLen();
    setWebServiceData(data, offset, data_len);
    web_len += data_len;
    setWebServiceDataLen(web_len);
}
void popWebServiceDataAll(char *buf) {
    int web_len = getWebServiceDataLen();
    getWebServiceData(buf, web_len);
    setWebServiceDataLen(0);
}

void setRecordEn(int en) { recordEn = en; }
int getRecordEn() { return recordEn; }

void setRecordCmdFlag(int flag) { recordCmdFlag = flag; }
int getRecordCmdFlag() { return recordCmdFlag; }

void setRecordEnReal(int en) { recordEnReal = en; }
int getRecordEnReal() { return recordEnReal; }

void setWifiModeCmd(int mode) { wifiModeCmd = mode; }
int getWifiModeCmd() { return wifiModeCmd; }

void setWifiModeState(int state) { wifiModeState = state; }
int getWifiModeState() { return wifiModeState; }

void setWifiConnectIsAlive(int en) { wifiConnectIsAlive = en; }
int getWifiConnectIsAlive() { return wifiConnectIsAlive; }

void setDnaCheck(int en) { dnaCheck = en; }
int getDnaCheck() { return dnaCheck; }

void setThmPath(char *path) { snprintf(thmPath, sizeof(thmPath), "%s\0", path); }
void getThmPath(char *path, int size) { snprintf(path, size, "%s\0", thmPath); }

void setDirPath(char *path) { snprintf(dirPath, sizeof(dirPath), "%s\0", path); }
void getDirPath(char *path, int size) { snprintf(path, size, "%s\0", dirPath); }

void setSdPath(char *path) { snprintf(sdPath, sizeof(sdPath), "%s\0", path); }
void getSdPath(char *path, int size) { snprintf(path, size, "%s\0", sdPath); }

void setSdState(int state) { sdState = state; }
int getSdState() { return sdState; }





//==================== fucntion ====================
void initCountryFunc() 
{  
	mCountryCode = readCountryCode();
//tmp	waitLanguage(mCountryCode);
    db_debug("initLensFunc() mCountryCode=%d\n", mCountryCode);
}
	
void initCustomerFunc() 
{
    char name[16] = "AletaS2\0";
	char *ptr = NULL;

    mCustomerCode = readCustomerCode();
	sprintf(wifiAPssid, "%s\0", mSSID);
//tmp    ledStateArray[showSsidHead] = 0;
    db_debug("initCustomerFunc() mCustomerCode=%d\n", mCustomerCode);

    switch(mCustomerCode) {
    case CUSTOMER_CODE_LETS:
		sprintf(name, "LET'S\0");
		sprintf(bottomFileNameSource, "background_lets\0");
//tmp    	ControllerServer.machineNmae = "LET'S";
    	setModelName(&name[0]);
    	writeWifiMaxLink(10);
    	break;
    case CUSTOMER_CODE_ALIBABA:
		sprintf(name, "Alibaba\0");
		sprintf(bottomFileNameSource, "background_alibaba\0");
//tmp    	ControllerServer.machineNmae = "Alibaba";
    	setModelName(&name[0]);
    	writeWifiMaxLink(10);
    	setSensorLogEnable(1);
    	break;
    case CUSTOMER_CODE_PIIQ:
		sprintf(name, "peek\0");
		sprintf(bottomFileNameSource, "background_peek\0");
    	//wifiAPssid = ssid.replace("US_", "peek-HD_");
		ptr = strchr(mSSID, '_');
		sprintf(wifiAPssid, "peek-HD_%s\0", (ptr+1) );
//tmp        ledStateArray[showSsidHead] = 1;
//tmp        ControllerServer.machineNmae = "peek";
    	setModelName(&name[0]);
    	writeWifiMaxLink(1); //控制連線上限範圍 1 - 10
    	break;
    default:
    	setModelName(&name[0]);
    	writeWifiMaxLink(10);
    	break;
    }
}

void destroyCamera()
{
	free_us360_buf();
	free_wifiserver_buf();
	free_lidar_buf();
	SPI_Close();
	QSPI_Close();
}

int initSysConfig() 
{
	MPP_SYS_CONF_S sys_config{};
	memset(&sys_config, 0, sizeof(sys_config));
	sys_config.nAlignWidth = 32;
	AW_MPI_SYS_SetConf(&sys_config);
	return AW_MPI_SYS_Init_S1();
}

void initCamera()
{
	if(malloc_us360_buf() < 0) 	    goto end;
	if(malloc_wifiserver_buf() < 0) goto end;
//  if(malloc_lidar_buf() < 0)	    goto end;			// 記憶體需求過大, 導致程式無法執行

	initSysConfig();

	if(EyeseeLinux::StorageManager::GetInstance()->IsMounted() ) {
		makeUS360Folder();
		makeTestFolder();
		readWifiConfig(&mWifiApSsid[0], &mWifiApPassword[0]);
	}
	else {
		getNewSsidPassword(&wifiApSsid[0], &wifiApPassword[0]);
	}
	db_debug("US363Camera() ssid=%s pwd=%s", wifiApSsid, wifiApPassword);

	stratWifiAp(&wifiApSsid[0], &wifiApPassword[0], 0);
	start_wifi_server(8555);
	
//	us360_init();
	return;
	
end:
	db_debug("US363Camera() Error!");
	destroyCamera();	
}

void databinInit(int country, int customer) 
{
	ReadUS360DataBin(country, customer);
	
	//TagVersion = 			0;
	int nowVer  = Get_DataBin_Now_Version();
	int readVer = Get_DataBin_Version();
    if(readVer != nowVer) {
        Set_DataBin_Version(nowVer);
        writeUS360DataBinFlag = 1;
    }
	
    //TagDemoMode = 		1;
    char us360_ver[16];
    Get_DataBin_US360Version(&us360_ver[0], sizeof(us360_ver) );
    if(strcmp(us360_ver, mUS363Version) != 0) {
        Set_DataBin_US360Version(&mUS363Version[0]);
        writeUS360DataBinFlag = 1;
    }
	
	//TagCamPositionMode = 	2;
    switch(Get_DataBin_CamPositionMode() ) {
    case 0:  setCtrlCameraPositionMode(CAMERA_POSITION_CTRL_MODE_AUTO); break;
    case 1:  setCtrlCameraPositionMode(CAMERA_POSITION_CTRL_MODE_MANUAL); setCameraPositionMode(CAMERA_POSITION_0);   break;
    case 2:  setCtrlCameraPositionMode(CAMERA_POSITION_CTRL_MODE_MANUAL); setCameraPositionMode(CAMERA_POSITION_180); break;
    case 3:  setCtrlCameraPositionMode(CAMERA_POSITION_CTRL_MODE_MANUAL); setCameraPositionMode(CAMERA_POSITION_90);  break;
    default: setCtrlCameraPositionMode(CAMERA_POSITION_CTRL_MODE_MANUAL); setCameraPositionMode(CAMERA_POSITION_0);   break;
    }
    set_A2K_DMA_CameraPosition(getCameraPositionMode());
	
	//TagPlayMode = 		3;
	//TagResoultMode = 		4;
    mResolutionMode = Get_DataBin_ResoultMode();
	
	//TagEVValue = 			5;
    setAETergetRateWifiCmd(Get_DataBin_EVValue() );
	
	//TagMFValue = 			6;
    setALIGlobalPhiWifiCmd(Get_DataBin_MFValue() );
	
	//TagMF2Value = 		7;
    setALIGlobalPhi2WifiCmd(Get_DataBin_MF2Value() );
	
	//TagISOValue = 		8;
    int iso = Get_DataBin_ISOValue();
    if(iso == -1) setAEGGainWifiCmd(0, iso);
    else          setAEGGainWifiCmd(1, iso);
	
	//TagExposureValue = 	9;
    int exp = Get_DataBin_ExposureValue();
    if(exp == -1) setExposureTimeWifiCmd(0, exp);
    else          setExposureTimeWifiCmd(1, exp);
	
	//TagWBMode = 			10;
    mWhiteBalanceMode = Get_DataBin_WBMode();
    setWBMode(mWhiteBalanceMode);
	
	//TagCaptureMode = 		11;
    mCaptureMode = Get_DataBin_CaptureMode();
	
	//TagCaptureCnt = 		12;
    mCaptureCnt = Get_DataBin_CaptureCnt();
	
	//TagCaptureSpaceTime = 13;
    mCaptureIntervalTime = Get_DataBin_CaptureSpaceTime();
	
	//TagSelfTimer = 		14;
    mSelfieTime = Get_DataBin_SelfTimer();
	
	//TagTimeLapseMode = 	15;
    mTimeLapseMode = Get_DataBin_TimeLapseMode();
	
	//TagSaveToSel = 		16;
	//TagWifiDisableTime = 	17;
    mWifiDisableTime = Get_DataBin_WifiDisableTime();
	
	//TagEthernetMode = 	18;
	//TagEthernetIP = 		19;
	//TagEthernetMask = 	20;
	//TagEthernetGateWay = 	21;
	//TagEthernetDNS = 		22;
	//TagMediaPort = 		23;
    Set_DataBin_MediaPort(Get_DataBin_MediaPort() ); //檢測自己數值是否正常
	
	//TagDrivingRecord = 	24;
    if(customer == CUSTOMER_CODE_PIIQ)
    	setDrivingRecordMode(0);	//Get_DataBin_DrivingRecord();
    else
    	setDrivingRecordMode(1);	//Get_DataBin_DrivingRecord();
	
	//TagUS360Versin = 		25;
	//TagWifiChannel = 		26;
    mWifiChannel = Get_DataBin_WifiChannel();
    WriteWifiChannel(mWifiChannel);
	
	//TagExposureFreq = 	27;
    int freq = Get_DataBin_ExposureFreq();
    SetAEGEPFreq(freq);
	
	//TagFanControl = 		28;
	int ctrl = Get_DataBin_FanControl();
	setFanCtrl(ctrl);
	
	//TagSharpness = 		29;
    setStrengthWifiCmd(Get_DataBin_Sharpness() );
	
	//TagUserCtrl30Fps = 	30;
    //mUserCtrl = Get_DataBin_UserCtrl30Fps();
	
	//TagCameraMode = 		31;
    SetCameraMode(Get_DataBin_CameraMode() );

	//TagColorSTMode = 		32;
    setColorStitchingMode(COLOR_STITCHING_MODE_ON);
//tmp    SetColorSTSW(getColorStitchingMode());      //S2:smooth.c / us360_func.c
	
	//TagAutoGlobalPhiAdjMode = 33;
    setAutoGlobalPhiAdjMode(Get_DataBin_AutoGlobalPhiAdjMode());
    setSmoothParameter(3, getAutoGlobalPhiAdjMode());
	
	//TagHDMITextVisibility = 34;
    setHDMITextVisibilityMode(Get_DataBin_HDMITextVisibility());
	
	//TagSpeakerMode = 		35;
    setSpeakerMode(Get_DataBin_SpeakerMode());
	
	//TagLedBrightness = 	36;
    setLedBrightness(Get_DataBin_LedBrightness());

	//TagOledControl = 		37;
    setOledControl(Get_DataBin_OledControl());

	//TagDelayValue = 		38;
	//TagImageQuality = 	39;
    setJpegQualityMode(Get_DataBin_ImageQuality());
	
	//TagPhotographReso = 	40;
	//TagRecordReso = 		41;
	//TagTimeLapseReso = 	42;
	//TagTranslucent = 		43;
    setTranslucentMode(TRANSLUCENT_MODE_ON);
    SetTransparentEn(getTranslucentMode());
	
	//TagCompassMaxx = 		44;
	//TagCompassMaxy = 		45;
	//TagCompassMaxz = 		46;
	//TagCompassMinx = 		47;
	//TagCompassMiny = 		48;
	//TagCompassMinz = 		49;
	//TagDebugLogMode = 	50;
    mDebugLogSaveToSDCard = Get_DataBin_DebugLogMode();
	
	//TagBottomMode = 		51;
    setBottomMode(Get_DataBin_BottomMode());
	//TagBottomSize = 		52;
    setBottomSize(Get_DataBin_BottomSize());
//tmp    SetBottomValue(getBottomMode(), getBottomSize());      //S2:awcodec.c

	//TagHdrEvMode = 		53;
    setHdrIntervalEvMode(Get_DataBin_hdrEvMode());
    switch(getHdrIntervalEvMode()) {
    case HDR_INTERVAL_EV_MODE_LOW: 
        setWdrMode(WDR_MODE_LOW); 
        break;
    case HDR_INTERVAL_EV_MODE_MIDDLE:
        setWdrMode(WDR_MODE_MIDDLE); 
        break;
    case HDR_INTERVAL_EV_MODE_HIGHT:
        setWdrMode(WDR_MODE_HIGHT); 
        break;
    }
    SetWDRLiveStrength(getWdrMode(());
	
    //TagBitrate = 			54;
//tmp    SetBitrateMode(Get_DataBin_Bitrate() );

    //TagHttpAccount =      55;
	//TagHttpPassword =     56;
	//TagHttpPort =         57;
    Set_DataBin_HttpPort(Get_DataBin_HttpPort()); //檢測自己數值是否正常
	
    //TagCompassMode  = 	58;
//tmp    setbmm050_enable(Get_DataBin_CompassMode() );

	//TagGsensorMode  = 	59;
//tmp    setBma2x2_enable(Get_DataBin_GsensorMode() );

	//TagCapHdrMode =		60;
	//TagBottomTMode =		61;
	setBottomTextMode(Get_DataBin_BottomTMode());

	//TagBottomTColor =		62;
	//TagBottomBColor =		63;
	//TagBottomTFont =		64;
	//TagBottomTLoop =		65;
	//TagBottomText =		66;
	//TagFpgaEncodeType =	67;
	setTimelapseEncodeType(Get_DataBin_FpgaEncodeType());

	//TagWbRGB =			68;
	//TagContrast =			69;
	int contrast = Get_DataBin_Contrast();
	setContrast(contrast);
	
	//TagSaturation =		70;
	int sv = Get_DataBin_Saturation();
	setSaturation(GetSaturationValue(sv));
	
	//TagFreeCount
	calSdFreeSize(&sdFreeSize);
//tmp    getRECTimes(sdFreeSize);
    mFreeCount = Get_DataBin_FreeCount();
    if(mFreeCount == -1){
//tmp    	Set_DataBin_FreeCount(GetSpacePhotoNum() );
    	mFreeCount = Get_DataBin_FreeCount();
    	writeUS360DataBinFlag = 1;
    }else{
//tmp    	int de = mFreeCount - GetSpacePhotoNum();
		int de = mFreeCount;
    	if(de > 100 || de < -100){
//tmp    		freeCount = GetSpacePhotoNum();
    	}else if(de > 10){
    		mFreeCount += 10;
    	}else if(de > 0){
    		mFreeCount += de;
    	}else if(de < -10){
    		mFreeCount -= 10;
    	}else if(de < -9){
    		mFreeCount -= de;
    	}
    	Set_DataBin_FreeCount(mFreeCount);
    	writeUS360DataBinFlag = 1;
    }
	
    //TagBmodeSec
    setAEGBExp1Sec(Get_DataBin_BmodeSec() );
	
    //TagBmodeGain
    setAEGBExpGain(Get_DataBin_BmodeGain() );
	
    //TagHdrManual
    set_A2K_DeGhostEn(Get_DataBin_HdrDeghosting() );
    Setting_HDR7P_Proc(Get_DataBin_HdrManual(), hdr_mode);
	
    //TagAebNumber
    setting_AEB(Get_DataBin_AebNumber(), Get_DataBin_AebIncrement() * 2);
	
    //TagLiveQualityMode
    setJpegLiveQualityMode(Get_DataBin_LiveQualityMode());
    set_A2K_JPEG_Live_Quality_Mode(getJpegLiveQualityMode());
	
    //TagWbTemperature
    //TagWbTint
    setWBTemperature(Get_DataBin_WbTemperature() * 100, Get_DataBin_WbTint() );
	
    //TagRemoveHdrMode
    Setting_RemovalHDR_Proc(Get_DataBin_RemoveHdrMode() );
	
    //TagAntiAliasingEn
    if(customer == CUSTOMER_CODE_ALIBABA) {
    	if(Get_DataBin_AntiAliasingEn() == 1)
    		Set_DataBin_AntiAliasingEn(0);
    	SetAntiAliasingEn(0);
    }
    else {
    	SetAntiAliasingEn(Get_DataBin_AntiAliasingEn() );
    }
	
    //TagRemoveAntiAliasingEn
    if(customer == CUSTOMER_CODE_ALIBABA) {
    	if(Get_DataBin_RemoveAntiAliasingEn() == 1)
    		Set_DataBin_RemoveAntiAliasingEn(0);
    	SetRemovalAntiAliasingEn(0);
    }
    else {
    	SetRemovalAntiAliasingEn(Get_DataBin_RemoveAntiAliasingEn() );
    }
	
    //TagLiveBitrate = 			94;
//tmp    SetLiveBitrateMode(Get_DataBin_LiveBitrate() );

    //TagLiveBitrate = 			95;
    setPowerSavingMode(Get_DataBin_PowerSaving());
}

void onCreate() 
{
	int i, j, ret = 0, len = 0;
	    
//tmp    systemlog = new SystemLog();
//tmp    systemlog.start();
//tmp    systemlog.addLog("info", System.currentTimeMillis(), "machine", "system is start.", "---");

    set_A2K_Softwave_Version(&mUS363Version[0]);
//    SetmMcuVersion(mMcuVersion);      //max+ S3 沒有mcu
    readPcbVersion(&mPcbVersion[0]);
        
//tmp    int led_mode = GetLedControlMode();
//tmp    ChangeLedMode(led_mode);
//tmp    readMcuUpdate();      //max+ S3 沒有mcu
        
    TestToolCmdInit();
    memset(&audioRecEmptyBytes[0], 0, sizeof(audioRecEmptyBytes));
    for(i = 0; i < 8; i++) 
		doResize_mode[i] = -1;
        
//tmp    setVersionOLED(mUS363Version.getBytes());

    setCpuFreq(4, CPU_FULL_SPEED);
        
    unsigned long long long defaultSysTime = 1420041600000L;                     // 2015/01/01 00:00:00
    unsigned long long long nowSysTime;
	get_current_usec(&nowSysTime);
    if(defaultSysTime > nowSysTime){										// lester+ 180207
		setSysTime(defaultSysTime);
    }
//tmp    writeHistory();
	
    ret = ReadTestResult();
    if(ret != 0) 
		setDoAutoStitchFlag(1);

    readWifiConfig(&mWifiApSsid[0], &mWifiApPassword[0]);
    
    //CreatCountryList();
    initCountryFunc();
    initCustomerFunc(); 
    databinInit(mCountryCode, mCustomerCode);
        
//tmp    ControllerServer.changeDataToDataBin();
    Get_DataBin_HttpAccount(&mHttpAccount[0], sizeof(mHttpAccount) );
    Get_DataBin_HttpPassword(&mHttpPassword[0], sizeof(mHttpPassword) );	
//tmp	Live555Thread(httpAccount.getBytes(), httpAccount.getBytes().length, httpPassword.getBytes(), httpPassword.getBytes().length);

    //checkSaveSmoothBin();     //#debug smooth
//tmp    Init_OLED_Country();
//tmp    Init_US360_OLED();
//tmp    disableShutdown(1);  
    ReadLensCode();           //#old/new lens
//tmp    CreatBackgroundDir();
//tmp    Copy_FPGA_File();
//tmp    initThmFile();         //#thm icon
//tmp    initGsensor();         //#gsensor校正資料
//tmp    initCompass();         //#gsensor資料
        
//tmp    if(mEth != null) {
		Get_DataBin_EthernetIP(&mEthernetIP[0], sizeof(mEthernetIP) );
        Get_DataBin_EthernetMask(&mEthernetMask[0], sizeof(mEthernetMask) );
        Get_DataBin_EthernetGateWay(&mEthernetGway[0], sizeof(mEthernetGway) );
        Get_DataBin_EthernetDNS(&mEthernetDns[0], sizeof(mEthernetDns) );
//tmp        mEth.SetInfo(Get_DataBin_EthernetMode(), ip_s, mask_s, gway_s, dns_s);     //#設定Ethernet參數
//tmp    }
        
//tmp    int wifiMode = sharedPreferences.getInt("wifiMode", 0);
//tmp    if(wifiMode == 1){
//tmp		rwWifiData(0,1);
//tmp        initConnectWifi();
//tmp        db_debug("open wifi mode Wifi,\n" + wifiMode);
//tmp        changeWifiMode(1);
//tmp    }else{
//tmp        db_debug("open wifi mode AP,\n" + wifiMode);
//tmp        initWifiAP();	// 開啟WIFI AP
//tmp    }
//tmp    initLidar();				// rex+ 201021

//tmp    initWifiSerThe();

    //if(GetIsStandby() == 0)    
	//	FanCtrlFunc(getFanCtrl());      //max+ S3 沒有風扇
        
    FPGA_Ctrl_Power_Func(0, 0);

    LoadConfig(1);    

    ret = LoadParameterTmp();
    if(ret != 1 && ret != -1) {		//debug, 紀錄S2發生檔案錯誤問題
        db_error("LoadParameterTmp() error! ret=%d\n", ret);   
        WriteLoadParameterTmpErr(ret);
    }

    ModeTypeSelectS2(Get_DataBin_PlayMode(), Get_DataBin_ResoultMode(), getHdmiState(), Get_DataBin_CameraMode());

    int kpixel = ResolutionModeToKPixel(getResolutionMode() );
//tmp    setOLEDMainModeResolu(getPlayModeTmp(), kpixel);

//tmp    SetOLEDMainType(GetCameraMode(), GetCaptureCnt(), GetCaptureMode(), getTimeLapseMode(), 0);
//tmp    showOLEDDelayValue(Get_DataBin_DelayValue());      	
		
    setStitchingOut(getCameraMode(), getPlayMode(), getResolutionMode(), getFPS()); 
    LineTableInit();  
        
    for(i = 0; i < 8; i++) 
        writeCmdTable(i, getResolutionMode(), getFPS(), 0, 1, 0); 
        
    getPath();		//取得 THMPath / DirPath  //產生資料夾
 
//tmp        sendImgThe = new SendImageThread();
//tmp        sendImgThe.start();

//tmp    textFpgaCheckSum.setText(String.format("0x%08x", getfpgaCheckSum() ));

        // 監聽 SD CARD 插拔
        // install an intent filter to receive SD card related events.
/*	//tmp        mReceiver = new BroadcastReceiver() {
            public void onReceive(Context context, Intent intent) {
                if (intent.getAction() == Intent.ACTION_MEDIA_MOUNTED){ 
                    Log.d("Main", "ACTION_MEDIA_MOUNTED");
                    set_sd_card_state(1);		//Main.sd_state = 1;
                    sd_path = intent.getData().getPath();
                	byte[] path = new byte[64]; 
                	//getSDPath(path);
                	//sd_path = path.toString();
                    Log.d("Main", "ACTION_MEDIA_MOUNTED, path = " + sd_path);
                    setSDPathStr(sd_path.getBytes(), sd_path.getBytes().length);
                    
                    getOutputPath(); 
                }
                else if(intent.getAction() == Intent.ACTION_MEDIA_UNMOUNTED){ 
                    Log.d("Main", "ACTION_MEDIA_UNMOUNTED");
                    set_sd_card_state(0);		//Main.sd_state = 0;
                    sd_path = "/mnt/extsd";
                    setSDPathStr(sd_path.getBytes(), sd_path.getBytes().length);
                }
            }
        };*/

    Show_Now_Mode_Message(getPlayMode(), getResolutionMode(), getFPS(), 0);
        
//tmp        handler1 = new Handler();
//tmp        handler1.post(runnable1);

    create_thread_5ms();

//tmp        handler10ms = new Handler();
//tmp        handler10ms.post(runnable10ms);
//tmp        handler100ms = new Handler();
//tmp        handler100ms.post(runnable100ms);
    create_thread_1s();
	//create_thread_20ms();

//tmp        handler1s_cmdSt=new Handler();
//tmp        handler1s_cmdSt.post(runnable1s_cmdSt);
    set_timeout_start(0);
    set_timeout_start(7);

/*	//tmp        stateTool = new StateTool();
        parametersTool = new ParametersTool();
        parametersTool.setParameterName(1, "Adj_WB_R");
        parametersTool.setParameterName(2, "Adj_WB_G");
        parametersTool.setParameterName(3, "Adj_WB_B");
        parametersTool.setParameterName(4, "Show_DDR_En");
        parametersTool.setParameterName(5, "Show_DDR_Addr");
        parametersTool.setParameterName(6, "FanTempThreshold");
        parametersTool.setParameterName(7, "FanTempDownClose");
        parametersTool.setParameterName(8, "S_Mask_Line_Adj");
        parametersTool.setParameterName(9, "CPUCore");
        parametersTool.setParameterName(10, "Skip_WatchDog");
        parametersTool.setParameterName(11, "Sensor_Lens_Gain");
        parametersTool.setParameterName(12, "S_Lens_Table[0]");
        parametersTool.setParameterName(13, "S_Lens_Table[1]");
        parametersTool.setParameterName(14, "S_Lens_Table[2]");
        parametersTool.setParameterName(15, "S_Lens_Table[3]");
        parametersTool.setParameterName(16, "S_Lens_Table[4]");
        parametersTool.setParameterName(17, "S_Lens_Table[5]");
        parametersTool.setParameterName(18, "S_Lens_Table[6]");
        parametersTool.setParameterName(19, "S_Lens_Table[7]");
        parametersTool.setParameterName(20, "S_Lens_Table[8]");
        parametersTool.setParameterName(21, "S_Lens_Table[9]");
        parametersTool.setParameterName(22, "S_Lens_Table[10]");
        parametersTool.setParameterName(23, "S_Mask_Line_Adj");
        parametersTool.setParameterName(24, "Smooth_YUV_Speed");
        parametersTool.setParameterName(25, "Smooth_Z_Speed");
        parametersTool.setParameterName(26, "LOGE_Enable");
        parametersTool.setParameterName(27, "SLensEn");
        parametersTool.setParameterName(28, "S0LensBrigAdj");
        parametersTool.setParameterName(29, "S1LensBrigAdj");
        parametersTool.setParameterName(30, "S2LensBrigAdj");
        parametersTool.setParameterName(31, "S3LensBrigAdj");
        parametersTool.setParameterName(32, "SLensTopYLimit");
        parametersTool.setParameterName(33, "SLensLowYLimit");
        parametersTool.setParameterName(34, "ColorSTsw");
        parametersTool.setParameterName(35, "ST_Mix_En");
        parametersTool.setParameterName(36, "Show40pointEn");
        parametersTool.setParameterName(37, "doGlobalPhiAdj");
        parametersTool.setParameterName(38, "ALIRelationD0");
        parametersTool.setParameterName(39, "ALIRelationD1");
        parametersTool.setParameterName(40, "ALIRelationD2");
        //parametersTool.setParameterName(41, "GP_CloseRate");
        //parametersTool.setParameterName(42, "GP_FarRate");
        parametersTool.setParameterName(42, "do_Auto_ST");
        parametersTool.setParameterName(43, "LensRateTable[0]");
        parametersTool.setParameterName(44, "LensRateTable[1]");
        parametersTool.setParameterName(45, "LensRateTable[2]");
        parametersTool.setParameterName(46, "LensRateTable[3]");
        parametersTool.setParameterName(47, "LensRateTable[4]");
        parametersTool.setParameterName(48, "LensRateTable[5]");
        parametersTool.setParameterName(49, "LensRateTable[6]");
        parametersTool.setParameterName(50, "LensRateTable[7]");
        parametersTool.setParameterName(51, "LensRateTable[8]");
        parametersTool.setParameterName(52, "LensRateTable[9]");
        parametersTool.setParameterName(53, "LensRateTable[10]");
        parametersTool.setParameterName(54, "LensRateTable[11]");
        parametersTool.setParameterName(55, "LensRateTable[12]");
        parametersTool.setParameterName(56, "LensRateTable[13]");
        parametersTool.setParameterName(57, "LensRateTable[14]");
        parametersTool.setParameterName(58, "LensRateTable[15]");
        parametersTool.setParameterName(59, "ShowSensor");
        parametersTool.setParameterName(60, "SensorXStep");
        parametersTool.setParameterName(61, "SensorLensRate");
        parametersTool.setParameterName(62, "S2_R_Gain");
        parametersTool.setParameterName(63, "S2_G_Gain");
        parametersTool.setParameterName(64, "S2_B_Gain"); 
        parametersTool.setParameterName(65, "S2_A_Gain");
        parametersTool.setParameterName(66, "S2_D_Gain");
        parametersTool.setParameterName(67, "RXDelaySetF0");
        parametersTool.setParameterName(68, "RXDelaySetF1");
        parametersTool.setParameterName(69, "S2_Focus_Mode");
        parametersTool.setParameterName(70, "WriteRegSID");
        parametersTool.setParameterName(71, "WriteRegIdx");
        parametersTool.setParameterName(72, "Show_ST_Line");
        parametersTool.setParameterName(73, "S_X_Offset_1");
        parametersTool.setParameterName(74, "S_Y_Offset_1");
        parametersTool.setParameterName(75, "S_X_Offset_2");
        parametersTool.setParameterName(76, "S_Y_Offset_2");
		parametersTool.setParameterName(77, "S_X_Offset_3");
        parametersTool.setParameterName(78, "S_Y_Offset_3");
        parametersTool.setParameterName(79, "S_X_Offset_4");
        parametersTool.setParameterName(80, "S_Y_Offset_4");
        parametersTool.setParameterName(81, "S_X_Offset_6");
        parametersTool.setParameterName(82, "S_Y_Offset_6");
        parametersTool.setParameterName(83, "Do_Auto_Stitch");
        parametersTool.setParameterName(84, "AutoSTsw");
        parametersTool.setParameterName(85, "ALIGlobalPhi3Idx");
        parametersTool.setParameterName(86, "Smooth_Debug_Flag");
        parametersTool.setParameterName(87, "Smooth_Avg_Weight");
        parametersTool.setParameterName(88, "Smooth_O_Idx");
        parametersTool.setParameterName(89, "Smooth_O_Y_Value");
        parametersTool.setParameterName(90, "Smooth_O_U_Value");
        parametersTool.setParameterName(91, "Smooth_O_V_Value");
        parametersTool.setParameterName(92, "Smooth_O_Z_Value");
        parametersTool.setParameterName(93, "Smooth_M_Weight");
        parametersTool.setParameterName(94, "Smooth_Level");
        parametersTool.setParameterName(95, "Smooth_Weight_Th");
        parametersTool.setParameterName(96, "Smooth_Low_Level");
        parametersTool.setParameterName(97, "ShowSmoothType");
        parametersTool.setParameterName(98, "ShowSmoothNum");
        parametersTool.setParameterName(99, "ShowSmoothMode");
        parametersTool.setParameterName(100, "Smooth_XY_Space");
        parametersTool.setParameterName(101, "Smooth_FarWeight");
        parametersTool.setParameterName(102, "Smooth_DelSlope");
        parametersTool.setParameterName(103, "Smooth_Function");
        parametersTool.setParameterName(104, "ShowStitchMode");
        parametersTool.setParameterName(105, "ShowStitchType");
        parametersTool.setParameterName(106, "Show_FX_DDR");
        parametersTool.setParameterName(107, "Get_12K_ST_Line");
        parametersTool.setParameterName(108, "NR3D_Leve");
        parametersTool.setParameterName(109, "Save_Smooth_En");
        parametersTool.setParameterName(110, "ST_Cmd_Mode");
        parametersTool.setParameterName(111, "do_Sensor_Reset");
        parametersTool.setParameterName(112, "TranspareentEn");
        parametersTool.setParameterName(113, "ISP2_YR");
        parametersTool.setParameterName(114, "ISP2_YG");
        parametersTool.setParameterName(115, "ISP2_YB");
        parametersTool.setParameterName(116, "ISP2_UR");
        parametersTool.setParameterName(117, "ISP2_UG");
        parametersTool.setParameterName(118, "ISP2_UB");
        parametersTool.setParameterName(119, "Smooth_O_Idx");
        parametersTool.setParameterName(120, "Focus_Tool_Num");
        parametersTool.setParameterName(121, "Focus_Mode2");
        parametersTool.setParameterName(122, "BatteryWorkR");
        parametersTool.setParameterName(123, "GammaLineOff");
        parametersTool.setParameterName(124, "RGB_Offset_I");
        parametersTool.setParameterName(125, "RGB_Offset_O");
        parametersTool.setParameterName(126, "WDR1PI0");
        parametersTool.setParameterName(127, "WDR1PI1");
        parametersTool.setParameterName(128, "WDR1PV0");
        parametersTool.setParameterName(129, "WDR1PV1");
        parametersTool.setParameterName(130, "Add_Mask_X");
        parametersTool.setParameterName(131, "Add_Mask_Y");
        parametersTool.setParameterName(132, "Add_Pre_Mask_X");
        parametersTool.setParameterName(133, "Add_Pre_Mask_Y");
        parametersTool.setParameterName(134, "DG_Offset[0]");
        parametersTool.setParameterName(135, "DG_Offset[1]");
        parametersTool.setParameterName(136, "DG_Offset[2]");
        parametersTool.setParameterName(137, "DG_Offset[3]");
        parametersTool.setParameterName(138, "DG_Offset[4]");
        parametersTool.setParameterName(139, "DG_Offset[5]");
        parametersTool.setParameterName(140, "XXXX");
        parametersTool.setParameterName(141, "XXXX");
        parametersTool.setParameterName(142, "XXXX");
        parametersTool.setParameterName(143, "XXXX");
        parametersTool.setParameterName(144, "XXXX");
        parametersTool.setParameterName(145, "WDRTablePixel");
        parametersTool.setParameterName(146, "WDR2PI0");
        parametersTool.setParameterName(147, "WDR2PI1");
        parametersTool.setParameterName(148, "WDR2PV0");
        parametersTool.setParameterName(149, "WDR2PV1");
        parametersTool.setParameterName(150, "Focus_Sensor");
        parametersTool.setParameterName(151, "Focus_Idx");
        parametersTool.setParameterName(152, "Focus_Offset_X");
        parametersTool.setParameterName(153, "Focus_Offset_Y");
        parametersTool.setParameterName(154, "Focus_EP_Idx");
        parametersTool.setParameterName(155, "Focus_Gain_Idx");
        parametersTool.setParameterName(156, "F0_Speed");
        parametersTool.setParameterName(157, "F1_Speed");
        parametersTool.setParameterName(158, "F2_Speed");
        parametersTool.setParameterName(159, "Gamma_Adj_A");
        parametersTool.setParameterName(160, "Gamma_Adj_B");
        parametersTool.setParameterName(161, "Y_Off_Value");
        parametersTool.setParameterName(162, "Saturation_C");
        parametersTool.setParameterName(163, "Saturation_Ku");
        parametersTool.setParameterName(164, "Saturation_Kv");
        parametersTool.setParameterName(165, "SL_Adj_Gap0");
        parametersTool.setParameterName(166, "S_Lens_D_E");
        parametersTool.setParameterName(167, "S_Lens_End");
        parametersTool.setParameterName(168, "S_Lens_C_Y");
        parametersTool.setParameterName(169, "Saturation_Th");
        parametersTool.setParameterName(170, "AEG_B_Exp_1Sec");
        parametersTool.setParameterName(171, "AEG_B_Exp_Gain");
        parametersTool.setParameterName(172, "AGainOffset");
        parametersTool.setParameterName(173, "Adj_R_sY");
        parametersTool.setParameterName(174, "Adj_G_sY");
        parametersTool.setParameterName(175, "Adj_B_sY");
        parametersTool.setParameterName(176, "Adj_R_nX");
        parametersTool.setParameterName(177, "Adj_G_nX");
        parametersTool.setParameterName(178, "Adj_B_nX");
        parametersTool.setParameterName(179, "BD_TH_Debug");
        parametersTool.setParameterName(180, "AWB_TH_Max");
        parametersTool.setParameterName(181, "AWB_TH_Min");
        parametersTool.setParameterName(182, "AWB_CrCb_G");
        parametersTool.setParameterName(183, "RGB_Matrix_RR");
        parametersTool.setParameterName(184, "RGB_Matrix_RG");
        parametersTool.setParameterName(185, "RGB_Matrix_RB");
        parametersTool.setParameterName(186, "RGB_Matrix_GR");
        parametersTool.setParameterName(187, "RGB_Matrix_GG");
        parametersTool.setParameterName(188, "RGB_Matrix_GB");
        parametersTool.setParameterName(189, "RGB_Matrix_BR");
        parametersTool.setParameterName(190, "RGB_Matrix_BG");
        parametersTool.setParameterName(191, "RGB_Matrix_BB");  
        parametersTool.setParameterName(192, "2YUV_Matrix_YR");
        parametersTool.setParameterName(193, "2YUV_Matrix_YG");
        parametersTool.setParameterName(194, "2YUV_Matrix_YB");
        parametersTool.setParameterName(195, "2YUV_Matrix_UR");
        parametersTool.setParameterName(196, "2YUV_Matrix_UG");
        parametersTool.setParameterName(197, "2YUV_Matrix_UB");
        parametersTool.setParameterName(198, "2YUV_Matrix_VR");
        parametersTool.setParameterName(199, "2YUV_Matrix_VG");
        parametersTool.setParameterName(200, "2YUV_Matrix_VB");
        parametersTool.setParameterName(201, "HDR_Tone");
        parametersTool.setParameterName(202, "Live_Quality_Mode");
        parametersTool.setParameterName(203, "Gamma_Max");
        parametersTool.setParameterName(204, "WDR_Pixel");
        parametersTool.setParameterName(205, "XXXX");
        parametersTool.setParameterName(206, "XXXX");
        parametersTool.setParameterName(207, "do_Defect");
        parametersTool.setParameterName(208, "Defect_Th");
        parametersTool.setParameterName(209, "Defect_X");
        parametersTool.setParameterName(210, "Defect_Y");
        parametersTool.setParameterName(211, "Defect_En");
        parametersTool.setParameterName(212, "Defect_Debug_En");
        parametersTool.setParameterName(213, "GainI_R");
        parametersTool.setParameterName(214, "GainI_G");
        parametersTool.setParameterName(215, "GainI_B");
        parametersTool.setParameterName(216, "HDR_Auto_Th[0]");
        parametersTool.setParameterName(217, "HDR_Auto_Th[1]");
        parametersTool.setParameterName(218, "HDR_Auto_Target[0]");
        parametersTool.setParameterName(219, "HDR_Auto_Target[1]");
        parametersTool.setParameterName(220, "Removal_Distance");
        parametersTool.setParameterName(221, "Removal_Divide");
        parametersTool.setParameterName(222, "Removal_Compare");
        parametersTool.setParameterName(223, "Removal_Debug_Img");
        parametersTool.setParameterName(224, "Motion_Th");
        parametersTool.setParameterName(225, "Motion_Diff_Pix");
        parametersTool.setParameterName(226, "Overlay_Mul");
        parametersTool.setParameterName(227, "DeGhost_Th");
        parametersTool.setParameterName(228, "Color_Rate_0");
        parametersTool.setParameterName(229, "Color_Rate_1");
        parametersTool.setParameterName(230, "Read_F0_ADDR");
        parametersTool.setParameterName(231, "Read_F1_ADDR"); 
        parametersTool.setParameterName(232, "Make_DNA");
        parametersTool.setParameterName(233, "Color_Temp_G_Rate");
        parametersTool.setParameterName(234, "Color_Temp_Th");
        parametersTool.setParameterName(235, "ADT_Adj_Idx");
        parametersTool.setParameterName(236, "ADT_Adj_Value");
        parametersTool.setParameterName(237, "ADC_value");
        parametersTool.setParameterName(238, "XXXX");
        parametersTool.setParameterName(239, "XXXX");
        parametersTool.setParameterName(240, "XXXX");
        parametersTool.setParameterName(241, "XXXX");
        parametersTool.setParameterName(242, "XXXX");
        parametersTool.setParameterName(243, "doConvert");
        parametersTool.setParameterName(244, "Muxer_Type");
        parametersTool.setParameterName(245, "3D_Res_Mode");
        parametersTool.setParameterName(246, "Driv_Del");
        parametersTool.setParameterName(247, "Plant_Adj_En");
        parametersTool.setParameterName(248, "Plant_Pan");
        parametersTool.setParameterName(249, "Plant_Tilt");
        parametersTool.setParameterName(250, "Plant_Rotate");
        parametersTool.setParameterName(251, "Plant_Wide");
        parametersTool.setParameterName(252, "3D-Model");
        parametersTool.setParameterName(253, "Noise_TH");
        parametersTool.setParameterName(254, "Adj_Y_target");
        parametersTool.setParameterName(255, "Gamma_Offset[0]");
        parametersTool.setParameterName(256, "Gamma_Offset[1]");

        sensorTool = new ParametersTool(); 
        sensorTool.setParameterName(1, "R1"); 
        sensorTool.setParameterName(2, "R2"); 
        sensorTool.setParameterName(3, "R3"); 
        sensorTool.setParameterName(4, "ZoomX");
        sensorTool.setParameterName(5, "ZoomY");
        sensorTool.setParameterName(6, "P0_X");
        sensorTool.setParameterName(7, "P0_Y");
        sensorTool.setParameterName(8, "P1_X");
        sensorTool.setParameterName(9, "P1_Y");
        sensorTool.setParameterName(10, "P2_X");
        sensorTool.setParameterName(11, "P2_Y");
        sensorTool.setParameterName(12, "P3_X");
        sensorTool.setParameterName(13, "P3_Y");
        sensorTool.setParameterName(14, "P4_X");
        sensorTool.setParameterName(15, "P4_Y");
*/
//tmp    runAdbWifiCmd();

    setSendMcuA64Version(&mSSID[0], &mPwd[0], &mUS363Version[0]);

//tmp    startWebService();


//tmp        if(wifiSerThe != null){
//tmp        	wifiSerThe.UninstallEn = checkIsUserApp();
//tmp        	db_debug("checkIsUserApp=%d", wifiSerThe.UninstallEn);
//tmp        }
    
        // UDP broadcast thread
//tmp        udpBroThe = new UDPBroadcastThread(ssid);
//tmp        udpBroThe.setName("UDPBroadcastThread");
//tmp        udpBroThe.start();

    SetPlaySoundFlag(7);	//playSound(7);	// 最後才播放音效, 避免聲音斷斷續續
}

void startPreview()
{
#if 0 //max+
	EyeseeLinux::Camera* camera =
      EyeseeLinux::CameraFactory::GetInstance()->CreateCamera(
          EyeseeLinux::CAM_NORMAL_0);
	camera->StartPreview();
	camera->ShowPreview();
#endif
}