/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include <Device/databin.h>
#include <Foundation.h>
#include <System/endian.h>
#include <System/logger.h>
#include <common/extension/filesystem.h>
#include <common/extension/vector.h>

#include "Device/US363/us360.h"

Json Device::DataBin::default_value_ = {
    {KeyString(Keys::Version), 170906},

    // 畫面移動模式, 0: G-sensor 1: 慣性(有摩擦力) 2: 慣性(無摩擦力)
    {KeyString(Keys::DemoMode), 1},

    // G-sensor方向, 0: AUTO 1: 0度 2: 180度 3: 90度
    {KeyString(Keys::PositionMode), 0},

    // 畫面觀看模式, 0: Global 1: Front 2:360 3: 240 4: 180 5: 4分割
    {KeyString(Keys::PlayMode), 0},

    // 畫面解析度, 0:Fix 1:12K(Global) 2:4K(Global) 7:8K(Global)
    // 8:10K(Global) 12:6K(Global) 13:3K(Global) 14:2K(Global)
    {KeyString(Keys::ResoultMode), 1},

    // EV值, -6: EV-2 -5: EV-1.6
    // -4: EV-1.3 -3: EV-1 -2: EV-0.6 -1: EV-0.3 0: EV+0 1:
    // EV+0.3 2: EV+0.6 3: EV+1 4: EV+1.3 5: EV+1.6 6: EV+2
    {KeyString(Keys::EVValue), 0},

    // MF值, -32 ~ 32(目前實際值為 -27 ~ 36)
    {KeyString(Keys::MFValue1), -9},

    // MF2值, -32 ~ 32(目前實際值為 -27 ~ 36)
    {KeyString(Keys::MFValue2), -9},

    // ISO值, -1: AUTO 0: ISO
    // 100 20: ISO 200 40: ISO 400 60: ISO 800
    // 80: ISO 1600 120: ISO 3200 140: ISO 6400
    {KeyString(Keys::ISOValue), -1},

    // 曝光時間, -1: AUTO 260: 1 240: 1/2
    // 220: 1/4 200: 1/8 180: 1/15 160: 1/30 140:
    // 1/60 120: 1/120 100: 1/250 80: 1/500 60: 1/1000
    // 40: 1/2000 20: 1/4000 0: 1/8000  -20: 1/16000  -40: 1/32000
    {KeyString(Keys::ExposureValue), -1},

    // 白平衡模式, 0: AUTO, 1: Filament Lamp, 2: Daylight Lamp,
    // 3: Sun, 4: Cloudy, >1000: 色溫
    {KeyString(Keys::WhiteBlanceMode), 0},

    // 拍照模式, -1: 連拍, 0: 一般拍照, 2:2秒自拍, 10:10秒自拍
    {KeyString(Keys::CaptureMode), 0},

    // 連拍次數, 0: 1次, 2: 2次, 3: 3次
    {KeyString(Keys::CaptureCount), 0},

    // 連拍間隔時間, 500: 500ms, 1000: 1000ms
    {KeyString(Keys::CaptureSpaceTime), 160},

    // 自拍倒數, 0: none, 2: 2秒自拍, 10: 10秒自拍
    {KeyString(Keys::SelfTimer), 0},

    // 縮時錄影模式, 0: none, 1: 1s, 2: 2s, 3: 5s, 4: 10s, 5: 30s, 6: 60s,
    // 7:0.166s
    {KeyString(Keys::TimeLapseMode), 0},

    // 儲存空間位置, 0: Internal Storage, 1: MicroSD
    {KeyString(Keys::SaveTo), 1},

    {KeyString(Keys::EthernetIP), ""},

    {KeyString(Keys::EthernetMask), ""},

    {KeyString(Keys::EthernetGateway), ""},

    {KeyString(Keys::EthernetDNS), ""},

    // wifi自動關閉時間, -2:not disable, 60:1分鐘, 120:2分鐘, 180:3分鐘,
    // 300:5分鐘
    {KeyString(Keys::WifiDisableTime), 300},

    // Ethernet 模式, 0:DHCP, 1:MANUAL
    {KeyString(Keys::EthernetMode), 0},

    // Ethernet IP, 0.0.0.0
    {KeyString(Keys::EthernetIP), 0},

    // MediaPort, 1~65535
    {KeyString(Keys::SocketPort), 8555},

    // 循環錄影, 0: off, 1: on
    {KeyString(Keys::LoopRecording), 1},

    {KeyString(Keys::US360Version), ""},

    // Wifi Channel
    {KeyString(Keys::WifiChannel), 6},

    // EP freq, 60:60Hz, 50:50Hz
    {KeyString(Keys::ExposureFreq), 60},

    // 風扇控制, 0: off, 1: fast, 2: median, 3: slow
    {KeyString(Keys::FanControl), 2},

    // Sharpness, 0 ~ 15
    {KeyString(Keys::Sharpness), 8},

    // User控制30FPS,		0:off 1:on
    {KeyString(Keys::UserCtrl30FPS), 0},

    // 攝影機模式, 0:Cap, 1:Rec, 2:Time_Lapse, 3:Cap_HDR, 4:Cap_5_Sensor,
    // 5:日拍HDR, 6:夜拍, 7:夜拍HDR, 8:Sport, 9:SportWDR, 10:RecWDR,
    // 11:Time_LapseWDR, 12:B快門, 13:動態移除 , 14: 3D-Model
    {KeyString(Keys::CameraMode), 5},

    // 顏色縫合模式, 0:off, 1:on, 2:auto
    {KeyString(Keys::ColorSTMode), 1},

    // 即時縫合模式, 0~100
    {KeyString(Keys::AutoGlobalPhiAdjMode), 100},

    // HDMI文字訊息, 0:hide 1:show
    {KeyString(Keys::HDMITextVisibility), 1},

    // 喇叭, 0:On 1:Off
    {KeyString(Keys::SpeakerEnable), 0},

    // LED亮度 -1:自動,亮度:0 ~ 100
    {KeyString(Keys::LEDBrightness), -1},

    // OLED控制 0:On 1:Off
    {KeyString(Keys::ScreenControl), 0},

    //延遲時間控制 0:none 2:2s 5:5s 10:10s 20:20s
    {KeyString(Keys::CountDown), 0},

    // 影像品質			0:高,1:中,2:低
    {KeyString(Keys::ImageQuality), 0},

    // 拍照解析度			1:12k,7:8k,12:6k
    {KeyString(Keys::PhotoResolution), 1},

    // 錄影解析度			2:4k,13:3k,14:2k
    {KeyString(Keys::RecordResolution), 2},

    // 縮時解析度			1:12k,7:8k,12:6k,2:4k
    {KeyString(Keys::TimeLapseResolution), 12},

    // 半透明設定			0:關,1:開,2:自動
    {KeyString(Keys::Translucent), 1},

    // 電子羅盤X軸最大值
    {KeyString(Keys::CompassMaxX), -60000},

    // 電子羅盤y軸最大值
    {KeyString(Keys::CompassMaxY), -60000},

    // 電子羅盤z軸最大值
    {KeyString(Keys::CompassMaxZ), -60000},

    // 電子羅盤X軸最小值
    {KeyString(Keys::CompassMinX), 60000},

    // 電子羅盤y軸最小值
    {KeyString(Keys::CompassMinY), 60000},

    // 電子羅盤z軸最小值
    {KeyString(Keys::CompassMinZ), 60000},

    // 除錯訊息存至SD卡 0:關, 1:開
    {KeyString(Keys::DebugModeEnable), 0},

    // 底圖設定 0:關, 1:延伸, 2:底圖(default), 3:鏡像, 4:底圖(user)
    {KeyString(Keys::NadirMode), 2},

    // 底圖大小				10 ~ 100
    {KeyString(Keys::NadirSize), 50},

    // HDR EV設定, 2:0,-1,-2 4:+1,-1,-3 8:+2,-1,-4
    {KeyString(Keys::HDREvMode), 4},

    // Rec Bitrate 控制,		0:最高 5:中等 10:最低
    {KeyString(Keys::RecordBitrate), 5},

    // 網頁/RTSP帳號
    {KeyString(Keys::HttpAccount), ""},

    // 網頁/RTSP密碼
    {KeyString(Keys::HttpPassword), ""},

    // HttpPort,		1024 ~ 65534
    {KeyString(Keys::HttpPort), 8080},

    // 電子羅盤開關		0:關 1:開
    {KeyString(Keys::CompassEnable), 1},

    // G-Sensor開關		0:關 1:開
    {KeyString(Keys::GSensorEnable), 1},

    // 拍照HDR模式開關	0:日拍, 1:日拍HDR, 2:夜拍, 3:夜拍HDR
    {KeyString(Keys::PhotoHDRMode), 1},

    // 底圖文字控制		0:關 1:開
    {KeyString(Keys::NadirTextEnable), 0},

    // 底圖文字顏色代碼	0:white 1:bright_pink 2:red 3:orange 4:yellow
    // 5:chartreuse 6:green 7:spring_green 8:cyan 9:azure 10:blue 11:violet
    // 12:magenta 13:black
    {KeyString(Keys::NadirTextColor), 0},

    // 底圖文字背景顏色代碼 0:white 1:bright_pink 2:red 3:orange 4:yellow
    // 5:chartreuse 6:green 7:spring_green 8:cyan 9:azure 10:blue 11:violet
    // 12:magenta 13:black
    {KeyString(Keys::NadirBackgroundColor), 13},

    // 底圖文字字型 0:Default 1:Default_bold 2:MONOSPACE 3:SANS_SERIF 4:SERIF
    {KeyString(Keys::NadirTextFont), 0},

    // 底圖循環顯示 0:關 1:開
    {KeyString(Keys::NadirTextLoopEnable), 1},

    // 底圖文字
    {KeyString(Keys::NadirText), std::array<char, 70>()},

    // 縮時錄影檔案類型	0:JPEG 1:H.264
    {KeyString(Keys::TimeLapseEncodeType), 1},

    // 白平衡RGB數值(0~255)	R.G.B.0
    {KeyString(Keys::WhiteBalanceRGB), 0x64646400},

    // 對比度(暫定-7 ~ 7)
    {KeyString(Keys::Contrast), 0},

    // 彩度(暫定-7 ~ 7)
    {KeyString(Keys::Saturation), 0},

    // 剩餘拍攝張數/錄影時間
    {KeyString(Keys::FreeCount), -1},

    // 儲存縮時模式 0: none	1: 1s	2: 2s	3: 5s	4: 10s	5: 30s
    // 6: 60s 7:0.166s
    {KeyString(Keys::TimeLapseSaveMode), 0},

    // B快門時間
    {KeyString(Keys::BModeSec), 4},

    // B快門ISO
    {KeyString(Keys::BModeGain), 0},

    // HDR手動模式開關 0:關  1:開  2:Auto
    {KeyString(Keys::HDRManualEnable), 2},

    // HDR手動模式張數 3,5,7
    {KeyString(Keys::HDRNumber), 5},

    // HDR手動模式EV值 5:+-0.5 10:+-1.0 15:+-1.5 20:+-2.0 25:+-2.5
    {KeyString(Keys::HDRIncrement), 10},

    // HDR手動模式強度 30 ~ 90
    {KeyString(Keys::HDRStrength), 60},

    // HDR手動模式影調壓縮 -10 ~ 10
    {KeyString(Keys::HDRTone), 0},

    // AEB手動模式張數	3,5,7
    {KeyString(Keys::AEBNumber), 7},

    // AEB手動模式EV值	5:+-0.5 10:+-1.0 15:+-1.5 20:+-2.0 25:+-2.5
    {KeyString(Keys::AEBIncrement), 10},

    // Live Quality Mode 0:10FPS Hight Quality	1:5FPS Low Quality
    {KeyString(Keys::LiveQuality), 0},

    // 白平衡色溫值		20 ~ 100(2200K ~ 10000K)
    {KeyString(Keys::WhiteBalanceTemperature), 50},

    // HDR去鬼影 0:關 1:開
    {KeyString(Keys::HDRDeghosting), 0},

    // 動態移除模式		0:關 1:自動 2:弱 3:中 4:強 5:自訂
    {KeyString(Keys::RemovalHDRMode), 1},

    // Not Use
    {KeyString(Keys::RemovalHDRNumber), 5},

    // 動態移除HDR EV值 	5,10,15,20,25
    {KeyString(Keys::RemovalHDRIncrement), 10},

    // 動態移除HDR強度	30 ~ 90
    {KeyString(Keys::RemovalHDRStrength), 60},

    // 反鋸齒			0:off 1:on
    {KeyString(Keys::AntiAliasingEnable), 1},

    // 動態移除反鋸齒	0:off 1:on
    {KeyString(Keys::RemovalAntiAliasingEnable), 1},

    // 白平衡Tint		-15 ~ +15
    {KeyString(Keys::WhiteBalanceTint), 0},

    // HDR Auto模式強度	30 ~ 90
    {KeyString(Keys::HDRAutoStrength), 60},

    // 動態移除HDR Auto強度	30 ~ 90
    {KeyString(Keys::RemoveHDRAutoStrength), 60},

    // Live Bitrate 控制,		0:最高 5:中等 10:最低
    {KeyString(Keys::LiveBitrate), 5},

    // 省電模式 			0:Off 1:On
    {KeyString(Keys::PowerSavingEnable), 0}};

char Device::DataBin::path[] = "/data/databin";

bool is_data_bin_created() {
  return Device::DataBin::instance().is_data_bin_exsit_;
}

void init_data_bin(int freq, int customer) {
  Device::DataBin::instance().SetDefaultTo(freq, customer);
}

void save_data_bin() { Device::DataBin::instance().Save(); }

void delete_data_bin() { Device::DataBin::instance().Delete(); }

int get_raw_data_bin(char* buffer) {
  Device::DataBin::instance().RawDataBin(buffer);
}

void set_int_to_data_bin_by_key(const char* key, int value) {
  Device::DataBin::instance().SetValue(key, value);
}

void set_char_array_to_data_bin_by_key(const char* key, char* array, int size) {
  // Log(LogLevel::Debug) << '\n';
}

int get_int_from_data_bin_by_key(const char* key) {
  return Device::DataBin::instance().GetValue<int>(key);
}

void get_char_array_from_data_bin_by_key(const char* key, char* save_to) {
  // auto string = Device::DataBin::instance().GetValue<std::string>(key);
  // std::copy(string.begin(), string.end(), save_to);
}

Device::DataBin::DataBin() : logger(System::Logger(std::string{"DataBin"})) {
  Log(LogLevel::Debug) << '\n';
  if (filesystem::exists(path)) {
    std::ifstream file{path};
    data_bin_ << file;
    is_data_bin_exsit_ = true;
  } else {
    data_bin_ = {};
  }
  Log(LogLevel::Debug) << '\n';
}

Device::DataBin::~DataBin() { Save(); }

/* * * * * 其他成員 * * * * */

void Device::DataBin::SetDefaultTo(int freq, int customer) {
  Log(LogLevel::Debug) << '\n';
  SetValue(Keys::ExposureFreq, freq);
  switch (customer) {
    case CUSTOMER_CODE_PIIQ:                  // PIIQ Default
      SetValue(Keys::ResoultMode, 1);         // ResoultMode: 12K
      SetValue(Keys::CameraMode, 5);          // CameraMode: HDR
      SetValue(Keys::HDRManualEnable, 1);     // HDR Manual: On
      SetValue(Keys::HDRNumber, 7);           // Frame: 7 Cnt
      SetValue(Keys::HDRIncrement, 15);       // EV: +-1.5
      SetValue(Keys::HDRStrength, 40);        // Strength: 40
      SetValue(Keys::HDRDeghosting, 0);       // DeGhost: Off
      SetValue(Keys::AntiAliasingEnable, 0);  // Anti-Aliasing: Off
      SetValue(Keys::NadirMode, 2);           // 底圖: Default
      SetValue(Keys::NadirTextEnable, 0);     // 底圖文字: Off
      SetValue(Keys::LoopRecording, 0);       // DrivingRecord: On
      break;
    case CUSTOMER_CODE_ALIBABA:
      SetValue(Keys::PowerSavingEnable, 1);
      break;
    default:
      break;
  }
  Log(LogLevel::Debug) << '\n';
  Save();
}

int Device::DataBin::RawDataBin(char* buffer) {
  Log(LogLevel::Debug) << '\n';
  std::vector<char> result;
  Insert(result, hton(GetValue<int>(Keys::Version)));
  Insert(result, hton(GetValue<int>(Keys::DemoMode)));
  Insert(result, hton(GetValue<int>(Keys::PositionMode)));
  Insert(result, hton(GetValue<int>(Keys::PlayMode)));
  Insert(result, hton(GetValue<int>(Keys::ResoultMode)));
  Insert(result, hton(GetValue<int>(Keys::EVValue)));
  Insert(result, hton(GetValue<int>(Keys::MFValue1)));
  Insert(result, hton(GetValue<int>(Keys::MFValue2)));
  Insert(result, hton(GetValue<int>(Keys::ISOValue)));
  Insert(result, hton(GetValue<int>(Keys::ExposureValue)));
  Insert(result, hton(GetValue<int>(Keys::WhiteBlanceMode)));
  Insert(result, hton(GetValue<int>(Keys::CaptureMode)));
  Insert(result, hton(GetValue<int>(Keys::CaptureCount)));
  Insert(result, hton(GetValue<int>(Keys::CaptureSpaceTime)));
  Insert(result, hton(GetValue<int>(Keys::SelfTimer)));
  Insert(result, hton(GetValue<int>(Keys::TimeLapseMode)));
  Insert(result, hton(GetValue<int>(Keys::SaveTo)));
  Insert(result, hton(GetValue<int>(Keys::WifiDisableTime)));
  Insert(result, hton(GetValue<int>(Keys::EthernetMode)));
  Insert(result, std::array<char, 4>());  // EthernetIP
  Insert(result, std::array<char, 4>());  // EthernetMask
  Insert(result, std::array<char, 4>());  // EthernetGateWay
  Insert(result, std::array<char, 4>());  // EthernetDNS
  Insert(result, hton(GetValue<int>(Keys::SocketPort)));
  Insert(result, hton(GetValue<int>(Keys::LoopRecording)));
  Insert(result, std::array<char, 16>());  // US360Version
  Insert(result, hton(GetValue<int>(Keys::WifiChannel)));
  Insert(result, hton(GetValue<int>(Keys::ExposureFreq)));
  Insert(result, hton(GetValue<int>(Keys::FanControl)));
  Insert(result, hton(GetValue<int>(Keys::Sharpness)));
  Insert(result, hton(GetValue<int>(Keys::UserCtrl30FPS)));
  Insert(result, hton(GetValue<int>(Keys::CameraMode)));
  Insert(result, hton(GetValue<int>(Keys::ColorSTMode)));
  Insert(result, hton(GetValue<int>(Keys::AutoGlobalPhiAdjMode)));
  Insert(result, hton(GetValue<int>(Keys::HDMITextVisibility)));
  Insert(result, hton(GetValue<int>(Keys::SpeakerEnable)));
  Insert(result, hton(GetValue<int>(Keys::LEDBrightness)));
  Insert(result, hton(GetValue<int>(Keys::ScreenControl)));
  Insert(result, hton(GetValue<int>(Keys::CountDown)));
  Insert(result, hton(GetValue<int>(Keys::ImageQuality)));
  Insert(result, hton(GetValue<int>(Keys::PhotoResolution)));
  Insert(result, hton(GetValue<int>(Keys::RecordResolution)));
  Insert(result, hton(GetValue<int>(Keys::TimeLapseResolution)));
  Insert(result, hton(GetValue<int>(Keys::Translucent)));
  Insert(result, hton(GetValue<int>(Keys::CompassMaxX)));
  Insert(result, hton(GetValue<int>(Keys::CompassMaxY)));
  Insert(result, hton(GetValue<int>(Keys::CompassMaxZ)));
  Insert(result, hton(GetValue<int>(Keys::CompassMinX)));
  Insert(result, hton(GetValue<int>(Keys::CompassMinY)));
  Insert(result, hton(GetValue<int>(Keys::CompassMinZ)));
  Insert(result, hton(GetValue<int>(Keys::DebugModeEnable)));
  Insert(result, hton(GetValue<int>(Keys::NadirMode)));
  Insert(result, hton(GetValue<int>(Keys::NadirSize)));
  Insert(result, hton(GetValue<int>(Keys::HDREvMode)));
  Insert(result, hton(GetValue<int>(Keys::RecordBitrate)));
  Insert(result, std::array<char, 32>());  // HttpAccount
  Insert(result, std::array<char, 32>());  // HttpPassword
  Insert(result, hton(GetValue<int>(Keys::HttpPort)));
  Insert(result, hton(GetValue<int>(Keys::CompassEnable)));
  Insert(result, hton(GetValue<int>(Keys::GSensorEnable)));
  Insert(result, hton(GetValue<int>(Keys::PhotoHDRMode)));
  Insert(result, hton(GetValue<int>(Keys::NadirTextEnable)));
  Insert(result, hton(GetValue<int>(Keys::NadirTextColor)));
  Insert(result, hton(GetValue<int>(Keys::NadirBackgroundColor)));
  Insert(result, hton(GetValue<int>(Keys::NadirTextFont)));
  Insert(result, hton(GetValue<int>(Keys::NadirTextLoopEnable)));
  // Insert(result, GetValue<std::array<char, 70>>(Keys::NadirText));
  Insert(result, std::array<char, 70>());
  Insert(result, hton(GetValue<int>(Keys::TimeLapseEncodeType)));
  Insert(result, hton(GetValue<int>(Keys::WhiteBalanceRGB)));
  Insert(result, hton(GetValue<int>(Keys::Contrast)));
  Insert(result, hton(GetValue<int>(Keys::Saturation)));
  Insert(result, hton(GetValue<int>(Keys::FreeCount)));
  Insert(result, hton(GetValue<int>(Keys::TimeLapseSaveMode)));
  Insert(result, hton(GetValue<int>(Keys::BModeSec)));
  Insert(result, hton(GetValue<int>(Keys::BModeGain)));
  Insert(result, hton(GetValue<int>(Keys::HDRManualEnable)));
  Insert(result, hton(GetValue<int>(Keys::HDRNumber)));
  Insert(result, hton(GetValue<int>(Keys::HDRIncrement)));
  Insert(result, hton(GetValue<int>(Keys::HDRStrength)));
  Insert(result, hton(GetValue<int>(Keys::HDRTone)));
  Insert(result, hton(GetValue<int>(Keys::AEBNumber)));
  Insert(result, hton(GetValue<int>(Keys::AEBIncrement)));
  Insert(result, hton(GetValue<int>(Keys::LiveQuality)));
  Insert(result, hton(GetValue<int>(Keys::WhiteBalanceTemperature)));
  Insert(result, hton(GetValue<int>(Keys::HDRDeghosting)));
  Insert(result, hton(GetValue<int>(Keys::RemovalHDRMode)));
  Insert(result, hton(GetValue<int>(Keys::RemovalHDRNumber)));
  Insert(result, hton(GetValue<int>(Keys::RemovalHDRIncrement)));
  Insert(result, hton(GetValue<int>(Keys::RemovalHDRStrength)));
  Insert(result, hton(GetValue<int>(Keys::AntiAliasingEnable)));
  Insert(result, hton(GetValue<int>(Keys::RemovalAntiAliasingEnable)));
  Insert(result, hton(GetValue<int>(Keys::WhiteBalanceTint)));
  Insert(result, hton(GetValue<int>(Keys::HDRAutoStrength)));
  Insert(result, hton(GetValue<int>(Keys::RemoveHDRAutoStrength)));
  Insert(result, hton(GetValue<int>(Keys::LiveBitrate)));
  Insert(result, hton(GetValue<int>(Keys::PowerSavingEnable)));
  std::copy(result.begin(), result.end(), buffer);
  Log(LogLevel::Debug) << '\n';
  return result.size();
}

void Device::DataBin::Save() {
  std::ofstream file{path};
  file << data_bin_;
}

void Device::DataBin::Delete() {
  if (filesystem::exists(path)) filesystem::remove(path);
}
