/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __DATABIN_H__
#define __DATABIN_H__

#ifdef __cplusplus
extern "C" {
#endif

void Set_DataBin_Version(int Version);
void Set_DataBin_DemoMode(int DemoMode);
void Set_DataBin_CamPositionMode(int CamPositionMode);
void Set_DataBin_PlayMode(int PlayMode);
void Set_DataBin_ResoultMode(int ResoultMode);
void Set_DataBin_EVValue(int EVValue);
void Set_DataBin_MFValue(int MFValue);
void Set_DataBin_MF2Value(int MF2Value);
void Set_DataBin_ISOValue(int ISOValue);
void Set_DataBin_ExposureValue(int ExposureValue);
void Set_DataBin_WBMode(int WBMode);
void Set_DataBin_CaptureMode(int CaptureMode);
void Set_DataBin_CaptureCnt(int CaptureCnt);
void Set_DataBin_CaptureSpaceTime(int CaptureSpaceTime);
void Set_DataBin_SelfTimer(int SelfTimer);
void Set_DataBin_TimeLapseMode(int TimeLapseMode);
void Set_DataBin_SaveToSel(int SaveToSel);
void Set_DataBin_WifiDisableTime(int wifiDisableTime);
void Set_DataBin_EthernetMode(int ethernetMode);
void Set_DataBin_EthernetIP(char *ip);
void Set_DataBin_EthernetMask(char *mask);
void Set_DataBin_EthernetGateWay(char *gateway);
void Set_DataBin_EthernetDNS(char *dns);
void Set_DataBin_MediaPort(int port);
void Set_DataBin_DrivingRecord(int record);
void Set_DataBin_US360Version(char *version);
void Set_DataBin_WifiChannel(int channel);
void Set_DataBin_ExposureFreq(int freq);
void Set_DataBin_FanControl(int ctrl);
void Set_DataBin_Sharpness(int sharp);
void Set_DataBin_UserCtrl30Fps(int ctrl);
void Set_DataBin_CameraMode(int CameraMode);
void Set_DataBin_ColorSTMode(int ColorSTMode);
void Set_DataBin_AutoGlobalPhiAdjMode(int AutoGlobalPhiAdjMode);
void Set_DataBin_HDMITextVisibility(int HDMITextVisibility);
void Set_DataBin_SpeakerMode(int SpeakerMode);
void Set_DataBin_LedBrightness(int LedBrightness);
void Set_DataBin_OledControl(int OledControl);
void Set_DataBin_DelayValue(int DelayValue);
void Set_DataBin_ImageQuality(int ImageQuality);
void Set_DataBin_PhotographReso(int PhotographReso);
void Set_DataBin_RecordReso(int RecordReso);
void Set_DataBin_TimeLapseReso(int TimeLapseReso);
void Set_DataBin_Translucent(int Translucent);
void Set_DataBin_CompassMaxx(int CompassMaxx);
void Set_DataBin_CompassMaxy(int CompassMaxy);
void Set_DataBin_CompassMaxz(int CompassMaxz);
void Set_DataBin_CompassMinx(int CompassMinx);
void Set_DataBin_CompassMiny(int CompassMiny);
void Set_DataBin_CompassMinz(int CompassMinz);
void Set_DataBin_DebugLogMode(int DebugLogMode);
void Set_DataBin_BottomMode(int BottomMode);
void Set_DataBin_BottomSize(int BottomSize);
void Set_DataBin_hdrEvMode(int HdrEvMode);
void Set_DataBin_Bitrate(int bitrate);
void Set_DataBin_HttpAccount(char *account);
void Set_DataBin_HttpPassword(char *password);
void Set_DataBin_HttpPort(int httpPort);
void Set_DataBin_CompassMode(int CompassMode);
void Set_DataBin_GsensorMode(int GsensorMode);
void Set_DataBin_CapHdrMode(int CapHdrMode);
void Set_DataBin_BottomTMode(int BottomTMode);
void Set_DataBin_BottomTColor(int BottomTColor);
void Set_DataBin_BottomBColor(int BottomBColor);
void Set_DataBin_BottomTFont(int BottomTFont);
void Set_DataBin_BottomTLoop(int BottomTLoop);
void Set_DataBin_BottomText(char *text);
void Set_DataBin_FpgaEncodeType(int FpgaEncodeType);
void Set_DataBin_WbRGB(char *rgb);
void Set_DataBin_Contrast(int Contrast);
void Set_DataBin_Saturation(int Saturation);
void Set_DataBin_FreeCount(int FreeCount);
void Set_DataBin_SaveTimelapse(int SaveTimelapse);
void Set_DataBin_BmodeSec(int BmodeSec);
void Set_DataBin_BmodeGain(int BmodeGain);
void Set_DataBin_HdrManual(int HdrManual);
void Set_DataBin_HdrNumber(int HdrNumber);
void Set_DataBin_HdrIncrement(int HdrIncrement);
void Set_DataBin_HdrStrength(int HdrStrength);
void Set_DataBin_HdrTone(int HdrTone);
void Set_DataBin_AebNumber(int AebNumber);
void Set_DataBin_AebIncrement(int AebIncrement);
void Set_DataBin_LiveQualityMode(int Mode);
void Set_DataBin_WbTemperature(int temp);
void Set_DataBin_HdrDeghosting(int mode);
void Set_DataBin_RemoveHdrMode(int val);
void Set_DataBin_RemoveHdrNumber(int val);
void Set_DataBin_RemoveHdrIncrement(int val);
void Set_DataBin_RemoveHdrStrength(int val);
void Set_DataBin_AntiAliasingEn(int val);
void Set_DataBin_RemoveAntiAliasingEn(int val);
void Set_DataBin_WbTint(int tint);
void Set_DataBin_HdrAutoStrength(int HdrStrength);
void Set_DataBin_RemoveHdrAutoStrength(int val);
void Set_DataBin_LiveBitrate(int bitrate);
void Set_DataBin_PowerSaving(int mode);


int Get_DataBin_Version();
int Get_DataBin_DemoMode();
int Get_DataBin_CamPositionMode();
int Get_DataBin_PlayMode();
int Get_DataBin_ResoultMode();
int Get_DataBin_EVValue();
int Get_DataBin_MFValue();
int Get_DataBin_MF2Value();
int Get_DataBin_ISOValue();
int Get_DataBin_ExposureValue();
int Get_DataBin_WBMode();
int Get_DataBin_CaptureMode();
int Get_DataBin_CaptureCnt();
int Get_DataBin_CaptureSpaceTime();
int Get_DataBin_SelfTimer();
int Get_DataBin_TimeLapseMode();
int Get_DataBin_SaveToSel();
int Get_DataBin_WifiDisableTime();
int Get_DataBin_EthernetMode();
void Get_DataBin_EthernetIP(char *ip, int size);
void Get_DataBin_EthernetMask(char *mask, int size);
void Get_DataBin_EthernetGateWay(char *gateway, int size);
void Get_DataBin_EthernetDNS(char *dns, int size);
int Get_DataBin_MediaPort();
int Get_DataBin_DrivingRecord();
void Get_DataBin_US360Version(char *ver, int len);
int Get_DataBin_WifiChannel();
int Get_DataBin_ExposureFreq();
int Get_DataBin_FanControl();
int Get_DataBin_Sharpness();
int Get_DataBin_UserCtrl30Fps();
int Get_DataBin_CameraMode();
int Get_DataBin_ColorSTMode();
int Get_DataBin_AutoGlobalPhiAdjMode();
int Get_DataBin_HDMITextVisibility();
int Get_DataBin_SpeakerMode();
int Get_DataBin_LedBrightness();
int Get_DataBin_OledControl();
int Get_DataBin_DelayValue();
int Get_DataBin_ImageQuality();
int Get_DataBin_PhotographReso();
int Get_DataBin_RecordReso();
int Get_DataBin_TimeLapseReso();
int Get_DataBin_Translucent();
int Get_DataBin_CompassMaxx();
int Get_DataBin_CompassMaxy();
int Get_DataBin_CompassMaxz();
int Get_DataBin_CompassMinx();
int Get_DataBin_CompassMiny();
int Get_DataBin_CompassMinz();
int Get_DataBin_DebugLogMode();
int Get_DataBin_BottomMode();
int Get_DataBin_BottomSize();
int Get_DataBin_hdrEvMode();
int Get_DataBin_Bitrate();
void Get_DataBin_HttpAccount(char *account, int size);
void Get_DataBin_HttpPassword(char *pwd, int size);
int Get_DataBin_HttpPort();
int Get_DataBin_CompassMode();
int Get_DataBin_GsensorMode();
int Get_DataBin_CapHdrMode();
int Get_DataBin_BottomTMode();
int Get_DataBin_BottomTColor();
int Get_DataBin_BottomBColor();
int Get_DataBin_BottomTFont();
int Get_DataBin_BottomTLoop();
void Get_DataBin_BottomText(char *text, int size);
int Get_DataBin_FpgaEncodeType();
void Get_DataBin_WbRGB(char *rgb, int size);
int Get_DataBin_Contrast();
int Get_DataBin_Saturation();
int Get_DataBin_FreeCount();
int Get_DataBin_SaveTimelapse();
int Get_DataBin_BmodeSec();
int Get_DataBin_BmodeGain();
int Get_DataBin_HdrManual();
int Get_DataBin_HdrNumber();
int Get_DataBin_HdrIncrement();
int Get_DataBin_HdrStrength();
int Get_DataBin_HdrTone();
int Get_DataBin_AebNumber();
int Get_DataBin_AebIncrement();
int Get_DataBin_LiveQualityMode();
int Get_DataBin_WbTemperature();
int Get_DataBin_HdrDeghosting();
int Get_DataBin_RemoveHdrMode();
int Get_DataBin_RemoveHdrNumber();
int Get_DataBin_RemoveHdrIncrement();
int Get_DataBin_RemoveHdrStrength();
int Get_DataBin_AntiAliasingEn();
int Get_DataBin_RemoveAntiAliasingEn();
int Get_DataBin_WbTint();
int Get_DataBin_HdrAutoStrength();
int Get_DataBin_RemoveHdrAutoStrength();
int Get_DataBin_LiveBitrate();
int Get_DataBin_PowerSaving();

int Get_DataBin_Now_Version(void);
int split_c(char **buf, char *str, char *del);
void WriteUS360DataBin();
void ReadUS360DataBin(int country, int customer);
void DeleteUS360DataBin();
int CheckExpFreqDefault(int country);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__DATABIN_H__