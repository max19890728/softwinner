/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __US360_PARAMETER_TMP_H__
#define __US360_PARAMETER_TMP_H__

#ifdef __cplusplus
extern "C" {
#endif

#define PARAMETERTMP_CHECKSUM 20151126

struct Tmp_Parameter_struct_H
{
	int rec_cnt;
	int cap_cnt;

	int AEG_EP_H_Init;
	int AEG_EP_L_Init;
	int AEG_gain_H_Init;

	int Temp_gain_R_Init;
	int Temp_gain_G_Init;
	int Temp_gain_B_Init;

	int CheckSum;

	int rev[247];
};

int Save_Parameter_Tmp_File();
int LoadParameterTmp();
void Set_Parameter_Tmp_RecCnt(int cnt);
int Get_Parameter_Tmp_RecCnt();
void Set_Parameter_Tmp_CapCnt(int cnt);
int Get_Parameter_Tmp_CapCnt();
void Set_Parameter_Tmp_AEG(int ep_h, int ep_l, int gain);
void Set_Parameter_Tmp_GainRGB(int r, int g, int b);
void WriteLoadParameterTmpErr(int ret);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__US360_PARAMETER_TMP_H__