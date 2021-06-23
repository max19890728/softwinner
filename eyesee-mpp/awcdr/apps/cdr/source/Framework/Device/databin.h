/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <Foundation.h>
#include <System/logger.h>
#include <linux/types.h>

#include <common/utils/json.hpp>
#include <map>
#include <memory>
#include <set>

#define DATA_BIN_KEY_TABLE                                  \
  X(Version, "Version")                                     \
  X(DemoMode, "DemoMode")                                   \
  X(PositionMode, "PositionMode")                           \
  X(PlayMode, "PlayMode")                                   \
  X(ResoultMode, "ResoultMode")                             \
  X(EVValue, "EVValue")                                     \
  X(MFValue1, "MFValue1")                                   \
  X(MFValue2, "MFValue2")                                   \
  X(ISOValue, "ISOValue")                                   \
  X(ExposureValue, "ExposureValue")                         \
  X(WhiteBlanceMode, "WhiteBlanceMode")                     \
  X(CaptureMode, "CaptureMode")                             \
  X(CaptureCount, "CaptureCount")                           \
  X(CaptureSpaceTime, "CaptureSpaceTime")                   \
  X(SelfTimer, "SelfTimer")                                 \
  X(TimeLapseMode, "TimeLapseMode")                         \
  X(SaveTo, "SaveTo")                                       \
  X(WifiDisableTime, "WifiDisableTime")                     \
  X(EthernetMode, "EthernetMode")                           \
  X(EthernetIP, "EthernetIP")                               \
  X(EthernetMask, "EthernetMask")                           \
  X(EthernetGateway, "EthernetGateway")                     \
  X(EthernetDNS, "EthernetDNS")                             \
  X(SocketPort, "SocketPort")                               \
  X(LoopRecording, "LoopRecording")                         \
  X(US360Version, "US360Version")                           \
  X(WifiChannel, "WifiChannel")                             \
  X(ExposureFreq, "ExposureFreq")                           \
  X(FanControl, "FanControl")                               \
  X(Sharpness, "Sharpness")                                 \
  X(UserCtrl30FPS, "UserCtrl30FPS")                         \
  X(CameraMode, "CameraMode")                               \
  X(ColorSTMode, "ColorSTMode")                             \
  X(AutoGlobalPhiAdjMode, "AutoGlobalPhiAdjMode")           \
  X(HDMITextVisibility, "HDMITextVisibility")               \
  X(SpeakerEnable, "SpeakerEnable")                         \
  X(LEDBrightness, "LEDBrightness")                         \
  X(ScreenControl, "ScreenControl")                         \
  X(CountDown, "CountDown")                                 \
  X(ImageQuality, "ImageQuality")                           \
  X(PhotoResolution, "PhotoResolution")                     \
  X(RecordResolution, "RecordResolution")                   \
  X(TimeLapseResolution, "TimeLapseResolution")             \
  X(Translucent, "Translucent")                             \
  X(CompassMaxX, "CompassMaxX")                             \
  X(CompassMaxY, "CompassMaxY")                             \
  X(CompassMaxZ, "CompassMaxZ")                             \
  X(CompassMinX, "CompassMinX")                             \
  X(CompassMinY, "CompassMinY")                             \
  X(CompassMinZ, "CompassMinZ")                             \
  X(DebugModeEnable, "DebugModeEnable")                     \
  X(NadirMode, "NadirMode")                                 \
  X(NadirSize, "NadirSize")                                 \
  X(HDREvMode, "HDREvMode")                                 \
  X(RecordBitrate, "RecordBitrate")                         \
  X(HttpAccount, "HttpAccount")                             \
  X(HttpPassword, "HttpPassword")                           \
  X(HttpPort, "HttpPort")                                   \
  X(CompassEnable, "CompassEnable")                         \
  X(GSensorEnable, "GSensorEnable")                         \
  X(PhotoHDRMode, "PhotoHDRMode")                           \
  X(NadirTextEnable, "NadirTextEnable")                     \
  X(NadirTextColor, "NadirTextColor")                       \
  X(NadirBackgroundColor, "NadirBackgroundColor")           \
  X(NadirTextFont, "NadirTextFont")                         \
  X(NadirTextLoopEnable, "NadirTextLoopEnable")             \
  X(NadirText, "NadirText")                                 \
  X(TimeLapseEncodeType, "TimeLapseEncodeType")             \
  X(WhiteBalanceRGB, "WhiteBalanceRGB")                     \
  X(Contrast, "Contrast")                                   \
  X(Saturation, "Saturation")                               \
  X(FreeCount, "FreeCount")                                 \
  X(TimeLapseSaveMode, "TimeLapseSaveMode")                 \
  X(BModeSec, "BModeSec")                                   \
  X(BModeGain, "BModeGain")                                 \
  X(HDRManualEnable, "HDRManualEnable")                     \
  X(HDRNumber, "HDRNumber")                                 \
  X(HDRIncrement, "HDRIncrement")                           \
  X(HDRStrength, "HDRStrength")                             \
  X(HDRTone, "HDRTone")                                     \
  X(AEBNumber, "AEBNumber")                                 \
  X(AEBIncrement, "AEBIncrement")                           \
  X(LiveQuality, "LiveQuality")                             \
  X(WhiteBalanceTemperature, "WhiteBalanceTemperature")     \
  X(HDRDeghosting, "HDRDeghosting")                         \
  X(RemovalHDRMode, "RemovalHDRMode")                       \
  X(RemovalHDRNumber, "RemovalHDRNumber")                   \
  X(RemovalHDRIncrement, "RemovalHDRIncrement")             \
  X(RemovalHDRStrength, "RemovalHDRStrength")               \
  X(AntiAliasingEnable, "AntiAliasingEnable")               \
  X(RemovalAntiAliasingEnable, "RemovalAntiAliasingEnable") \
  X(WhiteBalanceTint, "WhiteBalanceTint")                   \
  X(HDRAutoStrength, "HDRAutoStrength")                     \
  X(RemoveHDRAutoStrength, "RemoveHDRAutoStrength")         \
  X(LiveBitrate, "LiveBitrate")                             \
  X(PowerSavingEnable, "PowerSavingEnable")

extern "C" bool is_data_bin_created();

extern "C" void init_data_bin(int, int);

extern "C" void save_data_bin();

extern "C" void delete_data_bin();

extern "C" int get_raw_data_bin(char*);

extern "C" void set_int_to_data_bin_by_key(const char* /* key */,
                                           int /* value */);

extern "C" void set_char_array_to_data_bin_by_key(const char* /* key */,
                                                  char* /* array */,
                                                  int /* size */);

extern "C" int get_int_from_data_bin_by_key(const char* /* key */);

extern "C" void get_char_array_from_data_bin_by_key(const char* /* key */,
                                                     char* /* save to */);

namespace Device {

struct Address {};

// 用於存取與整合攝影機設定值結構
class DataBin final {
 public:
#define X(a, b) a,
  enum class Keys : int { DATA_BIN_KEY_TABLE };
#undef X

  constexpr static inline const char* KeyString(Keys key) {
#define X(a, b) b,
    char* table[] = {DATA_BIN_KEY_TABLE};
#undef X
    return table[static_cast<int>(key)];
  }

  static char path[];

  // - MARK: 初始化器
 public:
  static DataBin& instance() {
    static DataBin instance;
    return instance;
  }

 private:
  DataBin();
  ~DataBin();
  DataBin(const DataBin&) = delete;
  DataBin& operator=(const DataBin&) = delete;

  /* * * * * 其他成員 * * * * */

 public:
  struct Result {};

  static Json default_value_;

  bool is_data_bin_exsit_ = false;

 public:
  void SetDefaultTo(int /* freq */, int /* customer */);

  template <typename T>
  inline void SetValue(/* by */ std::string key, T value) {
    data_bin_[key] = value;
    Save();
  }

  template <typename T>
  inline void SetValue(/* by */ Keys key, T value) {
    data_bin_[KeyString(key)] = value;
    Save();
  }

  template <typename T>
  inline auto GetValue(/* by */ std::string key,
                       /* with */ T default_value) -> T {
    return data_bin_.value(
        key, Device::DataBin::default_value_.value(key, default_value));
  }

  template <typename T>
  inline auto GetValue(/* by */ std::string key) -> T {
    if (data_bin_.contains(key)) {
      return data_bin_.at(key);
    } else {
      return DataBin::default_value_.at(key).get<T>();
    }
  }

  template <typename T>
  inline auto GetValue(/* by */ Keys key) -> T {
    auto key_string = KeyString(key);
    Log(LogLevel::Debug) << key_string << '\n';
    Log(LogLevel::Debug) << DataBin::default_value_[key_string].get<T>()
                         << '\n';
    if (data_bin_.contains(key_string)) {
      return data_bin_.at(key_string);
    } else {
      return DataBin::default_value_.at(key_string).get<T>();
    }
  }

  template <int Size>
  inline auto GetValue(/* by */ Keys key) -> std::string {
    auto string = GetValue<std::string>(key);
    if (string.size() < Size) {
      for (int count = 0; count < Size - string.size(); count++) string += " ";
    }
    return string.substr(0, Size);
  }

  int RawDataBin(char*);

  void Save();

  void Delete();

  // void AddObserver(Any, Action, int, std::string /* key */) {}

 private:
  System::Logger logger;

  Json data_bin_;
};
}  // namespace Device
