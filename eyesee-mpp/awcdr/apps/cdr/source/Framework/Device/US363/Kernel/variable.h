/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __VARIABLE_H__
#define __VARIABLE_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ANDROID_CODE
  #define SPEED_ISP2     77.310
  #define SPEED_Stitch   73.587
  //#define SPEED_Jpeg     91.261
  #define SPEED_Jpeg     88.261
  #define SPEED_DMA      100.0
  #define SPEED_USB      12.5
  #define SPEED_Base     50

//OldLens: 0.46		NewLens: 0.34
//  #define  LENS2IMG(x)             (x == 1)? 0.34: 0.46
//  #define  LENS2CEN0               (0.945-0.4)
//  #define  LENS2CEN1(x)            (4.556 - LENS2IMG(x) )
//
//  #define  RULE_MINI               100.0
//  #define  RULE_UNIT(x)            (65536.0 * LENS2CEN1(x) / RULE_MINI / 8)

  extern float LENS2IMG[2];
  extern float LENS2CEN0[2];
  extern float LENS2CEN1[2];

  extern float RULE_MINI;
  extern float RULE_UNIT[2];
#endif

struct FPGA_Speed_Struct {
  int ISP2;
  int Stitch_Img;
  int Stitch_ZYUV;
  int Smooth;
  int Jpeg;
  int H264;
  int DMA;
  int USB;
  int USB_H264;
};
extern struct FPGA_Speed_Struct FPGA_Speed[6];
extern int JPEG_Size[6];

struct Adjust_Line_I3_Header_Struct {
  int    Resolution;
  int    H_Degree_offset;	// 旋轉角, width / 64 / 4 = ? (奇數需要偏3度)
  int    Binn;              // Sensor Binn & Big Map Binn
  int    S_Binn;            // Small Binn
  int    FPGA_Binn;         // FPGA Binn
  int    Sum;
  unsigned Target_P;        // unit : 32 byte
  int    H_Blocks;
  int    V_Blocks;
  int    Phi_P[3];
  int    Thita_P[4];
};
extern struct Adjust_Line_I3_Header_Struct A_L_I3_Header[6];

struct Adjust_Line_I3_Line_Struct {
  int    Line_Mode;  // 0: H  1: V
  int    SL_Sensor_Id[2];
  int    SL_Point[6];
  int    SL_Sum[6];
};
extern struct Adjust_Line_I3_Line_Struct  A_L_I3_Line[8];

struct thita_phi_Struct {
  unsigned short phi;
  unsigned short thita;
};
struct D_thita_phi_Struct {
  short phi;
  short thita;
};
struct Adjust_Line_O_Struct {
  struct thita_phi_Struct G_t_p[5];               // Global
  struct thita_phi_Struct S_t_p1;              // Sensor  p1
  struct D_thita_phi_Struct A_t_p;               // Adjust sum
};
struct Adjust_Line_S2_Struct {
  int                  Sum;
  int                  Source_Idx[24];
  int                  Top_Bottom[24];   // 0 : Top  1: Bottom
  int                  D_X;
  int                  D_Y;
  int                  D_phi;
  int                  D_thita;
  int                  P0_phi[24];
  int                  P0_thita[24];
  int                  P1_phi[24];
  int                  P1_thita[24];
  int                  F_phi[24];
  int                  F_thita[24];

  struct Adjust_Line_O_Struct AL_p[24];
  unsigned short S_rate_Idx[0x1000];
  unsigned short S_rate_Sub[0x1000];
  
  float		       	   Rotate1[24];				//Default位置與Sensor中心角度
  float		       	   Rotate2[24];             //實際點的位置與Sensor中心角度
  float		       	   Rotate_Avg;             	//平均旋轉角度
  float                S_P2_XY[24][2];			//[Idx][X/Y]
  float                XY_Offset[24][2];		//[Idx][X/Y]
  float                XY_Offset_Avg[2];		//XY平均偏移量, [X/Y]
  int				   Distance[24]; 			//實際點的位置與Sensor中心距離
};

struct Adjust_Line_S3_Struct {
  int                  Sum;
  int                  Source_Idx[256];
  int                  Top_Bottom[256];   // 0 : Top  1: Bottom
  struct thita_phi_Struct S_t_p[256];       // Sensor  p1
  unsigned short S_rate_Idx[0x1000];
  unsigned short S_rate_Sub[0x1000];
};

struct Posi_Struct {
  int X;
  int Y;
  int C;
  int C_Y;
  int C_U;
  int C_V;
};
struct A_L_I_Posi {
  struct thita_phi_Struct   G_t_p[4];
  struct thita_phi_Struct   S_t_p[4];
  struct Posi_Struct        XY[4];
  int    Target_P;
};
struct Adjust_Line_I3_P_Struct {
  int                      Sensor_Idx;
  int                      A_L_S_Idx;
  struct thita_phi_Struct  S_t_p;              // Top Sensor
  struct A_L_I_Posi        YUV;
  struct A_L_I_Posi        Z;
};
struct YC_Temp_Struct {
	int Y;
	int U;
	int V;
};
struct Adjust_Line_I3_Struct {
  int                             Line_Mode;  // 0: H  1: V
  int                             Line_No;
  int                             Line_Idx;
  struct thita_phi_Struct         G_t_p;
  struct Adjust_Line_I3_P_Struct  p[2];

  int                             Smooth_P[64];     // 最多使用64點，執行擴散係數
  int                             Smooth_V[64];
  int                             Smooth_Total;
  int                             Smooth_Sum;

  struct YC_Temp_Struct           YC_Temp[2];
  struct YC_Temp_Struct           YC_Sub;
};

extern struct Adjust_Line_S2_Struct A_L_S2[5];
extern struct Adjust_Line_S3_Struct A_L_S3[6][5];
extern struct Adjust_Line_I3_Struct A_L_I3[6][512];

// rex+ 180315
#define  ST_H_Img        0
#define  ST_H_Trans      1
#define  ST_H_YUV        2
#define  ST_H_ZV         3
#define  ST_H_ZH         4

struct ST_Header_Struct {
  int    Size;
  int    Start_Idx[5];  // 0: Map 1:YUV 2: Z
  int    Sum[5][2];		// [idx][f_id]
};  // 384(bytes)
extern struct ST_Header_Struct ST_Header[6];
extern struct ST_Header_Struct ST_S_Header[5];
extern struct ST_Header_Struct ST_3DModel_Header[8];

#define Head_REC_Start_Code	0x03000000
typedef struct H_File_Head_rec_H
{
    unsigned		Start_Code;			// (4) 		0x03000000
    unsigned char	CameraMode;			// (1)
    unsigned char	Resolution;			// (1)
    unsigned		Time_Stamp;			// (4)
    unsigned		Frame_Stamp;		// (4)
    unsigned 		Size_V;				// (4)
    unsigned 		Size_H;				// (4)
    unsigned 		QP			: 8;	//
    unsigned 		F_Cnt		: 8;	//          rex+ Frame Count (Max = 15)
    unsigned 		Alpha		: 8;	//
    unsigned 		IP_M		: 3;	//         	0: I   1: P
    unsigned 		Beta		: 5;	// (4)
    unsigned char	clip		: 5;	//
    unsigned char	rev_bit		: 3;	// (1)
    unsigned char   rev[3];				// (3)
#ifdef ANDROID_CODE
} __attribute__((packed)) H_File_Head_rec;			// total 30 bytes
#else
} H_File_Head_rec;			// total 30 bytes
#endif

void do_sc_cmd_Tx_func(char *cmd_M, char *cmd_S);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif  //__VARIABLE_H__
