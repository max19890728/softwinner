/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/us363_camera.h"

#include <pthread.h>
#include <mpi_sys.h>
#include <mpi_venc.h>
#include <common/extension/vector.h>

#include "Device/spi.h"
#include "Device/qspi.h"
#include "Device/US363/us360.h"
#include "Device/US363/us363_para.h"
#include "Device/US363/Kernel/k_spi_cmd.h"
#include "Device/US363/Kernel/FPGA_Pipe.h"
#include "Device/US363/Driver/Lidar/lidar.h"
#include "Device/US363/Driver/MCU/mcu.h"
#include "Device/US363/Driver/Fan/fan.h"
#include "Device/US363/Cmd/spi_cmd.h"
#include "Device/US363/Cmd/fpga_driver.h"
#include "Device/US363/Cmd/Smooth.h"
#include "Device/US363/Cmd/us360_func.h"
#include "Device/US363/Cmd/fpga_download.h"
#include "Device/US363/Cmd/main_cmd.h"
#include "Device/US363/Cmd/defect.h"
#include "Device/US363/Cmd/dna.h"
#include "Device/US363/Cmd/h264_header.h"
#include "Device/US363/Net/ux363_network_manager.h"
#include "Device/US363/Net/ux360_wifiserver.h"
#include "Device/US363/Data/databin.h"
#include "Device/US363/Data/wifi_config.h"
#include "Device/US363/Data/pcb_version.h"
#include "Device/US363/Data/customer.h"
#include "Device/US363/Data/country.h"
#include "Device/US363/Data/us363_folder.h"
#include "Device/US363/Data/us360_config.h"
#include "Device/US363/Data/us360_parameter_tmp.h"
#include "Device/US363/System/sys_time.h"
#include "Device/US363/System/sys_cpu.h"
#include "Device/US363/System/sys_power.h"
#include "Device/US363/Test/test.h"
#include "Device/US363/Test/focus.h"
#include "Device/US363/Media/live360_thread.h"
#include "Device/US363/Media/recoder_thread.h"
#include "Device/US363/Media/Camera/uvc.h"

#include "device_model/display.h"
#include "device_model/media/camera/camera.h"
#include "device_model/media/camera/camera_factory.h"
#include "device_model/storage_manager.h"
//#include "device_model/system/uevent_manager.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363Camera"

//==================== main parameter ====================
char mUS363Version[64] = "v1.00.00\0";          /** VersionStr */

char mWifiSsid[32] = "US_0000\0";               /** ssid */         //檔案的 SSID
char mWifiPassword[16] = "88888888\0";          /** pwd */          //檔案的 Password
char mWifiApSsid[32] = "US_0000\0";             /** wifiAPssid */   //實際設定 WIFI 的 SSID

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
    
int mTimelapseEncodeType = FPGA_VIDEO_ENCODE_TYPE_FPGA_H264;		/** FPGA_Encode_Type */   
    
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
int dnaCheckOk = 0/*, dnaCheckOkLst = -1*/;             //0:err 1:ok  /** dna_check_ok, dna_check_ok_lst */

char thmPath[128];                              /** THM_path */
char dirPath[128];                              /** DIR_path */
char sdPath[128] = "/mnt/extsd";                /** sd_path */
int sdState = 0/*, sdStateLst = -1*/;               /** sd_state, lst_sd_state */
unsigned long long sdFreeSize = 0;                   /** sd_freesize */
unsigned long long sdAllSize = 0;                    /** sd_allsize */
int writeFileError = 0;                       /** write_file_error */

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
unsigned long long lidarDelay = -1;

#define LIDAR_BUFFER_MAX    0x400000            /* 4MB */
int lidarCanBeFind = 1;                         // 0禁止使用 1:正常
int lidarCycleTime = 10;                        /** timerLidarCycle */
int lidarPtrEnable = 0;                         /** LidarPtrEnable */
int lidarBufMod = 1;	                        /** LidarBufMod */   // 0:512x256, 1:1Kx512, 2:2Kx1K
int lidarBufEnd = 2000000;			            /** LidarBufEnd */   // 設定讀取的總數量
int lidarBufPtr = 0;				            /** LidarBufPtr */   // 目前已讀取的數量
int lidarScanCnt = 0;                           /** lidarScanCnt */
int *lidarSinCosBuffer;		                        /** LidarBuffer[LIDAR_BUFFER_MAX] */   // 最大 2M 筆資料 (1K x 2K) x 2 (sin & cos)
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
int uvcErrCount = 0;                          /** UVCErrCount */

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
int isRtmpHandleException = 0;
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
char *rtspBuffer;                               /** rtsp_buff[RTSP_BUFFER_MAX] */  //取得 H264 Data 後, 複製到 Wifi Server 的 rtspBuffer
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
int adjustSensorSyncFlag = 0;		                    /** Adj_Sensor_Sync_Flag */     //0:none 1:doSensorReset 2:doAdjSensorSync 3:doChooseMode

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

int menuFlag = 0;                             /** mMenuFlag */    //長按拍照鍵切換模式
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

int audioRecThreadEn = 0;                       /** isRecording */ //Audio Thread En, 負責接收 Wifi 的聲音
#define AUDIO_REC_EMPTY_BYTES_SIZE   10240
char audioRecEmptyBytes[AUDIO_REC_EMPTY_BYTES_SIZE];

std::vector<long>::iterator audioTimeStamp;          /** ls_audioTS */
std::vector<int>::iterator audioSize;                /** ls_readBufSize */
std::vector<char[]>::iterator audioBuffer;           /** ls_audioBuf */

int micSource = 0;			                    // 0:內部  1:外部
int localAudioRate = 44100, localAudioBits = 16, localAudioChannel = 1;     /** audioRate, audioBit, audioChannel */
int wifiAudioRate = 44100, wifiAudioBits = 16, wifiAudioChannel = 1;        /** wifi_audioRate, wifi_audioBit, wifi_audioChannel */

int audioTestCount = 0;                         //產線測試喇叭用, 播放一段聲音並錄下
int audioTestRecordEn = 0;                      /** audioTestRecording */

enum {
    LIVE360_STATE_STOP  = -1,
    LIVE360_STATE_START = 0
};
int live360En = 0;                              /** mLive360En */
int live360Cmd = 0;                             /** mLive360Cmd */
int live360State = LIVE360_STATE_STOP;          /** Live360_State */    // 0:start -1:stop
unsigned long long live360SendHttpCmdTime1 = 0;               /** Live360_t1 */

int thread_20ms_en = 1;             
int thread_1s_en = 1;
int thread_5ms_en = 1;

pthread_t thread_20ms_id;
pthread_t thread_1s_id;
pthread_t thread_5ms_id;

pthread_mutex_t mut_20ms_buf;
pthread_mutex_t mut_1s_buf;
pthread_mutex_t mut_5ms_buf;

int lockStopRecordEn = 0;                       /** lockRecordEnJNI */  //鎖定開始錄影後, 至少需要等5秒才能結束錄影, 避免快速操作錄影導致出錯
int task5s_lock_flag = 0;
unsigned long long task5s_lock_schedule = 5000000;		//us, 5s
unsigned long long task5s_lock_time;

//==================== get/set =====================
void getWifiSsid(char *ssid) { memcpy(ssid, &mWifiSsid[0], sizeof(mWifiSsid)); }

void getWifiApSsid(char *ssid) { memcpy(ssid, &mWifiApSsid[0], sizeof(mWifiApSsid)); }

void getWifiPassword(char *password) { memcpy(password, &mWifiPassword[0], sizeof(mWifiPassword)); }

void setDoAutoStitchFlag(int flag) { doAutoStitchingFlag = flag; }
int getDoAutoStitchFlag() { return doAutoStitchingFlag; }

//void setResolutionMode(int resolution) { mResolutionMode = resolution; }
//int getResolutionMode() { return mResolutionMode; }

void setResolutionWidthHeight(int resolution) {
    mResolutionWidth   = mResolutionWidthHeight[resolution][0];
    mResolutionHeight  = mResolutionWidthHeight[resolution][1];
}
void getResolutionWidthHeight(int *width, int *height) { 
    *width  = mResolutionWidth;
    *height = mResolutionHeight;
}

//void setPlayMode(int play_mode) { mPlayMode = play_mode; }
int getPlayMode() { return mPlayMode; }

//void setPlayModeTmp(int play_mode) { mPlayModeTmp = play_mode; }
//int getPlayModeTmp() { return mPlayModeTmp; }

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
int getSaturationInitFlag() { return saturationInitFlag; }

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

void setCameraPositionCtrlMode(int ctrl_mode) { mCameraPositionCtrlMode = ctrl_mode; }
int getCameraPositionCtrlMode() { return mCameraPositionCtrlMode; }

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

//void setHdrIntervalEvMode(int hdr_ev_mode) { mHdrIntervalEvMode = hdr_ev_mode; }
int getHdrIntervalEvMode() { return mHdrIntervalEvMode; }

//void setWdrMode(int wdr_mode) { mWdrMode = wdr_mode; }
//int getWdrMode() { return mWdrMode; }

//void setBottomMode(int btm_mode) { mBottomMode = btm_mode; }
//int getBottomMode() { return mBottomMode; }

//void setBottomSize(int size) { mBottomSize = size; }
//int getBottomSize() { return mBottomSize; }

void setBottomTextMode(int btm_text_mode) { 
    mBottomTextMode = btm_text_mode; 
    set_A2K_DMA_BottomTextMode(btm_text_mode);
}
int getBottomTextMode() { return mBottomTextMode; }

//void setTimelapseEncodeType(int enc_type) { mTimelapseEncodeType = enc_type; }
//int getTimelapseEncodeType() { return mTimelapseEncodeType; }

void setFpgaStandbyEn(int en) { 
    fpgaStandbyEn = en; 
    SetFPGASleepEn(fpgaStandbyEn);
}
int getFpgaStandbyEn() { return fpgaStandbyEn; }

void setPowerSavingSetingUiState(int state) { powerSavingSetingUiState = state; }
int getPowerSavingSetingUiState() { return powerSavingSetingUiState; }

void setDrivingRecordMode(int mode) { mDrivingRecordMode = mode; }
int getDrivingRecordMode() { return mDrivingRecordMode; }

void setDebugLogSaveToSDCard(int en) { mDebugLogSaveToSDCard = en; }
int getDebugLogSaveToSDCard() { return mDebugLogSaveToSDCard; }

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

//void setDnaCheckOk(int en) { dnaCheckOk = en; }
//int getDnaCheckOk() { return dnaCheckOk; }

void setThmPath(char *path) { snprintf(thmPath, sizeof(thmPath), "%s\0", path); }
void getThmPath(char *path, int size) { snprintf(path, size, "%s\0", thmPath); }

void setDirPath(char *path) { snprintf(dirPath, sizeof(dirPath), "%s\0", path); }
void getDirPath(char *path, int size) { snprintf(path, size, "%s\0", dirPath); }

void setSdPath(char *path) { snprintf(sdPath, sizeof(sdPath), "%s\0", path); }
void getSdPath(char *path, int size) { snprintf(path, size, "%s\0", sdPath); }

void setSdState(int state) { sdState = state; }
int getSdState() { return sdState; }

void setWriteUS360DataBinFlag(int flag) { writeUS360DataBinFlag = flag; }
int getWriteUS360DataBinFlag() { return writeUS360DataBinFlag; }

void setWriteFileError(int err) { writeFileError = err; }
int getWriteFileError() { return writeFileError; }

void setFpgaCtrlPower(int ctrl) { fpgaCtrlPower = ctrl; }
int getFpgaCtrlPower() { return fpgaCtrlPower; }

void setAudioRecThreadEn(int en) { audioRecThreadEn = en; }
int getAudioRecThreadEn() { return audioRecThreadEn; }

void setDcState(int state) { dcState = state; }
int getDcState() { return dcState; }

void setSdFreeSize(unsigned long long size) { sdFreeSize = size; }
unsigned long long getSdFreeSize() { return sdFreeSize; }
void subSdFreeSize(int size) { sdFreeSize -= size; }



//==================== fucntion ====================
int malloc_rtsp_buf() {
	//char rtspBuffer[RTSP_BUFFER_MAX];
	rtspBuffer = (char *)malloc(sizeof(char) * RTSP_BUFFER_MAX);
	if(rtspBuffer == NULL) goto error;
	return 0;
error:
	db_error("malloc_rtsp_buf() malloc error!");
	return -1;
}

void free_rtsp_buf() {
	if(rtspBuffer != NULL)
		free(rtspBuffer);
	rtspBuffer = NULL;
}

int malloc_lidar_sin_cos_buf() {
	//int lidarSinCosBuffer[LIDAR_BUFFER_MAX];
	lidarSinCosBuffer = (int *)malloc(sizeof(int) * LIDAR_BUFFER_MAX);
	if(lidarSinCosBuffer == NULL) goto error;
	return 0;
error:
	db_error("malloc_lidar_buf() malloc error!");
	return -1;
}

void free_lidar_sin_cos_buf() {
	if(lidarSinCosBuffer != NULL)
		free(lidarSinCosBuffer);
	lidarSinCosBuffer = NULL;
}



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
	sprintf(mWifiApSsid, "%s\0", mWifiSsid);
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
    	setBmxSensorLogEnable(1);
    	break;
    case CUSTOMER_CODE_PIIQ:
		sprintf(name, "peek\0");
		sprintf(bottomFileNameSource, "background_peek\0");
    	//mWifiApSsid = ssid.replace("US_", "peek-HD_");
		ptr = strchr(mWifiSsid, '_');
		sprintf(mWifiApSsid, "peek-HD_%s\0", (ptr+1) );
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
    free_rtsp_buf();
    free_lidar_sin_cos_buf();
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
	if(malloc_us360_buf() < 0) 	       goto end;
	if(malloc_wifiserver_buf() < 0)    goto end;
//  if(malloc_lidar_buf() < 0)	       goto end;			// 記憶體需求過大, 導致程式無法執行
    if(malloc_rtsp_buf() < 0)          goto end;
    if(malloc_lidar_sin_cos_buf() < 0) goto end;

	initSysConfig();

	if(EyeseeLinux::StorageManager::GetInstance()->IsMounted() ) {
		makeUS360Folder();
		makeTestFolder();
		readWifiConfig(&mWifiSsid[0], &mWifiPassword[0]);
	}
	else {
		getNewSsidPassword(&mWifiSsid[0], &mWifiPassword[0]);
	}
	db_debug("US363Camera() ssid=%s pwd=%s", mWifiSsid, mWifiPassword);

    memcpy(&mWifiApSsid[0], &mWifiSsid[0], sizeof(mWifiSsid));
	startWifiAp(&mWifiApSsid[0], &mWifiPassword[0], mWifiChannel, 0);   
	start_wifi_server(8555);
	
	us360_init();   
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
    case 0:  setCameraPositionCtrlMode(CAMERA_POSITION_CTRL_MODE_AUTO); break;
    case 1:  setCameraPositionCtrlMode(CAMERA_POSITION_CTRL_MODE_MANUAL); setCameraPositionMode(CAMERA_POSITION_0);   break;
    case 2:  setCameraPositionCtrlMode(CAMERA_POSITION_CTRL_MODE_MANUAL); setCameraPositionMode(CAMERA_POSITION_180); break;
    case 3:  setCameraPositionCtrlMode(CAMERA_POSITION_CTRL_MODE_MANUAL); setCameraPositionMode(CAMERA_POSITION_90);  break;
    default: setCameraPositionCtrlMode(CAMERA_POSITION_CTRL_MODE_MANUAL); setCameraPositionMode(CAMERA_POSITION_0);   break;
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
    setAegEpFreq(freq);
	
	//TagFanControl = 		28;
	int ctrl = Get_DataBin_FanControl();
	setFanCtrl(ctrl);
	
	//TagSharpness = 		29;
    setStrengthWifiCmd(Get_DataBin_Sharpness() );
	
	//TagUserCtrl30Fps = 	30;
    //mUserCtrl = Get_DataBin_UserCtrl30Fps();
	
	//TagCameraMode = 		31;
    mCameraMode = Get_DataBin_CameraMode();

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
    mBottomMode = Get_DataBin_BottomMode();
	//TagBottomSize = 		52;
    mBottomSize = Get_DataBin_BottomSize();
//tmp    SetBottomValue(mBottomMode, mBottomSize);      //S2:awcodec.c

	//TagHdrEvMode = 		53;
    mHdrIntervalEvMode = Get_DataBin_hdrEvMode();
    switch(mHdrIntervalEvMode) {
    case HDR_INTERVAL_EV_MODE_LOW: 
        mWdrMode = WDR_MODE_LOW; 
        break;
    case HDR_INTERVAL_EV_MODE_MIDDLE:
        mWdrMode = WDR_MODE_MIDDLE; 
        break;
    case HDR_INTERVAL_EV_MODE_HIGH:
        mWdrMode = WDR_MODE_HIGH; 
        break;
    }
    SetWDRLiveStrength(mWdrMode);
	
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
	mTimelapseEncodeType = Get_DataBin_FpgaEncodeType();

	//TagWbRGB =			68;
	//TagContrast =			69;
	int contrast = Get_DataBin_Contrast();
	setContrast(contrast);
	
	//TagSaturation =		70;
	int sv = Get_DataBin_Saturation();
	setSaturation(GetSaturationValue(sv));
	
	//TagFreeCount
    int de;
	calSdFreeSize(&sdFreeSize);
//tmp    getRECTimes(sdFreeSize);
    mFreeCount = Get_DataBin_FreeCount();
    if(mFreeCount == -1){
//tmp    	Set_DataBin_FreeCount(GetSpacePhotoNum() );
    	mFreeCount = Get_DataBin_FreeCount();
    	writeUS360DataBinFlag = 1;
    }else{
//tmp    	int de = mFreeCount - GetSpacePhotoNum();
		de = mFreeCount;
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
    Setting_HDR7P_Proc(Get_DataBin_HdrManual(), Get_DataBin_hdrEvMode());
	
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

void rwWifiData(int rw,int wifiMode) {  
//tmp
/*	if(rw == 0){
		wifiSSID = sharedPreferences.getString("wifiSSID", "");
		wifiPassword = sharedPreferences.getString("wifiPassword", "");
		wifiIP = sharedPreferences.getString("wifiIP", "");
		wifiGateway = sharedPreferences.getString("wifiGateway", "");
		wifiPrefix = sharedPreferences.getString("wifiPrefix", "");
		wifiDns1 = sharedPreferences.getString("wifiDns1", "");
		wifiDns2 = sharedPreferences.getString("wifiDns2", "");
		wifiType = sharedPreferences.getInt("wifiType", 0);
	}else{
		sharedPreferences.edit().putInt("wifiMode", wifiMode).apply();
		sharedPreferences.edit().putString("wifiSSID", wifiSSID).apply();
		sharedPreferences.edit().putString("wifiPassword", wifiPassword).apply();
		sharedPreferences.edit().putInt("wifiType", wifiType).apply();
		sharedPreferences.edit().putString("wifiIP", wifiIP).apply();
		sharedPreferences.edit().putString("wifiGateway", wifiGateway).apply();
		sharedPreferences.edit().putString("wifiPrefix", wifiPrefix).apply();
		sharedPreferences.edit().putString("wifiDns1", wifiDns1).apply();
		sharedPreferences.edit().putString("wifiDns2", wifiDns2).apply();
		Log.d("WifiServer", "wifi data :" + wifiMode + "/" + wifiSSID + "/" + wifiPassword);
	}*/   
}

void AletaS2Init() {  
   	//Sensor
    RXDelaySet(10, 0, 0x1);
    RXDelaySet(14, 0, 0x2);
    RXDelaySet(14, 0, 0x4);
    RXDelaySet(15, 0, 0x8);
    RXDelaySet(12, 1, 0xF);

    adjustSensorSyncFlag = 1;
    AS2MainCMDInit();
    SetPipeReStart();

    sendFpgaCmdStep = 0;
    powerSavingInit = 0;
    SetDefectStep(0);
    SetDefectState(0);
   	setLedBrightness(Get_DataBin_LedBrightness());
   	setOledControl(Get_DataBin_OledControl());
    get_current_usec(&powerSavingInitTime1);  
}

int FPGA_Download() {  
	if(DownloadProc() < 0)
		return -1;
	dnaCheckOk = dnaCheck();
    return 0; 
}

int FPGA_Init() {  
    int ret=0;
    ret = FPGA_Download();
    fpgaCheckSum = GetFpgaCheckSum();    //SetTextFpgaCheckSum_jni();
    AletaS2Init();
    fpgaCmdIdx = -1; fpgaCmdP1 = -1;
    return ret;   
}

void Check_Bottom_File(int mode, int tx_mode, int isInit) {   
//tmp
/*  	int i, cnt;
   	int width=0, height=0, err = 1;
   	String path;
    	
   	//mode: 0:關, 1:延伸, 2:加底圖(default), 3:鏡像, 4:加底圖(user)
   	if( (mode == 0 || mode == 1 || mode == 3) && tx_mode == 0 && isInit == 0) 
   		return;

   	if(mode == 2) cnt = 2;
   	else          cnt = 1;
    	
   	for(i = 0; i < cnt; i++) {
    	if(mode == 2) {
    		if(i == 0) path = "/mnt/sdcard/US360/Background/" + BOTTOM_FILE_NAME_DEFAULT + ".jpg";
    		else	   path = "/mnt/sdcard/US360/Background/" + BOTTOM_FILE_NAME_USER + ".jpg";
    	}
    	else          
    		path = "/mnt/sdcard/US360/Background/" + BOTTOM_FILE_NAME_USER + ".jpg";
    	File file = new File(path);
        if(file.exists()) {
            BitmapFactory.Options options = new BitmapFactory.Options();
            options.inPreferredConfig = Bitmap.Config.RGB_565;
            options.inPurgeable = true;
            options.inInputShareable = true;
            options.inJustDecodeBounds = true;
            BitmapFactory.decodeFile(path, options);
            height = options.outHeight;
            width = options.outWidth;   
            if(width == BOTTOM_S_WIDTH && height == BOTTOM_S_HEIGHT) {
            	err = 0;
            }
            else { 
                err = 1; 
                break; 
            }
        }   
        else { 
            err = 2; 
            break; 
        }
   	}
        
    if(err == 0)
       	SetDecBottomStep(1);
    else {
       	SetDecBottomStep(0);
        	
       	databin.setBottomMode(0);
       	BottomMode = databin.getBottomMode();  
       	SetBottomValue(BottomMode, BottomSize);
        	
       	databin.setBottomTMode(0);
       	BottomTextMode = databin.getBottomTMode();
       	SetBottomTextMode(BottomTextMode);
       	
       	writeUS360DataBin_flag = 1;        	
    }*/  
}

/*
 *  input
 *    sel --  0:power, 1:wifi
 *
 * */
void set_timeout_start(int sel)
{   
    switch(sel){
    case 0: get_current_usec(&checkTimeoutPowerTime1);
//tmp      		if(getMcuVersion() > 12 && getWifiDisableTime() > 0){
//tmp      			setMCULedSetting(MCU_SLEEP_TIMER_START, 0);
//tmp      		}
      		break;    // POWER
    //case 1: get_current_usec(&toutWifiT1);   break;    // WIFI
    case 2: get_current_usec(&checkTimeoutBurstTime1);  break;    // 連拍
    case 3: get_current_usec(&checkTimeoutSelfieTime1);   break;    // 自拍
    case 4: get_current_usec(&checkTimeoutLongKeyTime);  break;    // 長按事件
    case 5: get_current_usec(&checkTimeoutSaveTime1);   break;    // 存檔
    case 6: get_current_usec(&checkTimeoutTakeTime1);   break;    // 拍照
    //case 7: get_current_usec(&toutPowerKey); break;    // power key
    } 
}

void get_timeout_start(int sel, unsigned long long *time)
{   
    switch(sel){
    case 0: *time = checkTimeoutPowerTime1; break;    // POWER
    //case 1: *time = toutWifiT1;   break;    // WIFI
    case 2: *time = checkTimeoutBurstTime1;  break;    // 連拍
    case 3: *time = checkTimeoutSelfieTime1;   break;    // 自拍
    case 4: *time = checkTimeoutLongKeyTime;  break;    // 長按事件
    case 5: *time = checkTimeoutSaveTime1;   break;    // 存檔
    case 6: *time = checkTimeoutTakeTime1;   break;    // 拍照
    //case 7: *time = toutPowerKey; break;    // power key
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
    case 0: t1 = checkTimeoutPowerTime1;     break;
    //case 1: t1 = toutWifiT1;      break;
    case 2: t1 = checkTimeoutBurstTime1;     break;
    case 3: t1 = checkTimeoutSelfieTime1;      break;
    case 4: t1 = checkTimeoutLongKeyTime;     break;
    case 5: t1 = checkTimeoutSaveTime1;      break;
    case 6: t1 = checkTimeoutTakeTime1;      break;
    //case 7: t1 = toutPowerKey;    break;
    }
    if(t1 == 0) return -1;
    if(t2 < t1){
        switch(sel){
        case 0: checkTimeoutPowerTime1 = t2; break;
        //case 1: toutWifiT1  = t2; break;
        case 2: checkTimeoutBurstTime1 = t2; break;
        case 3: checkTimeoutSelfieTime1  = t2; break;
        case 4: checkTimeoutLongKeyTime = t2; break;
        case 5: checkTimeoutSaveTime1  = t2; break;
        case 6: checkTimeoutTakeTime1  = t2; break;
        //case 7: toutPowerKey= t2; break;
        }
    }
    else{
        if( (t2 - t1) >= (msec * 1000) ){
            switch(sel){
            case 0: checkTimeoutPowerTime1 = 0; break;
            //case 1: toutWifiT1  = 0; break;
            case 2: checkTimeoutBurstTime1 = 0; break;
            case 3: checkTimeoutSelfieTime1  = 0; break;
            case 4: checkTimeoutLongKeyTime = 0; break;
            case 5: checkTimeoutSaveTime1  = 0; break;
            case 6: checkTimeoutTakeTime1  = 0; break;
            //case 7: toutPowerKey= 0; break;
            }
            return 1;
        }
    }
    return 0; 
}

int check_power() {   
	if(dcState == 1)
		return 1;
	else {
		if(batteryPower > 6)		//電量大於 6%
			return 1;
		else
			return 0;
	}  
}

void Set_Cap_Rec_Start_Time(unsigned long long time) {  
	powerSavingCapRecStartTime = time;
	powerSavingInit = 1;		//預防開機先拍照      
}

void Set_Cap_Rec_Finish_Time(unsigned long long time, unsigned long long overtime) {   
	powerSavingCapRecFinishTime = time;
	powerSavingOvertime = overtime;
	powerSavingInit = 1;		//預防開機先拍照   
}

void Set_Power_Saving_Wifi_Cmd(unsigned long long time, unsigned long long overtime, int debug) { 
	if(mPowerSavingMode == 1) {
		if(fpgaStandbyEn == 1)
			SetFPGASleepEn(0);
		Set_Cap_Rec_Finish_Time(time, overtime);
	}
    db_debug("Set_Power_Saving_Wifi_Cmd() PWS: debug=%d\n", debug);     
}

void audio_record_thread_release() {  
//tmp    
/*   	if(audioRecThd != null){
      	audioRecThd.interrupt();
       	audioRecThd = null;
    }*/     
}
    
void audio_record_thread_start() {   
//tmp    
/*  	audio_record_thread_release();
    audioRecThd = new AudioRecordThread();
    audioRecThd.setName("audioRecordThread");
   	audioRecThd.start();*/      
}

void doRecordVideo(int enable, int mode, int res, int time_lapse, int hdmi_state) {     
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
    set_timeout_start(0);                // 重新計數
    if(enable == 1 && check_power() == 1){
        // 開始錄影

        if(get_live360_state() == 0)
        	set_live360_state(-1);

        if(getSdState() == 1) {
//            systemlog.addLog("info", System.currentTimeMillis(), "machine", "start REC.", "---");
            if(time_lapse == 0) {
//tmp                ChangeLedMode(1);
                //checkCpuSpeed();
                if(checkMicInterface() == 0){
                  	audioRecThreadEn = 1;
                   	micSource = 0;
                	//audio_record_thread_start();
                }else{
                   	micSource = 1;
                   	//new AudioRecordLineInThread().start();
                }
            }else{
                //changeLedByTime(1,time_lapse);
//tmp             	ChangeLedMode(1);
	        	//audioRate    = getMicRate();
        		//audioChannel = getMicChannel();
        		//audioBit     = getMicBit();
            }
            setRecEn(0, time_lapse, mTimelapseEncodeType);
//tmp            paintOLEDRecording(1);                    // rex+ 151221
            get_current_usec(&nowTime);
            Set_Cap_Rec_Start_Time(nowTime);
            Set_Cap_Rec_Finish_Time(0, 0);
        }
        else {
            usleep(200000);
            stopREC(14);
        }
    }
    else{
    	setRecEn(-1, time_lapse, mTimelapseEncodeType);
//        systemlog.addLog("info", System.currentTimeMillis(), "machine", "stop REC.", "---");
        if(time_lapse == 0 || audioRecThreadEn == 1){
            audioRecThreadEn = 0;
            //audio_record_thread_release();
//tmp            int led_mode = GetLedControlMode();
//tmp            ChangeLedMode(led_mode);
            //checkCpuSpeed();
        }else{
            //isLedControl = false;
//tmp            SetIsLedControl(0);
        }
//tmp        paintOLEDRecording(0);                        // rex+ 151221
        get_current_usec(&nowTime);
        Set_Cap_Rec_Finish_Time(nowTime, POWER_SAVING_CMD_OVERTIME_5S);
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
        doRecordVideo(0, mPlayMode, mResolutionMode, getTimeLapseMode(), hdmi_state);
    }

    //強制設成Global模式
    if(play_mode != PLAY_MODE_GLOBAL) {
        mPlayMode = PLAY_MODE_GLOBAL;
        mPlayModeTmp = PLAY_MODE_GLOBAL;
    }

    //依照CameraMode限制解析度
    if(camera_mdoe == CAMERA_MODE_CAP || camera_mdoe == CAMERA_MODE_HDR || 
       camera_mdoe == CAMERA_MODE_SPORT || camera_mdoe == CAMERA_MODE_SPORT_WDR) {
    	switch(resolution) {
        case RESOLUTION_MODE_12K:
        case RESOLUTION_MODE_8K:
        case RESOLUTION_MODE_6K:
          	mResolutionMode = resolution;
            break;
        default:
         	mResolutionMode = RESOLUTION_MODE_12K;
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
          	mResolutionMode = resolution;
            break;
        default:
         	mResolutionMode = RESOLUTION_MODE_12K;
            break;
        }
        setFPS(50);
    }
    else if(camera_mdoe == CAMERA_MODE_AEB || camera_mdoe == CAMERA_MODE_RAW) {
      	mResolutionMode = RESOLUTION_MODE_12K;
        setFPS(100);
    }
    else if(camera_mdoe == CAMERA_MODE_REC || camera_mdoe == CAMERA_MODE_REC_WDR) {
        switch(resolution) {
        case RESOLUTION_MODE_4K:
        case RESOLUTION_MODE_3K:
        case RESOLUTION_MODE_2K:
          	mResolutionMode = resolution;
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
          	mResolutionMode = RESOLUTION_MODE_4K;
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
           	mResolutionMode = resolution;
            break;
        default:
          	mResolutionMode = RESOLUTION_MODE_6K;
            break;
        }
        setFPS(100);
    }
    else if(camera_mdoe == CAMERA_MODE_REMOVAL) {
    	mResolutionMode = RESOLUTION_MODE_12K;
        setFPS(100);
    }
    else if(camera_mdoe == CAMERA_MODE_3D_MODEL) {
    	mResolutionMode = RESOLUTION_MODE_12K;
        setFPS(100);
    }

    setResolutionWidthHeight(mResolutionMode);
    get_current_usec(&now_time);
    setChooseModeTime(now_time);  
}

void ReStart_Init_Func(int ctrl_pow) {   
	Load_Parameter_Tmp_Proc();
    FPGAdriverInit();
    Check_Bottom_File(mBottomMode, mBottomTextMode, 1);
    fpgaCmdP1 = 0;
    mPlayModeTmp = mPlayMode;
    chooseModeFlag = 1;
	
	int id   = Get_Camera_Id();
	int base = Get_Camera_Base();
    prepareCamera(id, base);              // openUVC, create uvc_thread & rec_thread   
}

void FPGA_Ctrl_Power_Func(int ctrl_pow, int flag) {    
    int ret;
    db_debug("FPGA_Ctrl_Power_Func: ctrl=%d flag=%d\n", ctrl_pow, flag);
    if(ctrl_pow == 0) {        //open
        ret = FPGA_Init();
        if(ret == 0){
            fpgaCtrlPowerService(ctrl_pow);     // FPGA Power ON
            ReStart_Init_Func(ctrl_pow);
            fpgaCtrlPower = ctrl_pow;
        }
        else {
         	fpgaPowerOff();
         	//fpgaCtrlPowerService(1);
          	//fpgaCtrlPower = 1;
        }
    }
    else {                    //close
        fpgaCtrlPowerService(ctrl_pow);
        fpgaCtrlPower = ctrl_pow;
    }   
}

void changeWifiMode(int mode) {     
//tmp    
/*   	if(clientS != null){
		if(!clientS.isClosed()){
			clientS.close();
		}
	}
   	if(mode == 0){
   		SetWifiModeState(0);		//mWifiModeState = 0;
   		wifiChangeStop = 1;
		wifiLinkType = 0;
		set_ipAddrFromAp("");		//ipAddrFromAp = "";
   		paintOLEDWifiSetting(9,wifiSSID.getBytes().length,wifiSSID.getBytes(),"Connecting. ".getBytes().length,"Connecting. ".getBytes());
		paintOLEDWifiIDPW(ssid.getBytes(), ssid.getBytes().length, pwd.getBytes(), pwd.getBytes().length);
		startWifiAP();
   	}else{
   		stopWifiAP();
   		new Thread(new Runnable(){
   			int startError = 0;
           	public void run(){
           		try {        			
           			wifihot.closeWifiAp();
           			wifiLinkType = 0;
           			wifiChangeStop = 0;
           			SetWifiModeState(4);
               		int reTest = 0;
               		int reMessage = 0;
               		while(true){
               			if(!wifi.isWifiEnabled()) {
               				if(reMessage%4 == 0){
               					paintOLEDWifiSetting(0,wifiSSID.getBytes().length,wifiSSID.getBytes(),"Connecting. ".getBytes().length,"Connecting. ".getBytes());
               					wifi.setWifiEnabled(true);
               				}
                       		Log.d("webService", "wifi start Test : "+reTest);
                       	}else{
                       		break;
                       	}
               			reTest = reTest + 1;
               			reMessage = reMessage + 1;
               			if(reTest > 30){
               				Log.d("webService", "wifi not open");
               				startError = 1;
               				paintOLEDWifiSetting(0,wifiSSID.getBytes().length,wifiSSID.getBytes(),"Wifi Open Failed".getBytes().length,"Wifi Open Failed".getBytes());
               				break;
               			}
                   		Thread.sleep(1000);
               		}
               		Log.d("webService", "changeWifiMode() 02-1");   
               		if(wifiPassword.trim().equals("")){
               			wifiLinkType = 2;
               		}
               		Log.d("webService", "changeWifiMode() 02-2  SSID=" + wifiSSID + "  Pwd=" + wifiPassword + "  Type=" + wifiLinkType); 
               		//String newip = wifihot.linkStart(wifiSSID, wifiPassword, wifiLinkType);
               		String newip = wifihot.linkWifi(wifiSSID, wifiPassword, wifiLinkType);
               		Log.d("webService", "changeWifiMode() 02-3");
               		if(startError == 0)
                   	if("".equals(newip)){
                   		/*Log.d("webService", "changeWifiMode() 03-1");
                   		reTest = 0;
                   		while(true){
                   			if(!wifihot.isWifiApEnabled()){
                   				wifihot.stratWifiAp(wifiAPssid, pwd, 3);
                   				Log.d("onvif", "wifi AP start Test : "+reTest);
                   			}else{
                   				paintOLEDWifiSetting(9,wifiSSID.getBytes().length,wifiSSID.getBytes(),"Connecting. ".getBytes().length,"Connecting. ".getBytes());
                   				paintOLEDWifiIDPW(ssid.getBytes(), ssid.getBytes().length, pwd.getBytes(), pwd.getBytes().length);
                   				break;
                   			}
                   			reTest = reTest + 1;
                   			if(reTest > 10){
                   				Log.d("onvif", "wifi AP not open");
                   				startError = 1;
                   				paintOLEDWifiSetting(0,wifiSSID.getBytes().length,wifiSSID.getBytes(),"Connecting Error".getBytes().length,"Connecting Error".getBytes());
                   				break;
                   			}
                       		Thread.sleep(3000);
                   		}
                   		Log.d("webService", "changeWifiMode() 03-2");
                   		*/
/*                   		startError = 1;
                   	}else{
                   		Log.d("webService", "changeWifiMode() 04-1");
                   		reTest = 0;
                   		paintOLEDWifiSetting(0,wifiSSID.getBytes().length,wifiSSID.getBytes(),"Connecting..".getBytes().length,"Connecting..".getBytes());
                   		while(true){
                   			if(wifiChangeStop == 1) break;
                   			if(newip.equals("0.0.0.0")){
                   				newip = getLocalIpStr(context);
                   				Thread.sleep(1000);
                   				reTest = reTest + 1;
                       			if(reTest > 15){
                       				Log.d("webService", "wifi not open");
                       				if(wifiLinkType == 0){
                       					wifiLinkType = 1;
                                   		wifihot.linkWifi(wifiSSID, wifiPassword,wifiLinkType);
                       					reTest = 0;
                       				}else{
                       					startError = 1;
                           				paintOLEDWifiSetting(0,wifiSSID.getBytes().length,wifiSSID.getBytes(),"Connecting Error".getBytes().length,"Connecting Error".getBytes());
                       					break;
                       				}
                       			}
                   			}else{
                   				set_ipAddrFromAp(newip);		//ipAddrFromAp = newip;
                   				paintOLEDWifiSetting(1,wifiSSID.getBytes().length,wifiSSID.getBytes(),newip.getBytes().length,newip.getBytes());
                   				SetWifiModeState(1);		//mWifiModeState = 1;
                   				break;
                   			}
                   		}
                   		Log.d("webService", "changeWifiMode() 04-2");
                   	}
               		if(startError == 1 && wifiChangeStop == 0){
                   		SetWifiModeState(2);		//mWifiModeState = 0;
               			wifiLinkType = 0;
               			set_ipAddrFromAp("");		//ipAddrFromAp = "";
                   		//paintOLEDWifiSetting(9,wifiSSID.getBytes().length,wifiSSID.getBytes(),"Connecting Error".getBytes().length,"Connecting Error".getBytes());
               			//paintOLEDWifiIDPW(ssid.getBytes(), ssid.getBytes().length, pwd.getBytes(), pwd.getBytes().length);
               			//startWifiAP();
                   	}
           		} catch (InterruptedException e) {
           			// TODO Auto-generated catch block
           			e.printStackTrace();
           		}
           	}
       	}).start();	
   	}*/  
}

/*
 * 播放音效
 * id :
 *         0 = 拍照,    1 = 三連拍, 2 = 五連拍, 3 = 十連拍, 4 = 開始錄影, 5 = 停止錄影,
 *         6 = 逼聲,    7 = 開機,    8 = 關機,    9 = 兩秒自拍, 10 = 十秒自拍, 11 = 提醒
 */
void playSound(int id){   
	playSoundId = id;
   	/*new Thread(new Runnable(){
		public void run(){
			try{
	            if(mp != null){
	                mp.stop();
	                mp.release();
	                mp = null;
	            }
	            if(speakerMode == 0){  
	            	switch(playSoundId){
		                case 0:     mp = MediaPlayer.create(Main.this, R.raw.shutter_ok);         break;        // 拍照
		                case 1:     mp = MediaPlayer.create(Main.this, R.raw.shutter_03);         break;        // 三連拍
		                case 2:     mp = MediaPlayer.create(Main.this, R.raw.shutter_05);        break;        // 五連拍
		                case 3:     mp = MediaPlayer.create(Main.this, R.raw.shutter_10);        break;        // 十連拍
		                case 4:     mp = MediaPlayer.create(Main.this, R.raw.start_rec);            break;        // 開始錄影
		                case 5:     mp = MediaPlayer.create(Main.this, R.raw.stop_rec);            break;        // 停止錄影
		                case 6:     mp = MediaPlayer.create(Main.this, R.raw.beepbeep);            break;        // 逼聲
		                case 7:     mp = MediaPlayer.create(Main.this, R.raw.power_on);            break;        // 開機
		                case 8:     mp = MediaPlayer.create(Main.this, R.raw.power_off);            break;        // 關機
		                case 9:     mp = MediaPlayer.create(Main.this, R.raw.selftimer_2sec);    break;        // 兩秒自拍
		                case 10:     mp = MediaPlayer.create(Main.this, R.raw.selftimer_10sec);    break;        // 十秒自拍
		                case 11:     mp = MediaPlayer.create(Main.this, R.raw.warning);             break;        // 提醒
		                case 12:    mp = MediaPlayer.create(Main.this, R.raw.test);                 break;        // test audio
		                case 13:    mp = MediaPlayer.create(Main.this, R.raw.selftimer_05sec);      break;        // 5秒自拍
		                case 14:    mp = MediaPlayer.create(Main.this, R.raw.selftimer_20sec);      break;        // 20秒自拍
		            }
			            
		            mp.seekTo(0);
		            mp.start();
	            }
	        
	        } catch (Exception e) { Log.e("Main","playSound error! id=" + playSoundId); }
       	}
    }).start();*/ 
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
		doResizeMode[i] = -1;
        
//tmp    setVersionOLED(mUS363Version.getBytes());

    setCpuFreq(4, CPU_FULL_SPEED);
        
    unsigned long long defaultSysTime = 1420041600000L;                     // 2015/01/01 00:00:00
    unsigned long long nowSysTime;
	get_current_usec(&nowSysTime);
    if(defaultSysTime > nowSysTime){										// lester+ 180207
		setSysTime(defaultSysTime);
    }
//tmp    writeHistory();
	
    ret = ReadTestResult();
    if(ret != 0) 
		setDoAutoStitchFlag(1);

    readWifiConfig(&mWifiSsid[0], &mWifiPassword[0]);
    
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
        
    int wifiMode = 0;   //tmp sharedPreferences.getInt("wifiMode", 0);
    if(wifiMode == 1){
		rwWifiData(0,1);
//tmp        initConnectWifi();
        db_debug("open wifi mode Wifi");
        changeWifiMode(1);
    }else{
        db_debug("open wifi mode AP");      
        startWifiAp(&mWifiApSsid[0], &mWifiPassword[0], mWifiChannel, 0);  //initWifiAP();	// 開啟WIFI AP
    }
//tmp    initLidar();				// rex+ 201021

//tmp    initWifiSerThe();

    //if(isStandby == 0)    
	//	FanCtrlFunc(mFanCtrl);      //max+ S3 沒有風扇
        
    FPGA_Ctrl_Power_Func(0, 0);

    LoadConfig(1);    

    ret = LoadParameterTmp();
    if(ret != 1 && ret != -1) {		//debug, 紀錄S2發生檔案錯誤問題
        db_error("LoadParameterTmp() error! ret=%d\n", ret);   
        WriteLoadParameterTmpErr(ret);
    }

    ModeTypeSelectS2(Get_DataBin_PlayMode(), Get_DataBin_ResoultMode(), getHdmiState(), Get_DataBin_CameraMode());

    int kpixel = ResolutionModeToKPixel(mResolutionMode);
//tmp    setOLEDMainModeResolu(mPlayModeTmp, kpixel);

//tmp    SetOLEDMainType(mCameraMode, GetCaptureCnt(), GetCaptureMode(), getTimeLapseMode(), 0);
//tmp    showOLEDDelayValue(Get_DataBin_DelayValue());      	
		
    setStitchingOut(mCameraMode, mPlayMode, mResolutionMode, getFPS()); 
    LineTableInit();  
        
    for(i = 0; i < 8; i++) 
        writeCmdTable(i, mResolutionMode, getFPS(), 0, 1, 0); 
        
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
                    sdState = 1;
                    sdPath = intent.getData().getPath();
                	byte[] path = new byte[64]; 
                	//getSDPath(path);
                	//sdPath = path.toString();
                    Log.d("Main", "ACTION_MEDIA_MOUNTED, path = " + sdPath);
                    setSDPathStr(sdPath.getBytes(), sdPath.getBytes().length);
                    
                    getOutputPath(); 
                }
                else if(intent.getAction() == Intent.ACTION_MEDIA_UNMOUNTED){ 
                    Log.d("Main", "ACTION_MEDIA_UNMOUNTED");
                    sdState = 0;
                    sdPath = "/mnt/extsd";
                    setSDPathStr(sdPath.getBytes(), sdPath.getBytes().length);
                }
            }
        };*/

    Show_Now_Mode_Message(mPlayMode, mResolutionMode, getFPS(), 0);
        
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

    setSendMcuA64Version(&mWifiSsid[0], &mWifiPassword[0], &mUS363Version[0]);

//tmp    startWebService();


//tmp        if(wifiSerThe != null){
//tmp        	wifiSerThe.UninstallEn = checkIsUserApp();
//tmp        	db_debug("checkIsUserApp=%d", wifiSerThe.UninstallEn);
//tmp        }
    
        // UDP broadcast thread
//tmp        udpBroThe = new UDPBroadcastThread(ssid);
//tmp        udpBroThe.setName("UDPBroadcastThread");
//tmp        udpBroThe.start();

    playSound(7);	// 最後才播放音效, 避免聲音斷斷續續   
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

/*
 * 依照手機是否持續發送同步CMD, 來判斷是否斷線
 */
int Check_Wifi_Connect_isAlive(void) {   
	unsigned long long nowTime;
	get_current_usec(&nowTime);
  	if(powerSavingSendDataStateTime == 0)
   		return 0;

   	if(nowTime < powerSavingSendDataStateTime) nowTime = powerSavingSendDataStateTime;
   	if( (nowTime - powerSavingSendDataStateTime) > POWER_SAVING_CMD_OVERTIME_15S) {
		powerSavingSendDataStateTime = 0;
		return 0;
	}
	return 1;  
}

int checkCanTakePicture()
{   
    unsigned long long now_time, choose_time;
    getChooseModeTime(&choose_time);
    get_current_usec(&now_time);
    if(now_time < choose_time) {                        //防止錯誤
        setChooseModeTime(now_time);
        return 1;
    }
    else if((now_time - choose_time) < 1000000) {       //切換模式後1秒才可拍照
        return 0;
    }
    else {
        return 1;
    }   
}

void LidarSetup(int mode) {   
   	//lidarBufEnd = 0x200000;				// 2M筆資料
   	//lidarBufPtr = 0;
   	switch(mode){
   	case 2:
       	// 0: 2K x 1K = 2M x 4(bytes)
       	lidarBufEnd = 1024*2048;
       	lidarBufPtr = 0;
   		break;
   	case 1:
       	// 1: 1K x 512 = 512K x 4(bytes)
   		lidarBufEnd = 512*1024;
   		lidarBufPtr = 0;
   		break;
   	case 0:
       	// 2: 512 x 256 = 128K x 4(bytes)
   		lidarBufEnd = 256*512;
   		lidarBufPtr = 0;
   		break;
   	}
//tmp   	if(ftdiSPI != null) ftdiSPI.setLidarBufMod(lidarBufMod);
//tmp	if(ftdiSPI != null) ftdiSPI.setLidarMode(mode, lidarBufEnd);
}

void setLidarStart() {    
    LidarSetup(lidarBufMod);
    db_debug("setLidarStart: LidarSetup: Mod=%d  End=%d", lidarBufMod, lidarBufEnd);
    lidarPtrEnable = 1; 
}

int doTakePicture(int enable)
{   
  	int ret = 1, num;
   	unsigned long long free = 0L, nowTime = 0L;
    char sd_path[128];

    //限制DNA錯誤, 不能拍照
   	/*if(dnaCheckOk != 1 && GetTestToolState() == -1) {
   		db_error("doTakePicture() dna check err!\n");
   		paintOLEDCheckError(1);
   		playSound(11);
   		return -1;
   	}*/

    if(get_live360_state() == 0)
    	set_live360_state(-1);

   	if(writeFileError == 1) {
   		playSound(11);
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
        writeUS360DataBinFlag = 1;
        if(getSdState() == 2)
            isNeedNewFreeCount = 1;
    } else {
        setSdState(0);
        db_debug("no SD Card\n");
    }
    set_timeout_start(0);                // 重新計數
    if(getSdState() == 1 || enable == 3 || enable == 4 || enable == 5 || enable == 7 || enable == 8) {
//tmp    	if(getDebugLogSaveToSDCard() == 1){
//tmp    		systemlog.saveDebugLog();
//tmp        }
//tmp    	systemlog.addLog("info", System.currentTimeMillis(), "machine", "doTakePicture", "---");

        if(checkCanTakePicture())
        	ret = setCapEn(enable, getCaptureCnt(), getCaptureIntervalTime());
        else
        	ret = -1;

        get_current_usec(&nowTime);
        if(ret < 0){
//tmp    		paintOLEDSnapshot(1);
    		playSound(11);
        	Set_Cap_Rec_Finish_Time(nowTime, POWER_SAVING_CMD_OVERTIME_5S);
        }else{
//tmp    		paintOLEDSnapshot(1);
    		playSound(0);
        	Set_Cap_Rec_Start_Time(nowTime);
        	Set_Cap_Rec_Finish_Time(0, POWER_SAVING_CMD_OVERTIME_5S);
        	setLidarStart();
        }

//        systemlog.addLog("info", System.currentTimeMillis(), "machine", "do Picture.", String.valueOf(enable));
    }

    return ret;  
}

void startREC(void)
{   
    /*if(dnaCheckOk != 1 && GetTestToolState() == -1) {
    	db_error("startREC() dna check err!\n");
    	paintOLEDCheckError(1);
    	playSound(11);
    	return;
    }*/

    if(writeFileError == 1) {
    	playSound(11);
    	return;
    }

    if(getRecordEn() == 0){                                    // 開始rec動作
        setRecordEn(1);
        setRecordCmdFlag(1);
        playSound(4);

//        systemlog.addLog("info", System.currentTimeMillis(), "machine", "doRecordStart", "---");
    }
}

int stopREC(int debug)
{     
	unsigned long long nowTime;

    if(getRecordEn() == 1){                                    // 結束rec動作
        setRecordEn(0);
        setRecordCmdFlag(1);
        playSound(5);

//tmp        systemlog.addLog("info", System.currentTimeMillis(), "machine", "doRecordStop", "---");
//tmp        if(getDebugLogSaveToSDCard() == 1){
//tmp      		systemlog.writeDebugLogFile();
//tmp        }
        get_current_usec(&nowTime);
        Set_Cap_Rec_Finish_Time(nowTime, POWER_SAVING_CMD_OVERTIME_5S);

//tmp        ls_audioBuf.clear();
//tmp        ls_audioTS.clear();
//tmp        ls_readBufSize.clear();
        return 1;
    }
    else
    	return 0; 
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
    		checkTimeoutBurstCount = cap_cnt;
            setCaptureIntervalTime(200);        // 每秒5張
            set_timeout_start(2);
            ret = doTakePicture(2);
            if(ret < 0)
            	playSound(11);
            else {
	            if     (cap_cnt ==  3) playSound(1);        // mp_shutter_03.start();    //sound.play(au_shutter_03, 1, 1, 0, 0, 1);
	            else if(cap_cnt ==  5) playSound(2);        // mp_shutter_05.start();    //sound.play(au_shutter_05, 1, 1, 0, 0, 1);
	            else if(cap_cnt == 10) playSound(3);        // mp_shutter_10.start();    //sound.play(au_shutter_10, 1, 1, 0, 0, 1);
            }
        }
        else{
        	set_timeout_start(6);
            checkTimeoutTakePicture = 1;
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

void timerLidar() {   
//tmp
/*   	//if(ftdi4222 != null) ftdi4222.onStart();		// show information
   	String hex0, hex1, hex2, hex3, hex4;
    	
   	if(LidarPtrEnable == 1){
   		if(lidarDelay == 0){
   			int delay = readCapturePrepareTime() * 100 + 2000; //readCapturePrepareTime() 單位為100ms
   			lidarDelay = System.currentTimeMillis() + delay;
   			lidarTime1 = System.currentTimeMillis();
   			lidarErr = 0;
   		}else if(lidarDelay > 0){
   			if(System.currentTimeMillis() - lidarDelay > 0){
   				lidarDelay = -1;
   				lidarCode = (int)(System.currentTimeMillis() - lidarTime1);
   			}
   		}else if(LidarBufPtr < LidarBufEnd){
   			//lidarCode = 1;
   			if(ftdiSPI != null) ftdiSPI.onStart();
   			if(ftdiSPI != null) LidarBufPtr = ftdiSPI.doMasterRead(lidarSinCosBuffer);
   			
   			if(LidarBufEnd != 0){
   				int per = (LidarBufEnd - LidarBufPtr) * 100 / LidarBufEnd;
   				if(per == lidarCode){
   					lidarErr += lidarCycleTime;
   					if(lidarErr >= 4000){
   						LidarPtrEnable = 0;
   						lidarCode = -100;
   					}
   				}else{
   					lidarCode = per;
   					lidarErr = 0;
   				}
   			}
   		}
   		else{
   			LidarPtrEnable = 0;			// 讀取資料結束
   			Log.e("timerLidar", "BufPtr="+LidarBufPtr+" BufEnd="+LidarBufEnd);
			for(int i=0; i<32; i+=4){
				hex0 = Integer.toHexString(lidarSinCosBuffer[i+0]);
				hex1 = Integer.toHexString(lidarSinCosBuffer[i+1]);
				hex2 = Integer.toHexString(lidarSinCosBuffer[i+2]);
				hex3 = Integer.toHexString(lidarSinCosBuffer[i+3]);
				hex4 = Integer.toHexString(i);
				Log.e("timerLidar", "rBuffer[0x"+i+"]={"+hex0+","+hex1+","+hex2+","+hex3+"}");
			}
			
			for(int i=LidarBufEnd-32; i<LidarBufEnd; i+=4){
				hex0 = Integer.toHexString(lidarSinCosBuffer[i+0]);
				hex1 = Integer.toHexString(lidarSinCosBuffer[i+1]);
				hex2 = Integer.toHexString(lidarSinCosBuffer[i+2]);
				hex3 = Integer.toHexString(lidarSinCosBuffer[i+3]);
				hex4 = Integer.toHexString(i);
				Log.e("timerLidar", "rBuffer[0x"+hex4+"]={"+hex0+","+hex1+","+hex2+","+hex3+"}");
			}
			//lidarCode = 2;
			//inputSinCosData(lidarSinCosBuffer, LidarBufEnd * 32);
			int LD363_4in1 = 0;
			if(LidarBufMod == 1){
				LD363_4in1 = 1;
			}
			wave8in(lidarSinCosBuffer, LidarBufEnd * 32,ftdiSPI.setup_LD363_ZT,ftdiSPI.setup_LD363_X,ftdiSPI.setup_LD363_Y,LD363_4in1);
			ftdiSPI.onDestroy();
			lidarDelay = 0;
			lidarCode = 200;
   		}
   	}else{
   		if(ftdiSPI != null){
   			if(lidarScanCnt > 99){ //1s
   				lidarScanCnt = 0;
       			ftdiSPI.onStart();
       			int isNeedGetVersion = ftdiSPI.getLidarVersion();
       			if(isNeedGetVersion == 1 && lidarState > 0 ){
       				Log.e("setLidarStart", "LidarSetup: Mod="+LidarBufMod+" End="+LidarBufEnd);
       		    	LidarSetup(LidarBufMod);
       		    	ftdiSPI.onStart();
       		    	ftdiSPI.getLidarVersion();
       				LidarBufPtr = ftdiSPI.doMasterRead(lidarSinCosBuffer);
       			}else{
       				ftdiSPI.onDestroy();
       			}
       			int writest = getLidarWriteState();
       			//Log.d("timerLidar", "getLidarWriteState: " + writest);
       			if(writest != 0){
       				//lidarCode = 3;
       			}else{
       				//lidarCode = 0;
       			}
       		}else{
       			lidarScanCnt++;
       		}
   			lidarDelay = 0;
   		}
   	}*/   
}

/*
 * weber+170203, 判斷是否更新recovery(檔名是否一致)
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
 * weber+170203, 判斷是否執行recovery
 * return:
 *         壓縮檔名稱 , null = no
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
		            setSendMcuA64Version(&mWifiSsid[0], &mWifiPassword[0], &empty_ver[0]);
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

int do_reboot_recovery() {    
//tmp    
/*    String cmd = "reboot recovery";
    try {
        Runtime runtime = Runtime.getRuntime();
        Process process = runtime.exec(cmd);
        //int     exitValue = process.waitFor();
    } catch (Exception e) { 
        return -1; 
    }*/   
}

/*
 * weber+170203, 執行recovery command
 *         zipName : 壓縮檔名稱(需包含.zip)
 * return:
 *         0 = success , -1 = fail
 */
int runRecoveryCmd(char *zipName) {     
	char fileStr[64]  = "/cache/recovery.log\0";
	char fileStr2[64] = "/cache/recovery/command\0";
	char str_tmp[64];
	struct stat sti;
	FILE *fp;

	//寫入檔名
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

	if(do_reboot_recovery() == -1)
    	return -1;

	return 0; 
}

void getSDAllSize(unsigned long long *size) {   
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

void do_power_standby() {     
	db_debug("do_power_standby %d\n", powerMode);
    if(powerMode == 3){
        powerMode = 1;                    // power mem

        stopREC(9);
        get_current_usec(&standbyTime);
//tmp        setLEDLightOn(0,1);
        isStandby = 1;
//tmp        paintOLEDStandby(1);
        SetAxp81xChg(1);
		int speed = 0;
		setFanSpeed(speed);
        setFanRotateSpeed(speed);

        FPGA_Ctrl_Power_Func(1, 3);
//tmp        setOledStandByMode(1);
        if(menuFlag == 1){
          	menuFlag = 0;
//tmp            closeOLEDMenu();
        }
        stopWifiAp();
        setPowerMode(powerMode);

        if(get_live360_state() == 0)
        	set_live360_state(-1);

//        systemlog.addLog("info", System.currentTimeMillis(), "machine", "system is standby.", "---");
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

void deleteSkipWatchDogFile(void) {    
	char fileStr[64] = "/mnt/sdcard/US360/skipWatchDog.bin\0";
	struct stat sti;
    if(stat(fileStr, &sti) == 0) {
        remove(fileStr);
    }   
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
    mPlayMode = mode;
    focusVisibilityMode = 0;

    db_debug("choose_mode() res_mode=%d fps=%d\n", res_mode, fps);
    setStitchingOut(c_mode, 0, res_mode, fps);
    MakeH264DataHeaderProc();
    adjustSensorSyncFlag = 3;
    get_Stitching_Out(&mode_tmp, &res_tmp);
    for(i = 0; i < 8; i++)
        writeCmdTable(i, res_tmp, fps, 0, 0, 0);
    //PlayMode = 0;

    if(c_mode == CAMERA_MODE_M_MODE && GetDefectState() == 1)
    	set_A2K_ISP2_Defect_En(1);
    else
    	set_A2K_ISP2_Defect_En(0);   
}

void Send_ST_Cmd_Proc(void) {   
	SendSTCmd();
	doCheckStitchingCmdDdrFlag = 0;   
}

int do_Auto_Stitch_Proc(int debug)
{    
   	int ret = -1;

   	db_debug("do_Auto_Stitch() flag=%d\n", doAutoStitchingFlag);
   	if(doAutoStitchingFlag == 1) {
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
		doAutoStitchingFlag = 0;
   	}
   	else ret = -1;

   	return ret;    
}

/*
 * sensor:
 * 0~4: 顯示4K單一 Sensor
 * -1:
 * -2:	顯示4K每顆 Sensor 的垂直縫合邊
 * -3:
 * -4:	顯示12K 九宮格, 調Focus用
 * -5:
 * -6:	顯示12K 4分割, 調Focus用
 * -8:	顯示FX DDR位址
 * -9:
 * -99:	顯示F2 DDR位址
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

void Set_OLED_MainType(int c_mode, int cap_cnt, int cap_mode, int tl_mode, int debug) {   
#ifdef __CLOSE_CODE__	//tmp	
   	switch(c_mode) {    // 相機模式, 0:Cap 1:Rec 2:TimeLapse 3:HDR(3P) 4:RAW(5P) 5:WDR 6:Night 7:NightWDR
   	case 0:                             // capture
       	if(cap_cnt != 0)       setOLEDMainType(3);    // 連拍
       	else if(cap_mode == 0) setOLEDMainType(2);    // 正常            // photo
       	else                   setOLEDMainType(2);    // 自拍
       	break;
   	case 1:                             // record
   	case 2:                             // time lapse
       	if(tl_mode == 0) setOLEDMainType(0);       // rex+ 160302
       	else             setOLEDMainType(1);
       	break;
   	case 3: setOLEDMainType(5); break;  // AEB
   	case 4: setOLEDMainType(6); break;  // RAW(5P)
   	case 5: setOLEDMainType(7); break;  // HDR
    case 6: setOLEDMainTypesetOLEDMainType(8); break;  // Night, 暫時181106
    case 7: setOLEDMainType(9); break;  // NightHDR, 暫時181106
    case 8:  setOLEDMainType(10); break;  // Sport
    case 9:  setOLEDMainType(11); break;  // SportWDR
    case 10: setOLEDMainType(12); break;  // RecWDR
    case 11: setOLEDMainType(13); break;  // TimeLapseWDR
    case 12: setOLEDMainType(14); break;  // B快門
    case 13: setOLEDMainType(15); break;  // remove
    case 14: setOLEDMainType(15); break;  // 3D Model
   	}
    showTimeLapse(LapseModeToSec(Get_DataBin_TimeLapseMode() ), CaptureModeToSec(Get_DataBin_CaptureMode() ), Get_DataBin_CaptureCnt() );
#endif	//__CLOSE_CODE__
}

void Change_Cap_12K_Mode() {    
    if(mCameraMode != 0 || mResolutionMode != RESOLUTION_MODE_12K || mPlayModeTmp !=0) {	//Change Mode: Cap 12K Mode
        mCameraMode = 0;
        mResolutionMode = 1;
        Set_OLED_MainType(mCameraMode, getCaptureCnt(), getCaptureMode(), getTimeLapseMode(), 13);
        mPlayModeTmp = 0;
        ModeTypeSelectS2(mPlayModeTmp, mResolutionMode, getHdmiState(), mCameraMode);
        choose_mode(mCameraMode, mPlayModeTmp, mResolutionMode, getFPS());
    }    
}

void Change_Raw_12K_Mode() {    
    if(mCameraMode != 4 || mResolutionMode != RESOLUTION_MODE_12K || mPlayModeTmp !=0) {	//Change Mode: Raw 12K Mode
        mCameraMode = 4;
        mResolutionMode = 1;
        Set_OLED_MainType(mCameraMode, getCaptureCnt(), getCaptureMode(), getTimeLapseMode(), 14);
        mPlayModeTmp = 0;
        ModeTypeSelectS2(mPlayModeTmp, mResolutionMode, getHdmiState(), mCameraMode);
        choose_mode(mCameraMode, mPlayModeTmp, mResolutionMode, getFPS());
    }   
}

void check_wifi_connected() {     
    // WIFI連線，不進入standby
    if(Get_WifiServer_Connected() == 1) {
      	set_timeout_start(0);
       	set_timeout_start(1);
    }  
}

void set_webserver_getdata_timeout() {   
//tmp
/*    if(ControllerServer.isWebServiceGetData == 1){
      	ControllerServer.isWebServiceGetData = 0;
      	set_timeout_start(0);						//接收Web資料時重新計時
    }*/   
}

int checkTypesOfCaptureMode() {    
    int c_mode = mCameraMode;
    if(c_mode == CAMERA_MODE_CAP || c_mode == CAMERA_MODE_AEB || c_mode == CAMERA_MODE_RAW || 
       c_mode == CAMERA_MODE_HDR || c_mode == CAMERA_MODE_NIGHT || c_mode == CAMERA_MODE_NIGHT_HDR ||
	   c_mode == CAMERA_MODE_SPORT || c_mode == CAMERA_MODE_SPORT_WDR || c_mode == CAMERA_MODE_M_MODE || 
       c_mode == CAMERA_MODE_REMOVAL || c_mode == CAMERA_MODE_3D_MODEL) {		//拍照模式
        return 1;
	}
    else
        return 0;   
}

void set_eth_connect_timeout() {     
    if(isStandby == 0) {
      	if(ethernetConnectCount >= ETHERNET_CONNECT_MAX) {
//tmp	        if(mEth != null) {
//tmp	            mEth.Check_Ethernet_Connect();
//tmp	            if(mEth.Ethernet_Connect != 0)
//tmp	            	set_timeout_start(0);
//tmp	        }
	        ethernetConnectCount = 1;
     	}
      	else if(ethernetConnectCount < 1) 
            ethernetConnectCount = 1;
       	else 					          
            ethernetConnectCount++;      	
    }   
}

void checkWifiAPLive(){    
//tmp    
/*    static int wifiApRestartCount = 0;
    if(wifiApEn == 1 && !wifi.isWifiApEnabled()) {
        wifiApRestartCount++;
        if(wifiApRestartCount > 10) {
//tmp         	Boolean ret = wifihot.stratWifiAp(wifiAPssid, pwd, 3);
            wifiApRestartCount = 0;
        }
    } 
    else {
        wifiApRestartCount = 0;
    }*/   
}

void check_rtmp_state() {     
    unsigned long long nowTime, vDiffTime, aDiffTime;
    if(rtmpSwitch == 0) {
        get_current_usec(&nowTime);
        vDiffTime = nowTime - sendRtmpVideoTime;
        aDiffTime = nowTime - sendRtmpAudioTime;
        if(rtmpVideoPushReady && rtmpAudioPushReady) {
        	if(vDiffTime > 10000 || aDiffTime > 10000) {
         		db_error("RTMP vedio or audio buffer no send.");
//tmp          		if(!isRtmpHandleException) rtmpHandleException(null);
           	} 
            else {
           		rtmpConnectCount = 1;
           	}
        }
    }   
}

//tmp 
/*String parseSSID(String ssid){
    int startLen = 1;
    int endLen = ssid.length()-1;
    if(startLen < endLen && endLen > 1){
    	ssid = ssid.substring(startLen,endLen);
    }else{
       	ssid = "NULL";
    }
    return ssid;
}*/
    
void check_wifi_state() {    
//tmp    
/*    int state = wifiModeState;
    if(state == 1 || state == 2 || state == 3){
        String nowSSID = parseSSID(wifi.getConnectionInfo().getSSID());
        if((state == 2 || state == 3 || ipAddrFromAp.equals("0.0.0.0")) && !wifiSSID.equals(nowSSID)){
          	db_debug("check_wifi_state: Wifi AP(\"%s\") re-connect", wifiSSID);
           	SetWifiModeState(4);		//mWifiModeState = 4;
           	changeWifiMode(1);
           	set_ipAddrFromAp(intToIpAddr(wifi.getConnectionInfo().getIpAddress() ) );		//ipAddrFromAp = intToIpAddr(wifi.getConnectionInfo().getIpAddress());
        }else if(!wifiSSID.equals(nowSSID)){
          	db_error("Main","Wifi AP(\"%s\") disconnect", wifiSSID);
           	if("unknown ssid".equals(nowSSID) || "NULL".equals(nowSSID)){
           		SetWifiModeState(2);
    			paintOLEDWifiSetting(1,nowSSID.getBytes().length,nowSSID.getBytes(),"Disconnection".getBytes().length,"Disconnection".getBytes());
       		}else if(!wifiSSID.equals(nowSSID)){
       			SetWifiModeState(3);
    			paintOLEDWifiSetting(1,nowSSID.getBytes().length,nowSSID.getBytes(),"WiFi Changed".getBytes().length,"WiFi Changed".getBytes());
       		}
        }
    }*/  
}

void set_wifi_ctemp_tint(int k, int t) {   
    Set_WifiServer_KelvinVal(k);
    Set_WifiServer_KelvinTintVal(t);
    Set_WifiServer_KelvinEn(1);   
}

void set_filetool_sensor() {   
//tmp    
/*    if(getBma2x2ZeroingState() == 2){
      	setBma2x2ZeroingState(3);
       	FileTool fTool = new FileTool();
       	String fileStr = "/mnt/sdcard/US360/SensorBin.txt";
       	int[] data = new int[3];
       	getBma2x2ZeroingData(data);
       	fTool.addData("gsensorInitX", data[0]);
       	fTool.addData("gsensorInitY", data[1]);
       	fTool.addData("gsensorInitZ", data[2]);
       	fTool.saveDataFile(fileStr);
       	fTool.readDataFile(fileStr);
       	Log.d("main", "sensor gsensorInitX:" + fTool.getDataValue("gsensorInitX"));
       	Log.d("main", "sensor gsensorInitY:" + fTool.getDataValue("gsensorInitY"));
       	Log.d("main", "sensor gsensorInitZ:" + fTool.getDataValue("gsensorInitZ"));
    }*/   
}

void Delete_ST_JPEG()  {    
//tmp    
/*   	int i;
   	char path[128];
   	for(i = 0; i < 5; i++) {
   		sprintf(path, "/mnt/sdcard/US360/Test/ISP2_S%d_0.jpg\0", i);
       	File file = new File(path);
       	try {
   			if(file.exists() )
   				file.delete();
       	}catch(Exception ex) { 
       		Log.d("Main", ex.getMessage()); 
       	}
   	}*/  
}

void do_Test_Mode_Func(int m_cmd, int s_cmd) {  
   	int Step, speed;
   	if(m_cmd != 7) return;
   	
   	switch(s_cmd) {
   	case 3:			//G-sensor
   		createTestModeGsensorFile();
   		break;
   	case 4:			//Sdcard
   		createTestModeSdcardFile();
   		break;
   	case 5:			//Focus
       	if(checkGetFocusFile() == 1) {
       		createTestModeFocusFile();
       		deleteGetFocusFile();
   		}
   		break;
   	case 7:			//Fan
   		speed = 99;
        setFanSpeed(speed);
//tmp        setTestModeFanSpeed(speed);    //S2: oled.c
        break;
  	case 9:			//HDMI
   		createTestModeHDMIFile();
   		break;
   	case 10:		//Wifi
        if(checkWifiInterface() == 1)
            createTestModeWifiFile();
        else
            createTestModeWifiErrorFile();
        break;
   	case 12:		//DNA
   		Step = GetTestToolStep();
        if(Step == 1){
            dnaCheckOk = dnaCheck();
            if(dnaCheckOk != 1)
                createTestModeDnaError();
            else
                createTestModeDnaDone();
            SetTestToolStep(2);
        }
  		break;
   	case 98:		//ST
   		Step = GetTestToolStep();
   		if(checkGetStJpgFile() == 1 && Step == 1) {
   			Delete_ST_JPEG();
           	if(checkAutoStitchFile() == 1)
       			createAutoStitchFinishFile();
       		else
       			createAutoStitchErrorFile();
           	SetTestToolStep(2);
   		}
   		break;
   	} 
}

void set_wifi_compass_value(int val) {   
	Set_WifiServer_CompassResultEn(1);
	Set_WifiServer_CompassResultVal(val);  
}

void set_filetool_compass(int *data) {    
//tmp    
/*	FileTool fTool = new FileTool();
	String fileStr = "/mnt/sdcard/US360/CompassBin.txt";
	fTool.addData("compassMaxX", data[0]);
	fTool.addData("compassMaxY", data[1]);
	fTool.addData("compassMaxZ", data[2]);
	fTool.addData("compassMinX", data[3]);
	fTool.addData("compassMinY", data[4]);
	fTool.addData("compassMinZ", data[5]);
	fTool.saveDataFile(fileStr);
	fTool.readDataFile(fileStr);
	Log.d("main", "compass set Max,Min: " +
	fTool.getDataValue("compassMaxX") + "/" +
	fTool.getDataValue("compassMaxY") + "/" +
	fTool.getDataValue("compassMaxZ") + "," +
	fTool.getDataValue("compassMinX") + "/" +
	fTool.getDataValue("compassMinY") + "/" +
	fTool.getDataValue("compassMinZ"));
	set_wifi_compass_value(1);*/   
}

int GetDiskInfo() {     
//tmp    
/*    int i;
    String id;
    StorageManager mStorageManager;
    mStorageManager = context.getApplicationContext().getSystemService(StorageManager.class);

    try {
		Method methodGetDisks = StorageManager.class.getMethod("getDisks");
		List disks = (List) methodGetDisks.invoke(mStorageManager);
			
        //DiskInfo
        Class<?> diskIndoClass = Class.forName("android.os.storage.DiskInfo");
        Method mGetDiskId = diskIndoClass.getMethod("getId");
            
        for(i = 0; i < disks.size(); i++) {
            Parcelable parcelable = (Parcelable) disks.get(i);
            id = (String) mGetDiskId.invoke(parcelable);
            if(id.equals("disk:179,24") )
            	return 1;
        }
            
    } catch (NoSuchMethodException e) {
        e.printStackTrace();
    } catch (InvocationTargetException e) {
        e.printStackTrace();
    } catch (IllegalAccessException e) {
        e.printStackTrace();
    } catch (ClassNotFoundException e) {
		e.printStackTrace();
	}
*/        
    return 0; 
}

void *thread_1s(void *junk)
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
    //nice(-6);    // 調整thread優先權

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
			if((nowTime - live360Time) > 60000000) {		// > 60s, 判斷為斷線
				set_live360_state(-1);
			}
		}
		if(lidarCycleTime == 1000) timerLidar();				// rex+ 201021, 1s

//    	getSaveEn();
//    	mHandler.obtainMessage(SYS_CYCLE_SHOW_NOW).sendToTarget();			// weber+170626

		//writeFileError = get_write_file_error();
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

			writeFileError = 0;

			if(sd_state == 1){
				//if(zip == null){
					do_recovery = checkDoRecovery(&sd_path[0], &zip_name[0]);
				//}
			}
		}
		if(sd_state == 1){
			do_Test_Mode_Func(TestToolCmd.MainCmd, TestToolCmd.SubCmd);
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

		calSdFreeSize(&sdFreeSize);
		getSDAllSize(&sdAllSize);
//tmp   if(GetSpacePhotoNum() <= 3 && sd_state == 1){
//tmp   	isNeedNewFreeCount = 1;
//tmp   }
		if(isNeedNewFreeCount == 1 && getImgReadyFlag() == 1){
			isNeedNewFreeCount = 0;
			if(sd_state == 1){
//tmp    		getRECTimes(sdFreeSize);
			}else{
//tmp    		getRECTimes(0L);
			}
//tmp    	int cnt = GetSpacePhotoNum();
			int cnt = 99;
			Set_DataBin_FreeCount(cnt);
			writeUS360DataBinFlag = 1;
		}

		if(getRecordEn() == 1 && getRecordCmdFlag() == 0){        // SD card full,結束rec動作
			if(get_rec_state() == -2 && lockStopRecordEn == 0){
				stopREC(10);
//tmp            	systemlog.addLog("info", System.currentTimeMillis(), "machine", "doRecordStop", "---");
//tmp            	if(getDebugLogSaveToSDCard() == 1){
//tmp        			systemlog.writeDebugLogFile();
//tmp            	}
			}
		}

		if(getRecordEn() == 1){
//tmp       recTime = getOLEDRecTime();
			set_timeout_start(0);                     // 錄影中，不進入standby

			if(check_power() == 0) {					  // 電量  <= 6% 結束錄影
//tmp        		systemlog.addLog("info", System.currentTimeMillis(), "machine", "doRecordStop", "LowPower RecordStop");
				stopREC(11);
			}
		}
		else{
			recTime = 0;
		}

		check_wifi_connected();

		if(getHdmiConnected() == 1){
			set_timeout_start(0);                    // HDMI插上，不進入standby
			if(powerMode == 3) setHdmiState(1);
			else 			   setHdmiState(0);
		}
		else setHdmiState(0);

		set_webserver_getdata_timeout();

		if(getHdmiState() == 1)
			do_Test_Mode_Func(TestToolCmd.MainCmd, TestToolCmd.SubCmd);

		do_Test_Mode_Func(TestToolCmd.MainCmd, TestToolCmd.SubCmd);

		if(getWifiModeCmd() == 1){              // change: WifiServerThread.java
			db_debug("thread1s() mWifiModeCmd == 1\n");
			setWifiModeCmd(0);
			setWifiModeState(0);
			rwWifiData(1, 0);
            snprintf(mWifpStaIpFromAp, sizeof(mWifpStaIpFromAp), "");
			changeWifiMode(0);
		}else if(getWifiModeCmd() == 2){
			db_debug("thread1s() mWifiModeCmd == 2\n");
			setWifiModeCmd(0);
			rwWifiData(1, mWifiStaReboot);
			changeWifiMode(1);
		}

        //產線測試喇叭用, 播放一段聲音並錄下, 3秒後結束
		if(audioTestRecordEn == 1){
			audioTestCount++;
			if(audioTestCount >= 3){
				audioTestCount = 0;
				audioTestRecordEn = 0;
			}
		}

		if(menuFlag == 1){                            // 進入menu，不進入standby
			set_timeout_start(0);
		}

		if(powerMode == 3){
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
				setPowerMode(powerMode);
			}
			db_debug("a64 should sleep\n");
		}

		set_eth_connect_timeout();

//    	captureDCnt = readCaptureDCnt();
		if(isStandby == 0){
			if(fan_ctrl_cnt >= 2){
				FanCtrlFunc(mFanCtrl);
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

		checkWifiAPLive();

		if(isStandby == 0){
			if(fpgaCtrlPower == 0){
				if(GetUVCfd() < 0){           // MainCmdStart
					if(sendFpgaCmdStep == 3) uvcErrCount++;
					else                     uvcErrCount = 0;
					db_error("00 uvcErrCount=%d ST_Flag=%d\n", uvcErrCount, sendFpgaCmdStep);
				}
				else uvcErrCount = 0;

				if(uvcErrCount >= 2 || (fpgaStandbyEn == 1 && uvcErrCount != 0) ){
					db_error("01 uvcErrCount=%d FPGASleepEn=%d\n", uvcErrCount, fpgaStandbyEn);
					FPGA_Ctrl_Power_Func(1, 1);		// Power OFF
					uvcErrCount = 0;
				}
			}
			else if(/*fpgaCtrlPower != 0 &&*/ downloadFileState != 2){
				FPGA_Ctrl_Power_Func(0, 2);			// Power ON
				uvcErrCount = 0;
			}
		}

		check_rtmp_state();

		check_wifi_state();

		if(kelvin_cnt > 0){
			kelvin_cnt = 0;
			int k = 2000;
			int t = 0;

			k = GetColorTemperature();
			t = GetTint();
			set_wifi_ctemp_tint(k, t);
		}else{
			kelvin_cnt++;
		}

#ifdef __CLOSE_CODE__	//tmp
		if(mBmm050Start > 0){
			if(getBmm050RegulateResult() == -1){
				db_debug("compass regulate failure\n");
				mBmm050Start = 0;
				set_wifi_compass_value(-1);
			}else if(getBmm050RegulateResult() == 1){
				int data[6];
				getBmm050Data(&data[0]);
				Set_DataBin_CompassMaxx(data[0]);
				Set_DataBin_CompassMaxy(data[1]);
				Set_DataBin_CompassMaxz(data[2]);
				Set_DataBin_CompassMinx(data[3]);
				Set_DataBin_CompassMiny(data[4]);
				Set_DataBin_CompassMinz(data[5]);
				writeUS360DataBinFlag = 1;
				mBmm050Start = 0;

				set_filetool_compass(&data[0]);
			}
		}
#endif	//__CLOSE_CODE__	

		if(getCameraPositionCtrlMode() == 1){			//Auto
			float sensorData2[3];
//tmp    	getBma2x2_orientation_data(&sensorData2[0], 1);
			if(mCameraPositionMode == 0) {			//正置
				if(sensorData2[2] > 135 || sensorData2[2] < -135)
					cp_cnt++;
				else
					cp_cnt = 0;
			}
			else {									//倒置
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
				do_Test_Mode_Func(TestToolCmd.MainCmd, TestToolCmd.SubCmd);
			}
		}

		set_filetool_sensor();

		setWifiConnectIsAlive(Check_Wifi_Connect_isAlive());
		if(getWifiConnectIsAlive() == 0) {		// disconnect
			//wifi audio rate init
		}

		if(powerSavingInit == 0) {		//開機/休眠啟動 省電模式Init
			if(mPowerSavingMode == 1) {		//On
				get_current_usec(&powerSavingInitTime2);
				if(getImgReadyFlag() == 1 && get_Init_Gamma_Table_En() == 0 &&
					(powerSavingInitTime2 - powerSavingInitTime1) > POWER_SAVING_INIT_OVERTIME) {		//開機/休眠啟動 等到畫面都ok, 才進入省電狀態
					Set_Cap_Rec_Finish_Time(powerSavingInitTime2, 0);
					powerSavingInit = 1;
				}
			}
			else								//Off
				powerSavingInit = 1;
		}

		if(mPowerSavingMode == 1 && fpgaStandbyEn == 0) {	//省電模式, 喚醒FPGA中  (拍照/錄影期間, 設定參數)
			get_current_usec(&nowTime);
			if(getPowerSavingSetingUiState() == 1 && powerSavingCapRecFinishTime == 0) {		//Wifi太久沒有要求同步資料, 視同斷線, 則進入省電狀態, 預防UI沒有收回直接關閉手機APP
				if(getWifiConnectIsAlive() == 0) {
					Set_Cap_Rec_Finish_Time(nowTime, 0);
					setPowerSavingSetingUiState(0);
				}
			}

			if(powerSavingCapRecStartTime != 0 || powerSavingCapRecFinishTime != 0) {
				if(checkTypesOfCaptureMode()) {		//拍照模式
					if(powerSavingCapRecStartTime != 0 && powerSavingCapRecFinishTime == 0 && CheckSaveJpegCnt() == 0) {		//拍照結束
						Set_Cap_Rec_Finish_Time(nowTime, POWER_SAVING_CMD_OVERTIME_5S);
					}
				}

				if(nowTime < powerSavingCapRecFinishTime) Set_Cap_Rec_Finish_Time(nowTime, POWER_SAVING_CMD_OVERTIME_5S);
//tmp        	if(powerSavingCapRecFinishTime != 0 && sendFpgaCmdStep == 3 && getCameraPositionModeChange() == 0 && GetDecBottomStep() == 0 &&
//tmp        			getHdmiState() == 0 && doCheckStitchingCmdDdrFlag == 1 && (nowTime - powerSavingCapRecFinishTime) > powerSavingOvertime) {
				if(powerSavingCapRecFinishTime != 0 && sendFpgaCmdStep == 3 && getCameraPositionModeChange() == 0 /*&& GetDecBottomStep() == 0*/ &&
						getHdmiState() == 0 && doCheckStitchingCmdDdrFlag == 1 && (nowTime - powerSavingCapRecFinishTime) > powerSavingOvertime) {
					Set_Cap_Rec_Start_Time(0);
					Set_Cap_Rec_Finish_Time(0, POWER_SAVING_CMD_OVERTIME_5S);
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

void System_Exit() {   
//tmp    System.exit(0);
}

void *thread_5ms(void *junk)
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
    //nice(-6);    // 調整thread優先權

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

		if(writeUS360DataBinFlag == 1){
			writeUS360DataBinFlag = 0;
			saveBinStep = 1;
			set_timeout_start(5);
		}
		else if(saveBinStep == 1){
			if(check_timeout_start(5, 1000) == 1){    // 設定1sec後儲存
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
				System_Exit();
		}

		if(dna_check_lst != dnaCheckOk){
			dna_check_lst = dnaCheckOk;
//        	mHandler.obtainMessage(FPGA_SHOW_NOW).sendToTarget();
		}

		//依HDMI狀態, 改變CPU頻率和FPS
		if(getHdmiState() != hdmi_state_lst) {
//tmp       SetRenderEn(getHdmiState());

			if(getHdmiState() == 1) {				//HDMI插上, 喚醒省電狀態
				if(getPowerSavingMode() == 1) {
					if(fpgaStandbyEn == 1) {
						SetFPGASleepEn(0);
						adjustSensorSyncFlag = 2;
					}
					Set_Cap_Rec_Start_Time(0);
					Set_Cap_Rec_Finish_Time(0, POWER_SAVING_CMD_OVERTIME_5S);
				}
			}
			else {								//HDMI拔出, 進入省電狀態
				if(getPowerSavingMode() == 1 && fpgaStandbyEn == 0)
					Set_Cap_Rec_Finish_Time(curTime, 0);
			}

			hdmi_state_lst = getHdmiState();
			setHdmiStateChange(1);
		}

		if(fpgaCtrlPower == 0) {
			do_Defect_Func();
			fpgaCmdIdx = readCmdIdx();
			if(fpgaCmdP1 != fpgaCmdIdx) {

				if(chooseModeFlag == 1) {        // 切換模式
					choose_mode(mCameraMode, mPlayModeTmp, mResolutionMode, getFPS());
					isNeedNewFreeCount = 1;
					chooseModeFlag = 0;
//                	mHandler.obtainMessage(REC_SHOW_NOW_MODE).sendToTarget();
					doCheckStitchingCmdDdrFlag = 0;
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
								do2DNR(1);		//銳利度調至最低

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
							chooseModeFlag = 1;
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
								if(getTesttoolDelayTime(2) < getTesttoolDelayTime(1))   //預防錯誤
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
								if(getTesttoolDelayTime(2) < getTesttoolDelayTime(1))   //預防錯誤 
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
						do_Test_Mode_Func(TestToolCmd.MainCmd, TestToolCmd.SubCmd);
						break;
					case 8:
						if(TestToolCmd.SubCmd == 0) {
							if(TestToolCmd.Step == 1) {
								Set_ISP2_Addr(2, 0, -1);
								writeCmdTable(4, mResolutionMode, getFPS(), 3, 0, 1);
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
								if(getTesttoolDelayTime(2) < getTesttoolDelayTime(1))   //預防錯誤 
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
								if(getTesttoolDelayTime(2) < getTesttoolDelayTime(1))   //預防錯誤 
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
							chooseModeFlag = 1;
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
				fpgaCmdP1 = fpgaCmdIdx;
			}

			if(getRecordCmdFlag() == 1) {
				if(getRecordEn() == 1){
					lockStopRecordEn = 1;
					get_current_usec(&task5s_lock_time);
					task5s_lock_flag = 1;
				}
				doRecordVideo(getRecordEn(), mPlayMode, mResolutionMode, getTimeLapseMode(), getHdmiState());
				setRecordCmdFlag(2);
			}

			if(getRecordCmdFlag() == 2 || get_live360_cmd() == 2) {
				if(audioRecThreadEn == 1)
					audio_record_thread_start();
				else
					audio_record_thread_release();
				if(getRecordCmdFlag() == 2) setRecordCmdFlag(3);
				if(get_live360_cmd() == 2) set_live360_cmd(3);
			}

			switch(sendFpgaCmdStep) {
			case 0:
//        		SetSendSTTime(1);		//Send_ST_T1 = System.currentTimeMillis();
//        		SetSendSTTime(2);		//Send_ST_T2 = System.currentTimeMillis();
				sendFpgaCmdStep = 1;
				break;
			case 1:
//        		SetSendSTTime(1);		//Send_ST_T1 = System.currentTimeMillis();
				if(GetMcuUpdateFlag() == 0) {
					if(do_Auto_Stitch_Proc(0) < 0)
						Send_ST_Cmd_Proc();
				}
				else
					Send_ST_Cmd_Proc();
				sendFpgaCmdStep = 2;
				break;
			case 2:
				get_current_usec(&SendMainCmdPipeT1);
				if(SendMainCmdPipeT1 < SendMainCmdPipeT2) SendMainCmdPipeT1 = SendMainCmdPipeT2;
				if( (SendMainCmdPipeT1 - SendMainCmdPipeT2) > 50000) {
					SendMainCmdPipeT2 = SendMainCmdPipeT1;
					choose_mode(mCameraMode, mPlayModeTmp, mResolutionMode, getFPS());
					SendMainCmdPipe(mCameraMode, getTimeLapseMode(), 1);
					MainCmdStart();

					SendH264EncodeTable();
					setFPGASpeed(0, 0xB80);
					setFPGASpeed(1, 0xB80);
					setFPGASpeed(2, 0xD00);

					//Defect Init
					set_A2K_ISP2_Defect_En(0);
					SetDefectState(0);
					ret = DefectInit();
					if(ret == 0 && mCameraMode == CAMERA_MODE_M_MODE) {
						set_A2K_ISP2_Defect_En(1);
						SetDefectState(1);
					}

					setSaturationInitFlag(1);
					sendFpgaCmdStep = 3;
					uvcErrCount = 0;
					adjustSensorSyncFlag = 0;
				}
				break;
			case 3:
				if(fpgaStandbyEn != 0) break;

				if(doCheckStitchingCmdDdrFlag == 0) {
					CheckSTCmdDDRCheckSum();
					doCheckStitchingCmdDdrFlag = 1;
				}

				get_current_usec(&SensorStateT1);
				if(SensorStateT1 < SensorStateT2) SensorStateT1 = SensorStateT2;
				if( (SensorStateT1 - SensorStateT2) > 1000000) {
					SensorStateT2 = SensorStateT1;
					state = readSensorState();
					if(state == 1) {
						Sensor_State_Cnt++;
						db_error("Cnt=%d ST=%d Flag=%d\n", Sensor_State_Cnt, sendFpgaCmdStep, adjustSensorSyncFlag);
						if(Sensor_State_Cnt >= 3) {
							adjustSensorSyncFlag = 1;
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
				if(adjustSensorSyncFlag == 0 && (AdjSensorSyncT1 - AdjSensorSyncT2) > 3000000) {
					AdjSensorSyncT2 = AdjSensorSyncT1;
					adjustSensorSyncFlag = 2;
				}

				if(getSaturationInitFlag() == 1) {
					setSaturationInitFlag(0);
					SetSaturationValue(0, getSaturation());
				}

				get_current_usec(&SendMainCmdPipeT1);
				if(SendMainCmdPipeT1 < SendMainCmdPipeT2) SendMainCmdPipeT1 = SendMainCmdPipeT2;
				if( (SendMainCmdPipeT1 - SendMainCmdPipeT2) > 50000) {
					SendMainCmdPipeT2 = SendMainCmdPipeT1;
					if(adjustSensorSyncFlag != 0) {
						SendMainCmdPipe(mCameraMode, getTimeLapseMode(), adjustSensorSyncFlag);
						adjustSensorSyncFlag = 0;
					}
					else {
						SendMainCmdPipe(mCameraMode, getTimeLapseMode(), 0);
					}
				}
				break;
			}

		}

		get_current_usec(&nowTime);
		if(task5s_lock_flag == 1 && (nowTime - task5s_lock_time) > task5s_lock_schedule) {		//java: Timer5sLockTask()
			task5s_lock_flag = 0;
			lockStopRecordEn = 0;
		}

		if(Get_WifiServer_Connected() == 1){
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

void *thread_20ms(void *junk)
{    
    static unsigned long long curTime, lstTime=0, runTime, nowTime;
    static int selfTimeMode = 0;
    int selfie_time;
    //nice(-6);    // 調整thread優先權

    while(thread_20ms_en)
    {
        get_current_usec(&curTime);
        if(curTime < lstTime) lstTime = curTime;
        else if((curTime - lstTime) >= 1000000){
            db_debug("thread_20ms: runTime=%d\n", (int)runTime);
            lstTime = curTime;
        }

        selfie_time = getSelfieTime();
        if(checkTimeoutTakePicture > 0){                    // 單張模式
            if(check_timeout_start(6, 500) == 1){
                db_debug("mTakePucture = 0\n");
                checkTimeoutTakePicture = 0;
            }
        }
        else if(checkTimeoutBurstCount > 0){                // 連拍模式
            if(check_timeout_start(2, checkTimeoutBurstCount*getCaptureIntervalTime()) == 1){
                checkTimeoutBurstCount = 0;
            }
        }
        else if(selfie_time > 0){                    // 自拍模式
        	checkTimeoutSelfieSec = selfie_time - 1;
            setSelfieTime(0);
//tmp            paintOLEDSelfTime(checkTimeoutSelfieSec);
            db_debug("selfmode selfTime:%d\n", checkTimeoutSelfieSec);
            set_timeout_start(3);
        }
        else if(checkTimeoutSelfieSec > 0){
            if(check_timeout_start(3, checkTimeoutSelfieSec*1000) == 1){
            	db_debug("selfmode selfFunction in, sd_state=%d\n", getSdState());
                checkTimeoutSelfieSec = 0;
                selfTimeMode = 0;
//tmp                ChangeLedMode(ledControlMode);
                if(getSdState() == 1){
                	Ctrl_Rec_Cap(mCameraMode, getCaptureCnt(), getRecordEn());
                	Set_WifiServer_GetEstimateEn(1);
                }
                else{
                	playSound(11);
                }

            }else{
            	get_current_usec(&nowTime);
				unsigned long long time;
				get_timeout_start(3, &time);
            	long leftTime = (checkTimeoutSelfieSec * 1000000) - (nowTime - time);
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
}

void create_thread_20ms() {  
    pthread_mutex_init(&mut_20ms_buf, NULL);
    if(pthread_create(&thread_20ms_id, NULL, thread_20ms, NULL) != 0) {
        db_error("Create thread_20ms fail !\n");
    }	
}