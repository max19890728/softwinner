/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __SMOOTH_H__
#define __SMOOTH_H__

#ifdef __cplusplus
extern "C" {
#endif

// 0 = unlimite  63: 100 CM
struct Smooth_Com_Struct {
	int Global_phi_Top;
	int Global_phi_Mid;
	int Global_phi_Btm;

	int Smooth_Debug_Flag;
	int Smooth_Avg_Weight;
	int Smooth_Base_Level;
	int Smooth_Pre_Weight;
	int Smooth_Auto_Rate;  // 100 : Auto 0: Manual
	int Smooth_XY_Space;
	int Smooth_Pre_Space;
	int Smooth_Manual_Space;
	int Smooth_Manual_Weight;
	int Smooth_Low_Level;
	int Smooth_Weight_Th;
};
extern struct Smooth_Com_Struct Smooth_Com;


struct Smooth_O_Struct {
  unsigned  	V             :7;
  unsigned  	F_V           :1;
  unsigned  	U             :7;
  unsigned  	F_U           :1;
  unsigned  	Y             :7;
  unsigned  	F_Y           :1;
  unsigned  	Z             :7;
  unsigned  	F_Z           :1;
};

struct Smooth_Z_I_Struct {
	  unsigned  	Weight        :10;
	  unsigned  	Rev0          :6;
	  unsigned  	Value0        :10;
	  unsigned  	Count0        :6;
};

struct Smooth_Debug_Struct {
	struct Smooth_Z_I_Struct Smooth_Z_V_I[6][512][32];
	struct Smooth_Z_I_Struct Smooth_Z_H_I[6][512][32];
	int    Smooth_I_Buf[6][70][512];
	int    Smooth_O_Buf[6][4][4][512];
	struct Smooth_O_Struct  Smooth_O[2][6][512];
	int    Smooth_Debug_Buf[6][3][512];
};
extern struct Smooth_Debug_Struct Smooth_Debug;


//----------------------------------------------------------------
extern int Smooth_Init;
extern struct Smooth_Z_I_Struct Smooth_Z_V_I[6][512][32];
extern struct Smooth_Z_I_Struct Smooth_Z_H_I[6][512][32];
extern int Smooth_YUV_Data[6][3][512][2];
extern int Smooth_I_Weight[6][67][512];
extern int Smooth_I_Data[6][67][512];
extern int Smooth_I_Temp[64][512];
extern int Smooth_I_Buf[6][70][512];
extern struct Smooth_O_Struct  Smooth_O[2][6][512];
extern int ColorST_SW, AutoST_SW;
extern int Smooth_YUV_Rate;
extern int Smooth_D_Max;

//----------------------------------------------------------------
void Smooth_O_Init(void);
void Smooth_I_Init(void);
int Send_Smooth_Table(int M_Mode, int idx);
void Smooth_I_to_O_Proc(int M_Mode);
void Smooth_Line_Trans(int S_Mode, int T_Mode);
void Make_Smooth_table(void);
void Smooth_Run(int Img_Mode, int c_mode);
void Set_Smooth_Speed_Mode(int mode);
int Get_Smooth_Speed_Mode(void);
void Set_Smooth_Debug(void);
void SaveSmoothFile(char *cap_path);
int ReadSmoothFile(char *path);
void setSmoothParameter(int idx, int value);
void checkSaveSmoothBin();
void setSmoothSpeed(int idx, int value);
void setSmoothXYSpace(int space);
void setSmoothFarWeight(int weight);
void setSmoothDelSlope(int slope);
void setSmoothFunction(int func);
void setSaveSmoothEn(int en);
void SetSmoothOIdx(int idx);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__SMOOTH_H__