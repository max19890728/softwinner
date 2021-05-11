/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Data/databin.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

#include "Device/US363/us360.h"
#include "Device/US363/Data/countrylist.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::DataBin"

#define SWAP32(x) ((((x)&0xff000000) >> 24) | (((x)&0x00ff0000) >> 8) | (((x)&0x0000ff00) << 8) | (((x)&0x000000ff) << 24))

const char dirPath[64]    = "/mnt/extsd/US360\0";
const char mainPath[64]   = "/mnt/extsd/US360/US360DataBin.txt\0";
const char backupPath[64] = "/mnt/extsd/US360/US360DataBin_back.txt\0";

#define totalLength		1024
int isReadFinish = 0;


static int 		Version = 170906;
static int 		DemoMode = 1;							// 畫面移動模式, 		0: G-sensor		1: 慣性(有摩擦力)	2: 慣性(無摩擦力)
static int 		CamPositionMode = 0;					// G-sensor方向,		0: AUTO		1: 0度	2: 180度		3: 90度
static int 		PlayMode = 0; 							// 畫面觀看模式,		0: Global	1: Front	2:360	3: 240	4: 180	5: 4分割
static int 		ResoultMode = 1;						// 畫面解析度,		0:Fix  1:12K(Global)  2:4K(Global)  7:8K(Global)  8:10K(Global)  12:6K(Global)  13:3K(Global)  14:2K(Global)
static int 		EVValue = 0;							// EV值,				-6: EV-2	-5: EV-1.6	-4: EV-1.3	-3: EV-1	-2: EV-0.6	-1: EV-0.3	0: EV+0	1: EV+0.3	2: EV+0.6	3: EV+1	4: EV+1.3	5: EV+1.6	6: EV+2
static int 		MFValue = -9;							// MF值,				-32 ~ 32(目前實際值為 -27 ~ 36)
static int 		MF2Value = -9;							// MF2值,				-32 ~ 32(目前實際值為 -27 ~ 36)
static int 		ISOValue = -1;							// ISO值,				-1: AUTO	0: ISO 100	20: ISO 200	40: ISO 400	60: ISO 800	80: ISO 1600	120: ISO 3200	140: ISO 6400
static int 		ExposureValue = -1;						// 曝光時間,			-1: AUTO	260: 1	240: 1/2	220: 1/4	200: 1/8	180: 1/15	160: 1/30	140: 1/60	120: 1/120	100: 1/250	80: 1/500	60: 1/1000	40: 1/2000	20: 1/4000	0: 1/8000  -20: 1/16000  -40: 1/32000
static int 		WBMode = 0;								// 白平衡模式,			0: AUTO	1: Filament Lamp	2: Daylight Lamp	3: Sun	4: Cloudy,	>1000: 色溫
static int		CaptureMode = 0;						// 拍照模式,			-1: 連拍		0: 一般拍照	2:2秒自拍	10:10秒自拍
static int		CaptureCnt = 0;							// 連拍次數,			0: 1次	2: 2次	3: 3次
static int		CaptureSpaceTime = 160;					// 連拍間隔時間,		500: 500ms	1000: 1000ms
static int		SelfTimer = 0;							// 自拍倒數,			0: none	2: 2秒自拍	10:	10秒自拍
static int 		TimeLapseMode = 0;						// 縮時錄影模式,		0: none	1: 1s	2: 2s	3: 5s	4: 10s	5: 30s	6: 60s	7:0.166s
static int 		SaveToSel = 1;							// 儲存空間位置,		0: Internal Storage	1: MicroSD
static int		WifiDisableTime = 300;					// wifi自動關閉時間, 	-2:not disable	60:1分鐘		120:2分鐘	180:3分鐘	300:5分鐘
static int		EthernetMode = 0;						// Ethernet 模式, 	0:DHCP	1:MANUAL
static char		EthernetIP[4] = "";						// Ethernet IP,			0.0.0.0
static char		EthernetMask[4] = "";					// Ethernet Mask,		0.0.0.0
static char		EthernetGateWay[4] = "";				// Ethernet GateWay,	0.0.0.0
static char		EthernetDNS[4] = "";					// Ethernet DNS,		0.0.0.0
static int 		MediaPort = 8555;						// MediaPort, 		1~65535
static int		DrivingRecord = 1;						// 循環錄影,			0: off	1: on
static char		US360Versin[16] = "";					// US360 Version
static int		WifiChannel = 6;						// Wifi Channel
static int		ExposureFreq = 60;						// EP freq,			60:60Hz 50:50Hz
static int		FanControl = 2;							// 風扇控制,			0: off	1: fast	2: median 3: slow
static int 		Sharpness = 8;							// Sharpness,     	0 ~ 15
static int		UserCtrl30Fps = 0;						// User控制30FPS,		0:off	1:on
static int		CameraMode = 5;							// 攝影機模式,	    0:Cap 1:Rec 2:Time_Lapse 3:Cap_HDR 4:Cap_5_Sensor 5:日拍HDR 6:夜拍  7:夜拍HDR 8:Sport 9:SportWDR 10:RecWDR 11:Time_LapseWDR 12:B快門 13:動態移除 14:3D-Model
static int 		ColorSTMode = 1;						// 顏色縫合模式,		0:off 1:on 2:auto
static int 		AutoGlobalPhiAdjMode = 100;				// 即時縫合模式,		0~100
static int		HDMITextVisibility = 1;					// HDMI文字訊息,		0:hide 1:show
static int		speakerMode = 0;						// 喇叭,				0:On 1:Off
static int		ledBrightness = -1;						// LED亮度			-1:自動,亮度:0 ~ 100
static int		oledControl = 0;						// OLED控制			0:On 1:Off
static int		delayValue = 0;							// 延遲時間控制			0:none 2:2s 5:5s 10:10s 20:20s
static int		imageQuality = 0;						// 影像品質			0:高,1:中,2:低
static int		photographReso = 1;						// 拍照解析度			1:12k,7:8k,12:6k
static int		recordReso = 2;							// 錄影解析度			2:4k,13:3k,14:2k
static int		timeLapseReso = 12;						// 縮時解析度			1:12k,7:8k,12:6k,2:4k
static int		translucent = 1;						// 半透明設定			0:關,1:開,2:自動
static int		compassMaxx = -60000;					// 電子羅盤X軸最大值
static int		compassMaxy = -60000;					// 電子羅盤y軸最大值
static int		compassMaxz = -60000;					// 電子羅盤z軸最大值
static int		compassMinx = 60000;					// 電子羅盤X軸最小值
static int		compassMiny = 60000;					// 電子羅盤y軸最小值
static int		compassMinz = 60000;					// 電子羅盤z軸最小值
static int		debugLogMode = 0;						// 除錯訊息存至SD卡		0:關, 1:開
static int		bottomMode = 2;							// 底圖設定				0:關, 1:延伸, 2:底圖(default), 3:鏡像, 4:底圖(user)
static int		bottomSize = 50;						// 底圖大小				10 ~ 100
static int		hdrEvMode = 4;							// HDR EV設定			2:0,-1,-2 4:+1,-1,-3 8:+2,-1,-4
static int		Bitrate = 5;							// Rec Bitrate 控制,		0:最高 5:中等 10:最低
static char		HttpAccount[32] = "";					// 網頁/RTSP帳號
static char		HttpPassword[32] = "";					// 網頁/RTSP密碼
static int		HttpPort = 8080;						// HttpPort,		1024 ~ 65534
static int		compassMode = 1;						// 電子羅盤開關		0:關 1:開
static int		gsensorMode = 1;						// G-Sensor開關		0:關 1:開
static int		capHdrMode = 1;							// 拍照HDR模式開關	0:日拍, 1:日拍HDR, 2:夜拍, 3:夜拍HDR
static int		bottomTMode = 0;						// 底圖文字控制		0:關 1:開
static int		bottomTColor = 0;						// 底圖文字顏色代碼	0:white 1:bright_pink 2:red 3:orange 4:yellow 5:chartreuse 6:green 7:spring_green 8:cyan 9:azure 10:blue 11:violet 12:magenta 13:black
static int		bottomBColor = 13;						// 底圖文字背景顏色代碼 0:white 1:bright_pink 2:red 3:orange 4:yellow 5:chartreuse 6:green 7:spring_green 8:cyan 9:azure 10:blue 11:violet 12:magenta 13:black
static int		bottomTFont	= 0;						// 底圖文字字型		0:Default 1:Default_bold 2:MONOSPACE 3:SANS_SERIF 4:SERIF
static int		bottomTLoop = 1;						// 底圖循環顯示		0:關 1:開
static char		bottomText[70] = "";					// 底圖文字
static int		fpgaEncodeType = 1;						// 縮時錄影檔案類型	0:JPEG 1:H.264
static char		wbRGB[4] = {100,100,100,0};				// 白平衡RGB數值(0~255)	R.G.B.0
static int		contrast = 0;							// 對比度(暫定-7 ~ 7)
static int		saturation = 0;							// 彩度(暫定-7 ~ 7)
static int		freeCount = -1;							// 剩餘拍攝張數/錄影時間
static int		saveTimelapse = 0;						// 儲存縮時模式		0: none	1: 1s	2: 2s	3: 5s	4: 10s	5: 30s	6: 60s	7:0.166s
static int		bmodeSec = 4;							// B快門時間
static int		bmodeGain = 0;							// B快門ISO
static int		hdrManual = 2;							// HDR手動模式開關	0:關  1:開  2:Auto
static int		hdrNumber = 5;							// HDR手動模式張數	3,5,7
static int		hdrIncrement = 10;						// HDR手動模式EV值	5:+-0.5 10:+-1.0 15:+-1.5 20:+-2.0 25:+-2.5
static int		hdrStrength = 60;						// HDR手動模式強度	30 ~ 90
static int		hdrTone = 0;							// HDR手動模式影調壓縮	-10 ~ 10
static int		aebNumber = 7;							// AEB手動模式張數	3,5,7
static int		aebIncrement = 10;						// AEB手動模式EV值	5:+-0.5 10:+-1.0 15:+-1.5 20:+-2.0 25:+-2.5
static int		LiveQualityMode = 0;					// Live Quality Mode 0:10FPS Hight Quality	1:5FPS Low Quality
static int		wbTemperature = 50;						// 白平衡色溫值		20 ~ 100(2200K ~ 10000K)
static int		hdrDeghosting = 0;						// HDR去鬼影 0:關 1:開
static int		removeHdrMode = 1;						// 動態移除模式		0:關 1:自動 2:弱 3:中 4:強 5:自訂
static int		removeHdrNumber = 5;					// Not Use
static int		removeHdrIncrement = 10;				// 動態移除HDR EV值 	5,10,15,20,25
static int		removeHdrStrength = 60;					// 動態移除HDR強度	30 ~ 90
static int		AntiAliasingEn = 1;						// 反鋸齒			0:off 1:on
static int		removeAntiAliasingEn = 1;				// 動態移除反鋸齒	0:off 1:on
static int		wbTint = 0;								// 白平衡Tint		-15 ~ +15
static int		hdrAutoStrength = 60;					// HDR Auto模式強度	30 ~ 90
static int		removeHdrAutoStrength = 60;				// 動態移除HDR Auto強度	30 ~ 90
static int		LiveBitrate = 5;						// Live Bitrate 控制,		0:最高 5:中等 10:最低
static int		PowerSaving = 0;						// 省電模式 			0:Off 1:On
//Reserved = 4 Bytes


#define TAG_MAX						96

#define TagVersion					0
#define TagDemoMode 				1
#define TagCamPositionMode			2
#define TagPlayMode 				3
#define TagResoultMode	 			4
#define TagEVValue 					5
#define TagMFValue  				6
#define TagMF2Value  				7
#define TagISOValue  				8
#define TagExposureValue  			9
#define TagWBMode 					10
#define TagCaptureMode  			11
#define TagCaptureCnt  				12
#define TagCaptureSpaceTime  		13
#define TagSelfTimer  				14
#define TagTimeLapseMode  			15
#define TagSaveToSel  				16
#define TagWifiDisableTime  		17
#define TagEthernetMode  			18
#define TagEthernetIP  				19
#define TagEthernetMask  			20
#define TagEthernetGateWay  		21
#define TagEthernetDNS  			22
#define TagMediaPort  				23
#define TagDrivingRecord  			24
#define TagUS360Versin  			25
#define TagWifiChannel  			26
#define TagExposureFreq  			27
#define TagFanControl  				28
#define TagSharpness  				29
#define TagUserCtrl30Fps  			30
#define TagCameraMode  				31
#define TagColorSTMode  			32
#define TagAutoGlobalPhiAdjMode 	33
#define TagHDMITextVisibility  		34
#define TagSpeakerMode  			35
#define TagLedBrightness  			36
#define TagOledControl  			37
#define TagDelayValue  				38
#define TagImageQuality 			39
#define TagPhotographReso  			40
#define TagRecordReso  				41
#define TagTimeLapseReso  			42
#define TagTranslucent  			43
#define TagCompassMaxx  			44
#define TagCompassMaxy  			45
#define TagCompassMaxz  			46
#define TagCompassMinx  			47
#define TagCompassMiny  			48
#define TagCompassMinz  			49
#define TagDebugLogMode  			50
#define TagBottomMode  				51
#define TagBottomSize  				52
#define TagHdrEvMode  				53
#define TagBitrate    				54
#define TagHttpAccount  			55
#define TagHttpPassword  			56
#define TagHttpPort   				57
#define TagCompassMode   			58
#define TagGsensorMode   			59
#define TagCapHdrMode 				60
#define TagBottomTMode 				61
#define TagBottomTColor 			62
#define TagBottomBColor 			63
#define TagBottomTFont 				64
#define TagBottomTLoop 				65
#define TagBottomText 				66
#define TagFpgaEncodeType 			67
#define TagWbRGB 					68
#define TagContrast 				69
#define TagSaturation 				70
#define TagFreeCount 				71
#define TagSaveTimelapse  			72
#define TagBmodeSec 		 		73
#define TagBmodeGain 		 		74
#define TagHdrManual 		 		75
#define TagHdrNumber 		 		76
#define TagHdrIncrement 			77
#define TagHdrStrength 				78
#define TagHdrTone 		 			79
#define TagAebNumber 		 		80
#define TagAebIncrement 			81
#define TagLiveQualityMode 			82
#define TagWbTemperature 			83
#define TagHdrDeghosting 			84
#define TagRemoveHdrMode 			85
#define TagRemoveHdrNumber 			86
#define TagRemoveHdrIncrement  		87
#define TagRemoveHdrStrength 		88
#define TagAntiAliasingEn 			89
#define TagRemoveAntiAliasingEn  	90
#define TagWbTint  					91
#define TagHdrAutoStrength  		92
#define TagRemoveHdrAutoStrength 	93
#define TagLiveBitrate  			94
#define TagPowerSaving  			95

static int versionDate = 190507;
void SetDataBinVersionDate(int ver) {
	versionDate = ver;
}
int GetDataBinVersionDate(void) {
	return versionDate;
}

//{ValType,		0: int 1: byte
//CheckType,	0: 不限制 1: 極限值限制  2:例外處理
//MinValue,		最小值
//MaxValue}		最大值
static int checkArray[TAG_MAX][4] = {
		{0,0,0,0},			// version
		{0,1,0,2},			// 畫面移動模式, 		0: G-sensor		1: 慣性(有摩擦力)	2: 慣性(無摩擦力)
		{0,1,0,3},			// G-sensor方向,			0: AUTO		1: 0度	2: 180度		3: 90度
		{0,1,0,5},			// 畫面觀看模式,			0: Global	1: Front	2:360	3: 240	4: 180	5: 4分割
		{0,2,0,14},			// 畫面解析度,			0: Fix	1: 10M	2: 2.5M	3: 2M	4: 1M	5: D1	6: 1.8M(3072x576)	7: 5M	8: 8M
		{0,1,-6,6},			// EV值,					-6: EV-2	-5: EV-1.6	-4: EV-1.3	-3: EV-1	-2: EV-0.6	-1: EV-0.3	0: EV+0	1: EV+0.3	2: EV+0.6	3: EV+1	4: EV+1.3	5: EV+1.6	6: EV+2
		{0,1,-32,36},		// MF值,					-32 ~ 36
		{0,1,-32,36},		// MF2值,				-32 ~ 36
		{0,1,-1,140},		// ISO值,				-1: AUTO	0: ISO 100	20: ISO 200	40: ISO 400	60: ISO 800	80: ISO 1600	120: ISO 3200	140: ISO 6400
		{0,1,-40,300},		// 曝光時間,				-1: AUTO	260: 1	240: 1/2	220: 1/4	200: 1/8	180: 1/15	160: 1/30	140: 1/60	120: 1/120	100: 1/250	80: 1/500	60: 1/1000	40: 1/2000	20: 1/4000	0: 1/8000  -20: 1/16000  -40: 1/32000
		{0,1,0,8000},		// 白平衡模式,			0: AUTO	1: Filament Lamp	2: Daylight Lamp	3: Sun	4: Cloudy,	>1000: 色溫
		{0,1,-1,10},		// 拍照模式,				-1: 連拍		0: 一般拍照	2:2秒自拍	10:10秒自拍
		{0,1,0,3},			// 連拍次數,				0: 1次	2: 2次	3: 3次
		{0,0,0,0},			// 連拍間隔時間,			500: 500ms	1000: 1000ms
		{0,1,0,30},			// 自拍倒數,				0: none	2: 2秒自拍	10:	10秒自拍
		{0,1,0,7},			// 縮時錄影模式,			0: none	1: 1s	2: 2s	3: 5s	4: 10s	5: 30s	6: 60s	7:0.166s
		{0,1,0,1},			// 儲存空間位置,			0: Internal Storage	1: MicroSD
		{0,1,-2,300},		// wifi自動關閉時間, 	-2:not disable	60:1分鐘		120:2分鐘	180:3分鐘	300:5分鐘
		{0,1,0,1},			// Ethernet 模式, 		0:DHCP	1:MANUAL
		{1,0,0,0},			// Ethernet IP,			0.0.0.0
		{1,0,0,0},			// Ethernet Mask,		0.0.0.0
		{1,0,0,0},			// Ethernet GateWay,	0.0.0.0
		{1,0,0,0},			// Ethernet DNS,		0.0.0.0
		{0,2,1,65535},		// MediaPort, 			1~65535
		{0,1,0,0},			// 循環錄影,				0: off	1: on
		{1,0,0,0},			// US360 Version
		{0,1,1,11},			// Wifi Channel
		{0,1,50,60},		// EP freq,				60:60Hz 50:50Hz
		{0,1,0,3},			// 風扇控制,				0: off	1: fast	2: median 3: slow
		{0,1,0,15},			// Sharpness,     		0 ~ 15
		{0,1,0,1},			// User控制30FPS,		0:off	1:on
		{0,1,0,14},			// 攝影機模式,			0:Cap 1:Rec 2:Time_Lapse 3:Cap_HDR 4:Cap_5_Sensor 5:日拍HDR 6:夜拍  7:夜拍HDR 8:Sport 9:SportWDR 10:RecWDR 11:Time_LapseWDR 12:B快門 13:動態移除 14:3D-Model
		{0,1,0,2},			// 顏色縫合模式,			0:off 1:on 2:auto
		{0,1,0,100},		// 即時縫合模式,			0~100
		{0,1,0,1},			// HDMI文字訊息,			0:hide 1:show
		{0,1,0,1},			// 喇叭,					0:On 1:Off
		{0,1,-1,100},		// LED亮度				-1:自動,亮度:0 ~ 100
		{0,1,0,1},			// OLED控制				0:On 1:Off
		{0,1,0,20},			// 延遲時間控制			0:none 2:2s 5:5s 10:10s 20:20s
		{0,1,0,2},			// 影像品質				0:高,1:中,2:低
		{0,2,1,14},			// 拍照解析度			1:12k,7:8k,12:6k
		{0,2,1,14},			// 錄影解析度			2:4k,13:3k,14:2k
		{0,2,1,14},			// 縮時解析度			7:8k,12:6k,2:4k
		{0,1,0,2},			// 半透明設定			0:關,1:開,2:自動
		{0,1,-60000,60000},		// 電子羅盤X軸最大值
		{0,1,-60000,60000},		// 電子羅盤y軸最大值
		{0,1,-60000,60000},		// 電子羅盤z軸最大值
		{0,1,-60000,60000},		// 電子羅盤X軸最小值
		{0,1,-60000,60000},		// 電子羅盤y軸最小值
		{0,1,-60000,60000},		// 電子羅盤z軸最小值
		{0,1,0,1},			// 除錯訊息存至SD卡		0:關, 1:開
		{0,1,0,4},			// 底圖設定				0:關, 1:延伸, 2:底圖(default), 3:鏡像, 4:底圖(user)
		{0,1,10,100},		// 底圖大小				10 ~ 100
		{0,1,2,8},			// HDR EV設定			2:0,-1,-2 4:+1,-1,-3 8:+2,-1,-4
		{0,1,0,10},			// Rec Bitrate				0:最高 5:中等 10:最低
		{1,0,0,0},			// HttpAccount			網頁/RTSP帳號
		{1,0,0,0},			// HttpPassword			網頁/RTSP密碼
		{0,2,1024,65534},	// HttpPort				1024 ~ 65534
		{0,1,0,1},			// 電子羅盤開關			0:關, 1:開
		{0,1,0,1},			// G-Sensor開關			0:關, 1:開
		{0,1,0,6},			// 拍照WDR開關			0:日拍, 1:日拍HDR, 2:夜拍, 3:夜拍HDR 4:運動 5:運動HDR 6:B快門
		{0,1,0,1},			// 底圖文字控制
		{0,1,0,13},			// 底圖文字顏色代碼
		{0,1,0,13},			// 底圖文字背景顏色代碼
		{0,1,0,4},			// 底圖文字字型
		{0,1,0,1},			// 底圖循環顯示
		{1,0,0,0},			// 底圖文字
		{0,2,0,1},			// 縮時錄影檔案類型
		{1,0,0,0},			// 白平衡RGB數值(0~255)	R.G.B.0
		{0,1,-7,7},			// 對比度 -7~7
		{0,1,-7,7},			// 彩度 -7~7
		{0,0,0,0},			// 剩餘拍攝張數/錄影時間
		{0,1,1,7},			// 儲存縮時模式(0不儲存)	1: 1s	2: 2s	3: 5s	4: 10s	5: 30s	6: 60s	7:0.166s
		{0,1,4,720},		// B快門時間
		{0,1,0,140},		// B快門ISO
		{0,1,0,2},			// HDR手動模式開關	0:關  1:開  2:Auto
		{0,1,3,7},			// HDR手動模式張數	3,5,7
		{0,1,5,25},			// HDR手動模式EV值	5:+-0.5 10:+-1.0 15:+-1.5 20:+-2.0 25:+-2.5
		{0,1,30,90},		// HDR手動模式強度	30 ~ 90
		{0,1,-10,10},		// HDR手動模式影調壓縮	-10 ~ 10
		{0,1,3,7},			// AEB手動模式張數	3,5,7
		{0,1,5,25},			// AEB手動模式EV值	5:+-0.5 10:+-1.0 15:+-1.5 20:+-2.0 25:+-2.5
		{0,1,0,1},			// Live Quality Mode 0:10FPS Hight Quality	1:5FPS Low Quality
		{0,1,22,100},		// 白平衡色溫值 20 ~ 100(2200K ~ 10000K)
		{0,1,0,1},			// HDR去鬼影 0:關 1:開
		{0,1,0,5},			// 動態移除模式 0:關 1:自動 2:弱 3:中 4:強 5:自訂
		{0,1,0,7},			// 動態移除HDR張數 3,5,7
		{0,1,0,25},			// 動態移除HDR EV值 5,10,15,20,25
		{0,1,30,90},		// 動態移除HDR強度 30 ~ 90
		{0,1,0,1},			// 反鋸齒				0:off 1:on
		{0,1,0,1},			// 動態移除反鋸齒		0:off 1:on
		{0,1,-15,15},		// 白平衡Tint		-15 ~ +15
		{0,1,30,90},		// HDR Auto模式強度	30 ~ 90
		{0,1,30,90},		// 動態移除HDR Auto強度 30 ~ 90
		{0,1,0,10},			// Live Bitrate				0:最高 5:中等 10:最低
		{0,1,0,1}			// 省電模式			0:Off 1:On
};
static int cameraReso[15][4] = {
		{1, 7,12,-1},				//Cap 			1:12K 	 7:8K 	12:6K
		{2,13,14,-1},				//Rec 			2:4K 	13:3K 	14:2K
		{1, 2, 7,12},				//TimeLapse 	1:12K 	 2:4K 	 7:8K 	12:6K
		{1,-1,-1,-1},				//AEB(3P)   	1:12K
		{1, 7,12,-1},				//RAW(5P)   	1:12K 	 7:8K 	12:6K
		{1, 7,12,-1},				//WDR       	1:12K 	 7:8K 	12:6K
		{1, 7,12,-1},				//Night     	1:12K 	 7:8K 	12:6K
		{1, 7,12,-1},				//NightWDR  	1:12K 	 7:8K 	12:6K
		{1, 7,12,-1},				//Sport 		1:12K 	 7:8K 	12:6K
		{1, 7,12,-1},				//SportWDR 		1:12K 	 7:8K 	12:6K
		{2,13,14,-1},				//RecWDR 		2:4K 	13:3K 	14:2K
		{1, 2, 7,12},				//TimeLapseWDR 	1:12K 	 2:4K 	 7:8K 	12:6K
		{1, 7,12,-1},				//Bmode 		1:12K 	 7:8K 	12:6K
		{1,-1,-1,-1},				//remove  	1:12K
		{1,-1,-1,-1}				//remove HDR	1:12K
};
int GetDataBinTotalLength(void) {
	return totalLength;
}

int checkIntValue(int tag,int value) {
	int x, length;
	int retunValue = value;
	int valueType = checkArray[tag][0];
	int checkType = checkArray[tag][1];
	int invalidRtspPorts[4] = {8080,8554,8556,8557};
	int invalidHttpPorts[4] = {8554,8555,8556,8557};
	int getReso = 0;

	if(valueType == 0){
		if(checkType == 1){
			if(retunValue < checkArray[tag][2]){
				retunValue = checkArray[tag][2];
			}
			if(retunValue > checkArray[tag][3]){
				retunValue = checkArray[tag][3];
			}
		}
		else if(checkType == 2){
			switch(tag){
			case TagResoultMode:
				for(x=0; x < 4; x++){
					if(cameraReso[CameraMode][x] == -1)
						break;
					if(cameraReso[CameraMode][x] == retunValue){
						getReso = 1;
						break;
					}
				}
				if(!getReso){
					retunValue = cameraReso[CameraMode][0];
				}
				break;
			case TagPhotographReso:
				for(x=0; x < 4; x++){
					if(cameraReso[0][x] == -1)
						break;
					if(cameraReso[0][x] == retunValue){
						getReso = 1;
						break;
					}
				}
				if(!getReso){
					retunValue = 1;
				}
				break;
			case TagRecordReso:
				for(x=0; x < 4; x++){
					if(cameraReso[1][x] == -1)
						break;
					if(cameraReso[1][x] == retunValue){
						getReso = 1;
						break;
					}
				}
				if(!getReso){
					retunValue = 14;
				}
				break;
			case TagTimeLapseReso:
				for(x=0; x < 4; x++){
					if(cameraReso[2][x] == -1)
						break;
					if(cameraReso[2][x] == retunValue){
						getReso = 1;
						break;
					}
				}
				if(!getReso){
					retunValue = 12;
				}
				break;
			case TagMediaPort:
		        if(retunValue < 1024 || retunValue > 65534){
		        	retunValue = 8555;
		        }else if(retunValue == HttpPort){
		        	retunValue = 8555;
		        }else{
		        	for(x=0; x<4;x++){
			        	if(retunValue == invalidRtspPorts[x]){
			        		retunValue = 8555;
			        		break;
			        	}
			        }
		        }
		        break;
			case TagHttpPort:
		        if(retunValue < 1024 || retunValue > 65534){
		        	retunValue = 8080;
		        }else if(retunValue == MediaPort){
		        	retunValue = 8080;
		        }else{
		        	for(x=0; x<4;x++){
			        	if(retunValue == invalidHttpPorts[x]){
			        		retunValue = 8080;
			        		break;
			        	}
			        }
		        }
				break;
			case TagFpgaEncodeType:
				if(timeLapseReso == 1)
					retunValue = 0;
				if(retunValue < checkArray[tag][2])
					retunValue = checkArray[tag][2];
				else if(retunValue > checkArray[tag][3])
					retunValue = checkArray[tag][3];
			}
		}
	}
	return retunValue;
}

int split_c(char **buf, char *str, char *del)
{
	int cnt = 0;
	char *s_tmp = strtok(str, del);
	while(s_tmp != NULL) {
		*buf++ = s_tmp;
		s_tmp = strtok(NULL, del);
		cnt++;
	}
	return cnt;
}


void Set_DataBin_Version(int ver){
	Version = checkIntValue(TagVersion,ver);
}
void Set_DataBin_DemoMode(int mode){
	DemoMode = checkIntValue(TagDemoMode,mode);
}
void Set_DataBin_CamPositionMode(int mode){
	CamPositionMode = checkIntValue(TagCamPositionMode,mode);
}
void Set_DataBin_PlayMode(int mode){
	PlayMode = checkIntValue(TagPlayMode,mode);
}
void Set_DataBin_ResoultMode(int mode){
	ResoultMode = checkIntValue(TagResoultMode,mode);
}
void Set_DataBin_EVValue(int value){
	EVValue = checkIntValue(TagEVValue,value);
}
void Set_DataBin_MFValue(int value){
	MFValue = checkIntValue(TagMFValue,value);
}
void Set_DataBin_MF2Value(int value){
	MF2Value = checkIntValue(TagMF2Value,value);
}
void Set_DataBin_ISOValue(int value){
	ISOValue = checkIntValue(TagISOValue,value);
}
void Set_DataBin_ExposureValue(int value){
	ExposureValue = checkIntValue(TagExposureValue,value);
}
void Set_DataBin_WBMode(int mode){
	WBMode = checkIntValue(TagWBMode,mode);
}
void Set_DataBin_CaptureMode(int mode){
	CaptureMode = checkIntValue(TagCaptureMode,mode);
}
void Set_DataBin_CaptureCnt(int cnt){
	CaptureCnt = checkIntValue(TagCaptureCnt,cnt);
}
void Set_DataBin_CaptureSpaceTime(int time){
	CaptureSpaceTime = checkIntValue(TagCaptureSpaceTime,time);
}
void Set_DataBin_SelfTimer(int time){
	SelfTimer = checkIntValue(TagSelfTimer,time);
}
void Set_DataBin_TimeLapseMode(int mode){
	TimeLapseMode = checkIntValue(TagTimeLapseMode,mode);
}
void Set_DataBin_SaveToSel(int sel){
	SaveToSel = checkIntValue(TagSaveToSel,sel);
}
void Set_DataBin_WifiDisableTime(int time){
	WifiDisableTime = checkIntValue(TagWifiDisableTime,time);
}
void Set_DataBin_EthernetMode(int mode){
	EthernetMode = checkIntValue(TagEthernetMode,mode);
}
void Set_DataBin_EthernetIP(char *ip){
	int i, size=0, val=0;
	char *tokens[4];
	char *del = "\\.";

	size = split_c(tokens, ip, del);
	if(size == 4){
		for(i=0; i<size; i++){
			val = atoi(tokens[i]);
			EthernetIP[i] = (char) val;
db_debug("Set_DataBin_EthernetIP() 01 EthernetIP[%d]=%d\n", i, EthernetIP[i]);
		}
	}
	else{		// error format
		for(i=0;i<4;i++){
			EthernetIP[i] = 0;
db_debug("Set_DataBin_EthernetIP() 02 EthernetIP[%d]=%d\n", i, EthernetIP[i]);
		}
	}
}
void Set_DataBin_EthernetMask(char *mask){
	int i, size=0, val=0;
	char *tokens[4];
	char *del = "\\.";

	size = split_c(tokens, mask, del);
	if(size == 4){
		for(i=0; i<size; i++){
			val = atoi(tokens[i]);
			EthernetMask[i] = (char) val;
		}
	}
	else{		// error format
		for(i=0;i<4;i++){
			EthernetMask[i] = 0;
		}
	}
}
void Set_DataBin_EthernetGateWay(char *gateway){
	int i, size=0, val=0;
	char *tokens[4];
	char *del = "\\.";

	size = split_c(tokens, gateway, del);
	if(size == 4){
		for(i=0; i<size; i++){
			val = atoi(tokens[i]);
			EthernetGateWay[i] = (char) val;
		}
	}
	else{		// error format
		for(i=0;i<4;i++){
			EthernetGateWay[i] = 0;
		}
	}
}
void Set_DataBin_EthernetDNS(char *dns){
	int i, size=0, val=0;
	char *tokens[4];
	char *del = "\\.";

	size = split_c(tokens, dns, del);
	if(size == 4){
		for(i=0; i<size; i++){
			val = atoi(tokens[i]);
			EthernetDNS[i] = (char) val;
		}
	}
	else{		// error format
		for(i=0;i<4;i++){
			EthernetDNS[i] = 0;
		}
	}
}
void Set_DataBin_MediaPort(int port){
	MediaPort = checkIntValue(TagMediaPort,port);
}
void Set_DataBin_DrivingRecord(int record){
	DrivingRecord = checkIntValue(TagDrivingRecord,record);
}
void Set_DataBin_US360Version(char *version){
	int x;
	int length = strlen(version);
	int size = sizeof(US360Versin);
	for(x = 0; x < size; x++) {
		US360Versin[x] = '\0';
		if(x < length) {
			if(x == (size-1) )
				US360Versin[x] = '\0';
			else if(version[x] == '\n' || version[x] == '\r')
				US360Versin[x] = '\0';
			else
				US360Versin[x] = version[x];
		}
	}
}
void Set_DataBin_WifiChannel(int channel){
	if(channel >= 1 && channel <= 11){
		WifiChannel = channel;
	}
	else{
		WifiChannel = 6;
	}
}
void Set_DataBin_ExposureFreq(int freq){
	if(freq == 50 || freq == 60){
		ExposureFreq = freq;
	}
	else{
		ExposureFreq = 60;
	}
}
void Set_DataBin_FanControl(int ctrl){
	FanControl = checkIntValue(TagFanControl,ctrl);
}
void Set_DataBin_Sharpness(int sharp){
	Sharpness = checkIntValue(TagSharpness,sharp);
}
void Set_DataBin_UserCtrl30Fps(int ctrl){
	UserCtrl30Fps = checkIntValue(TagUserCtrl30Fps,ctrl);
}
void Set_DataBin_CameraMode(int c_mode){
db_debug("Set_DataBin_CameraMode() OLDE: c_mode=%d\n", c_mode);
	CameraMode = checkIntValue(TagCameraMode,c_mode);
}
void Set_DataBin_ColorSTMode(int mode){
	ColorSTMode = checkIntValue(TagColorSTMode,mode);
}
void Set_DataBin_AutoGlobalPhiAdjMode(int mode){
	AutoGlobalPhiAdjMode = checkIntValue(TagAutoGlobalPhiAdjMode,mode);
}
void Set_DataBin_HDMITextVisibility(int visibility){
	HDMITextVisibility = checkIntValue(TagHDMITextVisibility,visibility);
}
void Set_DataBin_SpeakerMode(int mode){
	speakerMode = checkIntValue(TagSpeakerMode,mode);
}
void Set_DataBin_LedBrightness(int led){
	ledBrightness = checkIntValue(TagLedBrightness,led);
}
void Set_DataBin_OledControl(int ctrl){
	oledControl = checkIntValue(TagOledControl,ctrl);
}
void Set_DataBin_DelayValue(int value){
	delayValue = checkIntValue(TagDelayValue,value);
}
void Set_DataBin_ImageQuality(int quality){
	imageQuality = checkIntValue(TagImageQuality,quality);
}
void Set_DataBin_PhotographReso(int res){
	photographReso = checkIntValue(TagPhotographReso,res);
}
void Set_DataBin_RecordReso(int res){
	recordReso = checkIntValue(TagRecordReso,res);
}
void Set_DataBin_TimeLapseReso(int res){
	timeLapseReso = checkIntValue(TagTimeLapseReso,res);
}
void Set_DataBin_Translucent(int tran){
	translucent = checkIntValue(TagTranslucent,tran);
}
void Set_DataBin_CompassMaxx(int maxx){
	compassMaxx = checkIntValue(TagCompassMaxx,maxx);
}
void Set_DataBin_CompassMaxy(int maxy){
	compassMaxy = checkIntValue(TagCompassMaxy,maxy);
}
void Set_DataBin_CompassMaxz(int maxz){
	compassMaxz = checkIntValue(TagCompassMaxz,maxz);
}
void Set_DataBin_CompassMinx(int minx){
	compassMinx = checkIntValue(TagCompassMinx,minx);
}
void Set_DataBin_CompassMiny(int miny){
	compassMiny = checkIntValue(TagCompassMiny,miny);
}
void Set_DataBin_CompassMinz(int minz){
	compassMinz = checkIntValue(TagCompassMinz,minz);
}
void Set_DataBin_DebugLogMode(int mode){
	debugLogMode = checkIntValue(TagDebugLogMode,mode);
}
void Set_DataBin_BottomMode(int mode){
	bottomMode = checkIntValue(TagBottomMode,mode);
}
void Set_DataBin_BottomSize(int size){
	bottomSize = checkIntValue(TagBottomSize,size);
}
void Set_DataBin_hdrEvMode(int mode){
	hdrEvMode = checkIntValue(TagHdrEvMode,mode);
}
void Set_DataBin_Bitrate(int rate){
	Bitrate = checkIntValue(TagBitrate,rate);
}
void Set_DataBin_HttpAccount(char *account){
	int x;
	int len = strlen(account);
	//sprintf(HttpAccount, "%s", account);
	for(x = 0 ; x < 32;x++){
		if(x < len){
			if(account[x] == '\n' || account[x] == '\r')
				HttpAccount[x] = '\0';
			else
				HttpAccount[x] = account[x];
		}else{
			HttpAccount[x] = '\0';
		}
    }
}
void Set_DataBin_HttpPassword(char *password){
	int x;
	int len = strlen(password);
	//sprintf(HttpPassword, "%s", password);
	for(x = 0 ; x < 32;x++){
		if(x < len){
			if(password[x] == '\n' || password[x] == '\r')
				HttpPassword[x] = '\0';
			else
				HttpPassword[x] = password[x];
		}else{
			HttpPassword[x] = '\0';
		}
    }
}
void Set_DataBin_HttpPort(int port){
	HttpPort = checkIntValue(TagHttpPort,port);
}
void Set_DataBin_CompassMode(int mode){
	compassMode = checkIntValue(TagCompassMode,mode);
}
void Set_DataBin_GsensorMode(int mode){
	gsensorMode = checkIntValue(TagGsensorMode,mode);
}
void Set_DataBin_CapHdrMode(int mode){
	capHdrMode = checkIntValue(TagCapHdrMode,mode);
}
void Set_DataBin_BottomTMode(int mode){
	bottomTMode = checkIntValue(TagBottomTMode,mode);
}
void Set_DataBin_BottomTColor(int color){
	bottomTColor = checkIntValue(TagBottomTColor,color);
}
void Set_DataBin_BottomBColor(int color){
	bottomBColor = checkIntValue(TagBottomBColor,color);
}
void Set_DataBin_BottomTFont(int tfont){
	bottomTFont = checkIntValue(TagBottomTFont,tfont);
}
void Set_DataBin_BottomTLoop(int loop){
	bottomTLoop = checkIntValue(TagBottomTLoop,loop);
}
void Set_DataBin_BottomText(char *text){
	int x;
	int len = strlen(text);
	//sprintf(bottomText, "%s", text);
	for(x = 0 ; x < 70;x++){
		if(x < len){
			if(text[x] == '\n' || text[x] == '\r')
				bottomText[x] = '\0';
			else
				bottomText[x] = text[x];
		}else{
			bottomText[x] = 0;
		}
    }
}
void Set_DataBin_FpgaEncodeType(int type){
	fpgaEncodeType = checkIntValue(TagFpgaEncodeType,type);
}
void Set_DataBin_WbRGB(char *rgb){
	int i, size=0, val=0;
	char *tokens[4];
	char *del = "\\.";

	size = split_c(tokens, rgb, del);
	if(size == 3){
		for(i=0; i<size; i++){
			val = atoi(tokens[i]);
			wbRGB[i] = (char) val;
		}
	}else{		// error format
		for(i=0;i<4;i++){
			wbRGB[i] = 100;
		}
	}
}
void Set_DataBin_Contrast(int Contrast){
	contrast = checkIntValue(TagContrast,Contrast);
}
void Set_DataBin_Saturation(int Saturation){
	saturation = checkIntValue(TagSaturation,Saturation);
}
void Set_DataBin_FreeCount(int FreeCount){
	freeCount = checkIntValue(TagFreeCount,FreeCount);
}
void Set_DataBin_SaveTimelapse(int SaveTimelapse){
	saveTimelapse = checkIntValue(TagSaveTimelapse,SaveTimelapse);
}
void Set_DataBin_BmodeSec(int BmodeSec){
	bmodeSec = checkIntValue(TagBmodeSec,BmodeSec);
}
void Set_DataBin_BmodeGain(int BmodeGain){
	bmodeGain = checkIntValue(TagBmodeGain,BmodeGain);
}
void Set_DataBin_HdrManual(int HdrManual){
	hdrManual = checkIntValue(TagHdrManual,HdrManual);
}
void Set_DataBin_HdrNumber(int HdrNumber){
	hdrNumber = checkIntValue(TagHdrNumber,HdrNumber);
}
void Set_DataBin_HdrIncrement(int HdrIncrement){
	hdrIncrement = checkIntValue(TagHdrIncrement,HdrIncrement);
}
void Set_DataBin_HdrStrength(int HdrStrength){
	hdrStrength = checkIntValue(TagHdrStrength,HdrStrength);
}
void Set_DataBin_HdrTone(int HdrTone){
	hdrTone = checkIntValue(TagHdrTone,HdrTone);
}
void Set_DataBin_AebNumber(int AebNumber){
	aebNumber = checkIntValue(TagAebNumber,AebNumber);
}
void Set_DataBin_AebIncrement(int AebIncrement){
	aebIncrement = checkIntValue(TagAebIncrement,AebIncrement);
}
void Set_DataBin_LiveQualityMode(int Mode){
	LiveQualityMode = checkIntValue(TagLiveQualityMode, Mode);
}
void Set_DataBin_WbTemperature(int temp){
	wbTemperature = checkIntValue(TagWbTemperature, temp);
}
void Set_DataBin_HdrDeghosting(int mode){
	hdrDeghosting = checkIntValue(TagHdrDeghosting, mode);
}
void Set_DataBin_RemoveHdrMode(int val){
	removeHdrMode = checkIntValue(TagRemoveHdrMode, val);
}
void Set_DataBin_RemoveHdrNumber(int val){
	removeHdrNumber = checkIntValue(TagRemoveHdrNumber, val);
}
void Set_DataBin_RemoveHdrIncrement(int val){
	removeHdrIncrement = checkIntValue(TagRemoveHdrIncrement, val);
}
void Set_DataBin_RemoveHdrStrength(int val){
	removeHdrStrength = checkIntValue(TagRemoveHdrStrength, val);
}
void Set_DataBin_AntiAliasingEn(int val){
	AntiAliasingEn = checkIntValue(TagAntiAliasingEn, val);
}
void Set_DataBin_RemoveAntiAliasingEn(int val){
	removeAntiAliasingEn = checkIntValue(TagRemoveAntiAliasingEn, val);
}
void Set_DataBin_WbTint(int tint){
	wbTint = checkIntValue(TagWbTint, tint);
}
void Set_DataBin_HdrAutoStrength(int HdrStrength){
	hdrAutoStrength = checkIntValue(TagHdrAutoStrength,HdrStrength);
}
void Set_DataBin_RemoveHdrAutoStrength(int val){
	removeHdrAutoStrength = checkIntValue(TagRemoveHdrAutoStrength, val);
}
void Set_DataBin_LiveBitrate(int bitrate){
	LiveBitrate = checkIntValue(TagLiveBitrate,bitrate);
}
void Set_DataBin_PowerSaving(int mode){
	PowerSaving = checkIntValue(TagPowerSaving,mode);
}


int Get_DataBin_Version(){
	return Version;
}
int Get_DataBin_DemoMode(){
	return DemoMode;
}
int Get_DataBin_CamPositionMode(){
	return CamPositionMode;
}
int Get_DataBin_PlayMode(){
	return PlayMode;
}
int Get_DataBin_ResoultMode(){
	return ResoultMode;
}
int Get_DataBin_EVValue(){
	return EVValue;
}
int Get_DataBin_MFValue(){
	return MFValue;
}
int Get_DataBin_MF2Value(){
	return MF2Value;
}
int Get_DataBin_ISOValue(){
	return ISOValue;
}
int Get_DataBin_ExposureValue(){
	return ExposureValue;
}
int Get_DataBin_WBMode(){
	return WBMode;
}
int Get_DataBin_CaptureMode(){
	return CaptureMode;
}
int Get_DataBin_CaptureCnt(){
	return CaptureCnt;
}
int Get_DataBin_CaptureSpaceTime(){
	return CaptureSpaceTime;
}
int Get_DataBin_SelfTimer(){
	return SelfTimer;
}
int Get_DataBin_TimeLapseMode(){
	return TimeLapseMode;
}
int Get_DataBin_SaveToSel(){
	return SaveToSel;
}
int Get_DataBin_WifiDisableTime(){
	return WifiDisableTime;
}
int Get_DataBin_EthernetMode(){
	return EthernetMode;
}
void Get_DataBin_EthernetIP(char *ip, int size){
	char ip_tmp[64];
	sprintf(ip_tmp, "%d.%d.%d.%d\0", EthernetIP[0], EthernetIP[1], EthernetIP[2], EthernetIP[3]);
	if(strlen(ip_tmp) < size)
		sprintf(ip, "%s\0", ip_tmp);
	else
		memcpy(ip, &ip_tmp[0], size);
}
void Get_DataBin_EthernetMask(char *mask, int size){
	char mask_tmp[64];
	sprintf(mask_tmp, "%d.%d.%d.%d\0", EthernetMask[0], EthernetMask[1], EthernetMask[2], EthernetMask[3]);
	if(strlen(mask_tmp) < size)
		sprintf(mask, "%s\0", mask_tmp);
	else
		memcpy(mask, &mask_tmp[0], size);
}
void Get_DataBin_EthernetGateWay(char *gateway, int size){
	char gateway_tmp[64];
	sprintf(gateway_tmp, "%d.%d.%d.%d\0", EthernetGateWay[0], EthernetGateWay[1], EthernetGateWay[2], EthernetGateWay[3]);
	if(strlen(gateway_tmp) < size)
		sprintf(gateway, "%s\0", gateway_tmp);
	else
		memcpy(gateway, &gateway_tmp[0], size);
}
void Get_DataBin_EthernetDNS(char *dns, int size){
	char dns_tmp[64];
	sprintf(dns_tmp, "%d.%d.%d.%d\0", EthernetDNS[0], EthernetDNS[1], EthernetDNS[2], EthernetDNS[3]);
	if(strlen(dns_tmp) < size)
		sprintf(dns, "%s\0", dns_tmp);
	else
		memcpy(dns, &dns_tmp[0], size);
}
int Get_DataBin_MediaPort(){
	return MediaPort;
}
int Get_DataBin_DrivingRecord(){
	return DrivingRecord;
}
void Get_DataBin_US360Version(char *ver, int size){
	int i;
	int sz = sizeof(US360Versin);
	for(i = 0; i < sz; i++) {
		if(i == (size-1) ) {
			ver[i] = '\0';
			break;
		}
		if(US360Versin[i] == '\n' || US360Versin[i] == '\r')
			ver[i] = '\0';
		else
			ver[i] = US360Versin[i];
	}
}
int Get_DataBin_WifiChannel(){
	return WifiChannel;
}
int Get_DataBin_ExposureFreq(){
	return ExposureFreq;
}
int Get_DataBin_FanControl(){
	return FanControl;
}
int Get_DataBin_Sharpness(){
	return Sharpness;
}
int Get_DataBin_UserCtrl30Fps(){
	return UserCtrl30Fps;
}
int Get_DataBin_CameraMode(){
	return CameraMode;
}
int Get_DataBin_ColorSTMode(){
	return ColorSTMode;
}
int Get_DataBin_AutoGlobalPhiAdjMode(){
	return AutoGlobalPhiAdjMode;
}
int Get_DataBin_HDMITextVisibility(){
	return HDMITextVisibility;
}
int Get_DataBin_SpeakerMode(){
	return speakerMode;
}
int Get_DataBin_LedBrightness(){
	return ledBrightness;
}
int Get_DataBin_OledControl(){
	return oledControl;
}
int Get_DataBin_DelayValue(){
	return delayValue;
}
int Get_DataBin_ImageQuality(){
	return imageQuality;
}
int Get_DataBin_PhotographReso(){
	return photographReso;
}
int Get_DataBin_RecordReso(){
	return recordReso;
}
int Get_DataBin_TimeLapseReso(){
	return timeLapseReso;
}
int Get_DataBin_Translucent(){
	return translucent;
}
int Get_DataBin_CompassMaxx(){
	return compassMaxx;
}
int Get_DataBin_CompassMaxy(){
	return compassMaxy;
}
int Get_DataBin_CompassMaxz(){
	return compassMaxz;
}
int Get_DataBin_CompassMinx(){
	return compassMinx;
}
int Get_DataBin_CompassMiny(){
	return compassMiny;
}
int Get_DataBin_CompassMinz(){
	return compassMinz;
}
int Get_DataBin_DebugLogMode(){
	return debugLogMode;
}
int Get_DataBin_BottomMode(){
	return bottomMode;
}
int Get_DataBin_BottomSize(){
	return bottomSize;
}
int Get_DataBin_hdrEvMode(){
	return hdrEvMode;
}
int Get_DataBin_Bitrate(){
	return Bitrate;
}
void Get_DataBin_HttpAccount(char *account, int size){
	char def_acc[32] = "admin\0";
	if(strlen(HttpAccount) < size)
		sprintf(account, "%s\0", &HttpAccount[0]);
	else
		memcpy(account, &HttpAccount[0], size);
	if(strcmp(account, "") == 0) {
		if(strlen(def_acc) < size)
			sprintf(account, "%s\0", def_acc);
		else
			memcpy(account, &def_acc[0], size);
		Set_DataBin_HttpAccount(account);
	}
}
void Get_DataBin_HttpPassword(char *pwd, int size){
	char def_pwd[32] = "admin\0";
	if(strlen(HttpPassword) < size)
		sprintf(pwd, "%s\0", &HttpPassword[0]);
	else
		memcpy(pwd, &HttpPassword[0], size);
	if(strcmp(pwd, "") == 0) {
		if(strlen(def_pwd) < size)
			sprintf(pwd, "%s\0", def_pwd);
		else
			memcpy(pwd, &def_pwd[0], size);
		Set_DataBin_HttpPassword(pwd);
	}
}
int Get_DataBin_HttpPort(){
	return HttpPort;
}
int Get_DataBin_CompassMode(){
	return compassMode;
}
int Get_DataBin_GsensorMode(){
	return gsensorMode;
}
int Get_DataBin_CapHdrMode(){
	return capHdrMode;
}
int Get_DataBin_BottomTMode(){
	return bottomTMode;
}
int Get_DataBin_BottomTColor(){
	return bottomTColor;
}
int Get_DataBin_BottomBColor(){
	return bottomBColor;
}
int Get_DataBin_BottomTFont(){
	return bottomTFont;
}
int Get_DataBin_BottomTLoop(){
	return bottomTLoop;
}
void Get_DataBin_BottomText(char *text, int size){
	if(strlen(bottomText) < size)
		sprintf(text, "%s\0", &bottomText[0]);
	else
		memcpy(text, &bottomText[0], size);
}
int Get_DataBin_FpgaEncodeType(){
	return fpgaEncodeType;
}
void Get_DataBin_WbRGB(char *rgb, int size){
	char rgb_tmp[16];
	sprintf(rgb_tmp, "%d.%d.%d\0", wbRGB[0], wbRGB[1], wbRGB[2]);
	if(strlen(rgb_tmp) < size)
		sprintf(rgb, "%s\0", rgb_tmp);
	else
		memcpy(rgb, &rgb_tmp[0], size);
}
int Get_DataBin_Contrast(){
	return contrast;
}
int Get_DataBin_Saturation(){
	return saturation;
}
int Get_DataBin_FreeCount(){
	return freeCount;
}
int Get_DataBin_SaveTimelapse(){
	return saveTimelapse;
}
int Get_DataBin_BmodeSec(){
	return bmodeSec;
}
int Get_DataBin_BmodeGain(){
	return bmodeGain;
}
int Get_DataBin_HdrManual(){
	return hdrManual;
}
int Get_DataBin_HdrNumber(){
	return hdrNumber;
}
int Get_DataBin_HdrIncrement(){
	return hdrIncrement;
}
int Get_DataBin_HdrStrength(){
	return hdrStrength;
}
int Get_DataBin_HdrTone(){
	return hdrTone;
}
int Get_DataBin_AebNumber(){
	return aebNumber;
}
int Get_DataBin_AebIncrement(){
	return aebIncrement;
}
int Get_DataBin_LiveQualityMode(){
	return LiveQualityMode;
}
int Get_DataBin_WbTemperature(){
	return wbTemperature;
}
int Get_DataBin_HdrDeghosting(){
	return hdrDeghosting;
}
int Get_DataBin_RemoveHdrMode(){
	return removeHdrMode;
}
int Get_DataBin_RemoveHdrNumber(){
	return removeHdrNumber;
}
int Get_DataBin_RemoveHdrIncrement(){
	return removeHdrIncrement;
}
int Get_DataBin_RemoveHdrStrength(){
	return removeHdrStrength;
}
int Get_DataBin_AntiAliasingEn(){
	return AntiAliasingEn;
}
int Get_DataBin_RemoveAntiAliasingEn(){
	return removeAntiAliasingEn;
}
int Get_DataBin_WbTint(){
	return wbTint;
}
int Get_DataBin_HdrAutoStrength(){
	return hdrAutoStrength;
}
int Get_DataBin_RemoveHdrAutoStrength(){
	return removeHdrAutoStrength;
}
int Get_DataBin_LiveBitrate(){
	return LiveBitrate;
}
int Get_DataBin_PowerSaving(){
	return PowerSaving;
}

/*int getBytes(char *buf) {
	char tmp_buf[totalLength];
	char *ptr;
	int cnt = 0;

	memset(&tmp_buf[0], 0, sizeof(tmp_buf) );
	ptr = &tmp_buf[0];
	cnt = 0;

	memcpy(ptr, &Version, sizeof(Version) );
	ptr += sizeof(Version);
	cnt += sizeof(Version);

	memcpy(ptr, &DemoMode, sizeof(DemoMode) );
	ptr += sizeof(DemoMode);
	cnt += sizeof(DemoMode);

	memcpy(ptr, &CamPositionMode, sizeof(CamPositionMode) );
	ptr += sizeof(CamPositionMode);
	cnt += sizeof(CamPositionMode);

	memcpy(ptr, &PlayMode, sizeof(PlayMode) );
	ptr += sizeof(PlayMode);
	cnt += sizeof(PlayMode);

	memcpy(ptr, &ResoultMode, sizeof(ResoultMode) );
	ptr += sizeof(ResoultMode);
	cnt += sizeof(ResoultMode);

	memcpy(ptr, &EVValue, sizeof(EVValue) );
	ptr += sizeof(EVValue);
	cnt += sizeof(EVValue);

	memcpy(ptr, &MFValue, sizeof(MFValue) );
	ptr += sizeof(MFValue);
	cnt += sizeof(MFValue);

	memcpy(ptr, &MF2Value, sizeof(MF2Value) );
	ptr += sizeof(MF2Value);
	cnt += sizeof(MF2Value);

	memcpy(ptr, &ISOValue, sizeof(ISOValue) );
	ptr += sizeof(ISOValue);
	cnt += sizeof(ISOValue);

	memcpy(ptr, &ExposureValue, sizeof(ExposureValue) );
	ptr += sizeof(ExposureValue);
	cnt += sizeof(ExposureValue);

	memcpy(ptr, &WBMode, sizeof(WBMode) );
	ptr += sizeof(WBMode);
	cnt += sizeof(WBMode);

	memcpy(ptr, &CaptureMode, sizeof(CaptureMode) );
	ptr += sizeof(CaptureMode);
	cnt += sizeof(CaptureMode);

	memcpy(ptr, &CaptureCnt, sizeof(CaptureCnt) );
	ptr += sizeof(CaptureCnt);
	cnt += sizeof(CaptureCnt);

	memcpy(ptr, &CaptureSpaceTime, sizeof(CaptureSpaceTime) );
	ptr += sizeof(CaptureSpaceTime);
	cnt += sizeof(CaptureSpaceTime);

	memcpy(ptr, &SelfTimer, sizeof(SelfTimer) );
	ptr += sizeof(SelfTimer);
	cnt += sizeof(SelfTimer);

	memcpy(ptr, &TimeLapseMode, sizeof(TimeLapseMode) );
	ptr += sizeof(TimeLapseMode);
	cnt += sizeof(TimeLapseMode);

	memcpy(ptr, &SaveToSel, sizeof(SaveToSel) );
	ptr += sizeof(SaveToSel);
	cnt += sizeof(SaveToSel);

	memcpy(ptr, &WifiDisableTime, sizeof(WifiDisableTime) );
	ptr += sizeof(WifiDisableTime);
	cnt += sizeof(WifiDisableTime);

	memcpy(ptr, &EthernetMode, sizeof(EthernetMode) );
	ptr += sizeof(EthernetMode);
	cnt += sizeof(EthernetMode);

	memcpy(ptr, &EthernetIP[0], sizeof(EthernetIP) );
	ptr += sizeof(EthernetIP);
	cnt += sizeof(EthernetIP);

	memcpy(ptr, &EthernetMask[0], sizeof(EthernetMask) );
	ptr += sizeof(EthernetMask);
	cnt += sizeof(EthernetMask);

	memcpy(ptr, &EthernetGateWay[0], sizeof(EthernetGateWay) );
	ptr += sizeof(EthernetGateWay);
	cnt += sizeof(EthernetGateWay);

	memcpy(ptr, &EthernetDNS[0], sizeof(EthernetDNS) );
	ptr += sizeof(EthernetDNS);
	cnt += sizeof(EthernetDNS);

	memcpy(ptr, &MediaPort, sizeof(MediaPort) );
	ptr += sizeof(MediaPort);
	cnt += sizeof(MediaPort);

	memcpy(ptr, &DrivingRecord, sizeof(DrivingRecord) );
	ptr += sizeof(DrivingRecord);
	cnt += sizeof(DrivingRecord);

	memcpy(ptr, &US360Versin[0], sizeof(US360Versin) );
	ptr += sizeof(US360Versin);
	cnt += sizeof(US360Versin);

	memcpy(ptr, &WifiChannel, sizeof(WifiChannel) );
	ptr += sizeof(WifiChannel);
	cnt += sizeof(WifiChannel);

	memcpy(ptr, &ExposureFreq, sizeof(ExposureFreq) );
	ptr += sizeof(ExposureFreq);
	cnt += sizeof(ExposureFreq);

	memcpy(ptr, &FanControl, sizeof(FanControl) );
	ptr += sizeof(FanControl);
	cnt += sizeof(FanControl);

	memcpy(ptr, &Sharpness, sizeof(Sharpness) );
	ptr += sizeof(Sharpness);
	cnt += sizeof(Sharpness);

	memcpy(ptr, &UserCtrl30Fps, sizeof(UserCtrl30Fps) );
	ptr += sizeof(UserCtrl30Fps);
	cnt += sizeof(UserCtrl30Fps);

	memcpy(ptr, &CameraMode, sizeof(CameraMode) );
	ptr += sizeof(CameraMode);
	cnt += sizeof(CameraMode);

	memcpy(ptr, &ColorSTMode, sizeof(ColorSTMode) );
	ptr += sizeof(ColorSTMode);
	cnt += sizeof(ColorSTMode);

	memcpy(ptr, &AutoGlobalPhiAdjMode, sizeof(AutoGlobalPhiAdjMode) );
	ptr += sizeof(AutoGlobalPhiAdjMode);
	cnt += sizeof(AutoGlobalPhiAdjMode);

	memcpy(ptr, &HDMITextVisibility, sizeof(HDMITextVisibility) );
	ptr += sizeof(HDMITextVisibility);
	cnt += sizeof(HDMITextVisibility);

	memcpy(ptr, &speakerMode, sizeof(speakerMode) );
	ptr += sizeof(speakerMode);
	cnt += sizeof(speakerMode);

	memcpy(ptr, &ledBrightness, sizeof(ledBrightness) );
	ptr += sizeof(ledBrightness);
	cnt += sizeof(ledBrightness);

	memcpy(ptr, &oledControl, sizeof(oledControl) );
	ptr += sizeof(oledControl);
	cnt += sizeof(oledControl);

	memcpy(ptr, &delayValue, sizeof(delayValue) );
	ptr += sizeof(delayValue);
	cnt += sizeof(delayValue);

	memcpy(ptr, &imageQuality, sizeof(imageQuality) );
	ptr += sizeof(imageQuality);
	cnt += sizeof(imageQuality);

	memcpy(ptr, &photographReso, sizeof(photographReso) );
	ptr += sizeof(photographReso);
	cnt += sizeof(photographReso);

	memcpy(ptr, &recordReso, sizeof(recordReso) );
	ptr += sizeof(recordReso);
	cnt += sizeof(recordReso);

	memcpy(ptr, &timeLapseReso, sizeof(timeLapseReso) );
	ptr += sizeof(timeLapseReso);
	cnt += sizeof(timeLapseReso);

	memcpy(ptr, &translucent, sizeof(translucent) );
	ptr += sizeof(translucent);
	cnt += sizeof(translucent);

	memcpy(ptr, &compassMaxx, sizeof(compassMaxx) );
	ptr += sizeof(compassMaxx);
	cnt += sizeof(compassMaxx);

	memcpy(ptr, &compassMaxy, sizeof(compassMaxy) );
	ptr += sizeof(compassMaxy);
	cnt += sizeof(compassMaxy);

	memcpy(ptr, &compassMaxz, sizeof(compassMaxz) );
	ptr += sizeof(compassMaxz);
	cnt += sizeof(compassMaxz);

	memcpy(ptr, &compassMinx, sizeof(compassMinx) );
	ptr += sizeof(compassMinx);
	cnt += sizeof(compassMinx);

	memcpy(ptr, &compassMiny, sizeof(compassMiny) );
	ptr += sizeof(compassMiny);
	cnt += sizeof(compassMiny);

	memcpy(ptr, &compassMinz, sizeof(compassMinz) );
	ptr += sizeof(compassMinz);
	cnt += sizeof(compassMinz);

	memcpy(ptr, &debugLogMode, sizeof(debugLogMode) );
	ptr += sizeof(debugLogMode);
	cnt += sizeof(debugLogMode);

	memcpy(ptr, &bottomMode, sizeof(bottomMode) );
	ptr += sizeof(bottomMode);
	cnt += sizeof(bottomMode);

	memcpy(ptr, &bottomSize, sizeof(bottomSize) );
	ptr += sizeof(bottomSize);
	cnt += sizeof(bottomSize);

	memcpy(ptr, &hdrEvMode, sizeof(hdrEvMode) );
	ptr += sizeof(hdrEvMode);
	cnt += sizeof(hdrEvMode);

	memcpy(ptr, &Bitrate, sizeof(Bitrate) );
	ptr += sizeof(Bitrate);
	cnt += sizeof(Bitrate);

	memcpy(ptr, &HttpAccount[0], sizeof(HttpAccount) );
	ptr += sizeof(HttpAccount);
	cnt += sizeof(HttpAccount);

	memcpy(ptr, &HttpPassword[0], sizeof(HttpPassword) );
	ptr += sizeof(HttpPassword);
	cnt += sizeof(HttpPassword);

	memcpy(ptr, &HttpPort, sizeof(HttpPort) );
	ptr += sizeof(HttpPort);
	cnt += sizeof(HttpPort);

	memcpy(ptr, &compassMode, sizeof(compassMode) );
	ptr += sizeof(compassMode);
	cnt += sizeof(compassMode);

	memcpy(ptr, &gsensorMode, sizeof(gsensorMode) );
	ptr += sizeof(gsensorMode);
	cnt += sizeof(gsensorMode);

	memcpy(ptr, &capHdrMode, sizeof(capHdrMode) );
	ptr += sizeof(capHdrMode);
	cnt += sizeof(capHdrMode);

	memcpy(ptr, &bottomTMode, sizeof(bottomTMode) );
	ptr += sizeof(bottomTMode);
	cnt += sizeof(bottomTMode);

	memcpy(ptr, &bottomTColor, sizeof(bottomTColor) );
	ptr += sizeof(bottomTColor);
	cnt += sizeof(bottomTColor);

	memcpy(ptr, &bottomBColor, sizeof(bottomBColor) );
	ptr += sizeof(bottomBColor);
	cnt += sizeof(bottomBColor);

	memcpy(ptr, &bottomTFont, sizeof(bottomTFont) );
	ptr += sizeof(bottomTFont);
	cnt += sizeof(bottomTFont);

	memcpy(ptr, &bottomTLoop, sizeof(bottomTLoop) );
	ptr += sizeof(bottomTLoop);
	cnt += sizeof(bottomTLoop);

	memcpy(ptr, &bottomText[0], sizeof(bottomText) );
	ptr += sizeof(bottomText);
	cnt += sizeof(bottomText);

	memcpy(ptr, &fpgaEncodeType, sizeof(fpgaEncodeType) );
	ptr += sizeof(fpgaEncodeType);
	cnt += sizeof(fpgaEncodeType);

	memcpy(ptr, &wbRGB[0], sizeof(wbRGB) );
	ptr += sizeof(wbRGB);
	cnt += sizeof(wbRGB);

	memcpy(ptr, &contrast, sizeof(contrast) );
	ptr += sizeof(contrast);
	cnt += sizeof(contrast);

	memcpy(ptr, &saturation, sizeof(saturation) );
	ptr += sizeof(saturation);
	cnt += sizeof(saturation);

	memcpy(ptr, &freeCount, sizeof(freeCount) );
	ptr += sizeof(freeCount);
	cnt += sizeof(freeCount);

	memcpy(ptr, &saveTimelapse, sizeof(saveTimelapse) );
	ptr += sizeof(saveTimelapse);
	cnt += sizeof(saveTimelapse);

	memcpy(ptr, &bmodeSec, sizeof(bmodeSec) );
	ptr += sizeof(bmodeSec);
	cnt += sizeof(bmodeSec);

	memcpy(ptr, &bmodeGain, sizeof(bmodeGain) );
	ptr += sizeof(bmodeGain);
	cnt += sizeof(bmodeGain);

	memcpy(ptr, &hdrManual, sizeof(hdrManual) );
	ptr += sizeof(hdrManual);
	cnt += sizeof(hdrManual);

	memcpy(ptr, &hdrNumber, sizeof(hdrNumber) );
	ptr += sizeof(hdrNumber);
	cnt += sizeof(hdrNumber);

	memcpy(ptr, &hdrIncrement, sizeof(hdrIncrement) );
	ptr += sizeof(hdrIncrement);
	cnt += sizeof(hdrIncrement);

	memcpy(ptr, &hdrStrength, sizeof(hdrStrength) );
	ptr += sizeof(hdrStrength);
	cnt += sizeof(hdrStrength);

	memcpy(ptr, &hdrTone, sizeof(hdrTone) );
	ptr += sizeof(hdrTone);
	cnt += sizeof(hdrTone);

	memcpy(ptr, &aebNumber, sizeof(aebNumber) );
	ptr += sizeof(aebNumber);
	cnt += sizeof(aebNumber);

	memcpy(ptr, &aebIncrement, sizeof(aebIncrement) );
	ptr += sizeof(aebIncrement);
	cnt += sizeof(aebIncrement);

	memcpy(ptr, &LiveQualityMode, sizeof(LiveQualityMode) );
	ptr += sizeof(LiveQualityMode);
	cnt += sizeof(LiveQualityMode);

	memcpy(ptr, &wbTemperature, sizeof(wbTemperature) );
	ptr += sizeof(wbTemperature);
	cnt += sizeof(wbTemperature);

	memcpy(ptr, &hdrDeghosting, sizeof(hdrDeghosting) );
	ptr += sizeof(hdrDeghosting);
	cnt += sizeof(hdrDeghosting);

	memcpy(ptr, &removeHdrMode, sizeof(removeHdrMode) );
	ptr += sizeof(removeHdrMode);
	cnt += sizeof(removeHdrMode);

	memcpy(ptr, &removeHdrNumber, sizeof(removeHdrNumber) );
	ptr += sizeof(removeHdrNumber);
	cnt += sizeof(removeHdrNumber);

	memcpy(ptr, &removeHdrIncrement, sizeof(removeHdrIncrement) );
	ptr += sizeof(removeHdrIncrement);
	cnt += sizeof(removeHdrIncrement);

	memcpy(ptr, &removeHdrStrength, sizeof(removeHdrStrength) );
	ptr += sizeof(removeHdrStrength);
	cnt += sizeof(removeHdrStrength);

	memcpy(ptr, &AntiAliasingEn, sizeof(AntiAliasingEn) );
	ptr += sizeof(AntiAliasingEn);
	cnt += sizeof(AntiAliasingEn);

	memcpy(ptr, &removeAntiAliasingEn, sizeof(removeAntiAliasingEn) );
	ptr += sizeof(removeAntiAliasingEn);
	cnt += sizeof(removeAntiAliasingEn);

	memcpy(ptr, &wbTint, sizeof(wbTint) );
	ptr += sizeof(wbTint);
	cnt += sizeof(wbTint);

	memcpy(ptr, &hdrAutoStrength, sizeof(hdrAutoStrength) );
	ptr += sizeof(hdrAutoStrength);
	cnt += sizeof(hdrAutoStrength);

	memcpy(ptr, &removeHdrAutoStrength, sizeof(removeHdrAutoStrength) );
	ptr += sizeof(removeHdrAutoStrength);
	cnt += sizeof(removeHdrAutoStrength);

	memcpy(ptr, &LiveBitrate, sizeof(LiveBitrate) );
	ptr += sizeof(LiveBitrate);
	cnt += sizeof(LiveBitrate);

	memcpy(ptr, &PowerSaving, sizeof(PowerSaving) );
	ptr += sizeof(PowerSaving);
	cnt += sizeof(PowerSaving);

	memcpy(buf, &tmp_buf[0], sizeof(tmp_buf) );
	return cnt;
}*/
int getBytes(char *buf) {
	char tmp_buf[totalLength];
	char *ptr;
	int i, cnt = 0, tmp;

	memset(&tmp_buf[0], 0, sizeof(tmp_buf) );
	ptr = &tmp_buf[0];
	cnt = 0;

	tmp = SWAP32(Version);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(DemoMode);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(CamPositionMode);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(PlayMode);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(ResoultMode);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(EVValue);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(MFValue);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(MF2Value);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(ISOValue);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(ExposureValue);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(WBMode);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(CaptureMode);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(CaptureCnt);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(CaptureSpaceTime);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(SelfTimer);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(TimeLapseMode);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(SaveToSel);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(WifiDisableTime);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(EthernetMode);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	for(i = 0; i < 4; i++) {
		*ptr = EthernetIP[3-i];
		ptr++;
		cnt++;
	}

	for(i = 0; i < 4; i++) {
		*ptr = EthernetMask[3-i];
		ptr++;
		cnt++;
	}

	for(i = 0; i < 4; i++) {
		*ptr = EthernetGateWay[3-i];
		ptr++;
		cnt++;
	}

	for(i = 0; i < 4; i++) {
		*ptr = EthernetDNS[3-i];
		ptr++;
		cnt++;
	}

	tmp = SWAP32(MediaPort);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(DrivingRecord);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	for(i = 0; i < 16; i++) {
		*ptr = US360Versin[15-i];
		ptr++;
		cnt++;
	}

	tmp = SWAP32(WifiChannel);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(ExposureFreq);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(FanControl);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(Sharpness);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(UserCtrl30Fps);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(CameraMode);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(ColorSTMode);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(AutoGlobalPhiAdjMode);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(HDMITextVisibility);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(speakerMode);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(ledBrightness);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(oledControl);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(delayValue);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(imageQuality);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(photographReso);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(recordReso);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(timeLapseReso);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(translucent);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(compassMaxx);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(compassMaxy);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(compassMaxz);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(compassMinx);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(compassMiny);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(compassMinz);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(debugLogMode);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(bottomMode);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(bottomSize);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(hdrEvMode);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(Bitrate);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	for(i = 0; i < 32; i++) {
		*ptr = HttpAccount[31-i];
		ptr++;
		cnt++;
	}

	for(i = 0; i < 32; i++) {
		*ptr = HttpPassword[31-i];
		ptr++;
		cnt++;
	}

	tmp = SWAP32(HttpPort);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(compassMode);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(gsensorMode);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(capHdrMode);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(bottomTMode);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(bottomTColor);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(bottomBColor);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(bottomTFont);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(bottomTLoop);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	for(i = 0; i < 70; i++) {
		*ptr = bottomText[69-i];
		ptr++;
		cnt++;
	}

	tmp = SWAP32(fpgaEncodeType);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	for(i = 0; i < 4; i++) {
		*ptr = wbRGB[3-i];
		ptr++;
		cnt++;
	}

	tmp = SWAP32(contrast);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(saturation);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(freeCount);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(saveTimelapse);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(bmodeSec);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(bmodeGain);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(hdrManual);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(hdrNumber);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(hdrIncrement);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(hdrStrength);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(hdrTone);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(aebNumber);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(aebIncrement);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(LiveQualityMode);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(wbTemperature);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(hdrDeghosting);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(removeHdrMode);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(removeHdrNumber);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(removeHdrIncrement);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(removeHdrStrength);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(AntiAliasingEn);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(removeAntiAliasingEn);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(wbTint);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(hdrAutoStrength);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(removeHdrAutoStrength);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(LiveBitrate);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	tmp = SWAP32(PowerSaving);
	memcpy(ptr, &tmp, sizeof(tmp) );
	ptr += sizeof(tmp);
	cnt += sizeof(tmp);

	memcpy(buf, &tmp_buf[0], sizeof(tmp_buf) );
	return cnt;
}

/*	public void Bytes2Data(byte[] b){
		ByteBuffer buf = ByteBuffer.wrap(b);
		int pos = buf.position();
		int i=0;
		int bLen = b.length;

		this.Version = buf.getInt(pos);
		Log.d(TAG, "get Version = " + this.Version);
		pos +=4;

		this.DemoMode = buf.getInt(pos);
		Log.d(TAG, "get DemoMode = " + this.DemoMode);
		pos +=4;

		this.CamPositionMode = buf.getInt(pos);
		Log.d(TAG, "get CamPositionMode = " + this.CamPositionMode);
		pos +=4;

		this.PlayMode = buf.getInt(pos);
		Log.d(TAG, "get PlayMode = " + this.PlayMode);
		pos +=4;

		this.ResoultMode = buf.getInt(pos);
		Log.d(TAG, "get ResoultMode = " + this.ResoultMode);
		pos +=4;

		this.EVValue = buf.getInt(pos);
		Log.d(TAG, "get EVValue = " + this.EVValue);
		pos +=4;

		this.MFValue = buf.getInt(pos);
		Log.d(TAG, "get MFValue = " + this.MFValue);
		pos +=4;

		this.MF2Value = buf.getInt(pos);
		Log.d(TAG, "get MF2Value = " + this.MF2Value);
		pos +=4;

		this.ISOValue = buf.getInt(pos);
		Log.d(TAG, "get ISOValue = " + this.ISOValue);
		pos +=4;

		this.ExposureValue = buf.getInt(pos);
		Log.d(TAG, "get ExposureValue = " + this.ExposureValue);
		pos +=4;

		this.WBMode = buf.getInt(pos);
		Log.d(TAG, "get WBMode = " + this.WBMode);
		pos +=4;

		this.CaptureMode = buf.getInt(pos);
		Log.d(TAG, "get CaptureMode = " + this.CaptureMode);
		pos +=4;

		this.CaptureCnt = buf.getInt(pos);
		Log.d(TAG, "get CaptureCnt = " + this.CaptureCnt);
		pos +=4;

		this.CaptureSpaceTime = buf.getInt(pos);
		Log.d(TAG, "get CaptureSpaceTime = " + this.CaptureSpaceTime);
		pos +=4;

		this.SelfTimer = buf.getInt(pos);
		Log.d(TAG, "get SelfTimer = " + this.SelfTimer);
		pos +=4;

		this.TimeLapseMode = buf.getInt(pos);
		Log.d(TAG, "get TimeLapseMode = " + this.TimeLapseMode);
		pos +=4;

		this.SaveToSel = buf.getInt(pos);
		Log.d(TAG, "get SaveToSel = " + this.SaveToSel);
		pos +=4;

		this.WifiDisableTime = buf.getInt(pos);
		Log.d(TAG, "get WifiDisableTime = " + this.WifiDisableTime);
		pos +=4;

		this.EthernetMode = buf.getInt(pos);
		Log.d(TAG, "get EthernetMode = " + this.EthernetMode);
		pos +=4;

		for(i=0;i<4;i++){
			this.EthernetIP[i] = buf.get(pos);
			pos++;
		}
		Log.d(TAG, "get EthernetIP = " + getEthernetIP());

		for(i=0;i<4;i++){
			this.EthernetMask[i] = buf.get(pos);
			pos++;
		}
		Log.d(TAG, "get EthernetMask = " + getEthernetMask());

		for(i=0;i<4;i++){
			this.EthernetGateWay[i] = buf.get(pos);
			pos++;
		}
		Log.d(TAG, "get EthernetGateWay = " + getEthernetGateWay());

		for(i=0;i<4;i++){
			this.EthernetDNS[i] = buf.get(pos);
			pos++;
		}
		Log.d(TAG, "get EthernetDNS = " + getEthernetDNS());

		setMediaPort(buf.getInt(pos));
		pos += 4;
		Log.d(TAG, "get MediaPort = " + this.MediaPort);

		setDrivingRecord(buf.getInt(pos));
		pos += 4;
		Log.d(TAG, "get DrivingRecord = " + this.DrivingRecord);

		for(i=0;i<16;i++){
			this.US360Versin[i] = buf.get(pos);
			pos++;
		}
		Log.d(TAG, "get US360Version = " + getUS360Version());

		setWifiChannel(buf.getInt(pos));
		Log.d(TAG, "get WifiChannel = " + this.WifiChannel);
		pos +=4;


		if(pos >= bLen)	return;
		//if(bLen > pos){		// 為以後防錯, 120 Bytes -> 160 Bytes
			setExposureFreq(buf.getInt(pos));
			Log.d(TAG, "get ExposureFreq = " + this.ExposureFreq);
			pos +=4;
		//}

		if(pos >= bLen)	return;
		setFanControl(buf.getInt(pos));
		Log.d(TAG, "get FanControl = " + this.FanControl);
		pos +=4;

		if(pos >= bLen) return;
		setSharpness(buf.getInt(pos));
		Log.d(TAG, "get Sharpness = " + this.Sharpness);
		pos += 4;

		if(pos >= bLen) return;
		setUserCtrl30Fps(buf.getInt(pos));
		Log.d(TAG, "get UserCtrl30Fps = " + this.UserCtrl30Fps);
		pos += 4;

		if(pos >= bLen) return;
		this.CameraMode = buf.getInt(pos);
		Log.d(TAG, "get CameraMode = " + this.CameraMode);
		pos +=4;

		if(pos >= bLen) return;
		this.ColorSTMode = buf.getInt(pos);
		Log.d(TAG, "get ColorSTMode = " + this.ColorSTMode);
		pos +=4;

		if(pos >= bLen) return;
		this.AutoGlobalPhiAdjMode = buf.getInt(pos);
		Log.d(TAG, "get AutoGlobalPhiAdjMode = " + this.AutoGlobalPhiAdjMode);
		pos +=4;

		if(pos >= bLen) return;
		this.HDMITextVisibility = buf.getInt(pos);
		Log.d(TAG, "get HDMITextVisibility = " + this.HDMITextVisibility);
		pos +=4;

		if(pos >= bLen) return;
		this.speakerMode = buf.getInt(pos);
		Log.d(TAG, "get speakerMode = " + this.speakerMode);
		pos +=4;

		if(pos >= bLen) return;
		this.ledBrightness = buf.getInt(pos);
		Log.d(TAG, "get ledBrightness = " + this.ledBrightness);
		pos +=4;

		if(pos >= bLen) return;
		this.oledControl = buf.getInt(pos);
		Log.d(TAG, "get oledControl = " + this.oledControl);
		pos +=4;

		if(pos >= bLen) return;
		this.delayValue = buf.getInt(pos);
		Log.d(TAG, "get delayValue = " + this.delayValue);
		pos +=4;

		if(pos >= bLen) return;
		this.imageQuality = buf.getInt(pos);
		Log.d(TAG, "get imageQuality = " + this.imageQuality);
		pos +=4;

		if(pos >= bLen) return;
		this.photographReso = buf.getInt(pos);
		Log.d(TAG, "get photographReso = " + this.photographReso);
		pos +=4;

		if(pos >= bLen) return;
		this.recordReso = buf.getInt(pos);
		Log.d(TAG, "get recordReso = " + this.recordReso);
		pos +=4;

		if(pos >= bLen) return;
		this.timeLapseReso = buf.getInt(pos);
		Log.d(TAG, "get timeLapseReso = " + this.timeLapseReso);
		pos +=4;

		if(pos >= bLen) return;
		this.translucent = buf.getInt(pos);
		Log.d(TAG, "get translucent = " + this.translucent);
		pos +=4;

		if(pos >= bLen) return;
		this.compassMaxx = buf.getInt(pos);
		Log.d(TAG, "get compassMaxx = " + this.compassMaxx);
		pos +=4;

		if(pos >= bLen) return;
		this.compassMaxy = buf.getInt(pos);
		Log.d(TAG, "get compassMaxy = " + this.compassMaxy);
		pos +=4;

		if(pos >= bLen) return;
		this.compassMaxz = buf.getInt(pos);
		Log.d(TAG, "get compassMaxz = " + this.compassMaxz);
		pos +=4;

		if(pos >= bLen) return;
		this.compassMinx = buf.getInt(pos);
		Log.d(TAG, "get compassMinx = " + this.compassMinx);
		pos +=4;

		if(pos >= bLen) return;
		this.compassMiny = buf.getInt(pos);
		Log.d(TAG, "get compassMiny = " + this.compassMiny);
		pos +=4;

		if(pos >= bLen) return;
		this.compassMinz = buf.getInt(pos);
		Log.d(TAG, "get compassMinz = " + this.compassMinz);
		pos +=4;

		if(pos >= bLen) return;
		this.debugLogMode = buf.getInt(pos);
		Log.d(TAG, "get debugLogMode = " + this.debugLogMode);
		pos +=4;

		if(pos >= bLen) return;
		this.bottomMode = buf.getInt(pos);
		Log.d(TAG, "get bottomMode = " + this.getBottomMode());
		pos +=4;

		if(pos >= bLen) return;
		this.bottomSize = buf.getInt(pos);
		Log.d(TAG, "get bottomSize = " + this.bottomSize);
		pos +=4;

		if(pos >= bLen) return;
		this.hdrEvMode = buf.getInt(pos);
		Log.d(TAG, "get hdrEvMode = " + this.hdrEvMode);
		pos +=4;

		if(pos >= bLen) return;
		this.Bitrate = buf.getInt(pos);
		Log.d(TAG, "get Bitrate = " + this.Bitrate);
		pos +=4;

		if(pos >= bLen) return;
		for(i=0;i<32;i++){
			this.HttpAccount[i] = buf.get(pos);
			pos++;
		}
		Log.d(TAG, "get HttpAccount = " + getHttpAccount());

		if(pos >= bLen) return;
		for(i=0;i<32;i++){
			this.HttpPassword[i] = buf.get(pos);
			pos++;
		}
		Log.d(TAG, "get HttpPassword = " + getHttpPassword());

		if(pos >= bLen) return;
		this.HttpPort = buf.getInt(pos);
		Log.d(TAG, "get HttpPort = " + this.HttpPort);
		pos +=4;

		if(pos >= bLen) return;
		this.compassMode = buf.getInt(pos);
		Log.d(TAG, "get compassMode = " + this.compassMode);
		pos +=4;

		if(pos >= bLen) return;
		this.gsensorMode = buf.getInt(pos);
		Log.d(TAG, "get gsensorMode = " + this.gsensorMode);
		pos +=4;

		if(pos >= bLen) return;
		this.capHdrMode = buf.getInt(pos);
		Log.d(TAG, "get capHdrMode = " + this.capHdrMode);
		pos +=4;

		if(pos >= bLen) return;
		this.bottomTMode = buf.getInt(pos);
		Log.d(TAG, "get bottomTMode = " + this.bottomTMode);
		pos +=4;

		if(pos >= bLen) return;
		this.bottomTColor = buf.getInt(pos);
		Log.d(TAG, "get bottomTColor = " + this.bottomTColor);
		pos +=4;

		if(pos >= bLen) return;
		this.bottomBColor = buf.getInt(pos);
		Log.d(TAG, "get bottomBColor = " + this.bottomBColor);
		pos +=4;

		if(pos >= bLen) return;
		this.bottomTFont = buf.getInt(pos);
		Log.d(TAG, "get bottomTFont = " + this.bottomTFont);
		pos +=4;

		if(pos >= bLen) return;
		this.bottomTLoop = buf.getInt(pos);
		Log.d(TAG, "get bottomTLoop = " + this.bottomTLoop);
		pos +=4;

		if(pos >= bLen) return;
		for(i=0;i<70;i++){
			this.bottomText[i] = buf.get(pos);
			pos++;
		}
		Log.d(TAG, "get bottomText = " + getBottomText());

		if(pos >= bLen) return;
		this.fpgaEncodeType = buf.getInt(pos);
		Log.d(TAG, "get fpgaEncodeType = " + this.fpgaEncodeType);
		pos +=4;

		if(pos >= bLen) return;
		for(i=0;i<4;i++){
			this.wbRGB[i] = buf.get(pos);
			pos++;
		}
		Log.d(TAG, "get wbRGB = " + getWbRGB());

		if(pos >= bLen) return;
		this.contrast = buf.getInt(pos);
		Log.d(TAG, "get contrast = " + this.contrast);
		pos +=4;

		if(pos >= bLen) return;
		this.saturation = buf.getInt(pos);
		Log.d(TAG, "get saturation = " + this.saturation);
		pos +=4;

		if(pos >= bLen) return;
		this.freeCount = buf.getInt(pos);
		Log.d(TAG, "get freeCount = " + this.freeCount);
		pos +=4;

		if(pos >= bLen) return;
		this.saveTimelapse = buf.getInt(pos);
		Log.d(TAG, "get saveTimelapse = " + this.saveTimelapse);
		pos +=4;

		if(pos >= bLen) return;
		this.bmodeSec = buf.getInt(pos);
		Log.d(TAG, "get bmodeSec = " + this.bmodeSec);
		pos +=4;

		if(pos >= bLen) return;
		this.bmodeGain = buf.getInt(pos);
		Log.d(TAG, "get bmodeGain = " + this.bmodeGain);
		pos +=4;

		if(pos >= bLen) return;
		this.hdrManual = buf.getInt(pos);
		Log.d(TAG, "get hdrManual = " + this.hdrManual);
		pos +=4;

		if(pos >= bLen) return;
		this.hdrNumber = buf.getInt(pos);
		Log.d(TAG, "get hdrNumber = " + this.hdrNumber);
		pos +=4;

		if(pos >= bLen) return;
		this.hdrIncrement = buf.getInt(pos);
		Log.d(TAG, "get hdrIncrement = " + this.hdrIncrement);
		pos +=4;

		if(pos >= bLen) return;
		this.hdrStrength = buf.getInt(pos);
		Log.d(TAG, "get hdrStrength = " + this.hdrStrength);
		pos +=4;

		if(pos >= bLen) return;
		this.hdrTone = buf.getInt(pos);
		Log.d(TAG, "get hdrTone = " + this.hdrTone);
		pos +=4;

		if(pos >= bLen) return;
		this.aebNumber = buf.getInt(pos);
		Log.d(TAG, "get aebNumber = " + this.aebNumber);
		pos +=4;

		if(pos >= bLen) return;
		this.aebIncrement = buf.getInt(pos);
		Log.d(TAG, "get aebIncrement = " + this.aebIncrement);
		pos +=4;

		if(pos >= bLen) return;
		this.LiveQualityMode = buf.getInt(pos);
		Log.d(TAG, "get LiveQualityMode = " + this.LiveQualityMode);
		pos +=4;

		if(pos >= bLen) return;
		this.wbTemperature = buf.getInt(pos);
		Log.d(TAG, "get wbTemperature = " + this.wbTemperature);
		pos +=4;

		if(pos >= bLen) return;
		this.hdrDeghosting = buf.getInt(pos);
		Log.d(TAG, "get hdrDeghosting = " + this.hdrDeghosting);
		pos +=4;

		if(pos >= bLen) return;
		this.removeHdrMode = buf.getInt(pos);
		Log.d(TAG, "get removeHdrMode = " + this.removeHdrMode);
		pos +=4;

		if(pos >= bLen) return;
		this.removeHdrNumber = buf.getInt(pos);
		Log.d(TAG, "get removeHdrNumber = " + this.removeHdrNumber);
		pos +=4;

		if(pos >= bLen) return;
		this.removeHdrIncrement = buf.getInt(pos);
		Log.d(TAG, "get removeHdrIncrement = " + this.removeHdrIncrement);
		pos +=4;

		if(pos >= bLen) return;
		this.removeHdrStrength = buf.getInt(pos);
		Log.d(TAG, "get removeHdrStrength = " + this.removeHdrStrength);
		pos +=4;

		if(pos >= bLen) return;
		this.AntiAliasingEn = buf.getInt(pos);
		Log.d(TAG, "get AntiAliasingEn = " + this.AntiAliasingEn);
		pos +=4;

		if(pos >= bLen) return;
		this.removeAntiAliasingEn = buf.getInt(pos);
		Log.d(TAG, "get removeAntiAliasingEn = " + this.removeAntiAliasingEn);
		pos +=4;

		if(pos >= bLen) return;
		this.wbTint = buf.getInt(pos);
		Log.d(TAG, "get wbTint = " + this.wbTint);
		pos +=4;

		if(pos >= bLen) return;
		this.hdrAutoStrength = buf.getInt(pos);
		Log.d(TAG, "get hdrAutoStrength = " + this.hdrAutoStrength);
		pos +=4;

		if(pos >= bLen) return;
		this.removeHdrAutoStrength = buf.getInt(pos);
		Log.d(TAG, "get removeHdrAutoStrength = " + this.removeHdrAutoStrength);
		pos +=4;

		if(pos >= bLen) return;
		this.LiveBitrate = buf.getInt(pos);
		Log.d(TAG, "get LiveBitrate = " + this.LiveBitrate);
		pos +=4;

		if(pos >= bLen) return;
		this.PowerSaving = buf.getInt(pos);
		Log.d(TAG, "get PowerSaving = " + this.PowerSaving);
		pos +=4;
	}*/

void setValue(char *value, int pos){
	int val = 0;
	//byte[] a;
	switch(pos){
		case 0:
    		val = atoi(value);
			Version 			= val;
			break;
		case 1:
    		val = atoi(value);
			DemoMode 			= val;
			break;
		case 2:
    		val = atoi(value);
			CamPositionMode 	= val;
			break;
		case 3:
	    	val = atoi(value);
			PlayMode 			= val;
			break;
		case 4:
    		val = atoi(value);
			ResoultMode 		= val;
			break;
		case 5:
    		val = atoi(value);
			EVValue 			= val;
			break;
		case 6:
    		val = atoi(value);
			MFValue 			= val;
			break;
		case 7:
    		val = atoi(value);
			MF2Value 			= val;
			break;
		case 8:
    		val = atoi(value);
			ISOValue			= val;
			break;
		case 9:
    		val = atoi(value);
			ExposureValue		= val;
			break;
		case 10:
    		val = atoi(value);
			WBMode				= val;
			break;
		case 11:
    		val = atoi(value);
			CaptureMode		= val;
			break;
		case 12:
    		val = atoi(value);
			CaptureCnt			= val;
			break;
		case 13:
    		val = atoi(value);
			CaptureSpaceTime	= val;
			break;
		case 14:
    		val = atoi(value);
			SelfTimer			= val;
			break;
		case 15:
    		val = atoi(value);
			TimeLapseMode		= val;
			break;
		case 16:
    		val = atoi(value);
			SaveToSel			= val;
			break;
		case 17:
    		val = atoi(value);
			WifiDisableTime	= val;
			break;
		case 18:
    		val = atoi(value);
			EthernetMode 		= val;
			break;
		case 19:
			Set_DataBin_EthernetIP(value);
			break;
		case 20:
			Set_DataBin_EthernetMask(value);
			break;
		case 21:
			Set_DataBin_EthernetGateWay(value);
			break;
		case 22:
			Set_DataBin_EthernetDNS(value);
			break;
		case 23:		// media port
    		val = atoi(value);
			MediaPort = val;
			break;
		case 24:		// DrivingRecord
    		val = atoi(value);
    		Set_DataBin_DrivingRecord(val);
			break;
		case 25:		// US360 Version
			Set_DataBin_US360Version(value);
			break;
		case 26:		// wifi channel
    		val = atoi(value);
    		Set_DataBin_WifiChannel(val);
			break;
		case 27:		//EP freq
			val = atoi(value);
			Set_DataBin_ExposureFreq(val);
			break;
		case 28:		//FanControl
			val = atoi(value);
			Set_DataBin_FanControl(val);
			break;
		case 29:		// Sharpness
			val = atoi(value);
			Set_DataBin_Sharpness(val);
			break;
		case 30:		// UserCtrl30Fps
			val = atoi(value);
			Set_DataBin_UserCtrl30Fps(val);
			break;
		case 31:
    		val = atoi(value);
			CameraMode		= val;
			break;
		case 32:
    		val = atoi(value);
			ColorSTMode		= val;
			break;
		case 33:
    		val = atoi(value);
			AutoGlobalPhiAdjMode = val;
			break;
		case 34:
			val = atoi(value);
			HDMITextVisibility = val;
			break;
		case 35:
			val = atoi(value);
			speakerMode = val;
			break;
		case 36:
			val = atoi(value);
			ledBrightness = val;
			break;
		case 37:
			val = atoi(value);
			oledControl = val;
			break;
		case 38:
			val = atoi(value);
			delayValue = val;
			break;
		case 39:
			val = atoi(value);
			imageQuality = val;
			break;
		case 40:
			val = atoi(value);
			photographReso = val;
			break;
		case 41:
			val = atoi(value);
			recordReso = val;
			break;
		case 42:
			val = atoi(value);
			timeLapseReso = val;
			break;
		case 43:
			val = atoi(value);
			translucent = val;
			break;
		case 44:
			val = atoi(value);
			compassMaxx = val;
			break;
		case 45:
			val = atoi(value);
			compassMaxy = val;
			break;
		case 46:
			val = atoi(value);
			compassMaxz = val;
			break;
		case 47:
			val = atoi(value);
			compassMinx = val;
			break;
		case 48:
			val = atoi(value);
			compassMiny = val;
			break;
		case 49:
			val = atoi(value);
			compassMinz = val;
			break;
		case 50:
			val = atoi(value);
			debugLogMode = val;
			break;
		case 51:
			val = atoi(value);
			bottomMode = val;
			break;
		case 52:
			val = atoi(value);
			bottomSize = val;
			break;
		case 53:
			val = atoi(value);
			hdrEvMode = val;
			break;
		case 54:
			val = atoi(value);
			Bitrate = val;
			break;
		case 55:
			Set_DataBin_HttpAccount(value);
			break;
		case 56:
			Set_DataBin_HttpPassword(value);
			break;
		case 57:
			val = atoi(value);
			HttpPort = val;
			break;
		case 58:
			val = atoi(value);
			compassMode = val;
			break;
		case 59:
			val = atoi(value);
			gsensorMode = val;
			break;
		case 60:
			val = atoi(value);
			capHdrMode = val;
			break;
		case 61:
			val = atoi(value);
			bottomTMode = val;
			break;
		case 62:
			val = atoi(value);
			bottomTColor = val;
			break;
		case 63:
			val = atoi(value);
			bottomBColor = val;
			break;
		case 64:
			val = atoi(value);
			bottomTFont = val;
			break;
		case 65:
			val = atoi(value);
			bottomTLoop = val;
			break;
		case 66:
			Set_DataBin_BottomText(value);
			break;
		case 67:
			val = atoi(value);
			fpgaEncodeType = val;
			break;
		case 68:
			Set_DataBin_WbRGB(value);
			break;
		case 69:
			val = atoi(value);
			contrast = val;
			break;
		case 70:
			val = atoi(value);
			saturation = val;
			break;
		case 71:
			val = atoi(value);
			freeCount = val;
			break;
		case 72:
			val = atoi(value);
			saveTimelapse = val;
			break;
		case 73:
			val = atoi(value);
			bmodeSec = val;
			break;
		case 74:
			val = atoi(value);
			bmodeGain = val;
			break;
		case 75:
			val = atoi(value);
			hdrManual = val;
			break;
		case 76:
			val = atoi(value);
			hdrNumber = val;
			break;
		case 77:
			val = atoi(value);
			hdrIncrement = val;
			break;
		case 78:
			val = atoi(value);
			hdrStrength = val;
			break;
		case 79:
			val = atoi(value);
			hdrTone = val;
			break;
		case 80:
			val = atoi(value);
			aebNumber = val;
			break;
		case 81:
			val = atoi(value);
			aebIncrement = val;
			break;
		case 82:
			val = atoi(value);
			LiveQualityMode = val;
			break;
		case 83:
			val = atoi(value);
			wbTemperature = val;
			break;
		case 84:
			val = atoi(value);
			hdrDeghosting = val;
			break;
		case 85:
			val = atoi(value);
			removeHdrMode = val;
			break;
		case 86:
			val = atoi(value);
			removeHdrNumber = val;
			break;
		case 87:
			val = atoi(value);
			removeHdrIncrement = val;
			break;
		case 88:
			val = atoi(value);
			removeHdrStrength = val;
			break;
		case 89:
			val = atoi(value);
			AntiAliasingEn = val;
			break;
		case 90:
			val = atoi(value);
			removeAntiAliasingEn = val;
			break;
		case 91:
			val = atoi(value);
			wbTint = val;
			break;
		case 92:
			val = atoi(value);
			hdrAutoStrength = val;
			break;
		case 93:
			val = atoi(value);
			removeHdrAutoStrength = val;
			break;
		case 94:
			val = atoi(value);
			LiveBitrate = val;
			break;
		case 95:
			val = atoi(value);
			PowerSaving = val;
			break;
	}
}
void writeDatabin2Path(char *filePath){
	char versionStr[64], 	    demoModeStr[64], 			 camPositionModeStr[64], playModeStr[64], 		   resoultModeStr[64], 			 evValueStr[64],
		 mfValueStr[64], 	    mf2ValueStr[64], 			 isoValueStr[64], 		 exposureValueStr[64],     wbModeStr[64], 				 captureModeStr[64],
		 captureCntStr[64],     captureSpaceTimeStr[64], 	 selfTimerStr[64], 		 timeLapseModeStr[64],     saveToSelStr[64], 			 wifiDisabletTimeStr[64],
		 ethernetModeStr[64],   ethernetIPStr[64], 			 ethernetMaskStr[64], 	 ethernetGateWayStr[64],   ethernetDNSStr[64], 			 mediaPortStr[64],
		 drivingRecordStr[64],  us360VersionStr[64] , 		 wifiChannelStr[64], 	 exposureFreqStr[64], 	   fanControlStr[64], 			 sharpnessStr[64],
		 userCtrl30FpsStr[64],  cameraModeStr[64], 			 colorSTModeStr[64], 	 AutoGlobalPhiAdjMode[64], HDMITextVisibility[64],
		 speakerModeStr[64],    ledBrightnessStr[64], 		 oledControlStr[64], 	 delayValueStr[64], 	   imageQualityStr[64], 		 photographResoStr[64],
		 recordResoStr[64],     timeLapseResoStr[64], 		 translucentStr[64], 	 compassMaxxStr[64], 	   compassMaxyStr[64], 			 compassMaxzStr[64],
		 compassMinxStr[64],    compassMinyStr[64], 		 compassMinzStr[64], 	 debugLogModeStr[64], 	   bottomModeStr[64], 			 bottomSizeStr[64],
		 hdrEvModeStr[64], 	    bitrateStr[64], 			 httpAccountStr[64], 	 httpPasswordStr[64], 	   httpPortStr[64], 			 compassModeStr[64],
		 gsensorModeStr[64],    capHdrModeStr[64], 			 bottomTModeStr[64], 	 bottomTColorStr[64], 	   bottomBColorStr[64], 		 bottomTFontStr[64],
		 bottomTLoopStr[64],    bottomTextStr[128], 	 	 fpgaEncodeTypeStr[64],  wbRGBStr[64], 			   contrastStr[64], 			 saturationStr[64],
		 freeCountStr[64],      saveTimelapseStr[64], 		 bmodeSecStr[64], 	     bmodeGainStr[64],		   hdrManualStr[64],			 hdrNumberStr[64],
		 hdrIncrementStr[64],   hdrStrengthStr[64],			 hdrToneStr[64],	     aebNumberStr[64],		   aebIncrementStr[64],			 LiveQualityStr[64],
		 wbTemperatureStr[64],  hdrDeghostingStr[64],		 removeHdrModeStr[64],   removeHdrNumberStr[64],   removeHdrIncrementStr[64],	 removeHdrStrengthStr[64],
		 antiAliasingEnStr[64], removeAntiAliasingEnStr[64], wbTintStr[64],			 hdrAutoStrengthStr[64],   removeHdrAutoStrengthStr[64], liveBitrateStr[64],
		 powerSavingStr[64];

	char tmpStr[128];
	struct dirent *dir = NULL;//  = new File(dirPath);
	FILE *file = NULL;// = new File(filePath);

	dir = opendir(dirPath);
	if(dir == NULL) {
		db_debug("write not find us360databin dir\n");
		if(mkdir(dirPath, S_IRWXU) == 0) {
			dir = opendir(dirPath);
			if(dir == NULL) return;
		}
		else {
			db_error("make us360databin dir error\n");
			return;
		}
	}

	file = fopen(filePath, "wt");
	if(file != NULL) {
		sprintf(versionStr, 			  "Version: %d\r\n\0", 				 Get_DataBin_Version() );
		sprintf(demoModeStr, 			  "DemoMode: %d\r\n\0", 		 	 Get_DataBin_DemoMode() );
		sprintf(camPositionModeStr, 	  "CamPositionMode: %d\r\n\0", 		 Get_DataBin_CamPositionMode() );
		sprintf(playModeStr, 			  "PlayMode: %d\r\n\0", 			 Get_DataBin_PlayMode() );
		sprintf(resoultModeStr, 		  "ResoultMode: %d\r\n\0", 			 Get_DataBin_ResoultMode() );
		sprintf(evValueStr, 			  "EVValue: %d\r\n\0", 				 Get_DataBin_EVValue() );
		sprintf(mfValueStr, 			  "MFValue: %d\r\n\0", 				 Get_DataBin_MFValue() );
		sprintf(mf2ValueStr, 			  "MF2Value: %d\r\n\0", 			 Get_DataBin_MF2Value() );
		sprintf(isoValueStr, 			  "ISOValue: %d\r\n\0", 			 Get_DataBin_ISOValue() );
		sprintf(exposureValueStr, 		  "ExposureValue: %d\r\n\0", 		 Get_DataBin_ExposureValue() );
		sprintf(wbModeStr, 				  "WBMode: %d\r\n\0", 			 	 Get_DataBin_WBMode() );
		sprintf(captureModeStr, 		  "CaptureMode: %d\r\n\0", 			 Get_DataBin_CaptureMode() );
		sprintf(captureCntStr, 			  "CaptureCnt: %d\r\n\0", 			 Get_DataBin_CaptureCnt() );
		sprintf(captureSpaceTimeStr, 	  "CaptureSpaceTime: %d\r\n\0", 	 Get_DataBin_CaptureSpaceTime() );
		sprintf(selfTimerStr, 			  "SelfTimer: %d\r\n\0", 			 Get_DataBin_SelfTimer() );
		sprintf(timeLapseModeStr, 		  "TimeLapseMode: %d\r\n\0", 		 Get_DataBin_TimeLapseMode() );
		sprintf(saveToSelStr, 			  "SaveToSel: %d\r\n\0", 			 Get_DataBin_SaveToSel() );
		sprintf(wifiDisabletTimeStr, 	  "WifiDisableTime: %d\r\n\0", 		 Get_DataBin_WifiDisableTime() );
		sprintf(ethernetModeStr, 		  "EthernetMode: %d\r\n\0", 		 Get_DataBin_EthernetMode() );
		memset(&tmpStr[0], 0, sizeof(tmpStr) );
		Get_DataBin_EthernetIP(&tmpStr[0], sizeof(tmpStr) );
		sprintf(ethernetIPStr, 		  	  "EthernetIP: %s\r\n\0", 			 &tmpStr[0]);
		memset(&tmpStr[0], 0, sizeof(tmpStr) );
		Get_DataBin_EthernetMask(&tmpStr[0], sizeof(tmpStr) );
		sprintf(ethernetMaskStr, 		  "EthernetMask: %s\r\n\0", 		 &tmpStr[0]);
		memset(&tmpStr[0], 0, sizeof(tmpStr) );
		Get_DataBin_EthernetGateWay(&tmpStr[0], sizeof(tmpStr) );
		sprintf(ethernetGateWayStr, 	  "EthernetGateWay: %s\r\n\0", 		 &tmpStr[0]);
		memset(&tmpStr[0], 0, sizeof(tmpStr) );
		Get_DataBin_EthernetDNS(&tmpStr[0], sizeof(tmpStr) );
		sprintf(ethernetDNSStr, 		  "EthernetDNS: %s\r\n\0", 			 &tmpStr[0]);
		sprintf(mediaPortStr, 			  "MediaPort: %d\r\n\0", 			 Get_DataBin_MediaPort() );
		sprintf(drivingRecordStr, 		  "DrivingRecord: %d\r\n\0", 		 Get_DataBin_DrivingRecord() );
		memset(&tmpStr[0], 0, sizeof(tmpStr) );
		Get_DataBin_US360Version(&tmpStr[0], sizeof(tmpStr) );
		sprintf(us360VersionStr, 		  "US360Version: %s\r\n\0", 		 tmpStr);
		sprintf(wifiChannelStr, 		  "WifiChannel: %d\r\n\0", 			 Get_DataBin_WifiChannel() );
		sprintf(exposureFreqStr, 		  "ExposureFreq: %d\r\n\0", 		 Get_DataBin_ExposureFreq() );
		sprintf(fanControlStr, 			  "FanControl: %d\r\n\0", 			 Get_DataBin_FanControl() );
		sprintf(sharpnessStr, 			  "Sharpness: %d\r\n\0", 			 Get_DataBin_Sharpness() );
		sprintf(userCtrl30FpsStr, 		  "UserCtrl30Fps: %d\r\n\0", 		 Get_DataBin_UserCtrl30Fps() );
		sprintf(cameraModeStr, 			  "CameraMode: %d\r\n\0", 			 Get_DataBin_CameraMode() );
		sprintf(colorSTModeStr, 		  "ColorSTMode: %d\r\n\0", 			 Get_DataBin_ColorSTMode() );
		sprintf(AutoGlobalPhiAdjMode, 	  "AutoGlobalPhiAdjMode: %d\r\n\0",  Get_DataBin_AutoGlobalPhiAdjMode() );
		sprintf(HDMITextVisibility, 	  "HDMITextVisibility: %d\r\n\0", 	 Get_DataBin_HDMITextVisibility() );
		sprintf(speakerModeStr, 		  "SpeakerMode: %d\r\n\0", 			 Get_DataBin_SpeakerMode() );
		sprintf(ledBrightnessStr, 		  "LedBrightness: %d\r\n\0", 		 Get_DataBin_LedBrightness() );
		sprintf(oledControlStr, 		  "OledControl: %d\r\n\0", 			 Get_DataBin_OledControl() );
		sprintf(delayValueStr, 			  "DelayValue: %d\r\n\0", 			 Get_DataBin_DelayValue() );
		sprintf(imageQualityStr, 		  "ImageQuality: %d\r\n\0", 		 Get_DataBin_ImageQuality() );
		sprintf(photographResoStr, 		  "PhotographReso: %d\r\n\0", 		 Get_DataBin_PhotographReso() );
		sprintf(recordResoStr, 			  "RecordReso: %d\r\n\0", 			 Get_DataBin_RecordReso() );
		sprintf(timeLapseResoStr, 		  "TimeLapseReso: %d\r\n\0", 		 Get_DataBin_TimeLapseReso() );
		sprintf(translucentStr, 		  "Translucent: %d\r\n\0", 			 Get_DataBin_Translucent() );
		sprintf(compassMaxxStr, 		  "CompassMaxx: %d\r\n\0", 			 Get_DataBin_CompassMaxx() );
		sprintf(compassMaxyStr, 		  "CompassMaxy: %d\r\n\0", 			 Get_DataBin_CompassMaxy() );
		sprintf(compassMaxzStr, 		  "CompassMaxz: %d\r\n\0", 			 Get_DataBin_CompassMaxz() );
		sprintf(compassMinxStr, 		  "CompassMinx: %d\r\n\0", 			 Get_DataBin_CompassMinx() );
		sprintf(compassMinyStr, 		  "CompassMiny: %d\r\n\0", 			 Get_DataBin_CompassMiny() );
		sprintf(compassMinzStr, 		  "CompassMinz: %d\r\n\0", 			 Get_DataBin_CompassMinz() );
		sprintf(debugLogModeStr, 		  "DebugLogMode: %d\r\n\0", 		 Get_DataBin_DebugLogMode() );
		sprintf(bottomModeStr, 			  "BottomMode: %d\r\n\0", 			 Get_DataBin_BottomMode() );
		sprintf(bottomSizeStr, 			  "BottomSize: %d\r\n\0", 			 Get_DataBin_BottomSize() );
		sprintf(hdrEvModeStr, 			  "HdrEvMode: %d\r\n\0", 			 Get_DataBin_hdrEvMode() );
		sprintf(bitrateStr, 			  "Bitrate: %d\r\n\0", 				 Get_DataBin_Bitrate() );
		memset(&tmpStr[0], 0, sizeof(tmpStr) );
		Get_DataBin_HttpAccount(&tmpStr[0], sizeof(tmpStr) );
		sprintf(httpAccountStr, 		  "HttpAccount: %s\r\n\0", 			 &tmpStr[0]);
		memset(&tmpStr[0], 0, sizeof(tmpStr) );
		Get_DataBin_HttpPassword(&tmpStr[0], sizeof(tmpStr) );
		sprintf(httpPasswordStr, 		  "HttpPassword: %s\r\n\0", 		 &tmpStr[0]);
		sprintf(httpPortStr, 			  "HttpPort: %d\r\n\0", 			 Get_DataBin_HttpPort() );
		sprintf(compassModeStr, 		  "CompassMode: %d\r\n\0", 			 Get_DataBin_CompassMode() );
		sprintf(gsensorModeStr, 		  "GsensorMode: %d\r\n\0", 			 Get_DataBin_GsensorMode() );
		sprintf(capHdrModeStr, 			  "CapHdrMode: %d\r\n\0", 			 Get_DataBin_CapHdrMode() );
		sprintf(bottomTModeStr, 		  "BottomTMode: %d\r\n\0", 		 	 Get_DataBin_BottomTMode() );
		sprintf(bottomTColorStr, 		  "BottomTColor: %d\r\n\0", 		 Get_DataBin_BottomTColor() );
		sprintf(bottomBColorStr, 		  "BottomBColor: %d\r\n\0", 		 Get_DataBin_BottomBColor() );
		sprintf(bottomTFontStr, 		  "BottomTFont: %d\r\n\0", 			 Get_DataBin_BottomTFont() );
		sprintf(bottomTLoopStr, 		  "BottomTLoop: %d\r\n\0", 			 Get_DataBin_BottomTLoop() );
		memset(&tmpStr[0], 0, sizeof(tmpStr) );
		Get_DataBin_BottomText(&tmpStr[0], sizeof(tmpStr) );
		sprintf(bottomTextStr, 			  "BottomText: %s\r\n\0",			 &tmpStr[0]);
		sprintf(fpgaEncodeTypeStr, 		  "FpgaEncodeType: %d\r\n\0", 		 Get_DataBin_FpgaEncodeType() );
		memset(&tmpStr[0], 0, sizeof(tmpStr) );
		Get_DataBin_WbRGB(&tmpStr[0], sizeof(tmpStr) );
		sprintf(wbRGBStr, 				  "WbRGB: %s\r\n\0", 			 	 &tmpStr[0]);
		sprintf(contrastStr, 			  "Contrast: %d\r\n\0", 			 Get_DataBin_Contrast() );
		sprintf(saturationStr, 			  "Saturation: %d\r\n\0", 			 Get_DataBin_Saturation() );
		sprintf(freeCountStr, 			  "FreeCount: %d\r\n\0", 			 Get_DataBin_FreeCount() );
		sprintf(saveTimelapseStr, 		  "SaveTimelapse: %d\r\n\0", 		 Get_DataBin_SaveTimelapse() );
		sprintf(bmodeSecStr, 			  "BmodeSec: %d\r\n\0", 			 Get_DataBin_BmodeSec() );
		sprintf(bmodeGainStr, 			  "BmodeGain: %d\r\n\0", 			 Get_DataBin_BmodeGain() );
		sprintf(hdrManualStr, 			  "HdrManual: %d\r\n\0", 			 Get_DataBin_HdrManual() );
		sprintf(hdrNumberStr, 			  "HdrNumber: %d\r\n\0", 			 Get_DataBin_HdrNumber() );
		sprintf(hdrIncrementStr, 		  "HdrIncrement: %d\r\n\0", 		 Get_DataBin_HdrIncrement() );
		sprintf(hdrStrengthStr, 		  "HdrStrength: %d\r\n\0", 			 Get_DataBin_HdrStrength() );
		sprintf(hdrToneStr, 			  "HdrTone: %d\r\n\0", 				 Get_DataBin_HdrTone() );
		sprintf(aebNumberStr, 		  	  "AebNumber: %d\r\n\0", 		 	 Get_DataBin_AebNumber() );
		sprintf(aebIncrementStr, 		  "AebIncrement: %d\r\n\0", 		 Get_DataBin_AebIncrement() );
		sprintf(LiveQualityStr, 		  "LiveQualityMode: %d\r\n\0", 		 Get_DataBin_LiveQualityMode() );
		sprintf(wbTemperatureStr, 		  "wbTemperature: %d\r\n\0", 		 Get_DataBin_WbTemperature() );
		sprintf(hdrDeghostingStr, 		  "hdrDeghosting: %d\r\n\0", 		 Get_DataBin_HdrDeghosting() );
		sprintf(removeHdrModeStr, 		  "removeHdrMode: %d\r\n\0", 		 Get_DataBin_RemoveHdrMode() );
		sprintf(removeHdrNumberStr, 	  "removeHdrNumber: %d\r\n\0", 		 Get_DataBin_RemoveHdrNumber() );
		sprintf(removeHdrIncrementStr, 	  "removeHdrIncrement: %d\r\n\0", 	 Get_DataBin_RemoveHdrIncrement() );
		sprintf(removeHdrStrengthStr, 	  "removeHdrStrength: %d\r\n\0", 	 Get_DataBin_RemoveHdrStrength() );
		sprintf(antiAliasingEnStr, 		  "AntiAliasingEn: %d\r\n\0", 		 Get_DataBin_AntiAliasingEn() );
		sprintf(removeAntiAliasingEnStr,  "removeAntiAliasingEn: %d\r\n\0",  Get_DataBin_RemoveAntiAliasingEn() );
		sprintf(wbTintStr, 				  "wbTint: %d\r\n\0", 				 Get_DataBin_WbTint() );
		sprintf(hdrAutoStrengthStr, 	  "HdrAutoStrength: %d\r\n\0", 		 Get_DataBin_HdrAutoStrength() );
		sprintf(removeHdrAutoStrengthStr, "removeHdrAutoStrength: %d\r\n\0", Get_DataBin_RemoveHdrAutoStrength() );
		sprintf(liveBitrateStr, 		  "LiveBitrate: %d\r\n\0", 			 Get_DataBin_LiveBitrate() );
		sprintf(powerSavingStr, 		  "PowerSaving: %d\r\n\0", 			 Get_DataBin_PowerSaving() );


		fwrite(&versionStr[0], strlen(versionStr), 1, file);
		fwrite(&demoModeStr[0], strlen(demoModeStr), 1, file);
		fwrite(&camPositionModeStr[0], strlen(camPositionModeStr), 1, file);
		fwrite(&playModeStr[0], strlen(playModeStr), 1, file);
		fwrite(&resoultModeStr[0], strlen(resoultModeStr), 1, file);
		fwrite(&evValueStr[0], strlen(evValueStr), 1, file);
		fwrite(&mfValueStr[0], strlen(mfValueStr), 1, file);
		fwrite(&mf2ValueStr[0], strlen(mf2ValueStr), 1, file);
		fwrite(&isoValueStr[0], strlen(isoValueStr), 1, file);
		fwrite(&exposureValueStr[0], strlen(exposureValueStr), 1, file);
		fwrite(&wbModeStr[0], strlen(wbModeStr), 1, file);
		fwrite(&captureModeStr[0], strlen(captureModeStr), 1, file);
		fwrite(&captureCntStr[0], strlen(captureCntStr), 1, file);
		fwrite(&captureSpaceTimeStr[0], strlen(captureSpaceTimeStr), 1, file);
		fwrite(&selfTimerStr[0], strlen(selfTimerStr), 1, file);
		fwrite(&timeLapseModeStr[0], strlen(timeLapseModeStr), 1, file);
		fwrite(&saveToSelStr[0], strlen(saveToSelStr), 1, file);
		fwrite(&wifiDisabletTimeStr[0], strlen(wifiDisabletTimeStr), 1, file);
		fwrite(&ethernetModeStr[0], strlen(ethernetModeStr), 1, file);
		fwrite(&ethernetIPStr[0], strlen(ethernetIPStr), 1, file);
		fwrite(&ethernetMaskStr[0], strlen(ethernetMaskStr), 1, file);
		fwrite(&ethernetGateWayStr[0], strlen(ethernetGateWayStr), 1, file);
		fwrite(&ethernetDNSStr[0], strlen(ethernetDNSStr), 1, file);
		fwrite(&mediaPortStr[0], strlen(mediaPortStr), 1, file);
		fwrite(&drivingRecordStr[0], strlen(drivingRecordStr), 1, file);
		fwrite(&us360VersionStr[0], strlen(us360VersionStr), 1, file);
		fwrite(&wifiChannelStr[0], strlen(wifiChannelStr), 1, file);
		fwrite(&exposureFreqStr[0], strlen(exposureFreqStr), 1, file);
		fwrite(&fanControlStr[0], strlen(fanControlStr), 1, file);
		fwrite(&sharpnessStr[0], strlen(sharpnessStr), 1, file);
		fwrite(&userCtrl30FpsStr[0], strlen(userCtrl30FpsStr), 1, file);
		fwrite(&cameraModeStr[0], strlen(cameraModeStr), 1, file);
		fwrite(&colorSTModeStr[0], strlen(colorSTModeStr), 1, file);
		fwrite(&AutoGlobalPhiAdjMode[0], strlen(AutoGlobalPhiAdjMode), 1, file);
		fwrite(&HDMITextVisibility[0], strlen(HDMITextVisibility), 1, file);
		fwrite(&speakerModeStr[0], strlen(speakerModeStr), 1, file);
		fwrite(&ledBrightnessStr[0], strlen(ledBrightnessStr), 1, file);
		fwrite(&oledControlStr[0], strlen(oledControlStr), 1, file);
		fwrite(&delayValueStr[0], strlen(delayValueStr), 1, file);
		fwrite(&imageQualityStr[0], strlen(imageQualityStr), 1, file);
		fwrite(&photographResoStr[0], strlen(photographResoStr), 1, file);
		fwrite(&recordResoStr[0], strlen(recordResoStr), 1, file);
		fwrite(&timeLapseResoStr[0], strlen(timeLapseResoStr), 1, file);
		fwrite(&translucentStr[0], strlen(translucentStr), 1, file);
		fwrite(&compassMaxxStr[0], strlen(compassMaxxStr), 1, file);
		fwrite(&compassMaxyStr[0], strlen(compassMaxyStr), 1, file);
		fwrite(&compassMaxzStr[0], strlen(compassMaxzStr), 1, file);
		fwrite(&compassMinxStr[0], strlen(compassMinxStr), 1, file);
		fwrite(&compassMinyStr[0], strlen(compassMinyStr), 1, file);
		fwrite(&compassMinzStr[0], strlen(compassMinzStr), 1, file);
		fwrite(&debugLogModeStr[0], strlen(debugLogModeStr), 1, file);
		fwrite(&bottomModeStr[0], strlen(bottomModeStr), 1, file);
		fwrite(&bottomSizeStr[0], strlen(bottomSizeStr), 1, file);
		fwrite(&hdrEvModeStr[0], strlen(hdrEvModeStr), 1, file);
		fwrite(&bitrateStr[0], strlen(bitrateStr), 1, file);
		fwrite(&httpAccountStr[0], strlen(httpAccountStr), 1, file);
		fwrite(&httpPasswordStr[0], strlen(httpPasswordStr), 1, file);
		fwrite(&httpPortStr[0], strlen(httpPortStr), 1, file);
		fwrite(&compassModeStr[0], strlen(compassModeStr), 1, file);
		fwrite(&gsensorModeStr[0], strlen(gsensorModeStr), 1, file);
		fwrite(&capHdrModeStr[0], strlen(capHdrModeStr), 1, file);
		fwrite(&bottomTModeStr[0], strlen(bottomTModeStr), 1, file);
		fwrite(&bottomTColorStr[0], strlen(bottomTColorStr), 1, file);
		fwrite(&bottomBColorStr[0], strlen(bottomBColorStr), 1, file);
		fwrite(&bottomTFontStr[0], strlen(bottomTFontStr), 1, file);
		fwrite(&bottomTLoopStr[0], strlen(bottomTLoopStr), 1, file);
		fwrite(&bottomTextStr[0], strlen(bottomTextStr), 1, file);
		fwrite(&fpgaEncodeTypeStr[0], strlen(fpgaEncodeTypeStr), 1, file);
		fwrite(&wbRGBStr[0], strlen(wbRGBStr), 1, file);
		fwrite(&contrastStr[0], strlen(contrastStr), 1, file);
		fwrite(&saturationStr[0], strlen(saturationStr), 1, file);
		fwrite(&freeCountStr[0], strlen(freeCountStr), 1, file);
		fwrite(&saveTimelapseStr[0], strlen(saveTimelapseStr), 1, file);
		fwrite(&bmodeSecStr[0], strlen(bmodeSecStr), 1, file);
		fwrite(&bmodeGainStr[0], strlen(bmodeGainStr), 1, file);
		fwrite(&hdrManualStr[0], strlen(hdrManualStr), 1, file);
		fwrite(&hdrNumberStr[0], strlen(hdrNumberStr), 1, file);
		fwrite(&hdrIncrementStr[0], strlen(hdrIncrementStr), 1, file);
		fwrite(&hdrStrengthStr[0], strlen(hdrStrengthStr), 1, file);
		fwrite(&hdrToneStr[0], strlen(hdrToneStr), 1, file);
		fwrite(&aebNumberStr[0], strlen(aebNumberStr), 1, file);
		fwrite(&aebIncrementStr[0], strlen(aebIncrementStr), 1, file);
		fwrite(&LiveQualityStr[0], strlen(LiveQualityStr), 1, file);
		fwrite(&wbTemperatureStr[0], strlen(wbTemperatureStr), 1, file);
		fwrite(&hdrDeghostingStr[0], strlen(hdrDeghostingStr), 1, file);
		fwrite(&removeHdrModeStr[0], strlen(removeHdrModeStr), 1, file);
		fwrite(&removeHdrNumberStr[0], strlen(removeHdrNumberStr), 1, file);
		fwrite(&removeHdrIncrementStr[0], strlen(removeHdrIncrementStr), 1, file);
		fwrite(&removeHdrStrengthStr[0], strlen(removeHdrStrengthStr), 1, file);
		fwrite(&antiAliasingEnStr[0], strlen(antiAliasingEnStr), 1, file);
		fwrite(&removeAntiAliasingEnStr[0], strlen(removeAntiAliasingEnStr), 1, file);
		fwrite(&wbTintStr[0], strlen(wbTintStr), 1, file);
		fwrite(&hdrAutoStrengthStr[0], strlen(hdrAutoStrengthStr), 1, file);
		fwrite(&removeHdrAutoStrengthStr[0], strlen(removeHdrAutoStrengthStr), 1, file);
		fwrite(&liveBitrateStr[0], strlen(liveBitrateStr), 1, file);
		fwrite(&powerSavingStr[0], strlen(powerSavingStr), 1, file);

		fflush(file);
		fclose(file);
	}
	else
		db_error("write us360databin file error\n");
}
void WriteUS360DataBin(){
	db_debug("write us360databin readState:%d\n", isReadFinish);
	writeDatabin2Path(&mainPath[0]);
	db_debug("write us360databin_back readState:%d\n", isReadFinish);
	writeDatabin2Path(&backupPath[0]);
}

int readDatabinFromPath(char *filePath){
	int re = 1;
	FILE *fp;
	char *ptr, *tmp, *start=NULL;
	struct stat sti;
	char strNum[64];
	int count = 0, pos = 0;
	int posStrLen = TAG_MAX;
	char posStr[TAG_MAX][32] = {"Version", "DemoMode", "CamPositionMode", "PlayMode", "ResoultMode", "EVValue", "MFValue", "MF2Value",
						"ISOValue", "ExposureValue", "WBMode", "CaptureMode", "CaptureCnt", "CaptureSpaceTime", "SelfTimer",
						"TimeLapseMode", "SaveToSel", "WifiDisableTime", "EthernetMode", "EthernetIP", "EthernetMask", "EthernetGateWay", "EthernetDNS", "MediaPort",
						"DrivingRecord", "US360Version", "WifiChannel", "ExposureFreq", "FanControl", "Sharpness", "UserCtrl30Fps", "CameraMode", "ColorSTMode",
						"AutoGlobalPhiAdjMode", "HDMITextVisibility", "SpeakerMode", "LedBrightness", "OledControl", "DelayValue", "ImageQuality",
						"PhotographReso", "RecordReso", "TimeLapseReso", "Translucent", "CompassMaxx", "CompassMaxy", "CompassMaxz", "CompassMinx", "CompassMiny",
						"CompassMinz", "DebugLogMode", "BottomMode", "BottomSize", "HdrEvMode", "Bitrate", "HttpAccount", "HttpPassword", "HttpPort",
						"CompassMode", "GsensorMode", "CapHdrMode", "BottomTMode", "BottomTColor", "BottomBColor", "BottomTFont", "BottomTLoop", "BottomText",
						"FpgaEncodeType", "WbRGB", "Contrast", "Saturation", "FreeCount", "SaveTimelapse", "BmodeSec", "BmodeGain", "HdrManual", "HdrNumber",
						"HdrIncrement", "HdrStrength", "HdrTone", "AebNumber", "AebIncrement", "LiveQualityMode", "wbTemperature", "hdrDeghosting",
						"removeHdrMode", "removeHdrNumber", "removeHdrIncrement", "removeHdrStrength", "AntiAliasingEn", "removeAntiAliasingEn", "wbTint",
						"HdrAutoStrength", "removeHdrAutoStrength", "LiveBitrate", "PowerSaving"};

    if (-1 == stat (filePath, &sti)) {
        db_error("readDatabinFromPath() file not find\n");
        return -1;
    }
    else {
    	fp = fopen(filePath, "rt");
    	if(fp != NULL) {
    		while(!feof(fp) ) {
    			if(count < posStrLen){
    				memset(&strNum[0], 0, sizeof(strNum) );
    				fgets(strNum, sizeof(strNum), fp);
    				start = strstr(&strNum[0], &posStr[count][0]);
    				if(strNum[0] != 0 && start != NULL) {
	    				tmp = &strNum[0];
	    				tmp += (strlen(posStr[count]) + 2);
	    				//db_debug("readDatabinFromPath() 01 count=%d Str=%s StrLen=%d tmp=%s\n", count, posStr[count], strlen(posStr[count]), tmp);

			    		setValue(tmp, count);
						db_debug("readDatabinFromPath() 02 count:%d str:%s\n", count, tmp);
						count++;
    				}
    				else break;
    			}
    			else break;
    		}

    		fclose(fp);

    		if(count != posStrLen){		// 有新增DataBin內容
    			re = 0;
    		}
    	}
    }
	return re;
}
void ReadUS360DataBin(int country, int customer)
{
	int readType = 0, freq = 60;
	struct dirent *dir = NULL;
	//FILE *file = NULL, *file_b = NULL;
	struct stat sti;

	dir = opendir(dirPath);
	if(dir == NULL) {
		db_debug("ReadUS360DataBin() read not find us360databin dir\n");
		if(mkdir(dirPath, S_IRWXU) == 0) {
			dir = opendir(dirPath);
			if(dir == NULL) return;
		}
		else {
			db_error("ReadUS360DataBin() make us360databin dir error\n");
			return;
		}
	}
	if(dir != NULL) closedir(dir);

    if(stat (mainPath, &sti) == 0)
    	readType = 1;
    else if(stat (backupPath, &sti) == 0)
    	readType = 2;
    else
    	readType = 0;


   	if(readType == 0){		//檔案不存在, 產生檔案
   		db_debug("ReadUS360DataBin() read not find us360databin all\n");
   		freq = CheckExpFreqDefault(country);
   		Set_DataBin_ExposureFreq(freq);
   		if(customer == CUSTOMER_CODE_PIIQ) {		//PIIQ Default
   			Set_DataBin_ResoultMode(1);			//ResoultMode: 12K
   			Set_DataBin_CameraMode(5);			//CameraMode: HDR
   			Set_DataBin_HdrManual(1);			//HDR Manual: On
   			Set_DataBin_HdrNumber(7);			//Frame: 7 Cnt
   			Set_DataBin_HdrIncrement(15);		//EV: +-1.5
   			Set_DataBin_HdrStrength(40);		//Strength: 40
   			Set_DataBin_HdrDeghosting(0);		//DeGhost: Off
   			Set_DataBin_AntiAliasingEn(0);		//Anti-Aliasing: Off
   			Set_DataBin_BottomMode(2);			//底圖: Default
   			Set_DataBin_BottomTMode(0);			//底圖文字: Off
   			Set_DataBin_DrivingRecord(0);		//DrivingRecord: On
   		}
   		else if(customer == CUSTOMER_CODE_ALIBABA) {
   			Set_DataBin_PowerSaving(1);
   		}
   		WriteUS360DataBin();
   	}else{
   		char *fileStr = &mainPath[0];
   		if(readType == 1){
   			db_debug("read us360databin mainfile\n");
   		}else{
   			db_debug("read us360databin backupfile\n");
   			fileStr = &backupPath[0];
   		}
   		int readResult = readDatabinFromPath(fileStr);
   		if(readType == 1 && !readResult){
   			db_debug("read us360databin backupfile\n");
   			readResult = readDatabinFromPath(backupPath);
   		}
   		if(!readResult){		// 有新增DataBin內容
   			WriteUS360DataBin();
   		}
   		isReadFinish = 1;
   		db_debug("read us360databin finish:%d\n", readResult);
   	}

}

void DeleteUS360DataBin(void){
   	struct stat sti;
    if(stat(mainPath, &sti) == 0)
		remove(mainPath);
    if(stat(backupPath, &sti) == 0)
		remove(backupPath);
}

int CheckExpFreqDefault(int country){
	int freq = 60;
	freq = CheckCountryFreq(country);
	return freq;
}
