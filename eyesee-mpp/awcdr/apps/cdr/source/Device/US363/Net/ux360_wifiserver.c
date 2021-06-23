/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Net/ux360_wifiserver.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <fcntl.h> 

#include "Device/US363/us360.h"
//#include "ux360_main.h"
#include "Device/US363/Net/ux363_network_manager.h"
#include "Device/US363/Net/ux360_sock_cmd_header.h"
#include "Device/US363/Net/ux360_sock_cmd_sta_header.h"
#include "Device/US363/Net/ux360_sock_cmd_sta_header2.h"
#include "Device/US363/Net/ux360_sock_cmd_sta.h"
#include "Device/US363/Util/ux360_byteutil.h"
#include "Device/US363/System/sys_time.h"
#include "Device/US363/Util/ux360_list.h"
#include "Device/US363/Debug/fpga_dbt.h"
#include "Device/US363/Debug/fpga_dbt_service.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::WifiServer"


//thread
int nSock_thread_en = 1;
pthread_mutex_t mut_nSock_buf;
pthread_t thread_nSock_id;

int cmd_thread_en = 1;
pthread_mutex_t mut_cmd_buf;
pthread_t thread_cmd_id;

int rtsp_thread_en = 1;
pthread_mutex_t mut_rtsp_buf;
pthread_t thread_rtsp_id;

int webservice_thread_en = 1;
pthread_mutex_t mut_webservice_buf;
pthread_t thread_webservice_id;


int mserverSocket=-1;
//    public SocketCmdStatus socketCmdSt;
int SERVERPORT = 8555; 		// 提供給使用者連線的 SERVERPORT
int BACKLOG = 10; 				// 有多少個特定的連線佇列（pending connections queue）
//    public static int state = 0;
int mConnected = 0;

int mSnapshotEn = -1;				// 控制拍照, -1:normal, 1:enable 
int mCaptureMode = 0;
int mRecordEn = -1;					// 控制攝影, -1:normal, 1:enable, 0:disable
int mMjpegEn = 0;					// 控制Mjpeg是否要送到手機

//char mInputData[0x10000];
unsigned char *mInputData = NULL;
float mSensorVal[4];
float mTouchVal[4];
float mLstPanTilt[2];
int mOutputLength=0, mOutputCnt;
//char mOutputData[JAVA_UVC_BUF_MAX];
unsigned char *mOutputData = NULL;
   
//char gSocketDataQ[0x10000];
unsigned char *gSocketDataQ = NULL;
//char gInputData[0x10000];
unsigned char *gInputData = NULL;
int gSocketOst = 0;
    
int mRTSPEn = 0;
//char rtspBuffer[0x100000];
unsigned char *rtspBuffer = NULL;
int rtspBufLength = 0;
int rtspWidth = 3840;
int rtspHeight = 1920;

char mAdjustValue[4];
int mEV=0xffff, mMF=0xffff, mAdjustEn=0;						// rex+ 151023, 設定、調整參數
int mModeSelectEn=0, mPlayMode, mResoluMode;	// rex+ 151023, 控制mode切換
int mTimeLapseMode=0;
int MFMode, mMF2=0xffff;
int mISOEn=0, ISOValue, mFEn=0, FValue, mWBEn=0, WBMode;
int mTouchScreenEn=0;
int mDemoSpeedEn=0, DemoSpeed, mDemoSpeedStart=0, mDemoSpeedEn_lst=0; 
int mCamSelEn=0, CamMode;
int mSaveToSelEn=0, SaveToSel;
int mDataBinEn=0;
int mSetTime=0;
unsigned long long mSysTime=0;
int mRecStateEn=0;
int mFormatEn=0;
int mDataSwapEn=0;
int mPhotoEn=0;
char mPhotoPath[128];
int mImgEn=0;
int mSyncEn=0;
LINK_NODE *mExistFileName = NULL;	// Vector<String> mExistFileName = new Vector<String>();
//char mImgData[JAVA_UVC_BUF_MAX];
unsigned char *mImgData = NULL;
int mImgLen = -1,mImgTotle = -1;
unsigned long long ImgNowTime, ImgLstTime;
int mDownloadEn=0;
//char mDownloadData[JAVA_UVC_BUF_MAX];
unsigned char *mDownloadData = NULL;
int mDownloadLen = -1, mDownloadTotle = -1;
unsigned long long DownloadNowTime, DownloadLstTime;
LINK_NODE *mDownloadFileName = NULL;    // Vector<String> mDownloadFileName = new Vector<String>();
LINK_NODE *mDownloadFileSkip = NULL;	// Vector<Integer> mDownloadFileSkip = new Vector<Integer>();
int mDeleteEn=0;
LINK_NODE *mDeleteFileName = NULL;    // Vector<String> mDeleteFileName = new Vector<String>();
//    DataDownload datadownload = new DataDownload();
int mCaptureModeEn=0;
int mCapMode=0;
int mTimeLapseEn=0;
int mTimeLapse=0;
int mSharpnessEn=0;
int mSharpness=0;
int mZoneEn=0;				// 設定時區
char wifiZone[16];    //String wifiZone = null;		// 取得WIFI時區
//int mWifiDisableTimeEn=0;	// 設定WIFI Disable時間
//int wifiDisableTime=0;		// 取得欲設定WIFI Disable時間
int mCaptureCntEn=0;		// 設定captureCnt
int captureCnt=0;			// 取得欲設定CaptureCnt
    
int mEthernetSettingsEn = 0;// Ethernet Settings
int mEthernetMode;			// 取得Ethernet Mode
char mEthernetIP[INET6_ADDRSTRLEN];			// 取得Ethernet IP
char mEthernetMask[INET6_ADDRSTRLEN];		// 取得Ethernet Mask
char mEthernetGateway[INET6_ADDRSTRLEN];	// 取得Ethernet Gateway
char mEthernetDNS[INET6_ADDRSTRLEN];		// 取得Ethernet DNS
    
int mChangeFPSEn = 0;		// Change FPS
int mFPS;					// 取得FPS
    
int mMediaPortEn = 0;		// change media port
int mPort;					// 取得 media port
    
int mDrivingRecordEn = 0;	// DrivingRecord
int mDrivingRecord;
    
int mWifiChannelEn = 0;		// wifi channel
int mWifiChannel;
    
int mEPFreqEn = 0;			//EP freq
int mEPFreq;
    
int mFanCtrlEn = 0;			// fan control
int mFanCtrl;
    
int mWifiPwdEn = 0;			// wifi password
char sendWifiPwdValue[8];
    
int mWifiConfigEn = 0;		//modify ssid/password
char mWifiConfigSsid[16];
char mWifiConfigPwd[16];;
    
int sendFeedBackEn = 0;
char sendFeedBackAction[8];
int sendFeedBackValue = 0;
    
int sendWifiQuestEn = 0;
    
int mChangeDebugToolStateEn = 0;
int isDebugToolConnect = 0;
int fromWhereConnect = 0; //0:phone 1:debug tool

int mGetStateToolEn = 0;

int mGetParametersToolEn = 0;

int mGetSensorToolEn = 0;

int mCtrlOLEDEn = 0;
int ctrlOLEDNum = 0;

int mSetParametersToolEn = 0;
int mParametersNum = -1;
int mParametersVal = -1;

int mSetSensorToolEn = 0;
int mSensorToolNum = -1;
int mSensorToolVal = -1;
    
int setOutModePlane0_flag = 0;
    
int mGPSEn = 0;
double mGPSLocation[3];
    
int mAudioEn = 0;
    
int mIdCheckEn = 0;
int isFeedback = 0;
    
int sendCamModeCmd = 1;
int sendThmStatusCmd = 1;
int sendDataStatusCmd = 0;
int sendRtmpStatusCmd = 1;
    
int sendEthStateEn = 0;
    
int mColorSTModeEn = 0;
int mColorSTMode = 1;
    
int mTranslucentModeEn = 0;
int mTranslucentMode = 1;
    
int mAutoGlobalPhiAdjEn = 0;
int mAutoGlobalPhiAdjMode = 0;
    
int mAutoGlobalPhiAdjOneTimeEn = 0;
    
int mHDMITextVisibilityEn = 2;
int mHDMITextVisibilityMode = 1;
    
int mJPEGQualityModeEn = 0;
int mJPEGQualityMode = 0;
    
int mSpeakerModeEn = 0;
int mSpeakerMode = 0;
    
int mLedBrightnessEn = 0;
int mLedBrightness = -1;
    
int mOledControlEn = 0;
int mOledControl = 0;
    
int mResoSaveEn = 0;
int mResoSaveMode = 0;
int mResoSaveData = 0;
    
int mCmcdTimeEn = 0;
int mCmcdTime = 0;
    
int mDelayValEn = 0;
int mDelayVal = 0;
    
int mKelvinEn = 0;
int mKelvinVal = 0;
int mKelvinTintVal = 0;

int mMapItemEn = 0;
int mMapItemStatus = 0;
int mMapItemNumber = 0;
int mMapItemX = 0;
int mMapItemY = 0;

int mRoomDataEn = 0;
int mRoomDataLen = 0;
int mRoomDataVal[180];
    
int mCompassEn = 0;
int mCompassVal = 0;
    
int mCameraModeEn = 0;
int mCameraModeVal = 0;
int mRecordHdrVal = 0;
int mPlayTypeVal = 0;
    
int mPlayModeCmdEn = 0;
    
int mDebugLogModeEn = 0;
int mDebugLogModeVal = 0;
    
int mDatabinSyncEn = 0;
int mDatabinSyncVal = 0;
    
int mBottomModeEn = 0;
int mBottomModeVal = 0;
int mBottomSizeVal = 0;

int bottomEn = 0;
int bottomTotle = 0;
int bottomCount = 0;
//char bottomData[JAVA_UVC_BUF_MAX];
unsigned char *bottomData = NULL;
    
//bottom upload
int mBotmEn=0;
//char mBotmData[JAVA_UVC_BUF_MAX];
unsigned char *mBotmData = NULL;
int mBotmLen = 0, mBotmTotle = -1;
unsigned long long BotmNowTime , BotmLstTime;
    
int mHdrEvModeEn = 0;
int mHdrEvModeVal = 0;
int mHdrEvModeManual = 0;
int mHdrEvModeNumber = 5;
int mHdrEvModeIncrement = 10;
int mHdrEvModeStrength = 70;
int mHdrEvModeTone = 0;
int mHdrEvModeDeghost = 1;
    
int mAebModeEn = 0;
int mAebModeNumber = 5;
int mAebModeIncrement = 10;
    
int mLiveQualityEn = 0;
static int mLiveQualityMode = 1;
    
int mBitrateEn = 0;
int mBitrateMode = 0;
    
int mLiveBitrateEn = 0;
int mLiveBitrateMode = 0;
    
int mRTMPSwitchEn = 0;
int mRTMPSwitchMode = 1;
int mRTMPConfigureEn = 0;
    
int mSensorControlEn = 0;
int mCompassModeVal = 0;
int mGsensorModeVal = 0;
    
int mBottomTextEn = 0;
int mBottomTMode = 0;
int mBottomTColor = 0;
int mBottomBColor = 0;
int mBottomTFont = 0;
int mBottomTLoop = 0;
char mBottomText[128];
    
int mFpgaEncodeTypeEn = 0;
int mFpgaEncodeTypeVal = 0;
    
int mSetWBColorEn = 0;
int mSetWBRVal = 0;
int mSetWBGVal = 0;
int mSetWBBVal = 0;
    
int mSendWBColorEn = 0;
int mSendWBRVal = 0;
int mSendWBGVal = 0;
int mSendWBBVal = 0;
    
int mGetWBColorEn = 0;
    
int mSetContrastEn = 0;
int mSetContrastVal = 0;
    
int mSetSaturationEn = 0;
int mSetSaturationVal = 0;
    
int mWbTouchEn = 0;
int mWbTouchXVal = 0;
int mWbTouchYVal = 0;
    
int mBModeEn = 0;
int mBModeSec = 0;
int mBModeGain = 0;

int mLaserSwitchEn = 0;
int mLaserSwitchVal = 0;
    
int mWifiSsidEn = 0;			// wifi ssid
int sendWifiSsidLen = 7;
char sendWifiSsidValue[20];
    
int sendUninstallEn = 0;
int UninstallEn = 0;
int mUninstalldatesEn = 0;

int sendMacAddressEn = 0;
char wifiMacAddress[24];
char ethMacAddress[24];
    
int mInitializeDataBinEn = 0;
    
int mGetTHMListEn = 0;
int mSendTHMListEn = 0;
int mTHMListSize = 0;
//char mTHMListData[0x20000];
unsigned char *mTHMListData = NULL;
int mL63StatusSize = 0;
//char mL63StatusData[0x20000];
unsigned char *mL63StatusData = NULL;
int mPCDStatusSize = 0;
//char mPCDStatusData[0x20000];
unsigned char *mPCDStatusData = NULL;
int mSendTHMListSize = 0;
char *mSendTHMListData = NULL;
    
int mCompassResultEn = 0;
int mCompassResultVal = 0;
    
int mGetFolderEn = 0;
char mGetFolderVal[128];
    
int mSendFolderEn = 0;
int mSendFolderLen = 0;
long mSendFolderSize = 0;
//char mSendFolderNames[0x20000];
unsigned char *mSendFolderNames = NULL;
//char mSendFolderSizes[0x20000];
unsigned char *mSendFolderSizes = NULL;
    
int mSendHdrDefaultEn = 0;
//HDR預設模式
int hdrDefaultMode[3][4] = {
    {3,15,60,0},		//弱     frameCnt/EV/strength/tone
    {5,10,80,0},		//中
    {7,10,60,0} 		//強
};
    
int mSendToneEn = 0;
int mSendToneVal = 0;
    
int mGetEstimateEn = 0;
int mSetEstimateEn = 0;
unsigned long long mSetEstimateStamp = 0;
int mSetEstimateTime = 0;
int mCaptureEpTime = -1;
unsigned long long mCaptureEpStamp = 0;
    
int mGetDefectivePixelEn = 0;
int mSendDefectivePixelEn = 0;
int mSendDefectivePixelVal = 0;
    
int mSendWbTempVal = 0;
int mSendWbTintVal = 0;
int mSendWbTempEn = 0;
    
int mGetRemoveHdrMode = 1;
int mGetRemoveHdrEv = 10;
int mGetRemoveHdrStrength = 70;
int mGetRemoveHdrEn = 0;
    
int mGetAntiAliasingVal = 0;
int mGetAntiAliasingEn = 0;
    
int mGetRemoveAntiAliasingVal = 0;
int mGetRemoveAntiAliasingEn = 0;
    
int mPowerSavingEn = 0;
int mPowerSavingMode = 0;
    
int mSetingUIEn = 0;
int mSetingUIState = 0;	//0:close	1:open
    
int mDoAutoStitchEn = 0;
int mDoAutoStitchVal = 0;

int mDoGsensorResetEn = 0;
int mDoGsensorResetVal = 0;

//210618 - DBT
int mDbtDdrCmdEn = 0;
int mDbtDdrTotalSize = 0;

int mDbtInputDdrDataEn = 0;
int mDbtInputDdrDataSize = 0;
int mDbtInputDdrDataFinish = 0;

int mDbtOutputDdrDataEn = 0;
int mDbtOutputDdrDataSize = 0;
int mDbtOutputDdrDataOffset = 0;

int mDbtRegCmdEn = 0;
int mDbtInputRegDataFinish = 0;

int mDbtOutputRegDataEn = 0;
    
int sendFeedBackSTEn[3] = {0, 0, 0};			//ArrayList<Integer> sendFeedBackSTEn = new ArrayList<Integer>();
char sendFeedBackCmd[3][4] = {"", "", ""};		//ArrayList<String> sendFeedBackCmd = new ArrayList<String>();
	
	
int antiShakeID0=0, antiShakeID1=0;
#define antiShakeMax	1
float antiShakeVal[2][antiShakeMax];

#define GsensorValMax	16			/*array size : 0~15*/
#define backCount		3			/*從後往回數*/
#define	predictionTime	50			/*ms*/
#define	Threshold		12			/*臨界值*/
LINK_NODE *GsensorVal = NULL;    	//Vector<gsensorVal> GsensorVal = new Vector<gsensorVal>();
    
unsigned long long timestampCur, timestampLst=0;

int canAddValue = 1;

//final protected static char[] hexArray = "0123456789ABCDEF".toCharArray();
unsigned long long nowTime2, lstTime2;
#define SOCKET_DATA_Q_MAX	0x100000
int mSocketOst=0;
//char mSocketDataQ[JAVA_UVC_BUF_MAX];
unsigned char *mSocketDataQ = NULL;
int sockfd=-1; 	// 在 mserverSocket 進行 listen，sockfd 是新的連線
int mSocket=-1, qSocket=-1, gSocket=-1;
unsigned long long lstTimeConnect=0;
int connectID=0;
char mHostName[INET6_ADDRSTRLEN], mGpsHostName[INET6_ADDRSTRLEN], hostNameTmp[INET6_ADDRSTRLEN];
int mThreadCount = 0, qThreadCount = 0;
int touch_pan_tilt_init=0;
unsigned long long recStTime = 0;
unsigned long long originalRtspTimer = 0;

//char qSocketDataQ[0x10000];
unsigned char *qSocketDataQ = NULL;
//char qInputData[0x10000];
unsigned char *qInputData = NULL;

//char sendImgBuf[JAVA_UVC_BUF_MAX];
unsigned char *sendImgBuf = NULL;

unsigned char *inputStreamQ = NULL;	//為了SocketDataQ往前搬移, 因為memcpy同一個陣列APP會崩潰


void initGsensorVal(float *val, unsigned long long time, 
				struct gsensorVal_struct *gsensor) {
    gsensor->pan       = val[0];
    gsensor->tilt      = val[1];
    gsensor->rotate    = val[2];
    gsensor->wide      = val[3];
    gsensor->timestamp = time;
}

void getGsensorValue(char *val, int num, float *out) {
	int i;
    //float out[num];
    for(i = 0; i < num; i++) {
    	//out[i] = ByteBuffer.wrap(val, i*4, 4).getFloat();
		memcpy(&out[i], &val[i*4], 4);
    }
}

void free_mSendTHMListData(void) {
	if(mSendTHMListData != NULL) {
		free(mSendTHMListData);
		mSendTHMListData = NULL;
	}
}

void setSendFeedBackSTEn(int idx, int en) {
	sendFeedBackSTEn[idx] = en;
}

void setSendFeedBackCmd(int idx, char *cmd) {
	sprintf(&sendFeedBackCmd[idx][0], "%s", cmd);
}

void initCtrlFlag(void) {
   	// 設定初始狀態
   	mSocketOst=0;
   	lstTimeConnect = 0;
   	mConnected = 0;
   	mSnapshotEn = -1;
   	mRecordEn = -1;
   	mMjpegEn = 0;
   	mRTSPEn = 0;
   	mImgEn = 0;
   	mImgLen = -1;
	clear_list(mExistFileName); 	//mExistFileName.clear(); 
   	mDownloadEn = 0;
   	mDownloadLen = -1;
   	clear_list(mDownloadFileName); 	//mDownloadFileName.clear();
   	clear_list(mDownloadFileSkip); 	//mDownloadFileSkip.clear();
   	clear_list(mDeleteFileName); 	//mDeleteFileName.clear();
   	printf("initCtrlFlag() initCtrlFlag...\n");
}

void runLiveMode(float *fval) {
	// Pitch 傳入的數值為 +/- 90
	// Roll 傳入的數值為 +/- 180
	int i, run=0;
	antiShakeID0 ++;
	antiShakeID0 &= (antiShakeMax-1);
	antiShakeVal[0][antiShakeID0] = fval[0];
	float abs_tmp=0;
	float num=0, plus=0, minus=0, pcnt=0, mcnt=0;
	for(i = 0; i < antiShakeMax; i++) {
		if     (antiShakeVal[0][i] > 0){ plus += antiShakeVal[0][i]; pcnt++; }
		else if(antiShakeVal[0][i] < 0){ minus += antiShakeVal[0][i]; mcnt++; }
	}
	abs_tmp = abs(minus);
	num = (plus + abs_tmp) / antiShakeMax;
	if(plus > abs_tmp) num = num * -1;
	mLstPanTilt[0] = num;
	run = 1;

		
	float tilt = 180 - abs(fval[2]);			// sensor眼鏡模式
	antiShakeID1 ++; 
	antiShakeID1 &= (antiShakeMax-1);
	antiShakeVal[1][antiShakeID1] = tilt;
	num = 0;
	for(i = 0; i < antiShakeMax; i++){
		num += antiShakeVal[1][i];
	}
	num = num / antiShakeMax;
	mLstPanTilt[1] = num;
	run = 1;

	if(run == 1){
		if(touch_pan_tilt_init == 0) {		//連線一開始 init touch_pan touch_tilt
			touch_pan_tilt_init = 1;
			mModeSelectEn = 2;
//			mPlayMode = Main.PlayMode2;
		}
	    if(mModeSelectEn == 2) mModeSelectEn = 0;
		    
	    if( (mDemoSpeedEn == 0 || mDemoSpeedEn == 1 || mDemoSpeedEn == 2) && mDemoSpeedStart == 1) {
	    	mDemoSpeedStart = 2;
	    }
	}
}

/*
 * 	timestamp計數器
 */
unsigned long long TimestampCounter(int pos){
	get_current_usec(&timestampCur);
  	if(timestampCur < timestampLst)	timestampLst = timestampCur;
   	unsigned long long dt = 0;
   	if(timestampLst != 0){
   		if(pos <= Threshold){
   			dt = (unsigned long long) ((timestampCur - timestampLst) * 1.1);
   		}
   		else{
   			dt = (unsigned long long) ((timestampCur - timestampLst) * 0.9);
   		}    	  
   	}
   	timestampLst = timestampCur;
   	return dt;
}

int getGsensorPrediction(unsigned long long time, int p1, int p2, struct gsensorVal_struct *ret){
    	float value[4];
		struct gsensorVal_struct gVal_p1;
		struct gsensorVal_struct gVal_p2;
		
		if(GsensorVal == NULL) return -1;
		serch_node(GsensorVal, p1, (char*)&gVal_p1, sizeof(gVal_p1));
		serch_node(GsensorVal, p2, (char*)&gVal_p2, sizeof(gVal_p2));
		
    	unsigned long long timestamp1 = gVal_p1.timestamp;
    	unsigned long long timestamp2 = gVal_p2.timestamp;
    	unsigned long long timestamp  = timestamp2 + time; 
    	
    	float pan1 		= gVal_p1.pan;
    	float tilt1 	= gVal_p1.tilt;
    	float rotate1 	= gVal_p1.rotate;
    	float wide1 	= gVal_p1.wide;
    	
    	float pan2 		= gVal_p2.pan;
    	float tilt2 	= gVal_p2.tilt;
    	float rotate2 	= gVal_p2.rotate;
    	float wide2 	= gVal_p2.wide;
    	
    	if(pan1 < -90.0f && pan2 > 90.0f)   				pan1 += 360.0f;
    	else if(pan2 < -90.0f && pan1 > 90.0f)				pan2 += 360.0f;
    	
    	if(tilt1 < -90.0f && tilt2 > 90.0f)   				tilt1 += 360.0f;
    	else if(tilt2 < -90.0f && tilt1 > 90.0f)			tilt2 += 360.0f;
    	
    	unsigned long long dt12 		= timestamp2 - timestamp1;
    	unsigned long long dt			= timestamp  - timestamp1;
    	float dtt12		= dt*1.0f / dt12;
    	
    	float dPan12 	= pan2 - pan1;
    	float dTilt12 	= tilt2 - tilt1;
    	float dRotate12 = rotate2 - rotate1;
    	float dWide12 	= wide2 - wide1;
    	
    	value[0]		= pan1 		+ dPan12 * dtt12;
    	value[1]		= tilt1 	+ dTilt12 * dtt12;
    	value[2]		= rotate1 	+ dRotate12 * dtt12;
    	value[3]		= wide1 	+ dWide12 * dtt12;
    	
    	if(value[0] > 180.0f)			value[0] -= 360.0f;
    	else if(value[0] < -180.0f)		value[0] += 360.0f;
    	
    	if(value[1] > 180.0f)			value[1] -= 360.0f;
    	else if(value[1] < -180.0f)		value[1] += 360.0f;
    	printf("getGsensorPrediction() t1 = %lld , t2 = %lld , t = %lld\n", timestamp1, timestamp2, timestamp);
    	printf("getGsensorPrediction() dt12 = %lld , dt = %lld , dtt12 = %f\n", dt12, dt, dtt12);
    	printf("getGsensorPrediction() pan1 = %f , pan2 = %f , pan = %f\n", pan1, pan2, value[0]);
    	printf("getGsensorPrediction() tilt1 = %f , tilt2 = %f , tilt = %f\n", tilt1, tilt2, value[1]);
    	printf("getGsensorPrediction() -----------------------------------------\n");
    	
    	printf("getGsensorPrediction() [13] = %f\n", pan1);
    	printf("getGsensorPrediction() [15] = %f\n", pan2);
    	printf("getGsensorPrediction() [16] = %f\n", value[0]);
    	
		initGsensorVal(&value[0], timestamp, ret);
    	return 0;
}
    
void setGsensorValue(struct gsensorVal_struct val) {
   	if(canAddValue){
   		canAddValue = 0;
   		int size = get_list_size(GsensorVal);
		LINK_NODE **gVal_p = &GsensorVal;
       	if((size-backCount) > 0){
			remove_node(gVal_p, size-1);
       	}
       	if(size > GsensorValMax){
			remove_node(gVal_p, 0);
       	}
       	size = get_list_size(GsensorVal);
		add_node(GsensorVal, (char*)&val, sizeof(val));
       	size = get_list_size(GsensorVal);
       	if((size-backCount) >= 0){
			struct gsensorVal_struct gtmp;
			getGsensorPrediction(predictionTime, size-backCount, size-1, &gtmp);
			add_node(GsensorVal, (char*)&gtmp, sizeof(gtmp));
       	}
       	size = get_list_size(GsensorVal);
       	canAddValue = 1;
   	}   	
}

/*
    Main.wifiServerCallback wifiSerCB;		//copyRTSP()
    public void setWifiSerCB(Main.wifiServerCallback _wifiSerCB)
    {
    	wifiSerCB = _wifiSerCB;
    }
*/
    
void setDemoSpeesEn(int speed) {
	mDemoSpeedEn = speed;
}
	
void setDemoSpeesEnlst(int speed){
	mDemoSpeedEn_lst = speed;
}

void getSPSPPSLen(char *header, int header_len, int *len) {
	int i;
	//int len[2];
	int pos = 4;								//first 00 00 00 01 => SPS
	int find = 0;
				
	for(i = pos; i < (header_len-4); i++) {		//second 00 00 00 01 => PPS
		if(header[i] == 0x00 && header[i+1] == 0x00
		&& header[i+2] == 0x00 && header[i+3] == 0x01) {
			len[0] = i;
			pos = i + 4;
			find = 1;
			break;
		}
	}
					
	if(find) {
		for(i = pos; i < (header_len-4); i++) {	//third 00 00 00 01 => Frame
			if(header[i] == 0x00 && header[i+1] == 0x00
			&& header[i+2] == 0x00 && header[i+3] == 0x01) {
				len[1] = i;
				break;
			}
		}
	}
}

void macFormat(char *src, int slen, char *tar) {
	int i;
	char *p1, *p2;	
	p1 = src;
	p2 = tar;
	for(i = 0; i < slen; i+=2, p1+=2) {
		memcpy(p2, p1, 2);
		p2 += 2;
		*p2 = '-'; p2++;
	}
}
	
void readMacFiles(void) {
	int len=0;
	FILE *fp=NULL;
	char path1[128] = "/cache/mac_wlan.txt";
	char path2[128] = "/cache/mac_eth.txt";
	char macStr[24];
	struct stat sti;
		
	if (-1 == stat (path1, &sti)) {
        printf("readMacFiles() mac_wlan.txt file not find\n");
    }
  	else {
		fp = fopen(path1, "rt");
    	if(fp != NULL) {
    		memset(&macStr[0], 0, sizeof(macStr));
    		fgets(macStr, sizeof(macStr), fp);
			len = strlen(macStr);
    		if(macStr[0] != 0 && len == 12) {
				memset(&wifiMacAddress[0], 0, sizeof(wifiMacAddress));
				macFormat(&macStr[0], len, &wifiMacAddress[0]);
				printf("readMacFiles() mac_wlan: %s", wifiMacAddress);
    		}
			else
				printf("readMacFiles() mac_wlan error! len=%d", len);
    		fclose(fp);
    	}
	}
	
	if (-1 == stat (path2, &sti)) {
        printf("readMacFiles() mac_eth.txt file not find\n");
    }
	else {
		fp = fopen(path2, "rt");
    	if(fp != NULL) {
    		memset(&macStr[0], 0, sizeof(macStr));
    		fgets(macStr, sizeof(macStr), fp);
			len = strlen(macStr);
    		if(macStr[0] != 0 && len == 12) {
				memset(&ethMacAddress[0], 0, sizeof(ethMacAddress));
				macFormat(&macStr[0], len, &ethMacAddress[0]);
				printf("readMacFiles() mac_eth: %s", ethMacAddress);
    		}
			else
				printf("readMacFiles() mac_eth error! len=%d", len);
    		fclose(fp);
    	}
	}
}

void sigchld_handler(int s) {
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

/** 取得sockaddr，IPv4或IPv6 */
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/** get server socket */
int server_socket_init(int port) {
	struct addrinfo hints, *servinfo, *p;
	struct sigaction sa;
	int yes=1;
	int rv;
    int send_buf_size = 128 * 1024;
    int recv_buf_size = 128 * 1024;
    //struct timeval send_timeout = {1,0}; 
    //struct timeval recv_timeout = {1,0}; 

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		// 不用管是 IPv4 或 IPv6
	hints.ai_socktype = SOCK_STREAM;	// TCP stream sockets
	hints.ai_flags = AI_PASSIVE; 		// 幫我填好我的 IP

	char port_s[8];
	sprintf(port_s, "%d", port);
	if ((rv = getaddrinfo(NULL, port_s, &hints, &servinfo)) != 0) {
		printf("server_socket_init() getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}

	// 以迴圈找出全部的結果，並綁定（bind）到第一個能用的結果
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((mserverSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			printf("server_socket_init() server: socket\n");
			continue;
		}

		if (setsockopt(mserverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			printf("server_socket_init() setsockopt SO_REUSEADDR error\n");
			exit(1);
		}
        
        if (setsockopt(mserverSocket, SOL_SOCKET, SO_SNDBUF, &send_buf_size, sizeof(int)) == -1) {
			printf("server_socket_init() setsockopt SO_SNDBUF error\n");
			//exit(1);
		}
        
        if (setsockopt(mserverSocket, SOL_SOCKET, SO_RCVBUF, &recv_buf_size, sizeof(int)) == -1) {
			printf("server_socket_init() setsockopt SO_RCVBUF error\n");
			//exit(1);
		}
        
        /*if (setsockopt(mserverSocket, SOL_SOCKET, SO_SNDTIMEO, &send_timeout, sizeof(struct timeval)) == -1) {
			printf("server_socket_init() setsockopt SO_SNDTIMEO error\n");
			//exit(1);
		}*/

        /*if (setsockopt(mserverSocket, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(struct timeval)) == -1) {
			printf("server_socket_init() setsockopt SO_RCVTIMEO error\n");
			//exit(1);
		}*/

		if (bind(mserverSocket, p->ai_addr, p->ai_addrlen) == -1) {
			printf("server_socket_init() server: bind\n");
			close(mserverSocket);
			continue;
		}

		break;
	}

	if (p == NULL) {
		printf("server_socket_init() server: failed to bind\n");
		return -2;
	}

	freeaddrinfo(servinfo); // 全部都用這個 structure

	if (listen(mserverSocket, BACKLOG) == -1) {
		printf("server_socket_init() listen\n");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // 收拾全部死掉的 processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		printf("server_socket_init() sigaction\n");
		exit(1);
	}
	
	return 0;
}	

/** close socket */
void close_sock(int *sock) {
	if(*sock != -1) {
		close(*sock);
		*sock = -1;
	}
}

/** close all socket */
void close_all_sock(void) {
	if(mserverSocket != -1) close_sock(&mserverSocket);
	if(mSocket != -1) close_sock(&mSocket);
	if(gSocket != -1) close_sock(&gSocket);
	if(qSocket != -1) close_sock(&qSocket);
}

/** close clinet socket */
void close_clinet_sock(int *sock) {
	printf("close_clinet_sock() disconnect\n");
	close_sock(sock);
}

void stopThread(void){
	nSock_thread_en = 0;
	cmd_thread_en = 0;
	close_all_sock();
}

/** send data */
int send_data(int *sock, char *buf, int size) {
	int ret = -1;
	if(*sock != -1) {
		ret = send(*sock, buf, size, 0);
        if (ret < 0)
			printf("send_data() send buf error(%d)\n", ret);
		//else
		//	printf("send_data() send buf ret=%d size=%d buf[0x%x, 0x%x, 0x%x, 0x%x]\n", 
		//		ret, size, buf[0], buf[1], buf[2], buf[3]);
	}
	return ret;
}

/** read data  */
int read_data(int *sock, char *buf, int size) {
	int ret = -1;
	if(*sock != -1) {
		ret = recv(*sock, buf, size, 0);
		if(ret == 0) {
			close_clinet_sock(sock);
            printf("read_data() read buf error(%d)\n", ret);
        }
		else if (ret < 0)
			printf("read_data() read buf error(%d)\n", ret);
		//else
		//	printf("read_data() read buf ret=%d size=%d [0x%x, 0x%x, 0x%x, 0x%x]\n", 
		//		ret, size, buf[0], buf[1], buf[2], buf[3]);
	}
	return ret;
}

/** check socket read/write state */
void check_sock_rw_state(int sock, int r_en, int w_en, int *r_rv, int *w_rv) {
	int n, rv=0;
	fd_set readfds, writefds;
	struct timeval tv;
	fd_set *rp=NULL, *wp=NULL;
	
	if(r_en == 1) {
		FD_ZERO(&readfds);
		FD_SET(sock, &readfds);
		rp = &readfds;
	}
	if(w_en == 1) {
		FD_ZERO(&writefds);
		FD_SET(sock, &writefds);
		wp = &writefds;
	}
	n = sock + 1;
			
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	rv = select(n, rp, wp, NULL, &tv);
	if(rv > 0) {
		if(rp != NULL && r_rv != NULL) {
			if (FD_ISSET(sock, rp)) *r_rv = 1;
			else					*r_rv = 0;
		}
		
		if(wp != NULL && w_rv != NULL) {
			if (FD_ISSET(sock, wp)) *w_rv = 1;
			else				    *w_rv = 0;
		}
	}
	else {
		if(rv < 0)
			printf("check_sock_rw_state() select error\n"); // select() 發生錯誤
		//else
		//	printf("check_sock_rw_state() Timeout occurred!\n");
		if(r_rv != NULL) *r_rv = 0; 
		if(w_rv != NULL) *w_rv = 0;
	}
}

/** check client socket read/write state */
void check_client_sock_rw_state(int r_en, int w_en, int *r_rv, int *w_rv) {
	check_sock_rw_state(mSocket, r_en, w_en, r_rv, w_rv);
}

/** check client socket connect */
int check_client_sock_connect(int *sock) {
	int rv;
	char buf[256];
	check_sock_rw_state(*sock, 1, 0, &rv, NULL);
	if (rv == 1) {
		if(recv(*sock, buf, sizeof(buf), 0) > 0)
			return 1;
		else {
			close_clinet_sock(sock);
			return 0;
		}
	}
	return 0;
}

/**
 * *m:    目前處理完的大小, 供上層for迴圈判斷
 * *ost:  剩餘大小
 * *skip: 略過多少byte, = 0
 * *buf:  SocketDataQ, 
 * *tbuf: inputStreamQ, 供SocketDataQ資料前移用
 * max:   最大Size, 發生錯誤時設給*m, 讓上層跳出for迴圈
 * return ret: -1:Error, 0:未處理, 1:有對應的cmd
 */
int old_sock_command(int *m, int *ost, int *skip, char *buf, char *tbuf, int max) {
	int i, ret=0;
	char nbyte[16];
//printf("old_sock_command: m=%d ost=%d skip=%d max=%d buf=[0x%x, 0x%x, 0x%x, 0x%x]\n", *m, *ost, *skip, max, buf[0], buf[1], buf[2], buf[3]);		
	if(buf[0] == 'G' && buf[1] == 'S' && buf[2] == 'E' && buf[3] == 'R'){			// g-sensor
		*m += (20+*skip);
		*ost -= (20+*skip);
		*skip = 0;
		
		// 使用G-Sensor調整角度 
    	for(i = 0; i < 16; i++) {
    		nbyte[i] = buf[i+4];
		}
		getGsensorValue(&nbyte[0], 3, &mSensorVal[0]);					// 轉成mFloatVal
		printf("Cmd:'GSER' [0]=%f [1]=%f [2]=%f\n",  mSensorVal[0], mSensorVal[1], mSensorVal[2]);
//		if(Main.mNowVelocity == 0) runLiveMode(&mSensorVal[0]);
		
		if(*ost > 0) {
			memcpy(&tbuf[0], &buf[20], *ost);
			memcpy(&buf[0], &tbuf[0], *ost);
		}
		ret = 1;
    }
	else if(buf[0] == 'T' && buf[1] == 'O' && buf[2] == 'U' && buf[3] == 'H'){		// touch
		*m += (20+*skip);
		*ost -= (20+*skip);
		*skip = 0;
		
		// 使用Touch Screen調整角度 
	   	for(i = 0; i < 16; i++){
	   		nbyte[i] = buf[i+4];
	   	}
		getGsensorValue(&nbyte[0], 4, &mTouchVal[0]);					// 轉成mFloatVal
//		if(Main.AutoMoveFlag != 0) mTouchScreenEn = 1;  										
		if(mDemoSpeedStart == 0 || mDemoSpeedStart == 1) mDemoSpeedStart = 3;
		
		if(*ost > 0) {
			memcpy(&tbuf[0], &buf[20], *ost);
			memcpy(&buf[0], &tbuf[0], *ost);
		}
		ret = 1;
	}
	else if(buf[0] == 'S' && buf[1] == 'N' && buf[2] == 'A' && buf[3] == 'P'){		// snapshot / take picture
		if(mSnapshotEn == -1){
			*m += (20+*skip);
			*ost -= (20+*skip);
			*skip = 0;
			
	    	// 控制拍照開始
			char b[4];
			for(i=0; i<4; i++)	{ b[i] = buf[4+i];}
			mCaptureMode = atoi(b);
	    	mSnapshotEn = 1;		// rex+ 151015
	    	printf("Cmd:'SNAP' mSnapshotEn=%d mCaptureMode=%d\n", mSnapshotEn, mCaptureMode);
//	    	Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mCaptureMode", ""+mCaptureMode);
	    	
			if(*ost > 0) {
				memcpy(&tbuf[0], &buf[20], *ost);
				memcpy(&buf[0], &tbuf[0], *ost);
			}
		}     
		ret = 1;
	}
	else if(buf[0] == 'R' && buf[1] == 'E' && buf[2] == 'C' && buf[3] == 'E'){		// record enable
		*m += (20+*skip);
    	*ost -= (20+*skip);
    	*skip = 0;
			
    	// 控制攝影開始
    	for(i=0; i<4; i++){ mAdjustValue[i] = buf[i+4]; }
	    if(mAdjustValue[0] == 'R' && mAdjustValue[1] == 'E' && mAdjustValue[2] == 'C' && mAdjustValue[3] == 'M'){	//Time-Lapse MODE check code
	    	for(i=0; i<4; i++){ mAdjustValue[i] = buf[i+8]; }
	    	mTimeLapseMode = (byte2Int(&mAdjustValue[0], 4)) / 1000;
	    }
	    else{
	    	mTimeLapseMode = 0;
	    }
	    mRecordEn = 1;
			    	
    	printf("Cmd:'RECE' mTimeLapseMode=%d\n", mTimeLapseMode);
//    	Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mTimeLapseMode", ""+mTimeLapseMode);
	    	
		if(*ost > 0) {
			memcpy(&tbuf[0], &buf[20], *ost);
			memcpy(&buf[0], &tbuf[0], *ost);
		}
		ret = 1;
    }
    else if(buf[0] == 'R' && buf[1] == 'E' && buf[2] == 'C' && buf[3] == 'D'){		// record disable
		*m += (20+*skip);
	   	*ost -= (20+*skip);
	   	*skip = 0;
		
		// 控制攝影結束
	   	mRecordEn = 0;
//	   	Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mRecordEn", ""+mRecordEn);
	   	
		if(*ost > 0) {
			memcpy(&tbuf[0], &buf[20], *ost);
			memcpy(&buf[0], &tbuf[0], *ost);
		}
		ret = 1;
	}
	else if(buf[0] == 'M' && buf[1] == 'J' && buf[2] == 'P' && buf[3] == 'G'){		// record disable
		*m += (20+*skip);
	   	*ost -= (20+*skip);
	   	*skip = 0;
		
	   	// 啟動mjpg預覽模式
	   	mMjpegEn = 1;
	   	mRTSPEn = 0;
	   	printf("Cmd:'MJPG' mMjpegEn=%d mRTSPEn=%d\n", mMjpegEn, mRTSPEn);
//	   	Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mMjpegEn", ""+mMjpegEn);
	   	
		if(*ost > 0) {
			memcpy(&tbuf[0], &buf[20], *ost);
			memcpy(&buf[0], &tbuf[0], *ost);
		}
		ret = 1;
	}
	else if(buf[0] == 'A' && buf[1] == 'J' && buf[2] == 'E' && buf[3] == 'V'){		// adjust EV
		*m += (20+*skip);
	   	*ost -= (20+*skip);
	   	*skip = 0;
		
	   	// 調整EV, mEV=-6..0..6
	   	for(i=0; i<4; i++){ mAdjustValue[i] = buf[i+4]; }
	   	mEV = byte2Int(&mAdjustValue[0], 4);
	   	printf("Cmd:'AJEV' mEV=%d", mEV);
//	   	Main.systemlog.addLog("info", System.currentTimeMillis(), hostNameTmp, "Change EV.", String.valueOf(mEV));
	   	mAdjustEn = 1;
		
		if(*ost > 0) {
			memcpy(&tbuf[0], &buf[20], *ost);
			memcpy(&buf[0], &tbuf[0], *ost);
		}
		ret = 1;
	}
	else if(buf[0] == 'A' && buf[1] == 'J' && buf[2] == 'M' && buf[3] == 'F'){		// adjust MF
		*m += (20+*skip);
	   	*ost -= (20+*skip);
	   	*skip = 0;
		
	   	for(i=0; i<4; i++){ mAdjustValue[i] = buf[i+4]; }
	   	if(mAdjustValue[0] == 'M' && mAdjustValue[1] == 'F' && mAdjustValue[2] == '1' && mAdjustValue[3] == '1'){
			MFMode = 1;
	   	}
	   	else if(mAdjustValue[0] == 'M' && mAdjustValue[1] == 'F' && mAdjustValue[2] == '2' && mAdjustValue[3] == '2'){
	   		MFMode = 2;
	   	}
	   	for(i=0; i<4; i++){ mAdjustValue[i] = buf[i+8]; }
	   	if(MFMode == 1){
	    	// 調整MF, mMF=-32..0..16
			mMF = byte2Int(&mAdjustValue[0], 4);
	    	printf("Cmd='AJMF' mMF=%d\n", mMF);
//	    	Main.systemlog.addLog("info", System.currentTimeMillis(), hostNameTmp, "Change MF1.", String.valueOf(mMF));
	   	}
	   	else if(MFMode == 2){
	   		mMF2 = byte2Int(&mAdjustValue[0], 4);
	   		printf("Cmd='AJMF' mMF2=%d\n", mMF2);
//	   		Main.systemlog.addLog("info", System.currentTimeMillis(), hostNameTmp, "Change MF2.", String.valueOf(mMF2));
	   	}
	   	mAdjustEn = 1;
		
		if(*ost > 0) {
			memcpy(&tbuf[0], &buf[20], *ost);
			memcpy(&buf[0], &tbuf[0], *ost);
		}
		ret = 1;
	}
	else if(buf[0] == 'M' && buf[1] == 'O' && buf[2] == 'D' && buf[3] == 'E'){		// mode select
		*m += (20+*skip);
	   	*ost -= (20+*skip);
	   	*skip = 0;
		
	   	// 選擇模式 (mode)
	   	for(i=0; i<4; i++){ mAdjustValue[i] = buf[i+4]; }
	   	mPlayMode = byte2Int(&mAdjustValue[0], 4);		// 0:Global、1:Front、2:360、3:240、4:180、5:90x4、6:PIP
	   	for(i=0; i<4; i++){ mAdjustValue[i] = buf[i+8]; }
	   	mResoluMode = byte2Int(&mAdjustValue[0], 4);		// 1:10M、2:2.5M、3:2M、4:1M、5:D1 6:1.8M 7:5M 8:8M
	   	printf("Cmd:'MODE' mPlayMode=%d mResoluMode=%d\n", mPlayMode, mResoluMode);
//	   	Main.systemlog.addLog("info", System.currentTimeMillis(), hostNameTmp, "Change View Mode.", String.format("playmode=%d, resolumode=%d", mPlayMode, mResoluMode));
	   	mModeSelectEn = 1;
		
		if(*ost > 0) {
			memcpy(&tbuf[0], &buf[20], *ost);
			memcpy(&buf[0], &tbuf[0], *ost);
		}
		ret = 1;
	}
	else if(buf[0] == 'I' && buf[1] == 'S' && buf[2] == 'O' && buf[3] == 'M'){		// ISO select
		*m += (20+*skip);
	   	*ost -= (20+*skip);
	   	*skip = 0;
		
	   	// ISO選擇 (mode)
		for(i=0; i<4; i++){ mAdjustValue[i] = buf[i+4]; }
	   	ISOValue = byte2Int(&mAdjustValue[0], 4);
	   	printf("Cmd:'ISOM' ISOValue=%d\n", ISOValue);
//	   	Main.systemlog.addLog("info", System.currentTimeMillis(), hostNameTmp, "Change ISO.", String.valueOf(ISOValue));
	   	mISOEn = 1;
		
		if(*ost > 0) {
			memcpy(&tbuf[0], &buf[20], *ost);
			memcpy(&buf[0], &tbuf[0], *ost);
		}
		ret = 1;
	}
	else if(buf[0] == 'S' && buf[1] == 'H' && buf[2] == 'U' && buf[3] == 'T'){		// 快門 select
		*m += (20+*skip);
	   	*ost -= (20+*skip);
		*skip = 0;
		
	   	// 快門選擇 (mode)
	   	for(i=0; i<4; i++){ mAdjustValue[i] = buf[i+4]; }
	   	FValue = byte2Int(&mAdjustValue[0], 4);
	   	printf("Cmd:'SHUT' FValue=%d\n", FValue);
//	   	Main.systemlog.addLog("info", System.currentTimeMillis(), hostNameTmp, "Change Shutter.", String.valueOf(FValue));
	   	mFEn = 1;
		
		if(*ost > 0) {
			memcpy(&tbuf[0], &buf[20], *ost);
			memcpy(&buf[0], &tbuf[0], *ost);
		}
		ret = 1;
	}
	else if(buf[0] == 'W' && buf[1] == 'B' && buf[2] == 'M' && buf[3] == 'D'){		// WB select
		*m += (20+*skip);
	   	*ost -= (20+*skip);
		*skip = 0;
		
	   	// WB選擇 (mode)
	   	for(int i=0; i<4; i++){ mAdjustValue[i] = buf[i+4]; }
	   	WBMode = byte2Int(&mAdjustValue[0], 4);
	   	printf("Cmd:'WBMD' WBMode=%d\n", WBMode);
//	   	Main.systemlog.addLog("info", System.currentTimeMillis(), hostNameTmp, "Change AWB.", String.valueOf(WBMode));
	   	mWBEn = 1;
		
		if(*ost > 0) {
			memcpy(&tbuf[0], &buf[20], *ost);
			memcpy(&buf[0], &tbuf[0], *ost);
		}
		ret = 1;
	}
	else if(buf[0] == 'D' && buf[1] == 'E' && buf[2] == 'M' && buf[3] == 'O'){		// Demo Speed
		*m += (20+*skip);
	   	*ost -= (20+*skip);
	   	*skip = 0;
		
	   	// Demo Speed
	   	for(i=0; i<4; i++){ mAdjustValue[i] = buf[i+4]; }
	   	mDemoSpeedEn = byte2Int(&mAdjustValue[0], 4);
	   	for(i=0; i<4; i++){ mAdjustValue[i] = buf[i+8]; }
	   	DemoSpeed = byte2Int(&mAdjustValue[0], 4);
	   	printf("Cmd:'DEMO' mDemoSpeedEn=%d DemoSpeed=%d\n", mDemoSpeedEn, DemoSpeed);
//	   	Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mDemoSpeedEn", ""+mDemoSpeedEn);
//	   	Main.databin.setDemoMode(mDemoSpeedEn);
	   	mDemoSpeedStart = 1;
	   	if(mDemoSpeedEn != mDemoSpeedEn_lst) touch_pan_tilt_init = 0;
	   	mDemoSpeedEn_lst = mDemoSpeedEn; 
		
		if(*ost > 0) {
			memcpy(&tbuf[0], &buf[20], *ost);
			memcpy(&buf[0], &tbuf[0], *ost);
		}
		ret = 1;
	}
	else if(buf[0] == 'C' && buf[1] == 'A' && buf[2] == 'M' && buf[3] == 'S'){		// Cam select
		*m += (20+*skip);
	   	*ost -= (20+*skip);
	   	*skip = 0;
		
	   	// Cam select
	   	for(i=0; i<4; i++){ mAdjustValue[i] = buf[i+4]; }
	   	CamMode = byte2Int(&mAdjustValue[0], 4);
	   	printf("Cmd:'CAMS' CamMode=%d\n", CamMode);
//	   	Main.systemlog.addLog("info", System.currentTimeMillis(), hostNameTmp, "Change Camera Position.", String.valueOf(CamMode));
	   	mCamSelEn = 1;
		
		if(*ost > 0) {
			memcpy(&tbuf[0], &buf[20], *ost);
			memcpy(&buf[0], &tbuf[0], *ost);
		}
		ret = 1;
	}
	else if(buf[0] == 'S' && buf[1] == 'A' && buf[2] == 'V' && buf[3] == 'E'){		// 儲存至 select
		*m += (20+*skip);
	   	*ost -= (20+*skip);
		*skip = 0;
		
	   	// 儲存至 select
	   	for(i=0; i<4; i++){ mAdjustValue[i] = buf[i+4]; }
	   	SaveToSel = byte2Int(&mAdjustValue[0], 4);
	   	printf("Cmd:'SAVE' SaveToSel=%d\n", SaveToSel);
//	   	Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "SaveToSel", ""+SaveToSel);
	   	mSaveToSelEn = 1;
		
		if(*ost > 0) {
			memcpy(&tbuf[0], &buf[20], *ost);
			memcpy(&buf[0], &tbuf[0], *ost);
		}
		ret = 1;
	}
	else if(buf[0] == 'D' && buf[1] == 'A' && buf[2] == 'S' && buf[3] == 'T'){		// 第一次連線
		*m += (20+*skip);
	   	*ost -= (20+*skip);
	   	*skip = 0;	
printf("Cmd:'DAST' 00\n");		
	   	// 第一次連線
	   	char b[8];
	   	for(i=0; i<8; i++)	{ b[i] = buf[4+i];}
		mSysTime = byte2Long(&b[0], 8);
	   	mSetTime = 1;
	   	printf("Cmd:'DAST' mDataBinEn = 1\n");
	   	//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mSysTime", ""+mSysTime);
printf("Cmd:'DAST' 01\n");		
	   	if(lstTimeConnect < mSysTime)	lstTimeConnect = mSysTime;
printf("Cmd:'DAST' 02\n");		
	   	mDataBinEn = 1;
	   	mRecStateEn = 1;
	   	mConnected = 1;
//        setWifiOledEn(1);
        sendUninstallEn = 1;
        sendFeedBackEn = 1;
		//sendFeedBackAction = {'D', 'A', 'S', 'T'};
printf("Cmd:'DAST' 03\n");		
		sprintf(sendFeedBackAction, "DAST");
printf("Cmd:'DAST' 04\n");
    	sendFeedBackValue = 1;
printf("Cmd:'DAST' 05 ost=%d\n", *ost);
		if(*ost > 0) {
			memcpy(&tbuf[0], &buf[20], *ost);
			memcpy(&buf[0], &tbuf[0], *ost);
		}
		ret = 1;
printf("Cmd:'DAST' End\n");		
	}
	else if(buf[0] == 'F' && buf[1] == 'M' && buf[2] == 'A' && buf[3] == 'T'){		// Format SD
		*m += (20+*skip);
	   	*ost -= (20+*skip);
	   	*skip = 0;
		
	   	// Format SD
	   	printf("Cmd:'FMAT' mFormatEn = 1\n");
//	   	Main.systemlog.addLog("info", System.currentTimeMillis(), hostNameTmp, "Do Format SD Card.", "---");
	   	mFormatEn = 1;
		
		if(*ost > 0) {
			memcpy(&tbuf[0], &buf[20], *ost);
			memcpy(&buf[0], &tbuf[0], *ost);
		}
		ret = 1;
	}
	else if(buf[0] == 'S' && buf[1] == 'W' && buf[2] == 'A' && buf[3] == 'P'){		// data交換
		*m += (20+*skip);
	   	*ost -= (20+*skip);
	   	*skip = 0;
		
	   	// data交換
	   	printf("Cmd:'SWAP' mDataSwapEn = 1\n");
//	   	Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mDataSwapEn", "1");
	   	mDataSwapEn = 1;
	   	mConnected = 1;
//        setWifiOledEn(1);
		
		if(*ost > 0) {
			memcpy(&tbuf[0], &buf[20], *ost);
			memcpy(&buf[0], &tbuf[0], *ost);
		}
		ret = 1;
	}
	else if(buf[0] == 'S' && buf[1] == 'T' && buf[2] == 'H' && buf[3] == 'M'){		// 取THM檔
		*m += (20+*skip);
	   	*ost -= (20+*skip);
	   	*skip = 0;
		
	   	// 取THM檔
	   	mImgEn = 5;
	    clear_list(mExistFileName);		//mExistFileName.clear();
	   	printf("Cmd:'STHM' mImgEn = 5\n");
//	  	Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mImgEn", "5");
		
		if(*ost > 0) {
			memcpy(&tbuf[0], &buf[20], *ost);
			memcpy(&buf[0], &tbuf[0], *ost);
		}
		ret = 1;
	}
	else if(buf[0] == 'P' && buf[1] == 'H' && buf[2] == 'O' && buf[3] == 'T'){		// 取相簿圖
		*m += (20+*skip);
	   	*ost -= (20+*skip);
	   	*skip = 0;
		
	   	// 取相簿圖
	   	char a[4];
	    for( i=0;i<4;i++)	{ a[i] = buf[4+i];}       							    	
	    int FileNameLen	= byte2Int(&a[0], 4);
	    char b[FileNameLen];
	    for(i=0;i<FileNameLen;i++)	{ b[i] = buf[8+i];}
	    char FileName[FileNameLen];
		sprintf(FileName, "%s", b);
	    printf("Cmd:'PHOT' mExistFileName=%s\n", FileName);
//	    Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mExistFileName", ""+FileName);
	    add_node(mExistFileName, &FileName[0], sizeof(FileName));	//mExistFileName.add(FileName);
	    
		if(*ost > 0) {
			memcpy(&tbuf[0], &buf[20], *ost);
			memcpy(&buf[0], &tbuf[0], *ost);
		}
		ret = 1;
	}
	else if(buf[0] == 'P' && buf[1] == 'H' && buf[2] == 'O' && buf[3] == 'K'){		// 取相簿圖OK
		*m += (20+*skip);
		*ost -= (20+*skip);
	   	*skip = 0;
		
	   	// 取相簿圖OK
	   	mImgEn = 3;
	   	printf("Cmd:'PHOK' mImgEn = 3\n");
//	   	Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "PHOK", "3");
	  	
		if(*ost > 0) {
			memcpy(&tbuf[0], &buf[20], *ost);
			memcpy(&buf[0], &tbuf[0], *ost);
		}
		ret = 1;
	}
	else if(buf[0] == 'P' && buf[1] == 'S' && buf[2] == 'T' && buf[3] == 'P'){		// 停止取相簿圖
		*m += (20+*skip);
	   	*ost -= (20+*skip);
	   	*skip = 0;
		
	   	// 停止取相簿圖
	   	printf("Cmd:'PSTP' mImgEn = 2");
//	   	Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "PSTP", "2");
	   	mImgEn = 2;
	   	
		if(*ost > 0) {
			memcpy(&tbuf[0], &buf[20], *ost);
			memcpy(&buf[0], &tbuf[0], *ost);
		}
		ret = 1;
	}
	else if(buf[0] == 'L' && buf[1] == 'O' && buf[2] == 'A' && buf[3] == 'D'){		// 下載圖
		*skip = 0;
				
	   	// 下載圖
		int i;
	   	char c[4];
	   	for(i=0; i<4; i++)	{ c[i] = buf[4+i];}
	  	int cmdLen = atoi(c);
	    
	   	char a[4];
	   	for(i=0; i<4; i++)	{ a[i] = buf[8+i];}       							    	
	   	int downloadFileNameLen	= byte2Int(&a[0], 4);
	   	
	   	printf("Cmd:'LOAD' downloadFileNameLen=%d\n", downloadFileNameLen);
	   	char b[downloadFileNameLen];
	   	for(i=0; i<downloadFileNameLen; i++)	{ b[i] = buf[12+i];}
	  	char downloadFileName[downloadFileNameLen];
		sprintf(downloadFileName, "%s", b);
			    	
	   	char d[4];
	   	for(i=0; i<4; i++)	{ d[i] = buf[12+downloadFileNameLen+i];}
	   	int fileSkip = byte2Int(&d[0], 4);
		    	
	   	printf("Cmd:'LOAD' mDownloadFileName=%s fileSkip=%d\n", downloadFileName, fileSkip);
//	   	Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mDownloadFileName", ""+downloadFileName);
	   	add_node(mDownloadFileName, &downloadFileName[0], sizeof(downloadFileName));		//mDownloadFileName.add(downloadFileName);
	   	add_node(mDownloadFileSkip, (char*)&fileSkip, sizeof(fileSkip));					//mDownloadFileSkip.add(fileSkip);
	    	
	   	*m += (cmdLen+*skip);
		*ost -= (cmdLen+*skip);
		if(*ost > 0) {
			memcpy(&tbuf[0], &buf[cmdLen], *ost);
			memcpy(&buf[0], &tbuf[0], *ost);
		}
		ret = 1;
	}
	else if(buf[0] == 'L' && buf[1] == 'D' && buf[2] == 'O' && buf[3] == 'K'){		// 下載圖名稱OK
		*m += (20+*skip);
	   	*ost -= (20+*skip);
	   	*skip = 0;
		
	   	// 下載圖名稱OK;
	   	printf("Cmd:'LDOK' mDownloadEn = 3\n");
//	   	Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "LDOK", "3");
	   	mDownloadEn = 3;
		
		if(*ost > 0) {
			memcpy(&tbuf[0], &buf[20], *ost);
			memcpy(&buf[0], &tbuf[0], *ost);
		}
		ret = 1;
	}
	else if(buf[0] == 'L' && buf[1] == 'D' && buf[2] == 'S' && buf[3] == 'P'){		// 停止下載圖
		*m += (20+*skip);
	   	*ost -= (20+*skip);
	   	*skip = 0;
		
	   	// 停止下載圖
	   	printf("Cmd:'LDSP' mDownloadEn = 2\n");
//	   	Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "LDSP", "0");
	   	mDownloadEn = 2;
		
		if(*ost > 0) {
			memcpy(&tbuf[0], &buf[20], *ost);
			memcpy(&buf[0], &tbuf[0], *ost);
		}
		ret = 1;
	}
	else if(buf[0] == 'D' && buf[1] == 'E' && buf[2] == 'L' && buf[3] == 'E'){		// 刪除圖
		*m += (20+*skip);
	   	*ost -= (20+*skip);
	   	*skip = 0;
		
		// 刪除圖
	   	char a[4];
	   	for(i=0; i<4; i++)	{ a[i] = buf[4+i];}       							    	
	   	int deletedFileNameLen	= byte2Int(&a[0], 4);
	   	printf("Cmd:'DELE' deleteFileNameLen=%d\n", deletedFileNameLen);
    	char b[deletedFileNameLen];
	   	for(i=0; i<deletedFileNameLen; i++)	{ b[i] = buf[8+i];}
	   	char deleteFileName[deletedFileNameLen];
		sprintf(deleteFileName, "%s", b);
		printf("Cmd:'DELE' mdeleteFileName=%s\n", deleteFileName);
	   	add_node(mDeleteFileName, &deleteFileName[0], sizeof(deleteFileName));		//mDeleteFileName.add(deleteFileName);
		
		if(*ost > 0) {
			memcpy(&tbuf[0], &buf[20], *ost);
			memcpy(&buf[0], &tbuf[0], *ost);
		}
		ret = 1;
	}
	else if(buf[0] == 'D' && buf[1] == 'E' && buf[2] == 'O' && buf[3] == 'K'){		// 刪除圖名稱OK
		*m += (20+*skip);
	   	*ost -= (20+*skip);
	   	*skip = 0;
		
	   	// 刪除圖名稱OK;
	   	printf("Cmd:'DEOK' mDelete = 1\n");
	   	mDeleteEn = 1;
		
		if(*ost > 0) {
			memcpy(&tbuf[0], &buf[20], *ost);
			memcpy(&buf[0], &tbuf[0], *ost);
		}
		ret = 1;
	}
	else if(buf[0] == 'C' && buf[1] == 'P' && buf[2] == 'M' && buf[3] == 'D'){		// 拍照模式
		*m += (20+*skip);
		*ost -= (20+*skip);
		*skip = 0;
		
	   	// 拍照模式
		char b[4];
		for(i=0; i<4; i++)	{ b[i] = buf[4+i];}
		mCapMode = byte2Int(&b[0], 4);
		mCaptureModeEn = 1;		
	   	printf("Cmd:'CPMD' mCaptureModeEn=%d mCapMode=%d\n", mCaptureModeEn, mCapMode);
//	   	Main.systemlog.addLog("info", System.currentTimeMillis(), hostNameTmp, "Change Capture Mode.", String.valueOf(mCapMode));
	   	
		if(*ost > 0) {
			memcpy(&tbuf[0], &buf[20], *ost);
			memcpy(&buf[0], &tbuf[0], *ost);
		}
		ret = 1;		
	}
	else if(buf[0] == 'T' && buf[1] == 'L' && buf[2] == 'M' && buf[3] == 'D'){		// Time-Lapse模式
		*m += (20+*skip);
		*ost -= (20+*skip);
		*skip = 0;
		
	    // Time-Lapse模式
		char b[4];
		for(i=0; i<4; i++)	{ b[i] = buf[4+i];}
		mTimeLapse = byte2Int(&b[0], 4);
		mTimeLapseEn = 1;		
	    printf("Cmd:'TLMD' mTimeLapseEn=%d mTimeLapse=%d\n", mTimeLapseEn, mTimeLapse);
//	    Main.systemlog.addLog("info", System.currentTimeMillis(), hostNameTmp, "Change Time-Lapse Mode.", String.valueOf(mTimeLapse));
	    
		if(*ost > 0) {
			memcpy(&tbuf[0], &buf[20], *ost);
			memcpy(&buf[0], &tbuf[0], *ost);
		}
		ret = 1;		
	}
	else if(buf[0] == 'G' && buf[1] == 'R' && buf[2] == 'E' && buf[3] == 'C'){		// get rec state
		*m += (20+*skip);
		*ost -= (20+*skip);
		*skip = 0;
		
	   	// get rec state
		mRecStateEn = 1;
	   	printf("Cmd:'GREC' mRecStateEn = 1\n");
	   	
		if(*ost > 0) {
			memcpy(&tbuf[0], &buf[20], *ost);
			memcpy(&buf[0], &tbuf[0], *ost);
		}
		ret = 1;		
	}
	else if(buf[0] == 'S' && buf[1] == 'H' && buf[2] == 'A' && buf[3] == 'R'){		// sharpness
		*m += (20+*skip);
		*ost -= (20+*skip);
		*skip = 0;
		
	   	// sharpness
		mSharpnessEn = 1;
	   	char b[4];
	   	for(i=0; i<4; i++)	{ b[i] = buf[4+i];}
	   	mSharpness = byte2Int(&b[0], 4) + 8;
	   	printf("Cmd:'GREC' mSharpnessEn = 1, sharpness = %d\n", mSharpness);
//	   	Main.systemlog.addLog("info", System.currentTimeMillis(), hostNameTmp, "Change Sharpness.", String.valueOf(mSharpness));
	   	
		if(*ost > 0) {
			memcpy(&tbuf[0], &buf[20], *ost);
			memcpy(&buf[0], &tbuf[0], *ost);
		}
		ret = 1;
	}
	else if(buf[0] == 'B' && buf[1] == 'O' && buf[2] == 'T' && buf[3] == 'M' &&
			buf[4] == 'T' && buf[5] == 'O' && buf[6] == 'T' && buf[7] == 'L'){
		
		char c[4];
		for(i=0; i<4; i++){
			c[i] = buf[8+i];
		}
		bottomTotle = byte2Int(&c[0], 4);
//		datadownload.deleteBottomFile();
		printf("Cmd:'BOTMTOTL' getBottomTotle = " + bottomTotle);
		
		if(*ost >= 12){
			*m += 12;
			*ost -= 12;
			if(*ost > 0) {
				memcpy(&tbuf[0], &buf[12], *ost);
				memcpy(&buf[0], &tbuf[0], *ost);
			}
			bottomCount = 0;
		}
		ret = 1;
	}
	else if(buf[0] == 'B' && buf[1] == 'O' && buf[2] == 'T' && buf[3] == 'M' &&
			buf[4] == 0x20 && buf[5] == 0x18 && buf[6] == 0x04 && buf[7] == 0x11 ){
		
		printf("Cmd:'BOTM' Bottom setup cmd\n");
	   	int i;
		char c[4];
		for(i=0; i<4; i++){
			c[i] = buf[8+i];
		}
		int fileNameLen = atoi(c);
		char a[fileNameLen];
		for(i=0; i<fileNameLen; i++){
			a[i] = buf[12+i];
		}
		char fileName[fileNameLen];
		sprintf(fileName, "%s", a);
		char b[4];
		for(i=0; i<4; i++){
			b[i] = buf[12+fileNameLen+i];
		}
		int slen = byte2Int(&b[0], 4);
		printf("Cmd:'BOTM' Bottom *ost=%d slen=%d\n", *ost, slen);
		if(*ost >= (slen+16+fileNameLen)) {
			memcpy(&bottomData[0], &buf[16+fileNameLen], slen);
			*ost -= (slen+16+fileNameLen);
			if(*ost > 0) {
				memcpy(&tbuf[0], &buf[slen+16+fileNameLen], *ost);
				memcpy(&buf[0], &tbuf[0], *ost);
			}
			printf("Cmd:'BOTM' Bottom Len=%d\n", slen);
			printf("Cmd:'BOTM' Bottom FileName=%s\n", fileName);
						
			char writeName[64];
			sprintf(writeName, "%s", BOTTOM_FILE_NAME_USER);
			int isOrg = 0;
			if(strcmp(fileName, BOTTOM_FILE_NAME_ORG) == 0){
				sprintf(writeName, "%s", BOTTOM_FILE_NAME_ORG);
				isOrg = 1;
			}
						
/*			ret = datadownload.writeBottomFile(writeName, bottomData, slen);
			if(ret == 1){
				printf("Cmd:'BOTM' Download File: %s, Error!", fileName);
			}*/		
			bottomCount += slen;
			if(!isOrg){
				bottomEn = 1;
			}
		}else{
			*m = max;
		}
		ret = 1;
	}
//printf("old_sock_command: End m=%d ost=%d\n", *m, *ost);		
	return ret;
}

/**
 * *m:    目前處理完的大小, 供上層for迴圈判斷
 * *ost:  剩餘大小
 * *skip: 略過多少byte, = 0
 * *buf:  SocketDataQ, 
 * *tbuf: inputStreamQ, 供SocketDataQ資料前移用
 * type:  使用哪個遠端APP連線
 * return ret: -1:Error, 0:未處理, 1:有對應的cmd
 */
int new_sock_command1(int *m, int *ost, int *skip, char *buf, char *tbuf, int *app_m, int type) {
	int ret=0;
	char bHeader[16];
	struct socket_cmd_header_struct sock_cmd_h;
	sock_cmd_header_init(&sock_cmd_h);
	//printf("new_sock_command1: m=%d ost=%d buf=[0x%x,0x%x,0x%x,0x%x]\n", *m, *ost, buf[0], buf[1], buf[2], buf[3]);
	if(*ost >= 16 && buf[0] == 0x20 && buf[1] == 0x16 && buf[2] == 0x03 && buf[3] == 0x11){
		memcpy(&bHeader[0], &buf[0], 16);
	   	sock_cmd_header_Bytes2Data(&bHeader[0], &sock_cmd_h);
	   	if(!sock_cmd_header_checkHeader(&sock_cmd_h)){	
	   		printf("Check New Command1 Error!-----------------------------------------\n");
	   		printf("Check New Command1 Error! *ost = %d\n", *ost);
	   		printf("Check New Command1 Error! version = 0x%x\n", sock_cmd_h.version);
	   		printf("Check New Command1 Error! keyword = %c.%c.%c.%c\n", 
				sock_cmd_h.keyword[0], sock_cmd_h.keyword[1], sock_cmd_h.keyword[2], sock_cmd_h.keyword[3]);
	   		printf("Check New Command1 Error! dataLength = %d\n", sock_cmd_h.dataLength);
	   		printf("Check New Command1 Error! checkSum = %d\n", sock_cmd_h.checkSum);
	   		printf("Check New Command1 Error!-----------------------------------------\n");
			*ost = 0;
	   		return -1;
	   	}
	   	else{
	   		if(sock_cmd_h.keyword[0] == 'G' && sock_cmd_h.keyword[1] == 'S' && sock_cmd_h.keyword[2] == 'E' && sock_cmd_h.keyword[3] == 'R'){	// 新Gsensor Command
	   			int dataLen = sock_cmd_h.dataLength;
	   			if(*ost < (16+dataLen))	return -1;
				
	   			char data[16];
				memcpy(&data[0], &buf[16], 16);
	   			float value[4];
	   			getGsensorValue(&data[0], 4, &value[0]);
//	   			data = new byte[8];
				memcpy(&data[0], &buf[16+16], 8);
//	   			ByteBuffer buf = ByteBuffer.wrap(data);
		    	unsigned long long timestamp = byte2Long(&data[0], 8);
				    	
	    		value[1] = 180 - abs(value[1]);		// tilt
				struct gsensorVal_struct getVal;
				initGsensorVal(&value[0], timestamp, &getVal); 
		    	
//		    	if(Main.mNowVelocity == 0) setGsensorValue(getVal);      
					    	
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
    		else if(sock_cmd_h.keyword[0] == 'Z' && sock_cmd_h.keyword[1] == 'O' && sock_cmd_h.keyword[2] == 'N' && sock_cmd_h.keyword[3] == 'E'){	// 設定時區
    			int dataLen = sock_cmd_h.dataLength;
    			char data[dataLen];
    			if(*ost < (16+dataLen))	return -1;
				
				memcpy(&data[0], &buf[16], dataLen);
			    			
				sprintf(wifiZone, "%s", data);
    			mZoneEn = 1;
				
    			*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'ZONE' mZoneEn = 1 , zone = %s\n", wifiZone);
//				Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "zone", ""+wifiZone);
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
    		else if(sock_cmd_h.keyword[0] == 'P' && sock_cmd_h.keyword[1] == 'H' && sock_cmd_h.keyword[2] == 'O' && sock_cmd_h.keyword[3] == 'T'){	// 取相簿圖
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
		    			
    			char a[4];
				memcpy(&a[0], &buf[16], 4);    							    	
		    	int FileNameLen	= byte2Int(&a[0], 4);
				
		    	char b[FileNameLen];
				memcpy(&b[0], &buf[16+4], FileNameLen);  
		    	char FileName[128];
				sprintf(FileName, "%s", b);
		    	printf("Cmd1:'PHOT' mExistFileName = %s\n", FileName);
		    	add_node(mExistFileName, &FileName[0], sizeof(FileName));	//mExistFileName.add(FileName);
					    	
    			*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
    		else if(sock_cmd_h.keyword[0] == 'P' && sock_cmd_h.keyword[1] == 'H' && sock_cmd_h.keyword[2] == 'O' && sock_cmd_h.keyword[3] == 'K'){	// 取相簿圖OK
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
					    	
		    	mImgEn = 3;	
		    	printf("Cmd1:'PHOK' mImgEn = 3\n");
			    			
    			*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
    		else if(sock_cmd_h.keyword[0] == 'D' && sock_cmd_h.keyword[1] == 'E' && sock_cmd_h.keyword[2] == 'L' && sock_cmd_h.keyword[3] == 'E'){	// 刪除圖
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
					    	
    			char a[4];
				memcpy(&a[0], &buf[16], 4); 
		    	int deletedFileNameLen	= byte2Int(&a[0], 4);
					    	
			    char b[deletedFileNameLen];
				memcpy(&b[0], &buf[16+4], deletedFileNameLen); 
		    	char deleteFileName[128];
				sprintf(deleteFileName, "%s", b);
					    	
		    	printf("Cmd1:'DELE' mdeleteFileName = %s\n", deleteFileName);
		    	add_node(mDeleteFileName, &deleteFileName[0], sizeof(deleteFileName));		//mDeleteFileName.add(deleteFileName);
			    			
    			*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
			else if(sock_cmd_h.keyword[0] == 'D' && sock_cmd_h.keyword[1] == 'E' && sock_cmd_h.keyword[2] == 'O' && sock_cmd_h.keyword[3] == 'K'){	// 刪除圖OK
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
					    	
				printf("Cmd1:'DEOK' mDelete = 1\n");
			  	mDeleteEn = 1;
			    			
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
/*			else if(sock_cmd_h.keyword[0] == 'W' && sock_cmd_h.keyword[1] == 'I' && sock_cmd_h.keyword[2] == 'F' && sock_cmd_h.keyword[3] == 'I'){	// wifi關閉時間
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
			    			
				char a[4];
				memcpy(&a[0], &buf[16], 4); 
				wifiDisableTime = byte2Int(&a[0], 4);
			    			
				mWifiDisableTimeEn = 1;
				printf("Cmd1:'WIFI' mWifiDisableTimeEn = %d\n", wifiDisableTime);
//				Main.systemlog.addLog("info", System.currentTimeMillis(), hostNameTmp, "Change Sleep Time.", String.valueOf(wifiDisableTime));
			    			
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}*/
			else if(sock_cmd_h.keyword[0] == 'C' && sock_cmd_h.keyword[1] == 'C' && sock_cmd_h.keyword[2] == 'N' && sock_cmd_h.keyword[3] == 'T'){	// get captureCnt
				int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
				char a[4];
				memcpy(&a[0], &buf[16], 4); 
    			captureCnt = byte2Int(&a[0], 4);
			    			
    			mCaptureCntEn = 1;
    			printf("Cmd1:'CCNT' mCaptureCntEn = %d\n", captureCnt);
//    			Main.systemlog.addLog("info", System.currentTimeMillis(), hostNameTmp, "Change Capture Count.", String.valueOf(captureCnt));
			    			
    			*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
    		else if(sock_cmd_h.keyword[0] == 'N' && sock_cmd_h.keyword[1] == 'E' && sock_cmd_h.keyword[2] == 'T' && sock_cmd_h.keyword[3] == 'S'){	// Ethernet Settings
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			char a[4];
    			int pos = 16;
				memcpy(&a[0], &buf[pos], 4); 
    			pos += 4;
    			mEthernetMode = byte2Int(&a[0], 4);
			    			
    			char BytesIPLength[4];
				memcpy(&BytesIPLength[0], &buf[pos], 4); 
    			pos += 4;
    			int IPLength = byte2Int(&BytesIPLength[0], 4);
    			char BytesIP[IPLength];
				memcpy(&BytesIP[0], &buf[pos], IPLength); 
    			pos += IPLength;
				sprintf(mEthernetIP, "%s", BytesIP);
			    			
    			char BytesMaskLength[4];
				memcpy(&BytesMaskLength[0], &buf[pos], 4); 
    			pos += 4;
    			int MaskLength = byte2Int(&BytesMaskLength[0], 4);
    			char BytesMask[MaskLength];
				memcpy(&BytesMask[0], &buf[pos], MaskLength); 
    			pos += MaskLength;
				sprintf(mEthernetMask, "%s", BytesMask);
			    			
    			char BytesGatewayLength[4];
				memcpy(&BytesGatewayLength[0], &buf[pos], 4); 
    			pos += 4;
    			int GatewayLength = byte2Int(&BytesGatewayLength[0], 4);
    			char BytesGateway[GatewayLength];
				memcpy(&BytesGateway[0], &buf[pos], GatewayLength); 
    			pos += GatewayLength;
				sprintf(mEthernetGateway, "%s", BytesGateway);
			    			
    			char BytesDNSLength[4];
				memcpy(&BytesDNSLength[0], &buf[pos], 4); 
    			pos += 4;
    			int DNSLength = byte2Int(&BytesDNSLength[0], 4);
    			char BytesDNS[DNSLength];
				memcpy(&BytesDNS[0], &buf[pos], DNSLength); 
    			pos += DNSLength;
				sprintf(mEthernetDNS, "%s", BytesDNS);
			    			
    			mEthernetSettingsEn = 1;        							    			
    			printf("Cmd1:'NETS' mEthernetSettingsEn = %d , ip = %s , mask = %s , gateway = %s , dns = %s\n", mEthernetMode, mEthernetIP, mEthernetMask, mEthernetGateway, mEthernetDNS);
//    			Main.systemlog.addLog("info", System.currentTimeMillis(), hostNameTmp, "Change Ethernet Settings.", String.format("mode=%d, ip=%s, mask=%s, gatway=%s, dns=%s", mEthernetMode, mEthernetIP, mEthernetMask, mEthernetGateway, mEthernetDNS));
			    			
    			*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
    		else if(sock_cmd_h.keyword[0] == 'C' && sock_cmd_h.keyword[1] == 'F' && sock_cmd_h.keyword[2] == 'P' && sock_cmd_h.keyword[3] == 'S'){	// change FPS
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			char a[4];
				memcpy(&a[0], &buf[16], 4); 
    			mFPS = byte2Int(&a[0], 4);
			    			
    			mChangeFPSEn = 1;
    			printf("Cmd1:'CFPS' mChangeFPSEn = %d\n", mFPS);
//    			Main.systemlog.addLog("info", System.currentTimeMillis(), hostNameTmp, "Change FPS.", String.valueOf(mFPS));
			    			
    			*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
    		else if(sock_cmd_h.keyword[0] == 'H' && sock_cmd_h.keyword[1] == '2' && sock_cmd_h.keyword[2] == '6' && sock_cmd_h.keyword[3] == '4'){	// enable H264
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			mRTSPEn = 1;
    			mMjpegEn = 0;
    			printf("Cmd1:'H264' mRTSPEn = %d , mMjpegEn = %d\n", mRTSPEn, mMjpegEn);
			    		
    			*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;     							    			
    		}
    		else if(sock_cmd_h.keyword[0] == 'P' && sock_cmd_h.keyword[1] == 'O' && sock_cmd_h.keyword[2] == 'R' && sock_cmd_h.keyword[3] == 'T'){	// change media port
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			char a[4];
				memcpy(&a[0], &buf[16], 4); 
    			mPort = byte2Int(&a[0], 4);
		    			
    			mMediaPortEn = 1;
    			printf("Cmd1:'PORT' mMediaPortEn = %d\n", mPort);
//    			Main.systemlog.addLog("info", System.currentTimeMillis(), hostNameTmp, "Change Port.", String.valueOf(mPort));
			    			
    			*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
    		else if(sock_cmd_h.keyword[0] == 'D' && sock_cmd_h.keyword[1] == 'V' && sock_cmd_h.keyword[2] == 'R' && sock_cmd_h.keyword[3] == 'C'){	// DrivingRecord
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			char a[4];
				memcpy(&a[0], &buf[16], 4); 
    			mDrivingRecord = byte2Int(&a[0], 4);
			    			
    			mDrivingRecordEn = 1;
    			printf("Cmd1:'DVRC' mDrivingRecordEn = %d\n", mDrivingRecord);
//    			Main.systemlog.addLog("info", System.currentTimeMillis(), hostNameTmp, "Change Driving Record.", String.valueOf(mDrivingRecord));
			    			
    			*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
    		else if(sock_cmd_h.keyword[0] == 'W' && sock_cmd_h.keyword[1] == 'F' && sock_cmd_h.keyword[2] == 'C' && sock_cmd_h.keyword[3] == 'H'){	// Wifi Channel
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
		    			
    			char a[4];
				memcpy(&a[0], &buf[16], 4); 
    			mWifiChannel = byte2Int(&a[0], 4);
		    			
    			mWifiChannelEn = 1;
    			printf("Cmd1:'DVRC' mWifiChannelEn = %d\n", mWifiChannel);
//    			Main.systemlog.addLog("info", System.currentTimeMillis(), hostNameTmp, "Change Wifi Channel.", String.valueOf(mWifiChannel));
			    			
    			*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
    		else if(sock_cmd_h.keyword[0] == 'E' && sock_cmd_h.keyword[1] == 'P' && sock_cmd_h.keyword[2] == 'F' && sock_cmd_h.keyword[3] == 'Q'){	// EP freq
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			char a[4];
				memcpy(&a[0], &buf[16], 4); 
    			mEPFreq = byte2Int(&a[0], 4);
	    			
    			mEPFreqEn = 1;
    			printf("Cmd1:'EPFQ' mEPFreqEn = %d\n", mEPFreq);
//    			Main.systemlog.addLog("info", System.currentTimeMillis(), hostNameTmp, "Change Frequency.", String.valueOf(mEPFreq));
	    			
    			*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
    		else if(sock_cmd_h.keyword[0] == 'F' && sock_cmd_h.keyword[1] == 'C' && sock_cmd_h.keyword[2] == 'T' && sock_cmd_h.keyword[3] == 'L'){	// fan control
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			char a[4];
				memcpy(&a[0], &buf[16], 4); 
    			mFanCtrl = byte2Int(&a[0], 4);
			    			
    			mFanCtrlEn = 1;
    			printf("Cmd1:'EPFQ' mFanCtrlEn = %d\n", mFanCtrl);
//    			Main.systemlog.addLog("info", System.currentTimeMillis(), hostNameTmp, "Change Fan Control.", String.valueOf(mFanCtrl));
			    			
    			*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
    		else if(sock_cmd_h.keyword[0] == 'S' && sock_cmd_h.keyword[1] == 'T' && sock_cmd_h.keyword[2] == 'O' && sock_cmd_h.keyword[3] == 'P'){	// stop livestreaming
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			mRTSPEn = 0;
    			mMjpegEn = 0;
    			printf("Cmd1:'STOP' stop : mjpeg = %d , h264 = %d\n", mMjpegEn, mRTSPEn);
//    			Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "stopLive", "---");
			    			
    			*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
    		else if(sock_cmd_h.keyword[0] == 'W' && sock_cmd_h.keyword[1] == 'F' && sock_cmd_h.keyword[2] == 'C' && sock_cmd_h.keyword[3] == 'F'){ // wifi config
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
		    			
    			int pos = 16;
    			char ssid[7];
				memcpy(&ssid[0], &buf[pos], 7); 
				sprintf(mWifiConfigSsid, "%s", ssid);
    			pos += 7;
			    			
    			char password[8]; 
				memcpy(&password[0], &buf[pos], 8); 
				sprintf(mWifiConfigPwd, "%s", password);
			    			
    			mWifiConfigEn = 1;
    			printf("Cmd1:'WFCF' mWifiConfigEn : Ssid = %s , Pwd = %s\n", mWifiConfigSsid, mWifiConfigPwd);
//    			Main.systemlog.addLog("info", System.currentTimeMillis(), hostNameTmp, "Change SSID and PWD.", String.format("ssid=%s, pwd=%s", mWifiConfigSsid, mWifiConfigPwd));
			    			
    			*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
    		else if(sock_cmd_h.keyword[0] == 'W' && sock_cmd_h.keyword[1] == 'F' && sock_cmd_h.keyword[2] == 'M' && sock_cmd_h.keyword[3] == 'D'){ // wifi Mode
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
	    			
    			int pos = 16;
			    			
    			char BytesWifiMode[4];
				memcpy(&BytesWifiMode[0], &buf[pos], 4); 
    			pos += 4;
    			int wifiMode = byte2Int(&BytesWifiMode[0], 4);
			    			
    			char BytesSSIDLength[4];
				memcpy(&BytesSSIDLength[0], &buf[pos], 4); 
    			pos += 4;
    			int SSIDLength = byte2Int(&BytesSSIDLength[0], 4);
    			char BytesSSID[SSIDLength];
				memcpy(&BytesSSID[0], &buf[pos], SSIDLength); 
    			pos += SSIDLength;
    			char newSsid[7];
				sprintf(newSsid, "%s", BytesSSID);
			    			
    			char BytesPWDLength[4];
				memcpy(&BytesPWDLength[0], &buf[pos], 4); 
    			pos += 4;
    			int PWDLength = byte2Int(&BytesPWDLength[0], 4);
    			char BytesPWD[PWDLength];
				memcpy(&BytesPWD[0], &buf[pos], PWDLength); 
    			pos += PWDLength;
				char newPwd[8];
				sprintf(newPwd, "%s", BytesPWD);
			    			
    			char BytesWifiType[4];
				memcpy(&BytesWifiType[0], &buf[pos], 4); 
    			pos += 4;
//    			int newType = byte2Int(&BytesWifiType[0], 4);
			    			
    			char BytesIPLength[4];
				memcpy(&BytesIPLength[0], &buf[pos], 4); 
    			pos += 4;
    			int IPLength = byte2Int(&BytesIPLength[0], 4);
    			char BytesIP[IPLength];
				memcpy(&BytesIP[0], &buf[pos], IPLength); 
    			pos += IPLength;
    			char newIP[16];
				sprintf(newIP, "%s", BytesIP);
			    			
    			char BytesGatewayLength[4];
				memcpy(&BytesGatewayLength[0], &buf[pos], 4); 
    			pos += 4;
    			int GatewayLength = byte2Int(&BytesGatewayLength[0], 4);
    			char BytesGateway[GatewayLength];
				memcpy(&BytesGateway[0], &buf[pos], GatewayLength); 
    			pos += GatewayLength;
    			char newGateway[16];
				sprintf(newGateway, "%s", BytesGateway);
			    			
    			char BytesPrefixLength[4];
				memcpy(&BytesPrefixLength[0], &buf[pos], 4); 
    			pos += 4;
    			int PrefixLength = byte2Int(&BytesPrefixLength[0], 4);
    			char BytesPrefix[PrefixLength];
				memcpy(&BytesPrefix[0], &buf[pos], PrefixLength); 
    			pos += PrefixLength;
    			char newPrefix[PrefixLength];
				sprintf(newPrefix, "%s", BytesPrefix);
			    			
    			char BytesDns1Length[4];
				memcpy(&BytesDns1Length[0], &buf[pos], 4); 
    			pos += 4;
    			int Dns1Length = byte2Int(&BytesDns1Length[0], 4);
    			char BytesDns1[Dns1Length];
				memcpy(&BytesDns1[0], &buf[pos], Dns1Length); 
    			pos += Dns1Length;
    			char newDns1[16];
				sprintf(newDns1, "%s", BytesDns1);
			    			
    			char BytesDns2Length[4];
				memcpy(&BytesDns2Length[0], &buf[pos], 4); 
    			pos += 4;
    			int Dns2Length = byte2Int(&BytesDns2Length[0], 4);
    			char BytesDns2[Dns2Length];
				memcpy(&BytesDns2[0], &buf[pos], Dns2Length); 
    			pos += Dns2Length;
    			char newDns2[16];
				sprintf(newDns2, "%s", BytesDns2);
				
				int reboot = 0;
			    if((dataLen + 16) > pos){
			    	char BytesReboot[4];
					memcpy(&BytesReboot[0], &buf[pos], 4);
					pos += 4;
					reboot = byte2Int(&BytesReboot[0], 4);	
			    }
			    			
//    			Main.mWifiModeCmd = wifiMode;
//				Main.wifiSSID = newSsid;
//    			Main.wifiPassword = newPwd;
//    			Main.wifiType = newType;
//    			Main.wifiIP = newIP;
//    			Main.wifiGateway = newGateway;
//    			Main.wifiPrefix = newPrefix;
//    			Main.wifiDns1 = newDns1;
//    			Main.wifiDns2 = newDns2;
//				Main.wifiReboot = reboot;
			    			
    			printf("Cmd1:'WFMD' mWifiModeCmd : Mode = %d Ssid = %s , Pwd = %s reboot = %d\n", wifiMode, newSsid, newPwd, reboot);
    			printf("Cmd1:'WFMD' mWifiModeCmd : type = %d ip = %s , gateway = %s prefix = %s dns1 = %s dns2 = %s\n", wifiMode, newIP, newGateway, newPrefix, newDns1, newDns2);
    			
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
    		else if(sock_cmd_h.keyword[0] == 'D' && sock_cmd_h.keyword[1] == 'B' && sock_cmd_h.keyword[2] == 'T' && sock_cmd_h.keyword[3] == 'L'){ // debugtool connect
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			fromWhereConnect = 1;
    			mChangeDebugToolStateEn = 1;
    			isDebugToolConnect = 1;
    			printf("Cmd1:'DBTL' mChangeDebugToolStateEn = %d , isDebugToolConnect = %d\n", isDebugToolConnect, isDebugToolConnect);
//    			Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "isDebugToolConnect", ""+isDebugToolConnect);
    			
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
    		else if(sock_cmd_h.keyword[0] == 'G' && sock_cmd_h.keyword[1] == 'S' && sock_cmd_h.keyword[2] == 'T' && sock_cmd_h.keyword[3] == 'A'){ // get statetool
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
				
    			mGetStateToolEn = 1;
    			printf("Cmd1:'GSTA' mGetStateToolEn = 1\n");
//    			Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mGetStateToolEn", ""+mGetStateToolEn);
			    			
    			*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
    		else if(sock_cmd_h.keyword[0] == 'G' && sock_cmd_h.keyword[1] == 'P' && sock_cmd_h.keyword[2] == 'A' && sock_cmd_h.keyword[3] == 'R'){ // get parameterstool
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
				
    			mGetParametersToolEn = 1;
    			printf("Cmd1:'GPAR' mGetParametersToolEn = 1\n");
//    			Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mGetParametersToolEn", ""+mGetParametersToolEn);
    			
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
    		else if(sock_cmd_h.keyword[0] == 'G' && sock_cmd_h.keyword[1] == 'A' && sock_cmd_h.keyword[2] == 'J' && sock_cmd_h.keyword[3] == 'S'){ // get sensortool
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			mGetSensorToolEn = 1;
    			printf("Cmd1:'GAJS' mGetSensorToolEn = 1\n");
			    			
    			*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
    		else if(sock_cmd_h.keyword[0] == 'S' && sock_cmd_h.keyword[1] == 'A' && sock_cmd_h.keyword[2] == 'S' && sock_cmd_h.keyword[3] == 'I'){ // set adj sensor idx
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			char tmp[4];
    			int /*idx,*/ pos = 16;
				memcpy(&tmp[0], &buf[pos], 4); 
//    			idx = byte2Int(&tmp[0], 4);
//    			Main.AdjSensorIdx = idx;
//    			printf("Cmd1:'SASI' AdjSensorIdx = %d\n", Main.AdjSensorIdx);
			    			
    			*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
    		else if(sock_cmd_h.keyword[0] == 'O' && sock_cmd_h.keyword[1] == 'L' && sock_cmd_h.keyword[2] == 'E' && sock_cmd_h.keyword[3] == 'D'){ // get oled num
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			char a[4];
				memcpy(&a[0], &buf[16], 4); 
    			ctrlOLEDNum = byte2Int(&a[0], 4);
			    			
    			mCtrlOLEDEn = 1;
				printf("Cmd1:'OLED' ctrlOLEDNum = %d\n", ctrlOLEDNum);
//    			Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "ctrlOLEDNum", ""+ctrlOLEDNum);
			    			
    			*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
    		else if(sock_cmd_h.keyword[0] == 'S' && sock_cmd_h.keyword[1] == 'P' && sock_cmd_h.keyword[2] == 'A' && sock_cmd_h.keyword[3] == 'R'){ // set Parameterstool
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			char a[4];
				memcpy(&a[0], &buf[16], 4); 
    			mParametersNum = byte2Int(&a[0], 4);
		    			
				memcpy(&a[0], &buf[20], 4); 
    			mParametersVal = byte2Int(&a[0], 4);
			    			
    			mSetParametersToolEn = 1;
    			printf("Cmd1:'SPAR' mParametersNum = %d , mParametersVal = %d\n", mParametersNum, mParametersVal);
//    			Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mParametersNum"+mParametersNum, ""+mParametersVal);
			    			
    			*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
    		else if(sock_cmd_h.keyword[0] == 'S' && sock_cmd_h.keyword[1] == 'A' && sock_cmd_h.keyword[2] == 'J' && sock_cmd_h.keyword[3] == 'S'){ // set Sensortool
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			char a[4];
				memcpy(&a[0], &buf[16], 4); 
    			mSensorToolNum = byte2Int(&a[0], 4);
		    			
				memcpy(&a[0], &buf[20], 4); 
    			mSensorToolVal = byte2Int(&a[0], 4);
			    			
    			mSetSensorToolEn = 1;
    			printf("Cmd1:'SAJS' mSensorToolNum = %d , mSensorToolVal = %d\n", mSensorToolNum, mSensorToolVal);
			    			
    			*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
    		else if(sock_cmd_h.keyword[0] == 'G' && sock_cmd_h.keyword[1] == 'P' && sock_cmd_h.keyword[2] == 'S' && sock_cmd_h.keyword[3] == 'L'){ // set GPS Location
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			 
    			char data[8];
				memcpy(&data[0], &buf[16], 8); 
    			mGPSLocation[0] = byte2Double(&data[0], 8);
			    			
				memcpy(&data[0], &buf[16+8], 8); 
    			mGPSLocation[1] = byte2Double(&data[0], 8);
	    			
				memcpy(&data[0], &buf[16+16], 8); 
    			mGPSLocation[2] = byte2Double(&data[0], 8);
			    			
    			mGPSEn = 1;
    			if(gSocket != -1 && type == 0){
    				//location app連線中
    			}else{
//    				setGPSLocation(1, mGPSLocation[0], mGPSLocation[1], mGPSLocation[2]);
    			}
			    			
    			printf("Cmd1:'GPSL' GPS mGPSLocation[0]=%f , mGPSLocation[1]=%f , mGPSLocation[2]=%f", mGPSLocation[0], mGPSLocation[1], mGPSLocation[2]);
    			//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mGPS", "---");
    			
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
    		else if(sock_cmd_h.keyword[0] == 'A' && sock_cmd_h.keyword[1] == 'U' && sock_cmd_h.keyword[2] == 'D' && sock_cmd_h.keyword[3] == 'O'){ // get audio data
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
/*			    			
				// rtmp audio
				if(Main.rtmp_switch == 0 && Main.mPublisher != null){
					if(Main.mPublisher.sumAudioBufSize >= (Main.mPublisher.maxAudioBufSize*2/5) && Main.mPublisher.bufferRound == 0){
						Main.mPublisher.audioBufInPos -= (Main.mPublisher.maxAudioBufSize/5);
						Main.mPublisher.sumAudioBufSize -= (Main.mPublisher.maxAudioBufSize/5);
					}else{
						byte[] a = new byte[4];
						System.arraycopy(socketDataQ, 24, a, 0, 4);
						int readBufSize = ByteUtil.byte2Int(a);
									
						System.arraycopy(socketDataQ, 28, a, 0, 4);
						int audioBufSize = ByteUtil.byte2Int(a);
									 
						byte[] data = new byte[audioBufSize];
						System.arraycopy(socketDataQ, 32, data, 0, audioBufSize);
									
						if(readBufSize > 100 && audioBufSize > 100){
							if((Main.mPublisher.audioBufInPos+readBufSize) <= SrsPublisher.MAX_BUFFER_SIZE){
								System.arraycopy(data, 0, Main.mPublisher.audioBuffer, Main.mPublisher.audioBufInPos, readBufSize);
											
								if((Main.mPublisher.audioBufInPos+readBufSize) == SrsPublisher.MAX_BUFFER_SIZE){
									Main.mPublisher.audioBufInPos = 0;
									Main.mPublisher.bufferRound++;
								}else{
									Main.mPublisher.audioBufInPos += readBufSize;
								}
								Main.mPublisher.sumAudioBufSize += readBufSize;
							}else{
								int offset = SrsPublisher.MAX_BUFFER_SIZE - Main.mPublisher.audioBufInPos;
								int offset2 = readBufSize - offset;
								if(offset > 0)
									System.arraycopy(data, 0, Main.mPublisher.audioBuffer, Main.mPublisher.audioBufInPos, offset);
								System.arraycopy(data, offset, Main.mPublisher.audioBuffer, 0, offset2);
											
								Main.mPublisher.audioBufInPos = offset2;
								Main.mPublisher.sumAudioBufSize += readBufSize;
								Main.mPublisher.bufferRound++;
							}
						}
					}
				}
			    			
    			// record audio
    			if(Main.Time_Lapse_Mode == 0){
    				//if(Main.ls_audioTS.size() >= 200 && mAudioEn == 0){
					//	Main.ls_audioTS.clear();
    				//	Main.ls_readBufSize.clear();
    				//	Main.ls_audioBuf.clear();
    				//}
			    				
	    			byte[] a = new byte[8];
					System.arraycopy(socketDataQ, 16, a, 0, 8);
	    			long audioTS = ByteUtil.byte2Long(a);
	    			Main.ls_audioTS.add(audioTS);
				    			
	    			a = new byte[4];
	    			System.arraycopy(socketDataQ, 24, a, 0, 4);
	    			int readBufSize = ByteUtil.byte2Int(a);
	    			Main.ls_readBufSize.add(readBufSize);
				    			
	    			System.arraycopy(socketDataQ, 28, a, 0, 4);
	    			int audioBufSize = ByteUtil.byte2Int(a);
				    			 
	    			byte[] data = new byte[audioBufSize];
	    			System.arraycopy(socketDataQ, 32, data, 0, audioBufSize);
	    			Main.ls_audioBuf.add(data);
				    			
	    			mAudioEn = 1;
    			}
*/	
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
    		else if(sock_cmd_h.keyword[0] == 'I' && sock_cmd_h.keyword[1] == 'D' && sock_cmd_h.keyword[2] == 'C' && sock_cmd_h.keyword[3] == 'K'){ // id check
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
				
    			mIdCheckEn = 1;
    			printf("Cmd1:'IDCK' mIdCheck = %d\n", mIdCheckEn);
			    		
    			*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
    		else if(sock_cmd_h.keyword[0] == 'W' && sock_cmd_h.keyword[1] == 'F' && sock_cmd_h.keyword[2] == 'Q' && sock_cmd_h.keyword[3] == 'T'){ // wifi quest
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			char a[4];
				memcpy(&a[0], &buf[16], 4); 
    			int wifiQuest = byte2Int(&a[0], 4);
			    			
    			if(wifiQuest == 0) {
	    			mIdCheckEn = 1;
    			}
		    			
    			*m += (16+dataLen+*skip);
    			*ost -= (16+dataLen+*skip);
				*skip = 0;

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
    		else if(sock_cmd_h.keyword[0] == 'C' && sock_cmd_h.keyword[1] == 'S' && sock_cmd_h.keyword[2] == 'T' && sock_cmd_h.keyword[3] == 'M'){ // Color ST Mode
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			char a[4];
				memcpy(&a[0], &buf[16], 4); 
    			int mode = byte2Int(&a[0], 4);
			    			
    			mColorSTModeEn = 1;
    			mColorSTMode = mode;
				
    			*m += (16+dataLen+*skip);
    			*ost -= (16+dataLen+*skip);
				*skip = 0;
				//Main.systemlog.addLog("info", System.currentTimeMillis(), hostNameTmp, "Change Color ST Mode.", String.valueOf(mColorSTMode));

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
    		else if(sock_cmd_h.keyword[0] == 'T' && sock_cmd_h.keyword[1] == 'R' && sock_cmd_h.keyword[2] == 'N' && sock_cmd_h.keyword[3] == 'S'){ // Translucent control
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			char a[4];
				memcpy(&a[0], &buf[16], 4); 
    			int mode = byte2Int(&a[0], 4);
			    			
    			mTranslucentModeEn = 1;
    			mTranslucentMode = mode;
				
    			*m += (16+dataLen+*skip);
    			*ost -= (16+dataLen+*skip);
				*skip = 0;
							
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "Translucent", ""+mTranslucentMode);

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
    		else if(sock_cmd_h.keyword[0] == 'A' && sock_cmd_h.keyword[1] == 'G' && sock_cmd_h.keyword[2] == 'P' && sock_cmd_h.keyword[3] == 'M'){ // Auto Global Phi Adj Mode
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			char a[4];
				memcpy(&a[0], &buf[16], 4); 
    			int mode = byte2Int(&a[0], 4);
			    			
    			mAutoGlobalPhiAdjEn = 1;
    			mAutoGlobalPhiAdjMode = mode;
			    			
//    			Main.systemlog.addLog("info", System.currentTimeMillis(), hostNameTmp, "Change Auto Stitch Mode.", String.valueOf(mAutoGlobalPhiAdjMode));
				
    			*m += (16+dataLen+*skip);
    			*ost -= (16+dataLen+*skip);
				*skip = 0;

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
    		else if(sock_cmd_h.keyword[0] == 'A' && sock_cmd_h.keyword[1] == 'G' && sock_cmd_h.keyword[2] == 'P' && sock_cmd_h.keyword[3] == 'O'){ // do Auto Global Phi Adj One Time
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			char a[4];
				memcpy(&a[0], &buf[16], 4); 
    			int mode = byte2Int(&a[0], 4);
			    			
    			mAutoGlobalPhiAdjOneTimeEn = mode;
	    			
//    			Main.systemlog.addLog("info", System.currentTimeMillis(), hostNameTmp, "Do Auto Stitch One Time.", String.valueOf(mAutoGlobalPhiAdjOneTimeEn));
				
    			*m += (16+dataLen+*skip);
    			*ost -= (16+dataLen+*skip);
				*skip = 0;

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
			else if(sock_cmd_h.keyword[0] == 'E' && sock_cmd_h.keyword[1] == 'T' && sock_cmd_h.keyword[2] == 'H' && sock_cmd_h.keyword[3] == 'S'){	// eth state
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
					    	
		    	sendEthStateEn = 1;
			    			
    			*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
			else if(sock_cmd_h.keyword[0] == 'H' && sock_cmd_h.keyword[1] == 'D' && sock_cmd_h.keyword[2] == 'T' && sock_cmd_h.keyword[3] == 'V'){ // HDMI TextView visibility
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			char a[4];
				memcpy(&a[0], &buf[16], 4); 
    			int mode = byte2Int(&a[0], 4);
	    			
    			mHDMITextVisibilityEn = 1;
    			mHDMITextVisibilityMode = mode;
				
    			*m += (16+dataLen+*skip);
    			*ost -= (16+dataLen+*skip);
				*skip = 0;
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mHDMITextVisibilityMode", ""+mHDMITextVisibilityMode);

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
			else if(sock_cmd_h.keyword[0] == 'W' && sock_cmd_h.keyword[1] == 'F' && sock_cmd_h.keyword[2] == 'I' && sock_cmd_h.keyword[3] == 'D'){ // wifi ssid
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			mWifiSsidEn = 1;
				
    			*m += (16+dataLen+*skip);
    			*ost -= (16+dataLen+*skip);
				*skip = 0;

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
			else if(sock_cmd_h.keyword[0] == 'R' && sock_cmd_h.keyword[1] == 'M' && sock_cmd_h.keyword[2] == 'C' && sock_cmd_h.keyword[3] == 'F'){ // RTMP Configure
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			char a[512];
				memcpy(&a[0], &buf[16], 512);
//    			Main.rtmpUrl = (new String(a)).trim();
			    			
    			mRTMPConfigureEn = 1;
				
    			*m += (16+dataLen+*skip);
    			*ost -= (16+dataLen+*skip);
				*skip = 0;

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
			else if(sock_cmd_h.keyword[0] == 'A' && sock_cmd_h.keyword[1] == 'D' && sock_cmd_h.keyword[2] == 'I' && sock_cmd_h.keyword[3] == 'F'){ // Audio Infomation
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			char a[4];
				memcpy(&a[0], &buf[16], 4);
//    			int rate = byte2Int(&a[0], 4);
		    			
				memcpy(&a[0], &buf[20], 4);
//    			int bit = byte2Int(&a[0], 4);
			    			
				memcpy(&a[0], &buf[24], 4);
//    			int channel = byte2Int(&a[0], 4);
			    			
//			    Main.wifi_audioRate = rate;
//			    Main.wifi_audioBit = bit;
//			    Main.wifi_audioChannel = channel;
				
    			*m += (16+dataLen+*skip);
    			*ost -= (16+dataLen+*skip);
				*skip = 0;

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
			else if(sock_cmd_h.keyword[0] == 'U' && sock_cmd_h.keyword[1] == 'N' && sock_cmd_h.keyword[2] == 'I' && sock_cmd_h.keyword[3] == 'N'){ // uninstalldates
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			mUninstalldatesEn = 1;
				
    			*m += (16+dataLen+*skip);
    			*ost -= (16+dataLen+*skip);
				*skip = 0;

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
			else if(sock_cmd_h.keyword[0] == 'B' && sock_cmd_h.keyword[1] == 'I' && sock_cmd_h.keyword[2] == 'T' && sock_cmd_h.keyword[3] == 'R'){ // Rec Bitrate控制
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			char a[4];
				memcpy(&a[0], &buf[16], 4);
    			int mode = byte2Int(&a[0], 4);
			    			
    			mBitrateEn = 1;
    			mBitrateMode = mode;
			    			
    			//Main.systemlog.addLog("info", System.currentTimeMillis(), hostNameTmp, "Change Bitrate.", String.valueOf(mBitrateMode));
				
    			*m += (16+dataLen+*skip);
    			*ost -= (16+dataLen+*skip);
				*skip = 0;

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
			else if(sock_cmd_h.keyword[0] == 'L' && sock_cmd_h.keyword[1] == 'B' && sock_cmd_h.keyword[2] == 'T' && sock_cmd_h.keyword[3] == 'R'){ // Live Bitrate控制
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			char a[4];
				memcpy(&a[0], &buf[16], 4);
    			int mode = byte2Int(&a[0], 4);
			    			
    			mLiveBitrateEn = 1;
    			mLiveBitrateMode = mode;
			    			
    			//Main.systemlog.addLog("info", System.currentTimeMillis(), hostNameTmp, "Change Live Bitrate.", String.valueOf(mLiveBitrateMode));
				
    			*m += (16+dataLen+*skip);
    			*ost -= (16+dataLen+*skip);
				*skip = 0;

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
			else if(sock_cmd_h.keyword[0] == 'I' && sock_cmd_h.keyword[1] == 'M' && sock_cmd_h.keyword[2] == 'G' && sock_cmd_h.keyword[3] == 'Q'){ // JPEG Quality Mode
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			char a[4];
				memcpy(&a[0], &buf[16], 4);
    			int mode = byte2Int(&a[0], 4);
			    			
    			mJPEGQualityModeEn = 1;
    			mJPEGQualityMode = mode;
				
    			*m += (16+dataLen+*skip);
    			*ost -= (16+dataLen+*skip);
				*skip = 0;

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
			else if(sock_cmd_h.keyword[0] == 'S' && sock_cmd_h.keyword[1] == 'P' && sock_cmd_h.keyword[2] == 'K' && sock_cmd_h.keyword[3] == 'S'){ // speaker Mode
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			char a[4];
				memcpy(&a[0], &buf[16], 4);
    			int mode = byte2Int(&a[0], 4);
			    			
    			mSpeakerModeEn = 1;
    			mSpeakerMode = mode;
				
    			*m += (16+dataLen+*skip);
    			*ost -= (16+dataLen+*skip);
				*skip = 0;
				//Log.d("WifiServer","SpeakerMode:"+mSpeakerMode);
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mSpeakerMode", ""+mSpeakerMode);
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
			else if(sock_cmd_h.keyword[0] == 'L' && sock_cmd_h.keyword[1] == 'E' && sock_cmd_h.keyword[2] == 'D' && sock_cmd_h.keyword[3] == 'B'){ // ledBrightness Mode
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			char a[4];
				memcpy(&a[0], &buf[16], 4);
    			int value = byte2Int(&a[0], 4);
			    			
    			mLedBrightnessEn = 1;
    			mLedBrightness = value;
				
    			*m += (16+dataLen+*skip);
    			*ost -= (16+dataLen+*skip);
				*skip = 0;
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mLedBrightness", ""+mLedBrightness);

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
			else if(sock_cmd_h.keyword[0] == 'O' && sock_cmd_h.keyword[1] == 'L' && sock_cmd_h.keyword[2] == 'D' && sock_cmd_h.keyword[3] == 'S'){ // oledControl Mode
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			char a[4];
				memcpy(&a[0], &buf[16], 4);
    			int mode = byte2Int(&a[0], 4);
			    			
    			mOledControlEn = 1;
    			mOledControl = mode;
				
    			*m += (16+dataLen+*skip);
    			*ost -= (16+dataLen+*skip);
				*skip = 0;
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mOledControl", ""+mOledControl);

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
			else if(sock_cmd_h.keyword[0] == 'R' && sock_cmd_h.keyword[1] == 'E' && sock_cmd_h.keyword[2] == 'S' && sock_cmd_h.keyword[3] == 'O'){ // 解析度儲存 
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			char a[4];
				memcpy(&a[0], &buf[16], 4);
    			int mode = byte2Int(&a[0], 4);
			    			
    			char b[4];
				memcpy(&b[0], &buf[20], 4);
    			int reso = byte2Int(&b[0], 4);
			    			
    			mResoSaveEn = 1;
    			mResoSaveMode = mode;
    			mResoSaveData = reso;
				
    			*m += (16+dataLen+*skip);
    			*ost -= (16+dataLen+*skip);
				*skip = 0;
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mResoSaveMode"+mResoSaveMode, ""+mResoSaveData);

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
			else if(sock_cmd_h.keyword[0] == 'C' && sock_cmd_h.keyword[1] == 'M' && sock_cmd_h.keyword[2] == 'C' && sock_cmd_h.keyword[3] == 'D'){ // 延遲拍照/錄影
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			char a[4];
				memcpy(&a[0], &buf[16], 4);
    			int time = byte2Int(&a[0], 4);
			    			
    			mCmcdTimeEn = 1;
    			mCmcdTime = time;
				
    			*m += (16+dataLen+*skip);
    			*ost -= (16+dataLen+*skip);
				*skip = 0;
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mCmcdTime", ""+mCmcdTime);

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
			else if(sock_cmd_h.keyword[0] == 'D' && sock_cmd_h.keyword[1] == 'E' && sock_cmd_h.keyword[2] == 'L' && sock_cmd_h.keyword[3] == 'Y'){ // 延遲參數
				int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			char a[4];
				memcpy(&a[0], &buf[16], 4);
    			int delay = byte2Int(&a[0], 4);
			    			
    			mDelayValEn = 1;
    			mDelayVal = delay;
				
    			*m += (16+dataLen+*skip);
    			*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'DELY' dely:%d\n", mDelayVal);
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mDelayVal", ""+mDelayVal);
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'C' && sock_cmd_h.keyword[1] == 'O' && sock_cmd_h.keyword[2] == 'M' && sock_cmd_h.keyword[3] == 'P'){ // 電子羅盤校正
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			char a[4];
				memcpy(&a[0], &buf[16], 4);
    			int val = byte2Int(&a[0], 4);
			    								    			
    			mCompassEn = 1;
    			mCompassVal = val;
				
    			*m += (16+dataLen+*skip);
    			*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'COMP' mCompassVal:%d", mCompassVal);
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
			else if(sock_cmd_h.keyword[0] == 'W' && sock_cmd_h.keyword[1] == 'F' && sock_cmd_h.keyword[2] == 'M' && sock_cmd_h.keyword[3] == 'D'){ // wifi Mode
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			int pos = 16;
	    			
    			char BytesWifiMode[4];
				memcpy(&BytesWifiMode[0], &buf[pos], 4);
    			pos += 4;
    			int wifiMode = byte2Int(&BytesWifiMode[0], 4);
			    			
    			char BytesSSIDLength[4];
				memcpy(&BytesSSIDLength[0], &buf[pos], 4);
				pos += 4;
    			int SSIDLength = byte2Int(&BytesSSIDLength[0], 4);
    			char BytesSSID[SSIDLength];
				memcpy(&BytesSSID[0], &buf[pos], SSIDLength);
    			pos += SSIDLength;
    			char newSsid[7];
				sprintf(newSsid, "%s", BytesSSID);
			    			
    			char BytesPWDLength[4];
				memcpy(&BytesPWDLength[0], &buf[pos], 4);
    			pos += 4;
    			int PWDLength = byte2Int(&BytesPWDLength[0], 4);
    			char BytesPWD[PWDLength];
				memcpy(&BytesPWD[0], &buf[pos], PWDLength);
    			pos += PWDLength;
    			char newPwd[8];
				sprintf(newPwd, "%s", BytesPWD);
			    			
    			char BytesWifiType[4];
				memcpy(&BytesWifiType[0], &buf[pos], 4);
    			pos += 4;
//    			int newType = byte2Int(&BytesWifiType[0], 4);
			    			
    			char BytesIPLength[4];
				memcpy(&BytesIPLength[0], &buf[pos], 4);
    			pos += 4;
    			int IPLength = byte2Int(&BytesIPLength[0], 4);
    			char BytesIP[IPLength];
				memcpy(&BytesIP[0], &buf[pos], IPLength);
    			pos += IPLength;
    			char newIP[16];
				sprintf(newIP, "%s", BytesIP);
			    			
    			char BytesGatewayLength[4];
				memcpy(&BytesGatewayLength[0], &buf[pos], 4);
    			pos += 4;
    			int GatewayLength = byte2Int(&BytesGatewayLength[0], 4);
    			char BytesGateway[GatewayLength];
				memcpy(&BytesGateway[0], &buf[pos], GatewayLength);
    			pos += GatewayLength;
    			char newGateway[16];
				sprintf(newGateway, "%s", BytesGateway);
			    			
    			char BytesPrefixLength[4];
				memcpy(&BytesPrefixLength[0], &buf[pos], 4);
    			pos += 4;
    			int PrefixLength = byte2Int(&BytesPrefixLength[0], 4);
    			char BytesPrefix[PrefixLength];
				memcpy(&BytesPrefix[0], &buf[pos], PrefixLength);
    			pos += PrefixLength;
    			char newPrefix[PrefixLength];
				sprintf(newPrefix, "%s", BytesPrefix);
			    			
    			char BytesDns1Length[4];
				memcpy(&BytesDns1Length[0], &buf[pos], 4);
    			pos += 4;
    			int Dns1Length = byte2Int(&BytesDns1Length[0], 4);
    			char BytesDns1[Dns1Length];
				memcpy(&BytesDns1[0], &buf[pos], Dns1Length);
    			pos += Dns1Length;
    			char newDns1[16];
				sprintf(newDns1, "%s", BytesDns1);
			    			
				char BytesDns2Length[4];
				memcpy(&BytesDns2Length[0], &buf[pos], 4);
			    pos += 4;
			    int Dns2Length = byte2Int(&BytesDns2Length[0], 4);
			    char BytesDns2[Dns2Length];
				memcpy(&BytesDns2[0], &buf[pos], Dns2Length);
			    pos += Dns2Length;
			    char newDns2[16];
				sprintf(newDns2, "%s", BytesDns2);
				
				int reboot = 0;
			    if((dataLen + 16) > pos){
			    	char BytesReboot[4];
					memcpy(&BytesReboot[0], &buf[pos], 4);
				    pos += 4;
				    reboot = byte2Int(&BytesReboot[0], 4);	
			    }
			    			
//			    Main.mWifiModeCmd = wifiMode;
//			    Main.wifiSSID = newSsid;
//			    Main.wifiPassword = newPwd;
//			    Main.wifiType = newType;
//			    Main.wifiIP = newIP;
//			    Main.wifiGateway = newGateway;
//			    Main.wifiPrefix = newPrefix;
//			    Main.wifiDns1 = newDns1;
//			    Main.wifiDns2 = newDns2;
//				Main.wifiReboot = reboot;
			    			
			    printf("Cmd1:'WFMD' mWifiModeCmd : Mode = %d Ssid = %s , Pwd = %s reboot = %d\n", wifiMode, newSsid, newPwd, reboot);
			    printf("Cmd1:'WFMD' mWifiModeCmd : type = %d ip = %s , gateway = %s prefix = %s dns1 = %s dns2 = %s\n", wifiMode, newIP, newGateway, newPrefix, newDns1, newDns2);
			    
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'C' && sock_cmd_h.keyword[1] == 'M' && sock_cmd_h.keyword[2] == 'M' && sock_cmd_h.keyword[3] == 'D'){ // Camera Mode
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
			    			
				char a[4];
				memcpy(&a[0], &buf[16], 4);
				int val = byte2Int(&a[0], 4);
													
				mCameraModeEn = 1;
				mCameraModeVal = val;
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'CMMD' mCameraModeVal:%d\n", mCameraModeVal);
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mCameraModeVal", ""+mCameraModeVal);
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'P' && sock_cmd_h.keyword[1] == 'L' && sock_cmd_h.keyword[2] == 'M' && sock_cmd_h.keyword[3] == 'D'){ // playmode cmd Mode
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
				
				char a[4];
				memcpy(&a[0], &buf[16], 4);
				int val = byte2Int(&a[0], 4);				    			
				mPlayMode = val;
				
				memcpy(&a[0], &buf[20], 4);
				val = byte2Int(&a[0], 4);	
				mResoluMode = val;
				
				memcpy(&a[0], &buf[24], 4);
				val = byte2Int(&a[0], 4);
				mCameraModeVal = val;
				
				mRecordHdrVal = 0;
				mPlayTypeVal = 0;
				if(dataLen >= 16){
					memcpy(&a[0], &buf[28], 4);
					val = byte2Int(&a[0], 4);
					mRecordHdrVal = val;
				}
				if(dataLen >= 20){
					memcpy(&a[0], &buf[32], 4);
					val = byte2Int(&a[0], 4);
					mPlayTypeVal = val;
				}
				
				mPlayModeCmdEn = 1;
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'PLMD' mPlayModeCmd P/R/C:%d/%d/%d\n", mPlayMode, mResoluMode, mCameraModeVal);
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mPlayModeCmd P/R/C:"+mPlayMode+"/"+mResoluMode+"/"+mCameraModeVal, "");
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'D' && sock_cmd_h.keyword[1] == 'B' && sock_cmd_h.keyword[2] == 'M' && sock_cmd_h.keyword[3] == 'D'){ // DebugLogMode
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
				
				char a[4];
				memcpy(&a[0], &buf[16], 4);
				int val = byte2Int(&a[0], 4);
													
				mDebugLogModeEn = 1;
				mDebugLogModeVal = val;
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'DBMD' mDebugLogModeVal:%d\n", mDebugLogModeVal);
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mDebugLogModeVal", ""+mDebugLogModeVal);
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'S' && sock_cmd_h.keyword[1] == 'Y' && sock_cmd_h.keyword[2] == 'N' && sock_cmd_h.keyword[3] == 'C'){ //設定同步請求
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
				
				char a[4];
				memcpy(&a[0], &buf[16], 4);
				int val = byte2Int(&a[0], 4);
													
				mDatabinSyncEn = 1;
				mDatabinSyncVal = val;
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'SYNC' mDatabinSyncVal:%d\n", mDatabinSyncVal);
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mDatabinSyncVal", ""+mDatabinSyncVal);
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'B' && sock_cmd_h.keyword[1] == 'O' && sock_cmd_h.keyword[2] == 'S' && sock_cmd_h.keyword[3] == 'W'){ //底圖設定
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
				
				char a[4];
				memcpy(&a[0], &buf[16], 4);
				int mode = byte2Int(&a[0], 4);
													
				mBottomModeEn = 1;
				mBottomModeVal = mode;
				
				memcpy(&a[0], &buf[20], 4);
				int size = byte2Int(&a[0], 4);
				
				mBottomSizeVal = size;
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'BOSW' mBottomModeVal:%d\n", mBottomModeVal);
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mBottomModeVal" + mBottomModeVal, ""+mBottomSizeVal);
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'H' && sock_cmd_h.keyword[1] == 'D' && sock_cmd_h.keyword[2] == 'R' && sock_cmd_h.keyword[3] == 'E'){ // HdrEvMode
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
				
				char a[4];
				memcpy(&a[0], &buf[16], 4);
				int val = byte2Int(&a[0], 4);
													
				mHdrEvModeVal = val;
				mHdrEvModeManual = 0;

				if(dataLen > 20){
					memcpy(&a[0], &buf[20], 4);
					val = byte2Int(&a[0], 4);
					mHdrEvModeManual = val;
					
					memcpy(&a[0], &buf[24], 4);
					val = byte2Int(&a[0], 4);
					mHdrEvModeNumber = val;
					
					memcpy(&a[0], &buf[28], 4);
					val = byte2Int(&a[0], 4);
					mHdrEvModeIncrement = val;
					
					memcpy(&a[0], &buf[32], 4);
					val = byte2Int(&a[0], 4);
					mHdrEvModeStrength = val;
					
					memcpy(&a[0], &buf[36], 4);
					val = byte2Int(&a[0], 4);
					mHdrEvModeTone = val;
				}
				if(dataLen > 24){
					memcpy(&a[0], &buf[40], 4);
					val = byte2Int(&a[0], 4);
					mHdrEvModeDeghost = val;
				}
				mHdrEvModeEn = 1;
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'HDRE' mHdrEvModeVal:%d/%d/%d/%d/%d/%d/%d\n", mHdrEvModeVal, mHdrEvModeManual, mHdrEvModeNumber, mHdrEvModeIncrement, mHdrEvModeStrength, mHdrEvModeTone, mHdrEvModeDeghost);
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mHdrEvModeVal", ""+mHdrEvModeVal);
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'G' && sock_cmd_h.keyword[1] == 'B' && sock_cmd_h.keyword[2] == 'M' && sock_cmd_h.keyword[3] == 'I'){ // getBottomImage
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
				
				mBotmEn = 3;
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'GBMI' mGetBottomImage\n");
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mGetBottomImage", "");
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'I' && sock_cmd_h.keyword[1] == 'D' && sock_cmd_h.keyword[2] == 'T' && sock_cmd_h.keyword[3] == 'B'){ // initialize databin
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
				
				mInitializeDataBinEn = 1;
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'IDTB' mInitializeDataBin\n");
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'G' && sock_cmd_h.keyword[1] == 'T' && sock_cmd_h.keyword[2] == 'H' && sock_cmd_h.keyword[3] == 'M'){ // get thm list
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
			
				mGetTHMListEn = 1;
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'GTHM' mGetTHMLiist\n");
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'S' && sock_cmd_h.keyword[1] == 'T' && sock_cmd_h.keyword[2] == 'H' && sock_cmd_h.keyword[3] == 'M'){ // send thm list
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
				
				char a[4];
				memcpy(&a[0], &buf[16], 4);
				mSendTHMListSize = byte2Int(&a[0], 4);
				
				free_mSendTHMListData();
				mSendTHMListData = calloc(0, mSendTHMListSize);
				if(mSendTHMListData != NULL)
					memcpy(&mSendTHMListData[0], &buf[20], mSendTHMListSize);
				else
					printf("Cmd1:'STHM' calloc error!\n");
				
				mImgEn = 8;
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'STHM' mGetTHMLiist\n");
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'S' && sock_cmd_h.keyword[1] == 'N' && sock_cmd_h.keyword[2] == 'C' && sock_cmd_h.keyword[3] == 'L'){ //感測器開關
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
				
				char a[4];
				memcpy(&a[0], &buf[16], 4);
				int compassMode = byte2Int(&a[0], 4);
													
				mSensorControlEn = 1;
				mCompassModeVal = compassMode;
				
				memcpy(&a[0], &buf[20], 4);
				int gsensorMode = byte2Int(&a[0], 4);
				
				mGsensorModeVal = gsensorMode;
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'SNCL' mSensorControlEn:%d/%d\n", compassMode, mGsensorModeVal);
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mSensorControlEn", mCompassModeVal+"/"+mGsensorModeVal);
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'B' && sock_cmd_h.keyword[1] == 'M' && sock_cmd_h.keyword[2] == 'T' && sock_cmd_h.keyword[3] == 'T'){ //底圖文字控制
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
				
				char a[4];
				memcpy(&a[0], &buf[16], 4);
				int mode = byte2Int(&a[0], 4);
													
				mBottomTextEn = 1;
				mBottomTMode = mode;
				
				memcpy(&a[0], &buf[20], 4);
				mBottomTColor = byte2Int(&a[0], 4);
				
				memcpy(&a[0], &buf[24], 4);
				mBottomBColor = byte2Int(&a[0], 4);
				
				memcpy(&a[0], &buf[28], 4);
				mBottomTFont = byte2Int(&a[0], 4);
				
				memcpy(&a[0], &buf[32], 4);
				mBottomTLoop = byte2Int(&a[0], 4);
				
				memcpy(&a[0], &buf[36], 4);
				int tLen = byte2Int(&a[0], 4);
				
				char b[tLen];
				memcpy(&b[0], &buf[40], tLen);
				sprintf(mBottomText, "%s", b);
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'BMTT' mBottomTextEn:%d/%s\n", mBottomTMode, mBottomText);
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mBottomTextEn",mBottomTMode + "/" +mBottomText);
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'F' && sock_cmd_h.keyword[1] == 'E' && sock_cmd_h.keyword[2] == 'T' && sock_cmd_h.keyword[3] == 'E'){ // FETE 縮時檔案類型
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
				
				char a[4];
				memcpy(&a[0], &buf[16], 4);
				int val = byte2Int(&a[0], 4);
													
				mFpgaEncodeTypeEn = 1;
				mFpgaEncodeTypeVal = val;
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'FETE' mFpgaEncodeTypeVal:%d\n", mFpgaEncodeTypeVal);
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mFpgaEncodeTypeVal", ""+mFpgaEncodeTypeVal);
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'S' && sock_cmd_h.keyword[1] == 'W' && sock_cmd_h.keyword[2] == 'B' && sock_cmd_h.keyword[3] == 'C'){ // set WBRGB
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
				
				char a[4];
				memcpy(&a[0], &buf[16], 4);
				int r = byte2Int(&a[0], 4);
				
				memcpy(&a[0], &buf[20], 4);
				int g = byte2Int(&a[0], 4);
				
				memcpy(&a[0], &buf[24], 4);
				int b = byte2Int(&a[0], 4);
													
				mSetWBColorEn = 1;
				if(r > 255)r = 255;
				if(g > 255)g = 255;
				if(b > 255)b = 255;
				mSetWBRVal = r;
				mSetWBGVal = g;
				mSetWBBVal = b;
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'SWBC' mSetWBColorVal:%d/%d/%d\n", mSetWBRVal, mSetWBGVal, mSetWBBVal);
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mSetWBColorVal", ""+mSetWBRVal+"/"+mSetWBGVal+"/"+mSetWBBVal);
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'G' && sock_cmd_h.keyword[1] == 'W' && sock_cmd_h.keyword[2] == 'B' && sock_cmd_h.keyword[3] == 'C'){ // need WBRGB
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
					
				mGetWBColorEn = 1;
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'GWBC' mGetWBColorEn:1\n");
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mGetWBColorEn", "");
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'S' && sock_cmd_h.keyword[1] == 'C' && sock_cmd_h.keyword[2] == 'N' && sock_cmd_h.keyword[3] == 'T'){ // set contrast
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
					
				char a[4];
				memcpy(&a[0], &buf[16], 4);
				int val = byte2Int(&a[0], 4);
													
				mSetContrastEn = 1;
				mSetContrastVal = val;
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'SCNT' mSetContrastEn:%d\n", mSetContrastVal);
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mSetContrastEn", ""+mSetContrastVal);
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'S' && sock_cmd_h.keyword[1] == 'S' && sock_cmd_h.keyword[2] == 'U' && sock_cmd_h.keyword[3] == 'N'){ // set saturation
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
					
				char a[4];
				memcpy(&a[0], &buf[16], 4);
				int val = byte2Int(&a[0], 4);
													
				mSetSaturationEn = 1;
				mSetSaturationVal = val;
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'SSUN' mSetSaturationEn:%d\n", mSetSaturationVal);
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mSetSaturationEn", ""+mSetSaturationVal);
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'W' && sock_cmd_h.keyword[1] == 'B' && sock_cmd_h.keyword[2] == 'T' && sock_cmd_h.keyword[3] == 'H'){ // 白平衡參考點
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
					
				char a[4];
				memcpy(&a[0], &buf[16], 4);
				int x = byte2Int(&a[0], 4);
				
				memcpy(&a[0], &buf[20], 4);
				int y = byte2Int(&a[0], 4);
													
				mWbTouchEn = 1;
				mWbTouchXVal = x;
				mWbTouchYVal = y;
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'WBTH' mWbTouchEn X/Y: %d/%d\n", mWbTouchXVal, mWbTouchYVal);
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mWbTouchEn X/Y", mWbTouchXVal +"/"+ mWbTouchYVal);
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'B' && sock_cmd_h.keyword[1] == 'M' && sock_cmd_h.keyword[2] == 'O' && sock_cmd_h.keyword[3] == 'D'){ // bmode B快門參數
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
					
				char a[4];
				memcpy(&a[0], &buf[16], 4);
				int mSec = byte2Int(&a[0], 4);
				
				memcpy(&a[0], &buf[20], 4);
				int mGain = byte2Int(&a[0], 4);
													
				mBModeEn = 1;
				mBModeSec = mSec;
				mBModeGain = mGain;
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'BMOD' mMode Sec/Gain: %d/%d\n", mBModeSec, mBModeGain);
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mMode Sec/Gain", mBModeSec +"/"+ mBModeGain);
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'A' && sock_cmd_h.keyword[1] == 'P' && sock_cmd_h.keyword[2] == 'P' && sock_cmd_h.keyword[3] == 'M'){ // app mode APP類型
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
					
				char a[4];
				memcpy(&a[0], &buf[16], 4);
				int mode = byte2Int(&a[0], 4);
				*app_m = mode;
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'APPM' mApp Mode : %d\n", *app_m);
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mApp Mode : ", appMode +"");
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'G' && sock_cmd_h.keyword[1] == 'F' && sock_cmd_h.keyword[2] == 'O' && sock_cmd_h.keyword[3] == 'D'){ // getfolder
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
					
				char a[4];
				memcpy(&a[0], &buf[16], 4);
				int len = byte2Int(&a[0], 4);
				
				char s[len];
				memcpy(&s[0], &buf[20], len);
				sprintf(mGetFolderVal, "%s", s);
				mGetFolderEn = 1;
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'GFOD' mGetFolder val: %s\n", mGetFolderVal);
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mGetFolder val:", mGetFolderVal);
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'A' && sock_cmd_h.keyword[1] == 'E' && sock_cmd_h.keyword[2] == 'B' && sock_cmd_h.keyword[3] == 'E'){ // AEBMode
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
				
				char a[4];
				memcpy(&a[0], &buf[16], 4);
				int val = byte2Int(&a[0], 4);
				
				mAebModeNumber = val;
				
				memcpy(&a[0], &buf[20], 4);
				val = byte2Int(&a[0], 4);
				mAebModeIncrement = val;
				
				mAebModeEn = 1;
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'AEBE' mAEBModeVal:%d/%d\n", mAebModeNumber, mAebModeIncrement);
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mAEBModeVal", ""+mAebModeNumber + "/" + mAebModeIncrement);
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'L' && sock_cmd_h.keyword[1] == 'I' && sock_cmd_h.keyword[2] == 'V' && sock_cmd_h.keyword[3] == 'Q'){ // LiveQualityMode
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
				
				char a[4];
				memcpy(&a[0], &buf[16], 4);
				int val = byte2Int(&a[0], 4);
				
				mLiveQualityMode = val;
				mLiveQualityEn = 1;
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'LIVQ' mLiveQualityMode:%d\n", mLiveQualityMode);
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mLiveQualityMode", ""+mLiveQualityMode);
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'G' && sock_cmd_h.keyword[1] == 'H' && sock_cmd_h.keyword[2] == 'R' && sock_cmd_h.keyword[3] == 'D'){ // 請求HDR預設值
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
				
				mSendHdrDefaultEn = 1;
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'GHRD' mSendHdrDefaultEn:%d\n", mSendHdrDefaultEn);
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mSendHdrDefaultEn", ""+mSendHdrDefaultEn);
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'T' && sock_cmd_h.keyword[1] == 'O' && sock_cmd_h.keyword[2] == 'N' && sock_cmd_h.keyword[3] == 'E'){ // TONE(WDR)設定
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
				
				char a[4];
				memcpy(&a[0], &buf[16], 4);
				int val = byte2Int(&a[0], 4);
				
				mSendToneVal = val;
				mSendToneEn = 1;
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'TONE' mSendToneVal:%d\n", mSendToneVal);
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mSendToneVal", ""+mSendToneVal);
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'G' && sock_cmd_h.keyword[1] == 'E' && sock_cmd_h.keyword[2] == 'S' && sock_cmd_h.keyword[3] == 'T'){ // estimate time獲取預估時間
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
				
				mGetEstimateEn = 1;

				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'GEST' mGetEstimateEn:\n");
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mGetEstimateEn", "");
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'G' && sock_cmd_h.keyword[1] == 'D' && sock_cmd_h.keyword[2] == 'F' && sock_cmd_h.keyword[3] == 'P'){ // 執行去除壞點
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;

				mGetDefectivePixelEn = 1;
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'GDFP' mGetDefectivePixelEn:\n");
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mGetDefectivePixelEn", "");
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'W' && sock_cmd_h.keyword[1] == 'B' && sock_cmd_h.keyword[2] == 'T' && sock_cmd_h.keyword[3] == 'P'){ //wbtp 白平衡色溫設定
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
				
				char a[4];
				memcpy(&a[0], &buf[16], 4);
				int val = byte2Int(&a[0], 4);
				mSendWbTempVal = val;
				
				if(dataLen > 4) {
					memcpy(&a[0], &buf[20], 4);
					val = byte2Int(&a[0], 4);
					mSendWbTintVal = val;
				}
				else
					mSendWbTintVal = 0;
				
				mSendWbTempEn = 1;
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'WBTP' mSendWbTempVal:%d mSendWbTintVal:%d\n", mSendWbTempVal, mSendWbTintVal);
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mSendWbTempVal", ""+mSendWbTempVal);
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'G' && sock_cmd_h.keyword[1] == 'R' && sock_cmd_h.keyword[2] == 'M' && sock_cmd_h.keyword[3] == 'V'){ //grmv動態移除hdr設定
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
				
				char a[4];
				memcpy(&a[0], &buf[16], 4);
				int val = byte2Int(&a[0], 4);
				mGetRemoveHdrMode = val;
				
				memcpy(&a[0], &buf[20], 4);
				val = byte2Int(&a[0], 4);
				mGetRemoveHdrEv = val;
				
				memcpy(&a[0], &buf[24], 4);
				val = byte2Int(&a[0], 4);
				mGetRemoveHdrStrength = val;
				
				mGetRemoveHdrEn = 1;
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'GRMV' mGetRemoveHdrMode:%d, %d/%d\n"+mGetRemoveHdrMode, mGetRemoveHdrEv, mGetRemoveHdrStrength);
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mGetRemoveHdrMode", ""+mGetRemoveHdrMode);
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'C' && sock_cmd_h.keyword[1] == 'M' && sock_cmd_h.keyword[2] == 'S' && sock_cmd_h.keyword[3] == 'G'){ //cmd message
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
				
				char a[4];
				memcpy(&a[0], &buf[16], 4);
				int code = byte2Int(&a[0], 4);
				char msg[32];
				sprintf(msg, "code: %d", code);
				char b[8];
				switch(code){
				case 0:
					memcpy(&b[0], &buf[20], 8);
					mSysTime = byte2Long(&b[0], 8);
					mSetTime = 1;
					sprintf(msg, "%s, time: %lld", msg, mSysTime);
					return -1;
				}
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'CMSG' cmdMessage %s\n", msg);
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "cmdMessage", ""+msg);
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'A' && sock_cmd_h.keyword[1] == 'L' && sock_cmd_h.keyword[2] == 'I' && sock_cmd_h.keyword[3] == 'A'){ //Anti-Aliasing
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
				
				char a[4];
				memcpy(&a[0], &buf[16], 4);
				int val = byte2Int(&a[0], 4);
				
				mGetAntiAliasingVal = val;
				mGetAntiAliasingEn = 1;
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'ALIA' mGetAntiAliasingVal:%d\n", mGetAntiAliasingVal);
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mGetAntiAliasingVal", ""+mGetAntiAliasingVal);
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'R' && sock_cmd_h.keyword[1] == 'A' && sock_cmd_h.keyword[2] == 'L' && sock_cmd_h.keyword[3] == 'A'){ //Removal Anti-Aliasing
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
				
				char a[4];
				memcpy(&a[0], &buf[16], 4);
				int val = byte2Int(&a[0], 4);
				
				mGetRemoveAntiAliasingVal = val;
				mGetRemoveAntiAliasingEn = 1;
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd1:'RALA' mGetRemoveAntiAliasingVal:%d\n", mGetRemoveAntiAliasingVal);
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mGetRemoveAntiAliasingVal", ""+mGetRemoveAntiAliasingVal);
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'P' && sock_cmd_h.keyword[1] == 'W' && sock_cmd_h.keyword[2] == 'S' && sock_cmd_h.keyword[3] == 'M'){ // Power Saving Mode
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
				
				char a[4];
				memcpy(&a[0], &buf[16], 4);
				int mode = byte2Int(&a[0], 4);
				
				mPowerSavingEn = 1;
				mPowerSavingMode = mode;
				
				//Main.systemlog.addLog("info", System.currentTimeMillis(), hostNameTmp, "Change Power Saving Mode.", String.valueOf(mPowerSavingMode));
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'S' && sock_cmd_h.keyword[1] == 'E' && sock_cmd_h.keyword[2] == 'T' && sock_cmd_h.keyword[3] == 'S'){ // Seting UI State
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
				
				char a[4];
				memcpy(&a[0], &buf[16], 4);
				int state = byte2Int(&a[0], 4);
				
				mSetingUIEn = 1;
				mSetingUIState = state;
				//Main.Send_Data_State_t = System.currentTimeMillis();
				//Main.systemlog.addLog("info", System.currentTimeMillis(), hostNameTmp, "Change Seting UI En.", String.valueOf(mSetingUIState));
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'D' && sock_cmd_h.keyword[1] == 'A' && sock_cmd_h.keyword[2] == 'S' && sock_cmd_h.keyword[3] == 'T'){ // Do Auto Stitching
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
				
				char a[4];
				memcpy(&a[0], &buf[16], 4);
				int val = byte2Int(&a[0], 4);
				
				mDoAutoStitchEn = 1;
				mDoAutoStitchVal = val;
				
				//Main.systemlog.addLog("info", System.currentTimeMillis(), hostNameTmp, "Change Do Auto Stitching", String.valueOf(mPowerSavingMode));
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'G' && sock_cmd_h.keyword[1] == 'S' && sock_cmd_h.keyword[2] == 'R' && sock_cmd_h.keyword[3] == 'S'){ // Do Gensor reset
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			char a[4];
    			memcpy(&a[0], &buf[16], 4);
    			int val = byte2Int(&a[0], 4);
			    			
    			mDoGsensorResetEn = 1;
    			mDoGsensorResetVal = val;
			    			
    			//Main.systemlog.addLog("info", System.currentTimeMillis(), hostNameTmp, "Change Do Auto Stitching", String.valueOf(mPowerSavingMode));
			    			
    			*m += (16+dataLen+*skip);
    			*ost -= (16+dataLen+*skip);
				*skip = 0;

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
			else if(sock_cmd_h.keyword[0] == 'L' && sock_cmd_h.keyword[1] == 'A' && sock_cmd_h.keyword[2] == 'S' && sock_cmd_h.keyword[3] == 'S'){ // Laser 開關
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			char a[4];
    			memcpy(&a[0], &buf[16], 4);
    			int val = byte2Int(&a[0], 4);
			    								    			
    			mLaserSwitchEn = 1;
    			mLaserSwitchVal = val;

    			*m += (16+dataLen+*skip);
    			*ost -= (16+dataLen+*skip);
				*skip = 0;
				//Log.d("WifiServer","mLaserSwitchVal:"+mLaserSwitchVal);
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mLaserSwitchVal", ""+mLaserSwitchVal);
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
            else if(sock_cmd_h.keyword[0] == 'D' && sock_cmd_h.keyword[1] == 'W' && sock_cmd_h.keyword[2] == 'D' && sock_cmd_h.keyword[3] == 'C'){ // DBT Write DDR Cmd
                int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			fpga_dbt_rw_cmd_struct cmd;
    			memcpy(&cmd, &buf[16], dataLen);
                					
                mDbtDdrTotalSize = cmd.size;
                mDbtInputDdrDataSize = 0;
                mDbtOutputDdrDataSize = 0;
                mDbtOutputDdrDataOffset = 0;
                setDbtDdrRWCmd(&cmd);
                mDbtDdrCmdEn = 1;

    			*m += (16+dataLen+*skip);
    			*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd:'DWDC' size=%d\n", mDbtDdrTotalSize);
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mLaserSwitchVal", ""+mLaserSwitchVal);
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
            else if(sock_cmd_h.keyword[0] == 'D' && sock_cmd_h.keyword[1] == 'W' && sock_cmd_h.keyword[2] == 'D' && sock_cmd_h.keyword[3] == 'R'){ // DBT Write DDR Data
                int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	{
                    printf("Cmd:'DWDR' size error, buf[0x%x, 0x%x, 0x%x, 0x%x]\n", buf[0], buf[1], buf[2], buf[3]);
                    return -1;
                }
                
                int offset, size;
                memcpy(&offset, &buf[16], sizeof(offset));
				size = (dataLen - sizeof(offset));
			    			
                if((offset + size) <= mDbtDdrTotalSize) {
                    copyDataToDbtDdrRWBuf(&buf[16+sizeof(offset)], size, offset);
                    //printf("Cmd:'DWDR' buf[0x%x, 0x%x, 0x%x, 0x%x]\n", 
                    //    buf[16+sizeof(offset)], buf[16+sizeof(offset)+1], buf[16+sizeof(offset)+2], buf[16+sizeof(offset)+3]);
                }
                						    			
    			//mDbtInputDdrDataEn = 1;
                //mDbtInputDdrDataSize += dataLen;
                if((offset + size) >= mDbtDdrTotalSize) {
                    //mDbtInputDdrDataSize = 0;
                    mDbtDdrTotalSize = 0;
                    mDbtInputDdrDataEn = 1;
                    printf("Cmd:'DWDR' get data finish!, size=%d offset=%d\n", size, offset);
                }
                else
                    printf("Cmd:'DWDR' get data, size=%d offset=%d\n", size, offset);

    			*m += (16+dataLen+*skip);
    			*ost -= (16+dataLen+*skip);
				*skip = 0;
				//printf("Cmd:'DWDR' dataLen=%d total=%d offset=%d m=%d ost=%d\n", dataLen, mDbtDdrTotalSize, mDbtInputDdrDataSize, *m, *ost);
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mLaserSwitchVal", ""+mLaserSwitchVal);
				if(*ost > 0) {
                    //printf("Cmd:'DWDR' 01 m=%d ost=%d buf=[0x%x,0x%x,0x%x,0x%x]\n", *m, *ost, buf[16+dataLen], buf[16+dataLen+1], buf[16+dataLen+2], buf[16+dataLen+3]);
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
                    //printf("Cmd:'DWDR' 02 m=%d ost=%d buf=[0x%x,0x%x,0x%x,0x%x]\n", *m, *ost, buf[0], buf[1], buf[2], buf[3]);
				}
				ret = 1;
    		}
            else if(sock_cmd_h.keyword[0] == 'D' && sock_cmd_h.keyword[1] == 'W' && sock_cmd_h.keyword[2] == 'R' && sock_cmd_h.keyword[3] == 'G'){ // DBT Write Reg Cmd + Data
                int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
			    			
    			fpga_reg_rw_struct cmd;
    			memcpy(&cmd, &buf[16], dataLen);
                						    			
                setDbtRegRWCmd(&cmd);
                mDbtRegCmdEn = 1;

    			*m += (16+dataLen+*skip);
    			*ost -= (16+dataLen+*skip);
				*skip = 0;
				printf("Cmd:'DWRG'\n");
				//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mLaserSwitchVal", ""+mLaserSwitchVal);
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
    		}
			else{
				printf("new_sock_command1() New Command1 Error! *ost=%d CMD=%c.%c.%c.%c\n", 
					*ost, sock_cmd_h.keyword[0], sock_cmd_h.keyword[1], sock_cmd_h.keyword[2], sock_cmd_h.keyword[3]);
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (16+dataLen))	return -1;
				
				*m += (16+dataLen+*skip);
				*ost -= (16+dataLen+*skip);
				*skip = 0;
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[16+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 0;
			}
		}
	}
	
	if(ret == 1) {
		printf("new_sock_command1: Cmd '%c%c%c%c'\n", 
			sock_cmd_h.keyword[0], sock_cmd_h.keyword[1], sock_cmd_h.keyword[2], sock_cmd_h.keyword[3]);
	}
	return ret;
}

/**
 * *m:    目前處理完的大小, 供上層for迴圈判斷
 * *ost:  剩餘大小
 * *skip: 略過多少byte, = 0
 * *buf:  SocketDataQ, 
 * *tbuf: inputStreamQ, 供SocketDataQ資料前移用
 * return ret: -1:Error, 0:未處理, 1:有對應的cmd
 */
int new_sock_command2(int *m, int *ost, int *skip, char *buf, char *tbuf) {
	int ret=0;
	char bHeader[32];
	struct sock_cmd_sta_header_struct sock_cmd_h;
	sock_cmd_sta_header_init(&sock_cmd_h);
	if(*ost >= 32 && buf[0] == 0x20 && buf[1] == 0x17 && buf[2] == 0x03 && buf[3] == 0x16){
		memcpy(&bHeader[0], &buf[0], 32);
	   	sock_cmd_sta_header_Bytes2Data(&bHeader[0], &sock_cmd_h);
		if(!sock_cmd_sta_header_checkHeader(&sock_cmd_h)){
			printf("Check New Command2 Status Error!-----------------------------------------\n");
	   		printf("Check New Command2 Status Error! *ost = %d\n", *ost);
	   		printf("Check New Command2 Status Error! version = 0x%x\n", sock_cmd_h.version);
	   		printf("Check New Command2 Status Error! keyword = %c.%c.%c.%c\n", 
				sock_cmd_h.keyword[0], sock_cmd_h.keyword[1], sock_cmd_h.keyword[2], sock_cmd_h.keyword[3]);
	   		printf("Check New Command2 Status Error! dataLength = %d\n", sock_cmd_h.dataLength);
			printf("Check New Command2 Status Error! sourceId = %d\n", sock_cmd_h.sourceId);
			printf("Check New Command2 Status Error! targetId = %d\n", sock_cmd_h.targetId);
			printf("Check New Command2 Status Error! reserve1 = %d\n", sock_cmd_h.reserve1);
			printf("Check New Command2 Status Error! reserve2 = %d\n", sock_cmd_h.reserve2);
	   		printf("Check New Command2 Status Error! checkSum = %d\n", sock_cmd_h.checkSum);
	   		printf("Check New Command2 Status Error!-----------------------------------------\n");
			*ost = 0;
			return -1;
		}
		else{
			isFeedback = 1;
			if(sock_cmd_h.keyword[0] == 'R' && sock_cmd_h.keyword[1] == 'E' && sock_cmd_h.keyword[2] == 'C' && sock_cmd_h.keyword[3] == 'E'){		// record enable
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (32+dataLen)) return -1;
				
				if(getSockCmdSta_tId_Record() != sock_cmd_h.sourceId){
					setSockCmdSta_recStatus(0);
					setSockCmdSta_tId_Record(sock_cmd_h.sourceId);
					
					char recm[4];
					memcpy(&recm[0], &buf[32], 4);
					if(recm[0] == 'R' && recm[1] == 'E' && recm[2] == 'C' && recm[3] == 'M'){	//Time-Lapse MODE check code
						char a[4];
						memcpy(&a[0], &buf[36], 4);
						mTimeLapseMode = (byte2Int(&a[0], 4)) / 1000;
					}
					else{
						mTimeLapseMode = 0;
					}
					printf("Cmd2:'RECE' [Cmd Status] mRecordEn = 1, mTimeLapseMode = %d\n", mTimeLapseMode);
					mRecordEn = 1;
					
					usleep(1000000);

					setSockCmdSta_recStatus(1);
					sprintf(&sendFeedBackCmd[0][0], "RECE");
				}
				printf("Cmd2:'RECE' [Cmd Status] mRecordEn = 1\n");
				
				*m += (32+dataLen+*skip);
				*ost -= (32+dataLen+*skip);
				*skip = 0;

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[32+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'R' && sock_cmd_h.keyword[1] == 'E' && sock_cmd_h.keyword[2] == 'C' && sock_cmd_h.keyword[3] == 'D'){		// record disable
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (32+dataLen)) return -1;
				
				if(sock_cmd_h.sourceId != getSockCmdSta_tId_Record()){
					setSockCmdSta_recStatus(0);
					setSockCmdSta_tId_Record(sock_cmd_h.sourceId);
					
					mRecordEn = 0;
					
					usleep(1000000);

					setSockCmdSta_recStatus(1);
					sprintf(&sendFeedBackCmd[0][0], "RECD");
				}
				printf("Cmd2:'RECD' [Cmd Status] mRecordEn = 0\n");
				
				*m += (32+dataLen+*skip);
				*ost -= (32+dataLen+*skip);
				*skip = 0;

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[32+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'S' && sock_cmd_h.keyword[1] == 'T' && sock_cmd_h.keyword[2] == 'H' && sock_cmd_h.keyword[3] == 'M'){		// 取THM檔
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (32+dataLen)) return -1;
				
				if(sock_cmd_h.sourceId != getSockCmdSta_tId_Thumbnail()){
					setSockCmdSta_thmStatus(0);
					setSockCmdSta_tId_Thumbnail(sock_cmd_h.sourceId);
					
					mImgEn = 5;
					clear_list(mExistFileName);		//mExistFileName.clear();
				}
				printf("Cmd2:'STHM' [Cmd Status] mImgEn = 5\n");
				
				*m += (32+dataLen+*skip);
				*ost -= (32+dataLen+*skip);
				*skip = 0;

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[32+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'P' && sock_cmd_h.keyword[1] == 'H' && sock_cmd_h.keyword[2] == 'O' && sock_cmd_h.keyword[3] == 'K'){	// 取相簿圖OK
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (32+dataLen)) return -1;
				
				if(sock_cmd_h.sourceId != getSockCmdSta_tId_Image()){
					setSockCmdSta_imgStatus(0);
					setSockCmdSta_tId_Image(sock_cmd_h.sourceId);
					
					mImgEn = 3;
				}
				printf("Cmd2:'PHOK' [Cmd Status] mImgEn = 3\n");
				
				*m += (32+dataLen+*skip);
				*ost -= (32+dataLen+*skip);
				*skip = 0;

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[32+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else{
				printf("new_sock_command2() New Command2 Error! *ost=%d CMD=%c.%c.%c.%c\n", 
					*ost, sock_cmd_h.keyword[0], sock_cmd_h.keyword[1], sock_cmd_h.keyword[2], sock_cmd_h.keyword[3]);
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (32+dataLen))	return -1;
				
				*m += (32+dataLen+*skip);
				*ost -= (32+dataLen+*skip);
				*skip = 0;
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[32+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 0;
			}
		}
	}
	
	if(ret == 1) {
		printf("new_sock_command2: Cmd '%c%c%c%c'\n", 
			sock_cmd_h.keyword[0], sock_cmd_h.keyword[1], sock_cmd_h.keyword[2], sock_cmd_h.keyword[3]);
	}
	return ret;
}

/**
 * *m:    目前處理完的大小, 供上層for迴圈判斷
 * *ost:  剩餘大小
 * *skip: 略過多少byte, = 0
 * *buf:  SocketDataQ, 
 * *tbuf: inputStreamQ, 供SocketDataQ資料前移用
 * return ret: -1:Error, 0:未處理, 1:有對應的cmd
 */
int new_sock_command3(int *m, int *ost, int *skip, char *buf, char *tbuf) {
	int ret=0;
	char bHeader[32];
	struct sock_cmd_sta_header2_struct sock_cmd_h;
	sock_cmd_sta_header2_init(&sock_cmd_h);
	if(*ost >= 32 && buf[0] == 0x20 && buf[1] == 0x17 && buf[2] == 0x03 && buf[3] == 0x29){
		memcpy(&bHeader[0], &buf[0], 32);
	   	sock_cmd_sta_header2_Bytes2Data(&bHeader[0], &sock_cmd_h);
		if(!sock_cmd_sta_header2_checkHeader(&sock_cmd_h)){
			printf("Check New Command3 Status Error!-----------------------------------------\n");
	   		printf("Check New Command3 Status Error! *ost = %d\n", *ost);
	   		printf("Check New Command3 Status Error! version = 0x%x\n", sock_cmd_h.version);
	   		printf("Check New Command3 Status Error! keyword = %c.%c.%c.%c\n", 
				sock_cmd_h.keyword[0], sock_cmd_h.keyword[1], sock_cmd_h.keyword[2], sock_cmd_h.keyword[3]);
	   		printf("Check New Command3 Status Error! dataLength = %d\n", sock_cmd_h.dataLength);
			printf("Check New Command3 Status Error! sourceId = %d\n", sock_cmd_h.sourceId);
			printf("Check New Command3 Status Error! targetId = %d\n", sock_cmd_h.targetId);
			printf("Check New Command3 Status Error! reserve1 = %d\n", sock_cmd_h.reserve1);
			printf("Check New Command3 Status Error! reserve2 = %d\n", sock_cmd_h.reserve2);
	   		printf("Check New Command3 Status Error! checkSum = %d\n", sock_cmd_h.checkSum);
	   		printf("Check New Command3 Status Error!-----------------------------------------\n");
			*ost = 0;
			return -1;
		}
		else{
			isFeedback = 0;
			if(sock_cmd_h.keyword[0] == 'R' && sock_cmd_h.keyword[1] == 'E' && sock_cmd_h.keyword[2] == 'C' && sock_cmd_h.keyword[3] == 'E'){		// record enable
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (32+dataLen)) return -1;

				unsigned long long nowTime;
				get_current_usec(&nowTime);
				if((nowTime - recStTime) > 3000){
					recStTime = nowTime;
					setSockCmdSta_recLock(0);
				}
				
				if(getSockCmdSta_recLock() == 0){
					if(getSockCmdSta_tId_Record() != sock_cmd_h.sourceId){
						setSockCmdSta_recLock(1);
						setSockCmdSta_f_recStatus(1);
						setSockCmdSta_tId_Record(sock_cmd_h.sourceId);
						
						char recm[4];
						memcpy(&recm[0], &buf[32], 4);
						if(recm[0] == 'R' && recm[1] == 'E' && recm[2] == 'C' && recm[3] == 'M'){	//Time-Lapse MODE check code
							char a[4];
							memcpy(&a[0], &buf[36], 4);
							mTimeLapseMode = (byte2Int(&a[0], 4)) / 1000;
						}
						else{
							mTimeLapseMode = 0;
						}
						printf("Cmd3:'RECE' [Cmd Status2] mRecordEn = 1, mTimeLapseMode = %d\n", mTimeLapseMode);
						mRecordEn = 1;
					}
				}
				printf("Cmd3:'RECE' [Cmd Status2] mRecordEn = 1\n");
				
				*m += (32+dataLen+*skip);
				*ost -= (32+dataLen+*skip);
				*skip = 0;

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[32+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'R' && sock_cmd_h.keyword[1] == 'E' && sock_cmd_h.keyword[2] == 'C' && sock_cmd_h.keyword[3] == 'D'){		// record disable
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (32+dataLen)) return -1;

				unsigned long long nowTime;
				get_current_usec(&nowTime);
				if((nowTime - recStTime) > 3000){
					recStTime = nowTime;
					setSockCmdSta_recLock(0);
				}
				
				if(getSockCmdSta_recLock() == 0){
					if(sock_cmd_h.sourceId != getSockCmdSta_tId_Record()){
						setSockCmdSta_recLock(1);
						setSockCmdSta_f_recStatus(0);
						setSockCmdSta_tId_Record(sock_cmd_h.sourceId);
						mRecordEn = 0;
					}
				}
				printf("Cmd3:'RECD' [Cmd Status2] mRecordEn = 0\n");
				
				*m += (32+dataLen+*skip);
				*ost -= (32+dataLen+*skip);
				*skip = 0;

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[32+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'S' && sock_cmd_h.keyword[1] == 'T' && sock_cmd_h.keyword[2] == 'H' && sock_cmd_h.keyword[3] == 'M'){		// 取THM檔
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (32+dataLen)) return -1;
				
				if(sock_cmd_h.sourceId != getSockCmdSta_tId_Thumbnail()){
					setSockCmdSta_tId_Thumbnail(sock_cmd_h.sourceId);
					
					mImgEn = 5;
					clear_list(mExistFileName);		//mExistFileName.clear();
				}
				printf("Cmd3:'STHM' [Cmd Status2] mImgEn = 5\n");
				
				*m += (32+dataLen+*skip);
				*ost -= (32+dataLen+*skip);
				*skip = 0;

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[32+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'P' && sock_cmd_h.keyword[1] == 'H' && sock_cmd_h.keyword[2] == 'O' && sock_cmd_h.keyword[3] == 'K'){	// 取相簿圖OK
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (32+dataLen)) return -1;
				
				if(sock_cmd_h.sourceId != getSockCmdSta_tId_Image()){
					setSockCmdSta_tId_Image(sock_cmd_h.sourceId);
					
					mImgEn = 3;
				}
				printf("Cmd3:'PHOK' [Cmd Status2] mImgEn = 3\n");
				
				*m += (32+dataLen+*skip);
				*ost -= (32+dataLen+*skip);
				*skip = 0;

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[32+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'S' && sock_cmd_h.keyword[1] == 'W' && sock_cmd_h.keyword[2] == 'A' && sock_cmd_h.keyword[3] == 'P'){		// data交換
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (32+dataLen)) return -1;
				
				if(sock_cmd_h.sourceId != getSockCmdSta_tId_Data()){
					setSockCmdSta_tId_Data(sock_cmd_h.sourceId);
					
					setSockCmdSta_dataStatus(0);
					sendDataStatusCmd = 1;
					mConnected = 1;
//					setWifiOledEn(1);
//					unsigned long long nowTime;
//					get_current_usec(&nowTime);
//					Main.Send_Data_State_t = nowTime;
				}
				printf("Cmd3:'SWAP' [Cmd Status2] mDataSwapEn = 1\n");
				
				*m += (32+dataLen+*skip);
				*ost -= (32+dataLen+*skip);
				*skip = 0;

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[32+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else if(sock_cmd_h.keyword[0] == 'R' && sock_cmd_h.keyword[1] == 'M' && sock_cmd_h.keyword[2] == 'S' && sock_cmd_h.keyword[3] == 'W'){		// rtmp開關
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (32+dataLen)) return -1;
				
				if(sock_cmd_h.sourceId != getSockCmdSta_tId_Rtmp()){
					setSockCmdSta_tId_Rtmp(sock_cmd_h.sourceId);
					
					char val[4];
					memcpy(&val[0], &buf[32], 4);
					int mode = byte2Int(&val[0], 4);
					
					mRTMPSwitchEn = 1;
					mRTMPSwitchMode = mode;
				}
				printf("Cmd3:'RMSW' [Cmd Status2] mRtmpSwitchEn = 1\n");
				
				*m += (32+dataLen+*skip);
				*ost -= (32+dataLen+*skip);
				*skip = 0;

				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[32+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 1;
			}
			else{
				printf("new_sock_command3() New Command3 Error! *ost=%d CMD=%c.%c.%c.%c\n", 
					*ost, sock_cmd_h.keyword[0], sock_cmd_h.keyword[1], sock_cmd_h.keyword[2], sock_cmd_h.keyword[3]);
				int dataLen = sock_cmd_h.dataLength;
				if(*ost < (32+dataLen)) return -1;
				
				*m += (32+dataLen+*skip);
				*ost -= (32+dataLen+*skip);
				*skip = 0;
				if(*ost > 0) {
					memcpy(&tbuf[0], &buf[32+dataLen], *ost);
					memcpy(&buf[0], &tbuf[0], *ost);
				}
				ret = 0;
			}
		}
	}
	
	if(ret == 1) {
		printf("new_sock_command3: Cmd '%c%c%c%c'\n", 
			sock_cmd_h.keyword[0], sock_cmd_h.keyword[1], sock_cmd_h.keyword[2], sock_cmd_h.keyword[3]);
	}
	return ret;
}

/**
 * type: 使用哪個遠端APP連線
 */
int process_input_stream(int type) {
	int r1=0, r2=0, r3=0, r4=0;
	int m, max, skip=0;
	int appMode=0;
	int socketOst=0;
	char *socketDataQ = NULL, *tbuf = NULL;

	if(type == 1){						//location模式
		socketOst = gSocketOst;
		socketDataQ = &gSocketDataQ[0];
	}else{
		socketOst = mSocketOst;
		socketDataQ = &mSocketDataQ[0];
	}
	tbuf = &inputStreamQ[0];
	memcpy(tbuf, socketDataQ, socketOst);
	
	max = socketOst;
	for(m = 0; m < max; ) {
		if(socketOst >= 20+skip) {
			r1 = old_sock_command(&m, &socketOst, &skip, socketDataQ, tbuf, max);
            //if(r1 == 1) 
            //    printf("process_input_stream() r1=%d\n", r1);
			if(r1 == -1) {
                printf("process_input_stream() r1=%d\n", r1);
                break;
            }
			
			r2 = new_sock_command1(&m, &socketOst, &skip, socketDataQ, tbuf, &appMode, type);
            //if(r2 == 1) 
            //    printf("process_input_stream() r2=%d\n", r2);
			if(r2 == -1) {
                printf("process_input_stream() r2=%d\n", r2);
                break;
            }
			
			r3 = new_sock_command2(&m, &socketOst, &skip, socketDataQ, tbuf);
            //if(r3 == 1) 
            //    printf("process_input_stream() r3=%d\n", r3);
			if(r3 == -1) {
                printf("process_input_stream() r3=%d\n", r3);
                break;
            }
			
			r4 = new_sock_command3(&m, &socketOst, &skip, socketDataQ, tbuf);
            //if(r4 == 1) 
            //    printf("process_input_stream() r4=%d\n", r4);
			if(r4 == -1) {
                printf("process_input_stream() r4=%d\n", r4);
                break;
            }
			
			if(r1 == 0 && r2 == 0 && r3 == 0 && r4 == 0) {
                //printf("process_input_stream() no cmd\n");
				socketOst = 0;
				break;
			}
		}
		else {
			printf("process_input_stream() socketOst=%d error\n");
			break;
		}
	}
	
	if(type == 1){
		gSocketOst = socketOst;
		//this.gSocketDataQ = socketDataQ;
	}else{
		mSocketOst = socketOst;
		//this.socketDataQ = socketDataQ;
	}
		
	return appMode;
}


/**
 * Create Socket -> cmd_thread Start 
 */
void nSock_thread(void) {
	struct sockaddr_storage their_addr; // 連線者的位址資訊 
	socklen_t sin_size;
	char hostNameTmp[INET6_ADDRSTRLEN];
    int send_buf_size = 0, recv_buf_size = 0;
    struct timeval send_timeout = {0}; 
    struct timeval recv_timeout = {0}; 
    int len = 0;
	
	while(nSock_thread_en) {
		printf("nSock_thread() server: waiting for connections...\n");
		sin_size = sizeof(their_addr);
		sockfd = accept(mserverSocket, (struct sockaddr *)&their_addr, &sin_size);
		if (sockfd == -1) {
			printf("nSock_thread() accept\n");
			continue;
		}
		fcntl(sockfd, F_SETFL, O_NONBLOCK);
		inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), hostNameTmp, sizeof(hostNameTmp));
		printf("nSock_thread() server: got connection from %s\n", hostNameTmp);

		int isMHost=0;
		if(strcmp(mHostName, mGpsHostName) == 0) {
			sprintf(mGpsHostName, " ");
		}
		if(strcmp(hostNameTmp, mGpsHostName) == 0) {
			/*if(gIn != null){
				gIn.close();
				gIn = null;
			}
			if(gOs != null){
				gOs.close();
				gOs = null;
			}
			if(gSocket != null){
				gSocket.close();
			}*/
		}
		if(mSocket != -1){
			/*if(in != null && os != null && !mSocket.isClosed()){
				printf("nSock_thread() get new connect, have oled connect\n");
			}else{
				isMHost = 1;
			}*/
		}else{
			isMHost = 1;
		}
		
		if(strcmp(mHostName, " ") == 0 || isMHost == 1){
			if(mSocket != -1){
				close(mSocket);
				mSocket = -1;
			}
			mSocket = sockfd;
			connectID ++;
			mWifiPwdEn = 1;
	    			
			memcpy(&mHostName[0], &hostNameTmp[0], sizeof(mHostName));
			isMHost = 1;
            
            len = sizeof(int);
            getsockopt(mserverSocket, SOL_SOCKET, SO_SNDBUF, &send_buf_size, &len);
            getsockopt(mserverSocket, SOL_SOCKET, SO_RCVBUF, &recv_buf_size, &len);
            len = sizeof(struct timeval);
            getsockopt(mserverSocket, SOL_SOCKET, SO_SNDTIMEO, &send_timeout, &len);
            getsockopt(mserverSocket, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, &len);
            printf("get socket 01 hostNameTmp=%s s_bs=%d r_bs=%d s_to=%d r_to=%d\n", hostNameTmp, send_buf_size, recv_buf_size, send_timeout.tv_sec, recv_timeout.tv_sec);            
		}else if(strcmp(mHostName, hostNameTmp) == 0){
			mSocket = sockfd;
			connectID ++;
			mWifiPwdEn = 1;
	    			
			memcpy(&mHostName[0], &hostNameTmp[0], sizeof(mHostName));
			isMHost = 1;
		}else{
			qSocket = sockfd;
			
			//qIn = qSocket.getInputStream();
			//qOs = qSocket.getOutputStream();
		}
		printf("nSock_thread() isMHost=%d mSocket=%d qSocket=%d gSocket=%d\n", 
			isMHost, mSocket, qSocket, gSocket);
		//cmd_thread_en = 1;
	}
}

int qSock_command(int *m, int *ost, int *skip, char *buf, int qw) {
	int ret=0;
	char bHeader[16];
	struct socket_cmd_header_struct sock_cmd_h;
	sock_cmd_header_init(&sock_cmd_h);
	
	if(*ost >= 16 && buf[0] == 0x20 && buf[1] == 0x16 && buf[2] == 0x03 && buf[3] == 0x11){
		memcpy(&bHeader[0], &buf[0], 16);
	   	sock_cmd_header_Bytes2Data(&bHeader[0], &sock_cmd_h);
	   	if(!sock_cmd_header_checkHeader(&sock_cmd_h)){
			printf("qSock_command() Second Check New Command Error! socketOst = %d\n", *ost);
			/*
       		Log.d("WifiServer", "Second Check New Command Error!-----------------------------------------");
       		Log.d("WifiServer", "Second Check New Command Error! socketOst = " + *ost);
       		Log.d("WifiServer", "Second Check New Command Error! version = " + sock_cmd_h.version);
       		Log.d("WifiServer", "Second Check New Command Error! keyword = " + sock_cmd_h.keyword);
       		Log.d("WifiServer", "Second Check New Command Error! dataLength = " + sock_cmd_h.dataLength);
       		Log.d("WifiServer", "Second Check New Command Error! checkSum = " + sock_cmd_h.checkSum);
       		Log.d("WifiServer", "Second Check New Command Error!-----------------------------------------");
       		*/
			*ost = 0;
       		return -1;
       	}
       	else{
       		if(sock_cmd_h.keyword[0] == 'I' && sock_cmd_h.keyword[1] == 'D' && sock_cmd_h.keyword[2] == 'C' && sock_cmd_h.keyword[3] == 'K'){ // id check
       			int dataLen = sock_cmd_h.dataLength;
       			if(*ost < (16+dataLen))	return -1;
    			/*	    			
				if(qw) {
					cmdHeader = new SocketCommandHeader(new byte[]{'W','F','Q','T'}, 0);
					qDos.write(cmdHeader.getBytes());
					Log.d("WifiServer", "Second sendWifiQuest");

					cmdHeader = new SocketCommandHeader(new byte[]{'I','D','C','K'}, 8);
					qDos.write(cmdHeader.getBytes());
					qDos.writeInt(3);
					qDos.writeInt(0);
					Log.d("WifiServer", "Second sendIdCheck : value = 3, reConnectWifi : value = 0");
											
					qDos.flush();
				}
    			*/		
       			*m += (16+dataLen+*skip);
       			*ost -= (16+dataLen+*skip);
    			*skip = 0;

				if(*ost > 0) memcpy(&qSocketDataQ[0], &qSocketDataQ[16+dataLen], *ost);
				ret = 1;
			}else if(sock_cmd_h.keyword[0] == 'W' && sock_cmd_h.keyword[1] == 'F' && sock_cmd_h.keyword[2] == 'Q' && sock_cmd_h.keyword[3] == 'T'){ // wifi quest
       			int dataLen = sock_cmd_h.dataLength;
       			if(*ost < (16+dataLen))	return -1;
    			/*		    			
       			byte[] a = new byte[4];
       			System.arraycopy(qSocketDataQ, 16, a, 0, 4);
       			int wifiQuest = ByteUtil.byte2Int(a);
    							    			
       			Log.d("WifiServer", "Second receive WFQT");
    				    			
       			if(wifiQuest == 0){
       				InetAddress ia = qSocket.getInetAddress();
       				mHostName = ia.getCanonicalHostName();
    						    				
        			cmdHeader = new SocketCommandHeader(new byte[]{'I','D','C','K'}, 8);
					qDos.write(cmdHeader.getBytes());
           			qDos.writeInt(0);
           			qDos.writeInt(0);
        			Log.d("WifiServer", "Second sendIdCheck : value = 0, reConnectWifi : value = 0");
        											
        			qDos.flush();
        			Log.d("WifiServer", "Second 1");
        			if(in != null){
        				in.close();
        				in = null;
        			}
        			if(os != null){
        				os.close();
        				os = null;
        			}
        			if(mSocket != null){
        				mSocket.close();
        				mSocket = null;
        			}
        			Log.d("WifiServer", "Second 2");
        			mSocket = qSocket;
    							    				
    				in = mSocket.getInputStream();
    	   			os = mSocket.getOutputStream();
    	   			qIn = null;
    	   			qOs = null;
    	   			qSocket = null;
    	   			Log.d("WifiServer", "Second 3");
    			}
    			*/				    			
    			*m += (16+dataLen+*skip);
    			*ost -= (16+dataLen+*skip);
        		*skip = 0;

				if(*ost > 0) memcpy(&qSocketDataQ[0], &qSocketDataQ[16+dataLen], *ost);
				ret = 1;								
			}else if(sock_cmd_h.keyword[0] == 'A' && sock_cmd_h.keyword[1] == 'P' && sock_cmd_h.keyword[2] == 'P' && sock_cmd_h.keyword[3] == 'M'){ // app mode APP類型
				int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
    			/*				    				
    			byte[] a = new byte[4];
    			System.arraycopy(socketDataQ, 16, a, 0, 4);
    			int mode = ByteUtil.byte2Int(a);
    							    			
    			if(mode == 1){
					gSocket = qSocket;
    				if(gIn != null && gOs != null){
    					gIn.close();
    					gOs.close();
    	  				gIn = null;
    	  				gOs = null;
    	   			}
    				gIn = gSocket.getInputStream();
    	  			gOs = gSocket.getOutputStream();
    	   			qSocket = null;
    	   			qIn = null;
    	   			qOs = null;
    	   			mGpsHostName = hostNameTmp;
    	   			Log.d("WifiServer", "get GpsHostName: "+mGpsHostName);
    			}
    			*/				    			
    			*m += (16+dataLen+*skip);
    			*ost -= (16+dataLen+*skip);
				*skip = 0;
    			//Log.d("WifiServer","mMode Sec/Gain: "+ mBModeSec +"/"+ mBModeGain);
    			//Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "mMode Sec/Gain", mBModeSec +"/"+ mBModeGain);
    			//if(socketOst > 0) System.arraycopy(socketDataQ, 16+dataLen, socketDataQ, 0, socketOst);
				if(*ost > 0) memcpy(&qSocketDataQ[0], &qSocketDataQ[16+dataLen], *ost);
				ret = 1;
    		}else{
    			printf("qSock_command() Second New Command Error! socketOst=%d CMD=%c.%c.%c.%c\n", *ost, sock_cmd_h.keyword[0], sock_cmd_h.keyword[1], sock_cmd_h.keyword[2], sock_cmd_h.keyword[3]);
    			int dataLen = sock_cmd_h.dataLength;
    			if(*ost < (16+dataLen))	return -1;
    			/*				    			
    			cmdHeader = new SocketCommandHeader(new byte[]{'W','F','Q','T'}, 0);
    			qDos.write(cmdHeader.getBytes());
    			Log.d("WifiServer", "Second sendWifiQuest");

    			cmdHeader = new SocketCommandHeader(new byte[]{'I','D','C','K'}, 8);
    			qDos.write(cmdHeader.getBytes());
    			qDos.writeInt(3);
    			qDos.writeInt(0);
				Log.d("WifiServer", "Second sendIdCheck : value = 3, reConnectWifi : value = 0");
    										
    			qDos.flush();
    			*/		    			
    			*m += (16+dataLen+*skip);
    			*ost -= (16+dataLen+*skip);
            	*skip = 0;
            	if(*ost > 0) memcpy(&qSocketDataQ[0], &qSocketDataQ[16+dataLen], *ost);
				ret = 0;
    		}
    	}
	}else{
    	printf("qSock_command() Second Command Error! socketOst=%d CMD=%c.%c.%c.%c\n", *ost, mSocketDataQ[0], mSocketDataQ[1], mSocketDataQ[2], mSocketDataQ[3]);
    	/*					    	
    	SocketCommandHeader cmdHeader = new SocketCommandHeader(new byte[]{'W','F','Q','T'}, 0);
    	qDos.write(cmdHeader.getBytes());
		Log.d("WifiServer", "Second sendWifiQuest");

		cmdHeader = new SocketCommandHeader(new byte[]{'I','D','C','K'}, 8);
		qDos.write(cmdHeader.getBytes());
		qDos.writeInt(3);
		qDos.writeInt(0);
		Log.d("WifiServer", "Second sendIdCheck : value = 3, reConnectWifi : value = 0");
										
		qDos.flush();
    	*/						    	
    	*ost = 0;
		ret = 0;
    }	
	return ret;
}

void qSock_process_input_stream(int qw) {
	int r1=0;
	int qSocketOst, m, max, skip=0;
	char sock_buf[256];
	max = qSocketOst;
	for(m = 0; m < max; ) {
		r1 = qSock_command(&m, &qSocketOst, &skip, &sock_buf[0], qw);
		printf("qSock_process_input_stream() r1=%d\n", r1);
		if(r1 == -1) break;
		
		if(r1 == 0) {
			qSocketOst = 0;
			break;
		}
	}
}

void process_output_stream(int mw, int gw) {
	int ver=0;
	char cmd[16];
	memset(&cmd[0], 0, sizeof(cmd));
	if(mDataBinEn == 1){
		mDataBinEn = 0;
		ver = 0x20151117;
		sprintf(cmd, "DATA");
        printf("SCmd:'DATA' 01\n");	
//		int len = Main.databin.getBytes().length;
//		if(Main.databin != null){
			if(mw){
				send_data(&mSocket, &cmd[0], 4);
				send_data(&mSocket, (char*)&ver, sizeof(ver));
				send_data(&mSocket, &cmd[0], 4);
//				send_data(&mSocket, (char*)&len, sizeof(len));
//				send_data(&mSocket, (char*)&Main.databin, sizeof(Main.databin));
			}
			if(gw){	
				send_data(&gSocket, &cmd[0], 4);
				send_data(&gSocket, (char*)&ver, sizeof(ver));
				send_data(&gSocket, &cmd[0], 4);
//				send_data(&gSocket, (char*)&len, sizeof(len));
//				send_data(&gSocket, (char*)&Main.databin, sizeof(Main.databin));
			}
//		}	
        printf("SCmd:'DATA' 02\n");	
	}
	if(mRecStateEn == 1){
		mRecStateEn = 0;
		ver = 0x20151222;
		sprintf(cmd, "RECS");
		int val=0;
		if(mw){
			send_data(&mSocket, &cmd[0], 4);
			send_data(&mSocket, (char*)&ver, sizeof(ver));
//			if(Main.mRecordEn == 1) val = 0; 
//			else                    val = -1;
			send_data(&mSocket, (char*)&val, sizeof(val));
//			send_data(&mSocket, (char*)&Main.recTime, sizeof(Main.recTime));
		}
		if(gw){
			send_data(&gSocket, &cmd[0], 4);
			send_data(&gSocket, (char*)&ver, sizeof(ver));
//			if(Main.mRecordEn == 1) val = 0; 
//			else                    val = -1;
			send_data(&gSocket, (char*)&val, sizeof(val));
//			send_data(&gSocket, (char*)&Main.recTime, sizeof(Main.recTime));
		}			
//		printf("SCmd:'RECS' recState = %d , recTime = %d\n", Main.mRecordEn, Main.recTime);
	}
	if(mDataSwapEn == 1){
		mDataSwapEn = 0;
		ver = 0x20151222;
		sprintf(cmd, "SWAP");
		int val=0;
		unsigned long long nowTime;
		get_current_usec(&nowTime);
//		Main.systemTime = nowTime;
		if(mw){
			send_data(&mSocket, &cmd[0], 4);
			send_data(&mSocket, (char*)&ver, sizeof(ver));
//			if(Main.write_file_error == 1) val = 3;
//			else						   val = Main.sd_state;
			send_data(&mSocket, (char*)&val, sizeof(val));
//			send_data(&mSocket, (char*)&Main.sd_freesize, sizeof(Main.sd_freesize));
//			send_data(&mSocket, (char*)&Main.sd_allsize, sizeof(Main.sd_allsize));
//			send_data(&mSocket, (char*)&Main.power, sizeof(Main.power));
//			send_data(&mSocket, (char*)&Main.systemTime, sizeof(Main.systemTime));
		}
		if(gw){
			send_data(&gSocket, &cmd[0], 4);
			send_data(&gSocket, (char*)&ver, sizeof(ver));
//			if(Main.write_file_error == 1) val = 3;
//			else						   val = Main.sd_state;
			send_data(&gSocket, (char*)&val, sizeof(val));
//			send_data(&gSocket, (char*)&Main.sd_freesize, sizeof(Main.sd_freesize));
//			send_data(&gSocket, (char*)&Main.sd_allsize, sizeof(Main.sd_allsize));
//			send_data(&gSocket, (char*)&Main.power, sizeof(Main.power));
//			send_data(&gSocket, (char*)&Main.systemTime, sizeof(Main.systemTime));
		}			
	}			
	if(mPhotoEn == 1){
		FILE *fp;
		struct stat sti;
		int nLen=0, bLen=0, len=0;
		
		if (-1 == stat (mPhotoPath, &sti)) {
			printf("process_output_stream() file not find\n");
		}
		else {
			fp = fopen(mPhotoPath, "rb");
			if(fp != NULL){
				nLen = sti.st_size;
				bLen = sizeof(sendImgBuf);
				if(nLen > bLen) len = bLen;
				else			len = nLen;
				fread(&sendImgBuf[0], len, 1, fp);
				fclose(fp);
			}
			
			printf("SCmd:'PHOT' THM Func: file len = %d\n", nLen);
			ver = 0x20151123;
			sprintf(cmd, "PHOT");
			if(mw) {
				send_data(&mSocket, &cmd[0], 4);
				send_data(&mSocket, (char*)&ver, sizeof(ver));
				send_data(&mSocket, (char*)&len, sizeof(len));
				send_data(&mSocket, &sendImgBuf[0], len);
			}
			if(gw) {
				send_data(&gSocket, &cmd[0], 4);
				send_data(&gSocket, (char*)&ver, sizeof(ver));
				send_data(&gSocket, (char*)&len, sizeof(len));
				send_data(&gSocket, &sendImgBuf[0], len);
			}
		}
		mPhotoEn = 0;
	}
	if(mImgEn == 5){
		if(mImgTotle != -1){
			sprintf(cmd, "STHMTOTL");
			if(mw){
				send_data(&mSocket, &cmd[0], 8);
				send_data(&mSocket, (char*)&mImgTotle, sizeof(mImgTotle));
			}
			if(gw){
				send_data(&gSocket, &cmd[0], 8);
				send_data(&gSocket, (char*)&mImgTotle, sizeof(mImgTotle));
			}
			
			printf("SCmd:'STHMTOTL' mImgEn = 5, mTHMTotle = %d\n", mImgTotle);
			mImgTotle = -1;
			mImgEn = 6;
		}
	}
	if(mImgEn == 8){
		if(mImgTotle != -1){
			sprintf(cmd, "STHMTOTL");
			if(mw){
				send_data(&mSocket, &cmd[0], 8);
				send_data(&mSocket, (char*)&mImgTotle, sizeof(mImgTotle));
			}
			if(gw){
				send_data(&gSocket, &cmd[0], 8);
				send_data(&gSocket, (char*)&mImgTotle, sizeof(mImgTotle));
			}
			
			printf("SCmd:'STHMTOTL' mImgEn = 8, mTHMTotle = %d\n", mImgTotle);
			mImgTotle = -1;
			mImgEn = 6;
		}
	}
	if(mImgEn == 6){
		if(mImgLen > 0){
			if(mw){
				send_data(&mSocket, &mImgData[0], mImgLen);
			}
			if(gw){
				send_data(&gSocket, &mImgData[0], mImgLen);
			}
			printf("SCmd:'' mImgEn = 6, mTHMLen = %d\n", mImgLen);
			mImgLen = -1;										
		}	
		
		get_current_usec(&ImgNowTime);
		if(((ImgNowTime - ImgLstTime) > 10000) || (ImgNowTime < ImgLstTime)){
			ImgLstTime = ImgNowTime;
			if(mImgLen == -1)	mImgLen = 0;
		}
	}
	if(mImgEn == 7){
		mImgEn = 0;		
		ver = 0x20151230;
		sprintf(cmd, "STHMISOK");
		if(mw){
			send_data(&mSocket, &cmd[0], 8);
			send_data(&mSocket, (char*)&ver, sizeof(ver));
		}
		if(gw){
			send_data(&gSocket, &cmd[0], 8);
			send_data(&gSocket, (char*)&ver, sizeof(ver));
		}
		printf("SCmd:'STHMISOK' mImgEn = 7, send THM is OK !\n");
		mImgTotle = -1;
	}
	if(mImgEn == 3){
		if(mImgTotle != -1){
			sprintf(cmd, "SIMGTOTL");
			if(mw){
				send_data(&mSocket, &cmd[0], 8);
				send_data(&mSocket, (char*)&mImgTotle, sizeof(mImgTotle));
			}
			if(gw){
				send_data(&gSocket, &cmd[0], 8);
				send_data(&gSocket, (char*)&mImgTotle, sizeof(mImgTotle));
			}
			printf("SCmd:'SIMGTOTL' mImgEn = 3, mImgTotle = %d\n", mImgTotle);
			mImgTotle = -1;
			mImgEn = 1;
		}
	}
	if(mImgEn == 1){
		if(mImgLen > 0){
			if(mw){
				send_data(&mSocket, &mImgData[0], mImgLen);
			}
			if(gw){
				send_data(&gSocket, &mImgData[0], mImgLen);
			}
			printf("SCmd:'' mImgEn = 1, mImgLen = %d\n", mImgLen);
			mImgLen = -1;										
		}	
		
		get_current_usec(&ImgNowTime);
		if(((ImgNowTime - ImgLstTime) > 100000) || (ImgNowTime < ImgLstTime)){
			ImgLstTime = ImgNowTime;
			if(mImgLen == -1)	mImgLen = 0;
		}
	}
	if(mImgEn == 4){
		mImgEn = 0;		
		ver = 0x20151124;
		sprintf(cmd, "SIMGISOK");
		if(mw){
			send_data(&mSocket, &cmd[0], 8);
			send_data(&mSocket, (char*)&ver, sizeof(ver));
		}
		if(gw){
			send_data(&gSocket, &cmd[0], 8);
			send_data(&gSocket, (char*)&ver, sizeof(ver));
		}
		printf("SCmd:'SIMGISOK' mImgEn = 4, send Img is OK !");
		clear_list(mExistFileName);		//mExistFileName.clear();
		mImgTotle = -1;
		mImgLen = 0;
		
		if(isFeedback){
			setSockCmdSta_imgStatus(1);
			sprintf(&sendFeedBackCmd[2][0], "PHOK");
			setSockCmdSta_thmStatus(1);
			sprintf(&sendFeedBackCmd[1][0], "STHM");
		}else{
			setSockCmdSta_thmStatus(0);
		}
	}
	if(mDownloadEn == 3){
		if(mDownloadTotle != -1){
			sprintf(cmd, "DOWNTOTL");
			if(mw){
				send_data(&mSocket, &cmd[0], 8);
				send_data(&mSocket, (char*)&mDownloadTotle, sizeof(mDownloadTotle));
			}
			if(gw){
				send_data(&gSocket, &cmd[0], 8);
				send_data(&gSocket, (char*)&mDownloadTotle, sizeof(mDownloadTotle));
			}
			printf("SCmd:'DOWNTOTL' mDownloadEn = 3, mDownloadTotle = %d\n", mDownloadTotle);
			mDownloadTotle = -1;
			mDownloadEn = 1;
		}
	}
	if(mDownloadEn == 1){
		if(mDownloadLen > 0){
			if(mw){
				send_data(&mSocket, &mDownloadData[0], mDownloadLen);
			}
			if(gw){
				send_data(&gSocket, &mDownloadData[0], mDownloadLen);
			}
			printf("SCmd:'DOWNTOTL' mDownloadEn = 1, mDownloadLen = %d\n", mDownloadLen);
			mDownloadLen = -1;
		}
		
		get_current_usec(&DownloadNowTime);
		if(((DownloadNowTime - DownloadLstTime) > 20000) || (DownloadNowTime < DownloadLstTime)){
			DownloadLstTime = DownloadNowTime;
			if(mDownloadLen == -1)		mDownloadLen = 0;
		}
	}
	if(mDownloadEn == 4){
		mDownloadEn = 0;
		ver = 0x20151130;
		sprintf(cmd, "DOWNISOK");
		if(mw){
			send_data(&mSocket, &cmd[0], 8);
			send_data(&mSocket, (char*)&ver, sizeof(ver));
		}
		if(gw){
			send_data(&gSocket, &cmd[0], 8);
			send_data(&gSocket, (char*)&ver, sizeof(ver));
		}
		printf("SCmd:'DOWNISOK' mDownloadEn = 4, Download is OK !\n");
		clear_list(mDownloadFileName); 	//mDownloadFileName.clear();
		clear_list(mDownloadFileSkip); 	//mDownloadFileSkip.clear();
		mDownloadTotle = -1;
		mDownloadLen = 0;
	}
	if(mDeleteEn == 2){
		mDeleteEn = 0;
		ver = 0x20151203;
		sprintf(cmd, "DELEISOK");
		if(mw){
			send_data(&mSocket, &cmd[0], 8);
			send_data(&mSocket, (char*)&ver, sizeof(ver));
		}
		if(gw){
			send_data(&gSocket, &cmd[0], 8);
			send_data(&gSocket, (char*)&ver, sizeof(ver));
		}
		printf("SCmd:'DELEISOK' Delete is OK !\n");
		clear_list(mDeleteFileName); 	//mDeleteFileName.clear();
	}
	// rex+ 151016,  傳送mjpg圖檔到手機, 
	if(mMjpegEn == 1){
		if(mOutputLength > 0){
			int nLen = mOutputLength;
			ver = 0x20150814;
//			doType = 1;
			if(mw){
				send_data(&mSocket, (char*)&ver, sizeof(ver));
				send_data(&mSocket, (char*)&nLen, sizeof(nLen));
				send_data(&mSocket, &mOutputData[0], nLen);
			}
//			doType = 2;
			if(gw){
				send_data(&gSocket, (char*)&ver, sizeof(ver));
				send_data(&gSocket, (char*)&nLen, sizeof(nLen));
				send_data(&gSocket, &mOutputData[0], nLen);
			}
			mOutputLength = -1;
			mOutputCnt++;
		}
		// 控制每秒傳送5張
		get_current_usec(&nowTime2);
		if(((nowTime2 - lstTime2) > 200000) || (nowTime2 < lstTime2)){
			lstTime2 = nowTime2;
			if(mOutputLength == -1)	mOutputLength = 0;
		}
	}
	// weber+ 160418, rtsp
	if(mRTSPEn == 1){
		get_current_usec(&originalRtspTimer);
//		if(wifiSerCB != null)	wifiSerCB.copyRTSP(); 
		if(rtspBufLength > 0){
/*			if(rtspWidth != SrsEncoder.vOutWidth || rtspHeight != SrsEncoder.vOutHeight){
				Main.outWidth = SrsEncoder.vOutWidth = rtspWidth;
				Main.outHeight = SrsEncoder.vOutHeight = rtspHeight;
				if(Main.mPublisher != null){
					mRTMPSwitchEn = 1;
				}
			}
				
			int h_len = 50;
			if(rtspBufLength < 50)
				h_len = rtspBufLength;
			char h264_header[h_len];
			memcpy(&h264_header[0], &rtspBuffer[1000], h_len);
			int h264_len[2];
			getSPSPPSLen(&h264_header[0], h_len, &h264_len[0]);
			if(h264_len[1] > 0){
				if(SrsEncoder.h264_length == 0){
					int tmp_flags = rtspBuffer[1000+h264_len[1]+4] & 0x1f;
					SrsEncoder.h264_flags = tmp_flags == 5?1:0;
					SrsEncoder.h264_header_len = h264_len;
					SrsEncoder.h264_header = new byte[h264_len[1]];
					System.arraycopy(rtspBuffer, 1000, SrsEncoder.h264_header, 0, h264_len[1]);
					SrsEncoder.h264_data = new byte[rtspBufLength-(1000+h264_len[1])];
					System.arraycopy(rtspBuffer, 1000+h264_len[1], SrsEncoder.h264_data, 0, rtspBufLength-(1000+h264_len[1]));
					SrsEncoder.h264_length = rtspBufLength-(1000+h264_len[1]);
					
					if(Main.mPublisher != null)
						Main.mPublisher.runOnGetYuvFrame();
				}
			}*/
			ver = 0x20160418;
//			doType = 1;
			if(mw){
				send_data(&mSocket, (char*)&ver, sizeof(ver));
				send_data(&mSocket, (char*)&rtspBufLength, sizeof(rtspBufLength));
				send_data(&mSocket, &rtspBuffer[0], rtspBufLength);
			}
//			doType = 2;
			if(gw){
				send_data(&gSocket, (char*)&ver, sizeof(ver));
				send_data(&gSocket, (char*)&rtspBufLength, sizeof(rtspBufLength));
				send_data(&gSocket, &rtspBuffer[0], rtspBufLength);
			}
		}
	}
	if(mBotmTotle != -1){
		sprintf(cmd, "BOTMTOTL");
		if(mw){
			send_data(&mSocket, &cmd[0], 8);
			send_data(&mSocket, (char*)&mBotmTotle, sizeof(mBotmTotle));
		}
		if(gw){
			send_data(&gSocket, &cmd[0], 8);
			send_data(&gSocket, (char*)&mBotmTotle, sizeof(mBotmTotle));
		}
		printf("SCmd:'BOTMTOTL' mBotmTotle = %d\n", mBotmTotle);
		mBotmTotle = -1;
		mBotmEn = 1;
	}
	if(mBotmEn == 1){
		if(mBotmLen > 0){
			if(mw){
				send_data(&mSocket, &mBotmData[0], mBotmLen);
			}
			if(gw){
				send_data(&gSocket, &mBotmData[0], mBotmLen);
			}
			printf("SCmd:'' mBotmLen = %d\n", mBotmLen);
			mBotmLen = -1;
		}
		
		get_current_usec(&BotmNowTime);
		if(((BotmNowTime - BotmLstTime) > 20000) || (BotmNowTime < BotmLstTime)){
			BotmLstTime = BotmNowTime;
			if(mBotmLen == -1)		mBotmLen = 0;
		}
	}
	if(sendFeedBackEn == 1){		// send feedback
		struct socket_cmd_header_struct cmdHeader;
		sprintf(cmd, "FDBK");
printf("SCmd:'FDBK' 00\n");		
		set_sock_cmd_header(&cmd[0], 8, &cmdHeader);
		if(mw){
printf("SCmd:'FDBK' 01-1\n");			
			send_data(&mSocket, (char*)&cmdHeader, sizeof(cmdHeader));
printf("SCmd:'FDBK' 01-2\n");			
			send_data(&mSocket, &sendFeedBackAction[0], 4);
printf("SCmd:'FDBK' 01-3\n");
			send_data(&mSocket, (char*)&sendFeedBackValue, sizeof(sendFeedBackValue));		
		}
		if(gw){			
			send_data(&gSocket, (char*)&cmdHeader, sizeof(cmdHeader));			
			send_data(&gSocket, &sendFeedBackAction[0], 4);
			send_data(&gSocket, (char*)&sendFeedBackValue, sizeof(sendFeedBackValue));
		}
		printf("SCmd:'FDBK' sendFeedBack : action = %s , value = %d\n", sendFeedBackAction, sendFeedBackValue);
		sendFeedBackEn = 0;
	}
	if(mWifiPwdEn == 1){			// send wifi password
		struct socket_cmd_header_struct cmdHeader;
		sprintf(cmd, "WFPW");
		set_sock_cmd_header(&cmd[0], 8, &cmdHeader);
		if(mw){
			send_data(&mSocket, (char*)&cmdHeader, sizeof(cmdHeader));
			send_data(&mSocket, &sendWifiPwdValue[0], 8);
		}
		if(gw){
			send_data(&gSocket, (char*)&cmdHeader, sizeof(cmdHeader));
			send_data(&gSocket, &sendWifiPwdValue[0], 8);
		}
		printf("SCmd:'WFPW' sendWifiPwd : value = %s\n", sendWifiPwdValue);
		mWifiPwdEn = 0;
	}	
	if(mGetStateToolEn == 1){		// send statetool
//		if(Main.stateTool != null
			struct socket_cmd_header_struct cmdHeader;
			sprintf(cmd, "SSTA");
//			set_sock_cmd_header(&cmd[0], Main.stateTool.getBytes().length + 4, &cmdHeader);
			if(mw){
				send_data(&mSocket, (char*)&cmdHeader, sizeof(cmdHeader));
//				send_data(&mSocket, (char*)&Main.stateTool.getBytes().length, sizeof(Main.stateTool.getBytes().length));
//				send_data(&mSocket, (char*)&Main.stateTool.getBytes(), sizeof(Main.stateTool));
			}
			if(gw){
				send_data(&gSocket, (char*)&cmdHeader, sizeof(cmdHeader));
//				send_data(&gSocket, (char*)&Main.stateTool.getBytes().length, sizeof(Main.stateTool.getBytes().length));
//				send_data(&gSocket, (char*)&Main.stateTool.getBytes(), sizeof(Main.stateTool));
			}
//		}
		mGetStateToolEn = 0;
	}
	if(mGetParametersToolEn == 1){		// send parameterstool
//		if(Main.parametersTool != null){
			struct socket_cmd_header_struct cmdHeader;
			sprintf(cmd, "SPAR");
//			set_sock_cmd_header(&cmd[0], Main.parametersTool.getBytes().length + 4, &cmdHeader);
			if(mw){
				send_data(&mSocket, (char*)&cmdHeader, sizeof(cmdHeader));
//				send_data(&mSocket, (char*)&Main.parametersTool.getBytes().length, sizeof(Main.parametersTool.getBytes().length));
//				send_data(&mSocket, (char*)&Main.parametersTool.getBytes(), sizeof(Main.parametersTool));
			}
			if(gw){
				send_data(&gSocket, (char*)&cmdHeader, sizeof(cmdHeader));
//				send_data(&gSocket, (char*)&Main.parametersTool.getBytes().length, sizeof(Main.parametersTool.getBytes().length));
//				send_data(&gSocket, (char*)&Main.parametersTool.getBytes(), sizeof(Main.parametersTool));
			}	
//		}
		mGetParametersToolEn = 0;
	}
	if(mGetSensorToolEn == 1){		// send sensortool
//		if(Main.sensorTool != null){
			struct socket_cmd_header_struct cmdHeader;
			sprintf(cmd, "SAJS");
//			set_sock_cmd_header(&cmd[0], Main.sensorTool.getBytes().length + 4, &cmdHeader);
			if(mw){
				send_data(&mSocket, (char*)&cmdHeader, sizeof(cmdHeader));
//				send_data(&mSocket, (char*)&Main.sensorTool.getBytes().length, sizeof(Main.sensorTool.getBytes().length));
//				send_data(&mSocket, (char*)&Main.sensorTool.getBytes(), sizeof(Main.sensorTool));
			}
			if(gw){
				send_data(&gSocket, (char*)&cmdHeader, sizeof(cmdHeader));
//				send_data(&gSocket, (char*)&Main.sensorTool.getBytes().length, sizeof(Main.sensorTool.getBytes().length));
//				send_data(&gSocket, (char*)&Main.sensorTool.getBytes(), sizeof(Main.sensorTool));
			}
//		}
		mGetSensorToolEn = 0;
	}
	if(mIdCheckEn == 1){			// send id check
		int val = 0;
		struct socket_cmd_header_struct cmdHeader;
		sprintf(cmd, "IDCK");
		set_sock_cmd_header(&cmd[0], 8, &cmdHeader);
		if(mw){
			send_data(&mSocket, (char*)&cmdHeader, sizeof(cmdHeader));
			send_data(&mSocket, (char*)&val, sizeof(val));
			send_data(&mSocket, (char*)&val, sizeof(val));
		}
		if(gw){
			send_data(&gSocket, (char*)&cmdHeader, sizeof(cmdHeader));
			send_data(&gSocket, (char*)&val, sizeof(val));
			send_data(&gSocket, (char*)&val, sizeof(val));
		}
		printf("SCmd:'' sendIdCheck : value = 0, reConnectWifi : value = 0\n");
		mIdCheckEn = 0;
	}
	if(mSyncEn == 1){			// send sync databin
		struct socket_cmd_header_struct cmdHeader;
		sprintf(cmd, "SYNC");
//		set_sock_cmd_header(&cmd[0], Main.databin.getBytes().length + 8, &cmdHeader);
		if(mw){
			send_data(&mSocket, (char*)&cmdHeader, sizeof(cmdHeader));
			send_data(&mSocket, (char*)&mDatabinSyncVal, sizeof(mDatabinSyncVal));
//			send_data(&mSocket, (char*)&Main.databin.getBytes().length, sizeof(Main.databin.getBytes().length));
//			send_data(&mSocket, (char*)&Main.databin.getBytes(), sizeof(Main.databin));
		}
		if(gw){
			send_data(&gSocket, (char*)&cmdHeader, sizeof(cmdHeader));
			send_data(&gSocket, (char*)&mDatabinSyncVal, sizeof(mDatabinSyncVal));
//			send_data(&gSocket, (char*)&Main.databin.getBytes().length, sizeof(Main.databin.getBytes().length));
//			send_data(&gSocket, (char*)&Main.databin.getBytes(), sizeof(Main.databin));
		}
		printf("SCmd:'SYNC' sendSyncDatabin : code:%d\n", mDatabinSyncVal);
		mSyncEn = 0;
	}
	if(sendUninstallEn == 1){
		struct socket_cmd_header_struct cmdHeader;
		sprintf(cmd, "UNIN");
		set_sock_cmd_header(&cmd[0], 4, &cmdHeader);
		if(mw){
			send_data(&mSocket, (char*)&cmdHeader, sizeof(cmdHeader));
			send_data(&mSocket, (char*)&UninstallEn, sizeof(UninstallEn));
		}
		if(gw){
			send_data(&gSocket, (char*)&cmdHeader, sizeof(cmdHeader));
			send_data(&gSocket, (char*)&UninstallEn, sizeof(UninstallEn));
		}
		printf("SCmd:'UNIN' sendUninstallEn : en = %d\n", UninstallEn);
		sendUninstallEn = 0;
	}
	if(sendMacAddressEn == 1){
		char data[64];
		sprintf(data, "%s,%s", &wifiMacAddress[0], &ethMacAddress[0]);
		int len = strlen(data);
		struct socket_cmd_header_struct cmdHeader;
		sprintf(cmd, "MACA");
		set_sock_cmd_header(&cmd[0], 4 + len, &cmdHeader);
		if(mw){
			send_data(&mSocket, (char*)&cmdHeader, sizeof(cmdHeader));
			send_data(&mSocket, (char*)&len, sizeof(len));
			send_data(&mSocket, &data[0], len);
		}
		if(gw){
			send_data(&gSocket, (char*)&cmdHeader, sizeof(cmdHeader));
			send_data(&gSocket, (char*)&len, sizeof(len));
			send_data(&gSocket, &data[0], len);
		}
		printf("SCmd:'MACA' sendMacAddressEn : macData = %s\n", data);
		sendMacAddressEn = 0;
	}
	if(fromWhereConnect == 0){
		int i, sid, tid;
		int size = sizeof(sendFeedBackSTEn) / sizeof(int);
		struct sock_cmd_sta_header_struct cmdHeader;
		char req[8];
		for(i=0; i<size; i++){
			if(sendFeedBackSTEn[i] == 1){
				switch(i){
				case 0:
					if(getSockCmdSta_recStatus() == 1){
						tid = getSockCmdSta_tId_Record();
						setSockCmdSta_sId_Record(tid);
						if(strcmp(&sendFeedBackCmd[i][0], "RECE") == 0){
							sprintf(cmd, "FDBK");
							sid = getSockCmdSta_sId_Record();
							set_sock_cmd_sta_header(&cmd[0], 4, sid, tid, &cmdHeader);
							sprintf(req, "RECE");
							if(mw){
								send_data(&mSocket, (char*)&cmdHeader, sizeof(cmdHeader));
								send_data(&mSocket, &req[0], 4);
							}
							if(gw){
								send_data(&gSocket, (char*)&cmdHeader, sizeof(cmdHeader));
								send_data(&gSocket, &req[0], 4);
							}	
							printf("SCmd:'FDBK' [Cmd Status] sendFeedBack : cmd = RECE\n");
						}else if(strcmp(&sendFeedBackCmd[i][0], "RECD") == 0){
							sprintf(cmd, "FDBK");
							sid = getSockCmdSta_sId_Record();
							set_sock_cmd_sta_header(&cmd[0], 4, sid, tid, &cmdHeader);
							sprintf(req, "RECD");
							if(mw){
								send_data(&mSocket, (char*)&cmdHeader, sizeof(cmdHeader));
								send_data(&mSocket, &req[0], 4);
							}
							if(gw){
								send_data(&gSocket, (char*)&cmdHeader, sizeof(cmdHeader));
								send_data(&gSocket, &req[0], 4);
							}
							printf("SCmd:'FDBK' [Cmd Status] sendFeedBack : cmd = RECD\n");
						}
					}
					sendFeedBackSTEn[0] = 0;
					break;
				case 1:
					if(getSockCmdSta_thmStatus() == 1){
						tid = getSockCmdSta_tId_Thumbnail();
						setSockCmdSta_sId_Thumbnail(tid);
						if(strcmp(&sendFeedBackCmd[i][0], "STHM") == 0){
							sprintf(cmd, "FDBK");
							sid = getSockCmdSta_sId_Thumbnail();
							set_sock_cmd_sta_header(&cmd[0], 4, sid, tid, &cmdHeader);
							sprintf(req, "STHM");
							if(mw){
								send_data(&mSocket, (char*)&cmdHeader, sizeof(cmdHeader));
								send_data(&mSocket, &req[0], 4);
							}
							if(gw){
								send_data(&gSocket, (char*)&cmdHeader, sizeof(cmdHeader));
								send_data(&gSocket, &req[0], 4);
							}	
							printf("SCmd:'FDBK' [Cmd Status] sendFeedBack : cmd = STHM\n");
						}
					}
					sendFeedBackSTEn[1] = 0;
					break;
				case 2:
					if(getSockCmdSta_imgStatus() == 1){
						tid = getSockCmdSta_tId_Image();
						setSockCmdSta_sId_Image(tid);
						if(strcmp(&sendFeedBackCmd[i][0], "PHOK") == 0){
							sprintf(cmd, "FDBK");
							sid = getSockCmdSta_sId_Image();
							set_sock_cmd_sta_header(&cmd[0], 4, sid, tid, &cmdHeader);
							sprintf(req, "PHOK");
							if(mw){
								send_data(&mSocket, (char*)&cmdHeader, sizeof(cmdHeader));
								send_data(&mSocket, &req[0], 4);
							}
							if(gw){
								send_data(&gSocket, (char*)&cmdHeader, sizeof(cmdHeader));
								send_data(&gSocket, &req[0], 4);
							}
							printf("SCmd:'FDBK' [Cmd Status] sendFeedBack : cmd = PHOK\n");
						}
					}
					sendFeedBackSTEn[2] = 0;
					break;
				}
			}
		}
	}
	if(mKelvinEn == 1){			// send id check
		struct socket_cmd_header_struct cmdHeader;
		sprintf(cmd, "KELV");
		set_sock_cmd_header(&cmd[0], 8, &cmdHeader);
		if(mw){
			send_data(&mSocket, (char*)&cmdHeader, sizeof(cmdHeader));
			send_data(&mSocket, (char*)&mKelvinVal, sizeof(mKelvinVal));
			send_data(&mSocket, (char*)&mKelvinTintVal, sizeof(mKelvinTintVal));
		}
		if(gw){
			send_data(&gSocket, (char*)&cmdHeader, sizeof(cmdHeader));
			send_data(&gSocket, (char*)&mKelvinVal, sizeof(mKelvinVal));
			send_data(&gSocket, (char*)&mKelvinTintVal, sizeof(mKelvinTintVal));
		}
		printf("SCmd:'KELV' sendKelvin : value = %d tint=%d\n", mKelvinVal, mKelvinTintVal);
		mKelvinEn = 0;
	}
	if(mMapItemEn == 1){		// send 3D data
		struct socket_cmd_header_struct cmdHeader;
		sprintf(cmd, "MAPI");
		set_sock_cmd_header(&cmd[0], 16, &cmdHeader);
		if(mw){
			send_data(&mSocket, (char*)&cmdHeader, sizeof(cmdHeader));
			send_data(&mSocket, (char*)&mMapItemStatus, sizeof(mMapItemStatus));
			send_data(&mSocket, (char*)&mMapItemNumber, sizeof(mMapItemNumber));
			send_data(&mSocket, (char*)&mMapItemX, sizeof(mMapItemX));
			send_data(&mSocket, (char*)&mMapItemY, sizeof(mMapItemY));
		}
		if(gw){
			send_data(&gSocket, (char*)&cmdHeader, sizeof(cmdHeader));
			send_data(&gSocket, (char*)&mMapItemStatus, sizeof(mMapItemStatus));
			send_data(&gSocket, (char*)&mMapItemNumber, sizeof(mMapItemNumber));
			send_data(&gSocket, (char*)&mMapItemX, sizeof(mMapItemX));
			send_data(&gSocket, (char*)&mMapItemY, sizeof(mMapItemY));
		}			
		printf("sendMapItem : number=%d,x=%d,y=%d\n", mMapItemNumber, mMapItemX, mMapItemY);
		mMapItemEn = 0;
	}		
	if(mRoomDataEn == 1){		// send room data
		struct socket_cmd_header_struct cmdHeader;
		sprintf(cmd, "RMDT");
		set_sock_cmd_header(&cmd[0], 4 + mRoomDataLen*4, &cmdHeader);
		if(mw){
			send_data(&mSocket, (char*)&cmdHeader, sizeof(cmdHeader));
			send_data(&mSocket, (char*)&mRoomDataLen, sizeof(mRoomDataLen));
			for(int x = 0; x < mRoomDataLen; x++){
				send_data(&mSocket, (char*)&mRoomDataVal[x], sizeof(mRoomDataVal[x]));
			}
		}
		if(gw){
			send_data(&gSocket, (char*)&cmdHeader, sizeof(cmdHeader));
			send_data(&gSocket, (char*)&mRoomDataLen, sizeof(mRoomDataLen));
			for(int x = 0; x < mRoomDataLen; x++){
				send_data(&gSocket, (char*)&mRoomDataVal[x], sizeof(mRoomDataVal[x]));
			}
		}		
		printf("sendRoomData : len=%d\n", mRoomDataLen);
		mRoomDataEn = 0;
	}
					
	//主機主動發送自己狀態
	if(sendCamModeCmd == 1){
		sendCamModeCmd = 0;
		int sid = getSockCmdSta_sId_Record();
		int tid = getSockCmdSta_tId_Record();
		int f_sta = getSockCmdSta_f_recStatus();
		int p_sta = getSockCmdSta_p_recStatus();
		if(getSockCmdSta_recStatus() == 0){
			setSockCmdSta_sId_Record(sid+1);
			setSockCmdSta_recStatus(1);
		}
		struct sock_cmd_sta_header2_struct cmdHeader2;
		sprintf(cmd, "CAMO");
		set_sock_cmd_sta_header2(&cmd[0], 20, sid, tid, &cmdHeader2);
		if(mw){
			send_data(&mSocket, (char*)&cmdHeader2, sizeof(cmdHeader2));
//			send_data(&mSocket, (char*)&Main.databin.getCameraMode(), sizeof(Main.databin.getCameraMode()));
			send_data(&mSocket, (char*)&f_sta, sizeof(f_sta));
			send_data(&mSocket, (char*)&p_sta, sizeof(p_sta));
//			send_data(&mSocket, (char*)&Main.recTime, sizeof(Main.recTime));
//			send_data(&mSocket, (char*)&Main.Time_Lapse_Mode, sizeof(Main.Time_Lapse_Mode));
		}
		if(gw){
			send_data(&gSocket, (char*)&cmdHeader2, sizeof(cmdHeader2));
//			send_data(&gSocket, (char*)&Main.databin.getCameraMode(), sizeof(Main.databin.getCameraMode()));
			send_data(&gSocket, (char*)&f_sta, sizeof(f_sta));
			send_data(&gSocket, (char*)&p_sta, sizeof(p_sta));
//			send_data(&gSocket, (char*)&Main.recTime, sizeof(Main.recTime));
//			send_data(&gSocket, (char*)&Main.Time_Lapse_Mode, sizeof(Main.Time_Lapse_Mode));
		}
		printf("SCmd:'CAMO' [Cmd Status2] send CAMO\n");
	}
	if(sendThmStatusCmd == 1){
		sendThmStatusCmd = 0;
		int sid = getSockCmdSta_sId_Thumbnail();
		int tid = getSockCmdSta_tId_Thumbnail();
		if(getSockCmdSta_thmStatus() == 0){
			setSockCmdSta_sId_Thumbnail(sid+1);
			setSockCmdSta_thmStatus(1);
		}
		struct sock_cmd_sta_header2_struct cmdHeader2;
		sprintf(cmd, "THST");
		set_sock_cmd_sta_header2(&cmd[0], 0, sid, tid, &cmdHeader2);
		if(mw){
			send_data(&mSocket, (char*)&cmdHeader2, sizeof(cmdHeader2));
		}
		if(gw){
			send_data(&gSocket, (char*)&cmdHeader2, sizeof(cmdHeader2));
		}
		printf("SCmd:'THST' Cmd Status2] send THST\n");
	}
	if(sendDataStatusCmd == 1){
		sendDataStatusCmd = 0;
		int sid = getSockCmdSta_sId_Data();
		int tid = getSockCmdSta_tId_Data();
		if(getSockCmdSta_dataStatus() == 0){
			setSockCmdSta_sId_Data(sid+1);
			setSockCmdSta_dataStatus(1);
		}
		struct sock_cmd_sta_header2_struct cmdHeader2;
		sprintf(cmd, "DTST");
		set_sock_cmd_sta_header2(&cmd[0], 84, sid, tid, &cmdHeader2);
		int currentValue[3];
		int val=0;
//		getLiveShowValue(currentValue);
		float sensorData[3];
//		getBma2x2orientationdata(sensorData, 1); 
		int pitch = (int)(sensorData[1] * 100);
		int roll = (int)(sensorData[2] * 100);
//		get_current_usec(&Main.systemTime);
		if(mw){
			send_data(&mSocket, (char*)&cmdHeader2, sizeof(cmdHeader2));
//			if(Main.write_file_error == 1) val = 3;
//			else						   val = Main.sd_state;
			send_data(&mSocket, (char*)&val, sizeof(val));
//			send_data(&mSocket, (char*)&Main.sd_freesize, sizeof(Main.sd_freesize));
//			send_data(&mSocket, (char*)&Main.sd_allsize, sizeof(Main.sd_allsize));
//			send_data(&mSocket, (char*)&Main.power, sizeof(Main.power));
//			send_data(&mSocket, (char*)&Main.systemTime, sizeof(Main.systemTime));
//			send_data(&mSocket, (char*)&Main.FPS, sizeof(Main.FPS));
//			send_data(&mSocket, (char*)&Main.freeCount, sizeof(Main.freeCount));
			send_data(&mSocket, (char*)&currentValue[0], sizeof(currentValue[0]));
			send_data(&mSocket, (char*)&currentValue[1], sizeof(currentValue[1]));
			send_data(&mSocket, (char*)&currentValue[2], sizeof(currentValue[2]));
//			send_data(&mSocket, (char*)&Main.captureWaitTime, sizeof(Main.captureWaitTime));
//			send_data(&mSocket, (char*)&Main.captureDCnt, sizeof(Main.captureDCnt));
			send_data(&mSocket, (char*)&pitch, sizeof(pitch));
			send_data(&mSocket, (char*)&roll, sizeof(roll));
//			send_data(&mSocket, (char*)&Main.lidarState, sizeof(Main.lidarState));
//			send_data(&mSocket, (char*)&Main.lidarCode, sizeof(Main.lidarCode));
//			send_data(&mSocket, (char*)&Main.lidarVersion, sizeof(Main.lidarVersion));
		}
		if(gw){
			send_data(&gSocket, (char*)&cmdHeader2, sizeof(cmdHeader2));
//			if(Main.write_file_error == 1) val = 3;
//			else						   val = Main.sd_state;
			send_data(&gSocket, (char*)&val, sizeof(val));
//			send_data(&gSocket, (char*)&Main.sd_freesize, sizeof(Main.sd_freesize));
//			send_data(&gSocket, (char*)&Main.sd_allsize, sizeof(Main.sd_allsize));
//			send_data(&gSocket, (char*)&Main.power, sizeof(Main.power));
//			send_data(&gSocket, (char*)&Main.systemTime, sizeof(Main.systemTime));
//			send_data(&gSocket, (char*)&Main.FPS, sizeof(Main.FPS));
//			send_data(&gSocket, (char*)&Main.freeCount, sizeof(Main.freeCount));
			send_data(&gSocket, (char*)&currentValue[0], sizeof(currentValue[0]));
			send_data(&gSocket, (char*)&currentValue[1], sizeof(currentValue[1]));
			send_data(&gSocket, (char*)&currentValue[2], sizeof(currentValue[2]));
//			send_data(&gSocket, (char*)&Main.captureWaitTime, sizeof(Main.captureWaitTime));
//			send_data(&gSocket, (char*)&Main.captureDCnt, sizeof(Main.captureDCnt));
			send_data(&gSocket, (char*)&pitch, sizeof(pitch));
			send_data(&gSocket, (char*)&roll, sizeof(roll));
//			send_data(&gSocket, (char*)&Main.lidarState, sizeof(Main.lidarState));
//			send_data(&gSocket, (char*)&Main.lidarCode, sizeof(Main.lidarCode));
//			send_data(&gSocket, (char*)&Main.lidarVersion, sizeof(Main.lidarVersion));
		}
		printf("SCmd:'DTST' [Cmd Status2] send DTST\n");
	}
	if(sendRtmpStatusCmd == 1){
		sendRtmpStatusCmd = 0;
		int sid = getSockCmdSta_sId_Rtmp();
		int tid = getSockCmdSta_tId_Rtmp();
		if(getSockCmdSta_rtmpStatus() == 0){
			setSockCmdSta_sId_Rtmp(sid+1);
			setSockCmdSta_rtmpStatus(1);
		}
		struct sock_cmd_sta_header2_struct cmdHeader2;
		sprintf(cmd, "RMSW");
		set_sock_cmd_sta_header2(&cmd[0], 4, sid, tid, &cmdHeader2);
		if(mw){
			send_data(&mSocket, (char*)&cmdHeader2, sizeof(cmdHeader2));
//			send_data(&mSocket, (char*)&Main.rtmp_switch, sizeof(Main.rtmp_switch));
		}
		if(gw){
			send_data(&gSocket, (char*)&cmdHeader2, sizeof(cmdHeader2));
//			send_data(&gSocket, (char*)&Main.rtmp_switch, sizeof(Main.rtmp_switch));
		}
		printf("SCmd:'RMSW' [Cmd Status2] send RMSW\n");
	}
	if(sendEthStateEn == 1){
//		if(Main.mEth != null){
//			int ipLen = Main.mEth.now_IP.getBytes().length;
			int ipLen = 0;
			int dataLen = 4 + 4 + ipLen;
			struct socket_cmd_header_struct cmdHeader;
			sprintf(cmd, "ETHS");
			set_sock_cmd_header(&cmd[0], dataLen, &cmdHeader);
			if(mw){
				send_data(&mSocket, (char*)&cmdHeader, sizeof(cmdHeader));
//				send_data(&mSocket, (char*)&Main.mEth.Ethernet_Connect, sizeof(Main.mEth.Ethernet_Connect));
				send_data(&mSocket, (char*)&ipLen, sizeof(ipLen));
//				send_data(&mSocket, (char*)&Main.mEth.now_IP.getBytes(), sizeof(Main.mEth.now_IP.getBytes()));	
			}
			if(gw){
				send_data(&gSocket, (char*)&cmdHeader, sizeof(cmdHeader));
//				send_data(&gSocket, (char*)&Main.mEth.Ethernet_Connect, sizeof(Main.mEth.Ethernet_Connect));
				send_data(&gSocket, (char*)&ipLen, sizeof(ipLen));
//				send_data(&gSocket, (char*)&Main.mEth.now_IP.getBytes(), sizeof(Main.mEth.now_IP.getBytes()));		
			}
//		}
		sendEthStateEn = 0;
	}
	if(mWifiSsidEn == 1){			// send wifi ssid
		struct socket_cmd_header_struct cmdHeader;
		sprintf(cmd, "WFID");
		set_sock_cmd_header(&cmd[0], sendWifiSsidLen, &cmdHeader);
		if(mw){
			send_data(&mSocket, (char*)&cmdHeader, sizeof(cmdHeader));
			send_data(&mSocket, &sendWifiSsidValue[0], sendWifiSsidLen);
		}
		if(gw){
			send_data(&gSocket, (char*)&cmdHeader, sizeof(cmdHeader));
			send_data(&gSocket, &sendWifiSsidValue[0], sendWifiSsidLen);
		}
		printf("SCmd:'WFID' sendWifiSsid : value = %s\n", sendWifiSsidValue);
		mWifiSsidEn = 0;
	}
	if(mSendTHMListEn == 1){
		int dataLen = 4 + mTHMListSize + 4 + mL63StatusSize + 4 + mPCDStatusSize;
		struct socket_cmd_header_struct cmdHeader;
		sprintf(cmd, "GTHM");
		set_sock_cmd_header(&cmd[0], dataLen, &cmdHeader);
		if(mw){
			send_data(&mSocket, (char*)&cmdHeader, sizeof(cmdHeader));
			send_data(&mSocket, (char*)&mTHMListSize, sizeof(mTHMListSize));
			send_data(&mSocket, &mTHMListData[0], mTHMListSize);
			send_data(&mSocket, &mL63StatusSize, mL63StatusSize);
			send_data(&mSocket, &mL63StatusData[0], mL63StatusSize);
			send_data(&mSocket, &mPCDStatusSize, mPCDStatusSize);
			send_data(&mSocket, &mPCDStatusData[0], mPCDStatusSize);
		}
		if(gw){
			send_data(&gSocket, (char*)&cmdHeader, sizeof(cmdHeader));
			send_data(&gSocket, (char*)&mTHMListSize, sizeof(mTHMListSize));
			send_data(&gSocket, &mTHMListData[0], mTHMListSize);
			send_data(&gSocket, &mL63StatusSize, mL63StatusSize);
			send_data(&gSocket, &mL63StatusData[0], mL63StatusSize);
			send_data(&gSocket, &mPCDStatusSize, mPCDStatusSize);
			send_data(&gSocket, &mPCDStatusData[0], mPCDStatusSize);
		}
		printf("SCmd:'GTHM' SendTHMList : size:%d,obj size: %d,pcd size: \n", mTHMListSize, mL63StatusSize, mPCDStatusSize);
		mSendTHMListEn = 0;
	}
	if(mCompassResultEn == 1){
		struct socket_cmd_header_struct cmdHeader;
		sprintf(cmd, "CPRS");
		set_sock_cmd_header(&cmd[0], 4, &cmdHeader);
		if(mw){
			send_data(&mSocket, (char*)&cmdHeader, sizeof(cmdHeader));
			send_data(&mSocket, (char*)&mCompassResultVal, sizeof(mCompassResultVal));
		}
		if(gw){
			send_data(&gSocket, (char*)&cmdHeader, sizeof(cmdHeader));
			send_data(&gSocket, (char*)&mCompassResultVal, sizeof(mCompassResultVal));
		}
		printf("SCmd:'CPRS' SendCompassResult :%d\n", mCompassResultVal);
		mCompassResultEn = 0;
	}
	if(mSendWBColorEn == 1){
		struct socket_cmd_header_struct cmdHeader;
		sprintf(cmd, "GWBC");
		set_sock_cmd_header(&cmd[0], 12, &cmdHeader);
		if(mw){
			send_data(&mSocket, (char*)&cmdHeader, sizeof(cmdHeader));
			send_data(&mSocket, (char*)&mSendWBRVal, sizeof(mSendWBRVal));
			send_data(&mSocket, (char*)&mSendWBGVal, sizeof(mSendWBGVal));
			send_data(&mSocket, (char*)&mSendWBBVal, sizeof(mSendWBBVal));
		}
		if(gw){
			send_data(&gSocket, (char*)&cmdHeader, sizeof(cmdHeader));
			send_data(&gSocket, (char*)&mSendWBRVal, sizeof(mSendWBRVal));
			send_data(&gSocket, (char*)&mSendWBGVal, sizeof(mSendWBGVal));
			send_data(&gSocket, (char*)&mSendWBBVal, sizeof(mSendWBBVal));
		}
		printf("SCmd:'GWBC' SendWBRGB :%d\n", mCompassResultVal);
		mSendWBColorEn = 0;
	}
	if(mSendFolderEn == 1){
		int dataLen = strlen(mSendFolderNames);
		int sizeLen = strlen(mSendFolderSizes);
		struct socket_cmd_header_struct cmdHeader;
		sprintf(cmd, "SFOD");
		set_sock_cmd_header(&cmd[0], 16+dataLen+sizeLen, &cmdHeader);
		if(mw){
			send_data(&mSocket, (char*)&cmdHeader, sizeof(cmdHeader));
			send_data(&mSocket, (char*)&mSendFolderSize, sizeof(mSendFolderSize));
			send_data(&mSocket, (char*)&dataLen, sizeof(dataLen));
			send_data(&mSocket, &mSendFolderNames[0], dataLen);
			send_data(&mSocket, (char*)&sizeLen, sizeof(sizeLen));
			send_data(&mSocket, &mSendFolderSizes[0], sizeLen);
		}
		if(gw){
			send_data(&gSocket, (char*)&cmdHeader, sizeof(cmdHeader));
			send_data(&gSocket, (char*)&mSendFolderSize, sizeof(mSendFolderSize));
			send_data(&gSocket, (char*)&dataLen, sizeof(dataLen));
			send_data(&gSocket, &mSendFolderNames[0], dataLen);
			send_data(&gSocket, (char*)&sizeLen, sizeof(sizeLen));
			send_data(&gSocket, &mSendFolderSizes[0], sizeLen);
		}
		printf("SCmd:'SFOD' mSendFolderVal :%s\n", mSendFolderSizes);
		mSendFolderEn = 0;
	}
	if(mSendHdrDefaultEn == 1){
		int x, y;
		struct socket_cmd_header_struct cmdHeader;
		sprintf(cmd, "SHRD");
		set_sock_cmd_header(&cmd[0], 48, &cmdHeader);
		if(mw){
			send_data(&mSocket, (char*)&cmdHeader, sizeof(cmdHeader));
			for(x = 0;x < 3;x++){
				for(y = 0; y < 4;y++){
					send_data(&mSocket, (char*)&hdrDefaultMode[x][y], sizeof(hdrDefaultMode[x][y]));
				}
			}
		}
		if(gw){
			send_data(&gSocket, (char*)&cmdHeader, sizeof(cmdHeader));
			for(x = 0;x < 3;x++){
				for(y = 0; y < 4;y++){
					send_data(&gSocket, (char*)&hdrDefaultMode[x][y], sizeof(hdrDefaultMode[x][y]));
				}
			}
		}
		printf("SCmd:'SHRD' SendSHRD :\n");
		mSendHdrDefaultEn = 0;
	}
	if(mSetEstimateEn == 1){
		struct socket_cmd_header_struct cmdHeader;
		sprintf(cmd, "SEST");
		set_sock_cmd_header(&cmd[0], 24, &cmdHeader);
		if(mw){
			send_data(&mSocket, (char*)&cmdHeader, sizeof(cmdHeader));
			send_data(&mSocket, (char*)&mSetEstimateStamp, sizeof(mSetEstimateStamp));
			send_data(&mSocket, (char*)&mSetEstimateTime, sizeof(mSetEstimateTime));
			send_data(&mSocket, (char*)&mCaptureEpTime, sizeof(mCaptureEpTime));
			send_data(&mSocket, (char*)&mCaptureEpStamp, sizeof(mCaptureEpStamp));
		}
		if(gw){
			send_data(&gSocket, (char*)&cmdHeader, sizeof(cmdHeader));
			send_data(&gSocket, (char*)&mSetEstimateStamp, sizeof(mSetEstimateStamp));
			send_data(&gSocket, (char*)&mSetEstimateTime, sizeof(mSetEstimateTime));
			send_data(&gSocket, (char*)&mCaptureEpTime, sizeof(mCaptureEpTime));
			send_data(&gSocket, (char*)&mCaptureEpStamp, sizeof(mCaptureEpStamp));
		}
		printf("SCmd:'SEST' mSetEstimateEn stamp/time:%lld/%d\n", mSetEstimateStamp, mSetEstimateTime);
		mSetEstimateEn = 0;
	}
	if(mSendDefectivePixelEn == 1){
		struct socket_cmd_header_struct cmdHeader;
		sprintf(cmd, "SDFP");
		set_sock_cmd_header(&cmd[0], 4, &cmdHeader);
		if(mw){
			send_data(&mSocket, (char*)&cmdHeader, sizeof(cmdHeader));
			send_data(&mSocket, (char*)&mSendDefectivePixelVal, sizeof(mSendDefectivePixelVal));
		}
		if(gw){
			send_data(&gSocket, (char*)&cmdHeader, sizeof(cmdHeader));
			send_data(&gSocket, (char*)&mSendDefectivePixelVal, sizeof(mSendDefectivePixelVal));
		}
		printf("SCmd:'SDFP' mSendDefectivePixelEn mSendDefectivePixelVal:%d\n", mSendDefectivePixelVal);
		mSendDefectivePixelEn = 0;
	}
    if(mDbtOutputDdrDataEn == 1) {
        int ret, total_size = 0, step_size = 0, size = 0;
        struct socket_cmd_header_struct sock_cmd_h;
        fpga_ddr_rw_struct ddr_p;
        getDbtDdrRWStruct(&ddr_p);
        step_size = 8192;
        total_size = ddr_p.cmd.size;
        if((total_size - mDbtOutputDdrDataOffset) < step_size)
            size = (total_size - mDbtOutputDdrDataOffset);
        else
            size = step_size;
        set_sock_cmd_header("DRDR", (size + sizeof(mDbtOutputDdrDataOffset)), &sock_cmd_h);
        if(mw){
            ret = send_data(&mSocket, (char*)&sock_cmd_h, sizeof(sock_cmd_h));
            if(ret <= 0) printf("SCmd:'DRDR' mw sock_cmd error, ret=%d\n", ret);
            ret = send_data(&mSocket, (char*)&mDbtOutputDdrDataOffset, sizeof(mDbtOutputDdrDataOffset));
            if(ret <= 0) printf("SCmd:'DRDR' mw offset error, ret=%d\n", ret);
            ret = send_data(&mSocket, (char*)&ddr_p.buf[mDbtOutputDdrDataOffset], size);
            if(ret <= 0) printf("SCmd:'DRDR' mw data error, ret=%d\n", ret);
        }
        if(gw){
            ret = send_data(&gSocket, (char*)&sock_cmd_h, sizeof(sock_cmd_h));
            if(ret <= 0) printf("SCmd:'DRDR' gw sock_cmd error, ret=%d\n", ret);
            ret = send_data(&gSocket, (char*)&mDbtOutputDdrDataOffset, sizeof(mDbtOutputDdrDataOffset));
            if(ret <= 0) printf("SCmd:'DRDR' gw offset error, ret=%d\n", ret);
            ret = send_data(&gSocket, (char*)&ddr_p.buf[mDbtOutputDdrDataOffset], size);
            if(ret <= 0) printf("SCmd:'DRDR' gw data error, ret=%d\n", ret);
        }
        printf("SCmd:'DRDR' offset=%d\n", mDbtOutputDdrDataOffset);
        mDbtOutputDdrDataOffset += size;
        if(mDbtOutputDdrDataOffset >= total_size) {
            mDbtOutputDdrDataOffset = 0;
            mDbtOutputDdrDataEn = 0;
        }
    }
    if(mDbtOutputRegDataEn == 1) {
        mDbtOutputRegDataEn = 0;
        struct socket_cmd_header_struct sock_cmd_h;
        fpga_reg_rw_struct reg_p;
        getDbtRegRWStruct(&reg_p);
        set_sock_cmd_header("DRRG", sizeof(reg_p.data), &sock_cmd_h);
        if(mw){
			send_data(&mSocket, (char*)&sock_cmd_h, sizeof(sock_cmd_h));
			send_data(&mSocket, (char*)&reg_p.data, sizeof(reg_p.data));
		}
		if(gw){
			send_data(&gSocket, (char*)&sock_cmd_h, sizeof(sock_cmd_h));
			send_data(&gSocket, (char*)&reg_p.data, sizeof(reg_p.data));
		}
        printf("SCmd:'DRRG'\n");
    }
    if(mDbtInputDdrDataFinish == 1) {
        mDbtInputDdrDataFinish = 0;
        struct socket_cmd_header_struct sock_cmd_h;
        int val = 1;
        set_sock_cmd_header("DWDF", sizeof(val), &sock_cmd_h);
        if(mw){
			send_data(&mSocket, (char*)&sock_cmd_h, sizeof(sock_cmd_h));
			send_data(&mSocket, (char*)&val, sizeof(val));
		}
		if(gw){
			send_data(&gSocket, (char*)&sock_cmd_h, sizeof(sock_cmd_h));
			send_data(&gSocket, (char*)&val, sizeof(val));
		}
        printf("SCmd:'DWDF'\n");
    }
    if(mDbtInputRegDataFinish == 1) {
        mDbtInputRegDataFinish = 0;
        struct socket_cmd_header_struct sock_cmd_h;
        int val = 1;
        set_sock_cmd_header("DWRF", sizeof(val), &sock_cmd_h);
        if(mw){
			send_data(&mSocket, (char*)&sock_cmd_h, sizeof(sock_cmd_h));
			send_data(&mSocket, (char*)&val, sizeof(val));
		}
		if(gw){
			send_data(&gSocket, (char*)&sock_cmd_h, sizeof(sock_cmd_h));
			send_data(&gSocket, (char*)&val, sizeof(val));
		}
        printf("SCmd:'DWRF'\n");
    }
//	doType = 1;
//	if(mLive){
//		mDos.flush();
//	}
//	doType = 2;
//	if(gLive){
//		gDos.flush();
//	}	
}

/**
 * Listener Socket Read/Write
 */
void cmd_thread(void) {
	int mLen=0, gLen=0, qLen=0;
	int mR=0, mW=0, gR=0, gW=0, qR=0, qW=0;
	static unsigned long long curTime, lstTime=0, runTime;
	
//	int mLive=0, gLive=0, qLive=0;
//	int connectID2 = connectID;
//	int disconnect_flag = 0;
	unsigned long long qLstTimeConnect = 0;
	int qSocketOst = 0;
	//int doType = 0;
	
	fromWhereConnect = 0;
	mThreadCount++;
	sendMacAddressEn = 1;
	
	while(cmd_thread_en) {
		get_current_usec(&curTime);
        if(curTime < lstTime) lstTime = curTime;
        else if((curTime - lstTime) >= 3000000){
            //printf("cmd_thread() runTime=%lld\n", runTime);
            lstTime = curTime;
        }
		
/*		if(connectID2 != connectID || disconnect_flag == 1) {
			printf("cmd_thread() connectID2=%d connectID=%d disconnect_flag=%d\n", connectID2, connectID, disconnect_flag);
			disconnect_flag = 0;
					
			mChangeDebugToolStateEn = 1;
	    	isDebugToolConnect = 1;
	    	printf("cmd_thread() mChangeDebugToolStateEn = %d , isDebugToolConnect = %d\n", isDebugToolConnect, isDebugToolConnect);
	    	
			break;
		}
		
		// rex+ 151111, 判斷wifi timeout
		if(lstTimeConnect != 0){
			unsigned long long nowTimeConnect;
			get_current_usec(&nowTimeConnect);
			if(nowTimeConnect < lstTimeConnect)	lstTimeConnect = nowTimeConnect;
			if(nowTimeConnect - lstTimeConnect > 5000000){
				printf("TimeOut!\n");
				printf("!!!!!now = %lld\n", nowTimeConnect);
				printf("!!!!!lst = %lld\n", lstTimeConnect);
				printf("!!!!!!!t = %lld\n", (nowTimeConnect-lstTimeConnect));
//				Main.systemlog.addLog("WifiServer", System.currentTimeMillis(), "User", "TimeOut", "---");
//				initCtrlFlag();
				//lstTimeConnect = 0;
//                setWifiOledEn(0);
			}
		}*/
//		mLive = 0; gLive = 0; qLive = 0;
		mR = 0;  mW = 0; 
		gR = 0;  gW = 0; 
		qR = 0;  qW = 0;
		if(mSocket != -1 /*&& in != null && os != null && !mSocket.isClosed()*/){
//			mLive = 1;
			check_sock_rw_state(mSocket, 1, 1, &mR, &mW);
		}
		if(gSocket != -1 /*&& gIn != null && gOs != null && !gSocket.isClosed()*/){
//			gLive = 1;
			check_sock_rw_state(gSocket, 1, 1, &gR, &gW);
		}
		if(qSocket != -1 /*&& qIn != null && qOs != null && !qSocket.isClosed()*/){
//			qLive = 1;
			check_sock_rw_state(qSocket, 1, 1, &qR, &qW);
		}
		/*if(!mLive && !gLive && !qLive){
			printf("cmd_thread() live check m/g/q: %d/%d/%d\n", mLive, gLive, qLive);
			break;
		}*/
		//doType = 0;
		
        if(mW == 1 || gW == 1)
            process_output_stream(mW, gW);

		//doType = 1;
		if(mR == 1) {	//if(mLive) {
			mLen = read_data(&mSocket, &mInputData[0], 0x10000);          
			if(mLen > 0){
				get_current_usec(&lstTimeConnect);
				if(mSocketOst < 0) mSocketOst = 0;
				if(mSocketOst + mLen < SOCKET_DATA_Q_MAX){
					memcpy(&mSocketDataQ[mSocketOst], &mInputData[0], mLen);
					mSocketOst += mLen;
				}
				else{
					printf("cmd_thread() socketOst over size! mSocketOst=%d\n", mSocketOst);
					mSocketOst = 0;
				}
			}
				
			int appMode = process_input_stream(0);
			if(appMode == 1){
				gSocket = mSocket;
				close_sock(&mSocket);
				memcpy(&mGpsHostName[0], &mHostName[0], sizeof(mGpsHostName));
				memset(&mHostName[0], 0, sizeof(mHostName));	//mHostName = "";
				printf("cmd_thread() get GpsHostName: %s\n", mGpsHostName);
			}
		}
		//doType = 2;
		if(gR == 1) {	//if(gLive) {
			gLen = read_data(&gSocket, &gInputData[0], 0x10000);
			if(gLen > 0){
				get_current_usec(&lstTimeConnect);
				if(gSocketOst < 0) gSocketOst = 0;
				if(gSocketOst + gLen < SOCKET_DATA_Q_MAX){
					memcpy(&gSocketDataQ[gSocketOst], &gInputData[0], gLen);
					gSocketOst += gLen;
				}
				else{
					printf("cmd_thread() gSocketOst over size! gSocketOst=%d\n", gSocketOst);
					gSocketOst = 0;
				}
				process_input_stream(1);
			}
		}
		//doType = 3;
		if(qR == 1) {	//if(qLive){
			qLen = read_data(&qSocket, &qInputData[0], 0x10000);
			if(qLen > 0) {
				get_current_usec(&qLstTimeConnect);
				if(qSocketOst < 0) qSocketOst = 0;
				if(qSocketOst + qLen < SOCKET_DATA_Q_MAX){
					memcpy(&qSocketDataQ[qSocketOst], &qInputData[0], gLen);
					qSocketOst += qLen;
				}
				else{
					printf("cmd_thread() qSocketOst over size! qSocketOst=%d\n", qSocketOst);
					qSocketOst = 0;
				}

				qSock_process_input_stream(qW);
			}
		}
		
		//if(mSocket != -1) check_client_sock_connect(&mSocket);
		//if(gSocket != -1) check_client_sock_connect(&gSocket);
		//if(qSocket != -1) check_client_sock_connect(&qSocket);
		
		get_current_usec(&runTime);
        runTime -= curTime;
        if(runTime < 10000){
            usleep(10000 - runTime);
            get_current_usec(&runTime);
            runTime -= curTime;
        }
	}
	mThreadCount--;
	printf("cmd_thread() process thread end (en=%d)\n", cmd_thread_en);
}

/**
 * Rtsp Worker
 */
void rtsp_thread(void) {
	static unsigned long long curTime, lstTime=0, runTime;
	
	// Read Mac Files
	readMacFiles();
		
	if(originalRtspTimer == 0)
		get_current_usec(&originalRtspTimer);
		
	while(rtsp_thread_en) {
		get_current_usec(&curTime);
        if(curTime < lstTime) lstTime = curTime;
        else if((curTime - lstTime) >= 3000000){
            //printf("rtsp_thread() runTime=%lld\n", runTime);
            lstTime = curTime;
        }
		
/*		if(Main.rtmp_switch == 0 && (curTime-originalRtspTimer) >= 1000000){
			if(wifiSerCB != null) wifiSerCB.copyRTSP();
			if(rtspBufLength > 0){
				if(rtspWidth != SrsEncoder.vOutWidth || rtspHeight != SrsEncoder.vOutHeight){
					Main.outWidth = SrsEncoder.vOutWidth = rtspWidth;
					Main.outHeight = SrsEncoder.vOutHeight = rtspHeight;
					if(Main.mPublisher != null){
						mRTMPSwitchEn = 1;
					}
				}
				
				int h_len = 50;
				if(rtspBufLength < 50)
					h_len = rtspBufLength;
				char h264_header[h_len];
				memcpy(&h264_header[0], &rtspBuffer[1000], h_len);
				int h264_len[2];
				getSPSPPSLen(&h264_header[0], sizeof(h264_header), &h264_len[0]);
				if(h264_len[1] > 0){
					if(SrsEncoder.h264_length == 0){
						int tmp_flags = rtspBuffer[1000+h264_len[1]+4] & 0x1f;
						SrsEncoder.h264_flags = tmp_flags == 5?1:0;
						SrsEncoder.h264_header_len = h264_len;
						SrsEncoder.h264_header = new byte[h264_len[1]];
						memcpy(&SrsEncoder.h264_header[0], &rtspBuffer[1000], h264_len[1]);
						SrsEncoder.h264_data = new byte[rtspBufLength-(1000+h264_len[1])];
						memcpy(&SrsEncoder.h264_data[0], &rtspBuffer[1000+h264_len[1]], rtspBufLength-(1000+h264_len[1]));
						SrsEncoder.h264_length = rtspBufLength-(1000+h264_len[1]);
						
						if(Main.mPublisher != null){
							Main.mPublisher.runOnGetYuvFrame();
						}
					}
				}
			}
		}*/
		
		get_current_usec(&runTime);
        runTime -= curTime;
        if(runTime < 30000){
            usleep(30000 - runTime);
            get_current_usec(&runTime);
            runTime -= curTime;
        }
	}

	printf("rtsp_thread() process thread end (en=%d)\n", rtsp_thread_en);
}

/**
 * Web Service
 */
void webservice_thread(void) {
	static unsigned long long curTime, lstTime=0, runTime;
	
	while(webservice_thread_en) {
		get_current_usec(&curTime);
        if(curTime < lstTime) lstTime = curTime;
        else if((curTime - lstTime) >= 3000000){
            //printf("webservice_thread() runTime=%lld\n", runTime);
            lstTime = curTime;
        }
		
/*		if(Main.webServiceDataLen > 0 && ControllerServer.isServerProcessing == 0){
			int len = Main.webServiceDataLen;
			if(len > 0){
				if(gSocketOst < 0) gSocketOst = 0;
				if(gSocketOst + len < 0x10000){
					memcpy(&gSocketDataQ[gSocketOst], &Main.webServiceData[0], len);
					gSocketOst += len;
					Main.webServiceDataLen = 0;
					printf("webservice_thread() get webservice len: %d\n", gSocketOst);
					process_input_stream(1);
				}
				else{
					printf("webservice_thread() WifiServer: Err! socketOst=%d\n", gSocketOst);
					gSocketOst = 0;
				}
			}
		}*/
		
		get_current_usec(&runTime);
        runTime -= curTime;
        if(runTime < 20000){
            usleep(20000 - runTime);
            get_current_usec(&runTime);
            runTime -= curTime;
        }
	}

	printf("webservice_thread() process thread end (en=%d)\n", webservice_thread_en);
}

// create thread
void create_server_thread(void) {
	// nSock_thread
	pthread_mutex_init(&mut_nSock_buf, NULL);
    if(pthread_create(&thread_nSock_id, NULL, (void *)nSock_thread, NULL) != 0) {
        printf("Create nSock_thread fail !\n");
    }
	
	// cmd_thread
	pthread_mutex_init(&mut_cmd_buf, NULL);
    if(pthread_create(&thread_cmd_id, NULL, (void *)cmd_thread, NULL) != 0) {
        printf("Create cmd_thread fail !\n");
    }
	
	// rtsp_thread
	pthread_mutex_init(&mut_rtsp_buf, NULL);
    if(pthread_create(&thread_rtsp_id, NULL, (void *)rtsp_thread, NULL) != 0) {
        printf("Create rtsp_thread fail !\n");
    }
	
	// webservice_thread
	pthread_mutex_init(&mut_webservice_buf, NULL);
    if(pthread_create(&thread_webservice_id, NULL, (void *)webservice_thread, NULL) != 0) {
        printf("Create webservice_thread fail !\n");
    }
}

int start_wifi_server(int port) {
	SERVERPORT = port;
	if (server_socket_init(SERVERPORT) == 0) {
		sock_cmd_sta_initVal();
		create_server_thread();
	}
	else {
		printf("start_wifi_server() server socket error!\n");
		return -1;
	}
	return 0;
}

int malloc_mOutputData() {
	//char mOutputData[JAVA_UVC_BUF_MAX];
	mOutputData = (unsigned char *)malloc(sizeof(unsigned char) * UVC_BUF_MAX);
	if(mOutputData == NULL) goto error;
	return 0;
error:
	db_error("malloc_mOutputData() malloc error!");
	return -1;
}

void free_mOutputData() {
	if(mOutputData != NULL)
		free(mOutputData);
	mOutputData = NULL;
}

int malloc_mImgData() {
	//char mImgData[JAVA_UVC_BUF_MAX];
	mImgData = (unsigned char *)malloc(sizeof(unsigned char) * UVC_BUF_MAX);
	if(mImgData == NULL) goto error;
	return 0;
error:
	db_error("malloc_mImgData() malloc error!");
	return -1;
}

void free_mImgData() {
	if(mImgData != NULL)
		free(mImgData);
	mImgData = NULL;
}

int malloc_mDownloadData() {
	//char mDownloadData[JAVA_UVC_BUF_MAX];
	mDownloadData = (unsigned char *)malloc(sizeof(unsigned char) * UVC_BUF_MAX);
	if(mDownloadData == NULL) goto error;
	return 0;
error:
	db_error("malloc_mDownloadData() malloc error!");
	return -1;
}

void free_mDownloadData() {
	if(mDownloadData != NULL)
		free(mDownloadData);
	mDownloadData = NULL;
}

int malloc_bottomData() {
	//char bottomData[JAVA_UVC_BUF_MAX];
	bottomData = (unsigned char *)malloc(sizeof(unsigned char) * UVC_BUF_MAX);
	if(bottomData == NULL) goto error;
	return 0;
error:
	db_error("malloc_bottomData() malloc error!");
	return -1;
}

void free_bottomData() {
	if(bottomData != NULL)
		free(bottomData);
	bottomData = NULL;
}

int malloc_mBotmData() {
	//char mBotmData[JAVA_UVC_BUF_MAX];
	mBotmData = (unsigned char *)malloc(sizeof(unsigned char) * UVC_BUF_MAX);
	if(mBotmData == NULL) goto error;
	return 0;
error:
	db_error("malloc_mBotmData() malloc error!");
	return -1;
}

void free_mBotmData() {
	if(mBotmData != NULL)
		free(mBotmData);
	mBotmData = NULL;
}

int malloc_mSocketDataQ() {
	//char mSocketDataQ[JAVA_UVC_BUF_MAX];
	mSocketDataQ = (unsigned char *)malloc(sizeof(unsigned char) * SOCKET_DATA_Q_MAX);
	if(mSocketDataQ == NULL) goto error;
	return 0;
error:
	db_error("malloc_mSocketDataQ() malloc error!");
	return -1;
}

void free_mSocketDataQ() {
	if(mSocketDataQ != NULL)
		free(mSocketDataQ);
	mSocketDataQ = NULL;
}

int malloc_sendImgBuf() {
	//char sendImgBuf[JAVA_UVC_BUF_MAX];
	sendImgBuf = (unsigned char *)malloc(sizeof(unsigned char) * UVC_BUF_MAX);
	if(sendImgBuf == NULL) goto error;
	return 0;
error:
	db_error("malloc_sendImgBuf() malloc error!");
	return -1;
}

void free_sendImgBuf() {
	if(sendImgBuf != NULL)
		free(sendImgBuf);
	sendImgBuf = NULL;
}

int malloc_mInputData() {
	//char mInputData[0x10000];
	mInputData = (unsigned char *)malloc(sizeof(unsigned char) * 0x10000);
	if(mInputData == NULL) goto error;
	return 0;
error:
	db_error("malloc_mInputData() malloc error!");
	return -1;
}

void free_mInputData() {
	if(mInputData != NULL)
		free(mInputData);
	mInputData = NULL;
}

int malloc_gSocketDataQ() {
	//char gSocketDataQ[0x10000];
	gSocketDataQ = (unsigned char *)malloc(sizeof(unsigned char) * SOCKET_DATA_Q_MAX);
	if(gSocketDataQ == NULL) goto error;
	return 0;
error:
	db_error("malloc_gSocketDataQ() malloc error!");
	return -1;
}

void free_gSocketDataQ() {
	if(gSocketDataQ != NULL)
		free(gSocketDataQ);
	gSocketDataQ = NULL;
}

int malloc_gInputData() {
	//char gInputData[0x10000];
	gInputData = (unsigned char *)malloc(sizeof(unsigned char) * 0x10000);
	if(gInputData == NULL) goto error;
	return 0;
error:
	db_error("malloc_gInputData() malloc error!");
	return -1;
}

void free_gInputData() {
	if(gInputData != NULL)
		free(gInputData);
	gInputData = NULL;
}

int malloc_rtspBuffer() {
	//char rtspBuffer[0x100000];
	rtspBuffer = (unsigned char *)malloc(sizeof(unsigned char) * 0x100000);
	if(rtspBuffer == NULL) goto error;
	return 0;
error:
	db_error("malloc_rtspBuffer() malloc error!");
	return -1;
}

void free_rtspBuffer() {
	if(rtspBuffer != NULL)
		free(rtspBuffer);
	rtspBuffer = NULL;
}

int malloc_mTHMListData() {
	//char mTHMListData[0x20000];
	mTHMListData = (unsigned char *)malloc(sizeof(unsigned char) * 0x20000);
	if(mTHMListData == NULL) goto error;
	return 0;
error:
	db_error("malloc_mTHMListData() malloc error!");
	return -1;
}

void free_mTHMListData() {
	if(mTHMListData != NULL)
		free(mTHMListData);
	mTHMListData = NULL;
}

int malloc_mL63StatusData() {
	//char mL63StatusData[0x20000];
	mL63StatusData = (unsigned char *)malloc(sizeof(unsigned char) * 0x20000);
	if(mL63StatusData == NULL) goto error;
	return 0;
error:
	db_error("malloc_mL63StatusData() malloc error!");
	return -1;
}

void free_mL63StatusData() {
	if(mL63StatusData != NULL)
		free(mL63StatusData);
	mL63StatusData = NULL;
}

int malloc_mPCDStatusData() {
	//char mPCDStatusData[0x20000];
	mPCDStatusData = (unsigned char *)malloc(sizeof(unsigned char) * 0x20000);
	if(mPCDStatusData == NULL) goto error;
	return 0;
error:
	db_error("malloc_mPCDStatusData() malloc error!");
	return -1;
}

void free_mPCDStatusData() {
	if(mPCDStatusData != NULL)
		free(mPCDStatusData);
	mPCDStatusData = NULL;
}

int malloc_mSendFolderNames() {
	//char mSendFolderNames[0x20000];
	mSendFolderNames = (unsigned char *)malloc(sizeof(unsigned char) * 0x20000);
	if(mSendFolderNames == NULL) goto error;
	return 0;
error:
	db_error("malloc_mSendFolderNames() malloc error!");
	return -1;
}

void free_mSendFolderNames() {
	if(mSendFolderNames != NULL)
		free(mSendFolderNames);
	mSendFolderNames = NULL;
}

int malloc_mSendFolderSizes() {
	//char mSendFolderSizes[0x20000];
	mSendFolderSizes = (unsigned char *)malloc(sizeof(unsigned char) * 0x20000);
	if(mSendFolderSizes == NULL) goto error;
	return 0;
error:
	db_error("malloc_mSendFolderSizes() malloc error!");
	return -1;
}

void free_mSendFolderSizes() {
	if(mSendFolderSizes != NULL)
		free(mSendFolderSizes);
	mSendFolderSizes = NULL;
}

int malloc_qSocketDataQ() {
	//char qSocketDataQ[0x10000];
	qSocketDataQ = (unsigned char *)malloc(sizeof(unsigned char) * SOCKET_DATA_Q_MAX);
	if(qSocketDataQ == NULL) goto error;
	return 0;
error:
	db_error("malloc_qSocketDataQ() malloc error!");
	return -1;
}

void free_qSocketDataQ() {
	if(qSocketDataQ != NULL)
		free(qSocketDataQ);
	qSocketDataQ = NULL;
}

int malloc_qInputData() {
	//char qInputData[0x10000];
	qInputData = (unsigned char *)malloc(sizeof(unsigned char) * 0x10000);
	if(qInputData == NULL) goto error;
	return 0;
error:
	db_error("malloc_qInputData() malloc error!");
	return -1;
}

void free_qInputData() {
	if(qInputData != NULL)
		free(qInputData);
	qInputData = NULL;
}

int malloc_inputStreamQ() {
	inputStreamQ = (unsigned char *)malloc(sizeof(unsigned char) * SOCKET_DATA_Q_MAX);
	if(inputStreamQ == NULL) goto error;
	return 0;
error:
	db_error("malloc_inputStreamQ() malloc error!");
	return -1;
}

void free_inputStreamQ() {
	if(inputStreamQ != NULL)
		free(inputStreamQ);
	inputStreamQ = NULL;
}


void free_wifiserver_buf() {
	free_mOutputData();
	free_mImgData();
	free_mDownloadData();
	free_bottomData();
	free_mBotmData();
	free_mSocketDataQ();
	free_sendImgBuf();
	
	free_mInputData();
	free_gSocketDataQ();
	free_gInputData();
	free_rtspBuffer();
	free_mTHMListData();
	free_mL63StatusData();
	free_mPCDStatusData();
	free_mSendFolderNames();
	free_mSendFolderSizes();
	free_qSocketDataQ();
	free_qInputData();
	
	free_inputStreamQ();
}

/*
 * 直接宣告一塊大記憶體, 程式會跳掉,
 * 所以改為動態配置記憶體
 */
int malloc_wifiserver_buf() {
	if(malloc_mOutputData() < 0) 	  goto error;
	if(malloc_mImgData() < 0) 		  goto error;
	if(malloc_mDownloadData() < 0) 	  goto error;
	if(malloc_bottomData() < 0)		  goto error;
	if(malloc_mBotmData() < 0)		  goto error;
	if(malloc_mSocketDataQ() < 0)	  goto error;
	if(malloc_sendImgBuf() < 0)		  goto error;
	
	if(malloc_mInputData() < 0)		  goto error;
	if(malloc_gSocketDataQ() < 0)	  goto error;
	if(malloc_gInputData() < 0)		  goto error;
	if(malloc_rtspBuffer() < 0)		  goto error;
	if(malloc_mTHMListData() < 0)	  goto error;
	if(malloc_mL63StatusData() < 0)	  goto error;
	if(malloc_mPCDStatusData() < 0)	  goto error;
	if(malloc_mSendFolderNames() < 0) goto error;
	if(malloc_mSendFolderSizes() < 0) goto error;
	if(malloc_qSocketDataQ() < 0)	  goto error;
	if(malloc_qInputData() < 0)		  goto error;
	
	if(malloc_inputStreamQ() < 0)	  goto error;

	return 0;
error:
	db_error("malloc_wifiserver_buf() malloc error!");
	free_wifiserver_buf();
	return -1;
}

void setDbtDdrCmdEn(int en) { mDbtDdrCmdEn = en; }
int getDbtDdrCmdEn() { return mDbtDdrCmdEn; }

void setDbtInputDdrDataEn(int en) { mDbtInputDdrDataEn = en; }
int getDbtInputDdrDataEn() { return mDbtInputDdrDataEn; }

void setDbtInputDdrDataFinish(int flag) { mDbtInputDdrDataFinish = flag; }
int getDbtInputDdrDataFinish() { return mDbtInputDdrDataFinish; }

void setDbtOutputDdrDataEn(int en) { mDbtOutputDdrDataEn = en; }
int getDbtOutputDdrDataEn() { return mDbtOutputDdrDataEn; }

void setDbtRegCmdEn(int en) { mDbtRegCmdEn = en; }
int getDbtRegCmdEn() { return mDbtRegCmdEn; }

void setDbtInputRegDataFinish(int flag) { mDbtInputRegDataFinish = flag; }
int getDbtInputRegDataFinish() { return mDbtInputRegDataFinish; }

void setDbtOutputRegDataEn(int en) { mDbtOutputRegDataEn = en; }
int getDbtOutputRegDataEn() { return mDbtOutputRegDataEn; }