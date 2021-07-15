/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __US360_FUNC_H__
#define __US360_FUNC_H__

#include <math.h>

#include "Device/US363/Cmd/variable.h"

//======================================================================
#ifdef __cplusplus
extern "C" {
#endif

#define S2_STITCH_VERSION_CHECK_CODE 	0x180726
#define S2_STITCH_VERSION 				(S2_STITCH_VERSION_CHECK_CODE + 7)
#define S2_STITCH_VERSION_DEBUG 		(S2_STITCH_VERSION_CHECK_CODE + 999)

#define Stitch_Block_Max     26000
#define Stitch_Sesnor_Max    8192
#define Stitch_3DModel_Max   512

#define log2(x) 	(log(x)*1.44269504088896340736)
#define log2f(x) 	((float)log2(x))

#define S2_Height	80


#define sensor_tiltX 0
#define sensor_tiltY 0

//#define D_Sensor_Zoom (Sensor_Size_Y * 2 * 0.94)
#define D_Sensor_Zoom(x) 	(x == 1)? 5990: 6100

//#define START_SENSOR_Y 67		//解影像底部閃綠色問題,改為68
//#define START_SENSOR_Y 68
#define START_SENSOR_Y 60
#define pi 3.141592654

#define Source_X_Delta  70
#define Source_Y_Delta  4

#define ISP2_T_Base 0x005B0000
#define Sitching_S_Base 0x00590000


#define Sensor_13_X_Offset   -100
#define Sensor_24_X_Offset   -501
//#define Sensor_Size_X   (4320 + 28 + 16)
//#define Sensor_Size_Y   (3252 + 24)
#define Sensor_Size_X   4354
#define Sensor_Size_Y   3274

#define Sensor_X_Step   4608
#define Sensor_Y_Step   1104


//#define Sensor_Size4_X  (Sensor_Size_X * 4)
//#define Sensor_Size4_Y  (Sensor_Size_Y * 4)


#define Sensor_C_X_Base_Default (Sensor_Size_X / 2)
#define Sensor_C_Y_Base_Default (Sensor_Size_Y / 2)
//OldLens: -0.28	NewLens: 0.3
#define RateDefault(x)	(x == 1)? 0.3: -0.28
#define SensorR3Default 18000

#define GPadj_Step_Cnt 33
#define GPadj_Step_Cnt_TestTool 17

//======================================================================
#define OUT_MODE_GLOBAL             0
#define OUT_MODE_FRONT              1
#define OUT_MODE_360                2
#define OUT_MODE_240                3
#define OUT_MODE_180                4
#define OUT_MODE_SPLIT4             5
#define OUT_MODE_PIP                6

#define RUL_0M_EMPTY                0
#define RUL_10M_6144_3072           1
#define RUL_2M5_3072_1536           2
#define RUL_2M_1920_1080            3
#define RUL_1M_1280_720             4
#define RUL_D1_720_480              5
#define RUL_1M8_3072_576            6
#define RUL_5M_4090_2048            7
#define RUL_8M_3840_2160            8
#define RUL_7M_6144_1152_P3072      9
#define RUL_CB_WIFI_640_640        10
#define RUL_CB_HDMI_1280_720       11
#define RUL_16M_6144_3072          12
#define RUL_4M_2880_1440           13
#define RUL_2M_1920_960            14

#define RUL_S2_0M_EMPTY             RUL_0M_EMPTY
//#define RUL_S2_66M_11500_5760       RUL_10M_6144_3072
//#define RUL_S2_8M_3840_1920         RUL_2M5_3072_1536
//#define RUL_S2_2M_1920_1080         RUL_2M_1920_1080
//#define RUL_S2_1M_1280_720          RUL_1M_1280_720
//#define RUL_S2_8M_3840_2160         RUL_D1_720_480
//#define RUL_S2_8M_3840_720          RUL_1M8_3072_576
//#define RUL_S2_33M_7680_3840        RUL_5M_4090_2048
//#define RUL_S2_33M_7680_4320        RUL_8M_3840_2160
//#define RUL_S2_66M_6144_11520_2160  RUL_7M_6144_1152_P3072
//#define RUL_S2_CB_WIFI_640_640      RUL_CB_WIFI_640_640
//#define RUL_S2_CB_HDMI_1280_720     RUL_CB_HDMI_1280_720
//#define RUL_S2_16M_6144_3072        RUL_16M_6144_3072
//#define RUL_S2_4M_2880_1440         RUL_4M_2880_1440
//#define RUL_S2_2M_1920_960          RUL_2M_1920_960
#define RUL_S2_66M_11500_5760           1
#define RUL_S2_33M_7680_3840            7
#define RUL_S2_16M_6144_3072            12
#define RUL_S2_8M_3840_1920             2
#define RUL_S2_4M_2880_1440             13
#define RUL_S2_2M_1920_960              14

struct Output_Ctl_Struct {
 int Resolution_Mode;
 // 0: Fix 1: G66M 2: G7M 3: 2M 4: 1M 5: 4K 6: 6K360 7: G29M 8: 8K 9: 12K360  10:Cardboard 11: 270

 // 0: Fix 1: 10M 2: 2.5M 3: 2M 4: 1M 5: D1 6: 3072x576 7: 5M 8: 8M 9: 7M

 int pan;                  // unit : 1 / 36000 degree
 int Tilt;                 // unit : 1 / 36000 degree
 int Rotate;               // unit : 1 / 36000 degree
 int Wide;                 // unit : 1 / 36000 degree
 int Rev[3];
};


struct Stitching_Out_Struct {
  int Output_Mode;
  int Rev[7];
  struct Output_Ctl_Struct  OC[32];		// 0: Global 1: Front 2: 360 3: 240 4: 180+180 5: Split x 4 6: PIP
};

struct US360_Stitching_Line_Table {
	  unsigned		CB_Mode; // 0: Image 1: Stitching Line 2: Global Phi Adj
	  unsigned		CB_Idx;
	  unsigned      X_Posi;
	  unsigned	    Y_Posi;
};

struct US360_Stitching_Table {
  unsigned		CB_DDR_P        :24;	// unit: burst
  unsigned  	CB_Sensor_ID	:3;
  unsigned  	CB_Alpha_Flag	:1;
  unsigned  	CB_Block_ID		:3;
  unsigned  	CB_Mask			:1;
  unsigned  	CB_Posi_X0     	:16;
  unsigned     	CB_Posi_Y0      :16;
  unsigned     	CB_Posi_X1      :16;
  unsigned     	CB_Posi_Y1      :16;
  unsigned     	CB_Posi_X2      :16;
  unsigned     	CB_Posi_Y2      :16;
  unsigned     	CB_Posi_X3      :16;
  unsigned     	CB_Posi_Y3      :16;
  unsigned     	CB_Adj_Y0       :8;
  unsigned     	CB_Adj_Y1       :8;
  unsigned     	CB_Adj_Y2       :8;
  unsigned     	CB_Adj_Y3       :8;
  unsigned     	CB_Adj_U0       :8;
  unsigned     	CB_Adj_U1       :8;
  unsigned     	CB_Adj_U2       :8;
  unsigned     	CB_Adj_U3       :8;
  unsigned     	CB_Adj_V0       :8;
  unsigned     	CB_Adj_V1       :8;
  unsigned     	CB_Adj_V2       :8;
  unsigned     	CB_Adj_V3       :8;
};

struct MP_B_Sub_Struct {
  unsigned  	V_YUV           :15;
  unsigned  	F_YUV           :1;
  unsigned  	P               :9;
  unsigned  	Rev             :7;
  unsigned  	V_Y             :15;
  unsigned  	F_Y             :1;
  unsigned  	V_X             :15;
  unsigned  	F_X             :1;
};	//size=8byte

struct MP_B_Struct {
  struct MP_B_Sub_Struct  	MP_A0;
  struct MP_B_Sub_Struct  	MP_B0;
  struct MP_B_Sub_Struct  	MP_A1;
  struct MP_B_Sub_Struct  	MP_B1;
  struct MP_B_Sub_Struct  	MP_A2;
  struct MP_B_Sub_Struct  	MP_B2;
  struct MP_B_Sub_Struct  	MP_A3;
  struct MP_B_Sub_Struct  	MP_B3;
};	//size=64byte



struct US360_Stitching_FeedBack {
	unsigned short     Y;
	unsigned short     U;
	unsigned short     V;
	unsigned short     Sum :  13;
	unsigned short     ID  :   3;
};



struct XY_Posi {
  int Sensor_Sel;

  short          I_phi;
  unsigned short I_thita;

  struct Posi_Struct S[5];          // 0~3 Sensor
  int    Transparent[5];
  unsigned C_Cnt[5];
  unsigned C_Line;
  unsigned Checked;
};


struct thita_phi_offset_Struct {
  int phi_offset;
  int thita_offset;
};

struct Adj_Sensor_Struct {
	int Rotate_R1;             // unit : 1 / 36000 degree  // AX
	int Rotate_R2;             // unit : 1 / 36000 degree  // AY
	int Rotate_R3;             // unit : 1 / 36000 degree  // AR
	int Rev1;

	unsigned int Zoom_X;       // unit : pixel/360         // AZ
	unsigned int Zoom_Y;       // unit : pixel/360         // AZ

	struct Posi_Struct S[5]; // 0~3  corner 4: center
	struct thita_phi_offset_Struct S2[24];
	struct thita_phi_offset_Struct S2_Position[24];
	int Rev2[16];
};

struct Trans_par {
	unsigned Source_P;        // unit : 32 byte
	unsigned Target_P;        // unit : 32 byte
	int Source_Scale;         // 0: 1920x1080x4  1: 960x540x4
	int Flip;                 // 0: Normal  1: H   2: V  3: Flip
	int T_Size_X;             // unit : 64 pixel
	int T_Size_Y;             // unit : 64 pixel
};



struct ISP2_Image_Table_Struct {
  int X0;
  int X1;
  int Y0;
  int Y1;
  int X_Block_Start;
  int X_Block_Stop;
};



struct Point_Posi {
  struct thita_phi_Struct G_t_p;
  struct thita_phi_Struct S_t_p[5];
  struct Posi_Struct S[5];
};


struct Adjust_Line_YUV_Struct {
  int Y;
  int U;
  int V;
};

struct Adjust_Line_RGB_Struct {
  int R;
  int G;
  int B;
};

struct Adjust_Line_RGB_Rate_Struct {
  float R;
  float G;
  float B;
};


#define Adjust_Line_I2_MAX 58
// Input
struct Adjust_Line_I2_Struct {
  float       Distance;
  float       Height;           // 垂直高度
  int         S_Id[2];
  int         A_L_S_Idx[2];
  int         XY_P[2][2];		// 中心點座標(x, y)
  float       F_phi;
  float       F_thita;
  float		  S_P2_XY[2][2]; 	// 計算後中心點偏移量(x,y)
  unsigned 	  Sum_Y[2];			// 縫合點平均亮度(16*16)
//  int         Diff_X;
//  int         Diff_Y;
  struct thita_phi_Struct  tp[2][2];
  struct XY_Posi     DP[2][2];        // S [P0.P1][Top.Bottom]
  float       T_phi;
  float       D_phi;
// from A_L_I
  int                    Top_Sensor;
  int                    Top_A_L_S_Idx;
  int                    Bottom_Sensor;
  int                    Bottom_A_L_S_Idx;
  struct thita_phi_Struct S_t_p1;              // Top Sensor
  struct thita_phi_Struct G_t_p[5];            // Global
  struct thita_phi_Struct S_t_p2;              // Bottom Sensor
  struct XY_Posi          S_XY[5];
  struct Posi_Struct      Screen;              // Screen Position
  int phi_offset;
  int thita_offset;
  struct D_thita_phi_Struct A_t_p1;              // Factory adjust
  struct thita_phi_Struct A_t_p2;              // Auto    adjust

};


// Output



#define  Stitch_Line_phi0    42.0
#define  Stitch_Line_phi1    62.0
#define  Stitch_Line_phi2    156.0

#define  Stitch_Line_thita0  45.0
#define  Stitch_Line_thita1  135.0
#define  Stitch_Line_thita2  225.0
#define  Stitch_Line_thita3  315.0

struct FPGA_DDR_One_Struct {
  int    Mode;     // Mode 0: Base on sensor    1: Line
  int    Y;
  int    X;
  int    DDR_P;
};


struct FPGA_DDR_Placement_Struct {
	struct FPGA_DDR_One_Struct ISP1[2][3][3];  // 0: B/S 1: page 2: Sensor
	struct FPGA_DDR_One_Struct ISP2[2][2][3];  // 0: B/S 1: page 2: Sensor
	struct FPGA_DDR_One_Struct NR3D[2][3];     // 0: B/S 1: Sensor
	struct FPGA_DDR_One_Struct Stitch[2][2];   // 0: B/S 1: page
	struct FPGA_DDR_One_Struct Jpeg[2][2];     // 0: B/S 1: page
	struct FPGA_DDR_One_Struct Jpeg_Header[2][2];  // 0: B/S 1: page
	struct FPGA_DDR_One_Struct Stitch_Command[2][2];  // 0: B/S 1: page
	struct FPGA_DDR_One_Struct Main_Command[2][2];  // 0: B/S 1: page
};

struct YUVC_Line_Struct {
  int Pan;                  // unit : 1 / 36000 degree
  int Tilt;                 // unit : 1 / 36000 degree
  struct Posi_Struct S[4]; // 0~3  corner
};

#define Test_Block_Table_MAX 64
struct Test_Block_Table_Struct {
	unsigned Sensor;
	unsigned Type;					//0:Focus 1:Auto Sitching
	unsigned Num;
	struct Posi_Struct S_P;		//ISP2 相對位置
	struct Posi_Struct T_P;		//縫合後相對位置
	float  S_P2_X;
	float  S_P2_Y;
	short  Pixel[128][128];
	unsigned Sum_Y;			//縫合點平均亮度(16*16)

	unsigned rev[8];
	//unsigned Error_Cnt;
};
extern struct Test_Block_Table_Struct Test_Block_Table_Default[Test_Block_Table_MAX];
extern struct Test_Block_Table_Struct Test_Block_Table[Test_Block_Table_MAX];

/*
 * SENSOR_ADJ_CHECK_SUM:
 * 0x20180725: 新增 stitch_ver
 */
#define SENSOR_ADJ_CHECK_SUM	0x20180725
typedef struct Test_Tool_Adj_Struct_h {
	int checksum;
	struct Adj_Sensor_Struct Adj_Sensor[5];			//同步本機與製具的 Adj_Sensor[] Struct
	struct Test_Block_Table_Struct Test_Block_Table[Test_Block_Table_MAX];
	unsigned SensorZoom;
	char test_tool_ver[32];							//製具寫回版本
	int stitch_ver;									//製具寫回縫合版本

	char rev[600];
}Test_Tool_Adj_Struct;
extern Test_Tool_Adj_Struct Test_Tool_Adj;

/*
 * TEST_RESULT_CHECK_SUM:
 * 0x20180725: 新增 CheckSum
 *   		        新增 StitchVer
 */
#define TEST_RESULT_CHECK_SUM	0x20180725
typedef struct Test_Tool_Result_Struct_h {
	char SSID[32];
	char TestToolVer[32];		//製具縫合校正時, 製具的版本
	char AletaVer[32];			//縫合校正時, 本機的版本
	int  TestBlockTableNum;
	int CheckSum;
	int StitchVer;				//紀錄縫合演算法的版本

	char rev[1016];
}Test_Tool_Result_Struct;
extern Test_Tool_Result_Struct Test_Tool_Result;

typedef struct Adj_Sensor_Lens_Struct_h {
	int checksum;

	float Sensor_Lens_Map_Th[5][80][128];
	float Sensor_Lens_Map_Bright[5][80][128];

	char rev[1024];
}Adj_Sensor_Lens_Struct;
extern Adj_Sensor_Lens_Struct Adj_Sensor_Lens_S;

//typedef struct Resolution_Spec_Struct_h {
//	int flash;
//	float frame_time;
//	int clk;
//	float fps;
//	float sw_time;
//}Resolution_Spec_Struct;
//extern Resolution_Spec_Struct Resolution_Spec[13][2];

//extern Resolution_Spec_Struct FPS_1_60Hz_Spec;
//extern Resolution_Spec_Struct FPS_1_50Hz_Spec;
//extern Resolution_Spec_Struct FPS_6_60Hz_Spec;
//extern Resolution_Spec_Struct FPS_5_50Hz_Spec;
//extern Resolution_Spec_Struct FPS_30_60Hz_Spec;
//extern Resolution_Spec_Struct FPS_25_50Hz_Spec;
//extern Resolution_Spec_Struct FPS_24_60Hz_Spec;
//extern Resolution_Spec_Struct FPS_10_60Hz_Spec;
//extern Resolution_Spec_Struct FPS_10_50Hz_Spec;

typedef struct EP_Line_2_Gain_Table_Struct_h {
	int max_idx;
	int min_idx;
	int table[350];		//350: 60Hz, clk=1, 281 Line; 50Hz, clk=1, 338 Line
}EP_Line_2_Gain_Table_Struct;
extern EP_Line_2_Gain_Table_Struct EP_Line_2_Gain_Table[2];


typedef struct A_L_I_Relation_Dis_Struct_h {
	int num;
	int dis;
}A_L_I_Relation_Dis_Struct;

typedef struct A_L_I_Relation_Struct_h {
	int sum;
	A_L_I_Relation_Dis_Struct p[6];
}A_L_I_Relation_Struct;
extern A_L_I_Relation_Struct ALI_R[40];
extern int ALI_R_D0_Rate, ALI_R_D1_Rate, ALI_R_D2_Rate;

typedef struct Sensor_Lens_Map_Parameter_Struct_h {
	int   st_p1;
	float rate1;

	int   st_p2;
	float rate2;

	float angle;
	float distance;
}Sensor_Lens_Map_Parameter_Struct;
extern Sensor_Lens_Map_Parameter_Struct Sensor_Lens_Map_P[5][64][512];

typedef struct ALS_Angle_Sorting_Buf_Struct_h {
	int idx;
	float angle;
	float distance;
}ALS_Angle_Sorting_Buf_Struct;
extern ALS_Angle_Sorting_Buf_Struct Sitching_Point_P[5][24];

typedef struct XY_Offset_Sturct_h {
	int X;
	int Y;
}XY_Offset_Sturct;

typedef struct Sensor_Calibration_Live_Struct_h {
	unsigned CheckSum;					//0x20180213
	unsigned CalEn;
	struct Adj_Sensor_Struct S[5];

	char rev[128];
}Sensor_Calibration_Live_Struct;

struct Out_Plant_Mode_Struct {
	struct Trans_par  TP;
	int Pan;                  // unit : 1 / 36000 degree
	int Tilt;                 // unit : 1 / 36000 degree
	int Rotate;               // unit : 1 / 36000 degree
	int Wide;                 // unit : 1 / 36000 degree
};

struct Out_Pillar_Mode_Struct {
	struct Trans_par  TP;
	int Start_thita;          // unit : 1 / 36000 degree
	int Stop_thita;           // unit : 1 / 36000 degree
	int Start_phi;            // unit : 1 / 36000 degree

	// 3D Model 傾斜校正用
	int Adj_yaw;			  	  // thita
	int Adj_pitch;			  	  // phi
	int Adj_roll;			  	  // rotate
};

//======================================================================

extern struct US360_Stitching_Table ST_I[2][Stitch_Block_Max];
//extern struct US360_Stitching_Table ST_O[2][Stitch_Block_Max];
extern struct US360_Stitching_Line_Table SLT[2][Stitch_Block_Max];
extern struct US360_Stitching_Table SL[Adjust_Line_I2_MAX*2];
extern struct US360_Stitching_Table *ST_P;
extern struct US360_Stitching_Line_Table *SLT_P;

extern int ST_Sum_Test[2];
extern struct US360_Stitching_Table ST_Test[2][16384];      // 32(B)x2x16K=1MB
extern struct US360_Stitching_Line_Table SLT_Test[2][16384];

extern struct Stitching_Out_Struct Stitching_Out;

extern struct ISP2_Image_Table_Struct Global_ISP2_Image_Table[5];
extern struct ISP2_Image_Table_Struct ISP2_Image_Table[5];

extern int Sensor_C_X_Base, Sensor_C_Y_Base;
extern struct Adj_Sensor_Struct   Adj_Sensor_Default[2][5];
extern struct Adj_Sensor_Struct   Adj_Sensor[2][5];

//extern struct Out_Global_Mode_Struct  Out_Mode_G_0;		// 11520 x 5760
//extern struct Out_Global_Mode_Struct  Out_Mode_G_1;		//  7680 x 3840
//extern struct Out_Global_Mode_Struct  Out_Mode_G_2;		//  3840 x 1920
//extern struct Out_Global_Mode_Struct  Out_Mode_G_3;		//  6144 x 3072
//extern struct Out_Global_Mode_Struct  Out_Mode_G_4;		//  2880 x 1440
//extern struct Out_Global_Mode_Struct  Out_Mode_G_5;		//  1920 x 960


extern float pi2;
extern int ST_Sum[8][2];
extern int SL_ST_Sum[8][2];
extern struct XY_Posi  Map_Posi_Tmp[100][200];           //rex+s 180622, 省記憶體[6]->[1]

extern struct Adj_Sensor_Struct   Adj_Sensor_Command[5];

extern unsigned SensorR3;
extern unsigned SensorZoom[2];


//extern int Sensor_Mask_Line_Shift;
extern int Sitching_offsetX, Sitching_offsetY;
extern float Rate[2];
extern int Total_tilt;

extern struct YUVC_Line_Struct   YUVC_Line[32];

extern int Send_SL_Idx;

extern unsigned short Trans_ZY_phi[129][256];
extern unsigned short Trans_ZY_thita[129][256];
extern unsigned short Trans_Sin[256];

extern unsigned long long Timestamp;	// weber+ 160413, JPEG檔時戳

extern int Adj_TY, Adj_TU, Adj_TV;
extern struct Adjust_Line_I2_Struct A_L_I2_Default[Adjust_Line_I2_MAX];
extern struct Adjust_Line_I2_Struct A_L_I2[Adjust_Line_I2_MAX];
extern struct Adjust_Line_I2_Struct A_L_I2_File[Adjust_Line_I2_MAX];
extern float Flash_2_Gain_Table[120];

typedef struct Sensor_Lens_Table_Struct_h {
	float gain;
	float table[10];
}Sensor_Lens_Table_Struct;
extern Sensor_Lens_Table_Struct Sensor_Lens_Table_S;

extern float Sensor_Lens_Gain;
extern float Sensor_Lens_Table[2][11];

extern float Sensor_Lens_Map[5][256][512];
extern float Sensor_Lens_Map_Th[5][256][512];
extern float Sensor_Lens_Map_Bright[5][256][512];
extern char Sensor_Lens_Map_Cmd[5][64][512];


//extern int display_en;

extern int T_Y_max, T_Y_min, T_U_max, T_V_max;
extern int SLens_Y_Top_Limit, SLens_Y_Low_Limit;

#define Sensor_Lens_Map_Cnt_Max 5
extern int Sensor_Lens_Map_Cnt;

extern int GPadj_Width, GPadj_Height;
extern int ALI_R_D0_Rate, ALI_R_D1_Rate, ALI_R_D2_Rate;

extern int Sensor_X_Step_debug;
extern float LensRateTable[2][16];
extern int ST_Line_Offset;
extern int do_calibration_flag;

extern int LensCode;
extern int Send_WDR_Live_Table_Flag;
extern float WDR_Live_Target_Table[1024];
extern int doSensorLensEn;

//======================================================================
void Make_A_L_I3_Table( void);
void Make_thita_rate3(int M_Mode);
void Get_thita_rate3(int M_Mode,int S_Id, short I_phi, unsigned short I_thita , struct MP_B_Sub_Struct *MP_BS_p, struct MP_B_Sub_Struct *MP_B2S_p, int Block_Mode, int Sub_Idx, int y, int x);
int I_Sin( unsigned short thita);
int I_Cos( unsigned short thita2);

void Stitch_Init_All(void);
void Test_Init_All(void);
void Make_ZY_Table( void);
void Trans_ZY( unsigned short phi1, unsigned short thita1, unsigned short *phi2, unsigned short *thita2 );
void Map_To_Sensor_Trans(int M_Mode, struct XY_Posi *XY_P);
void Add_ST_Table_Proc(int M_Mode, int V_Blocks, int H_Blocks, struct ST_Header_Struct *ST_H, int P_Mode, int t_offset);
void Map_3D_Rotate( float phi1, float thita1, unsigned short I_thita0, unsigned short I_phi0, unsigned short I_rotate0,unsigned short *I_phi3, unsigned short *I_thita35);
void Make_Lens_Rate_Line_Table_Proc( void);
//void Make_Sensor_Mask_Line_Table_Proc(void);
//void Write_Map_Proc(void);
//void Global_Proc(void);


void Get_YUVC_Line(void);
//void Global_Sensor_Trans(void);
void Test_Tool_Adj_Struct_Init(void);


void Global2Sensor(int M_Mode, int S_Id,short I_phi, unsigned short I_thita, int Global_degree);
void Sensor2Global(int S_Id, struct XY_Posi * XY_P, int Global_degree);
void Camera_Calibration(void);

//void ReadSensorAdj(void);
void Send_Adj_Sensor_Lens_File(void);

int Sensor_Lens_Adj(int mode);
void S_Rotate_R_Init(int M_Mode);

void Sensor_Lens_Map_Buf_Init(void);


int Get_EP_Gain(int idx, int freq);
int Get_EP_Line(int idx, int freq);
int Find_Flash_2_Gain_Table_Idx(int value);

int get_block_complexity(char *buf, int s_x, int s_y);
int get_block_dc(char *buf, int s_x, int s_y);
int get_block_sub_sum(char *buf, int s_x, int s_y);

#ifdef ANDROID_CODE
int send_Sitching_Cmd(int f_id);
int send_Sitching_Cmd_Test(void);
int send_Sitching_Tran_Cmd(int f_id);
int Send_MP_B_Table(int f_id, int idx);
#endif

void Write_Map_Proc(int M_Mode);
void ResolutionSpecInit(void);


struct Resolution_Mode_Struct {
  int  S_X;
  int  S_Y;
  int  bit_rate;
};

void Make_Map_sensor_id(void);

void Make_Transparent(int idx, struct XY_Posi * XY_P);

void Calibration_Sensor_Center(int last);
void Make_ALI2(void);
void Make_A_L_S2_Table(void);

float Sensor_Lens_Map_Angle(int cx, int cy, int px, int py);
float Sensor_Lens_Map_Distance(int cx, int cy, int px, int py);

void ALS_Angle_Sorting(void);
float Sensor_Lens_Map_Angle(int cx, int cy, int px, int py);
float Sensor_Lens_Map_Distance(int cx, int cy, int px, int py);
void Sensor_Lens_Map_ST_Init(void);
void Set_CB_Proc(struct US360_Stitching_Table *ST_P);

int ST_Table_Mix(int SF_A, int S_A, int F_A, int V_A, int SF_B, int S_B, int F_B, int V_B);
void Add_ST_O_Table_Proc(void);

void Get_M_Mode(int *M_Mode, int *S_Mode);
void do_ST_Line_Offset(void);

//void Set_JPEG_Size(int stream, int size);
//void Set_FPGA_Speed_USB(int M_Mode, int rate);

int Write_S_Cal_Live_File(void);
int Read_S_Cal_Live_File(void);

void Set_Calibration_Flag(int flag);

void SetTransparentEn(int en);
int GetTransparentEn(int mode);

void Get_FId_MT(int s_id, int *f_id, int *mt);
int send_Sitching_Cmd_3DModel(int f_id, int isInit);
int Send_MP_B_Table_3DModel(int f_id, int idx);
int send_Sitching_Tran_Cmd_3DModel(int f_id);
void SendSTCmd3DModel(int isInit);
void Make_Plant_To_Global_Table(struct Out_Plant_Mode_Struct plane_s);
void Write_3D_Model_Proc(int M_Mode, int isInit, struct Out_Plant_Mode_Struct *plant_s, struct Out_Pillar_Mode_Struct *pillar_s);
void Set_Angle_3DModel_Init(int en);

void get_Stitching_Out(int *mode, int *res);
void AdjFunction(void);
void SendSTCmd(void);
int doAutoStitch(void);
void WriteSensorAdjFile(void);
void ReadSensorAdjFile(void);
void SetColorSTSW(int en);
void doSensorLensAdj(int s_id, int mode);
int ReadLensCode();
void SetWDRLiveStrength(int value);
void LineTableInit(void);

//---------------------------------------
int WriteLensRateTable();
void ReadAdjSensorLensFile();
void setSensroLensTable(int idx, int value);
void SetSensorLensYLimit(int idx, int value);
void setSTMixEn(int en);
void setALIRelationRate(int idx, int value);
void setLensRateTable(int idx, int value);
void setSensorXStep(int value);
void setSensorLensRate(int value);
void setGlobalSensorXOffset(int idx, int offset);
void setGlobalSensorYOffset(int idx, int offset);
void SetAutoSTSW(int en);
void setSmooth(int idx, int yuvz, int value, int f_id);
void setShowSmoothIdx(int idx, int value);
void setWDR1PI0(int pi0);
void setWDR1PI1(int pi1);
void setWDR1PV0(int pv0);
void setWDR1PV1(int pv1);
void setWDRTablePixel(int pix);
void setWDR2PI0(int pi0);
void setWDR2PI1(int pi1);
void setWDR2PV0(int pv0);
void setWDR2PV1(int pv1);
void SetSLAdjGap0(int value);
void SetSensorLensDE(int value);
void SetSensorLensEnd(int value);
void SetSensorLensCY(int value);
void SetPlantParameter(int idx, int value);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__US360_FUNC_H__
