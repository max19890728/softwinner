/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/
 
#ifndef __FOCUS_H__
#define __FOCUS_H__

#ifdef __cplusplus
extern "C" {
#endif

#define FOCUS_SENSOR_WIDTH		4364
#define FOCUS_SENSOR_HEIGHT		3282
#define FOCUS_SENSOR_C_X		(FOCUS_SENSOR_WIDTH  >> 1)
#define FOCUS_SENSOR_C_Y		(FOCUS_SENSOR_HEIGHT >> 1)
#define FOCUS_IMG_X			1920
#define FOCUS_IMG_Y			1024
#define FOCUS_WIN_X			(FOCUS_IMG_X >> 1)
#define FOCUS_WIN_Y			(FOCUS_IMG_Y >> 1)
#define FOCUS_SEARCH_SIZE_1	256
#define FOCUS_SEARCH_SIZE_2	64
#define FOCUS_SCAN_SIZE		15
#define FOCUS_LOCK_SIZE		7

/*typedef struct Focus_struct_h
{
  unsigned short	Check_Code;
  unsigned short	Block_Level;		// 2的次方
  unsigned short	S_X;
  unsigned short	S_Y;
  unsigned short	C_X;
  unsigned short	C_Y;
  unsigned		Rev[13];		// total: 64 byte
} Focus_struct;
extern Focus_struct Focus_Command;*/

struct Focus_Posi_Struct {
	int X;
	int Y;
};
extern struct Focus_Posi_Struct Adj_Foucs_Center_Default[5][4];
extern struct Focus_Posi_Struct Adj_Foucs_Center[5][4];
extern struct Focus_Posi_Struct Adj_Foucs_Posi_Default[4][5][4];
extern struct Focus_Posi_Struct Adj_Foucs_Posi[5][4];

extern float Search_Foucs_Temp[5][4][2];

/*typedef struct Focus_Block_Struct_H {
	unsigned focus[4];
	unsigned Y[4][128*128];
	struct Focus_Posi_Struct P[4];
} Focus_Block_Struct;
extern Focus_Block_Struct Focus_Block_Data;*/

struct Focus_Parameter_Struct {
	struct Focus_Posi_Struct S_Posi;			//4分割在Sensor上的座標
	struct Focus_Posi_Struct W_Posi;			//4分割在畫面上的座標(1920x1024)
	struct Focus_Posi_Struct Posi_Offset;		//外圍框的Offset
	struct Focus_Posi_Struct W_Center_Offset;	//原中心點偏移量(Img座標)
	struct Focus_Posi_Struct S_Center_Offset;	//原中心點偏移量(Sensor座標)
	struct Focus_Posi_Struct Center;			//中心點在Sensor上的座標	= S_Posi + Posi_Offset + S_Center_Offset + Img(1920x1024) / 4
	short Pixel_W[FOCUS_WIN_Y][FOCUS_WIN_X];
	short Pixel_I[FOCUS_SEARCH_SIZE_1][FOCUS_SEARCH_SIZE_1];
	short Pixel_O[FOCUS_SEARCH_SIZE_2][FOCUS_SEARCH_SIZE_2];
	unsigned Focus;
	unsigned Error;
	unsigned IsPass;
	unsigned Focus_Th;
	int D_XY[2];								//與邊界的距離[X/Y]
	float Degree;								//與Sensor中心夾角
	float Focus_Rate;							//0x20181107 +
	unsigned Focus_Max;

	char rev[108];
};
//extern struct Focus_Parameter_Struct Focus_P[5][4];

struct Focus_Result_Struct {
	int CheckSum;
	int Pass;
	int Degree;
	struct Focus_Posi_Struct Sensor_Offset;
	struct Focus_Parameter_Struct FocusData[4];
	int Tool_Num;
	char Sample_Num[16];
	float Tool_Focus_Rate;		//not use
	unsigned Focus_Min;				//0x20181107 +
	unsigned Focus_Min_Idx;				//0x20181107 +

	char rev[944];
};
extern struct Focus_Result_Struct Focus_Result[5];		//[sensor]

#define FOCUS_ADJ_CHECK_SUM_0		0x20180807
#define FOCUS_ADJ_CHECK_SUM_1		0x20181107
#define FOCUS_NOT_CHANGE_CODE	0xFFFFFF
struct Test_Tool_Focus_Adj_Struct {
	int CheckSum;
	int Focus_Th[5][4];
	int White_Th;
	int Black_Th;
	int Ep;
	int Gain;
	struct Focus_Posi_Struct Offset[5];
	int Tool_Num;
	char Sample_Num[16];
	float Tool_Focus_Rate;		//not use
	int Adj_Posi_Flag;
	float Focus_Rate[5][4];			//0x20181107 +

	char rev[148];
};
extern struct Test_Tool_Focus_Adj_Struct Test_Tool_Focus_Adj;

extern int Focus_Scan_Table[FOCUS_SCAN_SIZE][FOCUS_SCAN_SIZE];
extern int Focus_Scan_Table_Tran[FOCUS_SCAN_SIZE][FOCUS_SCAN_SIZE];

extern int Adj_Focus_Error[5][4];

extern int Focus_Tool_Id;
extern int White_Th;
extern int Black_Th;
extern int Focus_Tool_Num;

void SetFocusToolNum(int num);
void FocusScanTableTranInit(void);
void FocusResultInit(void);
void Search_Focus_Dot(int s_id, char *img);
void doFocus(struct Focus_Parameter_Struct *fs, float th_rate);
//void Focus_Block(struct Focus_Parameter_Struct *fs);
void Focus_Block(struct Focus_Parameter_Struct *fs, int x1, int y1, int x2, int y2);
int Test_Tool_Focus_Adj_Init(int tool_num);
void WriteTestToolFocusAdjFile(int tool_num);
void WriteFocusResult(int s_id);
int ReadFocusResult(int s_id, int type);
void WriteAdjFocusPosi(int tool_num);
void ReadAdjFocusPosi(int s_id, int tool_num);
int check_focus_time(unsigned long long time);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__FOCUS_H__
