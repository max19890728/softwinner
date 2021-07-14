/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Data/databin.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "Device/US363/Data/countrylist.h"
#include "Device/US363/us360.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::DataBin"

#define totalLength 1024
int isReadFinish = 0;

static char EthernetIP[4] = "";       // Ethernet IP,			0.0.0.0
static char EthernetMask[4] = "";     // Ethernet Mask,		0.0.0.0
static char EthernetGateWay[4] = "";  // Ethernet GateWay,	0.0.0.0
static char EthernetDNS[4] = "";      // Ethernet DNS,		0.0.0.0

static char US360Versin[16] = "";  // US360 Version

static int WifiChannel = 6;

static int ExposureFreq = 60;

static int CameraMode = 5;

//{ValType,		0: int 1: byte
// CheckType,	0: 不限制 1: 極限值限制  2:例外處理
// MinValue,		最小值
// MaxValue}		最大值
static int checkArray[96][4] = {
    {0, 0, 0, 0},   // version
    {0, 1, 0, 2},   // 畫面移動模式, 		0: G-sensor		1:
                    // 慣性(有摩擦力) 2: 慣性(無摩擦力)
    {0, 1, 0, 3},   // G-sensor方向,			0: AUTO		1: 0度
                    // 2: 180度 3: 90度
    {0, 1, 0, 5},   // 畫面觀看模式,			0: Global	1: Front
                    // 2:360 3: 240 4: 180	5: 4分割
    {0, 2, 0, 14},  // 畫面解析度,			0: Fix	1: 10M	2: 2.5M
                    // 3: 2M 4: 1M 5: D1 6: 1.8M(3072x576)	7: 5M	8: 8M
    {0, 1, -6,
     6},  // EV值,					-6: EV-2	-5:
          // EV-1.6 -4: EV-1.3 -3: EV-1 -2: EV-0.6	-1: EV-0.3	0: EV+0
          // 1: EV+0.3 2: EV+0.6	3: EV+1	4: EV+1.3	5:
          // EV+1.6	6: EV+2
    {0, 1, -32, 36},  // MF值,					-32 ~ 36
    {0, 1, -32, 36},  // MF2值,				-32 ~ 36
    {0, 1, -1, 140},  // ISO值,				-1: AUTO	0: ISO
                      // 100	20: ISO 200	40: ISO 400 60: ISO 800 80: ISO
                      // 1600	120: ISO 3200	140: ISO 6400
    {0, 1, -40,
     300},            // 曝光時間,				-1: AUTO	260: 1
                      // 240: 1/2 220: 1/4	200: 1/8 180: 1/15 160: 1/30	140:
                      // 1/60 120: 1/120 100: 1/250	80: 1/500	60: 1/1000 40:
                      // 1/2000 20: 1/4000	0: 1/8000  -20: 1/16000  -40: 1/32000
    {0, 1, 0, 8000},  // 白平衡模式,			0: AUTO	1: Filament Lamp
                      // 2: Daylight Lamp	3: Sun	4: Cloudy, >1000: 色溫
    {0, 1, -1, 10},   // 拍照模式,				-1: 連拍
                      // 0: 一般拍照	2:2秒自拍	10:10秒自拍
    {0, 1, 0, 3},     // 連拍次數,				0: 1次	2: 2次	3: 3次
    {0, 0, 0,
     0},            // 連拍間隔時間,			500: 500ms	1000: 1000ms
    {0, 1, 0, 30},  // 自拍倒數,				0: none	2:
                    // 2秒自拍 10: 10秒自拍
    {0, 1, 0, 7},   // 縮時錄影模式,			0: none	1: 1s	2: 2s
                    // 3: 5s 4: 10s 5: 30s	6: 60s	7:0.166s
    {0, 1, 0,
     1},               // 儲存空間位置,			0: Internal Storage 1: MicroSD
    {0, 1, -2, 300},   // wifi自動關閉時間, 	-2:not disable	60:1分鐘
                       // 120:2分鐘	180:3分鐘	300:5分鐘
    {0, 1, 0, 1},      // Ethernet 模式, 		0:DHCP	1:MANUAL
    {1, 0, 0, 0},      // Ethernet IP,			0.0.0.0
    {1, 0, 0, 0},      // Ethernet Mask,		0.0.0.0
    {1, 0, 0, 0},      // Ethernet GateWay,	0.0.0.0
    {1, 0, 0, 0},      // Ethernet DNS,		0.0.0.0
    {0, 2, 1, 65535},  // MediaPort, 			1~65535
    {0, 1, 0, 0},      // 循環錄影,				0: off	1: on
    {1, 0, 0, 0},      // US360 Version
    {0, 1, 1, 11},     // Wifi Channel
    {0, 1, 50, 60},    // EP freq,				60:60Hz 50:50Hz
    {0, 1, 0, 3},      // 風扇控制,				0: off	1: fast	2:
                       // median 3: slow
    {0, 1, 0, 15},     // Sharpness,     		0 ~ 15
    {0, 1, 0, 1},      // User控制30FPS,		0:off	1:on
    {0, 1, 0, 14},     // 攝影機模式,			0:Cap 1:Rec 2:Time_Lapse
                    // 3:Cap_HDR 4:Cap_5_Sensor 5:日拍HDR 6:夜拍  7:夜拍HDR
                    // 8:Sport 9:SportWDR 10:RecWDR 11:Time_LapseWDR 12:B快門
                    // 13:動態移除 14:3D-Model
    {0, 1, 0, 2},    // 顏色縫合模式,			0:off 1:on 2:auto
    {0, 1, 0, 100},  // 即時縫合模式,			0~100
    {0, 1, 0, 1},    // HDMI文字訊息,			0:hide 1:show
    {0, 1, 0, 1},    // 喇叭,					0:On 1:Off
    {0, 1, -1,
     100},         // LED亮度				-1:自動,亮度:0 ~ 100
    {0, 1, 0, 1},  // OLED控制				0:On 1:Off
    {0, 1, 0,
     20},           // 延遲時間控制			0:none 2:2s 5:5s 10:10s 20:20s
    {0, 1, 0, 2},   // 影像品質				0:高,1:中,2:低
    {0, 2, 1, 14},  // 拍照解析度			1:12k,7:8k,12:6k
    {0, 2, 1, 14},  // 錄影解析度			2:4k,13:3k,14:2k
    {0, 2, 1, 14},  // 縮時解析度			7:8k,12:6k,2:4k
    {0, 1, 0, 2},   // 半透明設定			0:關,1:開,2:自動
    {0, 1, -60000, 60000},  // 電子羅盤X軸最大值
    {0, 1, -60000, 60000},  // 電子羅盤y軸最大值
    {0, 1, -60000, 60000},  // 電子羅盤z軸最大值
    {0, 1, -60000, 60000},  // 電子羅盤X軸最小值
    {0, 1, -60000, 60000},  // 電子羅盤y軸最小值
    {0, 1, -60000, 60000},  // 電子羅盤z軸最小值
    {0, 1, 0, 1},           // 除錯訊息存至SD卡		0:關, 1:開
    {0, 1, 0, 4},           // 底圖設定				0:關, 1:延伸,
                            // 2:底圖(default), 3:鏡像, 4:底圖(user)
    {0, 1, 10, 100},        // 底圖大小				10 ~ 100
    {0, 1, 2,
     8},  // HDR EV設定			2:0,-1,-2 4:+1,-1,-3 8:+2,-1,-4
    {0, 1, 0,
     10},                 // Rec Bitrate				0:最高 5:中等 10:最低
    {1, 0, 0, 0},         // HttpAccount			網頁/RTSP帳號
    {1, 0, 0, 0},         // HttpPassword			網頁/RTSP密碼
    {0, 2, 1024, 65534},  // HttpPort				1024 ~ 65534
    {0, 1, 0, 1},         // 電子羅盤開關			0:關, 1:開
    {0, 1, 0, 1},         // G-Sensor開關			0:關, 1:開
    {0, 1, 0, 6},         // 拍照WDR開關			0:日拍, 1:日拍HDR,
                   // 2:夜拍, 3:夜拍HDR 4:運動 5:運動HDR 6:B快門
    {0, 1, 0, 1},   // 底圖文字控制
    {0, 1, 0, 13},  // 底圖文字顏色代碼
    {0, 1, 0, 13},  // 底圖文字背景顏色代碼
    {0, 1, 0, 4},   // 底圖文字字型
    {0, 1, 0, 1},   // 底圖循環顯示
    {1, 0, 0, 0},   // 底圖文字
    {0, 2, 0, 1},   // 縮時錄影檔案類型
    {1, 0, 0, 0},   // 白平衡RGB數值(0~255)	R.G.B.0
    {0, 1, -7, 7},  // 對比度 -7~7
    {0, 1, -7, 7},  // 彩度 -7~7
    {0, 0, 0, 0},   // 剩餘拍攝張數/錄影時間
    {0,
     1, 1, 7},  // 儲存縮時模式(0不儲存)	1: 1s	2: 2s	3: 5s	4: 10s	5: 30s	6: 60s	7:0.166s
    {0, 1, 4, 720},  // B快門時間
    {0, 1, 0, 140},  // B快門ISO
    {0, 1, 0, 2},    // HDR手動模式開關	0:關  1:開  2:Auto
    {0, 1, 3, 7},    // HDR手動模式張數	3,5,7
    {0, 1, 5,
     25},  // HDR手動模式EV值	5:+-0.5 10:+-1.0 15:+-1.5 20:+-2.0 25:+-2.5
    {0, 1, 30, 90},   // HDR手動模式強度	30 ~ 90
    {0, 1, -10, 10},  // HDR手動模式影調壓縮	-10 ~ 10
    {0, 1, 3, 7},     // AEB手動模式張數	3,5,7
    {0, 1, 5,
     25},  // AEB手動模式EV值	5:+-0.5 10:+-1.0 15:+-1.5 20:+-2.0 25:+-2.5
    {0, 1, 0,
     1},  // Live Quality Mode 0:10FPS Hight Quality	1:5FPS Low Quality
    {0, 1, 22, 100},  // 白平衡色溫值 20 ~ 100(2200K ~ 10000K)
    {0, 1, 0, 1},     // HDR去鬼影 0:關 1:開
    {0, 1, 0, 5},  // 動態移除模式 0:關 1:自動 2:弱 3:中 4:強 5:自訂
    {0, 1, 0, 7},     // 動態移除HDR張數 3,5,7
    {0, 1, 0, 25},    // 動態移除HDR EV值 5,10,15,20,25
    {0, 1, 30, 90},   // 動態移除HDR強度 30 ~ 90
    {0, 1, 0, 1},     // 反鋸齒				0:off 1:on
    {0, 1, 0, 1},     // 動態移除反鋸齒		0:off 1:on
    {0, 1, -15, 15},  // 白平衡Tint		-15 ~ +15
    {0, 1, 30, 90},   // HDR Auto模式強度	30 ~ 90
    {0, 1, 30, 90},   // 動態移除HDR Auto強度 30 ~ 90
    {0, 1, 0,
     10},         // Live Bitrate				0:最高 5:中等 10:最低
    {0, 1, 0, 1}  // 省電模式			0:Off 1:On
};
static int cameraReso[15][4] = {
    {1, 7, 12, -1},   // Cap 			1:12K 	 7:8K 	12:6K
    {2, 13, 14, -1},  // Rec 			2:4K 	13:3K 	14:2K
    {1, 2, 7, 12},    // TimeLapse 	1:12K 	 2:4K 	 7:8K 	12:6K
    {1, -1, -1, -1},  // AEB(3P)   	1:12K
    {1, 7, 12, -1},   // RAW(5P)   	1:12K 	 7:8K 	12:6K
    {1, 7, 12, -1},   // WDR       	1:12K 	 7:8K 	12:6K
    {1, 7, 12, -1},   // Night     	1:12K 	 7:8K 	12:6K
    {1, 7, 12, -1},   // NightWDR  	1:12K 	 7:8K 	12:6K
    {1, 7, 12, -1},   // Sport 		1:12K 	 7:8K 	12:6K
    {1, 7, 12, -1},   // SportWDR 		1:12K 	 7:8K 	12:6K
    {2, 13, 14, -1},  // RecWDR 		2:4K 	13:3K 	14:2K
    {1, 2, 7, 12},    // TimeLapseWDR 	1:12K 	 2:4K 	 7:8K 	12:6K
    {1, 7, 12, -1},   // Bmode 		1:12K 	 7:8K 	12:6K
    {1, -1, -1, -1},  // remove  	1:12K
    {1, -1, -1, -1}   // remove HDR	1:12K
};

int checkIntValue(int tag, int value) {
  return 0;
  // int x, length;
  // int retunValue = value;
  // int valueType = checkArray[tag][0];
  // int checkType = checkArray[tag][1];
  // int invalidRtspPorts[4] = {8080, 8554, 8556, 8557};
  // int invalidHttpPorts[4] = {8554, 8555, 8556, 8557};
  // int getReso = 0;

  // if (valueType == 0) {
  //   if (checkType == 1) {
  //     if (retunValue < checkArray[tag][2]) {
  //       retunValue = checkArray[tag][2];
  //     }
  //     if (retunValue > checkArray[tag][3]) {
  //       retunValue = checkArray[tag][3];
  //     }
  //   } else if (checkType == 2) {
  //     switch (tag) {
  //       case TagResoultMode:
  //         for (x = 0; x < 4; x++) {
  //           if (cameraReso[CameraMode][x] == -1) break;
  //           if (cameraReso[CameraMode][x] == retunValue) {
  //             getReso = 1;
  //             break;
  //           }
  //         }
  //         if (!getReso) {
  //           retunValue = cameraReso[CameraMode][0];
  //         }
  //         break;
  //       case TagPhotographReso:
  //         for (x = 0; x < 4; x++) {
  //           if (cameraReso[0][x] == -1) break;
  //           if (cameraReso[0][x] == retunValue) {
  //             getReso = 1;
  //             break;
  //           }
  //         }
  //         if (!getReso) {
  //           retunValue = 1;
  //         }
  //         break;
  //       case TagRecordReso:
  //         for (x = 0; x < 4; x++) {
  //           if (cameraReso[1][x] == -1) break;
  //           if (cameraReso[1][x] == retunValue) {
  //             getReso = 1;
  //             break;
  //           }
  //         }
  //         if (!getReso) {
  //           retunValue = 14;
  //         }
  //         break;
  //       case TagTimeLapseReso:
  //         for (x = 0; x < 4; x++) {
  //           if (cameraReso[2][x] == -1) break;
  //           if (cameraReso[2][x] == retunValue) {
  //             getReso = 1;
  //             break;
  //           }
  //         }
  //         if (!getReso) {
  //           retunValue = 12;
  //         }
  //         break;
  //       case TagMediaPort:
  //         if (retunValue < 1024 || retunValue > 65534) {
  //           retunValue = 8555;
  //         } else if (retunValue == HttpPort) {
  //           retunValue = 8555;
  //         } else {
  //           for (x = 0; x < 4; x++) {
  //             if (retunValue == invalidRtspPorts[x]) {
  //               retunValue = 8555;
  //               break;
  //             }
  //           }
  //         }
  //         break;
  //       case TagHttpPort:
  //         if (retunValue < 1024 || retunValue > 65534) {
  //           retunValue = 8080;
  //         } else if (retunValue == Get_DataBin_MediaPort()) {
  //           retunValue = 8080;
  //         } else {
  //           for (x = 0; x < 4; x++) {
  //             if (retunValue == invalidHttpPorts[x]) {
  //               retunValue = 8080;
  //               break;
  //             }
  //           }
  //         }
  //         break;
  //       case TagFpgaEncodeType:
  //         if (timeLapseReso == 1) retunValue = 0;
  //         if (retunValue < checkArray[tag][2])
  //           retunValue = checkArray[tag][2];
  //         else if (retunValue > checkArray[tag][3])
  //           retunValue = checkArray[tag][3];
  //     }
  //   }
  // }
  // return retunValue;
}

// - MARK: Set Function

void Set_DataBin_Version(int version) {
  set_int_to_data_bin_by_key("Version", version);
}

void Set_DataBin_DemoMode(int mode) {
  set_int_to_data_bin_by_key("DemoMode", mode);
}

void Set_DataBin_CamPositionMode(int mode) {
  set_int_to_data_bin_by_key("PositionMode", mode);
}

void Set_DataBin_PlayMode(int mode) {
  set_int_to_data_bin_by_key("PlayMode", mode);
}

void Set_DataBin_ResoultMode(int mode) {
  set_int_to_data_bin_by_key("ResoultMode", mode);
}

void Set_DataBin_EVValue(int value) {
  set_int_to_data_bin_by_key("EVValue", value);
}

void Set_DataBin_MFValue(int value) {
  set_int_to_data_bin_by_key("MFValue1", value);
}

void Set_DataBin_MF2Value(int value) {
  set_int_to_data_bin_by_key("MFValue2", value);
}

void Set_DataBin_ISOValue(int value) {
  set_int_to_data_bin_by_key("ISOValue", value);
}

void Set_DataBin_ExposureValue(int value) {
  set_int_to_data_bin_by_key("ExposureValue", value);
}

void Set_DataBin_WBMode(int mode) {
  set_int_to_data_bin_by_key("WhiteBlanceMode", mode);
}

void Set_DataBin_CaptureMode(int mode) {
  set_int_to_data_bin_by_key("CaptureMode", mode);
}

void Set_DataBin_CaptureCnt(int cnt) {
  set_int_to_data_bin_by_key("CaptureCount", cnt);
}

void Set_DataBin_CaptureSpaceTime(int time) {
  set_int_to_data_bin_by_key("CaptureSpaceTime", time);
}

void Set_DataBin_SelfTimer(int time) {
  set_int_to_data_bin_by_key("SelfTimer", time);
}

void Set_DataBin_TimeLapseMode(int mode) {
  set_int_to_data_bin_by_key("TimeLapseMode", mode);
}

void Set_DataBin_SaveToSel(int sel) {
  set_int_to_data_bin_by_key("SaveTo", sel);
}

void Set_DataBin_WifiDisableTime(int time) {
  set_int_to_data_bin_by_key("WifiDisableTime", time);
}

void Set_DataBin_EthernetMode(int mode) {
  set_int_to_data_bin_by_key("EthernetMode", mode);
}

void Set_DataBin_EthernetIP(char *ip) {
  int i, size = 0, val = 0;
  char *tokens[4];
  char *del = "\\.";

  size = split_c(tokens, ip, del);
  if (size == 4) {
    for (i = 0; i < size; i++) {
      val = atoi(tokens[i]);
      EthernetIP[i] = (char)val;
      db_debug("Set_DataBin_EthernetIP() 01 EthernetIP[%d]=%d\n", i,
               EthernetIP[i]);
    }
  } else {  // error format
    for (i = 0; i < 4; i++) {
      EthernetIP[i] = 0;
      db_debug("Set_DataBin_EthernetIP() 02 EthernetIP[%d]=%d\n", i,
               EthernetIP[i]);
    }
  }
}

void Set_DataBin_EthernetMask(char *mask) {
  int i, size = 0, val = 0;
  char *tokens[4];
  char *del = "\\.";

  size = split_c(tokens, mask, del);
  if (size == 4) {
    for (i = 0; i < size; i++) {
      val = atoi(tokens[i]);
      EthernetMask[i] = (char)val;
    }
  } else {  // error format
    for (i = 0; i < 4; i++) {
      EthernetMask[i] = 0;
    }
  }
}

void Set_DataBin_EthernetGateWay(char *gateway) {
  int i, size = 0, val = 0;
  char *tokens[4];
  char *del = "\\.";

  size = split_c(tokens, gateway, del);
  if (size == 4) {
    for (i = 0; i < size; i++) {
      val = atoi(tokens[i]);
      EthernetGateWay[i] = (char)val;
    }
  } else {  // error format
    for (i = 0; i < 4; i++) {
      EthernetGateWay[i] = 0;
    }
  }
}

void Set_DataBin_EthernetDNS(char *dns) {
  int i, size = 0, val = 0;
  char *tokens[4];
  char *del = "\\.";

  size = split_c(tokens, dns, del);
  if (size == 4) {
    for (i = 0; i < size; i++) {
      val = atoi(tokens[i]);
      EthernetDNS[i] = (char)val;
    }
  } else {  // error format
    for (i = 0; i < 4; i++) {
      EthernetDNS[i] = 0;
    }
  }
}

void Set_DataBin_MediaPort(int port) {
  set_int_to_data_bin_by_key("SocketPort", port);
}

void Set_DataBin_DrivingRecord(int record) {
  set_int_to_data_bin_by_key("LoopRecording", record);
}

void Set_DataBin_US360Version(char *version) {
  int x;
  int length = strlen(version);
  int size = sizeof(US360Versin);
  for (x = 0; x < size; x++) {
    US360Versin[x] = '\0';
    if (x < length) {
      if (x == (size - 1))
        US360Versin[x] = '\0';
      else if (version[x] == '\n' || version[x] == '\r')
        US360Versin[x] = '\0';
      else
        US360Versin[x] = version[x];
    }
  }
}

void Set_DataBin_WifiChannel(int channel) {
  if (channel >= 1 && channel <= 11) {
    WifiChannel = channel;
  } else {
    WifiChannel = 6;
  }
}

void Set_DataBin_ExposureFreq(int freq) {
  if (freq == 50 || freq == 60) {
    ExposureFreq = freq;
  } else {
    ExposureFreq = 60;
  }
}

void Set_DataBin_FanControl(int ctrl) {
  set_int_to_data_bin_by_key("FanControl", ctrl);
}

void Set_DataBin_Sharpness(int sharp) {
  set_int_to_data_bin_by_key("Sharpness", sharp);
}

void Set_DataBin_UserCtrl30Fps(int ctrl) {
  set_int_to_data_bin_by_key("UserCtrl30FPS", ctrl);
}

void Set_DataBin_CameraMode(int c_mode) {
  set_int_to_data_bin_by_key("CameraMode", c_mode);
}

void Set_DataBin_ColorSTMode(int mode) {
  set_int_to_data_bin_by_key("ColorSTMode", mode);
}

void Set_DataBin_AutoGlobalPhiAdjMode(int mode) {
  set_int_to_data_bin_by_key("AutoGlobalPhiAdjMode", mode);
}

void Set_DataBin_HDMITextVisibility(int visibility) {
  set_int_to_data_bin_by_key("HDMITextVisibility", visibility);
}

void Set_DataBin_SpeakerMode(int mode) {
  set_int_to_data_bin_by_key("SpeakerEnable", mode);
}

void Set_DataBin_LedBrightness(int led) {
  set_int_to_data_bin_by_key("LEDBrightness", led);
}

void Set_DataBin_OledControl(int ctrl) {
  set_int_to_data_bin_by_key("ScreenControl", ctrl);
}

void Set_DataBin_DelayValue(int value) {
  set_int_to_data_bin_by_key("CountDown", value);
}

void Set_DataBin_ImageQuality(int quality) {
  set_int_to_data_bin_by_key("ImageQuality", quality);
}

void Set_DataBin_PhotographReso(int res) {
  set_int_to_data_bin_by_key("PhotoResolution", res);
}

void Set_DataBin_RecordReso(int res) {
  set_int_to_data_bin_by_key("RecordResolution", res);
}

void Set_DataBin_TimeLapseReso(int res) {
  set_int_to_data_bin_by_key("TimeLapseResolution", res);
}

void Set_DataBin_Translucent(int tran) {
  set_int_to_data_bin_by_key("Translucent", tran);
}

void Set_DataBin_CompassMaxx(int maxx) {
  set_int_to_data_bin_by_key("CompassMaxX", maxx);
}

void Set_DataBin_CompassMaxy(int maxy) {
  set_int_to_data_bin_by_key("CompassMaxY", maxy);
}

void Set_DataBin_CompassMaxz(int maxz) {
  set_int_to_data_bin_by_key("CompassMaxZ", maxz);
}

void Set_DataBin_CompassMinx(int minx) {
  set_int_to_data_bin_by_key("CompassMinX", minx);
}

void Set_DataBin_CompassMiny(int miny) {
  set_int_to_data_bin_by_key("CompassMinY", miny);
}

void Set_DataBin_CompassMinz(int minz) {
  set_int_to_data_bin_by_key("CompassMinZ", minz);
}

void Set_DataBin_DebugLogMode(int mode) {
  set_int_to_data_bin_by_key("DebugModeEnable", mode);
}

void Set_DataBin_BottomMode(int mode) {
  set_int_to_data_bin_by_key("NadirMode", mode);
}

void Set_DataBin_BottomSize(int size) {
  set_int_to_data_bin_by_key("NadirSize", size);
}

void Set_DataBin_hdrEvMode(int mode) {
  set_int_to_data_bin_by_key("HDREvMode", mode);
}

void Set_DataBin_Bitrate(int rate) {
  set_int_to_data_bin_by_key("RecordBitrate", rate);
}

void Set_DataBin_HttpAccount(char *account) {
  int x;
  int len = strlen(account);
  // sprintf(HttpAccount, "%s", account);
  for (x = 0; x < 32; x++) {
    if (x < len) {
      // if (account[x] == '\n' || account[x] == '\r')
      // HttpAccount[x] = '\0';
      // else
      // HttpAccount[x] = account[x];
    } else {
      // HttpAccount[x] = '\0';
    }
  }
}

void Set_DataBin_HttpPassword(char *password) {
  int x;
  int len = strlen(password);
  // sprintf(HttpPassword, "%s", password);
  for (x = 0; x < 32; x++) {
    if (x < len) {
      // if (password[x] == '\n' || password[x] == '\r')
      // HttpPassword[x] = '\0';
      // else
      // HttpPassword[x] = password[x];
    } else {
      // HttpPassword[x] = '\0';
    }
  }
}

void Set_DataBin_HttpPort(int port) {
  set_int_to_data_bin_by_key("HttpPort", port);
}

void Set_DataBin_CompassMode(int mode) {
  set_int_to_data_bin_by_key("CompassEnable", mode);
}

void Set_DataBin_GsensorMode(int mode) {
  set_int_to_data_bin_by_key("GSensorEnable", mode);
}

void Set_DataBin_CapHdrMode(int mode) {
  set_int_to_data_bin_by_key("PhotoHDRMode", mode);
}

void Set_DataBin_BottomTMode(int mode) {
  set_int_to_data_bin_by_key("NadirTextEnable", mode);
}

void Set_DataBin_BottomTColor(int color) {
  set_int_to_data_bin_by_key("NadirTextColor", color);
}

void Set_DataBin_BottomBColor(int color) {
  set_int_to_data_bin_by_key("NadirBackgroundColor", color);
}

void Set_DataBin_BottomTFont(int tfont) {
  set_int_to_data_bin_by_key("NadirTextFont", tfont);
}

void Set_DataBin_BottomTLoop(int loop) {
  set_int_to_data_bin_by_key("NadirTextLoopEnable", loop);
}

void Set_DataBin_BottomText(char *text) {
  int x;
  int len = strlen(text);
  // sprintf(bottomText, "%s", text);
  for (x = 0; x < 70; x++) {
    if (x < len) {
      // if (text[x] == '\n' || text[x] == '\r')
      // bottomText[x] = '\0';
      // else
      // bottomText[x] = text[x];
    } else {
      // bottomText[x] = 0;
    }
  }
}

void Set_DataBin_FpgaEncodeType(int type) {
  set_int_to_data_bin_by_key("TimeLapseEncodeType", type);
}

void Set_DataBin_WbRGB(char *rgb) {
  int i, size = 0, val = 0;
  char *tokens[4];
  char *del = "\\.";

  size = split_c(tokens, rgb, del);
  if (size == 3) {
    for (i = 0; i < size; i++) {
      val = atoi(tokens[i]);
      // wbRGB[i] = (char)val;
    }
  } else {  // error format
    for (i = 0; i < 4; i++) {
      // wbRGB[i] = 100;
    }
  }
}

void Set_DataBin_Contrast(int Contrast) {
  set_int_to_data_bin_by_key("Contrast", Contrast);
}

void Set_DataBin_Saturation(int Saturation) {
  set_int_to_data_bin_by_key("Saturation", Saturation);
}

void Set_DataBin_FreeCount(int FreeCount) {
  set_int_to_data_bin_by_key("FreeCount", FreeCount);
}

void Set_DataBin_SaveTimelapse(int SaveTimelapse) {
  set_int_to_data_bin_by_key("TimeLapseSaveMode", SaveTimelapse);
}

void Set_DataBin_BmodeSec(int BmodeSec) {
  set_int_to_data_bin_by_key("BModeSec", BmodeSec);
}

void Set_DataBin_BmodeGain(int BmodeGain) {
  set_int_to_data_bin_by_key("BModeGain", BmodeGain);
}

void Set_DataBin_HdrManual(int HdrManual) {
  set_int_to_data_bin_by_key("HDRManualEnable", HdrManual);
}

void Set_DataBin_HdrNumber(int HdrNumber) {
  set_int_to_data_bin_by_key("HDRNumber", HdrNumber);
}

void Set_DataBin_HdrIncrement(int HdrIncrement) {
  set_int_to_data_bin_by_key("HDRIncrement", HdrIncrement);
}

void Set_DataBin_HdrStrength(int HdrStrength) {
  set_int_to_data_bin_by_key("HDRStrength", HdrStrength);
}

void Set_DataBin_HdrTone(int HdrTone) {
  set_int_to_data_bin_by_key("HDRTone", HdrTone);
}

void Set_DataBin_AebNumber(int AebNumber) {
  set_int_to_data_bin_by_key("AEBNumber", AebNumber);
}

void Set_DataBin_AebIncrement(int AebIncrement) {
  set_int_to_data_bin_by_key("AEBIncrement", AebIncrement);
}

void Set_DataBin_LiveQualityMode(int Mode) {
  set_int_to_data_bin_by_key("LiveQuality", Mode);
}

void Set_DataBin_WbTemperature(int temp) {
  set_int_to_data_bin_by_key("WhiteBalanceTemperature", temp);
}

void Set_DataBin_HdrDeghosting(int mode) {
  set_int_to_data_bin_by_key("HDRDeghosting", mode);
}

void Set_DataBin_RemoveHdrMode(int val) {
  set_int_to_data_bin_by_key("RemovalHDRMode", val);
}

void Set_DataBin_RemoveHdrNumber(int val) {
  set_int_to_data_bin_by_key("RemovalHDRNumber", val);
}

void Set_DataBin_RemoveHdrIncrement(int val) {
  set_int_to_data_bin_by_key("RemovalHDRIncrement", val);
}

void Set_DataBin_RemoveHdrStrength(int val) {
  set_int_to_data_bin_by_key("RemovalHDRStrength", val);
}

void Set_DataBin_AntiAliasingEn(int val) {
  set_int_to_data_bin_by_key("AntiAliasingEnable", val);
}

void Set_DataBin_RemoveAntiAliasingEn(int val) {
  set_int_to_data_bin_by_key("RemovalAntiAliasingEnable", val);
}

void Set_DataBin_WbTint(int tint) {
  set_int_to_data_bin_by_key("WhiteBalanceTint", tint);
}

void Set_DataBin_HdrAutoStrength(int HdrStrength) {
  set_int_to_data_bin_by_key("HDRAutoStrength", HdrStrength);
}

void Set_DataBin_RemoveHdrAutoStrength(int val) {
  set_int_to_data_bin_by_key("RemoveHDRAutoStrength", val);
}

void Set_DataBin_LiveBitrate(int bitrate) {
  set_int_to_data_bin_by_key("LiveBitrate", bitrate);
}

void Set_DataBin_PowerSaving(int mode) {
  set_int_to_data_bin_by_key("PowerSavingEnable", mode);
}

// - MARK: Get Function

int Get_DataBin_Version() { return get_int_from_data_bin_by_key("Version"); }

int Get_DataBin_DemoMode() { return get_int_from_data_bin_by_key("DemoMode"); }

int Get_DataBin_CamPositionMode() {
  return get_int_from_data_bin_by_key("PositionMode");
}

int Get_DataBin_PlayMode() { return get_int_from_data_bin_by_key("PlayMode"); }

int Get_DataBin_ResoultMode() {
  return get_int_from_data_bin_by_key("ResoultMode");
}

int Get_DataBin_EVValue() { return get_int_from_data_bin_by_key("EVValue"); }

int Get_DataBin_MFValue() { return get_int_from_data_bin_by_key("MFValue1"); }

int Get_DataBin_MF2Value() { return get_int_from_data_bin_by_key("MFValue2"); }

int Get_DataBin_ISOValue() { return get_int_from_data_bin_by_key("ISOValue"); }

int Get_DataBin_ExposureValue() {
  return get_int_from_data_bin_by_key("ExposureValue");
}

int Get_DataBin_WBMode() {
  return get_int_from_data_bin_by_key("WhiteBlanceMode");
}

int Get_DataBin_CaptureMode() {
  return get_int_from_data_bin_by_key("CaptureMode");
}

int Get_DataBin_CaptureCnt() {
  return get_int_from_data_bin_by_key("CaptureCount");
}

int Get_DataBin_CaptureSpaceTime() {
  return get_int_from_data_bin_by_key("CaptureSpaceTime");
}

int Get_DataBin_SelfTimer() {
  return get_int_from_data_bin_by_key("SelfTimer");
}

int Get_DataBin_TimeLapseMode() {
  return get_int_from_data_bin_by_key("TimeLapseMode");
}

int Get_DataBin_SaveToSel() { return get_int_from_data_bin_by_key("SaveTo"); }

int Get_DataBin_WifiDisableTime() {
  return get_int_from_data_bin_by_key("WifiDisableTime");
}

int Get_DataBin_EthernetMode() {
  return get_int_from_data_bin_by_key("EthernetMode");
}

void Get_DataBin_EthernetIP(char *ip, int size) {
  char ip_tmp[64];
  sprintf(ip_tmp, "%d.%d.%d.%d\0", EthernetIP[0], EthernetIP[1], EthernetIP[2],
          EthernetIP[3]);
  if (strlen(ip_tmp) < size)
    sprintf(ip, "%s\0", ip_tmp);
  else
    memcpy(ip, &ip_tmp[0], size);
}

void Get_DataBin_EthernetMask(char *mask, int size) {
  char mask_tmp[64];
  sprintf(mask_tmp, "%d.%d.%d.%d\0", EthernetMask[0], EthernetMask[1],
          EthernetMask[2], EthernetMask[3]);
  if (strlen(mask_tmp) < size)
    sprintf(mask, "%s\0", mask_tmp);
  else
    memcpy(mask, &mask_tmp[0], size);
}

void Get_DataBin_EthernetGateWay(char *gateway, int size) {
  char gateway_tmp[64];
  sprintf(gateway_tmp, "%d.%d.%d.%d\0", EthernetGateWay[0], EthernetGateWay[1],
          EthernetGateWay[2], EthernetGateWay[3]);
  if (strlen(gateway_tmp) < size)
    sprintf(gateway, "%s\0", gateway_tmp);
  else
    memcpy(gateway, &gateway_tmp[0], size);
}

void Get_DataBin_EthernetDNS(char *dns, int size) {
  char dns_tmp[64];
  sprintf(dns_tmp, "%d.%d.%d.%d\0", EthernetDNS[0], EthernetDNS[1],
          EthernetDNS[2], EthernetDNS[3]);
  if (strlen(dns_tmp) < size)
    sprintf(dns, "%s\0", dns_tmp);
  else
    memcpy(dns, &dns_tmp[0], size);
}

int Get_DataBin_MediaPort() {
  return get_int_from_data_bin_by_key("SocketPort");
}

int Get_DataBin_DrivingRecord() {
  return get_int_from_data_bin_by_key("LoopRecording");
}

void Get_DataBin_US360Version(char *ver, int size) {
  int i;
  int sz = sizeof(US360Versin);
  for (i = 0; i < sz; i++) {
    if (i == (size - 1)) {
      ver[i] = '\0';
      break;
    }
    if (US360Versin[i] == '\n' || US360Versin[i] == '\r')
      ver[i] = '\0';
    else
      ver[i] = US360Versin[i];
  }
}

int Get_DataBin_WifiChannel() {
  return get_int_from_data_bin_by_key("WifiChannel");
}

int Get_DataBin_ExposureFreq() {
  return get_int_from_data_bin_by_key("ExposureFreq");
}

int Get_DataBin_FanControl() {
  return get_int_from_data_bin_by_key("FanControl");
}

int Get_DataBin_Sharpness() {
  return get_int_from_data_bin_by_key("Sharpness");
}

int Get_DataBin_UserCtrl30Fps() {
  return get_int_from_data_bin_by_key("UserCtrl30FPS");
}

int Get_DataBin_CameraMode() {
  return get_int_from_data_bin_by_key("CameraMode");
}

int Get_DataBin_ColorSTMode() {
  return get_int_from_data_bin_by_key("ColorSTMode");
}

int Get_DataBin_AutoGlobalPhiAdjMode() {
  return get_int_from_data_bin_by_key("AutoGlobalPhiAdjMode");
}

int Get_DataBin_HDMITextVisibility() {
  return get_int_from_data_bin_by_key("HDMITextVisibility");
}

int Get_DataBin_SpeakerMode() {
  return get_int_from_data_bin_by_key("SpeakerEnable");
}

int Get_DataBin_LedBrightness() {
  return get_int_from_data_bin_by_key("LEDBrightness");
}

int Get_DataBin_OledControl() {
  return get_int_from_data_bin_by_key("ScreenControl");
}

int Get_DataBin_DelayValue() {
  return get_int_from_data_bin_by_key("CountDown");
}

int Get_DataBin_ImageQuality() {
  return get_int_from_data_bin_by_key("ImageQuality");
}

int Get_DataBin_PhotographReso() {
  return get_int_from_data_bin_by_key("PhotoResolution");
}

int Get_DataBin_RecordReso() {
  return get_int_from_data_bin_by_key("RecordResolution");
}

int Get_DataBin_TimeLapseReso() {
  return get_int_from_data_bin_by_key("TimeLapseResolution");
}

int Get_DataBin_Translucent() {
  return get_int_from_data_bin_by_key("Translucent");
}

int Get_DataBin_CompassMaxx() {
  return get_int_from_data_bin_by_key("CompassMaxX");
}

int Get_DataBin_CompassMaxy() {
  return get_int_from_data_bin_by_key("CompassMaxY");
}

int Get_DataBin_CompassMaxz() {
  return get_int_from_data_bin_by_key("CompassMaxZ");
}

int Get_DataBin_CompassMinx() {
  return get_int_from_data_bin_by_key("CompassMinX");
}

int Get_DataBin_CompassMiny() {
  return get_int_from_data_bin_by_key("CompassMinY");
}

int Get_DataBin_CompassMinz() {
  return get_int_from_data_bin_by_key("CompassMinZ");
}

int Get_DataBin_DebugLogMode() {
  return get_int_from_data_bin_by_key("DebugModeEnable");
}

int Get_DataBin_BottomMode() {
  return get_int_from_data_bin_by_key("NadirMode");
}

int Get_DataBin_BottomSize() {
  return get_int_from_data_bin_by_key("NadirSize");
}

int Get_DataBin_hdrEvMode() {
  return get_int_from_data_bin_by_key("HDREvMode");
}

int Get_DataBin_Bitrate() {
  return get_int_from_data_bin_by_key("RecordBitrate");
}

void Get_DataBin_HttpAccount(char *account, int size) {
  // char def_acc[32] = "admin\0";
  // if (strlen(HttpAccount) < size)
  //   sprintf(account, "%s\0", &HttpAccount[0]);
  // else
  //   memcpy(account, &HttpAccount[0], size);
  // if (strcmp(account, "") == 0) {
  //   if (strlen(def_acc) < size)
  //     sprintf(account, "%s\0", def_acc);
  //   else
  //     memcpy(account, &def_acc[0], size);
  //   Set_DataBin_HttpAccount(account);
  // }
}

void Get_DataBin_HttpPassword(char *pwd, int size) {
  // char def_pwd[32] = "admin\0";
  // if (strlen(HttpPassword) < size)
  //   sprintf(pwd, "%s\0", &HttpPassword[0]);
  // else
  //   memcpy(pwd, &HttpPassword[0], size);
  // if (strcmp(pwd, "") == 0) {
  //   if (strlen(def_pwd) < size)
  //     sprintf(pwd, "%s\0", def_pwd);
  //   else
  //     memcpy(pwd, &def_pwd[0], size);
  //   Set_DataBin_HttpPassword(pwd);
  // }
}

int Get_DataBin_HttpPort() { return get_int_from_data_bin_by_key("HttpPort"); }

int Get_DataBin_CompassMode() {
  return get_int_from_data_bin_by_key("CompassEnable");
}

int Get_DataBin_GsensorMode() {
  return get_int_from_data_bin_by_key("GSensorEnable");
}

int Get_DataBin_CapHdrMode() {
  return get_int_from_data_bin_by_key("PhotoHDRMode");
}

int Get_DataBin_BottomTMode() {
  return get_int_from_data_bin_by_key("NadirTextEnable");
}

int Get_DataBin_BottomTColor() {
  return get_int_from_data_bin_by_key("NadirTextColor");
}

int Get_DataBin_BottomBColor() {
  return get_int_from_data_bin_by_key("NadirBackgroundColor");
}

int Get_DataBin_BottomTFont() {
  return get_int_from_data_bin_by_key("NadirTextFont");
}

int Get_DataBin_BottomTLoop() {
  return get_int_from_data_bin_by_key("NadirTextLoopEnable");
}

void Get_DataBin_BottomText(char *text, int size) {
  get_char_array_from_data_bin_by_key("NadirText", text);
  // if (strlen(bottomText) < size)
  //   sprintf(text, "%s\0", &bottomText[0]);
  // else
  //   memcpy(text, &bottomText[0], size);
}

int Get_DataBin_FpgaEncodeType() {
  return get_int_from_data_bin_by_key("TimeLapseEncodeType");
}

void Get_DataBin_WbRGB(char *rgb, int size) {
  // char rgb_tmp[16];
  // sprintf(rgb_tmp, "%d.%d.%d\0", wbRGB[0], wbRGB[1], wbRGB[2]);
  // if (strlen(rgb_tmp) < size)
  //   sprintf(rgb, "%s\0", rgb_tmp);
  // else
  //   memcpy(rgb, &rgb_tmp[0], size);
}

int Get_DataBin_Contrast() { return get_int_from_data_bin_by_key("Contrast"); }

int Get_DataBin_Saturation() {
  return get_int_from_data_bin_by_key("Saturation");
}

int Get_DataBin_FreeCount() {
  return get_int_from_data_bin_by_key("FreeCount");
}

int Get_DataBin_SaveTimelapse() {
  return get_int_from_data_bin_by_key("TimeLapseSaveMode");
}

int Get_DataBin_BmodeSec() { return get_int_from_data_bin_by_key("BModeSec"); }

int Get_DataBin_BmodeGain() {
  return get_int_from_data_bin_by_key("BModeGain");
}

int Get_DataBin_HdrManual() {
  return get_int_from_data_bin_by_key("HDRManualEnable");
}

int Get_DataBin_HdrNumber() {
  return get_int_from_data_bin_by_key("HDRNumber");
}

int Get_DataBin_HdrIncrement() {
  return get_int_from_data_bin_by_key("HDRIncrement");
}

int Get_DataBin_HdrStrength() {
  return get_int_from_data_bin_by_key("HDRStrength");
}

int Get_DataBin_HdrTone() { return get_int_from_data_bin_by_key("HDRTone"); }

int Get_DataBin_AebNumber() {
  return get_int_from_data_bin_by_key("AEBNumber");
}

int Get_DataBin_AebIncrement() {
  return get_int_from_data_bin_by_key("AEBIncrement");
}

int Get_DataBin_LiveQualityMode() {
  return get_int_from_data_bin_by_key("LiveQuality");
}

int Get_DataBin_WbTemperature() {
  return get_int_from_data_bin_by_key("WhiteBalanceTemperature");
}

int Get_DataBin_HdrDeghosting() {
  return get_int_from_data_bin_by_key("HDRDeghosting");
}

int Get_DataBin_RemoveHdrMode() {
  return get_int_from_data_bin_by_key("RemovalHDRMode");
}

int Get_DataBin_RemoveHdrNumber() {
  return get_int_from_data_bin_by_key("RemovalHDRNumber");
}

int Get_DataBin_RemoveHdrIncrement() {
  return get_int_from_data_bin_by_key("RemovalHDRIncrement");
}

int Get_DataBin_RemoveHdrStrength() {
  return get_int_from_data_bin_by_key("RemovalHDRStrength");
}

int Get_DataBin_AntiAliasingEn() {
  return get_int_from_data_bin_by_key("AntiAliasingEnable");
}

int Get_DataBin_RemoveAntiAliasingEn() {
  return get_int_from_data_bin_by_key("RemovalAntiAliasingEnable");
}

int Get_DataBin_WbTint() {
  return get_int_from_data_bin_by_key("WhiteBalanceTint");
}

int Get_DataBin_HdrAutoStrength() {
  return get_int_from_data_bin_by_key("HDRAutoStrength");
}

int Get_DataBin_RemoveHdrAutoStrength() {
  return get_int_from_data_bin_by_key("RemoveHDRAutoStrength");
}

int Get_DataBin_LiveBitrate() {
  return get_int_from_data_bin_by_key("LiveBitrate");
}

int Get_DataBin_PowerSaving() {
  return get_int_from_data_bin_by_key("PowerSavingEnable");
}

// - MARK: Other Function

static int getBytes(char *buffer) { return get_raw_data_bin(buffer); }

static void writeDatabin2Path(char *filePath) { save_data_bin(); }

//static int versionDate = 190507;

//int GetDataBinVersionDate(void) { return versionDate; }

//void SetDataBinVersionDate(int ver) { versionDate = ver; }

#define DATABIN_VERSION_DATE    190507
int Get_DataBin_Now_Version(void) {
	return DATABIN_VERSION_DATE;
}

int split_c(char **buf, char *str, char *del) {
  int cnt = 0;
  char *s_tmp = strtok(str, del);
  while (s_tmp != NULL) {
    *buf++ = s_tmp;
    s_tmp = strtok(NULL, del);
    cnt++;
  }
  return cnt;
}

void WriteUS360DataBin() { save_data_bin(); }

void ReadUS360DataBin(int country, int customer) {
  if (!is_data_bin_created()) {
    init_data_bin(CheckExpFreqDefault(country), customer);
  }
}

void DeleteUS360DataBin() { delete_data_bin(); }

int CheckExpFreqDefault(int country) {
  int freq = 60;
  freq = CheckCountryFreq(country);
  return freq;
}
