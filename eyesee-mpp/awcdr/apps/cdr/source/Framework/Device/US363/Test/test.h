/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/
 
#ifndef __TEST_H__
#define __TEST_H__

#ifdef __cplusplus
extern "C" {
#endif

struct Test_Tool_Cmd_Struct_H {
	int State;		// 0:deconnect 1:connect

	/*	MainCmd
	 *  0:change mode  (SubCmd: mode )
	 *  1:show ISP2    (SubCmd: 0:S0 1:S1 2:S2 3:S3 4:ALL)
	 *
	 *  2:auto sitching
	 *  3:RTC
	 *  4:mic & speaker
	 *  5:cell
	 *
	 *  6:key
	 *  7:OLED
	 *  8:LED
	 *
	 *  9:write machine data
	 */
	int MainCmd;
	int SubCmd;
	int Step;

	int rev[60];
};
extern struct Test_Tool_Cmd_Struct_H TestToolCmd;

/*
 * TEST_RESULT_CHECK_SUM:
 * 0x20180725: 新增 CheckSum
 *   		        新增 StitchVer
 */
//#define TEST_RESULT_CHECK_SUM	0x20180725
//typedef struct Test_Tool_Result_Struct_h {
//	char SSID[32];
//	char TestToolVer[32];		//製具縫合校正時, 製具的版本
//	char AletaVer[32];			//縫合校正時, 本機的版本
//	int  TestBlockTableNum;
//	int CheckSum;
//	int StitchVer;				//紀錄縫合演算法的版本
//
//	char rev[1016];
//}Test_Tool_Result_Struct;
//extern Test_Tool_Result_Struct Test_Tool_Result;

extern int testtool_set_AEG, testtool_set_gain;

int do_FX_ST_Test(int f_id);
void S2AddSTTableProcTest(void);
void STTableTestS2AllSensor(void);
void TestToolCmdInit(void);
void deleteTestModeOledFile(void);
void createTestModeOledFile(void);
void deleteTestModeLedFile(void);
void createTestModeLedFile(void);
void deleteTestModeGsensorFile(void);
void createTestModeGsensorFile(void);
void deleteTestModeGsensorErrorFile(void);
void createTestModeGsensorErrorFile(void);
void deleteTestModeSdcardFile(void);
void createTestModeSdcardFile(void);
void deleteTestModeSdcardSkipFile(void);
void createTestModeSdcardSkipFile(void);
void deleteTestModeFocusFile(void);
void createTestModeFocusFile(void);
void deleteGetFocusFile(void);
int checkGetFocusFile(void);
void deleteAdjFocusRAW(void);
void deleteAdjFocusPosi(void);
void deleteTestModeButtonActionFile(void);
void createTestModeButtonActionFile(void);
void deleteTestModeButtonPowerFile(void);
void createTestModeButtonPowerFile(void);
void deleteTestModeFanFile(void);
void createTestModeFanFile(void);
void deleteTestModeBatteryFile(void);
void createTestModeBatteryFile(void);
void deleteTestModeBatteryErrorFile(void);
void createTestModeBatteryErrorFile(void);
void deleteTestModeHDMIFile(void);
void createTestModeHDMIFile(void);
void deleteTestModeHDMISkipFile(void);
void createTestModeHDMISkipFile(void);
void deleteTestModeWifiFile(void);
void createTestModeWifiFile(void);
void deleteTestModeWifiErrorFile(void);
void createTestModeWifiErrorFile(void);
void deleteTestModeAudioFile(void);
void createTestModeAudioFile(void);
void deleteTestModeAudioDoneFile(void);
void createTestModeAudioDoneFile(void);
void deleteTestModeDna(void);
void deleteTestModeDnaDone(void);
void createTestModeDnaDone(void);
void deleteTestModeDnaError(void);
void createTestModeDnaError(void);
void deleteAutoStitchFinishFile(void);
void createAutoStitchFinishFile(void);
void createAutoStitchRestartFile(void);
void createAutoStitchErrorFile(void);
int checkAutoStitchFile(void);
void deleteGetStJpgFile(void);
int checkGetStJpgFile(void);
void deleteBackgroundBottomUserFile(void);
void deleteTestModeFiles(void);
void STTableTestS2ShowSTLine(void);
void WriteTestResult(int type, int debug);
int ReadTestResult(void);

int get_TestToolCmd(void);
int GetTestToolState();
void SetTestToolStep(int step);
int GetTestToolStep();
void STTableTestS2Focus(int s_id);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__TEST_H__