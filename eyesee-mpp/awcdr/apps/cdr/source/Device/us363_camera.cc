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

//==================== parameter ====================
char mUS363Version[64] = "v1.00.00\0";

char mWifiApSsid[32] = "US_0000\0"; 
char mWifiApPassword[16] = "88888888\0";

int mCountryCode = 840;		//國家代碼 對應ISO 3166-1  158台灣 392日本 156中國 840美國 
int mCustomerCode = 0;		//辨識編號 0:Ultracker	10137:Let's   2067001:阿里巴巴
char mPcbVersion[8] = "V0.0\0";

char mHttpAccount[32] = "admin\0";
char mHttpPassword[32] = "admin\0";

char mEthernetIP[64];
char mEthernetMask[64];
char mEthernetGway[64];
char mEthernetDns[64];

int mCtrlCameraPositionMode = 1;    // 0:手動 1:自動(G Sensor)
int mCameraPositionMode = 0;        // 0:0 1:180 2:90
int mCameraPositionModelst = 0;
int mCameraPositionModeChange = 0;

int mResolutionMode = 7;
int mResolutionMode_lst = 7;


//==================== variable ====================
#define AUDIO_REC_EMPTY_BYTES_SIZE   1024
char audioRecEmptyBytes[AUDIO_REC_EMPTY_BYTES_SIZE];

int doResize_mode[8];
int writeUS360DataBin_flag = 0;

//==================== get/set =====================
void getUS363Version(char *version) {
    sprintf(version, "%s\0", mUS363Version);
}

void setCtrlCameraPositionMode(int mode) {
	mCtrlCameraPositionMode = mode;
}
int getCtrlCameraPositionMode(void) {
	return mCtrlCameraPositionMode;
}

int setCameraPositionMode(int mode) {
	mCameraPositionMode = mode;
    if(mCameraPositionMode != mCameraPositionModelst) {
        mCameraPositionModelst = mCameraPositionMode;
        mCameraPositionModeChange = 1;
    }
    else {
        mCameraPositionModeChange = 0;
    }
    return mCameraPositionModeChange;
}
int getCameraPositionMode(void) {
	return mCameraPositionMode;
}

int getCameraPositionModeChange(void) {
	return mCameraPositionModeChange;
}



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
		sprintf(BOTTOM_FILE_NAME_SOURCE, "background_lets\0");
//tmp    	ControllerServer.machineNmae = "LET'S";
    	setModelName(&name[0]);
    	writeWifiMaxLink(10);
    	break;
    case CUSTOMER_CODE_ALIBABA:
		sprintf(name, "Alibaba\0");
		sprintf(BOTTOM_FILE_NAME_SOURCE, "background_alibaba\0");
//tmp    	ControllerServer.machineNmae = "Alibaba";
    	setModelName(&name[0]);
    	writeWifiMaxLink(10);
    	setSensorLogEnable(1);
    	break;
    case CUSTOMER_CODE_PIIQ:
		sprintf(name, "peek\0");
		sprintf(BOTTOM_FILE_NAME_SOURCE, "background_peek\0");
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
        writeUS360DataBin_flag = 1;
    }
	
    //TagDemoMode = 		1;
    char readUS363Ver[16];
    Get_DataBin_US360Version(&readUS363Ver[0], sizeof(readUS363Ver) );
    if(strcmp(readUS363Ver, mUS363Version) != 0) {
        Set_DataBin_US360Version(&mUS363Version[0]);
        writeUS360DataBin_flag = 1;
    }
	
	//TagCamPositionMode = 	2;
    switch(Get_DataBin_CamPositionMode() ) {
    case 0:  setCtrlCameraPositionMode(1); break;
    case 1:  setCtrlCameraPositionMode(0); setCameraPositionMode(0); break;
    case 2:  setCtrlCameraPositionMode(0); setCameraPositionMode(1); break;
    case 3:  setCtrlCameraPositionMode(0); setCameraPositionMode(2); break;
    default: setCtrlCameraPositionMode(0); setCameraPositionMode(0); break;
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
    mWB_Mode = Get_DataBin_WBMode();
    setWBMode(mWB_Mode);
	
	//TagCaptureMode = 		11;
    CaptureMode = Get_DataBin_CaptureMode();
	
	//TagCaptureCnt = 		12;
    CaptureCnt = Get_DataBin_CaptureCnt();
	
	//TagCaptureSpaceTime = 13;
    CaptureSpaceTime = Get_DataBin_CaptureSpaceTime();
	
	//TagSelfTimer = 		14;
    SelfTimer = Get_DataBin_SelfTimer();
	
	//TagTimeLapseMode = 	15;
    Time_Lapse_Mode = Get_DataBin_TimeLapseMode();
	
	//TagSaveToSel = 		16;
	//TagWifiDisableTime = 	17;
    wifiDisableTime = Get_DataBin_WifiDisableTime();
	
	//TagEthernetMode = 	18;
	//TagEthernetIP = 		19;
	//TagEthernetMask = 	20;
	//TagEthernetGateWay = 	21;
	//TagEthernetDNS = 		22;
	//TagMediaPort = 		23;
    Set_DataBin_MediaPort(Get_DataBin_MediaPort() ); //檢測自己數值是否正常
	
	//TagDrivingRecord = 	24;
    if(customer == CUSTOMER_CODE_PIIQ)
    	DrivingRecord_Mode = 0;	//databin.getDrivingRecord();
    else
    	DrivingRecord_Mode = 1;	//Get_DataBin_DrivingRecord();
	
	//TagUS360Versin = 		25;
	//TagWifiChannel = 		26;
    WifiChannel = Get_DataBin_WifiChannel();
    WriteWifiChannel(WifiChannel);
	
	//TagExposureFreq = 	27;
    int freq = Get_DataBin_ExposureFreq();
    SetAEGEPFreq(freq);
	
	//TagFanControl = 		28;
	int ctrl = Get_DataBin_FanControl();
	SetFanCtrl(ctrl);
	
	//TagSharpness = 		29;
    setStrengthWifiCmd(Get_DataBin_Sharpness() );
	
	//TagUserCtrl30Fps = 	30;
    //User_Ctrl = Get_DataBin_UserCtrl30Fps();
	
	//TagCameraMode = 		31;
    SetCameraMode(Get_DataBin_CameraMode() );

	//TagColorSTMode = 		32;
    SetColorSTSW(1);		//SetColorSTSW(Get_DataBin_ColorSTMode() );
	
	//TagAutoGlobalPhiAdjMode = 33;
    doAutoGlobalPhiAdjMode = Get_DataBin_AutoGlobalPhiAdjMode();
    setSmoothParameter(3, doAutoGlobalPhiAdjMode);
	
	//TagHDMITextVisibility = 34;
    HDMITextVisibilityMode = Get_DataBin_HDMITextVisibility();
	
	//TagSpeakerMode = 		35;
    speakerMode = Get_DataBin_SpeakerMode();
	
	//TagLedBrightness = 	36;
//tmp    SetLedBrightness(Get_DataBin_LedBrightness() );

	//TagOledControl = 		37;
//tmp    setOledControl(Get_DataBin_OledControl() );

	//TagDelayValue = 		38;
	//TagImageQuality = 	39;
    JPEGQualityMode = Get_DataBin_ImageQuality();
    set_A2K_JPEG_Quality_Mode(JPEGQualityMode);
	
	//TagPhotographReso = 	40;
	//TagRecordReso = 		41;
	//TagTimeLapseReso = 	42;
	//TagTranslucent = 		43;
    Translucent_Mode = 1;			//Translucent_Mode = Get_DataBin_Translucent();
    SetTransparentEn(Translucent_Mode);
	
	//TagCompassMaxx = 		44;
	//TagCompassMaxy = 		45;
	//TagCompassMaxz = 		46;
	//TagCompassMinx = 		47;
	//TagCompassMiny = 		48;
	//TagCompassMinz = 		49;
	//TagDebugLogMode = 	50;
    DebugLog_Mode = Get_DataBin_DebugLogMode();
	
	//TagBottomMode = 		51;
    mBottomMode = Get_DataBin_BottomMode();
	
	//TagBottomSize = 		52;
    mBottomSize = Get_DataBin_BottomSize();
//tmp    SetBottomValue(mBottomMode, mBottomSize);

	//TagHdrEvMode = 		53;
    int wdr_mode=1;
    int hdr_mode = Get_DataBin_hdrEvMode();
    HdrEvMode = hdr_mode;
    setSensorHdrLevel(hdr_mode);
	if(hdr_mode == 2)      wdr_mode = 2;	//弱
	else if(hdr_mode == 4) wdr_mode = 1;	//中
	else if(hdr_mode == 8) wdr_mode = 0;	//強
    SetWDRLiveStrength(wdr_mode);
	
    //TagBitrate = 			54;
//tmp    SetBitrateMode(Get_DataBin_Bitrate() );

    //TagHttpAccount =      55;
	//HttpAccount = new byte[32];				// 網頁/RTSP帳號
	//TagHttpPassword =     56;
	//HttpPassword = new byte[32];			// 網頁/RTSP密碼
	//TagHttpPort =         57;
    Set_DataBin_HttpPort(Get_DataBin_HttpPort()); //檢測自己數值是否正常
	
    //TagCompassMode  = 	58;
//tmp    setbmm050_enable(Get_DataBin_CompassMode() );

	//TagGsensorMode  = 	59;
//tmp    setBma2x2_enable(Get_DataBin_GsensorMode() );

	//TagCapHdrMode =		60;
	//TagBottomTMode =		61;
	mBottomTextMode = Get_DataBin_BottomTMode();
//tmp	SetBottomTextMode(mBottomTextMode);

	//TagBottomTColor =		62;
	//TagBottomBColor =		63;
	//TagBottomTFont =		64;
	//TagBottomTLoop =		65;
	//TagBottomText =		66;
	//TagFpgaEncodeType =	67;
	mFPGA_Encode_Type = Get_DataBin_FpgaEncodeType();

	//TagWbRGB =			68;
	//TagContrast =			69;
	int contrast = Get_DataBin_Contrast();
	setContrast(contrast);
	
	//TagSaturation =		70;
	int sv = Get_DataBin_Saturation();
	Saturation = GetSaturationValue(sv);
	
	//TagFreeCount
	getSDFreeSize(&sd_freesize);
//tmp    getRECTimes(sd_freesize);
    freeCount = Get_DataBin_FreeCount();
    if(freeCount == -1){
//tmp    	Set_DataBin_FreeCount(GetSpacePhotoNum() );
    	freeCount = Get_DataBin_FreeCount();
    	writeUS360DataBin_flag = 1;
    }else{
//tmp    	int de = freeCount - GetSpacePhotoNum();
		int de = freeCount;
    	if(de > 100 || de < -100){
//tmp    		freeCount = GetSpacePhotoNum();
    	}else if(de > 10){
    		freeCount = freeCount + 10;
    	}else if(de > 0){
    		freeCount = freeCount + de;
    	}else if(de < -10){
    		freeCount = freeCount - 10;
    	}else if(de < -9){
    		freeCount = freeCount - de;
    	}
    	Set_DataBin_FreeCount(freeCount);
    	writeUS360DataBin_flag = 1;
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
    mLiveQualityMode = Get_DataBin_LiveQualityMode();
    set_A2K_JPEG_Live_Quality_Mode(mLiveQualityMode);
	
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
    Power_Saving_Mode = Get_DataBin_PowerSaving();
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
//tmp    readMcuUpdate();
        
    TestToolCmdInit();
    memset(&audioRecEmptyBytes[0], 0, sizeof(audioRecEmptyBytes));
    for(i = 0; i< 8; i++) 
		doResize_mode[i] = -1;
        
//tmp    setVersionOLED(mUS363Version.getBytes());

    setCpuFreq(4, CpuFullSpeed);
        
    unsigned long long defaultSysTime = 1420041600000L;                     // 2015/01/01 00:00:00
    unsigned long long nowSysTime;
	get_current_usec(&nowSysTime);
    if(defaultSysTime > nowSysTime){										// lester+ 180207
		setSysTime(defaultSysTime);
    }
//tmp    writeHistory();
	
    ret = ReadTestResult();
    if(ret != 0) 
		SetDoAutoStitchFlag(1);

    readWifiConfig(&mWifiApSsid[0], &mWifiApPassword[0]);
    //CreatCountryList();
    initCountryFunc();
    initCustomerFunc();
        
    databinInit(LangCode, customerCode);
        
//tmp    ControllerServer.changeDataToDataBin();
    Get_DataBin_HttpAccount(&mHttpAccount[0], sizeof(mHttpAccount) );
    Get_DataBin_HttpPassword(&mHttpPassword[0], sizeof(mHttpPassword) );	
//tmp	Live555Thread(httpAccount.getBytes(), httpAccount.getBytes().length, httpPassword.getBytes(), httpPassword.getBytes().length);

    //checkSaveSmoothBin();     //#debug smooth
//tmp    Init_OLED_Country();
//tmp    Init_US360_OLED();
//tmp    disableShutdown(1);  
    //ReadLensCode();           //#old/new lens
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
	//	FanCtrlFunc();      //max+ S3 沒有風扇
        
    FPGA_Ctrl_Power_Func(0, 0);

    LoadConfig(1);    

    ret = LoadParameterTmp();
    if(ret != 1 && ret != -1) {		//debug, 紀錄S2發生檔案錯誤問題
        db_error("LoadParameterTmp() error! ret=%d\n", ret);   
        WriteLoadParameterTmpErr(ret);
    }

    ModeTypeSelectS2(Get_DataBin_PlayMode(), Get_DataBin_ResoultMode(), GetHdmiState(), Get_DataBin_CameraMode() );

    int kpixel = ResolutionModeToKPixel(GetResolutionMode() );
//tmp    setOLEDMainModeResolu(GetPlayMode2Tmp(), kpixel);

//tmp    SetOLEDMainType(GetCameraMode(), GetCaptureCnt(), GetCaptureMode(), getTimeLapseMode(), 0);
//tmp    showOLEDDelayValue(Get_DataBin_DelayValue());
        
    char path[64]; 
	if(Get_DataBin_SaveToSel() == 0)
		sprintf(sd_path, "/mnt/sdcard\0");
    else if(Get_DataBin_SaveToSel() == 1) 
        getSDPath();       	
		
    setStitchingOut(GetCameraMode(), GetPlayMode2(), GetResolutionMode(), GetmFPS() ); 
    LineTableInit();  
        
    for(i = 0; i < 8; i++) 
        writeCmdTable(i, GetResolutionMode(), GetmFPS(), 0, 1, 0); 
        
    getPath();		//取得 THMPath / DirPath
 
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

    Show_Now_Mode_Message(GetPlayMode2(), GetResolutionMode(), GetmFPS(), 0);
        
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

    setSendMcuA64Version(&mSSID[0], &mPwd[0], &VersionStr[0]);

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
	//EyeseeLinux::Layer::GetInstance()->SetLayerAlpha(EyeseeLinux::LAYER_UI, 150);
	//EyeseeLinux::Layer::GetInstance()->SetLayerTop(EyeseeLinux::LAYER_UI);
	camera->ShowPreview();
#endif
}