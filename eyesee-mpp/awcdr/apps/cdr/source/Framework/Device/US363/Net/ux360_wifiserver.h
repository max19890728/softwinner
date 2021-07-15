/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __UX360_WIFISERVER_H__
#define __UX360_WIFISERVER_H__

#include "Device/US363/Util/ux360_list.h"


#ifdef __cplusplus
extern "C" {
#endif

struct gsensorVal_struct{
   	float pan;
   	float tilt;
   	float rotate;
   	float wide;
   	unsigned long long timestamp;
};


int start_wifi_server(int port);
void freemSendTHMListData(void);
void setSendFeedBackSTEn(int idx, int en);
void setSendFeedBackCmd(int idx, char *cmd);

void free_wifiserver_buf();
int malloc_wifiserver_buf();

void Set_WifiServer_DbtDdrCmdEn(int en);
int Get_WifiServer_DbtDdrCmdEn();
void Set_WifiServer_DbtInputDdrDataEn(int en);
int Get_WifiServer_DbtInputDdrDataEn();
void Set_WifiServer_DbtInputDdrDataFinish(int flag);
int Get_WifiServer_DbtInputDdrDataFinish();
void Set_WifiServer_DbtOutputDdrDataEn(int en);
int Get_WifiServer_DbtOutputDdrDataEn();
void Set_WifiServer_DbtRegCmdEn(int en);
int Get_WifiServer_DbtRegCmdEn();
void Set_WifiServer_DbtInputRegDataFinish(int flag);
int Get_WifiServer_DbtInputRegDataFinish();
void Set_WifiServer_DbtOutputRegDataEn(int en);
int Get_WifiServer_DbtOutputRegDataEn();

//---------------------------------------
void Set_WifiServer_SendFeedBackEn(int en);
int Get_WifiServer_SendFeedBackEn();
void Set_WifiServer_SendFeedBackAction(char *action, int action_len);
void Set_WifiServer_SendFeedBackValue(int val);
int Get_WifiServer_Connected();
void Set_WifiServer_KelvinEn(int en);
void Set_WifiServer_KelvinVal(int val);
void Set_WifiServer_KelvinTintVal(int val);
void Set_WifiServer_CompassResultEn(int en);
void Set_WifiServer_CompassResultVal(int val);

void Set_WifiServer_WifiDisableTimeEn(int en);
int Get_WifiServer_WifiDisableTimeEn();
int Get_WifiServer_WifiDisableTime();

void Set_WifiServer_ZoneEn(int en);
int Get_WifiServer_ZoneEn();
int Get_WifiServer_WifiZone(char *dst_buf, int dst_size);

void Set_WifiServer_SetTime(int en);
int Get_WifiServer_SetTime();
unsigned long long Get_WifiServer_SysTime();

void Set_WifiServer_SnapshotEn(int en);
int Get_WifiServer_SnapshotEn();

void Set_WifiServer_RecordEn(int en);
int Get_WifiServer_RecordEn();
int Get_WifiServer_TimeLapseMode();

void Set_WifiServer_AdjustEn(int en);
int Get_WifiServer_AdjustEn();
void Set_WifiServer_EV(int val);
int Get_WifiServer_EV();
void Set_WifiServer_MF(int val);
int Get_WifiServer_MF();
void Set_WifiServer_MF2(int val);
int Get_WifiServer_MF2();

void Set_WifiServer_ModeSelectEn(int en);
int Get_WifiServer_ModeSelectEn();
int Get_WifiServer_PlayMode();
void Set_WifiServer_ResoluMode(int mode);
int Get_WifiServer_ResoluMode();

void Set_WifiServer_ISOEn(int en);
int Get_WifiServer_ISOEn();
int Get_WifiServer_ISOValue();

void Set_WifiServer_FEn(int en);
int Get_WifiServer_FEn();
int Get_WifiServer_FValue();

void Set_WifiServer_WBEn(int en);
int Get_WifiServer_WBEn();
int Get_WifiServer_WBMode();

void Set_WifiServer_CamSelEn(int en);
int Get_WifiServer_CamSelEn();
int Get_WifiServer_CamMode();

void Set_WifiServer_SaveToSelEn(int en);
int Get_WifiServer_SaveToSelEn();
int Get_WifiServer_SaveToSel();

void Set_WifiServer_FormatEn(int en);
int Get_WifiServer_FormatEn();

void Set_WifiServer_DeleteEn(int en);
int Get_WifiServer_DeleteEn();
LINK_NODE *Get_WifiServer_DeleteFileName();

void Set_WifiServer_CaptureModeEn(int en);
int Get_WifiServer_CaptureModeEn();
int Get_WifiServer_CapMode();

void Set_WifiServer_TimeLapseEn(int en);
int Get_WifiServer_TimeLapseEn();
int Get_WifiServer_TimeLapse();

void Set_WifiServer_SharpnessEn(int en);
int Get_WifiServer_SharpnessEn();
int Get_WifiServer_Sharpness();

void Set_WifiServer_CaptureCntEn(int en);
int Get_WifiServer_CaptureCntEn();
int Get_WifiServer_CaptureCnt();

void Set_WifiServer_EthernetSettingsEn(int en);
int Get_WifiServer_EthernetSettingsEn();
int Get_WifiServer_EthernetMode();
int Get_WifiServer_EthernetIP(char *dst_buf, int dst_size);
int Get_WifiServer_EthernetMask(char *dst_buf, int dst_size);
int Get_WifiServer_EthernetGateway(char *dst_buf, int dst_size);
int Get_WifiServer_EthernetDNS(char *dst_buf, int dst_size);

void Set_WifiServer_ChangeFPSEn(int en);
int Get_WifiServer_ChangeFPSEn();
int Get_WifiServer_FPS();

void Set_WifiServer_MediaPortEn(int en);
int Get_WifiServer_MediaPortEn();
int Get_WifiServer_Port();

void Set_WifiServer_DrivingRecordEn(int en);
int Get_WifiServer_DrivingRecordEn();
int Get_WifiServer_DrivingRecord();

void Set_WifiServer_WifiChannelEn(int en);
int Get_WifiServer_WifiChannelEn();
int Get_WifiServer_WifiChannel();

void Set_WifiServer_EPFreqEn(int en);
int Get_WifiServer_EPFreqEn();
int Get_WifiServer_EPFreq();

void Set_WifiServer_FanCtrlEn(int en);
int Get_WifiServer_FanCtrlEn();
int Get_WifiServer_FanCtrl();

void Set_WifiServer_ColorSTModeEn(int en);
int Get_WifiServer_ColorSTModeEn();
int Get_WifiServer_mColorSTMode();

void Set_WifiServer_TranslucentModeEn(int en);
int Get_WifiServer_TranslucentModeEn();
int Get_WifiServer_TranslucentMode();

void Set_WifiServer_AutoGlobalPhiAdjEn(int en);
int Get_WifiServer_AutoGlobalPhiAdjEn();
int Get_WifiServer_AutoGlobalPhiAdjMode();

void Set_WifiServer_AutoGlobalPhiAdjOneTimeEn(int en);
int Get_WifiServer_AutoGlobalPhiAdjOneTimeEn();

void Set_WifiServer_HDMITextVisibilityEn(int en);
int Get_WifiServer_HDMITextVisibilityEn();
int Get_WifiServer_HDMITextVisibilityMode();

void Set_WifiServer_RTMPSwitchEn(int en);
int Get_WifiServer_RTMPSwitchEn();
int Get_WifiServer_RTMPSwitchMode();
void Set_WifiServer_SendRtmpStatusCmd(int cmd);

void Set_WifiServer_RTMPConfigureEn(int en);
int Get_WifiServer_RTMPConfigureEn();

void Set_WifiServer_BitrateEn(int en);
int Get_WifiServer_BitrateEn();
int Get_WifiServer_BitrateMode();

void Set_WifiServer_LiveBitrateEn(int en);
int Get_WifiServer_LiveBitrateEn();
int Get_WifiServer_LiveBitrateMode();

void Set_WifiServer_JPEGQualityModeEn(int en);
int Get_WifiServer_JPEGQualityModeEn();
int Get_WifiServer_JPEGQualityMode();

void Set_WifiServer_SpeakerModeEn(int en);
int Get_WifiServer_SpeakerModeEn();
int Get_WifiServer_SpeakerMode();

void Set_WifiServer_LedBrightnessEn(int en);
int Get_WifiServer_LedBrightnessEn();
int Get_WifiServer_LedBrightness();

void Set_WifiServer_OledControlEn(int en);
int Get_WifiServer_OledControlEn();
int Get_WifiServer_OledControl();

void Set_WifiServer_CmcdTimeEn(int en);
int Get_WifiServer_CmcdTimeEn();
int Get_WifiServer_CmcdTime();

void Set_WifiServer_SetEstimateEn(int en);
void Set_WifiServer_GetEstimateEn(int en);
int Get_WifiServer_GetEstimateEn();
void Set_WifiServer_SetEstimateTime(int time);
void Set_WifiServer_SetEstimateStamp(unsigned long long time);
void Set_WifiServer_CaptureEpTime(int time);
void Set_WifiServer_CaptureEpStamp(unsigned long long time);

void Set_WifiServer_DelayValEn(int en);
int Get_WifiServer_DelayValEn();
int Get_WifiServer_DelayVal();

void Set_WifiServer_ResoSaveEn(int en);
int Get_WifiServer_ResoSaveEn();
int Get_WifiServer_ResoSaveMode();
int Get_WifiServer_ResoSaveData();

void Set_WifiServer_CompassEn(int en);
int Get_WifiServer_CompassEn();
int Get_WifiServer_CompassVal();

void Set_WifiServer_CameraModeEn(int en);
int Get_WifiServer_CameraModeEn();
void Set_WifiServer_CameraModeVal(int mode);
int Get_WifiServer_CameraModeVal();

void Set_WifiServer_PlayModeCmdEn(int en);
int Get_WifiServer_PlayModeCmdEn();
int Get_WifiServer_RecordHdrVal();
int Get_WifiServer_PlayTypeVal();

void Set_WifiServer_DebugLogModeEn(int en);
int Get_WifiServer_DebugLogModeEn();
int Get_WifiServer_DebugLogModeVal();

void Set_WifiServer_DatabinSyncEn(int en);
int Get_WifiServer_DatabinSyncEn();
void Set_WifiServer_SyncEn(int en);

void Set_WifiServer_BottomModeEn(int en);
int Get_WifiServer_BottomModeEn();
int Get_WifiServer_BottomModeVal();
int Get_WifiServer_BottomSizeVal();

void Set_WifiServer_BottomEn(int en);
int Get_WifiServer_BottomEn();

void Set_WifiServer_BotmEn(int en);
int Get_WifiServer_BotmEn();
void Set_WifiServer_BotmTotle(int size);
int Get_WifiServer_BotmTotle();
void Set_WifiServer_BotmLen(int size);
int Get_WifiServer_BotmLen();
int Copy_To_WifiServer_BotmData(char *sbuf, int size, int dst_offset);

void Set_WifiServer_HdrEvModeEn(int en);
int Get_WifiServer_HdrEvModeEn();
int Get_WifiServer_HdrEvModeVal();
int Get_WifiServer_HdrEvModeManual();
int Get_WifiServer_HdrEvModeNumber();
int Get_WifiServer_HdrEvModeIncrement();
int Get_WifiServer_HdrEvModeStrength();
int Get_WifiServer_HdrEvModeDeghost();

int Get_WifiServer_HdrDefaultMode(int mode, int idx);

void Set_WifiServer_AebModeEn(int en);
int Get_WifiServer_AebModeEn();
int Get_WifiServer_AebModeNumber();
int Get_WifiServer_AebModeIncrement();

void Set_WifiServer_LiveQualityEn(int en);
int Get_WifiServer_LiveQualityEn();
int Get_WifiServer_LiveQualityMode();

void Set_WifiServer_SendToneEn(int en);
int Get_WifiServer_SendToneEn();
int Get_WifiServer_SendToneVal();

void Set_WifiServer_SendWbTempEn(int en);
int Get_WifiServer_SendWbTempEn();
int Get_WifiServer_SendWbTempVal();
int Get_WifiServer_SendWbTintVal();

void Set_WifiServer_GetRemoveHdrEn(int en);
int Get_WifiServer_GetRemoveHdrEn();
int Get_WifiServer_GetRemoveHdrMode();
int Get_WifiServer_GetRemoveHdrEv();
int Get_WifiServer_GetRemoveHdrStrength();

void Set_WifiServer_GetAntiAliasingEn(int en);
int Get_WifiServer_GetAntiAliasingEn();
int Get_WifiServer_GetAntiAliasingVal();

void Set_WifiServer_GetRemoveAntiAliasingEn(int en);
int Get_WifiServer_GetRemoveAntiAliasingEn();
int Get_WifiServer_GetRemoveAntiAliasingVal();

void Set_WifiServer_GetDefectivePixelEn(int en);
int Get_WifiServer_GetDefectivePixelEn();
void Set_WifiServer_SendDefectivePixelEn(int en);
void Set_WifiServer_SendDefectivePixelVal(int val);

void Set_WifiServer_UninstalldatesEn(int en);
int Get_WifiServer_UninstalldatesEn();

void Set_WifiServer_InitializeDataBinEn(int en);
int Get_WifiServer_InitializeDataBinEn();

void Set_WifiServer_SensorControlEn(int en);
int Get_WifiServer_SensorControlEn();
int Get_WifiServer_CompassModeVal();
int Get_WifiServer_GsensorModeVal();

void Set_WifiServer_BottomTextEn(int en);
int Get_WifiServer_BottomTextEn();
int Get_WifiServer_BottomTMode();
int Get_WifiServer_BottomTColor();
int Get_WifiServer_BottomBColor();
int Get_WifiServer_BottomTFont();
int Get_WifiServer_BottomTLoop();
int Get_WifiServer_BottomText(char *dst_buf, int dst_size);

void Set_WifiServer_FpgaEncodeTypeEn(int en);
int Get_WifiServer_FpgaEncodeTypeEn();
int Get_WifiServer_FpgaEncodeTypeVal();

void Set_WifiServer_SetWBColorEn(int en);
int Get_WifiServer_SetWBColorEn();
int Get_WifiServer_SetWBRVal();
int Get_WifiServer_SetWBGVal();
int Get_WifiServer_SetWBBVal();

void Set_WifiServer_GetWBColorEn(int en);
int Get_WifiServer_GetWBColorEn();
void Set_WifiServer_SendWBColorEn(int en);
void Set_WifiServer_SendWBRVal(int val);
void Set_WifiServer_SendWBGVal(int val);
void Set_WifiServer_SendWBBVal(int val);

void Set_WifiServer_SetContrastEn(int en);
int Get_WifiServer_SetContrastEn();
int Get_WifiServer_SetContrastVal();

void Set_WifiServer_SetSaturationEn(int en);
int Get_WifiServer_SetSaturationEn();
int Get_WifiServer_SetSaturationVal();

void Set_WifiServer_WbTouchEn(int en);
int Get_WifiServer_WbTouchEn();

void Set_WifiServer_BModeEn(int en);
int Get_WifiServer_BModeEn();
int Get_WifiServer_BModeSec();
int Get_WifiServer_BModeGain();

void Set_WifiServer_LaserSwitchEn(int en);
int Get_WifiServer_LaserSwitchEn();
int Get_WifiServer_LaserSwitchVal();

void Set_WifiServer_GetTHMListEn(int en);
int Get_WifiServer_GetTHMListEn();
void Set_WifiServer_THMListSize(int size);
int Copy_To_WifiServer_THMListData(char *sbuf, int size, int dst_offset);
void Set_WifiServer_L63StatusSize(int size);
int Copy_To_WifiServer_L63StatusData(char *sbuf, int size, int dst_offset);
void Set_WifiServer_PCDStatusSize(int size);
int Copy_To_WifiServer_PCDStatusData(char *sbuf, int size, int dst_offset);
void Set_WifiServer_SendTHMListEn(int en);

void Set_WifiServer_GetFolderEn(int en);
int Get_WifiServer_GetFolderEn();
int Get_WifiServer_GetFolderVal(char *dst_buf, int dst_size);
void Set_WifiServer_SendFolderLen(int size);
int Copy_To_WifiServer_SendFolderNames(char *sbuf, int size, int dst_offset);
int Copy_To_WifiServer_SendFolderSizes(char *sbuf, int size, int dst_offset);
void Set_WifiServer_SendFolderSize(long size);
void Set_WifiServer_SendFolderEn(int en);

void Set_WifiServer_PowerSavingEn(int en);
int Get_WifiServer_PowerSavingEn();
int Get_WifiServer_PowerSavingMode();

void Set_WifiServer_SetingUIEn(int en);
int Get_WifiServer_SetingUIEn();
int Get_WifiServer_SetingUIState();

void Set_WifiServer_DoAutoStitchEn(int en);
int Get_WifiServer_DoAutoStitchEn();

void Set_WifiServer_DoGsensorResetEn(int en);
int Get_WifiServer_DoGsensorResetEn();

void Set_WifiServer_ImgEn(int en);
int Get_WifiServer_ImgEn();
void Set_WifiServer_ImgTotle(int size);
int Get_WifiServer_ImgTotle();
void Set_WifiServer_ImgLen(int size);
int Get_WifiServer_ImgLen();
void Get_WifiServer_SendTHMListData(char *dst_buf, int dst_size);
int Copy_To_WifiServer_ImgData(char *sbuf, int size, int dst_offset);
LINK_NODE *Get_WifiServer_ExistFileName();

void Set_WifiServer_DownloadEn(int en);
int Get_WifiServer_DownloadEn();
void Set_WifiServer_DownloadTotle(int size);
int Get_WifiServer_DownloadTotle();
void Set_WifiServer_DownloadLen(int size);
int Get_WifiServer_DownloadLen();
LINK_NODE *Get_WifiServer_DownloadFileName();
LINK_NODE *Get_WifiServer_DownloadFileSkip();
int Copy_To_WifiServer_DownloadData(char *sbuf, int size, int dst_offset);

void Set_WifiServer_WifiConfigEn(int en);
int Get_WifiServer_WifiConfigEn();
void Get_WifiServer_WifiConfigSsid(char *dst_buf, int dst_size);
void Get_WifiServer_WifiConfigPwd(char *dst_buf, int dst_size);

void Set_WifiServer_GPSEn(int en);
int Get_WifiServer_GPSEn();

void Set_WifiServer_ChangeDebugToolStateEn(int en);
int Get_WifiServer_ChangeDebugToolStateEn();
void Set_WifiServer_IsDebugToolConnect(int val);
int Get_WifiServer_IsDebugToolConnect();

void Set_WifiServer_CtrlOLEDEn(int en);
int Get_WifiServer_CtrlOLEDEn();
int Get_WifiServer_CtrlOLEDNum();

void Set_WifiServer_SetSensorToolEn(int en);
int Get_WifiServer_SetSensorToolEn();
void Set_WifiServer_SensorToolNum(int val);
int Get_WifiServer_SensorToolNum();
void Set_WifiServer_SensorToolVal(int val);
int Get_WifiServer_SensorToolVal();

void Set_WifiServer_SetParametersToolEn(int en);
int Get_WifiServer_SetParametersToolEn();
void Set_WifiServer_ParametersNum(int val);
int Get_WifiServer_ParametersNum();
void Set_WifiServer_ParametersVal(int val);
int Get_WifiServer_ParametersVal();


#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__UX360_WIFISERVER_H__