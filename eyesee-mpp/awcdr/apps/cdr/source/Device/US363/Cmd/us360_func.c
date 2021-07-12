/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/
 
#include "Device/US363/Cmd/us360_func.h"
  
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>              /* low-level i/o */
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <math.h>
 
#include "Device/us363_camera.h"
#include "Device/US363/us363_para.h"
#include "Device/US363/Cmd/Smooth.h"
#include "Device/US363/Cmd/variable.h"
#include "Device/US363/Cmd/us360_define.h"
#include "Device/US363/Cmd/AletaS2_CMD_Struct.h"
//#ifdef ANDROID_CODE
#include "Device/US363/Cmd/spi_cmd.h"
#include "Device/US363/Test/test.h"
#include "Device/US363/Cmd/fpga_driver.h"
//#else
#include "Device/US363/Kernel/FPGA_Pipe.h"
//#endif
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::US360Func"

/*float  LENS2IMG[2]  = {0.46, 0.34};
float  LENS2CEN0    = (0.945-0.4);
float  LENS2CEN1[2] = {					//(4.556 - LENS2IMG(x) )
		(4.556-0.46),
		(4.556-0.34)
};
float  RULE_MINI    = 100.0;
float  RULE_UNIT[2] = {					//(65536.0 * LENS2CEN1(x) / RULE_MINI / 8)
		(65536.0 * (4.556-0.46) / 100.0 / 8),
		(65536.0 * (4.556-0.34) / 100.0 / 8)
};*/

float  LENS2IMG[2]  = {0.8, 0.65};
float  LENS2CEN0[2] = { (0.945-0.8), (0.945-0.65) };
float  LENS2CEN1[2] = {					//(4.556 - LENS2IMG(x) )
		(4.556-0.8),
		(4.556-0.65)
};
float  RULE_MINI    = 100.0;
float  RULE_UNIT[2] = {					//(65536.0 * LENS2CEN1(x) / RULE_MINI / 8)
		(65536.0 * (4.556-0.8) / 100.0 / 8),
		(65536.0 * (4.556-0.65) / 100.0 / 8)
};

struct US360_Stitching_Table ST_I[2][Stitch_Block_Max];
//struct US360_Stitching_Table ST_O[2][Stitch_Block_Max];
struct US360_Stitching_Line_Table SLT[2][Stitch_Block_Max];
struct MP_B_Struct  MP_B[2][Stitch_Block_Max];
struct MP_B_Struct  MP_B_2[2][Stitch_Block_Max];
char   Transparent_B[2][Stitch_Block_Max][4];

int ST_Sum_Test[2];
struct US360_Stitching_Table ST_Test[2][16384];			//測試程式 & debug程式使用
struct US360_Stitching_Line_Table SLT_Test[2][16384];

//          M_Mode Page
struct ST_Header_Struct  ST_Header[6] = {		//190717 解倒置色縫合錯誤問題, 對齊8
  {16200, {0	 , 9872 ,10472,10928,11384}},
  { 7200, {11968 ,16392 ,16800,17104,17408}},
  { 4608, {17768 ,20632 ,20960,21200,21440}},
  { 1800, {21760 ,22864 ,23072,23224,23376}},
  { 1152, {23584 ,24280 ,24448,24568,24688}},
  {  512, {24872 ,25192 ,25304,25384,25464}}
};

//Cap 5 Sensor ST Cmd
struct US360_Stitching_Table ST_S[Stitch_Sesnor_Max];       // #define Stitch_Sesnor_Max  	8192 * 32b = 0x40000(b)
struct ST_Header_Struct  ST_S_Header[5] = {     
  {1296, {0   , } },         //5104 ,                   // Removal, ((768*3)>>6) x (576>>6) x 4 = 324 x 4 = 1296
  {1296, {1304, } },         //6408 ,                   //          ((768*3)>>6) x (512>>6) x 4 = 288 x 4 = 1152
  {3710, {2608, } },         //0    ,                   // RAW, 70x53=3710, (4480 >> 6)=70, (3392 >> 6)=53
  { 945, {6328, } },         //3712 , 
  { 432, {7280, } }          //4664 , 
};                        
/*
  int    Size;
  int    Start_Idx[5];  // 0: Map 1:YUV 2: Z
  int    Sum[5][2];
*/

struct US360_Stitching_Table SL[Adjust_Line_I2_MAX*2];
struct US360_Stitching_Table *ST_P;
struct US360_Stitching_Line_Table *SLT_P;
struct MP_B_Struct *MP_B_P, *MP_B2_P;

struct Stitching_Out_Struct Stitching_Out = {
  0, {0},
  {
    { 1, 0, 0, 0, 0, {0} },
    { 1, 0, 0, 0, 0, {0} },
    { 1, 0, 0, 0, 0, {0} },
    { 1, 0, 0, 0, 0, {0} },
    { 1, 0, 0, 0, 0, {0} },
    { 1, 0, 0, 0, 0, {0} },
    { 1, 0, 0, 0, 0, {0} },
    { 1, 0, 0, 0, 0, {0} }
  }
};

struct ISP2_Image_Table_Struct Global_ISP2_Image_Table[5];
struct ISP2_Image_Table_Struct ISP2_Image_Table[5];

int Sensor_C_X_Base = Sensor_C_X_Base_Default, Sensor_C_Y_Base = Sensor_C_Y_Base_Default;

struct Adj_Sensor_Struct   Adj_Sensor_Default[2][5]=		//[LensCode][Sensor]
{
	{	{     0,  9000, 17700, 0,  D_Sensor_Zoom(0), D_Sensor_Zoom(0), { {0,0}, {0,0}, {0,0}, {0,0}, {    0,  0} } },
		{  -300, -1400, -9000, 0,  D_Sensor_Zoom(0), D_Sensor_Zoom(0), { {0,0}, {0,0}, {0,0}, {0,0}, { -120,  0} } },
		{  8700, -1400,  9000, 0,  D_Sensor_Zoom(0), D_Sensor_Zoom(0), { {0,0}, {0,0}, {0,0}, {0,0}, { -470,  0} } },
		{-18300, -1400, -9000, 0,  D_Sensor_Zoom(0), D_Sensor_Zoom(0), { {0,0}, {0,0}, {0,0}, {0,0}, { -120,  0} } },
		{ -9300, -1400,  9000, 0,  D_Sensor_Zoom(0), D_Sensor_Zoom(0), { {0,0}, {0,0}, {0,0}, {0,0}, { -470,  0} } } 	},

	{	{     0,  9000, 17700, 0,  D_Sensor_Zoom(1), D_Sensor_Zoom(1), { {0,0}, {0,0}, {0,0}, {0,0}, {    0,  0} } },
		{  -300, -1400, -9000, 0,  D_Sensor_Zoom(1), D_Sensor_Zoom(1), { {0,0}, {0,0}, {0,0}, {0,0}, { -120,  0} } },
		{  8700, -1400,  9000, 0,  D_Sensor_Zoom(1), D_Sensor_Zoom(1), { {0,0}, {0,0}, {0,0}, {0,0}, { -470,  0} } },
		{-18300, -1400, -9000, 0,  D_Sensor_Zoom(1), D_Sensor_Zoom(1), { {0,0}, {0,0}, {0,0}, {0,0}, { -120,  0} } },
		{ -9300, -1400,  9000, 0,  D_Sensor_Zoom(1), D_Sensor_Zoom(1), { {0,0}, {0,0}, {0,0}, {0,0}, { -470,  0} } } 	}
};

struct Adj_Sensor_Struct   Adj_Sensor[2][5]=
{
	{	{     0,  9000, 17700, 0,  D_Sensor_Zoom(0), D_Sensor_Zoom(0), { {0,0}, {0,0}, {0,0}, {0,0}, {    0,  0} } },
		{  -300, -1400, -9000, 0,  D_Sensor_Zoom(0), D_Sensor_Zoom(0), { {0,0}, {0,0}, {0,0}, {0,0}, { -120,  0} } },
		{  8700, -1400,  9000, 0,  D_Sensor_Zoom(0), D_Sensor_Zoom(0), { {0,0}, {0,0}, {0,0}, {0,0}, { -470,  0} } },
		{-18300, -1400, -9000, 0,  D_Sensor_Zoom(0), D_Sensor_Zoom(0), { {0,0}, {0,0}, {0,0}, {0,0}, { -120,  0} } },
		{ -9300, -1400,  9000, 0,  D_Sensor_Zoom(0), D_Sensor_Zoom(0), { {0,0}, {0,0}, {0,0}, {0,0}, { -470,  0} } }	},

	{	{     0,  9000, 17700, 0,  D_Sensor_Zoom(1), D_Sensor_Zoom(1), { {0,0}, {0,0}, {0,0}, {0,0}, {    0,  0} } },
		{  -300, -1400, -9000, 0,  D_Sensor_Zoom(1), D_Sensor_Zoom(1), { {0,0}, {0,0}, {0,0}, {0,0}, { -120,  0} } },
		{  8700, -1400,  9000, 0,  D_Sensor_Zoom(1), D_Sensor_Zoom(1), { {0,0}, {0,0}, {0,0}, {0,0}, { -470,  0} } },
		{-18300, -1400, -9000, 0,  D_Sensor_Zoom(1), D_Sensor_Zoom(1), { {0,0}, {0,0}, {0,0}, {0,0}, { -120,  0} } },
		{ -9300, -1400,  9000, 0,  D_Sensor_Zoom(1), D_Sensor_Zoom(1), { {0,0}, {0,0}, {0,0}, {0,0}, { -470,  0} } }	}
};

// =====================================



#define pi 3.141592654
float pi2 = pi/2;
int ST_Sum[8][2];
int SL_ST_Sum[8][2];
struct XY_Posi  Map_Posi_Tmp[100][200];

struct Adj_Sensor_Struct   Adj_Sensor_Command[5];

unsigned SensorR3 = SensorR3Default;
unsigned SensorZoom[2] = {D_Sensor_Zoom(0), D_Sensor_Zoom(1) };

#define Lens_Rate_Line_MAX  8192
int Lens_Rate_Line[Lens_Rate_Line_MAX];


struct thita_phi_Struct I_t_p_O_Temp[5];
struct Point_Posi       PP_Temp;
struct XY_Posi          XY_Temp;

#define A_L_I2_phiA0   (0x4000 - 11 * 0x10000 / 60)
#define A_L_I2_phiA1   (0x4000 - 13 * 0x10000 / 60)
#define A_L_I2_phiA2   (0x4000 - 15 * 0x10000 / 60)
#define A_L_I2_phiA3   (0x4000 - 17 * 0x10000 / 60)
#define A_L_I2_phiA4   (0x4000 - 19 * 0x10000 / 60)
#define A_L_I2_phiA5   (0x4000 - 21 * 0x10000 / 60)
#define A_L_I2_phiA6   (0x4000 - 23 * 0x10000 / 60)
#define A_L_I2_phiA7   (0x4000 - 25 * 0x10000 / 60)

#define A_L_I2_phiB0   (0x4000 -  7 * 0x10000 / 60)
#define A_L_I2_phiB1   (0x4000 - 10 * 0x10000 / 60)
#define A_L_I2_phiB2   (0x4000 -  8 * 0x10000 / 60)
#define A_L_I2_phiB3   (0x4000 - 27 * 0x10000 / 60)

#define A_L_I2_thitaA0 (53 * 0x10000 / 60)
#define A_L_I2_thitaA1 (38 * 0x10000 / 60)
#define A_L_I2_thitaA2 (23 * 0x10000 / 60)
#define A_L_I2_thitaA3 ( 8 * 0x10000 / 60)

#define A_L_I2_thitaB0 	( 6 * 0x10000 / 60)
#define A_L_I2_thitaB1 	( 3 * 0x10000 / 60)
#define A_L_I2_thitaB2 	( 0 * 0x10000 / 60)
#define A_L_I2_thitaB3 	(57 * 0x10000 / 60)
#define A_L_I2_thitaB4 	(54 * 0x10000 / 60)

#define A_L_I2_thitaB5 	(51 * 0x10000 / 60)
#define A_L_I2_thitaB6 	(48 * 0x10000 / 60)
#define A_L_I2_thitaB7 	(45 * 0x10000 / 60)
#define A_L_I2_thitaB8 	(42 * 0x10000 / 60)
#define A_L_I2_thitaB9 	(39 * 0x10000 / 60)

#define A_L_I2_thitaB10 (36 * 0x10000 / 60)
#define A_L_I2_thitaB11 (33 * 0x10000 / 60)
#define A_L_I2_thitaB12 (30 * 0x10000 / 60)
#define A_L_I2_thitaB13 (27 * 0x10000 / 60)
#define A_L_I2_thitaB14 (24 * 0x10000 / 60)

#define A_L_I2_thitaB15 (21 * 0x10000 / 60)
#define A_L_I2_thitaB16 (18 * 0x10000 / 60)
#define A_L_I2_thitaB17 (15 * 0x10000 / 60)
#define A_L_I2_thitaB18 (12 * 0x10000 / 60)
#define A_L_I2_thitaB19 ( 9 * 0x10000 / 60)

struct Adjust_Line_I2_Struct A_L_I2_Default[Adjust_Line_I2_MAX] = {
	// 0~7
	{ 94.0 ,    0.0, { 1,2}, { 0,20}, {{ 134, 247}, {1169, 250}}, A_L_I2_phiA7, A_L_I2_thitaA0,  {{0, 0},{0, 0}}, {0, 0} },
	{ 121.5 ,   0.0, { 1,2}, { 1,19}, {{ 308, 149}, { 995, 149}}, A_L_I2_phiA6, A_L_I2_thitaA0,  {{0, 0},{0, 0}}, {0, 0} },
	{ 153.5 ,   0.0, { 1,2}, { 2,18}, {{ 421, 101}, { 880, 103}}, A_L_I2_phiA5, A_L_I2_thitaA0,  {{0, 0},{0, 0}}, {0, 0} },
	{ 178.5 ,  40.0, { 1,2}, { 3,17}, {{ 645,  43}, { 655,  43}}, A_L_I2_phiA4, A_L_I2_thitaA0,  {{0, 0},{0, 0}}, {0, 0} },
	{ 175.5 ,  64.0, { 1,2}, { 4,16}, {{ 740,  27}, { 559,  28}}, A_L_I2_phiA3, A_L_I2_thitaA0,  {{0, 0},{0, 0}}, {0, 0} },
	{ 175.0 ,  88.0, { 1,2}, { 5,15}, {{ 840,  21}, { 459,  23}}, A_L_I2_phiA2, A_L_I2_thitaA0,  {{0, 0},{0, 0}}, {0, 0} },
	{ 179.5 , 119.5, { 1,2}, { 6,14}, {{ 962,  24}, { 335,  26}}, A_L_I2_phiA1, A_L_I2_thitaA0,  {{0, 0},{0, 0}}, {0, 0} },
	{ 191.0 , 154.0, { 1,2}, { 7,13}, {{1087,  40}, { 210,  43}}, A_L_I2_phiA0, A_L_I2_thitaA0,  {{0, 0},{0, 0}}, {0, 0} },

	// 8~15
	{ 94.0 ,    0.0, { 2,3}, { 0,22}, {{1166, 847}, { 125, 841}}, A_L_I2_phiA7, A_L_I2_thitaA1,  {{0, 0},{0, 0}}, {0, 0} },
	{ 121.5 ,   0.0, { 2,3}, { 1,21}, {{ 997, 942}, { 298, 942}}, A_L_I2_phiA6, A_L_I2_thitaA1,  {{0, 0},{0, 0}}, {0, 0} },
	{ 153.5 ,   0.0, { 2,3}, { 2,20}, {{ 880, 990}, { 413, 990}}, A_L_I2_phiA5, A_L_I2_thitaA1,  {{0, 0},{0, 0}}, {0, 0} },
	{ 174.5 ,  40.0, { 2,3}, { 3,19}, {{ 656,1049}, { 638,1052}}, A_L_I2_phiA4, A_L_I2_thitaA1,  {{0, 0},{0, 0}}, {0, 0} },
	{ 171.0 ,  64.0, { 2,3}, { 4,18}, {{ 559,1066}, { 736,1066}}, A_L_I2_phiA3, A_L_I2_thitaA1,  {{0, 0},{0, 0}}, {0, 0} },
	{ 170.0 ,  88.0, { 2,3}, { 5,17}, {{ 456,1071}, { 838,1072}}, A_L_I2_phiA2, A_L_I2_thitaA1,  {{0, 0},{0, 0}}, {0, 0} },
	{ 174.0 , 119.5, { 2,3}, { 6,16}, {{ 329,1068}, { 968,1068}}, A_L_I2_phiA1, A_L_I2_thitaA1,  {{0, 0},{0, 0}}, {0, 0} },
	{ 185.5 , 154.0, { 2,3}, { 7,15}, {{ 200,1049}, {1097,1049}}, A_L_I2_phiA0, A_L_I2_thitaA1,  {{0, 0},{0, 0}}, {0, 0} },

	// 16~23
	{ 93.5 ,    0.0, { 3,4}, { 0,20}, {{ 128, 243}, {1150, 244}}, A_L_I2_phiA7, A_L_I2_thitaA2,  {{0, 0},{0, 0}}, {0, 0} },
	{ 121.5 ,   0.0, { 3,4}, { 1,19}, {{ 307, 144}, { 971, 142}}, A_L_I2_phiA6, A_L_I2_thitaA2,  {{0, 0},{0, 0}}, {0, 0} },
	{ 153.5 ,   0.0, { 3,4}, { 2,18}, {{ 424,  98}, { 853,  96}}, A_L_I2_phiA5, A_L_I2_thitaA2,  {{0, 0},{0, 0}}, {0, 0} },
	{ 176.5 ,  40.0, { 3,4}, { 3,17}, {{ 641,  40}, { 636,  38}}, A_L_I2_phiA4, A_L_I2_thitaA2,  {{0, 0},{0, 0}}, {0, 0} },
	{ 172.5 ,  64.0, { 3,4}, { 4,16}, {{ 738,  25}, { 538,  24}}, A_L_I2_phiA3, A_L_I2_thitaA2,  {{0, 0},{0, 0}}, {0, 0} },
	{ 171.5 ,  88.0, { 3,4}, { 5,15}, {{ 840,  19}, { 435,  19}}, A_L_I2_phiA2, A_L_I2_thitaA2,  {{0, 0},{0, 0}}, {0, 0} },
	{ 176.5 , 119.5, { 3,4}, { 6,14}, {{ 968,  23}, { 309,  25}}, A_L_I2_phiA1, A_L_I2_thitaA2,  {{0, 0},{0, 0}}, {0, 0} },
	{ 187.0 , 154.0, { 3,4}, { 7,13}, {{1100,  43}, { 176,  43}}, A_L_I2_phiA0, A_L_I2_thitaA2,  {{0, 0},{0, 0}}, {0, 0} },

	// 24~31
	{ 93.5 ,    0.0, { 4,1}, { 0,22}, {{1147, 845}, { 136, 839}}, A_L_I2_phiA7, A_L_I2_thitaA3,  {{0, 0},{0, 0}}, {0, 0} },
	{ 121.0 ,   0.0, { 4,1}, { 1,21}, {{ 971, 943}, { 313, 942}}, A_L_I2_phiA6, A_L_I2_thitaA3,  {{0, 0},{0, 0}}, {0, 0} },
	{ 153.5 ,   0.0, { 4,1}, { 2,20}, {{ 855, 989}, { 429, 986}}, A_L_I2_phiA5, A_L_I2_thitaA3,  {{0, 0},{0, 0}}, {0, 0} },
	{ 181.0 ,  40.0, { 4,1}, { 3,19}, {{ 638,1047}, { 646,1043}}, A_L_I2_phiA4, A_L_I2_thitaA3,  {{0, 0},{0, 0}}, {0, 0} },
	{ 176.5 ,  64.0, { 4,1}, { 4,18}, {{ 543,1061}, { 740,1060}}, A_L_I2_phiA3, A_L_I2_thitaA3,  {{0, 0},{0, 0}}, {0, 0} },
	{ 175.5 ,  88.0, { 4,1}, { 5,17}, {{ 445,1068}, { 838,1066}}, A_L_I2_phiA2, A_L_I2_thitaA3,  {{0, 0},{0, 0}}, {0, 0} },
	{ 180.0 , 119.5, { 4,1}, { 6,16}, {{ 322,1065}, { 962,1063}}, A_L_I2_phiA1, A_L_I2_thitaA3,  {{0, 0},{0, 0}}, {0, 0} },
	{ 190.5 , 154.0, { 4,1}, { 7,15}, {{ 190,1048}, {1093,1043}}, A_L_I2_phiA0, A_L_I2_thitaA3,  {{0, 0},{0, 0}}, {0, 0} },

	// 32~36
	{ 204.5 , 210.0, { 0,1}, { 0,13}, {{ 437,  89}, {1305, 849}}, A_L_I2_phiB0, A_L_I2_thitaB0,  {{0, 0},{0, 0}}, {0, 0} },
	{ 191.5 , 215.0, { 0,1}, { 1,12}, {{ 573,  62}, {1337, 706}}, A_L_I2_phiB0, A_L_I2_thitaB1,  {{0, 0},{0, 0}}, {0, 0} },
	{ 187.0 , 220.0, { 0,1}, { 2,11}, {{ 721,  49}, {1350, 544}}, A_L_I2_phiB0, A_L_I2_thitaB2,  {{0, 0},{0, 0}}, {0, 0} },
	{ 190.5 , 215.0, { 0,1}, { 3,10}, {{ 875,  55}, {1338, 381}}, A_L_I2_phiB0, A_L_I2_thitaB3,  {{0, 0},{0, 0}}, {0, 0} },
	{ 203.5 , 210.0, { 0,1}, { 4, 9}, {{1011,  73}, {1304, 236}}, A_L_I2_phiB0, A_L_I2_thitaB4,  {{0, 0},{0, 0}}, {0, 0} },

	// 37~41
	{ 166.5 , 156.0, { 0,2}, { 6,12}, {{1333, 149}, { 180, 172}}, A_L_I2_phiB1, A_L_I2_thitaB5,  {{0, 0},{0, 0}}, {0, 0} },
	{ 150.5 , 156.0, { 0,2}, { 7,11}, {{1393, 322}, { 141, 340}}, A_L_I2_phiB1, A_L_I2_thitaB6,  {{0, 0},{0, 0}}, {0, 0} },
	{ 144.5 , 156.0, { 0,2}, { 8,10}, {{1423, 536}, { 123, 540}}, A_L_I2_phiB1, A_L_I2_thitaB7,  {{0, 0},{0, 0}}, {0, 0} },
	{ 149.5 , 156.0, { 0,2}, { 9, 9}, {{1407, 750}, { 135, 742}}, A_L_I2_phiB1, A_L_I2_thitaB8,  {{0, 0},{0, 0}}, {0, 0} },
	{ 164.0 , 156.0, { 0,2}, {10, 8}, {{1356, 925}, { 172, 909}}, A_L_I2_phiB1, A_L_I2_thitaB9,  {{0, 0},{0, 0}}, {0, 0} },

	// 42~46
	{ 202.5 , 210.0, { 0,3}, {12,13}, {{1040,1026}, {1306, 854}}, A_L_I2_phiB0, A_L_I2_thitaB10, {{0, 0},{0, 0}}, {0, 0} },
	{ 189.5 , 215.0, { 0,3}, {13,12}, {{ 902,1052}, {1345, 708}}, A_L_I2_phiB0, A_L_I2_thitaB11, {{0, 0},{0, 0}}, {0, 0} },
	{ 184.0 , 220.0, { 0,3}, {14,11}, {{ 750,1060}, {1361, 543}}, A_L_I2_phiB0, A_L_I2_thitaB12, {{0, 0},{0, 0}}, {0, 0} },
	{ 187.5 , 215.0, { 0,3}, {15,10}, {{ 604,1053}, {1352, 386}}, A_L_I2_phiB0, A_L_I2_thitaB13, {{0, 0},{0, 0}}, {0, 0} },
	{ 198.5 , 210.0, { 0,3}, {16, 9}, {{ 469,1031}, {1320, 241}}, A_L_I2_phiB0, A_L_I2_thitaB14, {{0, 0},{0, 0}}, {0, 0} },

	// 47~51
	{ 163.5 , 156.0, { 0,4}, {18,12}, {{ 144, 962}, { 150, 174}}, A_L_I2_phiB1, A_L_I2_thitaB15, {{0, 0},{0, 0}}, {0, 0} },
	{ 148.4 , 156.0, { 0,4}, {19,11}, {{  84, 787}, { 113, 345}}, A_L_I2_phiB1, A_L_I2_thitaB16, {{0, 0},{0, 0}}, {0, 0} },
	{ 143.5 , 156.0, { 0,4}, {20,10}, {{  56, 577}, { 100, 543}}, A_L_I2_phiB1, A_L_I2_thitaB17, {{0, 0},{0, 0}}, {0, 0} },
	{ 149.0 , 156.0, { 0,4}, {21, 9}, {{  72, 368}, { 119, 743}}, A_L_I2_phiB1, A_L_I2_thitaB18, {{0, 0},{0, 0}}, {0, 0} },
	{ 165.0 , 156.0, { 0,4}, {22, 8}, {{ 121, 191}, { 158, 909}}, A_L_I2_phiB1, A_L_I2_thitaB19, {{0, 0},{0, 0}}, {0, 0} },

	// 52~55
	{ 234.0 , 167.5, { 0,1}, { 5, 8}, {{1201,  70}, {1170,  62}}, A_L_I2_phiB2, A_L_I2_thitaA0,  {{0, 0},{0, 0}}, {0, 0} },
	{ 230.0 , 167.5, { 0,3}, {11,14}, {{1219,1019}, {1184,1025}}, A_L_I2_phiB2, A_L_I2_thitaA1,  {{0, 0},{0, 0}}, {0, 0} },
	{ 228.0 , 167.5, { 0,3}, {17, 8}, {{277, 1040}, {1182,  62}}, A_L_I2_phiB2, A_L_I2_thitaA2,  {{0, 0},{0, 0}}, {0, 0} },
	{ 232.0 , 167.5, { 0,1}, {23,14}, {{ 252,  93}, {1173,1023}}, A_L_I2_phiB2, A_L_I2_thitaA3,  {{0, 0},{0, 0}}, {0, 0} },

	// 56~57
	{  81.5 ,   0.0, { 2,4}, {21,22}, {{1414, 348}, {1397, 748}}, A_L_I2_phiB3, A_L_I2_thitaB2,  {{0, 0},{0, 0}}, {0, 0} },
	{  81.5 ,   0.0, { 2,4}, {22,21}, {{1412, 758}, {1399, 339}}, A_L_I2_phiB3, A_L_I2_thitaB12, {{0, 0},{0, 0}}, {0, 0} }
};
struct Adjust_Line_I2_Struct A_L_I2[Adjust_Line_I2_MAX];
struct Adjust_Line_I2_Struct A_L_I2_File[Adjust_Line_I2_MAX];

unsigned short Trans_ZY_phi[129][256];
unsigned short Trans_ZY_thita[129][256];
// Sin table  +- 0x4000 = 1.0 ~ -1.0
unsigned short Trans_Sin[256];

struct Test_Block_Table_Struct Test_Block_Table_Default[Test_Block_Table_MAX] = {
		{0, 0, 0, { 208, 233}, {  0, 128} },
		{0, 0, 0, {1084, 201}, {128, 128} },
		{1, 0, 1, { 620,  56}, {256, 128} },
		{1, 0, 1, { 659,  40}, {384, 128} },
		{2, 0, 2, { 845,  14}, {512, 128} },
		{2, 0, 2, { 429,  16}, {640, 128} },
		{3, 0, 3, {1133,  29}, {768, 128} },
		{3, 0, 3, { 139,  34}, {896, 128} },
//  Miller 20170713

		{1, 1, 0, { 208, 233}, {  0, 128} },
		{2, 2, 0, {1084, 201}, {128, 128} },
		{1, 1, 1, { 620,  56}, {256, 128} },
		{2, 2, 1, { 659,  40}, {384, 128} },
		{1, 1, 2, { 845,  14}, {512, 128} },
		{2, 2, 2, { 429,  16}, {640, 128} },
		{1, 1, 3, {1133,  29}, {768, 128} },
		{2, 2, 3, { 139,  34}, {896, 128} },

		{2, 1, 4, {1120, 843}, {  0, 256} },
		{3, 2, 4, { 142, 858}, {128, 256} },
		{2, 1, 5, { 673,1047}, {256, 256} },
		{3, 2, 5, { 590,1041}, {384, 256} },
		{2, 1, 6, { 455,1079}, {512, 256} },
		{3, 2, 6, { 806,1079}, {640, 256} },
		{2, 1, 7, { 164,1068}, {768, 256} },
		{3, 2, 7, {1096,1067}, {896, 256} },

		{3, 1, 8, { 171, 206}, {  0, 384} },
		{4, 2, 8, {1086, 235}, {128, 384} },
		{3, 1, 9, { 571,  67}, {256, 384} },
		{4, 2, 9, { 703,  40}, {384, 384} },
		{3, 1,10, { 825,  14}, {512, 384} },
		{4, 2,10, { 444,  10}, {640, 384} },
		{3, 1,11, {1121,  24}, {768, 384} },
		{4, 2,11, { 148,  31}, {896, 384} },

		{4, 1,12, {1077, 889}, {  0, 512} },
		{1, 2,12, { 215, 873}, {128, 512} },
		{4, 1,13, { 663,1042}, {256, 512} },
		{1, 2,13, { 618,1040}, {384, 512} },
		{4, 1,14, { 422,1076}, {512, 512} },
		{1, 2,14, { 854,1074}, {640, 512} },
		{4, 1,15, { 105,1050}, {768, 512} },
		{1, 2,15, {1168,1052}, {896, 512} },

		{0, 1,16, { 481,  81}, {  0, 128} },
		{1, 2,16, {1424, 280}, {128, 128} },
		{0, 1,17, { 743,  17}, {256, 128} },
		{1, 2,17, {1415, 527}, {384, 128} },
		{0, 1,18, { 958,  86}, {512, 128} },
		{1, 2,18, {1422, 822}, {640, 128} },

		{0, 1,19, {1369, 261}, {  0, 128} },
		{2, 2,19, {  59, 295}, {128, 128} },
		{0, 1,20, {1401, 548}, {256, 128} },
		{2, 2,20, {  36, 569}, {384, 128} },
		{0, 1,21, {1358, 837}, {512, 128} },
		{2, 2,21, {  76, 845}, {640, 128} },

		{0, 1,22, { 934, 964}, {  0, 128} },
		{3, 2,22, {1406, 296}, {128, 128} },
		{0, 1,23, { 722,1057}, {256, 128} },
		{3, 2,23, {1369, 556}, {384, 128} },
		{0, 1,24, { 490, 964}, {512, 128} },
		{3, 2,24, {1411, 795}, {640, 128} },

		{0, 1,25, {  84, 832}, {  0, 128} },
		{4, 2,25, {  75, 230}, {128, 128} },
		{0, 1,26, {  42, 536}, {256, 128} },
		{4, 2,26, {  36, 514}, {384, 128} },
		{0, 1,27, {  78, 250}, {512, 128} },
		{4, 2,27, {  56, 790}, {640, 128} }

};

int Make_Plant_To_Global_Table_Flag = 1;
float Plant_To_Global_Phi[30][60];
float Plant_To_Global_Thita[30][60];

int ST_3DModel_Idx = 0;
int Angle_3DModel_Init = 1;
struct ST_Header_Struct 			ST_3DModel_Header[8];
struct US360_Stitching_Table 		ST_I_3DModel[8][2][Stitch_3DModel_Max];
struct US360_Stitching_Line_Table	SLT_3DModel[8][2][Stitch_3DModel_Max];
struct MP_B_Struct  				MP_B_3DModel[8][2][Stitch_3DModel_Max];
struct MP_B_Struct 	 				MP_B_2_3DModel[8][2][Stitch_3DModel_Max];
char   Transparent_B_3DModel[8][2][Stitch_3DModel_Max][4];						//[page][f_id][block][corner]

// 1152 x 1152
struct Out_Plant_Mode_Struct  Out_Mode_Plant_3dModel_Default = {
	{0x000000, 0x00000000, 0, 0, 18, 18 },
    0,   18000,   0,    9000
};

struct Out_Plant_Mode_Struct  Out_Mode_Plant_3dModel = {
	{0x000000, 0x00000000, 0, 0, 18, 18 },
    0,   18000,   0,    9000
};

// 3840 x 64
struct Out_Pillar_Mode_Struct  Out_Mode_Pillar_3dModel_Default = {
	{0x000000, 0x00000000, 1, 0, 60, 1 },
    18000, 54000, 0,
    0, 0, 0
};

struct Out_Pillar_Mode_Struct  Out_Mode_Pillar_3dModel = {
	{0x000000, 0x00000000, 1, 0, 60, 1 },
    18000, 54000, 0,
    0, 0, 0
};

void Set_Angle_3DModel_Init(int en) {
	Angle_3DModel_Init = en;
}

/*
 * idx: 0:pan  1:tilt  2:rotate  3:wide
 */
void SetPlantParameter(int idx, int value) {
	switch(idx) {
	case 0: Out_Mode_Plant_3dModel.Pan    = value; break;
	case 1: Out_Mode_Plant_3dModel.Tilt   = value; break;
	case 2: Out_Mode_Plant_3dModel.Rotate = value; break;
	case 3:
		Out_Mode_Plant_3dModel.Wide   = value;
		Make_Plant_To_Global_Table_Flag = 1;
		break;
	}
}

void GetPlantParameter(int *value) {
	*value     = Out_Mode_Plant_3dModel.Pan;
	*(value+1) = Out_Mode_Plant_3dModel.Tilt;
	*(value+2) = Out_Mode_Plant_3dModel.Rotate;
	*(value+3) = Out_Mode_Plant_3dModel.Wide;
}

//  0: Big 1: Small
//  Page
//  Sensor Idx
struct FPGA_DDR_Placement_Struct F_DDR_P =
{
   {
	 {{{ 0,0,0 },
	  { 0,0,1 },
	  { 0,0,2 }},
	 {{ 0,0,3 },
	  { 0,0,4 },
	  { 0,0,5 }},
	 {{ 0,3,0 },
	  { 0,3,1 },
	  { 0,3,2 }}},
	{{{ 0,0,7 },
	  { 0,1,7 },
	  { 0,2,7 }},
	 {{ 0,3,7 },
	  { 0,4,7 },
	  { 0,5,7 }}}
	  },
  {
	{{{ 0,6,0},
	  { 0,6,2},
	  { 0,6,4}},
	 {{ 0,9,0},
	  { 0,9,2},
	  { 0,9,4}}},
	{{{ 0,0,6},
	  { 0,1,6},
	  { 0,2,6}},
	 {{ 0,3,6},
	  { 0,4,6},
	  { 0,5,6}}}
	  },
  {
	  {{0,3,3},
	   {0,3,4},
	   {0,3,5}},
	  {{0,6,7},
	   {0,7,7},
	   {0,8,7}}
	  },

  {
	  {{1,   0,    0},
	   {1,5776,    0}},
	  {{1,   0,11648},
	   {1,1936,11648}},
	  },

  {
	  {{1,11552,0x0000000},
	   {1,11552,0x1000000}},

	  {{1,13600,0x000000},
	   {1,13600,0x200000}}
	  },

};

// Miller 20171130    Begin
void Calc_FPGA_DDR_P(struct FPGA_DDR_One_Struct *P)
{
	if (P->Mode == 0) {
		 P->DDR_P = ((P->Y * Sensor_Y_Step) << 15) +  ((P->X * Sensor_X_Step) << 1);
	}
	else {
		 P->DDR_P = ((P->Y ) << 15) + ((P->X ));
	};
}

void FPGA_DDR_P_Init(void)
{
  int i,j,k;
  for (i = 0; i < 2; i++) {
	  for (j = 0; j < 3; j++) {
		  for (k = 0; k < 3; k++) {
			   Calc_FPGA_DDR_P(&F_DDR_P.ISP1[i][j][k]);
		  }
	  }
  }

  for (i = 0; i < 2; i++) {
	  for (j = 0; j < 2; j++) {
		  for (k = 0; k < 3; k++) {
			   Calc_FPGA_DDR_P(&F_DDR_P.ISP2[i][j][k]);
		  }
	  }
  }

  for (i = 0; i < 2; i++) {
		  for (k = 0; k < 3; k++) {
			   Calc_FPGA_DDR_P(&F_DDR_P.NR3D[i][k]);
		  }
  }

  for (i = 0; i < 2; i++) {
		  for (k = 0; k < 2; k++) {
			   Calc_FPGA_DDR_P(&F_DDR_P.Stitch[i][k]);
		  }
  }

  for (i = 0; i < 2; i++) {
		  for (k = 0; k < 2; k++) {
			   Calc_FPGA_DDR_P(&F_DDR_P.Jpeg[i][k]);
		  }
  }

}

int Test_Global_degree;
struct Test_Block_Table_Struct Test_Block_Table[Test_Block_Table_MAX];

int Global_Sensor_X_Offset_1 = 9,   Global_Sensor_Y_Offset_1 = 8;
int Global_Sensor_X_Offset_2 = 18,  Global_Sensor_Y_Offset_2 = 7;
int Global_Sensor_X_Offset_3 = 2,   Global_Sensor_Y_Offset_3 = 1;
int Global_Sensor_X_Offset_4 = 8,   Global_Sensor_Y_Offset_4 = -6;
int Global_Sensor_X_Offset_6 = 0,   Global_Sensor_Y_Offset_6 = 0;

XY_Offset_Sturct Global_Senosr_Offset[6][2] = {
		{ {72,  10}, {  0,  4} },		//12K
		{ {52,   0}, {-24,  0} },		//8K
		{ {52,   0}, {-24,  0} },		//6K
		{ { 0,   4}, {  0,  4} },		//4K
		{ {52,   0}, {-24,  0} },		//3K
		{ { 0,   4}, {  0,  0} }		//2K
};

//int Global_Senosr_X_Offset_3 = -12, Global_Senosr_Y_Offset_3 = 20;

// Miller 20171031    Begin


int G_Degree = 0;	//-325.304/32;		//-RULE_UNIT(LensCode)/8;
void Set_G_Degree(int lens_code)
{
	G_Degree = 0;	//-RULE_UNIT[lens_code] / 32;
}

int SL_Adj_Gap0 = 2000;		//2000 = 20%
void SetSLAdjGap0(int value)
{
	SL_Adj_Gap0 = value;
}

int GetSLAdjGap0(void)
{
	return SL_Adj_Gap0;
}

int Get_Degree_Offset(int M_Mode, int CP_Mode)
{
	int Degree, Degree_Offset = 0;

	if(CP_Mode == 1) {
		Degree = A_L_I3_Header[M_Mode].H_Degree_offset;
		if(Degree != 0) {
			if(M_Mode == 0) Degree_Offset = 3;		//12K, 3 Block = 6度 / (360度 / 180 Block)
			else			Degree_Offset = 1;		//4K,  1 Block = 6度 / (360度 / 60 Block)
		}
		else				Degree_Offset = 0;
	}
	else					Degree_Offset = 0;

	return Degree_Offset;
}

void Get_thita_rate3(int M_Mode,int S_Id, short I_phi, unsigned short I_thita , struct MP_B_Sub_Struct *MP_BS_p, struct MP_B_Sub_Struct *MP_B2S_p, int Block_Mode, int Sub_Idx, int y, int x)
{
	int i;
	unsigned short S_rate_Sub, I_S_rate_Sub;
	int S_t_p0A, S_t_p0B, S_t_p0C;
	int S_t_p64A, S_t_p64B;
	int I3_idx0, I3_idx1, I3_idx2;
	int phi0_2, phi_rateA, phi_rateB, phi_rateC, YUV_rateA, YUV_rateB, YUV_rateC;
	struct MP_B_Sub_Struct *Sub_pA,*Sub_pB,*Sub_pC,*Sub_pD;
	int T_X, T_Y;
	int T_X0, T_Y0;
	int T_X64, T_Y64;
	int D_X, D_Y;
	short V_Sin, V_Cos;
	unsigned short thita0;  short phi0;
	unsigned short thita64; short phi64;
	float Scale_Z;
	int Binn, Degree_Offset;
	int Global_degree;
	int SL_Adj_Gap1, Gap_rate;
	int p1, p2;
	int s_id0, s_id1;
	short c_phi;
	unsigned short S_rate_c_Sub;
	int Size_H, Size_V;
    int cp_mode = getCameraPositionMode();

// 	Z = 64
	if (Block_Mode != 2) Global_degree = RULE_UNIT[LensCode];
	else {
		if (Sub_Idx < 2) Global_degree = G_Degree;		//0.0;
		else             Global_degree = RULE_UNIT[LensCode];
	}

	Global2Sensor(M_Mode, S_Id, I_phi, I_thita, Global_degree);
	thita64 = PP_Temp.S_t_p[S_Id].thita;  phi64  = PP_Temp.S_t_p[S_Id].phi;

	S_t_p64A = A_L_S3[M_Mode][S_Id].S_rate_Idx[( (thita64 >> 4) & 0xfff)];
	if(S_t_p64A > (A_L_S3[M_Mode][S_Id].Sum-1) ) S_t_p64A = (A_L_S3[M_Mode][S_Id].Sum-1);
	S_rate_Sub = A_L_S3[M_Mode][S_Id].S_rate_Sub[( (thita64 >> 4) & 0xfff)] >> 2;

	T_X64 = PP_Temp.S[S_Id].X;
	T_Y64 = PP_Temp.S[S_Id].Y;
	if(Block_Mode == 0 && (S_Id == 2 || S_Id == 4) ) {			//縫合畫面才需要做限制
		//if(T_X64 < 0) T_X64 = 0;
		if(T_X64 > Sensor_Size_X) T_X64 = Sensor_Size_X;
		//if(T_Y64 < 0) T_Y64 = 0;
		//if(T_Y64 > Sensor_Size_Y) T_Y64 = Sensor_Size_Y;
	}

// 	Z = 0
	if (Block_Mode != 2) Global_degree = G_Degree;		//0.0;
	else {
		if (Sub_Idx < 2) Global_degree = G_Degree;		//0.0;
		else             Global_degree = RULE_UNIT[LensCode];
	}

	Global2Sensor(M_Mode, S_Id, I_phi, I_thita, Global_degree);
	thita0 = PP_Temp.S_t_p[S_Id].thita;  phi0   = PP_Temp.S_t_p[S_Id].phi;

	T_X0 = PP_Temp.S[S_Id].X;
	T_Y0 = PP_Temp.S[S_Id].Y;

	D_X = T_X64 - T_X0;
	D_Y = T_Y64 - T_Y0;

	Binn = A_L_I3_Header[M_Mode].Binn;

	Size_H = A_L_I3_Header[M_Mode].H_Blocks;
	Size_V = A_L_I3_Header[M_Mode].V_Blocks;

	if( (y < A_L_I3_Header[M_Mode].Phi_P[2] && cp_mode == 0) ||
		(y >= (Size_V - A_L_I3_Header[M_Mode].Phi_P[2]) && cp_mode == 1) || y == -1) {		//上部
		S_t_p0A = A_L_S3[M_Mode][S_Id].S_rate_Idx[( (thita0 >> 4) & 0xfff)];
		if(S_t_p0A > (A_L_S3[M_Mode][S_Id].Sum-1) ) S_t_p0A = (A_L_S3[M_Mode][S_Id].Sum-1);
		S_rate_Sub = A_L_S3[M_Mode][S_Id].S_rate_Sub[( (thita0 >> 4) & 0xfff)] >> 2;

		if (S_t_p0A < 1)  S_t_p0B = A_L_S3[M_Mode][S_Id].Sum - 1;
		else              S_t_p0B = S_t_p0A - 1;

		S_t_p0C = A_L_I3_Header[M_Mode].Sum + S_Id;

		I3_idx0 = A_L_S3[M_Mode][S_Id].Source_Idx[S_t_p0A];
		I3_idx1 = A_L_S3[M_Mode][S_Id].Source_Idx[S_t_p0B];
  		I3_idx2 = S_t_p0C;	//A_L_S3[M_Mode][S_Id].Source_Idx[S_t_p0C];
  		s_id0 = S_Id;
  		s_id1 = S_Id;
  		I_S_rate_Sub = 0x4000 - S_rate_Sub;
  		phi0_2 = (A_L_S3[M_Mode][S_Id].S_t_p[S_t_p0A].phi * I_S_rate_Sub + S_rate_Sub * A_L_S3[M_Mode][S_Id].S_t_p[S_t_p0B].phi ) >> 14;

  		SL_Adj_Gap1 = 10000 * (phi0_2 - phi0) / phi0_2;
  		if (SL_Adj_Gap1 >= 0) {
  			if (SL_Adj_Gap1 > SL_Adj_Gap0) {
  				phi_rateA =  0;
  				phi_rateB =  0;
  			}
  			else {
  				Gap_rate = 10000 * (SL_Adj_Gap0 - SL_Adj_Gap1) / SL_Adj_Gap0;
  				phi_rateA =  I_S_rate_Sub * Gap_rate / 10000;
  				phi_rateB =  S_rate_Sub   * Gap_rate / 10000;
  			}
  			phi_rateC =  0x4000 - phi_rateA - phi_rateB;
  		}
  		else {
  			Gap_rate = 10000; // * phi0 / phi0_2;
  			phi_rateA =  I_S_rate_Sub * Gap_rate / 10000;
  			phi_rateB =  S_rate_Sub   * Gap_rate / 10000;
  			phi_rateC =  0;
  		}

  		YUV_rateA =  I_S_rate_Sub * phi0 / phi0_2;
  		YUV_rateB =  S_rate_Sub   * phi0 / phi0_2;
  		YUV_rateC =  0;
	}
	else {		//底部
		/* 			|		|
		 *			|		|
		 * 		____A		B____
		 *			   P
		 *		________C________
		 */

		//依據x判斷參考哪2個點
		Degree_Offset = Get_Degree_Offset(M_Mode, cp_mode);
		if(cp_mode == 0) {
			if(x >= (A_L_I3_Header[M_Mode].Thita_P[3]-Degree_Offset) )	    p1 = 1;
			else if(x >= (A_L_I3_Header[M_Mode].Thita_P[2]-Degree_Offset) ) p1 = 0;
			else if(x >= (A_L_I3_Header[M_Mode].Thita_P[1]-Degree_Offset) ) p1 = 3;
			else if(x >= (A_L_I3_Header[M_Mode].Thita_P[0]-Degree_Offset) ) p1 = 2;
			else										   					p1 = 1;
			p2 = (p1 + 1) & 0x3;
		}
		else {
			if(x >= (A_L_I3_Header[M_Mode].Thita_P[3]-Degree_Offset) )	    p1 = 1;
			else if(x >= (A_L_I3_Header[M_Mode].Thita_P[2]-Degree_Offset) ) p1 = 2;
			else if(x >= (A_L_I3_Header[M_Mode].Thita_P[1]-Degree_Offset) ) p1 = 3;
			else if(x >= (A_L_I3_Header[M_Mode].Thita_P[0]-Degree_Offset) ) p1 = 0;
			else										        		    p1 = 1;
			p2 = (p1 + 1) & 0x3;
		}

		I3_idx0 = A_L_I3_Line[p1].SL_Point[M_Mode] + A_L_I3_Line[p1].SL_Sum[M_Mode] - 1;
		I3_idx1 = A_L_I3_Line[p2].SL_Point[M_Mode] + A_L_I3_Line[p2].SL_Sum[M_Mode] - 1;
		I3_idx2 = A_L_I3_Header[M_Mode].Sum + 5;		//特殊點, Smooth取4點平均
		s_id0 = A_L_I3[M_Mode][I3_idx0].p[1].Sensor_Idx;
		s_id1 = A_L_I3[M_Mode][I3_idx1].p[1].Sensor_Idx;
		S_t_p0A = A_L_I3[M_Mode][I3_idx0].p[1].A_L_S_Idx;
		S_t_p0B = A_L_I3[M_Mode][I3_idx1].p[1].A_L_S_Idx;
		if(A_L_I3[M_Mode][I3_idx0].G_t_p.thita > A_L_I3[M_Mode][I3_idx1].G_t_p.thita) {		//計算 參考AB比例
			if(A_L_I3[M_Mode][I3_idx0].G_t_p.thita > I_thita)
				S_rate_Sub = (I_thita + 0x10000 - A_L_I3[M_Mode][I3_idx0].G_t_p.thita) * 0x4000 / (A_L_I3[M_Mode][I3_idx1].G_t_p.thita + 0x10000 - A_L_I3[M_Mode][I3_idx0].G_t_p.thita);
			else
				S_rate_Sub = (I_thita - A_L_I3[M_Mode][I3_idx0].G_t_p.thita) * 0x4000 / (A_L_I3[M_Mode][I3_idx1].G_t_p.thita + 0x10000 - A_L_I3[M_Mode][I3_idx0].G_t_p.thita);
		}
		else {
			if(A_L_I3[M_Mode][I3_idx0].G_t_p.thita > I_thita)
				S_rate_Sub = (I_thita + 0x10000 - A_L_I3[M_Mode][I3_idx0].G_t_p.thita) * 0x4000 / (A_L_I3[M_Mode][I3_idx1].G_t_p.thita - A_L_I3[M_Mode][I3_idx0].G_t_p.thita);
			else
				S_rate_Sub = (I_thita - A_L_I3[M_Mode][I3_idx0].G_t_p.thita) * 0x4000 / (A_L_I3[M_Mode][I3_idx1].G_t_p.thita - A_L_I3[M_Mode][I3_idx0].G_t_p.thita);
		}
		I_S_rate_Sub = 0x4000 - S_rate_Sub;
		phi0_2 = (A_L_S3[M_Mode][s_id0].S_t_p[S_t_p0A].phi * I_S_rate_Sub + S_rate_Sub * A_L_S3[M_Mode][s_id1].S_t_p[S_t_p0B].phi ) >> 14;

		//計算參考C比例
		if(cp_mode == 0) {
			c_phi = Map_Posi_Tmp[Size_V][0].I_phi;
			S_rate_c_Sub   = (c_phi - I_phi) * 0x4000 / (c_phi - Map_Posi_Tmp[ A_L_I3_Header[M_Mode].Phi_P[2] ][0].I_phi);
		}
		else {
			c_phi = Map_Posi_Tmp[0][0].I_phi;
			S_rate_c_Sub   = (c_phi - I_phi) * 0x4000 / (c_phi - Map_Posi_Tmp[ Size_V - A_L_I3_Header[M_Mode].Phi_P[2] ][0].I_phi);
		}

		phi_rateA =  I_S_rate_Sub * S_rate_c_Sub / 0x4000;
		phi_rateB =  S_rate_Sub   * S_rate_c_Sub / 0x4000;
		phi_rateC =  0x4000 - phi_rateA - phi_rateB;

		YUV_rateA =  phi_rateA * phi0 / phi0_2;
		YUV_rateB =  phi_rateB * phi0 / phi0_2;
		YUV_rateC =  phi_rateC * phi0 / phi0_2;
	}

	Sub_pA =  MP_BS_p;
	Sub_pB =  MP_BS_p + 1;
	Sub_pC =  MP_B2S_p;
	Sub_pD =  MP_B2S_p + 1;

	Sub_pA->Rev   = 0;
	Sub_pA->P     = I3_idx0;
	Sub_pA->F_YUV = 0;
	Sub_pA->V_YUV = YUV_rateA;
	T_X = phi_rateA * D_X / 64 / Binn;
	T_Y = phi_rateA * D_Y / 64 / Binn;
	if (T_X >=0) Sub_pA->F_X = 0; else		 Sub_pA->F_X = 1;
	Sub_pA->V_X   = abs(T_X);
	if (T_Y >=0) Sub_pA->F_Y = 0; else		 Sub_pA->F_Y = 1;
	Sub_pA->V_Y   = abs(T_Y);

	Sub_pB->Rev   = 0;
	Sub_pB->P     = I3_idx1;
	Sub_pB->F_YUV = 0;
	Sub_pB->V_YUV = YUV_rateB;
	T_X = phi_rateB * D_X /64 / Binn;
	T_Y = phi_rateB * D_Y /64 / Binn;
	if (T_X >=0) Sub_pB->F_X = 0; else		 Sub_pB->F_X = 1;
	Sub_pB->V_X   = abs(T_X);
	if (T_Y >=0) Sub_pB->F_Y = 0; else		 Sub_pB->F_Y = 1;
	Sub_pB->V_Y   = abs(T_Y);

	Sub_pC->Rev   = 0;
	Sub_pC->P     = I3_idx2;
	Sub_pC->F_YUV = 0;
	Sub_pC->V_YUV = YUV_rateC;
	T_X = phi_rateC * D_X /64 / Binn;
	T_Y = phi_rateC * D_Y /64 / Binn;
	if (T_X >=0) Sub_pC->F_X = 0; else		 Sub_pC->F_X = 1;
	Sub_pC->V_X   = abs(T_X);
	if (T_Y >=0) Sub_pC->F_Y = 0; else		 Sub_pC->F_Y = 1;
	Sub_pC->V_Y   = abs(T_Y);

	Sub_pD->Rev   = 0;
	Sub_pD->P     = I3_idx2;
	Sub_pD->F_YUV = 0;
	Sub_pD->V_YUV = 0;
	Sub_pD->V_X   = 0;
	Sub_pD->V_Y   = 0;

	if (A_L_S3[M_Mode][s_id0].Top_Bottom[S_t_p0A] == 0) Sub_pA->F_YUV = 1;
	if (A_L_S3[M_Mode][s_id1].Top_Bottom[S_t_p0B] == 0) Sub_pB->F_YUV = 1;

	switch (Block_Mode) {
      	  case 0: break;
      	  case 1: Sub_pA->V_YUV = 0; Sub_pB->V_YUV = 0; Sub_pC->V_YUV = 0; break;
      	  case 2: Sub_pA->V_X = 0;Sub_pA->V_Y = 0;
	          	  Sub_pB->V_X = 0;Sub_pB->V_Y = 0;
	          	  Sub_pC->V_X = 0;Sub_pC->V_Y = 0;
	          	  break;
	};
};

void Set_CB_Proc(struct US360_Stitching_Table *ST_P)
{
    ST_P->CB_Sensor_ID = 0;
    ST_P->CB_Alpha_Flag = 0;
    ST_P->CB_Mask   = 0;
    ST_P->CB_Adj_Y0 = 0x80;
    ST_P->CB_Adj_Y1 = 0x80;
    ST_P->CB_Adj_Y2 = 0x80;
    ST_P->CB_Adj_Y3 = 0x80;
    ST_P->CB_Adj_U0 = 0x80;
    ST_P->CB_Adj_U1 = 0x80;
    ST_P->CB_Adj_U2 = 0x80;
    ST_P->CB_Adj_U3 = 0x80;
    ST_P->CB_Adj_V0 = 0x80;
    ST_P->CB_Adj_V1 = 0x80;
    ST_P->CB_Adj_V2 = 0x80;
    ST_P->CB_Adj_V3 = 0x80;
};

void Get_FId_MT(int s_id, int *f_id, int *mt)
{
	switch (s_id) {
	  case 0: *f_id = 1; *mt = 0; break;
	  case 1: *f_id = 1; *mt = 1; break;
	  case 2: *f_id = 0; *mt = 0; break;
	  case 3: *f_id = 1; *mt = 2; break;
	  case 4: *f_id = 0; *mt = 1; break;
	}
}

void Make_Stitch_Line_Block_One(int M_Mode,int S_Id, struct US360_Stitching_Table *ST_P, struct A_L_I_Posi *S_p, int Target_P, int Binn, int doTrans)
{
	int FPGA_ID, MT, B_Mode;
	int offsetX, offsetY;
	offsetX = 512;
	offsetY = 512;

    Get_FId_MT(S_Id, &FPGA_ID, &MT);

    if(M_Mode > 2) B_Mode = 3;
    else           B_Mode = 1;

    Set_CB_Proc(ST_P);
    ST_P->CB_Block_ID = 0;
    ST_P->CB_DDR_P   = Target_P;
    ST_P->CB_Sensor_ID = (MT+1);		//ST Mask 1~3
    if(doTrans == 1) ST_P->CB_Alpha_Flag = 1;
    else             ST_P->CB_Alpha_Flag = 0;
    ST_P->CB_Posi_X0 = S_p->XY[0].X * 4 /Binn  + MT * Sensor_X_Step * 4/B_Mode  + offsetX;
    ST_P->CB_Posi_Y0 = S_p->XY[0].Y * 4 /Binn  + offsetY;
    ST_P->CB_Posi_X1 = S_p->XY[1].X * 4 /Binn  + MT * Sensor_X_Step * 4/B_Mode  + offsetX;
    ST_P->CB_Posi_Y1 = S_p->XY[1].Y * 4 /Binn  + offsetY;
    ST_P->CB_Posi_X2 = S_p->XY[2].X * 4 /Binn  + MT * Sensor_X_Step * 4/B_Mode  + offsetX;
    ST_P->CB_Posi_Y2 = S_p->XY[2].Y * 4 /Binn  + offsetY;
    ST_P->CB_Posi_X3 = S_p->XY[3].X * 4 /Binn  + MT * Sensor_X_Step * 4/B_Mode  + offsetX;
    ST_P->CB_Posi_Y3 = S_p->XY[3].Y * 4 /Binn  + offsetY;
}

void Make_Empty_Stitch_Command(struct US360_Stitching_Table *st_p, struct US360_Stitching_Line_Table *slt_p, int T_Addr)
{
	Set_CB_Proc(st_p);
	st_p->CB_Block_ID = 0;
	st_p->CB_DDR_P = (T_Addr >> 5);		//空的位址
	st_p->CB_Posi_X0 = 0;
	st_p->CB_Posi_Y0 = 0;	//(128 << 2);
	st_p->CB_Posi_X1 = 0;
	st_p->CB_Posi_Y1 = 0;	//(128 << 2);
	st_p->CB_Posi_X2 = 0;
	st_p->CB_Posi_Y2 = 0;	//(128 << 2);
	st_p->CB_Posi_X3 = 0;
	st_p->CB_Posi_Y3 = 0;	//(128 << 2);

	slt_p->CB_Mode  = 0;
	slt_p->CB_Idx   = 0;
	slt_p->X_Posi   = 0;
	slt_p->Y_Posi   = 0;
}

void Make_Stitch_Line_Command(int M_Mode)
{
   struct Adjust_Line_I3_Struct *I3_p;
   struct Adjust_Line_I3_Line_Struct   *L_p;
   struct Adjust_Line_I3_Header_Struct *H_p;
   unsigned short thita1;
   short phi1;
   int i, j, k, L,M;
   int  T_Size_X, T_Size_Y, Binn;
   int I_Start; int I_Stop;
   int Block_size2, Block_size3, Block_size_VH, Block_size_H, Block_size_V, Block_size_D;
   struct thita_phi_Struct Temp_t_p;
   int S_Id;
   struct MP_B_Sub_Struct *MP_BS_P, *MP_B2S_P;
   int MT, FPGA_ID;
   int B_Idx[2];
   int Start_Idx;
   int Posi_Y, Posi_X;
   int Target_P;
   int Line_No;
   int Line_Mode2;

   H_p=&A_L_I3_Header[M_Mode];
   Binn     = A_L_I3_Header[M_Mode].Binn;
   Block_size2 = 0x10000 /  A_L_I3_Header[M_Mode].H_Blocks / 2;
   Block_size3 = 0x10000 /  A_L_I3_Header[M_Mode].H_Blocks / 2;

   Start_Idx = ST_Header[M_Mode].Start_Idx[ST_H_YUV];
   B_Idx[0] =0; B_Idx[1] =0;
   for(j =  0; j < H_p->Sum; j++) {
       I3_p = &A_L_I3[M_Mode][j];
       for (k = 0; k < 2; k++) {		//Top / Btm Sensor
	    S_Id =  I3_p->p[k].Sensor_Idx;
	    Get_FId_MT(S_Id, &FPGA_ID, &MT);

	    // YUV A_L_I3 Line
	    MP_B_P = &MP_B[FPGA_ID][Start_Idx + B_Idx[FPGA_ID]] ;
		MP_B2_P = &MP_B_2[FPGA_ID][Start_Idx + B_Idx[FPGA_ID]] ;
	    for (L = 0; L < 4; L++) {
			switch (L) {
			  case 0: Block_size_H = Block_size2 * -1; Block_size_V = Block_size2 * -1; break;
			  case 1: Block_size_H = Block_size2 *  1; Block_size_V = Block_size2 * -1; break;
			  case 2: Block_size_H = Block_size2 * -1; Block_size_V = Block_size2 *  1; break;
			  case 3: Block_size_H = Block_size2 *  1; Block_size_V = Block_size2 *  1; break;
			}
			switch (L){
			   case 0: MP_BS_P = &MP_B_P->MP_A0;  MP_B2S_P = &MP_B2_P->MP_A0; break;
			   case 1: MP_BS_P = &MP_B_P->MP_A1;  MP_B2S_P = &MP_B2_P->MP_A1; break;
			   case 2: MP_BS_P = &MP_B_P->MP_A2;  MP_B2S_P = &MP_B2_P->MP_A2; break;
			   case 3: MP_BS_P = &MP_B_P->MP_A3;  MP_B2S_P = &MP_B2_P->MP_A3; break;
			};
			Temp_t_p.phi   = (I3_p->G_t_p.phi   + Block_size_V) & 0xffff;
			Temp_t_p.thita = (I3_p->G_t_p.thita + Block_size_H) & 0xffff;
			I3_p->p[k].YUV.G_t_p[L]   = Temp_t_p;
			Get_thita_rate3(M_Mode, S_Id, Temp_t_p.phi, Temp_t_p.thita , MP_BS_P, MP_B2S_P, 1, L, -1, -1);
			I3_p->p[k].YUV.S_t_p[L] = PP_Temp.S_t_p[S_Id];
			I3_p->p[k].YUV.XY[L]    = PP_Temp.S[S_Id];
	    }

	    ST_P   = &ST_I[FPGA_ID][Start_Idx + B_Idx[FPGA_ID]] ;
	    SLT_P  = &SLT[FPGA_ID][Start_Idx + B_Idx[FPGA_ID]] ;

	    Line_No = I3_p->Line_No;
// Miller 20180919
	    if (Line_No <= 3) {
	      Posi_X = (Line_No << 1)+ k;
	      Posi_Y = 0;
	    }
	    else {
	      Posi_X = (Line_No << 1) + k;
	      Posi_Y = 0;
	    }

	    Posi_Y += I3_p->Line_Idx;

		if(M_Mode == 3 || M_Mode == 4 || M_Mode == 5) {
			Target_P   = ( ( ( (Posi_Y << 20) + (Posi_X << 6) ) << 1) >> 5) + (ST_YUV_LINE_STM2_P0_T_ADDR >> 5);
		}
		else {
			Target_P   = ( ( ( (Posi_Y << 20) + (Posi_X << 6) ) << 1) >> 5) + (ST_YUV_LINE_STM1_P0_T_ADDR >> 5);
		}

	    I3_p->p[k].YUV.Target_P =  Target_P;

	    Make_Stitch_Line_Block_One(M_Mode,S_Id, ST_P, &I3_p->p[k].YUV, Target_P, Binn, 0);

        SLT_P->CB_Mode  = 1;
        SLT_P->CB_Idx   = j;
        SLT_P->X_Posi   = k;
        SLT_P->Y_Posi   = 0;

	    B_Idx[FPGA_ID]++;
       }
   };

   //YUV ST CMD 對齊8
   int sum;
   for(j = 0; j < 2; j++) {
		sum = (B_Idx[j] & 0x7);
		if(sum != 0) {
			for(i = 0; i < (8-sum); i++) {
			    ST_P   = &ST_I[j][Start_Idx + B_Idx[j] ] ;
			    SLT_P  = &SLT[j][Start_Idx + B_Idx[j] ] ;
			    Target_P = ST_EMPTY_T_ADDR;
			    Make_Empty_Stitch_Command(ST_P, SLT_P, Target_P);
				B_Idx[j]++;
			}
		}
   }
   ST_Header[M_Mode].Sum[ST_H_YUV][0] = B_Idx[0];
   ST_Header[M_Mode].Sum[ST_H_YUV][1] = B_Idx[1];

   // Z A_L_I3 Line
   for (M = 0; M < 2; M++) {	//ZV / ZH
	   Start_Idx = ST_Header[M_Mode].Start_Idx[ST_H_ZV+M];
      B_Idx[0] =0; B_Idx[1] =0;
      for (j =  0; j < H_p->Sum; j++) {
   	   I3_p = &A_L_I3[M_Mode][j];
   	   for (k = 0; k < 2; k++) {
   		   S_Id =  I3_p->p[k].Sensor_Idx;
   		   Get_FId_MT(S_Id, &FPGA_ID, &MT);

   			   for (L = 0; L < 4; L++) {
     			 switch (L) {
       				 case 0:Block_size_D = Block_size2 * -1; Block_size_H = Block_size3 * -1; break;  // ((Block_size2*5)>>2) .. *5/4 縫合比對距離增加1.25倍
       				 case 1:Block_size_D = Block_size2 *  1; Block_size_H = Block_size3 * -1; break;
       				 case 2:Block_size_D = Block_size2 * -1; Block_size_H = Block_size3 *  1; break;
       				 case 3:Block_size_D = Block_size2 *  1; Block_size_H = Block_size3 *  1; break;
         		 }

   		   		 Line_Mode2 = (I3_p->Line_Mode + M) & 1;
   		   		 if (M == 0) {
    		   		 Temp_t_p.phi   = (I3_p->G_t_p.phi   + Block_size_D) & 0xffff;
    		   		 Temp_t_p.thita = (I3_p->G_t_p.thita + Block_size_D) & 0xffff;
   		   		 }
   		   		 else {
   		   			 Temp_t_p.phi   = (I3_p->G_t_p.phi   + Block_size_D) & 0xffff;
   		   			 Temp_t_p.thita = (I3_p->G_t_p.thita - Block_size_D) & 0xffff;
   		   		 }

   		       	 switch (L){
				   case 0: MP_BS_P = &MP_B_P->MP_A0; MP_B2S_P = &MP_B2_P->MP_A0; break;
				   case 1: MP_BS_P = &MP_B_P->MP_A1; MP_B2S_P = &MP_B2_P->MP_A1; break;
				   case 2: MP_BS_P = &MP_B_P->MP_A2; MP_B2S_P = &MP_B2_P->MP_A2; break;
				   case 3: MP_BS_P = &MP_B_P->MP_A3; MP_B2S_P = &MP_B2_P->MP_A3; break;
   		       	 };
   		       	 I3_p->p[k].Z.G_t_p[L]   = Temp_t_p;
				   Get_thita_rate3(M_Mode, S_Id, Temp_t_p.phi, Temp_t_p.thita , MP_BS_P, MP_B2S_P, 2, L, -1, -1);
   		       	 I3_p->p[k].Z.S_t_p[L] = PP_Temp.S_t_p[S_Id];
   		       	 I3_p->p[k].Z.XY[L]    = PP_Temp.S[S_Id];
   		       }

   		       ST_P   = &ST_I[FPGA_ID][Start_Idx + B_Idx[FPGA_ID]] ;
   		       SLT_P  = &SLT[FPGA_ID][Start_Idx + B_Idx[FPGA_ID]] ;

   		       Line_No = I3_p->Line_No;
// Miller 20180919
   		       if (Line_No <= 3) {
   		    	   Posi_X = (Line_No << 1) + k + M * 16;	//8;
				   Posi_Y = 0;
   			   }
   			   else {
   				   Posi_X = (Line_No << 1) + k + M * 16;	//8;
   				   Posi_Y = 0;
   			   }


   			   Posi_Y += I3_p->Line_Idx;

   			   if(M_Mode == 3 || M_Mode == 4 || M_Mode == 5)
				  Target_P   = ( ( ( (Posi_Y << 20) + (Posi_X << 6) ) << 1) >> 5) + (ST_Z_V_LINE_STM2_P0_T_ADDR >> 5);
			   else
				  Target_P   = ( ( ( (Posi_Y << 20) + (Posi_X << 6) ) << 1) >> 5) + (ST_Z_V_LINE_STM1_P0_T_ADDR >> 5);

   			   I3_p->p[k].Z.Target_P =  Target_P;

   			   Make_Stitch_Line_Block_One(M_Mode,S_Id, ST_P, &I3_p->p[k].Z, Target_P, Binn, 0);

   			   SLT_P->CB_Mode  = 0;
   			   SLT_P->CB_Idx   = j;
   			   SLT_P->X_Posi   = k;
   			   SLT_P->Y_Posi   = 0;

   			   B_Idx[FPGA_ID]++;

   	   }
      };
      ST_Header[M_Mode].Sum[ST_H_ZV+M][0] = B_Idx[0];
      ST_Header[M_Mode].Sum[ST_H_ZV+M][1] = B_Idx[1];
   };

}

void Make_thita_rate3(int M_Mode)
{
  int i,j,k;
  int Sum;
  unsigned short Temp_P0, Temp_P1, Temp_P2, Distance;
  for (k = 0; k < 5; k++) {
	Sum = A_L_S3[M_Mode][k].Sum;
	for(i=0;i<Sum; i++) {
	  Temp_P0 = A_L_S3[M_Mode][k].S_t_p[i].thita / 16;
	  if (i == 0) {
		Temp_P1 = A_L_S3[M_Mode][k].S_t_p[Sum-1].thita / 16;
	  }
	  else  {
		Temp_P1 = A_L_S3[M_Mode][k].S_t_p[i-1].thita / 16;
	  }

	  Distance = ((Temp_P1 - Temp_P0)) & 0x0fff;
	  for(j = 0; j < Distance; j++) {
		A_L_S3[M_Mode][k].S_rate_Idx[(Temp_P0 + j) & 0x0fff] = i;
		A_L_S3[M_Mode][k].S_rate_Sub[(Temp_P0 + j) & 0x0fff] = j *0x10000 / Distance;
      }
    }
  }
}


int  Make_A_L_S3_One_Line(int M_Mode, int S_Id, int S3_Start, int L_Idx, struct Adjust_Line_S3_Struct *S3_p , int dir)
{
   int i;
   struct Adjust_Line_I3_Struct *I3_p;
   struct Adjust_Line_I3_Line_Struct   *L_p;
   struct Adjust_Line_I3_Header_Struct *H_p;
   int I_Start; int I_Stop;
   int S3_Idx;
   int Sub_Idx;
   int T_B;

   S3_Idx = S3_Start;
   H_p = &A_L_I3_Header[M_Mode];
   L_p = &A_L_I3_Line[L_Idx];
   I_Start = L_p->SL_Point[M_Mode];
   I_Stop  = L_p->SL_Point[M_Mode] + L_p->SL_Sum[M_Mode];
   if (L_p->SL_Sensor_Id[0] == S_Id) T_B = 0;
   else                              T_B = 1;
   if (dir == 0) {
	 Sub_Idx = 0;
	 for (i = I_Start; i < I_Stop; i++) {
	   I3_p = &A_L_I3[M_Mode][i];
	   I3_p->Line_Mode = L_p->Line_Mode;
	   I3_p->Line_No   = L_Idx;
	   I3_p->Line_Idx  = Sub_Idx;
	   S3_p->Source_Idx[S3_Idx]  = i;
	   S3_p->Top_Bottom[S3_Idx]  = T_B;
	   S3_Idx++; Sub_Idx++;
	 }
   }
   else {
	 Sub_Idx = I_Stop - I_Start - 1;
	 for (i = I_Stop - 1; i >= I_Start; i--) {
	   I3_p = &A_L_I3[M_Mode][i];
	   I3_p->Line_Mode = L_p->Line_Mode;
	   I3_p->Line_No   = L_Idx;
	   I3_p->Line_Idx  = Sub_Idx;
	   S3_p->Source_Idx[S3_Idx]  = i;
	   S3_p->Top_Bottom[S3_Idx]  = T_B;
	   S3_Idx++; Sub_Idx--;
	 }
   }
   return(S3_Idx);
};

void Make_A_L_I3_Table( void)
{
  int Resolution_Mode = 0;
  int i,j,k,L;
  struct Adjust_Line_I3_Header_Struct *H_p;
  struct Adjust_Line_I3_Struct        *p2;
  int H_Blocks;
  int Sum;
  int Point; int Phi_Idx;
// Make A_L_I3
  for (i = 0; i < 6; i++) {
	  S_Rotate_R_Init(i);

	H_p  = &A_L_I3_Header[i];
	H_Blocks      = H_p->Resolution / 64;
	H_p->H_Blocks = H_Blocks;
	H_p->V_Blocks = H_Blocks / 2;
	H_p->Target_P = 0x00000000;
	H_p->Phi_P[0] = (H_Blocks * Stitch_Line_phi0 + 180) / 360;
	H_p->Phi_P[1] = (H_Blocks * Stitch_Line_phi1 + 180) / 360;
	H_p->Phi_P[2] = (H_Blocks * Stitch_Line_phi2 + 180) / 360;
	H_p->Thita_P[0] = (H_Blocks * (Stitch_Line_thita0 + H_p->H_Degree_offset) + 180) / 360;
	H_p->Thita_P[1] = (H_Blocks * (Stitch_Line_thita1 + H_p->H_Degree_offset) + 180) / 360;
	H_p->Thita_P[2] = (H_Blocks * (Stitch_Line_thita2 + H_p->H_Degree_offset) + 180) / 360;
	H_p->Thita_P[3] = (H_Blocks * (Stitch_Line_thita3 + H_p->H_Degree_offset) + 180) / 360;

// Miller 20180919
	Sum = 0;
	for (j = 0; j < 4; j++) {
//
//		A_L_I3_Line[j].SL_Point[i] = Sum;
//		Point = 0;
//		for (k = H_p->Phi_P[0]; k < H_p->Phi_P[1]; k++) {
//		   p2 = &A_L_I3[i][Sum];
//		   p2->G_t_p.phi   = 0x4000 - 0x10000 * k / H_p->H_Blocks;
//		   p2->G_t_p.thita = 0x10000 * H_p->Thita_P[j] / H_p->H_Blocks;
//		   for (L = 0; L < 2; L++) p2->p[L].Sensor_Idx  = A_L_I3_Line[j].SL_Sensor_Id[L];
//		   Sum++;
//		   Point++;
//		};
//		A_L_I3_Line[j].SL_Sum[i] = Point;
//
		A_L_I3_Line[j].SL_Point[i] = Sum;
		Point = 0;
		for (k = H_p->Phi_P[1] + 1; k <= H_p->Phi_P[2]; k++) {
		   p2 = &A_L_I3[i][Sum];
		   p2->G_t_p.phi   = 0x4000 - 0x10000 * k / H_p->H_Blocks;
		   p2->G_t_p.thita = 0x10000 * H_p->Thita_P[j] / H_p->H_Blocks;
		   for (L = 0; L < 2; L++) p2->p[L].Sensor_Idx  = A_L_I3_Line[j].SL_Sensor_Id[L];
		   Sum++;
		   Point++;
		};
		A_L_I3_Line[j].SL_Sum[i] = Point;
	}

	int Phi_Idx, Delta_Phi,ML,MR;
	int Real_Phi_degree, Real_Phi_P;

	Delta_Phi = A_L_I3_Header[i].Phi_P[1] - A_L_I3_Header[i].Phi_P[0];
	for (j = 0; j < 3; j++) {
		A_L_I3_Line[j+4].SL_Point[i] = Sum;
		Point = 0;

		ML = Delta_Phi - 1;
		MR = Delta_Phi - (H_p->Thita_P[j+1] - H_p->Thita_P[j]) + 1;
		for (k = H_p->Thita_P[j] + 1 ; k < H_p->Thita_P[j+1] ; k++) {
		   p2 = &A_L_I3[i][Sum];
		   if (j != 1)
		       {Phi_Idx = H_p->Phi_P[1]; Real_Phi_degree = Stitch_Line_phi1;  }
		   else   {
			   if (ML >= 0) {Phi_Idx = H_p->Phi_P[0] + ML; Real_Phi_degree = Stitch_Line_phi0 + ML * 360 / H_p->H_Blocks; };
			   if (MR >= 0) {Phi_Idx = H_p->Phi_P[0] + MR; Real_Phi_degree = Stitch_Line_phi0 + MR * 360 / H_p->H_Blocks; };
		   }

//		   p2->G_t_p.phi   =  0x4000 - 0x10000 * Phi_Idx / H_p->H_Blocks;
		   p2->G_t_p.phi   =  0x4000 - 0x10000 * Real_Phi_degree / 360;
		   p2->G_t_p.thita = 0x10000 * k / H_p->H_Blocks;
		   for (L = 0; L < 2; L++) p2->p[L].Sensor_Idx  = A_L_I3_Line[j+4].SL_Sensor_Id[L];
		   Sum++;
		   Point++;
		   ML--;   MR++;
		};
		A_L_I3_Line[j+4].SL_Sum[i] = Point;
	}

		A_L_I3_Line[7].SL_Point[i] = Sum;
		Point = 0;

		ML = Delta_Phi - 1;
		MR = Delta_Phi - ((H_p->Thita_P[0] - H_p->Thita_P[3])+ H_p->H_Blocks)  + 1 ;
		for (k = H_p->Thita_P[3] + 1 ; k < H_p->Thita_P[0] + H_p->H_Blocks; k++) {
		   p2 = &A_L_I3[i][Sum];

		   if (ML >= 0) {Phi_Idx = H_p->Phi_P[0] + ML; Real_Phi_degree = Stitch_Line_phi0 + ML * 360 / H_p->H_Blocks; };
		   if (MR >= 0) {Phi_Idx = H_p->Phi_P[0] + MR; Real_Phi_degree = Stitch_Line_phi0 + MR * 360 / H_p->H_Blocks; };

//		   p2->G_t_p.phi   =  0x4000 - 0x10000 * Phi_Idx / H_p->H_Blocks;
		   p2->G_t_p.phi   =  0x4000 - 0x10000 * Real_Phi_degree / 360;
		   p2->G_t_p.thita = 0x10000 * k / H_p->H_Blocks;
		   for (L = 0; L < 2; L++) p2->p[L].Sensor_Idx  = A_L_I3_Line[7].SL_Sensor_Id[L];
		   Sum++;
		   Point++;
		   ML--;   MR++;
		};
		A_L_I3_Line[7].SL_Sum[i] = Point;
    A_L_I3_Header[i].Sum = Sum;

// Make A_L_S3
	struct Adjust_Line_S3_Struct *S3_P;
	int S3_Idx;
// A_L_S3 Sensor 0
	S3_P = &A_L_S3[i][0]; S3_Idx = 0;
	S3_Idx = Make_A_L_S3_One_Line(i,  0,  S3_Idx,7, S3_P, 1);
	S3_Idx = Make_A_L_S3_One_Line(i,  0,  S3_Idx,6, S3_P, 1);
	S3_Idx = Make_A_L_S3_One_Line(i,  0,  S3_Idx,5, S3_P, 1);
	S3_Idx = Make_A_L_S3_One_Line(i,  0,  S3_Idx,4, S3_P, 1);
	S3_P->Sum = S3_Idx;
// A_L_S3 Sensor 1
	S3_P = &A_L_S3[i][1]; S3_Idx = 0;
	S3_Idx = Make_A_L_S3_One_Line(i,  1,  S3_Idx,3, S3_P, 1);
	S3_Idx = Make_A_L_S3_One_Line(i,  1,  S3_Idx,7, S3_P, 0);
	S3_Idx = Make_A_L_S3_One_Line(i,  1,  S3_Idx,0, S3_P, 0);
	S3_P->Sum = S3_Idx;
// A_L_S3 Sensor 2
	S3_P = &A_L_S3[i][2]; S3_Idx = 0;
	S3_Idx = Make_A_L_S3_One_Line(i,  2,  S3_Idx,2, S3_P, 1);
	S3_Idx = Make_A_L_S3_One_Line(i,  2,  S3_Idx,6, S3_P, 0);
	S3_Idx = Make_A_L_S3_One_Line(i,  2,  S3_Idx,3, S3_P, 0);
	S3_P->Sum = S3_Idx;
// A_L_S3 Sensor 3
	S3_P = &A_L_S3[i][3]; S3_Idx = 0;
	S3_Idx = Make_A_L_S3_One_Line(i,  3,  S3_Idx,1, S3_P, 1);
	S3_Idx = Make_A_L_S3_One_Line(i,  3,  S3_Idx,5, S3_P, 0);
	S3_Idx = Make_A_L_S3_One_Line(i,  3,  S3_Idx,2, S3_P, 0);
	S3_P->Sum = S3_Idx;
// A_L_S3 Sensor 4
	S3_P = &A_L_S3[i][4]; S3_Idx = 0;
	S3_Idx = Make_A_L_S3_One_Line(i,  4,  S3_Idx,0, S3_P, 1);
	S3_Idx = Make_A_L_S3_One_Line(i,  4,  S3_Idx,4, S3_P, 0);
	S3_Idx = Make_A_L_S3_One_Line(i,  4,  S3_Idx,1, S3_P, 0);
	S3_P->Sum = S3_Idx;

    int I3_Idex;
    for (j = 0; j < 5; j++) {
    	S3_P = &A_L_S3[i][j];
		for (k = 0; k < S3_P->Sum; k++) {
    	    I3_Idex = S3_P->Source_Idx[k];
    	    p2 = &A_L_I3[i][I3_Idex];
			Global2Sensor(i,j, p2->G_t_p.phi,  p2->G_t_p.thita, G_Degree);
			S3_P->S_t_p[k]     = PP_Temp.S_t_p[j];
			if (p2->p[0].Sensor_Idx == j) {
				p2->p[0].S_t_p     = PP_Temp.S_t_p[j];
				p2->p[0].A_L_S_Idx = k;
			}
			if (p2->p[1].Sensor_Idx == j) {
				p2->p[1].S_t_p     = PP_Temp.S_t_p[j];
				p2->p[1].A_L_S_Idx = k;
			}
		}
    }
	Make_thita_rate3(i);
    Make_Stitch_Line_Command(i);
  }
}//*/


// Miller 20171031   End




//int scale_mode = 0;
//int scale_size = 1;

void Make_ZY_Table( void)
{
  int i,j;
  float thita1_T;
  float rx1,ry1,rz1;
  float temp_r1;
  float thita1;
  float phi1;
  short Temp_phi,Temp_thita;
  for (i = 0; i <= 128; i++) {
    for (j = 0; j < 256; j++) {
      phi1   = i * pi/128.0;
      thita1 = j * pi/128.0;
      rx1 = sin(phi1) * cos(thita1);       // sin(temp_r) *
      ry1 = sin(phi1) * sin(thita1);       // sin(temp_r) *
      rz1 = sin(pi2 - phi1);
/*
      Temp_thita = 0x4000 - atan2(rz1,rx1) * 0x8000 / pi;
      Temp_phi   = 0x4000 - (asin(ry1))    * 0x8000 / pi;
      Trans_ZY_thita[i][j] = Temp_thita;
      Trans_ZY_phi[i][j]   = Temp_phi;
*/
      Trans_ZY_thita[i][j] = (short)(0x4000 - atan2(rz1,rx1) * 0x8000 / pi);
      Trans_ZY_phi[i][j]   = (short)(0x4000 - (asin(ry1))    * 0x8000 / pi);
    }
  }
// Make sin table
  for (i = 0; i < 256; i++) {
    Trans_Sin[i] = (short)(16384.01 * sin(i * pi / 128) );
  }

}

//int MT_Size;
/*void Trans_ZY( unsigned short phi1, unsigned short thita1, unsigned short *phi2, unsigned short *thita2 )
{
   unsigned short phi1_T;
   unsigned short ph,pl,th,tl;
   unsigned short ph_p,th_p;
   unsigned short pl2,tl2;
   short p0,p1,p2,p3;
   short t0,t1,t2,t3;
   short p21,p22,p23;
   short t21,t22,t23;
   unsigned short thita3;
   int flag;

   phi1_T = phi1;

   ph = (phi1_T >> 8) & 0x7f;
   ph_p = ph+1;
   th = (thita1 >> 8) & 0xff;  th_p = (th+1)& 0xff;
   pl = phi1_T & 0xff;
   tl = thita1 & 0xff;
   pl2= 0x100 - pl;
   tl2= 0x100 - tl;
   p0 = Trans_ZY_phi  [ph  ][th  ];
   t0 = Trans_ZY_thita[ph  ][th  ];
   p1 = Trans_ZY_phi  [ph  ][th_p];
   t1 = Trans_ZY_thita[ph  ][th_p];
   p2 = Trans_ZY_phi  [ph_p][th  ];
   t2 = Trans_ZY_thita[ph_p][th  ];
   p3 = Trans_ZY_phi  [ph_p][th_p];
   t3 = Trans_ZY_thita[ph_p][th_p];

   p21 = p1 - p0;
   p22 = p2 - p0;
   p23 = p3 - p0;
   t21 = t1 - t0;
   t22 = t2 - t0;
   t23 = t3 - t0;
   *phi2   = (p0+((( p22 * pl) * tl2 + (p21 * pl2 + p23 * pl) * tl) >> 16)) & 0xffff;
   flag = 0;
   if ((ph == 0x3f) || (ph == 0x40)) {
     if ((th == 0x3f) || (th == 0x40)) flag = 1;
     if ((th == 0xbf) || (th == 0xc0)) flag = 2;
   }
   switch (flag) {
       case 0:
	   *thita2 = (t0+((( t22 * pl) * tl2 + (t21 * pl2 + t23 * pl) * tl) >> 16)) & 0xffff;
	   break;
	   case 1:
		 if ((0x4000 - thita1 != 0) || (0x4000 - phi1 != 0))
			*thita2 = (short)(0x8000 * atan2(0x4000 - thita1, 0x4000 - phi1) / pi);
		 else
			*thita2 = 0;
	   break;
	   case 2:
		 if ((thita1 - 0xc000 != 0) || (0x4000 - phi1 != 0))
		   *thita2 = (short)(0x8000 * atan2(thita1 - 0xc000, 0x4000 - phi1) / pi);
		 else
			*thita2 = 0;
	   break;
   }
}//*/
void Trans_ZY( unsigned short phi1, unsigned short thita1, unsigned short *phi2, unsigned short *thita2 )
{
	float rx1,ry1,rz1;
	float Temp_thita;
	float Temp_phi;

	Temp_phi   = phi1   * pi/ 32768.0;
	Temp_thita = thita1 * pi/ 32768.0;
	rx1 = sin(Temp_phi) * cos(Temp_thita);       // sin(temp_r) *
	ry1 = sin(Temp_phi) * sin(Temp_thita);       // sin(temp_r) *
	rz1 = sin(pi2 - Temp_phi);
	*thita2 = (short)(0x4000 - atan2(rz1,rx1) * 0x8000 / pi);
	*phi2   = (short)(0x4000 - (asin(ry1))    * 0x8000 / pi);
}//*/

void Trans_YZ( unsigned short phi1, unsigned short thita1, unsigned short *phi2, unsigned short *thita2 )
{
	float rx1,ry1,rz1;
	float Temp_thita;
	float Temp_phi;

    Temp_phi   = phi1   * pi/ 32768.0;
    Temp_thita = thita1 * pi/ 32768.0;
    rx1 = sin(Temp_phi) * cos(Temp_thita);       // sin(temp_r) *
    ry1 = sin(Temp_phi) * sin(Temp_thita);       // sin(temp_r) *
    rz1 = sin(pi2 - Temp_phi);
    if (rz1 == 0 && ry1 == 0)
    	*thita2 = 0;
    else
    	*thita2 = (short)(0x4000 - atan2(ry1,rz1) * 0x8000 / pi);
    *phi2   = (short)(0x4000 - (asin(rx1))    * 0x8000 / pi);
}

int I_Sin( unsigned short thita)
{
   unsigned short th,tl,tl2,th_p;
   short t0,t1,t12;
   th = (thita >> 8) & 0xff;  th_p = (th+1)& 0xff;
   tl = thita & 0xff;
   tl2= 0x100 - tl;
   t0 = Trans_Sin[th  ];
   t1 = Trans_Sin[th_p];
   t12 = t1 - t0;

   return(t0 + (t12 * tl) / 256);
}

int I_Cos( unsigned short thita2)
{
   unsigned short th,tl,tl2, th_p;
   short t0,t1;
   unsigned short thita;
   thita = (thita2 + 0x4000) & 0xffff;
   th = (thita >> 8) & 0xff;  th_p = (th+1)& 0xff;
   tl = thita & 0xff;
   tl2= 0x100 - tl;
   t0 = Trans_Sin[th  ];
   t1 = Trans_Sin[th_p];

   return(((t0 * tl2 + t1 * tl) / 256));
}


void Make_thita_rate(void)
{
  int i,j,k;
  int Sum;
  unsigned short Temp_P0, Temp_P1, Temp_P2, Distance;
  for (k = 0; k < 5; k++) {
	Sum = A_L_S2[k].Sum;
    for(i=0;i<Sum; i++) {
       Temp_P0 = A_L_S2[k].AL_p[i].S_t_p1.thita / 16;
			if (i == (Sum - 1) ) {
	 Temp_P1 = A_L_S2[k].AL_p[0].S_t_p1.thita / 16;
      }
      else  {
	 Temp_P1 = A_L_S2[k].AL_p[i + 1].S_t_p1.thita / 16;
      }

      Distance = ((Temp_P0 - Temp_P1)) & 0x0fff;
      for(j = 0; j < Distance; j++) {
	A_L_S2[k].S_rate_Idx[(Temp_P1 + j) & 0x0fff] = i;
	A_L_S2[k].S_rate_Sub[(Temp_P1 + j) & 0x0fff] = j *0x10000 / Distance;
      }
    }
  }
}

void Make_ST_Sensor_Cmd(void)
{
	int binn, i, j, Addr;
	int T_DDR_P, offset_y, offset_x;
	int base_x, base_y;
	int st_size=0, size=0;
	int size_x, size_y;
	struct US360_Stitching_Table *ST_P;
	int sum, idx;

	offset_y = 64 * 0x8000;
	offset_x = 64 * 2;
	base_x = 512;
	base_y = 512;
    
    for(binn = 0; binn < 5; binn++) {
        //T_DDR_P = ST_STM1_P0_T_ADDR;                   // RAW, F2 DDR
        switch(binn) {
            // WDR 6x6 Binn, 4608/6=768 3456/6=576, 768*3*4=9216
            case 0: size_x = (9216 >> 6); size_y = (576 >> 6);  T_DDR_P = ST_STM1_P0_T_ADDR; break;
            case 1: size_x = (9216 >> 6); size_y = (576 >> 6);  T_DDR_P = ST_STM1_P0_T_ADDR + 576 * 0x8000; break;    // 9*6*8192
		    // RAW
            case 2: size_x = (4480 >> 6); size_y = (3392 >> 6); T_DDR_P = ST_STM1_P0_T_ADDR; break;
            case 3: size_x = (2240 >> 6); size_y = (1728 >> 6); T_DDR_P = ST_STM1_P0_T_ADDR; break;
            case 4: size_x = (1536 >> 6); size_y = (1152 >> 6); T_DDR_P = ST_STM1_P0_T_ADDR; break;
        }

        sum = 0;
        idx = ST_S_Header[binn].Start_Idx[0];
        for(i = 0 ; i < size_y ; i ++){
			for(j = 0 ; j < size_x ; j ++){
				ST_P  = &ST_S[idx+sum];    // RAW / Removal
				ST_P->CB_DDR_P = (T_DDR_P + i * offset_y + j * offset_x) >> 5;
				ST_P->CB_Sensor_ID = 4;
				ST_P->CB_Alpha_Flag = 0;
				ST_P->CB_Block_ID = 1;
				ST_P->CB_Mask = 0;
				ST_P->CB_Posi_X0 = base_x +       j * 64 * 4;
				ST_P->CB_Posi_Y0 = base_y +       i * 64 * 4;
				ST_P->CB_Posi_X1 = base_x + (j + 1) * 64 * 4;
				ST_P->CB_Posi_Y1 = base_y +       i * 64 * 4;
				ST_P->CB_Posi_X2 = base_x +       j * 64 * 4;
				ST_P->CB_Posi_Y2 = base_y + (i + 1) * 64 * 4;
				ST_P->CB_Posi_X3 = base_x + (j + 1) * 64 * 4;
				ST_P->CB_Posi_Y3 = base_y + (i + 1) * 64 * 4;
				ST_P->CB_Adj_Y0 = 0x80;
				ST_P->CB_Adj_Y1 = 0x80;
				ST_P->CB_Adj_Y2 = 0x80;
				ST_P->CB_Adj_Y3 = 0x80;
				ST_P->CB_Adj_U0 = 0x80;
				ST_P->CB_Adj_U1 = 0x80;
				ST_P->CB_Adj_U2 = 0x80;
				ST_P->CB_Adj_U3 = 0x80;
				ST_P->CB_Adj_V0 = 0x80;
				ST_P->CB_Adj_V1 = 0x80;
				ST_P->CB_Adj_V2 = 0x80;
				ST_P->CB_Adj_V3 = 0x80;

				sum++;
			}
		}
	    ST_S_Header[binn].Sum[0][0] = ST_S_Header[binn].Sum[0][1] = sum;
	}
}

void Make_ST_Cmd_Tmp(void)
{
	int i,j;  int Idx;
	Idx = 0;
	for(i = 0; i < 6; i++) {
		S_Rotate_R_Init(i);
		Write_Map_Proc(i);
/*		for (j = 0; j < 5; j++) {
			ST_Header[i].Start_Idx[j] = Idx;
			Idx += ST_Header[i].Sum[j][1];
		}*/
	}
	Make_ST_Sensor_Cmd();      // RAW / Removal
}

/*
 * M_Mode: 0->12K, scale=1
 *         1-> 8K, scale=2
 *         2-> 6K, scale=2
 *         3-> 4K, scale=3
 *         4-> 3K, scale=2
 *         5-> 2K, scale=2
 */
void Get_M_Mode(int *M_Mode, int *S_Mode)
{
    int c_mode, mode, res;
    c_mode = getCameraMode();
    mode = Stitching_Out.Output_Mode;
    res = Stitching_Out.OC[mode].Resolution_Mode;
    switch(res) {
        case 1:  *M_Mode = 0; *S_Mode = 3; break;   // 12K
        case 2:  *M_Mode = 3; *S_Mode = 3; break;   // 4K
        case 7:
            //拍照的6K/8K, 採用12K DMA成6K/8K
        	if(c_mode == CAMERA_MODE_TIMELAPSE) { *M_Mode = 1; *S_Mode = 4; }
        	else                                { *M_Mode = 0; *S_Mode = 3; }
        	break;   // 8K
        case 12:
            //拍照的6K/8K, 採用12K DMA成6K/8K
        	if(c_mode == CAMERA_MODE_TIMELAPSE) { *M_Mode = 2; *S_Mode = 4; }
        	else                                { *M_Mode = 0; *S_Mode = 3; }
        	break;   // 6K
        case 13: *M_Mode = 4; *S_Mode = 4; break;   // 3K
        case 14: *M_Mode = 5; *S_Mode = 5; break;   // 2K
        default: *M_Mode = 0; *S_Mode = 3; break;   // 12K
    }
}
int Get_Input_Scale(void)
{
    int Input_Scale;
    int c_mode, mode, res;
    c_mode = getCameraMode();
    mode = Stitching_Out.Output_Mode;
    res = Stitching_Out.OC[mode].Resolution_Mode;
    switch(res) {
        default:
        case 1:  Input_Scale = 1; break;   // 12K
        case 2:  Input_Scale = 3; break;   // 4K
        case 7:
            //拍照的6K/8K, 採用12K DMA成6K/8K
        	if(c_mode == CAMERA_MODE_TIMELAPSE) Input_Scale = 2;
        	else                                Input_Scale = 1;
        	break;   // 8K
        case 12:
            //拍照的6K/8K, 採用12K DMA成6K/8K
        	if(c_mode == CAMERA_MODE_TIMELAPSE) Input_Scale = 2;
        	else                                Input_Scale = 1;
        	break;   // 6K
        case 13: Input_Scale = 2; break;   // 3K
        case 14: Input_Scale = 2; break;   // 2K
    }
    return Input_Scale;
}
void Stitch_Init_All(void)
{
	int i;

	Set_G_Degree(LensCode);

	Smooth_O_Init();
#ifndef ANDROID_CODE
	for (i = 0; i < 5; i++)
	  Adj_Sensor_Command[i] = Adj_Sensor_Default[LensCode][i];
#endif
	Make_Lens_Rate_Line_Table_Proc();

	ResolutionSpecInit();

	Make_ZY_Table();
	Make_ALI2();
	Make_A_L_S2_Table();
	Make_A_L_I3_Table();
	Smooth_I_Init();
	Make_Smooth_table();
	Make_ST_Cmd_Tmp();
	FPGA_DDR_P_Init();
#ifdef ANDROID_CODE
    do_sc_cmd_Tx_func("MKST", "HEAD");      // send ST_Header[6]
#endif

    if(Make_Plant_To_Global_Table_Flag == 1) {
    	Make_Plant_To_Global_Table_Flag = 0;
    	Make_Plant_To_Global_Table(Out_Mode_Plant_3dModel);
    }
    SendSTCmd3DModel(1);
}

void SendSTCmd(void)
{
	int i, j;
	int M_Mode, S_Mode;
	Get_M_Mode(&M_Mode, &S_Mode);
#ifdef ANDROID_CODE
	send_WDR_Diffusion_Table(0);     // rex+
	//send_WDR_Live_Diffusion_Table();

	if(DebugJPEGMode == 1 && ISP2_Sensor == -9) {
		for(i = 0; i < 2; i++) {		//f_id
			send_Sitching_Cmd(i);
			send_Sitching_Sensor_Cmd(i);
			send_Sitching_Tran_Cmd(i);
			for(j = 0; j < 2; j++)		//MP_B / MP_B2
				Send_MP_B_Table(i, j);
		}
		Send_Smooth_Table(M_Mode, 0);
		do_ST_Table_Mix(M_Mode);
		for(i = 0; i < 5; i++)
			doSensorLensAdj(i, 2);
	}
	else {
		for(i = 0; i < 2; i++) {		//f_id
			send_Sitching_Cmd(i);
			send_Sitching_Sensor_Cmd(i);
			send_Sitching_Tran_Cmd(i);
			for(j = 0; j < 2; j++)		//MP_B / MP_B2
				Send_MP_B_Table(i, j);
		}
		Send_Smooth_Table(S_Mode, 0);
		do_ST_Table_Mix(S_Mode);
		for(i = 0; i < 5; i++)
			doSensorLensAdj(i, 2);
	}
#endif
}

void SendSTCmd3DModel(int isInit)
{
	int i, j;
	int cnt=1;
	int M_Mode, S_Mode;
	int idx = get_A2K_ST_3DModel_Idx();
	static int idx_lst = -1;
	Get_M_Mode(&M_Mode, &S_Mode);

	if(idx == idx_lst && isInit == 0)
		return;

	if(isInit == 1) {
		cnt = 8;
		ST_3DModel_Idx = 0;
	}
	else {
		cnt = 1;
		ST_3DModel_Idx = ((idx-2)&0x7);
		idx_lst = idx;
	}
	for(j = 0; j < cnt; j++) {
		Write_3D_Model_Proc(S_Mode, isInit, &Out_Mode_Plant_3dModel, &Out_Mode_Pillar_3dModel);
#ifdef ANDROID_CODE
		for(i = 0; i < 2; i++) {		//f_id
			send_Sitching_Cmd_3DModel(i, isInit);
//			send_Sitching_Tran_Cmd_3DModel(i);
//			for(j = 0; j < 2; j++)		//MP_B / MP_B2
//				Send_MP_B_Table_3DModel(i, j);
		}
//		Send_Smooth_Table(M_Mode, 0);
//		do_ST_Table_Mix(M_Mode);
#endif
		SetWaveDebug(9, ST_3DModel_Idx);
		ST_3DModel_Idx = ((ST_3DModel_Idx+1)&0x7);
	}
}

void Test_Init_All(void)
{
   Global_Sensor_X_Offset_3 = 0;
   Global_Sensor_Y_Offset_3 = 0;
   memset(&A_L_S2, 0, sizeof(A_L_S2) );
   Stitch_Init_All();
}
/*
 *    Init執行一次, 計算縫合相關Table
 */
void LineTableInit(void)
{
    Stitch_Init_All();
}

/*
 *    切MODE與調整Sensor時,執行一次, 重新計算縫合相關Table
 */
void AdjFunction(void)
{
	Set_G_Degree(LensCode);

//	S_Rotate_R_Init();
	Make_Lens_Rate_Line_Table_Proc();

	ResolutionSpecInit();

	int M_Mode = 3, S_Mode;
#ifdef ANDROID_CODE
	Get_M_Mode(&M_Mode, &S_Mode);
#endif

	Make_ZY_Table();
	Make_ALI2();
	Make_A_L_S2_Table();
	Make_A_L_I3_Table();
	Smooth_I_Init();
	Make_Smooth_table();
	Make_ST_Cmd_Tmp();

	if(Make_Plant_To_Global_Table_Flag == 1) {
		Make_Plant_To_Global_Table_Flag = 0;
		Make_Plant_To_Global_Table(Out_Mode_Plant_3dModel);
	}
}

int SLens_Y_Top_Limit = 80, SLens_Y_Low_Limit = 1;
void SetSensorLensYLimit(int idx, int value)
{
	if(idx == 0) SLens_Y_Top_Limit = value;
	else		 SLens_Y_Low_Limit = value;

	if(SLens_Y_Top_Limit > 1024) SLens_Y_Top_Limit = 1024;
	if(SLens_Y_Top_Limit < 1)    SLens_Y_Top_Limit = 1;

	if(SLens_Y_Low_Limit > 1024) SLens_Y_Low_Limit = 1024;
	if(SLens_Y_Low_Limit < 0)    SLens_Y_Low_Limit = 0;

	if(SLens_Y_Top_Limit <= SLens_Y_Low_Limit) SLens_Y_Low_Limit = SLens_Y_Top_Limit - 1;
}

void GetSensorLensYLimit(int *value)
{
	*value 	   = SLens_Y_Top_Limit;
	*(value+1) = SLens_Y_Low_Limit;
}

//修補暗角
int Sensor_Lens_D_E[2] = {0, 2580};		//爬升率開始改變
int Sensor_Lens_End[2] = {0, 400};		//爬升率
int Sensor_Lens_C_Y[2] = {0, 0};		//Sensor中心偏移
void SetSensorLensDE(int value)
{
	Sensor_Lens_D_E[LensCode] = value;
}

int GetSensorLensDE(void)
{
	return Sensor_Lens_D_E[LensCode];
}

void SetSensorLensEnd(int value)
{
	Sensor_Lens_End[LensCode] = value;
}

int GetSensorLensEnd(void)
{
	return Sensor_Lens_End[LensCode];
}

void SetSensorLensCY(int value)
{
	Sensor_Lens_C_Y[LensCode] = value;
}

int GetSensorLensCY(void)
{
	return Sensor_Lens_C_Y[LensCode];
}

float Sensor_Lens_trans(int D_X, int D_Y)
{
  float Adj_V, Adj_V2;
  int Distance;
  int D_H, D_L, D_E_Buf;

  Distance = sqrt(D_X*D_X + D_Y*D_Y);
  D_H = Distance / 256;
  D_L = Distance & 0xFF;
  D_E_Buf = Distance - Sensor_Lens_D_E[LensCode];
  if (D_E_Buf > 0)		//超過距離, 爬升率開始改變
	  Adj_V2 = (float)Sensor_Lens_End[LensCode] * (float)D_E_Buf / 256.0 / 1000.0;
  else
	  Adj_V2 = 0;

  if(D_H > 9) { D_H = 9; D_L = 256; }
  Adj_V = ( (Sensor_Lens_Table[LensCode][D_H] * (256.0 - (float)D_L) + Sensor_Lens_Table[LensCode][D_H + 1] * (float)D_L) / 256.0 + Adj_V2) * Sensor_Lens_Gain/* * rate_y*/ + 1.0;
  return(Adj_V);
}

int do_calibration_flag = 0;
short S_Rotate_R1_G[5], S_Rotate_R2_G[5], S_Rotate_R3_G[5];
void S_Rotate_R_Init(int M_Mode)
{
	int i;

	for(i = 0; i < 5; i++) {
#ifdef ANDROID_CODE
		if(do_calibration_flag == 1) {
			S_Rotate_R1_G[i]  = Adj_Sensor_Command[i].Rotate_R1 * 0x8000 / 18000;
			S_Rotate_R2_G[i]  = Adj_Sensor_Command[i].Rotate_R2 * 0x8000 / 18000;
			S_Rotate_R3_G[i]  = Adj_Sensor_Command[i].Rotate_R3 * 0x8000 / 18000;
		}
		else {
			if(i == 0) {
				S_Rotate_R1_G[i]  = Adj_Sensor_Command[i].Rotate_R1 * 0x8000 / 18000;
				S_Rotate_R2_G[i]  = Adj_Sensor_Command[i].Rotate_R2 * 0x8000 / 18000;
				S_Rotate_R3_G[i]  = (Adj_Sensor_Command[i].Rotate_R3 + (3-A_L_I3_Header[M_Mode].H_Degree_offset) * 100) * 0x8000 / 18000;
			}
			else {
				S_Rotate_R1_G[i]  = (Adj_Sensor_Command[i].Rotate_R1 + (3-A_L_I3_Header[M_Mode].H_Degree_offset) * 100) * 0x8000 / 18000;
				S_Rotate_R2_G[i]  = Adj_Sensor_Command[i].Rotate_R2 * 0x8000 / 18000;
				S_Rotate_R3_G[i]  = Adj_Sensor_Command[i].Rotate_R3 * 0x8000 / 18000;
			}
		}
#else
		S_Rotate_R1_G[i]  = Adj_Sensor_Command[i].Rotate_R1 * 0x8000 / 18000;
		S_Rotate_R2_G[i]  = Adj_Sensor_Command[i].Rotate_R2 * 0x8000 / 18000;
		S_Rotate_R3_G[i]  = Adj_Sensor_Command[i].Rotate_R3 * 0x8000 / 18000;
#endif
	}
}

int Adj_TY = 128, Adj_TU = 128, Adj_TV = 128;
int Delta_V = 127;	//30;
void setGlobalSensorXOffset(int idx, int offset)
{
	switch(idx){
		case 1:		Global_Sensor_X_Offset_1 = offset; break;
		case 2:		Global_Sensor_X_Offset_2 = offset; break;
		case 3:		Global_Sensor_X_Offset_3 = offset; break;
		case 4:		Global_Sensor_X_Offset_4 = offset; break;
		case 5:		Global_Sensor_X_Offset_6 = offset; break;
	}
}
void getGlobalSensorXOffset(int idx, int *val)
{
	switch(idx){
		case 1:		*val      = Global_Sensor_X_Offset_1; break;
		case 2:		*val      = Global_Sensor_X_Offset_2; break;
		case 3:		*val      = Global_Sensor_X_Offset_3; break;
		case 4:		*val      = Global_Sensor_X_Offset_4; break;
		case 5:		*val      = Global_Sensor_X_Offset_6; break;
	}
}
void setGlobalSensorYOffset(int idx, int offset)
{
	switch(idx){
		case 1:		Global_Sensor_Y_Offset_1 = offset; break;
		case 2:		Global_Sensor_Y_Offset_2 = offset; break;
		case 3:		Global_Sensor_Y_Offset_3 = offset; break;
		case 4:		Global_Sensor_Y_Offset_4 = offset; break;
		case 5:		Global_Sensor_Y_Offset_6 = offset; break;
	}
}
void getGlobalSensorYOffset(int idx, int *val)
{
	switch(idx){
		case 1:		*val      = Global_Sensor_Y_Offset_1; break;
		case 2:		*val      = Global_Sensor_Y_Offset_2; break;
		case 3:		*val      = Global_Sensor_Y_Offset_3; break;
		case 4:		*val      = Global_Sensor_Y_Offset_4; break;
		case 5:		*val      = Global_Sensor_Y_Offset_6; break;
	}
}

/*
 * 	製具程式做縫合校正時設成1, 結束時設成0
 */
void Set_Calibration_Flag(int flag)
{
	do_calibration_flag = flag;
}

/*
 * Sensor球型轉Sensor平面座標
 */
void Global2Sensor(int M_Mode, int S_Id,short I_phi, unsigned short I_thita, int Global_degree)
{
  short S_Rotate_R1, S_Rotate_R2, S_Rotate_R3;
  unsigned short I_phi3,I_thita3;
  unsigned short I_phi4,I_thita4;
  unsigned short I_phi5,I_thita5;
  int Global_degree2;

  static int maxT_Y, maxT_U, maxT_V, minT_Y, minT_U, minT_V;
  float T_X0,T_Y0;
  float T_X1,T_Y1;
  int i,j;
  int temp_r3, thita3;
  int temp_r45;
  int temp_r4;
  int Sensor_X, Sensor_Y;
  unsigned short S_rate_Idx;
  unsigned short S_rate_Sub, I_S_rate_Sub;
  short p0_phi, p1_phi;
  short p0_thita, p1_thita;
  if (S_Id == 0) Global_degree2 = Global_degree * LENS2CEN0[LensCode] / LENS2CEN1[LensCode];
  else           Global_degree2 = Global_degree;

   S_Rotate_R1  = S_Rotate_R1_G[S_Id];
   S_Rotate_R2  = S_Rotate_R2_G[S_Id];
   S_Rotate_R3  = S_Rotate_R3_G[S_Id];
   I_phi3   = I_phi;
   I_thita3 = I_thita;


   Trans_ZY(0x4000 - I_phi3, I_thita3 + S_Rotate_R1, &I_phi4, &I_thita4);
   Trans_ZY(I_phi4, I_thita4  + S_Rotate_R2, &I_phi5, &I_thita5);

   temp_r3    = I_phi5;
   thita3     = (I_thita5 + S_Rotate_R3) & 0xffff;
   PP_Temp.S_t_p[S_Id].phi   = temp_r3;
   PP_Temp.S_t_p[S_Id].thita = thita3;

   //Global_Senosr_Offset[6][2]
   int Offset_x = 0, Offset_y = 0;
#ifdef ANDROID_CODE
   if(do_calibration_flag == 1) {
	   Offset_x = 0;
	   Offset_y = 0;
   }
   else {
	   switch(M_Mode) {
	   case 0: Offset_x = Global_Sensor_X_Offset_1; 	Offset_y = Global_Sensor_Y_Offset_1; break;
	   case 1: Offset_x = Global_Sensor_X_Offset_2; 	Offset_y = Global_Sensor_Y_Offset_2; break;
	   case 2: Offset_x = Global_Sensor_X_Offset_2;  	Offset_y = Global_Sensor_Y_Offset_2; break;
	   case 3: Offset_x = Global_Sensor_X_Offset_3;     Offset_y = Global_Sensor_Y_Offset_3; break;
	   case 4: Offset_x = Global_Sensor_X_Offset_4; 	Offset_y = Global_Sensor_Y_Offset_4; break;
	   case 5: Offset_x = Global_Sensor_X_Offset_6; 	Offset_y = Global_Sensor_Y_Offset_6; break;
	   }
   }
#endif

   Sensor_X = Adj_Sensor_Command[S_Id].S[4].X + Sensor_C_X_Base + Offset_x;
   Sensor_Y = Adj_Sensor_Command[S_Id].S[4].Y + Sensor_C_Y_Base + Offset_y;

   if (temp_r3 < 0x4000) {	//180	//0x38E3 = 160
	   int delta_thita1, delta_phi1;
	   float  delta_thita2, delta_phi2;
	   int adj_thita, adj_phi;
	   int delta_Rate,temp_thita, temp_phi;
	   float A_t_p_thita;
       float A_t_p_phi;
       float thita_Rate, phi_Rate;
       int   thita_Rate2, I_thita_Rate2, thita_Rate3, I_thita_Rate3;
	   int T_Y,T_U,T_V;
       struct Adjust_Line_O_Struct *AL_p0, *AL_p1;

       // Make thita rate
       if(do_calibration_flag == 0) {
		   S_rate_Idx = A_L_S2[S_Id].S_rate_Idx[( (thita3 >> 4) & 0xfff)];

		   if(S_Id >= 5 || S_Id < 0){                                       // A_L_S2[5]
#ifdef ANDROID_CODE
			 db_error("Global2Sensor: M_Mode=%d S_Id=%d\n", M_Mode, S_Id);
#endif
			 return;
		   }
		   if(A_L_S2[S_Id].Sum > 24 || A_L_S2[S_Id].Sum <= 0){              // AL_p[24], rex+ 180222, data not ready
			 //db_error("Global2Sensor: S_Id=%d Sum=%d\n", A_L_S2[S_Id].Sum);   // Global2Sensor: S_Id=0 Sum=5701632
			 return;
		   }
		   if(S_rate_Idx > (A_L_S2[S_Id].Sum-1) ) S_rate_Idx = (A_L_S2[S_Id].Sum-1);
		   S_rate_Sub = A_L_S2[S_Id].S_rate_Sub[( (thita3 >> 4) & 0xfff)] >> 2;

		   int s_idx0, s_idx1;
		   AL_p0 = & A_L_S2[S_Id].AL_p[S_rate_Idx];
		   s_idx0 = A_L_S2[S_Id].Source_Idx[S_rate_Idx];
		   if(s_idx0 > Adjust_Line_I2_MAX-1) s_idx0 = Adjust_Line_I2_MAX-1;
		   if (S_rate_Idx >= A_L_S2[S_Id].Sum - 1) {
			   AL_p1 = & A_L_S2[S_Id].AL_p[0];
			   s_idx1 = A_L_S2[S_Id].Source_Idx[0];
			   if(s_idx1 > Adjust_Line_I2_MAX-1) s_idx1 = Adjust_Line_I2_MAX-1;
		   }
		   else {
			   AL_p1 = & A_L_S2[S_Id].AL_p[S_rate_Idx + 1];
			   s_idx1 = A_L_S2[S_Id].Source_Idx[S_rate_Idx + 1];
			   if(s_idx1 > Adjust_Line_I2_MAX-1) s_idx1 = Adjust_Line_I2_MAX-1;
		   }

		   I_S_rate_Sub = 0x4000 - S_rate_Sub;

		   p0_phi = AL_p0->A_t_p.phi;     p1_phi = AL_p1->A_t_p.phi;
		   p0_thita = AL_p0->A_t_p.thita; p1_thita = AL_p1->A_t_p.thita;
		   delta_phi1    =  (AL_p1->S_t_p1.phi  * I_S_rate_Sub + S_rate_Sub * AL_p0->S_t_p1.phi ) >> 14;
		   A_t_p_thita   =  ((float)p1_thita * (float)I_S_rate_Sub + (float)S_rate_Sub * (float)p0_thita) / 0x4000;
		   delta_phi2    =  ((float)p1_phi   * (float)I_S_rate_Sub + (float)S_rate_Sub * (float)p0_phi  ) / 0x4000;

		   if (delta_phi1 == 0) delta_phi1 =  1000;
       }
       else {
    	   delta_phi1  = 1000;
    	   A_t_p_thita = 0;
    	   delta_phi2  = 0;
       }

	   temp_r45 = (int) ( (temp_r3 + Global_degree2 * temp_r3 / 0x2000 + delta_phi2 * temp_r3 / delta_phi1) / 2);

	   if (temp_r45 < Lens_Rate_Line_MAX) temp_r4 = Lens_Rate_Line[temp_r45];
	   else                               temp_r4 = Lens_Rate_Line[Lens_Rate_Line_MAX - 1];

	   T_X0 = (      temp_r4 * I_Cos(thita3 + A_t_p_thita ) ) / 0x4000;
	   T_Y0 = (-1 *  temp_r4 * I_Sin(thita3 + A_t_p_thita ) ) / 0x4000;
	   T_X1 = (T_X0 + (Sensor_X << 4) ) / 16;
	   T_Y1 = (T_Y0 + (Sensor_Y << 4) ) / 16;
	 }
	 else {
		T_X1 = 0x4000;
		T_Y1 = 0x4000;
	 }
	 PP_Temp.S[S_Id].X  = T_X1;
	 PP_Temp.S[S_Id].Y  = T_Y1;
};


void Map_To_Sensor_Trans(int M_Mode, struct XY_Posi * XY_P)
{
  int S_Id;
  for (S_Id = 0; S_Id < 5; S_Id++) {
	Global2Sensor(M_Mode, S_Id, XY_P->I_phi, XY_P->I_thita, G_Degree);
    XY_P->S[S_Id] = PP_Temp.S[S_Id];
  }
  Make_Transparent(M_Mode, XY_P);
};

int Show_40point_En = 0;
void setShow40pointEn(int en)
{
	Show_40point_En = en;
}

int getShow40pointEn(void)
{
	return Show_40point_En;
}


int GPadj_Width = GPadj_Step_Cnt * 64, GPadj_Height = 40 * 64;
int ALI_R_D0_Rate = 5, ALI_R_D1_Rate = 2, ALI_R_D2_Rate = 1;
void setALIRelationRate(int idx, int value)
{
	switch(idx) {
	case 0: ALI_R_D0_Rate = value; break;
	case 1: ALI_R_D1_Rate = value; break;
	case 2: ALI_R_D2_Rate = value; break;
	}
}

void getALIRelationRate(int *val)
{
	*val     = ALI_R_D0_Rate;
	*(val+1) = ALI_R_D1_Rate;
	*(val+2) = ALI_R_D2_Rate;
}


A_L_I_Relation_Struct ALI_R[40] = {
	{2, { { 1,  1}, { 2,  2}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1} }	},    //0
	{3, { { 0,  1}, { 2,  1}, { 3,  2}, {-1, -1}, {-1, -1}, {-1, -1} }	},    //1
	{4, { { 0,  2}, { 1,  1}, { 3,  1}, { 4,  2}, {-1, -1}, {-1, -1} } 	},    //2
	{4, { { 1,  2}, { 2,  1}, { 4,  1}, { 5,  2}, {-1, -1}, {-1, -1} } 	},    //3
	{4, { { 2,  2}, { 3,  1}, { 3,  1}, { 4,  2}, {-1, -1}, {-1, -1} } 	},    //4
	{4, { { 3,  2}, { 4,  1}, { 6,  1}, { 7,  2}, {-1, -1}, {-1, -1} } 	},    //5
	{4, { { 4,  2}, { 5,  1}, { 7,  1}, { 8,  2}, {-1, -1}, {-1, -1} } 	},    //6
	{6, { { 5,  2}, { 6,  1}, { 8,  1}, { 9,  2}, {32,  1}, {33,  2} } 	},    //7
	{6, { { 6,  2}, { 7,  1}, { 9,  1}, {10,  2}, {32,  1}, {33,  2} } 	},    //8
	{4, { { 7,  2}, { 8,  1}, {10,  1}, {11,  2}, {-1, -1}, {-1, -1} } 	},    //9
	{4, { { 8,  2}, { 9,  1}, {11,  1}, {12,  2}, {-1, -1}, {-1, -1} } 	},    //10
	{4, { { 9,  2}, {10,  1}, {12,  1}, {13,  2}, {-1, -1}, {-1, -1} } 	},    //11
	{4, { {10,  2}, {11,  1}, {13,  1}, {14,  2}, {-1, -1}, {-1, -1} } 	},    //12
	{4, { {11,  2}, {12,  1}, {14,  1}, {15,  2}, {-1, -1}, {-1, -1} } 	},    //13
	{3, { {12,  2}, {13,  1}, {15,  1}, {-1, -1}, {-1, -1}, {-1, -1} }	},    //14
	{2, { {13,  2}, {14,  1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1} }	},    //15


	{2, { {17,  1}, {18,  2}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1} }	},    //16
	{3, { {16,  1}, {18,  1}, {19,  2}, {-1, -1}, {-1, -1}, {-1, -1} }	},    //17
	{4, { {16,  2}, {17,  1}, {19,  1}, {20,  2}, {-1, -1}, {-1, -1} } 	},    //18
	{4, { {17,  2}, {18,  1}, {20,  1}, {21,  2}, {-1, -1}, {-1, -1} } 	},    //19
	{4, { {18,  2}, {19,  1}, {21,  1}, {22,  2}, {-1, -1}, {-1, -1} } 	},    //20
	{4, { {19,  2}, {20,  1}, {22,  1}, {23,  2}, {-1, -1}, {-1, -1} } 	},    //21
	{4, { {20,  2}, {21,  1}, {23,  1}, {24,  2}, {-1, -1}, {-1, -1} } 	},    //22
	{6, { {21,  2}, {22,  1}, {24,  1}, {25,  2}, {39,  1}, {38,  2} } 	},    //23
	{6, { {22,  2}, {23,  1}, {25,  1}, {26,  2}, {39,  1}, {38,  2} } 	},    //24
	{4, { {23,  2}, {24,  1}, {26,  1}, {27,  2}, {-1, -1}, {-1, -1} } 	},    //25
	{4, { {24,  2}, {25,  1}, {27,  1}, {28,  2}, {-1, -1}, {-1, -1} } 	},    //26
	{4, { {25,  2}, {26,  1}, {28,  1}, {29,  2}, {-1, -1}, {-1, -1} } 	},    //27
	{4, { {26,  2}, {27,  1}, {29,  1}, {30,  2}, {-1, -1}, {-1, -1} } 	},    //28
	{4, { {27,  2}, {28,  1}, {30,  1}, {31,  2}, {-1, -1}, {-1, -1} } 	},    //29
	{3, { {28,  2}, {29,  1}, {31,  1}, {-1, -1}, {-1, -1}, {-1, -1} }	},    //30
	{2, { {29,  2}, {30,  1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1} }	},    //31


	{6, { { 6,  2}, { 7,  1}, { 8,  1}, { 9,  2}, {33,  1}, {34,  2} } 	},    //32
	{3, { {32,  1}, {34,  1}, {35,  2}, {-1, -1}, {-1, -1}, {-1, -1} } 	},    //33
	{4, { {32,  2}, {33,  1}, {35,  1}, {36,  2}, {-1, -1}, {-1, -1} } 	},    //34
	{4, { {33,  2}, {34,  1}, {36,  1}, {37,  2}, {-1, -1}, {-1, -1} } 	},    //35
	{4, { {34,  2}, {35,  1}, {37,  1}, {38,  2}, {-1, -1}, {-1, -1} } 	},    //36
	{4, { {35,  2}, {36,  1}, {38,  1}, {39,  2}, {-1, -1}, {-1, -1} } 	},    //37
	{3, { {36,  2}, {37,  1}, {39,  1}, {-1, -1}, {-1, -1}, {-1, -1} }	},    //38
	{6, { {22,  2}, {23,  1}, {24,  1}, {25,  2}, {38,  1}, {37,  2} } 	},    //39
};


int get_block_complexity(char *buf, int s_x, int s_y)
{
	int i, j;
	int g1, g2;
	int sum, cp;
	char *ptr1, *ptr2;

	ptr1 = (buf + s_y * GPadj_Width + s_x);
	ptr2 = (buf + (s_y+64) * GPadj_Width + s_x);
	sum = 0;
	for(i = 0; i < 64; i++) {
		for(j = 0; j < 63; j++) {
			g1 = *(ptr1 + i * GPadj_Width + j);
			g2 = *(ptr2 + i * GPadj_Width + j);
			sum += abs(g1 - g2);
		}
	}
	cp = (sum / (63*64) );

	return cp;
}

int get_block_dc(char *buf, int s_x, int s_y)
{
	int i, j;
	int y;
	int sum, dc;
	char *ptr;

	ptr = (buf + s_y * GPadj_Width + s_x);
	sum = 0;
	for(i = 0; i < 64; i++) {
		for(j = 0; j < 64; j++) {
			y = *(ptr + i * GPadj_Width + j);
			sum += y;
		}
	}
	dc = (sum >> 12);
	return dc;
}

int get_block_sub_sum(char *buf, int s_x, int s_y)
{
	int i, j;
	int y1, subY1, dc1;
	int y2, subY2, dc2;
	int sum;
	char *ptr1, *ptr2;

	//dc1 = get_block_dc(img, s_x, s_y);
	//dc2 = get_block_dc(img, s_x, (s_y+64) );

	ptr1 = (buf + s_y * GPadj_Width + s_x);
	ptr2 = (buf + (s_y+64) * GPadj_Width + s_x);
	sum = 0;
	for(i = 0; i < 64; i++) {
		for(j = 0; j < 64; j++) {
			y1 = *(ptr1+i*GPadj_Width+j);
			//subY1 = y1 - dc1;

			y2 = *(ptr2+i*GPadj_Width+j);
			//subY2 = y2 - dc2;

			//sum += abs( (subY1-subY2) );
			sum += abs( (y1-y2) );
		}
	}
	return sum;
}

int Sensor_X_Step_debug = Sensor_X_Step;
void setSensorXStep(int value)
{
	Sensor_X_Step_debug = value;
}

void getSensorXStep(int *val)
{
	*val      = Sensor_X_Step_debug;
}

int Block_Idx[2];
int Block_Size[2];

/*
 * P_Mode:  0:Global 1:Plant 2:Pillar 3:3D-Model
 */
void Add_ST_One_Proc(int M_Mode, int Start_Idx, int S_Id, int i, int j, int doTrans, int P_Mode, int t_offset)
{
	int FPGA_ID;
	int MT = 0;
	struct A_L_I_Posi  MP;
	int Target_P;
	int k;
	struct MP_B_Sub_Struct *MP_BS_P, *MP_B2S_P;
	char *Tran_P;

	Get_FId_MT(S_Id, &FPGA_ID, &MT);

	switch(P_Mode) {
	default:
	case 0: ST_P    = &ST_I[FPGA_ID][Start_Idx + Block_Idx[FPGA_ID]] ;
			SLT_P   = &SLT[FPGA_ID][Start_Idx  + Block_Idx[FPGA_ID]] ;
			MP_B_P  = &MP_B[FPGA_ID][Start_Idx + Block_Idx[FPGA_ID]] ;
			MP_B2_P = &MP_B_2[FPGA_ID][Start_Idx + Block_Idx[FPGA_ID]] ;
			Tran_P  = &Transparent_B[FPGA_ID][Start_Idx + Block_Idx[FPGA_ID] ][0];
			break;		// Global
	//200406 max+, Plant / Pillar / 3D-Model, Plant&Pillar暫時同3D-Model, 目前沒有這兩個模式
	case 1:							// Plant
	case 2:							// Pillar
	case 3: ST_P    = &ST_I_3DModel[ST_3DModel_Idx][FPGA_ID][Start_Idx + Block_Idx[FPGA_ID]] ;
			SLT_P   = &SLT_3DModel[ST_3DModel_Idx][FPGA_ID][Start_Idx  + Block_Idx[FPGA_ID]] ;
			MP_B_P  = &MP_B_3DModel[ST_3DModel_Idx][FPGA_ID][Start_Idx + Block_Idx[FPGA_ID]] ;
			MP_B2_P = &MP_B_2_3DModel[ST_3DModel_Idx][FPGA_ID][Start_Idx + Block_Idx[FPGA_ID]] ;
			Tran_P  = &Transparent_B_3DModel[ST_3DModel_Idx][FPGA_ID][Start_Idx + Block_Idx[FPGA_ID] ][0];
			break;		// 3D-Model.
	}

	int  V_Sensor_X_Step =  Sensor_X_Step_debug;	//Sensor_X_Step/2;

	SLT_P->CB_Mode  = 0;
	SLT_P->CB_Idx   = 0;
	SLT_P->X_Posi   = j;
	SLT_P->Y_Posi   = i;

	// Miller 20171103
	MP.XY[0] =  Map_Posi_Tmp[i  ][j  ].S[S_Id];
	MP.XY[1] =  Map_Posi_Tmp[i  ][j+1].S[S_Id];
	MP.XY[2] =  Map_Posi_Tmp[i+1][j  ].S[S_Id];
	MP.XY[3] =  Map_Posi_Tmp[i+1][j+1].S[S_Id];

	MP.G_t_p[0].phi   =  Map_Posi_Tmp[i  ][j  ].I_phi;
	MP.G_t_p[0].thita =  Map_Posi_Tmp[i  ][j  ].I_thita;
	MP.G_t_p[1].phi   =  Map_Posi_Tmp[i  ][j+1].I_phi;
	MP.G_t_p[1].thita =  Map_Posi_Tmp[i  ][j+1].I_thita;
	MP.G_t_p[2].phi   =  Map_Posi_Tmp[i+1][j  ].I_phi;
	MP.G_t_p[2].thita =  Map_Posi_Tmp[i+1][j  ].I_thita;
	MP.G_t_p[3].phi   =  Map_Posi_Tmp[i+1][j+1].I_phi;
	MP.G_t_p[3].thita =  Map_Posi_Tmp[i+1][j+1].I_thita;

	if(M_Mode == 3 || M_Mode == 4 || M_Mode == 5) {
		Target_P = ((((i << 21) + (j << 7)) + ST_STM2_P0_T_ADDR + t_offset) >> 5);		// << 21: 64*32*1024, << 7: 64*2
	}
	else {
		Target_P = ((((i << 21) + (j << 7)) + ST_STM1_P0_T_ADDR + t_offset) >> 5);
	}
	Make_Stitch_Line_Block_One(M_Mode,S_Id, ST_P, &MP, Target_P, A_L_I3_Header[M_Mode].Binn, doTrans);

	int D_X, D_Y;
	int  Map4, Map5; int Trans_Temp;
// Miller 20180925
	Map4 = (Map_Posi_Tmp[i  ][j  ].Transparent[S_Id] << 3) |
		   (Map_Posi_Tmp[i  ][j+1].Transparent[S_Id] << 2) |
		   (Map_Posi_Tmp[i+1][j  ].Transparent[S_Id] << 1) |
			Map_Posi_Tmp[i+1][j+1].Transparent[S_Id];


	switch (Map4) {
		case 0x0: Map5 = 0x1111; break;
		case 0x1: Map5 = 0x0112; break;
		case 0x2: Map5 = 0x1021; break;
		case 0x3: Map5 = 0x1122; break;
		case 0x4: Map5 = 0x1201; break;
		case 0x5: Map5 = 0x1212; break;
		case 0x6: Map5 = 0x1221; break;
		case 0x7: Map5 = 0x1223; break;
		case 0x8: Map5 = 0x2110; break;
		case 0x9: Map5 = 0x2112; break;
		case 0xa: Map5 = 0x2121; break;
		case 0xb: Map5 = 0x2132; break;
		case 0xc: Map5 = 0x2211; break;
		case 0xd: Map5 = 0x2312; break;
		case 0xe: Map5 = 0x3221; break;
		case 0xf: Map5 = 0x2222; break;
	}

	if(doTrans == 0) Map5 = 0x2222;

	for (k = 0; k < 4; k++) {
		 switch (k){
		   case 0: MP_BS_P = &MP_B_P->MP_A0; MP_B2S_P = &MP_B2_P->MP_A0; D_X = 0; D_Y = 0; break;
		   case 1: MP_BS_P = &MP_B_P->MP_A1; MP_B2S_P = &MP_B2_P->MP_A1; D_X = 1; D_Y = 0; break;
		   case 2: MP_BS_P = &MP_B_P->MP_A2; MP_B2S_P = &MP_B2_P->MP_A2; D_X = 0; D_Y = 1; break;
		   case 3: MP_BS_P = &MP_B_P->MP_A3; MP_B2S_P = &MP_B2_P->MP_A3; D_X = 1; D_Y = 1; break;
		 };
		 Get_thita_rate3(M_Mode, S_Id, MP.G_t_p[k].phi, MP.G_t_p[k].thita , MP_BS_P, MP_B2S_P, 0, k, i, j);

		 Trans_Temp =  ((3 - (Map5 >> ((3 - k)*4) & 0xf)) * 0x10) + 0x81;
		 Tran_P[k] = Trans_Temp;
	}
	Block_Idx[FPGA_ID]++;
	Block_Size[FPGA_ID]++;
}

int Input_Scale = 3;
/*
 * P_Mode:  0:Global 1:Plant 2:Pillar 3:3D-Model
 */
void Add_ST_Table_Proc(int M_Mode, int V_Blocks, int H_Blocks, struct ST_Header_Struct *ST_H, int P_Mode, int t_offset)
{
	int i, j, k;
	int Start_Idx;
	int C_Cnt, C_Line, L, Sensor_Sel, M_Cnt;
// Miller 20180925
	for (i = 0; i < V_Blocks; i++) {
		for (j = 0; j <  H_Blocks; j++) {
			C_Line = 0;  Sensor_Sel = 0; M_Cnt = 0;
			for (k = 0; k < 5; k++) {
				C_Cnt = 0;
				if (Map_Posi_Tmp[i  ][j  ].Transparent[k] == 1 ) C_Cnt++;
			  	if (Map_Posi_Tmp[i+1][j  ].Transparent[k] == 1 ) C_Cnt++;
			  	if (Map_Posi_Tmp[i  ][j+1].Transparent[k] == 1 ) C_Cnt++;
			  	if (Map_Posi_Tmp[i+1][j+1].Transparent[k] == 1 ) C_Cnt++;
			  	Map_Posi_Tmp[i][j].C_Cnt[k] = C_Cnt;
			  	if( (C_Cnt > 0) && (C_Cnt < 4) ) {
			  		C_Line = 1;
			  		if (C_Cnt > M_Cnt) {
			  			Sensor_Sel = k;
			  			M_Cnt = C_Cnt;
			  		}
			  	}
			  	if (C_Cnt == 4) { Sensor_Sel = k; M_Cnt = 4; }
			}
			Map_Posi_Tmp[i][j].Sensor_Sel = Sensor_Sel;
			Map_Posi_Tmp[i][j].C_Line = C_Line;
		}
	}

	int cnt_th = 3;
	switch(P_Mode) {
	default:
	case 0:  cnt_th = 3; break;		// Global
	case 1:							// Plant
	case 2:							// Pillar
	case 3:  cnt_th = 1; break;		// 3D-Model.
	}

	//Img
	Start_Idx = ST_H->Start_Idx[ST_H_Img];
	for (L = 0; L < 2; L++) {			//0:縫合線區塊  1:其它區塊
		for (i = 0; i < V_Blocks; i++) {
			for (j = 0; j <  H_Blocks; j++) {
				if (((Map_Posi_Tmp[i][j].C_Line == 1) && (L == 0)) || ((Map_Posi_Tmp[i][j].C_Line == 0) && (L == 1))) {
					for (k = 0; k < 5; k++) {
						if (Map_Posi_Tmp[i][j].C_Cnt[k] >= cnt_th && Map_Posi_Tmp[i][j].Sensor_Sel == k)
							Add_ST_One_Proc(M_Mode, Start_Idx, k, i, j, 0, P_Mode, t_offset);
					}
				}
			}
		}
	}
	ST_H->Sum[ST_H_Img][0] = Block_Size[0];
	ST_H->Sum[ST_H_Img][1] = Block_Size[1];
}

void Add_ST_Tran_Table_Proc(int M_Mode, int V_Blocks, int H_Blocks, struct ST_Header_Struct *ST_H, int P_Mode, int t_offset)
{
	int i, j, k;
	int Start_Idx;
	Start_Idx = ST_H->Start_Idx[ST_H_Img];
	for (i = 0; i < V_Blocks; i++) {
		for (j = 0; j <  H_Blocks; j++) {
			for (k = 0; k < 5; k++) {
			  if (Map_Posi_Tmp[i][j].C_Cnt[k] > 0 && Map_Posi_Tmp[i][j].C_Cnt[k] < 4 && Map_Posi_Tmp[i][j].Sensor_Sel != k) {
				  Add_ST_One_Proc(M_Mode, Start_Idx, k, i, j, 1, P_Mode, t_offset);
			  }
			}
		}
	}
	ST_H->Sum[ST_H_Trans][0] = Block_Size[0];
	ST_H->Sum[ST_H_Trans][1] = Block_Size[1];
}

// Miller 20171103


/*
 * Global球型旋轉
 */
void Map_3D_Rotate(float phi1, float thita1, unsigned short I_phi0, unsigned short I_thita0, unsigned short I_rotate0, unsigned short *I_phi35, unsigned short *I_thita35)
{
   unsigned short I_phi1,I_thita1;
   unsigned short I_phi2,I_thita2;
   unsigned short I_phi3,I_thita3;

   I_phi1   = (short)((0x4000 - (short)(phi1   * 0x8000 / pi) ) & 0xffff);
   I_thita1 = (short)(thita1 * 0x8000 / pi);
   Trans_ZY(I_phi1, I_thita1 + I_rotate0, &I_phi2, &I_thita2);
   Trans_ZY(I_phi2, 0x4000 - I_thita2 - I_phi0, &I_phi3, &I_thita3);
   *I_thita35 = 0x4000 - I_thita3 - I_thita0;
   *I_phi35   = 0x4000 - I_phi3;
};

/*
 * 繞X軸旋轉 (X:Sensor1/3 方向)
 */
void X_Rotate(unsigned short I_rotate0, float rx1, float ry1, float rz1, float *rx2, float *ry2, float *rz2) {
	float rotate = I_rotate0 * pi/ 32768.0;
	unsigned short rotate_tmp = I_rotate0 & 0xFFFF;
	if(rotate_tmp == 0 || rotate_tmp == 0x8000) {				// 0 / 180
		*rx2 = rx1;
		*ry2 = ry1;
		*rz2 = rz1;
	}
	else if(rotate_tmp == 0x4000 || rotate_tmp == 0xC000) {		// 90 / 270
		*rx2 = rx1;
		*ry2 = -rz1;
		*rz2 = ry1;
	}
	else {
		*rx2 = rx1;
		*ry2 = ry1*cos(rotate) - rz1*sin(rotate);
		*rz2 = ry1*sin(rotate) + rz1*cos(rotate);
	}
}
/*
 * 繞Y軸旋轉 (Y:Sensor2/4 方向)
 */
void Y_Rotate(unsigned short I_phi0, float rx1, float ry1, float rz1, float *rx2, float *ry2, float *rz2) {
	float phi = I_phi0 * pi/ 32768.0;
	unsigned short phi_tmp = I_phi0 & 0xFFFF;
	if(phi_tmp == 0 || phi_tmp == 0x8000) {					// 0 / 180
		*rx2 = rx1;
		*ry2 = ry1;
		*rz2 = rz1;
	}
	else if(phi_tmp == 0x4000 || phi_tmp == 0xC000) {		// 90 / 270
		*rx2 = rz1;
		*ry2 = ry1;
		*rz2 = -rx1;
	}
	else {
		*rx2 =  rx1*cos(phi) + rz1*sin(phi);
		*ry2 =  ry1;
		*rz2 = -rx1*sin(phi) + rz1*cos(phi);
	}
}
/*
 * 繞Z軸旋轉 (Z:Sensor0 方向)
 */
void Z_Rotate(unsigned short I_thita0, float rx1, float ry1, float rz1, float *rx2, float *ry2, float *rz2) {
	float thita = I_thita0 * pi/ 32768.0;
	unsigned short thita_tmp = I_thita0 & 0xFFFF;
	if(thita_tmp == 0 || thita_tmp == 0x8000) {					// 0 / 180
		*rx2 = rx1;
		*ry2 = ry1;
		*rz2 = rz1;
	}
	else if(thita_tmp == 0x4000 || thita_tmp == 0xC000) {		// 90 / 270
		*rx2 = -ry1;
		*ry2 = rx1;
		*rz2 = rz1;
	}
	else {
		*rx2 = rx1*cos(thita) - ry1*sin(thita);
		*ry2 = rx1*sin(thita) + ry1*cos(thita);
		*rz2 = rz1;
	}
}
void Pillar_Rotate(unsigned short I_phi1, unsigned short I_thita1, unsigned short I_phi0, unsigned short I_thita0, unsigned short I_rotate0,
		unsigned short *I_phi36, unsigned short *I_thita36) {
	float rx1,ry1,rz1;
	float rx2,ry2,rz2;
	float rx3,ry3,rz3;
	float rx4,ry4,rz4;
	float Temp_thita;
	float Temp_phi;
	float rotate;

	rotate = (float)I_rotate0 * pi/ 32768.0;
	Temp_phi   = pi2 - (float)I_phi1   * pi/ 32768.0;
	Temp_thita = (float)I_thita1 * pi/ 32768.0;
	rx1 = sin(Temp_phi) * cos(Temp_thita);       // sin(temp_r) *
	ry1 = sin(Temp_phi) * sin(Temp_thita);       // sin(temp_r) *
	rz1 = sin(pi2 - Temp_phi);

	X_Rotate(I_rotate0, rx1, ry1, rz1, &rx2, &ry2, &rz2);
	Y_Rotate(   I_phi0, rx2, ry2, rz2, &rx3, &ry3, &rz3);
	Z_Rotate( I_thita0, rx3, ry3, rz3, &rx4, &ry4, &rz4);

	*I_thita36 = (short)(atan2(ry4,rx4) * 0x8000 / pi);
	*I_phi36   = (short)(asin(rz4)      * 0x8000 / pi);
}

float Rate[2] = {RateDefault(0), RateDefault(1)};
float LensRateTable[2][16] = {
		{  0.003,  0.007,  0.005,  0.004,
		 0.003,  0.002,  0.000, -0.003,
		-0.006, -0.008, -0.009, -0.011,
		  -0.012, -0.013, -0.014, -0.015  },

		{   0.020,   0.016,   0.010,   0.005,
		    0.000,  -0.003,  -0.005,  -0.005,
		   -0.001,   0.003,   0.004,   0.004,
		    0.004,   0.004,  -0.012,  -0.018  }
};

void setSensorLensRate(int value)
{
	Rate[LensCode] = (float)value / 1000.0;
}

void getSensorLensRate(int *val)
{
	*val = (int)(Rate[LensCode] * 1000);
}

void setLensRateTable(int idx, int value)
{
	int i, j;
	if(idx > 15 || idx < 0 || value > 500 || value < -500) return;
	LensRateTable[LensCode][idx] = (float)value / 1000.0;
}

void getLensRateTable(int *val)
{
	int i;
	for(i = 0; i < 16; i++)
		*(val+i) = (int)(LensRateTable[LensCode][i] * 1000);
}

#ifdef ANDROID_CODE
int WriteLensRateTable() {
    int size;
    FILE *file = NULL;
    int fp2fd = 0;

    size = sizeof(LensRateTable[LensCode]);
    file = fopen("/mnt/sdcard/US360/Test/LensRateTable.bin", "wb");
    if(file == NULL) {
        return -1;
    }
    fwrite(&LensRateTable[LensCode][0], size, 1, file);

    fflush(file);
    fp2fd = fileno(file);
    fsync(fp2fd);

    //if(file)
    {
        fclose(file);
        close(fp2fd);
        file = NULL;
    }

    return 0;
}
#endif

#ifdef ANDROID_CODE
int ReadLensRateTable(void)
{
	int i, j;
    FILE *fp;
    int size;
    struct stat stFileInfo;

    size = sizeof(LensRateTable[LensCode]);

    int intStat;
    intStat = stat("/mnt/sdcard/US360/Test/LensRateTable.bin", &stFileInfo);
    if(intStat != 0) {
        db_error("LensRateTable.bin not exist!\n");
        return -1;
    }
    else if(size != stFileInfo.st_size) {
        db_error("LensRateTable.bin size not same!\n");
        return -2;
    }

    fp = fopen("/mnt/sdcard/US360/Test/LensRateTable.bin", "rb");
    if(fp == NULL) {
        db_error("Read LensRateTable.bin err!\n");
        return -3;
    }
    //fread(&LensRateTable_tmp[0], size, 1, fp);
    fread(&LensRateTable[LensCode][0], size, 1, fp);

/*	for(i = 8; i <= 15; i++) {
  		LensRateTable[LensCode][i] = 0.0;
		for(j = 8; j <= i; j++) {
   			LensRateTable[LensCode][i] += LensRateTable_tmp[j];
		}
	}
	for(i = 7; i >= 0; i--) {
   		LensRateTable[LensCode][i] = 0.0;
		for(j = 7; j >= i; j--) {
   			LensRateTable[LensCode][i] += LensRateTable_tmp[j];
		}
	}*/

    if(fp) fclose(fp);
    return 0;
}
#endif
void Make_Lens_Rate_Line_Table_Proc( void)
{
  int i;
  float temp_r3;
  float Rate_tmp, Rate_tmp2; // = 0.50; //0.52;
  float Delta_Rate;
  int idx, step, tmp;
  float r1, r2;

  if(LensCode == 0) {
	  Rate_tmp = Rate[LensCode];
	  step = Lens_Rate_Line_MAX >> 5;	// Lens_Rate_Line_MAX / 2 / 16
	  tmp = Lens_Rate_Line_MAX >> 1;
	  for (i = 0; i < Lens_Rate_Line_MAX; i++) {
		if(i < tmp) Rate_tmp2 = 0.0;
		else {
			idx = ( (i-tmp) / step);
			if(idx >= 15)
				Rate_tmp2 = LensRateTable[LensCode][15];
			else {
				r1 = (float)( (i-tmp) % step) / (float)step;
				r2 = (1.0 - r1);
				Rate_tmp2 = (LensRateTable[LensCode][idx] * r2 + LensRateTable[LensCode][idx+1] * r1);
			}
		}

		temp_r3 = (float)i * pi2 / (float)Lens_Rate_Line_MAX;
		Delta_Rate = (sin(temp_r3) - sin(0.5 * pi2)) * (float)i / (float)Lens_Rate_Line_MAX;
		Lens_Rate_Line[i] = ( (Rate_tmp2 + Rate_tmp * Delta_Rate  + 1.0) * (temp_r3 / pi2)) * 16.0 * ( (float)Adj_Sensor_Command[0].Zoom_X / 2.0) ; // + (1.0 -  Rate_tmp)
	  }
  }
  else {
	  Rate_tmp = Rate[LensCode];
	  step = Lens_Rate_Line_MAX >> 4;	// Lens_Rate_Line_MAX / 2 / 16
	  //tmp = Lens_Rate_Line_MAX >> 1;
	  for (i = 0; i < Lens_Rate_Line_MAX; i++) {
		//if(i < tmp) Rate_tmp2 = 0.0;
		//else {
			idx = (i / step);
			if(idx >= 15)
				Rate_tmp2 = LensRateTable[LensCode][15];
			else {
				r1 = (float)(i % step) / (float)step;
				r2 = (1.0 - r1);
				Rate_tmp2 = (LensRateTable[LensCode][idx] * r2 + LensRateTable[LensCode][idx+1] * r1);
			}
		//}

		temp_r3 = (float)i * pi2 / (float)Lens_Rate_Line_MAX;
		Delta_Rate = (sin(temp_r3) - sin(0.5 * pi2)) * (float)i / (float)Lens_Rate_Line_MAX;
		Lens_Rate_Line[i] = ( (Rate_tmp2 + Rate_tmp * Delta_Rate  + 1.0) * (temp_r3 / pi2)) * 16.0 * ( (float)Adj_Sensor_Command[0].Zoom_X / 2.0) ; // + (1.0 -  Rate_tmp)
	  }
  }
}

/*
 * 做Sensor1 / Sensor3切斜角
 */
void Do_Map_Posi_Oblique(int M_Mode, int CP_Mode)
{
	int i, j;
	int T_Size_X, T_Size_Y;
	int delta_Phi;
	int Phi_P0, Phi_P1;
	int Phi_P0_Tmp, Phi_P1_Tmp;
	int Thita_P0, Thita_P1, Thita_P2, Thita_P3;
	int Degree_Offset=0;
	int Transparent[5] = {1,0,0,0,0};
	int Start_P, End_P;

	T_Size_X = A_L_I3_Header[M_Mode].H_Blocks;
	T_Size_Y = A_L_I3_Header[M_Mode].V_Blocks;

	Phi_P0 = A_L_I3_Header[M_Mode].Phi_P[0];
	Phi_P1 = A_L_I3_Header[M_Mode].Phi_P[1];

	Thita_P0 = A_L_I3_Header[M_Mode].Thita_P[0];
	Thita_P1 = A_L_I3_Header[M_Mode].Thita_P[1];
	Thita_P2 = A_L_I3_Header[M_Mode].Thita_P[2];
	Thita_P3 = A_L_I3_Header[M_Mode].Thita_P[3];

	Degree_Offset = Get_Degree_Offset(M_Mode, CP_Mode);		//12K 4K Block為奇數, 所以需做偏移

	if(CP_Mode == 1) {
		Phi_P0_Tmp = T_Size_Y - Phi_P0 + 1;
		Phi_P1_Tmp = T_Size_Y - Phi_P1;
		Start_P = Phi_P1_Tmp;
		End_P   = Phi_P0_Tmp;
	}
	else {
		Phi_P0_Tmp = Phi_P0;
		Phi_P1_Tmp = Phi_P1;
		Start_P = Phi_P0_Tmp;
		End_P   = Phi_P1_Tmp;
	}
	for (i = Start_P; i < End_P; i++) {
		if(CP_Mode == 1) delta_Phi = i - Phi_P1_Tmp;
		else			 delta_Phi = Phi_P1_Tmp - i;
		for (j = 0; j <= delta_Phi; j++) {		//向內幾個Block
			memcpy(&Map_Posi_Tmp[i][Thita_P0 - j - Degree_Offset].Transparent[0], &Transparent[0], sizeof(Transparent) );
			memcpy(&Map_Posi_Tmp[i][Thita_P1 + j - Degree_Offset].Transparent[0], &Transparent[0], sizeof(Transparent) );
			memcpy(&Map_Posi_Tmp[i][Thita_P2 - j - Degree_Offset].Transparent[0], &Transparent[0], sizeof(Transparent) );
			memcpy(&Map_Posi_Tmp[i][Thita_P3 + j - Degree_Offset].Transparent[0], &Transparent[0], sizeof(Transparent) );
		}

		Map_Posi_Tmp[i][Thita_P0 - delta_Phi - Degree_Offset].Transparent[3] = 1;
		Map_Posi_Tmp[i][Thita_P1 + delta_Phi - Degree_Offset].Transparent[1] = 1;
		Map_Posi_Tmp[i][Thita_P2 - delta_Phi - Degree_Offset].Transparent[1] = 1;
		Map_Posi_Tmp[i][Thita_P3 + delta_Phi - Degree_Offset].Transparent[3] = 1;
	}
}

/*
 * 做Sensor2 / Sensor4 縫合線凹陷
 */
void Do_Map_Posi_Depression(int M_Mode, int CP_Mode)
{
	int i, j, k, k2;
	int T_Size_X, T_Size_Y;
	int delta_Phi, delta_Thita;
	int Phi_P0, Phi_P1;
	int Phi_P0_Tmp, Phi_P1_Tmp;
	int Thita_P0, Thita_P1, Thita_P2, Thita_P3;
	int Thita_P0_Tmp, Thita_P1_Tmp;
	int D_Thita;
	int Degree_Offset=0;
	int Transparent[5] = {1,0,0,0,0};
	int s1, s2;
    int cp_mode = getCameraPositionMode();

	T_Size_X = A_L_I3_Header[M_Mode].H_Blocks;
	T_Size_Y = A_L_I3_Header[M_Mode].V_Blocks;

	Phi_P0 = A_L_I3_Header[M_Mode].Phi_P[0];
	Phi_P1 = A_L_I3_Header[M_Mode].Phi_P[1];

	Thita_P0 = A_L_I3_Header[M_Mode].Thita_P[0];
	Thita_P1 = A_L_I3_Header[M_Mode].Thita_P[1];
	Thita_P2 = A_L_I3_Header[M_Mode].Thita_P[2];
	Thita_P3 = A_L_I3_Header[M_Mode].Thita_P[3];

	Degree_Offset = Get_Degree_Offset(M_Mode, cp_mode);		//12K 4K Block為奇數, 所以需做偏移

	if(cp_mode == 1) {		//倒擺

		if(M_Mode < 4) {		//錄影3K縫合效能不夠
			//做Sensor2 / Sensor4 縫合線凹陷
			Phi_P1_Tmp = T_Size_Y - Phi_P1;
			for(i = 0; i < 2; i++) {		//0:Sensor4縫合線	1:Sensor2縫合線
				if(i == 0) { Thita_P0_Tmp = Thita_P0 + 1 - Degree_Offset; Thita_P1_Tmp = Thita_P1 + 1 - Degree_Offset; }
				else       { Thita_P0_Tmp = Thita_P2 + 1 - Degree_Offset; Thita_P1_Tmp = Thita_P3 + 1 - Degree_Offset; }
				delta_Thita = (Thita_P1_Tmp - Thita_P0_Tmp) / 3;	//分3段(左右凹陷, 使用Sensor0畫面)
				for(k = 0; k < 2; k++) {		//Phi做2層
					for(j = Thita_P0_Tmp; j < Thita_P1_Tmp; j++) {
						D_Thita = (j-Thita_P0_Tmp) / delta_Thita;
						switch(D_Thita) {
						case 0:
						case 2: memcpy(&Map_Posi_Tmp[Phi_P1_Tmp-k][j].Transparent[0], &Transparent[0], sizeof(Transparent) );
								break;			//左右2段
						case 1: break;			//中間
						}

						if(i == 0) s1 = 4;
						else       s1 = 2;
						s2 = (s1 + 1) & 0x3;
						if(k == 0) {				//Phi_P1
							if(j == Thita_P1_Tmp - 1) {
								Map_Posi_Tmp[Phi_P1_Tmp-k][j].Transparent[s1] = 1;
								Map_Posi_Tmp[Phi_P1_Tmp-k][j].Transparent[s2] = 1;		//S3 / S1
							}
							else if(j == (Thita_P0_Tmp + delta_Thita*2) )
								Map_Posi_Tmp[Phi_P1_Tmp-k][j].Transparent[s1] = 1;
						}
						else if(k == 1) {			//Phi_P1-1
							if(j == Thita_P1_Tmp - 1) {
								Map_Posi_Tmp[Phi_P1_Tmp-k][j].Transparent[0] = 0;					//S0
								Map_Posi_Tmp[Phi_P1_Tmp-k][j].Transparent[s1] = 1;
							}
							else if(j <= (Thita_P0_Tmp + delta_Thita - 1) )
								Map_Posi_Tmp[Phi_P1_Tmp-k][j].Transparent[s1] = 1;
							else if(j >= (Thita_P0_Tmp + delta_Thita*2) )
								Map_Posi_Tmp[Phi_P1_Tmp-k][j].Transparent[s1] = 1;
						}
					}
				}
			}
		}
	}
	else {								//正擺

		if(M_Mode < 4) {		//錄影3K縫合效能不夠
			//做Sensor2 / Sensor4 縫合線凹陷
			Phi_P1_Tmp = Phi_P1;
			for(i = 0; i < 2; i++) {		//0:Sensor2縫合線	1:Sensor4縫合線
				if(i == 0) { Thita_P0_Tmp = Thita_P0; Thita_P1_Tmp = Thita_P1; }
				else       { Thita_P0_Tmp = Thita_P2; Thita_P1_Tmp = Thita_P3; }
				delta_Thita = (Thita_P1_Tmp - Thita_P0_Tmp) / 3;	//分3段(左右凹陷, 使用Sensor0畫面)
				for(k = 0; k < 2; k++) {
					for(j = Thita_P0_Tmp; j < Thita_P1_Tmp; j++) {
						D_Thita = (j-Thita_P0_Tmp) / delta_Thita;
						switch(D_Thita) {
						case 0:
						case 2: memcpy(&Map_Posi_Tmp[Phi_P1_Tmp+k][j].Transparent[0], &Transparent[0], sizeof(Transparent) );
							    break;			//左右2段
						case 1: break;			//中間
						}

						if(i == 0) s1 = 2;
						else       s1 = 4;
						s2 = (s1 + 1) & 0x3;
						if(k == 0) {				//Phi_P1
							if(j == Thita_P0_Tmp) {
								Map_Posi_Tmp[Phi_P1_Tmp+k][j].Transparent[s1] = 1;
								Map_Posi_Tmp[Phi_P1_Tmp+k][j].Transparent[s2] = 1;
							}
							else if(j == (Thita_P0_Tmp + delta_Thita*2) )
								Map_Posi_Tmp[Phi_P1_Tmp+k][j].Transparent[s1] = 1;
						}
						else if(k == 1) {			//Phi_P1+1
							if(j == Thita_P0_Tmp) {
								Map_Posi_Tmp[Phi_P1_Tmp+k][j].Transparent[0] = 0;
								Map_Posi_Tmp[Phi_P1_Tmp+k][j].Transparent[s1] = 1;
							}
							else if(j <= (Thita_P0_Tmp + delta_Thita - 1) )
								Map_Posi_Tmp[Phi_P1_Tmp+k][j].Transparent[s1] = 1;
							else if(j >= (Thita_P0_Tmp + delta_Thita*2) )
								Map_Posi_Tmp[Phi_P1_Tmp+k][j].Transparent[s1] = 1;
						}
					}
				}
			}
		}
	}
}

void Write_Map_Proc(int M_Mode)
{
	unsigned short thita1;
	short phi1;
	int i, j, k;
	int i2, j2;
	int  T_Size_X, T_Size_Y, Binn;
    int cp_mode = getCameraPositionMode();

	T_Size_X = A_L_I3_Header[M_Mode].H_Blocks;
	T_Size_Y = A_L_I3_Header[M_Mode].V_Blocks;
	Binn     = A_L_I3_Header[M_Mode].Binn;

	for (i =  0; i <= T_Size_Y; i++) {
		for (j = 0 ; j <=T_Size_X; j++) {
			if(cp_mode == 1) { i2 = T_Size_Y - i; j2 = T_Size_X - j; }
			else                        { i2 = i;            j2 = j;            }

			thita1 = (((j2) * 0x8000 / (T_Size_X / 2) )  + 0x8000) & 0xFFFF;
			phi1   = ((T_Size_Y / 2) - i2) * 0x4000 / (T_Size_Y / 2);

			if (phi1 > -16384) Map_Posi_Tmp[i][j].I_phi   = phi1;
			else               Map_Posi_Tmp[i][j].I_phi   = -16383;

			Map_Posi_Tmp[i][j].I_thita = thita1;

			Map_To_Sensor_Trans(M_Mode, &Map_Posi_Tmp[i][j]);
		}
	}

	Do_Map_Posi_Oblique(M_Mode, cp_mode);			//斜角
	if(M_Mode < 4)		//錄影3K縫合效能不夠
		Do_Map_Posi_Depression(M_Mode, cp_mode);		//凹陷

	Block_Idx[0] = 0; Block_Idx[1] = 0;
	Block_Size[0] = 0; Block_Size[1] = 0;
	Add_ST_Table_Proc(M_Mode, A_L_I3_Header[M_Mode].V_Blocks, A_L_I3_Header[M_Mode].H_Blocks, &ST_Header[M_Mode], 0, 0);

	Block_Size[0] = 0; Block_Size[1] = 0;
	Add_ST_Tran_Table_Proc(M_Mode, A_L_I3_Header[M_Mode].V_Blocks, A_L_I3_Header[M_Mode].H_Blocks, &ST_Header[M_Mode], 0, 0);
}

/*
 * 平面轉Global球型座標
 */
void Plant_To_Global(float T_X, float T_Y, float T_Z, float *phi1, float *thita1)
{
	float Length;
	if(T_X == 0 && T_Y == 0) *thita1 = 0;
	else					 *thita1 = atan2(T_X, T_Y); // + thita0;
	Length  = sqrt(T_X * T_X + T_Y * T_Y);
	*phi1   = pi2 - atan2(Length, T_Z); // + phi0;
}

void Make_Plant_To_Global_Table(struct Out_Plant_Mode_Struct plane_s)
{
	int i, j;
	int i2, j2;
	float Size_X;
	float Size_Y;
	float C_X, C_Y;
	float T_X, T_Y;
	float T_Z;
	float tan_tmp;
	int wide;

	Size_X = (plane_s.TP.T_Size_X << 6);
	Size_Y = (plane_s.TP.T_Size_Y << 6);

	C_X = Size_X / 2;
	C_Y = Size_Y / 2;

	wide = plane_s.Wide;

	tan_tmp = tan(pi * wide / 18000 / 2);
	if(tan_tmp == 0) {
		//db_error("Make_Plant_To_Global_Table() tan_tmp == 0 err!\n");
		return;
	}
	T_Z = C_X / tan_tmp;

	for(i = 0; i <= Size_Y; i += 64) {
		for(j = 0; j <= Size_X; j += 64) {
			i2 = (i >> 6);
			j2 = (j >> 6);
			T_Y = i - C_Y;
			T_X = j - C_X;
	    	Plant_To_Global(T_X, T_Y, T_Z, &Plant_To_Global_Phi[i2][j2], &Plant_To_Global_Thita[i2][j2]);
	    }
	}
}

void Write_Plant_Proc(int M_Mode, int isInit, struct Out_Plant_Mode_Struct *plant_s)
{
	unsigned Size_X;
	unsigned Size_Y;
	unsigned short I_thita35, I_phi35;
	unsigned short I_rotate0, I_thita0, I_phi0;
	int i,j,k;
	float thita1, phi1;
	struct Out_Plant_Mode_Struct Mode_Plant_Command_C;

	memset(&Map_Posi_Tmp[0][0], 0, sizeof(Map_Posi_Tmp));

	Mode_Plant_Command_C = *plant_s;
	if(Make_Plant_To_Global_Table_Flag == 1) {
		Make_Plant_To_Global_Table_Flag = 0;
		Make_Plant_To_Global_Table(Mode_Plant_Command_C);
	}

	Size_X = Mode_Plant_Command_C.TP.T_Size_X;
	Size_Y = Mode_Plant_Command_C.TP.T_Size_Y;
	I_thita0  = Mode_Plant_Command_C.Pan    * 0x8000 /18000;			//目標角度
	I_phi0    = Mode_Plant_Command_C.Tilt   * 0x8000 /18000;			//目標角度
	I_rotate0 = Mode_Plant_Command_C.Rotate * 0x8000 /18000;

	for (i = 0; i <= Size_Y; i++) {
	    for (j = 0; j <= Size_X; j++) {
	    	phi1   = Plant_To_Global_Phi[i][j];		//平面在球面上的座標
	    	thita1 = Plant_To_Global_Thita[i][j];
	    	Map_3D_Rotate(phi1, thita1, I_phi0, I_thita0, I_rotate0, &I_phi35, &I_thita35);		//目標角度上的平面在球面上的座標

	    	Map_Posi_Tmp[i][j].I_phi   = I_phi35;
	    	Map_Posi_Tmp[i][j].I_thita = I_thita35;
	    	Map_To_Sensor_Trans(M_Mode, &Map_Posi_Tmp[i][j]);
	    }
	}

	Block_Idx[0] = 0; Block_Idx[1] = 0;
	Block_Size[0] = 0; Block_Size[1] = 0;
	Add_ST_Table_Proc(M_Mode, Size_Y, Size_X, &ST_3DModel_Header[ST_3DModel_Idx], 1, 0);
}

void Write_Pillar_Proc(int M_Mode, int type, struct Out_Pillar_Mode_Struct *pillar_s)
{
	unsigned short thita1;
	short phi1;
	float Start_phi;
	float Size_X; float Size_Y;
	float C_Y;
	float T_Y;
	unsigned short L_Y;
	int i, j;
	int i2, j2;
	int i1, j1;
    int cp_mode = getCameraPositionMode();
	struct Out_Pillar_Mode_Struct Mode_Pillar_Command_C;

	memset(&Map_Posi_Tmp[0][0], 0, sizeof(Map_Posi_Tmp));

	Mode_Pillar_Command_C = *pillar_s;
	Size_X = Mode_Pillar_Command_C.TP.T_Size_X;
	Size_Y = Mode_Pillar_Command_C.TP.T_Size_Y;
	Start_phi = Mode_Pillar_Command_C.Start_phi * 0x8000 /18000;
	L_Y = abs(Mode_Pillar_Command_C.Start_thita - Mode_Pillar_Command_C.Stop_thita) * (Size_Y / Size_X ) * 0x8000 / 18000;

	for (i =  0; i <= Size_Y; i++) {
		for (j = 0 ; j <= Size_X; j++) {
			if(type == 1) { i1 = i; j1 = Size_X - j; }
			else 		  { i1 = i; j1 = j; }

			if(cp_mode == 1) {
				i2 = Size_Y - i1;
				j2 = Size_X - j1;
			}
			else {
				i2 = i1;
				j2 = j1;
			}

			thita1 = (short)( (((( Size_X - j2) * Mode_Pillar_Command_C.Start_thita) +  (j2  * Mode_Pillar_Command_C.Stop_thita)) / Size_X) * 0x8000 / 18000);
			phi1   = (short)( Start_phi   - L_Y * i2 / Size_Y);

			Map_Posi_Tmp[i][j].I_phi   = phi1;
			Map_Posi_Tmp[i][j].I_thita = thita1;
			Map_To_Sensor_Trans(M_Mode, &Map_Posi_Tmp[i][j]);
		}
	}

	Block_Idx[0] = 0; Block_Idx[1] = 0;
	Block_Size[0] = 0; Block_Size[1] = 0;
	Add_ST_Table_Proc(M_Mode, Size_Y, Size_X, &ST_3DModel_Header[ST_3DModel_Idx], 2, 0);
}

void Angle_Adjust_3DModel(int *pitch, int *roll, int *yaw)
{
#ifdef ANDROID_CODE
//tmp	*pitch = getBmg160_pitch();
//tmp	*roll  = getBmg160_roll();
//tmp	*yaw   = getBmg160_yaw();
#else
	*pitch = 0;
	*roll = 0;
	*yaw = 0;
#endif
}

void Write_3D_Model_Proc(int M_Mode, int isInit, struct Out_Plant_Mode_Struct *plant_s,
		struct Out_Pillar_Mode_Struct *pillar_s)
{
	unsigned Size_X;
	unsigned Size_Y;
	unsigned short I_thita35, I_phi35;
	unsigned short I_rotate0, I_thita0, I_phi0;
	unsigned short L_Y;
	int i,j,k;
	int i2, j2;
	float Start_phi;
	float thita1, phi1;
	struct Out_Plant_Mode_Struct  Mode_Plant_Command_C;
	struct Out_Pillar_Mode_Struct Mode_Pillar_Command_C;
	int pitch=0, roll=0, yaw=0;
	int t_offset=0;
	unsigned short thita2;
	short phi2;
    int cp_mode = getCameraPositionMode();

	if(Angle_3DModel_Init == 1) {
		Angle_3DModel_Init = 0;
		*plant_s  = Out_Mode_Plant_3dModel_Default;
		*pillar_s = Out_Mode_Pillar_3dModel_Default;
	}

	Angle_Adjust_3DModel(&pitch, &roll, &yaw);

	/*
	 * 	Plant 1152*1152
	 */
	plant_s->Tilt   = Out_Mode_Plant_3dModel_Default.Tilt   - pitch;
	//plant_s->Rotate = Out_Mode_Plant_3dModel_Default.Rotate + roll;
	plant_s->Pan    = Out_Mode_Plant_3dModel_Default.Pan    + yaw;
	Mode_Plant_Command_C = *plant_s;		//Out_Mode_Plant_3dModel;
	if(Make_Plant_To_Global_Table_Flag == 1) {
		Make_Plant_To_Global_Table_Flag = 0;
		Make_Plant_To_Global_Table(Mode_Plant_Command_C);
	}
	Size_X = Mode_Plant_Command_C.TP.T_Size_X;
	Size_Y = Mode_Plant_Command_C.TP.T_Size_Y;
	I_thita0  = (Mode_Plant_Command_C.Pan    << 15) /18000;			//目標角度
	I_phi0    = (Mode_Plant_Command_C.Tilt   << 15) /18000;			//目標角度
	I_rotate0 = (Mode_Plant_Command_C.Rotate << 15) /18000;
	memset(&Map_Posi_Tmp[0][0], 0, sizeof(Map_Posi_Tmp));
	for (i = 0; i <= Size_Y; i++) {
	    for (j = 0; j <= Size_X; j++) {
	    	phi1   = Plant_To_Global_Phi[i][j];		//平面在球面上的座標
	    	thita1 = Plant_To_Global_Thita[i][j];
	    	Map_3D_Rotate(phi1, thita1, I_phi0, I_thita0, I_rotate0, &I_phi35, &I_thita35);		//目標角度上的平面在球面上的座標

	    	Map_Posi_Tmp[i][j].I_phi   = I_phi35;
	    	Map_Posi_Tmp[i][j].I_thita = I_thita35;
	    	Map_To_Sensor_Trans(M_Mode, &Map_Posi_Tmp[i][j]);
	    }
	}
	Block_Idx[0] = 0; Block_Idx[1] = 0;
	Block_Size[0] = 0; Block_Size[1] = 0;
	t_offset = 0;
	Add_ST_Table_Proc(M_Mode, Size_Y, Size_X, &ST_3DModel_Header[ST_3DModel_Idx], 3, t_offset);

	/*
	 * 	Pillar 3840*64
	 */
	Mode_Pillar_Command_C = *pillar_s;
	Mode_Pillar_Command_C.Adj_yaw   = -yaw;
	Mode_Pillar_Command_C.Adj_pitch = -pitch;
	Mode_Pillar_Command_C.Adj_roll  = roll;
	I_thita0  = (Mode_Pillar_Command_C.Adj_yaw   << 15) /18000;
	I_phi0    = (Mode_Pillar_Command_C.Adj_pitch << 15) /18000;
	I_rotate0 = (Mode_Pillar_Command_C.Adj_roll  << 15) /18000;
	Size_X = Mode_Pillar_Command_C.TP.T_Size_X;
	Size_Y = Mode_Pillar_Command_C.TP.T_Size_Y;
	Start_phi = (Mode_Pillar_Command_C.Start_phi << 15) /18000;
	L_Y = abs(Mode_Pillar_Command_C.Start_thita - Mode_Pillar_Command_C.Stop_thita) * ((float)Size_Y / (float)Size_X ) * 0x8000 / 18000;
	memset(&Map_Posi_Tmp[0][0], 0, sizeof(Map_Posi_Tmp));
	for (i = 0; i <= Size_Y; i++) {
		for (j = 0 ; j <= Size_X; j++) {
			if(cp_mode == 1) {
				i2 = Size_Y - i;
				j2 = Size_X - j;
			}
			else {
				i2 = i;
				j2 = j;
			}
			thita2 = (short)( ((( ((Size_X - j2) * Mode_Pillar_Command_C.Start_thita) +  (j2 * Mode_Pillar_Command_C.Stop_thita) ) / Size_X) << 15) / 18000);
			phi2   = (short)( Start_phi   - L_Y * i2 / Size_Y);

			Pillar_Rotate(phi2, thita2, I_phi0, I_thita0, I_rotate0, &I_phi35, &I_thita35);

			Map_Posi_Tmp[i][j].I_phi   = I_phi35;
			Map_Posi_Tmp[i][j].I_thita = I_thita35;
			Map_To_Sensor_Trans(M_Mode, &Map_Posi_Tmp[i][j]);
		}
	}
	t_offset = ((S2_RES_3D1K_HEIGHT+64) << 15);
	Add_ST_Table_Proc(M_Mode, Size_Y, Size_X, &ST_3DModel_Header[ST_3DModel_Idx], 3, t_offset);
}

// for test machine  miller 20160812
void Sensor2Global(int S_Id, struct XY_Posi * XY_P, int Global_degree)
{
  int i; float S_Len;
  int S_X; int S_Y;
  float temp_r5, thita5;
  int Sensor_X, Sensor_Y;
  unsigned short I_phi5,I_thita5;
  short S_Rotate_R1, S_Rotate_R2, S_Rotate_R3;

  Sensor_X = Adj_Sensor_Command[S_Id].S[4].X + Sensor_C_X_Base;
  Sensor_Y = Adj_Sensor_Command[S_Id].S[4].Y + Sensor_C_Y_Base;

  S_X  = XY_P->S[S_Id].X *4 - Sensor_X * 4;
  S_Y  = XY_P->S[S_Id].Y *4 - Sensor_Y * 4;


  S_Len = Lens_Rate_Line_MAX;
  temp_r5 = sqrt(S_X * S_X  + S_Y * S_Y) * 4;
  thita5  =  atan2(S_Y*-1, S_X) * 32768 / pi;
  for (i = 0; i < Lens_Rate_Line_MAX; i++) {
    if (temp_r5 < Lens_Rate_Line[i]) {
       S_Len = i;
       break;
	}
  }


  unsigned short T_phi1; unsigned short T_thita1; unsigned short T_phi2; unsigned short T_thita2;
  unsigned short T_phi3; unsigned short T_thita3; unsigned short T_phi4; unsigned short T_thita4;

   S_Rotate_R1  = Adj_Sensor_Command[S_Id].Rotate_R1 * 0x8000 / 18000;
   S_Rotate_R2  = Adj_Sensor_Command[S_Id].Rotate_R2 * 0x8000 / 18000;
   S_Rotate_R3  = Adj_Sensor_Command[S_Id].Rotate_R3 * 0x8000 / 18000;

  T_phi1   = S_Len * 2  - Global_degree;
  T_thita1 = (short)(thita5 - S_Rotate_R3);

  Trans_YZ( T_phi1,  T_thita1, &T_phi2, &T_thita2 );
  Trans_YZ( T_phi2,  T_thita2 - S_Rotate_R2, &T_phi3, &T_thita3 );

  XY_P->I_phi   = 0x4000 - T_phi3;
  XY_P->I_thita = T_thita3 - S_Rotate_R1;

}

struct XY_Posi  Test_XY1, Test_XY2;

void Calibration_Sensor_Center(int last)
{
  int i,j; int S_Len;
  int S_Id; int S_X; int S_Y;
  short          I_phi;
  unsigned short I_thita;
  struct Adjust_Line_I2_Struct   *p1;
  struct Test_Block_Table_Struct *p2;
  struct XY_Posi                 *p3a, *p3b;
  struct XY_Posi                 P3C;

  struct Adjust_Line_S2_Struct   *p4;
  int Sum, T_B;

  float D,H;
  double asin_tmp, rate;


  S_Id = 0; S_X = 100; S_Y = 200;

  Test_XY1.I_phi   = 0x2000;
  Test_XY1.I_thita = 0x2000;

  for(i=0; i<Adjust_Line_I2_MAX; i++){
	for(j=0; j<2; j++){
		p1 = &A_L_I2[i];
		p1->DP[0][j].Sensor_Sel         = p1->S_Id[j];
		p1->DP[0][j].S[p1->S_Id[j]].X   = (p1->XY_P[j][0] + p1->S_P2_XY[j][0]) * 3;
		p1->DP[0][j].S[p1->S_Id[j]].Y   = (p1->XY_P[j][1] + p1->S_P2_XY[j][1]) * 3;
	}
  }

  int Temp_Global_degree; float Lens2Cen;
  for (i = 0; i < Adjust_Line_I2_MAX; i++) {
    for (j = 0; j < 2; j++) {
      p1  = &A_L_I2[i];
      p3a = &p1->DP[0][j];
      S_Id = p3a->Sensor_Sel;
	  if (S_Id == 0) Lens2Cen = LENS2CEN0[LensCode];
      else           Lens2Cen = LENS2CEN1[LensCode];
	  if(j == 0)
		  Temp_Global_degree =  p1->S_t_p1.phi * (Lens2Cen / p1->Distance);
	  else
		  Temp_Global_degree =  p1->S_t_p2.phi * (Lens2Cen / p1->Distance);
	  Test_Global_degree =  Temp_Global_degree;
	  Sensor2Global(S_Id, p3a, Test_Global_degree);
    }
    p1->F_phi   = (p1->DP[0][0].I_phi   +  p1->DP[0][1].I_phi) / 2;

    if ( (abs(p1->DP[0][0].I_thita - p1->DP[0][1].I_thita) & 0x7fff) < 0x4000)
      p1->F_thita = (p1->DP[0][0].I_thita +  p1->DP[0][1].I_thita) / 2;
    else
      p1->F_thita = ((p1->DP[0][0].I_thita +  p1->DP[0][1].I_thita + 0x10000) & 0x1ffff) / 2;


    H = p1->Height - S2_Height;
    D = p1->Distance ;

	rate = H/D;
	if (rate > 1) rate = 1;
	if (rate < -1) rate = -1;
	asin_tmp = asin(rate);
	p1->T_phi = 0x8000 * asin_tmp / pi;
	//p1->T_phi = 0x8000 * asin(H/D) / pi;
    p1->D_phi = p1->T_phi - p1->F_phi;

  }

  for (i = 0; i < 5; i++) {
      A_L_S2[i].D_X   = 0;
      A_L_S2[i].D_Y   = 0;
      A_L_S2[i].Sum    = 0;
  }

  int A_L_S_Idx;

  for (i = 0; i < Adjust_Line_I2_MAX; i++) {
    for (j = 0; j < 2; j++) {
      p1   = &A_L_I2[i];
      p3a  = &p1->DP[0][j];
      p3b  = &p1->DP[1][j];
      p3b->I_phi   = p1->T_phi;
      if (last == 0) p3b->I_phi   = p1->T_phi;
      else           p3b->I_phi   = p1->F_phi;

      p3b->I_thita = p1->F_thita;

      S_Id = p1->S_Id[j];
	  Temp_Global_degree = 65536.0 * LENS2CEN1[LensCode]  / p1->Distance / 8;


	  Global2Sensor(3, S_Id, p3b->I_phi, p3b->I_thita, Temp_Global_degree);
      p1->tp[1][j]   = PP_Temp.S_t_p[S_Id];
      p3b->S[S_Id]   = PP_Temp.S[S_Id];
      Global2Sensor(3, S_Id, p3a->I_phi, p3a->I_thita, Temp_Global_degree);
      p1->tp[0][j]   = PP_Temp.S_t_p[S_Id];

      A_L_S_Idx = p1->A_L_S_Idx[j];
      A_L_S2[S_Id].Source_Idx[A_L_S_Idx] = i;
      A_L_S2[S_Id].Top_Bottom[A_L_S_Idx] = j;
      A_L_S2[S_Id].Sum++;
    }
  }

  int D_X, D_Y;
  int D_phi , D_thita;
  int temp_thita;

  for (i = 0; i < 5; i++) {
    p4 = &A_L_S2[i];
    Sum = p4->Sum;
    D_X = 0;
    D_Y = 0;
    D_phi  = 0;
    D_thita  = 0;

    for (j = 0; j < Sum; j++) {

      p1   = &A_L_I2[p4->Source_Idx[j]];
      T_B  = p4->Top_Bottom[j];

      p3a  = &p1->DP[0][T_B];
      p3b  = &p1->DP[1][T_B];
      S_Id = p1->S_Id[T_B];

      D_X += (p3a->S[S_Id].X - p3b->S[S_Id].X);
      D_Y += (p3a->S[S_Id].Y - p3b->S[S_Id].Y);
      D_phi        += (p1->tp[0][T_B].phi   - p1->tp[1][T_B].phi );
      temp_thita    = (p1->tp[0][T_B].thita - p1->tp[1][T_B].thita );
      if (temp_thita < -32768) {
	D_thita  += (temp_thita + 0x10000);
      }
      else {
	if (temp_thita > 32768)
	  D_thita  += (temp_thita - 0x10000);
	else
	  D_thita  += temp_thita;
      }
      p4->P0_phi[j]   = p1->tp[0][T_B].phi;
      p4->P0_thita[j] = p1->tp[0][T_B].thita;
      p4->P1_phi[j]   = p1->tp[1][T_B].phi;
      p4->P1_thita[j] = p1->tp[1][T_B].thita;
	  p4->F_phi[j]    = p1->tp[0][T_B].phi   - p1->tp[1][T_B].phi;
      //p4->F_thita[j]  = p1->tp[0][T_B].thita - p1->tp[1][T_B].thita;
      p4->F_thita[j]  = (short)(p1->tp[0][T_B].thita - p1->tp[1][T_B].thita);	//解數值溢位問題
    }
    p4->D_X = D_X / Sum;
    p4->D_Y = D_Y / Sum;
    p4->D_phi    = D_phi    / Sum;
    p4->D_thita  = D_thita  / Sum;
  }

}

void Calibration_Check_Center(void)
{
  int i,j; int S_Len;
  int S_Id; int S_X; int S_Y;
  short          I_phi;
  unsigned short I_thita;
  struct Adjust_Line_I2_Struct   *p1;
  struct Test_Block_Table_Struct *p2;
  struct XY_Posi                 *p3a, *p3b;
  struct Adjust_Line_S2_Struct   *p4;
  int Sum, T_B;

  int Temp_Global_degree; float Lens2Cen;
  int A_L_S_Idx;

  for (i = 0; i < Adjust_Line_I2_MAX; i++) {
    for (j = 0; j < 2; j++) {
      p1   = &A_L_I2[i];
      p3a  = &p1->DP[0][j];
      p3b  = &p1->DP[1][j];
	  S_Id = p1->S_Id[j];
	  Temp_Global_degree =  65536.0 * LENS2CEN1[LensCode] / p1->Distance/8;

	  Global2Sensor(3,S_Id, p3b->I_phi, p3b->I_thita, Temp_Global_degree);
      p1->tp[1][j]   = PP_Temp.S_t_p[S_Id];

    }
  }

}

void Make_A_L_S2_Table(void)
{
  int i,j; int S_Len;
  int S_Id; int S_X; int S_Y;
  short          I_phi;
  unsigned short I_thita;
  struct Adjust_Line_I2_Struct   *p1;

  int Sum, T_B;


  S_Id = 0; S_X = 100; S_Y = 200;

  Test_XY1.I_phi   = 0x2000;
  Test_XY1.I_thita = 0x2000;


  for (i = 0; i < 5; i++) {
      A_L_S2[i].Sum    = 0;
  }

  int A_L_S_Idx;

  for (i = 0; i < Adjust_Line_I2_MAX; i++) {
      p1   = &A_L_I2[i];
      S_Id = p1->Top_Sensor;
      A_L_S_Idx = p1->Top_A_L_S_Idx;
      A_L_S2[S_Id].Source_Idx[A_L_S_Idx] = i;
      A_L_S2[S_Id].Top_Bottom[A_L_S_Idx] = 0;
	  A_L_S2[S_Id].AL_p[A_L_S_Idx].S_t_p1 = p1->S_t_p1;
	  A_L_S2[S_Id].Rotate1[A_L_S_Idx] = 0;
	  A_L_S2[S_Id].Rotate2[A_L_S_Idx] = 0;
	  A_L_S2[S_Id].S_P2_XY[A_L_S_Idx][0] = 0;
	  A_L_S2[S_Id].S_P2_XY[A_L_S_Idx][1] = 0;
      A_L_S2[S_Id].Sum++;

      S_Id = p1->Bottom_Sensor;
      A_L_S_Idx = p1->Bottom_A_L_S_Idx;
      A_L_S2[S_Id].Source_Idx[A_L_S_Idx] = i;
      A_L_S2[S_Id].Top_Bottom[A_L_S_Idx] = 1;
	  A_L_S2[S_Id].AL_p[A_L_S_Idx].S_t_p1 = p1->S_t_p2;
	  A_L_S2[S_Id].Rotate1[A_L_S_Idx] = 0;
	  A_L_S2[S_Id].Rotate2[A_L_S_Idx] = 0;
	  A_L_S2[S_Id].S_P2_XY[A_L_S_Idx][0] = 0;
	  A_L_S2[S_Id].S_P2_XY[A_L_S_Idx][1] = 0;
      A_L_S2[S_Id].Sum++;
  }

#ifdef ANDROID_CODE
  int top_s = 0, top_s_idx = 0, btm_s = 0, btm_s_idx = 0;
  for(i = 0; i < Adjust_Line_I2_MAX; i++){
  	top_s     = A_L_I2[i].Top_Sensor;
  	top_s_idx = A_L_I2[i].Top_A_L_S_Idx;
  	btm_s     = A_L_I2[i].Bottom_Sensor;
  	btm_s_idx = A_L_I2[i].Bottom_A_L_S_Idx;

  	if(do_calibration_flag == 1) {
		A_L_S2[top_s].AL_p[top_s_idx].A_t_p.phi   = 0;
		A_L_S2[top_s].AL_p[top_s_idx].A_t_p.thita = 0;

		A_L_S2[btm_s].AL_p[btm_s_idx].A_t_p.phi   = 0;
		A_L_S2[btm_s].AL_p[btm_s_idx].A_t_p.thita = 0;
  	}
  	else if(AutoST_SW == 1) {
		A_L_S2[top_s].AL_p[top_s_idx].A_t_p.phi   = Adj_Sensor_Command[top_s].S2[top_s_idx].phi_offset;
		A_L_S2[top_s].AL_p[top_s_idx].A_t_p.thita = Adj_Sensor_Command[top_s].S2[top_s_idx].thita_offset;

		A_L_S2[btm_s].AL_p[btm_s_idx].A_t_p.phi   = Adj_Sensor_Command[btm_s].S2[btm_s_idx].phi_offset;
		A_L_S2[btm_s].AL_p[btm_s_idx].A_t_p.thita = Adj_Sensor_Command[btm_s].S2[btm_s_idx].thita_offset;

		A_L_S2[top_s].AL_p[top_s_idx].S_t_p1.phi   = (short)Adj_Sensor_Command[top_s].S2_Position[top_s_idx].phi_offset;
		A_L_S2[top_s].AL_p[top_s_idx].S_t_p1.thita = (short)Adj_Sensor_Command[top_s].S2_Position[top_s_idx].thita_offset;

		A_L_S2[btm_s].AL_p[btm_s_idx].S_t_p1.phi   = (short)Adj_Sensor_Command[btm_s].S2_Position[btm_s_idx].phi_offset;
  		A_L_S2[btm_s].AL_p[btm_s_idx].S_t_p1.thita = (short)Adj_Sensor_Command[btm_s].S2_Position[btm_s_idx].thita_offset;
	}
  	else {
		A_L_S2[top_s].AL_p[top_s_idx].A_t_p.phi   = 0;
		A_L_S2[top_s].AL_p[top_s_idx].A_t_p.thita = 0;

		A_L_S2[btm_s].AL_p[btm_s_idx].A_t_p.phi   = 0;
		A_L_S2[btm_s].AL_p[btm_s_idx].A_t_p.thita = 0;

		A_L_S2[top_s].AL_p[top_s_idx].S_t_p1.phi   = 0;
		A_L_S2[top_s].AL_p[top_s_idx].S_t_p1.thita = 0;

		A_L_S2[btm_s].AL_p[btm_s_idx].S_t_p1.phi   = 0;
		A_L_S2[btm_s].AL_p[btm_s_idx].S_t_p1.thita = 0;
  	}
  }
#endif

  Make_thita_rate();
}


int S2_I0[5];
int S2_I2[5];

void Trans_S2_to_S(int S_id, struct Adjust_Line_S2_Struct *S_p1,int In_phi, int In_thita, short * Out_phi, short * Out_thita )
{
  int i,j;
  float rate0, rate1;
  float temp_phi, phi_rate;
  int p0, p1, p2;
  int I0, I2;

  I0 = S2_I0[S_id];
  I2 = S2_I2[S_id];

    for (i = 0; i < S_p1->Sum; i++) {
      if (S_p1->P0_thita[I2] <= In_thita) {
	break;
      }
      I0 = (I0+1) % S_p1->Sum;
      I2 = (I2+1) % S_p1->Sum;
    }

    p1 = In_thita;
    if (i == 0){
      p0 = S_p1->P0_thita[I0] + 0x10000;
      p2 = S_p1->P0_thita[I2];
    }
    else {
	if (i == S_p1->Sum) {
	  p0 = S_p1->P0_thita[I0];
	  p2 = S_p1->P0_thita[I2] - 0x10000;
	}
	else {
	  p0 = S_p1->P0_thita[I0];
	  p2 = S_p1->P0_thita[I2];
	}
    }

    rate1 = (float)(p1 - p2) / (float)(p0 - p2);
    rate0 = 1 - rate1;

    temp_phi = S_p1->P0_phi[I0] * rate1 +  S_p1->P0_phi[I2] * rate0;

//  phi_rate =  temp_phi / In_phi;

    *Out_thita =   S_p1->F_thita[I0] * rate1 +  S_p1->F_thita[I2] * rate0;
    *Out_phi   =  (S_p1->F_phi[I0] * rate1   +  S_p1->F_phi[I2] * rate0  ); //* phi_rate;

}

int Adj_phi[5][24];
int Adj_phi_All[5];

void Camera_Calibration(void)
{
  int i,j,k,L;
  struct Adjust_Line_S_Struct  *S_p0;
  struct Adjust_Line_S2_Struct  *S_p2;
  struct Adjust_Line_I2_Struct I_p0, I_p1;
  float D_thita_Sum, D_phi_Sum;
  float thita_rate, phi_rate;
  int Out_phi, Out_thita;


for (L = 0; L < 3; L++) {
  for (k = 0; k < 10; k++) {
	Calibration_Sensor_Center(0);
    D_phi_Sum = 0;
    D_thita_Sum = 0;
    for (i = 0; i < 5; i++) {
//      Adj_Sensor[LensCode][i].S[4].X += A_L_S2[i].D_X / 4;
//      Adj_Sensor[LensCode][i].S[4].Y += A_L_S2[i].D_Y / 4;
//      Adj_Sensor[LensCode][i].Rotate_R3 += (36000 * A_L_S2[i].D_thita / 0x10000);

      D_phi_Sum   += A_L_S2[i].D_phi;
    }
    phi_rate = 1 + D_phi_Sum / (5 * 0x2000);
    Adj_Sensor_Command[0].Zoom_X =  Adj_Sensor_Command[0].Zoom_X * phi_rate;

//    Stitch_Init_All();
	Make_Lens_Rate_Line_Table_Proc();
  }

  for (k = 0; k < 4; k++) {
	Calibration_Sensor_Center(1);
	i = 0;
	Adj_Sensor_Command[i].Rotate_R3 += (36000 * A_L_S2[i].D_thita / 0x10000);

	 Make_Lens_Rate_Line_Table_Proc();
  }

  for (k = 0; k < 10; k++) {
	Calibration_Sensor_Center(1);
    D_phi_Sum = 0;
    for (i = 0; i < 5; i++) {
      Adj_Sensor_Command[i].S[4].X += A_L_S2[i].D_X;
      Adj_Sensor_Command[i].S[4].Y += A_L_S2[i].D_Y;
      Adj_Sensor_Command[i].Rotate_R3 += (36000 * A_L_S2[i].D_thita / 0x10000);

      D_phi_Sum   += A_L_S2[i].D_phi;
    }
    phi_rate = 1 + D_phi_Sum / ( 5 * 0x2000);
    Adj_Sensor_Command[0].Zoom_X =  Adj_Sensor_Command[0].Zoom_X * phi_rate;
//    Stitch_Init_All();
     Make_Lens_Rate_Line_Table_Proc();
  }
};

  Calibration_Sensor_Center(1);
  Make_thita_rate();

  for (i = 0; i < 5; i++) {
    S_p2 = &A_L_S2[i];

    int I0, I2;

    for (j = 0; j < S_p2->Sum; j++) {
      I0 = j; I2 = (j + 1) % S_p2->Sum;
      if (S_p2->P0_thita[I0] < S_p2->P0_thita[I2]) {
    	S2_I0[i] = I0; S2_I2[i] = I2;
    	break;
      }
    }

    for (j = 0; j < S_p2->Sum; j++) {
	  Trans_S2_to_S(i, &A_L_S2[i], S_p2->AL_p[j].S_t_p1.phi , S_p2->AL_p[j].S_t_p1.thita, &S_p2->AL_p[j].A_t_p.phi, &S_p2->AL_p[j].A_t_p.thita );
	}
  }
  Calibration_Check_Center();

  int Top, Btm, Top_Idx, Btm_Idx;
   for (i = 0; i < Adjust_Line_I2_MAX; i++) {
 	Top        = A_L_I2[i].Top_Sensor;
 	Btm        = A_L_I2[i].Bottom_Sensor;
 	Top_Idx    = A_L_I2[i].Top_A_L_S_Idx;
 	Btm_Idx    = A_L_I2[i].Bottom_A_L_S_Idx;

 	Adj_Sensor_Command[Top].S2[Top_Idx].phi_offset   = A_L_S2[Top].AL_p[Top_Idx].A_t_p.phi;
 	Adj_Sensor_Command[Top].S2[Top_Idx].thita_offset = A_L_S2[Top].AL_p[Top_Idx].A_t_p.thita;

 	Adj_Sensor_Command[Btm].S2[Btm_Idx].phi_offset   = A_L_S2[Btm].AL_p[Btm_Idx].A_t_p.phi;
 	Adj_Sensor_Command[Btm].S2[Btm_Idx].thita_offset = A_L_S2[Btm].AL_p[Btm_Idx].A_t_p.thita;

 	Adj_Sensor_Command[Top].S2_Position[Top_Idx].phi_offset   = A_L_S2[Top].AL_p[Top_Idx].S_t_p1.phi;
 	Adj_Sensor_Command[Top].S2_Position[Top_Idx].thita_offset = A_L_S2[Top].AL_p[Top_Idx].S_t_p1.thita;

 	Adj_Sensor_Command[Btm].S2_Position[Btm_Idx].phi_offset   = A_L_S2[Btm].AL_p[Btm_Idx].S_t_p1.phi;
 	Adj_Sensor_Command[Btm].S2_Position[Btm_Idx].thita_offset = A_L_S2[Btm].AL_p[Btm_Idx].S_t_p1.thita;
   }

   int All, Sum;
   for(i = 0; i < 5; i++){
 	Adj_Sensor[LensCode][i] = Adj_Sensor_Command[i];
 	All = 0;
 	Sum = A_L_S2[i].Sum; All= 0;
 	for (j = 0; j < Sum; j++) {
 	   Adj_phi[i][j] = Adj_Sensor_Command[i].S2[j].phi_offset;
 	   All += abs(Adj_phi[i][j]);
 	}
 	Adj_phi_All[i] = All / Sum;
   }

  memcpy(&Test_Tool_Adj.Adj_Sensor[0], &Adj_Sensor[LensCode][0], sizeof(struct Adj_Sensor_Struct)*5 );
};

// for test machine  miller 20160812


struct YUVC_Line_Struct   YUVC_Line[32];
void Get_YUVC_Line(void)
{
  int i;
  for (i = 0 ; i < 32; i++) {
    YUVC_Line[i].Pan = i;
  }
};


Test_Tool_Adj_Struct Test_Tool_Adj;
Test_Tool_Result_Struct Test_Tool_Result;
Adj_Sensor_Lens_Struct Adj_Sensor_Lens_S;
void Test_Tool_Adj_Struct_Init(void)
{
	Test_Tool_Adj.checksum        = 0x20160503;
	memcpy(&Test_Tool_Adj.Adj_Sensor[0], &Adj_Sensor[LensCode][0], sizeof(struct Adj_Sensor_Struct)*5);
	memcpy(&Test_Tool_Adj.Test_Block_Table[0], &Test_Block_Table[0], sizeof(struct Test_Block_Table_Struct)*Test_Block_Table_MAX);
	Test_Tool_Adj.SensorZoom      = SensorZoom[LensCode];
	memset(&Test_Tool_Adj.test_tool_ver[0], 0, sizeof(Test_Tool_Adj.test_tool_ver) );
}
/*
 * 預設33ms的掃描線數目(line)
 */
int get_ep_ln_default_33ms(int freq)
{
    int ep_ln_33ms, scale = Get_Input_Scale();
    // EP_FRM_LENGTH_60Hz_FS_DEFAULT = 1222(line) = 33.33ms
    switch(scale){
    default:
    case 1: ep_ln_33ms = (freq == 0)? EP_FRM_LENGTH_60Hz_FS_DEFAULT: EP_FRM_LENGTH_50Hz_FS_DEFAULT;
            break;// 0x04c6
    case 2: ep_ln_33ms = (freq == 0)? EP_FRM_LENGTH_60Hz_D2_DEFAULT: EP_FRM_LENGTH_50Hz_D2_DEFAULT;
            break;// 0x0781
    case 3: ep_ln_33ms = (freq == 0)? EP_FRM_LENGTH_60Hz_D3_DEFAULT: EP_FRM_LENGTH_50Hz_D3_DEFAULT;
            break;// 0x0476
    }
    return ep_ln_33ms;
}
/*
 * 系統曝光週期內最大的掃描線數目(line)
 */
int get_ep_ln_maximum(int sys_fps)
{
    static int lst_ep_ln=0;
    int max_ep_ln;
    int freq = ISP_AEG_EP_IMX222_Freq;
    int ep_ln_33ms = get_ep_ln_default_33ms(freq);
    if(freq == 0) max_ep_ln = ((10000 / (float)(sys_fps)) * ep_ln_33ms) / 33.333;
    else          max_ep_ln = ((10000 / (float)(sys_fps)) * ep_ln_33ms) / 40.000;
    if(lst_ep_ln != max_ep_ln){
        lst_ep_ln = max_ep_ln;
        db_debug("get_ep_ln_maximum: freq=%d max_ep_ln=%d sys_fps=%d ep_ln_33ms=%d\n", freq, max_ep_ln, sys_fps, ep_ln_33ms);
    }
    return max_ep_ln;
}
/*
 * 計算sensor曝光時間(us)
 */
int cal_sensor_exp_us(int freq, int sys_fps, int frame, int integ)
{
    int ep_ln_33ms = get_ep_ln_default_33ms(freq);
    int fps_us = 10000000 / sys_fps;
    int exp_us = (frame + 1) * fps_us - integ * fps_us / (ep_ln_33ms * fps_us / 33333);
    return exp_us;
}
EP_Line_2_Gain_Table_Struct EP_Line_2_Gain_Table[2];
void EP_Idx_2_Gain_Table_Init2(void)
{
    int i, j=2, k;
    int idx_lst=0, max, min;
    float tmp, clk=1.0, ep_ln_33ms=1125.0, i2;
    int input_mode = Get_Input_Scale();

    for(k = 0; k < 2; k++) { 			//freq
        ep_ln_33ms = get_ep_ln_default_33ms(k);
        max = 0; min = 0;
        for(i = 0; i < 350; i++) {      // (1/120)*350=2.9sec
            tmp = i + 1;
            EP_Line_2_Gain_Table[k].table[i] = (int)(log2f( (float)tmp / (float)ep_ln_33ms) * 1280.0 + 2560.0);       // 1280=20*64, 2560=40*64

            if(EP_Line_2_Gain_Table[k].table[i] <= min)
                min = EP_Line_2_Gain_Table[k].table[i];      // 幾條掃描線
        }
        EP_Line_2_Gain_Table[k].max_idx = 0;
        EP_Line_2_Gain_Table[k].min_idx = min;
    }
}

// 計算真實曝光掃描線
void calculate_real_sensor_exp(int exp_idx, int *fr, int *ln, int *fps)
{
    int i_tmp=0, f_tmp=0;
    int ep_ln_33ms, ep_ln_max, ep_sec = 1;;
    int ep_tmp, ep_line;
    int freq = ISP_AEG_EP_IMX222_Freq;
    int sys_fps = getFPS();

    ep_ln_33ms = get_ep_ln_default_33ms(freq);
    // FPS = 100 -> 10fps, ep_ln_max = 3666 (每秒10fps -> 100ms -> 最多3666掃描線)
    //     = 50 -> 5fps, ep_ln_max = 7332 (每秒5fps -> 200ms -> 最多7332掃描線)
    ep_ln_max = get_ep_ln_maximum(sys_fps);

    if(exp_idx >= 0) {
        if     (exp_idx >= (180*64)) ep_sec = 4;		//4s	180*64=11520		line(4s) = line(1s)*4
    	else if(exp_idx >= (160*64)) ep_sec = 2;		//2s	160*64=10240		line(2s) = line(1s)*2
    	else                         ep_sec = 1;

        int base_t1, base_t2;
        if(freq == 0){ base_t1 = 120; base_t2 = 30; }       // NTSC
        else         { base_t1 = 100; base_t2 = 25; }       // PAL
        // ep_ln_33ms * base_t2 / base_t1 = 305.5(掃描線/每1/120秒)
        ep_tmp = Find_Flash_2_Gain_Table_Idx(exp_idx);
        ep_line = (ep_tmp * ep_ln_33ms * base_t2 / base_t1) * ep_sec;
    }
    else {
        ep_tmp = Get_EP_Gain(exp_idx, freq);
        ep_line = Get_EP_Line(ep_tmp, freq);
    }

    //AEG_EP_FRM_LENGTH: 曝光幾張畫面
    //AEG_EP_INTEG_TIME: 曝光幾條掃描線
    if(ep_line >= ep_ln_max) {
        f_tmp = ep_line / ep_ln_max;        // 曝光幾張
        i_tmp = ep_line % ep_ln_max;        // 曝光幾條
        if(i_tmp != 0) {
            *fr = f_tmp;
            *ln = (ep_ln_max - i_tmp);
        }
        else {
            *fr = f_tmp - 1;
            *ln = 0;
        }
    }
    else {
        *fr = 0;
        *ln = ep_ln_max - ep_line;
    }

    if     (exp_idx >= (180*64)) *fps = 2;    // 4s, 2fps
    else if(exp_idx >= (160*64)) *fps = 5;    // 2s, 5fps
    else if(exp_idx >= 0){
        if(freq == 0) {
            if(ep_tmp <= sys_fps)  *fps = sys_fps; // 設定錄影最大張數
            else if(ep_tmp <= 8)   *fps = 150;
            else if(ep_tmp <= 16)  *fps = 75;
            else if(ep_tmp <= 20)  *fps = 60;
            else if(ep_tmp <= 32)  *fps = 40;
            else if(ep_tmp <= 64)  *fps = 20;
            else if(ep_tmp <= 120) *fps = 10;
        }
        else {
            if(ep_tmp <= sys_fps)  *fps = sys_fps;
            else if(ep_tmp <= 8)   *fps = 120;
            else if(ep_tmp <= 16)  *fps = 60;
            else if(ep_tmp <= 20)  *fps = 30;
            else if(ep_tmp <= 32)  *fps = 20;
            else if(ep_tmp <= 64)  *fps = 10;
            else if(ep_tmp <= 120) *fps = 10;
        }
    }
    else *fps = sys_fps;
}

int Get_EP_Gain(int idx, int freq)
{
    int i, line=0, tmp=0, idx2=0, frm_line;

    // rex+ 180518, 節省運算次數, 從後面開始搜尋
    for(i = (350-1); i > 0; i--){
        if(idx >= EP_Line_2_Gain_Table[freq].table[i])
            return EP_Line_2_Gain_Table[freq].table[i];
    }
    return EP_Line_2_Gain_Table[freq].table[0];
}

int Get_EP_Line(int idx, int freq)
{
    int i, line=0;
    int ep_ln_33ms;

    ep_ln_33ms = get_ep_ln_default_33ms(freq);

    line = pow(2, ( ( (float)idx - 2560.0) / 1280.0) ) * (float)ep_ln_33ms;
    return line;
}

float Flash_2_Gain_Table[120];
void Flash_2_Gain_Table_Init(void)
{
    int i;
    float i2;
    //log2f(1) = 0
    //log2f(10) = 1
    //log2f(25) = 1.397
    //log2f(50) = 1.698
    //log2f(75) = 1.875
    //log2f(100) = 2
    for(i = 0; i < 120; i++) {
    	i2 = i + 1;
    	Flash_2_Gain_Table[i] = log2f(i2) * (20*64);       // 1280
    }
}

int Find_Flash_2_Gain_Table_Idx(int value)
{
	int low=0, high=119;
	int idx=0, idx_tmp;

	if(value > Flash_2_Gain_Table[high]) {
//		if(value >= (int)Flash_2_Gain_Table[119])      idx = 119;
//		else if(value >= (int)Flash_2_Gain_Table[63])  idx = 63;
//		else if(value >= (int)Flash_2_Gain_Table[31])  idx = 31;
		idx = 119;
	}
	else {
		while(low < high) {
			idx_tmp = ( (high - low) >> 1) + low;
			if( (int)Flash_2_Gain_Table[idx_tmp] > value)
				high = idx_tmp;
			else
				low = idx_tmp;

			if(low == high || (high - low) == 1 ) {
				idx = low;
				break;
			}
		}
	}
	return (idx+1);
}

void ResolutionSpecInit(void)
{
	EP_Idx_2_Gain_Table_Init2();
	Flash_2_Gain_Table_Init();
}
/*
 * return: ret_gain >= 0 正常
 *         ret_gain < 0  亮度不足
 */
void get_real_exp_gain_idx(int c_mode, int aeg_idx, int exp_max, int *ret_exp, int *ret_gain, int aeb_step)
{
    int freq = ISP_AEG_EP_IMX222_Freq;
    int ep_tmp, gain_tmp;
    int gain_adj;
    int exp_mul, exp_idx, gain_mul, gain_idx, bulb_sec, bulb_iso;

    get_AEG_UI_Setting(&exp_mul, &exp_idx, &gain_mul, &gain_idx, &bulb_sec, &bulb_iso);

    if(c_mode == 12){                   // M-Mode(B快門)啟動，控制live畫面
        exp_mul = 1;
        switch(bulb_sec){
        case 1: exp_idx = 140*64; break;
        case 2: exp_idx = 160*64; break;
        case 3: exp_idx = 170*64; break;
        default:
        case 4: exp_idx = 180*64; break;
        }
        gain_mul = 1;
        gain_idx = bulb_iso;     // 0:iso100
    }
    if(aeb_step >= 0){         // 0,1,2,4,5,6
        exp_mul = 0;
        gain_mul = 0;
    }

    if(exp_mul == 1 && gain_mul == 1) {
        if(exp_idx > exp_max) exp_idx = exp_max;
        *ret_exp = exp_idx;                                     // 手動設定exp
        *ret_gain = gain_idx;                                   // 手動設定gain
    }
    else if(exp_mul == 1 && gain_mul == 0) {              // 手動調整exp
        if(exp_idx > exp_max) exp_idx = exp_max;
        if(exp_idx < 0) {                                       // EP < 1/120 ~ 1/32000
            ep_tmp = Get_EP_Gain(exp_idx, freq);                // 防止日光燈閃爍
            *ret_exp = ep_tmp;
            *ret_gain = aeg_idx - ep_tmp;
        }
        else{                                                   // EP >= 1/120 ~ 4
            *ret_exp = exp_idx;
            *ret_gain = aeg_idx - exp_idx;
        }
    }
    else if(exp_mul == 0 && gain_mul == 1) {              // 手動調整gain
        exp_idx = aeg_idx - gain_idx;
        if(exp_idx < 0) {                                       // EP < 1/120
            ep_tmp = Get_EP_Gain(exp_idx, freq);
            *ret_exp = ep_tmp;
            //*ret_gain = gain_idx;                             // 餘數不給gain, 會造成閃爍
            gain_adj = (exp_idx - ep_tmp);
            if(gain_adj > 640) gain_adj = 640;
            if(gain_adj < -640) gain_adj = -640;
            *ret_gain = gain_idx + gain_adj;
        }
        else{
            if(exp_idx >= exp_max) {                            // EP > 1/15 or 1/8
                *ret_exp = exp_max;
                //*ret_gain = gain_idx + (exp_idx - (*ret_exp));// 餘數給gain, 會造成亮度不變
                //*ret_gain = gain_idx;                         // 餘數不給gain, 會造亮度lock
                gain_adj = (exp_idx - exp_max);
                if(gain_adj > 640) gain_adj = 640;
                if(gain_adj < -640) gain_adj = -640;
                *ret_gain = gain_idx + gain_adj;                // 餘數控制在640以內
            }
            else {
                //ep_tmp = Find_Flash_2_Gain_Table_Idx(exp_idx);
                //*ret_exp = Flash_2_Gain_Table[ep_tmp-1];
                //*ret_gain = (exp_idx - Flash_2_Gain_Table[ep_tmp-1]) + gain_idx;
                *ret_exp = ((exp_idx + 640) / 1280) * 1280;     // 需要整倍數
                *ret_gain = aeg_idx - (*ret_exp);               // 餘數給gain
            }
        }
    }
    else{  //(exp_mul == 0 && gain_mul == 0)
        exp_idx = aeg_idx;
        if(exp_idx < 0) {                               // EP < 1/120
            ep_tmp = Get_EP_Gain(exp_idx, freq);
            *ret_exp = ep_tmp;                          // 線性曝光時間
            *ret_gain = aeg_idx - ep_tmp;
        }
        else{
            // Step1: +160
            ep_tmp = aeg_idx + (160*64);                // -160*64=(1/32000)s, 轉為正整數
            // Step2: %20
            gain_tmp = ep_tmp % (20*64);                // 取餘數當gain
            ep_tmp -= gain_tmp;
            if(ep_tmp > exp_max + (160*64)) {           // 限制最大EP=1/15 or 1/8
                *ret_exp = exp_max;
                *ret_gain = gain_tmp + (ep_tmp - (exp_max + (160*64)));
            }
            else {
                *ret_exp = ep_tmp - (160*64);           // 10240
                *ret_gain = gain_tmp;
            }
        }
    }

    if(*ret_gain < 0) *ret_gain = 0;					//解固定ep, gain < 0 畫面呈現紫色問題
}

Sensor_Lens_Table_Struct Sensor_Lens_Table_S;
float Sensor_Lens_Gain = 1.2;
float Sensor_Lens_Table[2][11] = {
  { 0.000, 0.010, 0.045, 0.075,
    0.110, 0.160, 0.175, 0.195,
    0.300, 0.500, 0.700 },

  { 0.000, 0.010, 0.025, 0.045,
	0.070, 0.095, 0.120, 0.150,
	0.185, 0.230, 0.300 }
};
float Adj_Sensor_Lens_Th = 0;
int Sensor_Lens_Th = 8;

void setSensroLensTable(int idx, int value)
{
	switch(idx) {
	case 0:  Sensor_Lens_Gain      = (float)value / 1000.0; break;
	case 1:  Sensor_Lens_Table[LensCode][0]  = (float)value / 1000.0; break;
	case 2:  Sensor_Lens_Table[LensCode][1]  = (float)value / 1000.0; break;
	case 3:  Sensor_Lens_Table[LensCode][2]  = (float)value / 1000.0; break;
	case 4:  Sensor_Lens_Table[LensCode][3]  = (float)value / 1000.0; break;
	case 5:  Sensor_Lens_Table[LensCode][4]  = (float)value / 1000.0; break;
	case 6:  Sensor_Lens_Table[LensCode][5]  = (float)value / 1000.0; break;
	case 7:  Sensor_Lens_Table[LensCode][6]  = (float)value / 1000.0; break;
	case 8:  Sensor_Lens_Table[LensCode][7]  = (float)value / 1000.0; break;
	case 9:  Sensor_Lens_Table[LensCode][8]  = (float)value / 1000.0; break;
	case 10: Sensor_Lens_Table[LensCode][9]  = (float)value / 1000.0; break;
	case 11: Sensor_Lens_Table[LensCode][10] = (float)value / 1000.0; break;
	case 12: Sensor_Lens_Th    	   = value; 				break;
	}
}

void getSensroLensTable(int *val)
{
	*val      = (int)(Sensor_Lens_Gain * 1000);
	*(val+1)  = (int)(Sensor_Lens_Table[LensCode][0] * 1000);
	*(val+2)  = (int)(Sensor_Lens_Table[LensCode][1] * 1000);
	*(val+3)  = (int)(Sensor_Lens_Table[LensCode][2] * 1000);
	*(val+4)  = (int)(Sensor_Lens_Table[LensCode][3] * 1000);
	*(val+5)  = (int)(Sensor_Lens_Table[LensCode][4] * 1000);
	*(val+6)  = (int)(Sensor_Lens_Table[LensCode][5] * 1000);
	*(val+7)  = (int)(Sensor_Lens_Table[LensCode][6] * 1000);
	*(val+8)  = (int)(Sensor_Lens_Table[LensCode][7] * 1000);
	*(val+9)  = (int)(Sensor_Lens_Table[LensCode][8] * 1000);
	*(val+10) = (int)(Sensor_Lens_Table[LensCode][9] * 1000);
	*(val+11) = (int)(Sensor_Lens_Table[LensCode][10] * 1000);
	*(val+12) = Sensor_Lens_Th;
}

float Sensor_Lens_Map[5][256][512];
float Sensor_Lens_thita[5][256][512];
float Sensor_Lens_Map_Th[5][256][512];
float Sensor_Lens_Map_Bright[5][256][512];
char Sensor_Lens_Map_Cmd[5][64][512];

float Sensor_Lens_Map_Bright_40p[4][80][128];

int Sensor_Lens_Map_Cnt = 0;
float Sensor_Lens_Map_Bright_tmp[4][80][128];
void Sensor_Lens_Map_Buf_Init(void)
{
  int i, j, k;

  for(k = 0; k < 4; k++) {
	for(i = 0; i < 69; i++) {
		for(j = 0; j < 125; j++) {
			Sensor_Lens_Map[k][i][j] = 0;
			Sensor_Lens_Map_Th[k][i][j] = 0;
			Sensor_Lens_Map_Bright[k][i][j] = 0;
			Sensor_Lens_Map_Cmd[k][i][j] = 0;
		}
	}
  }
}

Sensor_Lens_Map_Parameter_Struct Sensor_Lens_Map_P[5][64][512];
ALS_Angle_Sorting_Buf_Struct Sitching_Point_P[5][24];
void ALS_Angle_Sorting(void)
{
    int i = 0, j = 0, s_id = 0;
    int sum = 0, angle_tmp = 0, idx_tmp = 0, dis_tmp = 0;

    for(s_id = 0; s_id < 5; s_id++) {
		sum = A_L_S2[s_id].Sum;
	    angle_tmp = 0;  idx_tmp = 0;  dis_tmp = 0;
	    for( i = 0; i < sum; i++) {
	        for( j = i; j < sum; j++) {
		        if(Sitching_Point_P[s_id][j].angle > Sitching_Point_P[s_id][i].angle) {
		            angle_tmp = Sitching_Point_P[s_id][j].angle;
		            Sitching_Point_P[s_id][j].angle = Sitching_Point_P[s_id][i].angle;
		            Sitching_Point_P[s_id][i].angle = angle_tmp;

		            idx_tmp = Sitching_Point_P[s_id][j].idx;
		            Sitching_Point_P[s_id][j].idx = Sitching_Point_P[s_id][i].idx;
		            Sitching_Point_P[s_id][i].idx = idx_tmp;

		            dis_tmp = Sitching_Point_P[s_id][j].distance;
		            Sitching_Point_P[s_id][j].distance = Sitching_Point_P[s_id][i].distance;
		            Sitching_Point_P[s_id][i].distance = dis_tmp;
					}
				}
	}
  }
}

float Sensor_Lens_Map_Angle(int cx, int cy, int px, int py)
{
  int mode;
  int dx, dy;
  float slope, angle, angle_tmp;

  if(px < cx && py >= cy)       mode = 0;
  else if(px >= cx && py >= cy) mode = 1;
  else if(px >= cx && py < cy)  mode = 2;
  else if(px < cx && py < cy)   mode = 3;

  dx = px - cx;
  dy = py - cy;
  if(dx == 0)      { slope = 0; angle_tmp = 90; }
  else if(dy == 0) { slope = 0; angle_tmp = 0; }
		else {
	slope = (float)dy / (float)dx;
	angle_tmp = atan(slope) * 180.0 / pi;
  }

  switch(mode) {
  case 0: angle = abs((int)angle_tmp);       break;
  case 1: angle = 180 - abs((int)angle_tmp); break;
  case 2: angle = 180 + abs((int)angle_tmp); break;
  case 3: angle = 360 - abs((int)angle_tmp); break;
	}

  return angle;
}

float Sensor_Lens_Map_Distance(int cx, int cy, int px, int py)
{
  int dx, dy;
  float distance;

  dx = px - cx;
  dy = py - cy;
  distance = sqrt(dx*dx + dy*dy);

  return distance;
}


float Sensor_Lens_Bright_Adj[4] = {0};
void SetSensorLensBright(int idx, int value)
{
	if(idx < 0 || idx >= 4) return;
	Sensor_Lens_Bright_Adj[idx] = value / 1000.0;
}

void GetSensorLensBright(int *value)
{
	*value 	   = (int)(Sensor_Lens_Bright_Adj[0] * 1000);
	*(value+1) = (int)(Sensor_Lens_Bright_Adj[1] * 1000);
	*(value+2) = (int)(Sensor_Lens_Bright_Adj[2] * 1000);
	*(value+3) = (int)(Sensor_Lens_Bright_Adj[3] * 1000);
}

int Sensor_Lens_Adj(int mode)
{
  int C_X, C_Y;
  int D_X, D_Y;
  int Distance;
  int D_H, D_L;
  float Adj_V;
  int i, j, k, t;
  int i2, j2;
  float gain = Sensor_Lens_Gain;
  int th_c = Sensor_Lens_Th;
  int th_rate, rate;
  int S_C_X, S_C_Y;		//Sensor Center XY
  FILE *fp;
  int fp2fd;
  char path[128];
  int real_y_tmp;

  int thita3;
  float thita3_f;
  unsigned short S_rate_Idx;
  unsigned short S_rate_Sub, I_S_rate_Sub;
  short p0_thita, p1_thita;
  int AL_p[2], AL_p_tmp;
  float AL_p_rate[2];

  float ALS_Adj_V;
  int ALS_D_X[2], ALS_D_Y[2];

  int Send_SLens_Cmd = 0;
  float sub, sub_t=0.3;
  static float ALS_Yrate_lst[4][24];

  memset(&Sensor_Lens_Map[0][0][0], 0, sizeof(Sensor_Lens_Map) );
  memset(&Sensor_Lens_Map_Cmd[0][0][0], 0, sizeof(Sensor_Lens_Map_Cmd) );

  int Input_Scale = mode;	//Get_Input_Scale();
  Send_SLens_Cmd = 0;
  for(k = 0; k < 5; k++) {
	  S_C_X = Adj_Sensor[LensCode][k].S[4].X;
	  S_C_Y = Adj_Sensor[LensCode][k].S[4].Y;
	  C_X = (Sensor_C_X_Base + S_C_X); C_Y = (Sensor_C_Y_Base + S_C_Y + Sensor_Lens_C_Y[LensCode]);

	  for (i = 0; i < 64; i+= Input_Scale) {
		  for (j = 0; j < 75; j+= Input_Scale) {
			 i2 = i / Input_Scale;
			 j2 = j / Input_Scale;
			 D_X= ( (j << 6) - C_X);
			 D_Y= ( (i << 6) - C_Y);
			 Adj_V = Sensor_Lens_trans(D_X, D_Y);
			 Sensor_Lens_Map[k][i2][j2] = Adj_V;

			 //if(mode == 0)
			//	 rate = 0x40;
			 //else
			     rate = Sensor_Lens_Map[k][i2][j2] * 0x40;
			 if(rate >= 0xFF) rate = 0xFF;

			 Sensor_Lens_Map_Cmd[k][i/Input_Scale][j/ Input_Scale]       = rate;	//B
			 Sensor_Lens_Map_Cmd[k][i/Input_Scale][j/ Input_Scale + 128] = rate;	//G
			 Sensor_Lens_Map_Cmd[k][i/Input_Scale][j/ Input_Scale + 256] = rate;	//G
			 Sensor_Lens_Map_Cmd[k][i/Input_Scale][j/ Input_Scale + 384] = rate;	//R
		  }
	  }
  }

#ifdef ANDROID_CODE
  if(TestToolCmd.MainCmd == 6) Send_SLens_Cmd = 1;
#endif

  return 1;	//Send_SLens_Cmd;
}

//int checkSLensFileFlag = 0;
#ifdef ANDROID_CODE

int checkSLensFile(void)
{
	FILE *fp;
	char path[128];

	fp = fopen("/mnt/sdcard/US360/Sensor_Lens_Map_Th.bin", "rb");
	if(fp != NULL) {
		fread(&Sensor_Lens_Map_Th[0][0][0], sizeof(Sensor_Lens_Map_Th), 1, fp);
		fclose(fp);
	}
	else return 0;

	fp = fopen("/mnt/sdcard/US360/Sensor_Lens_Map_Bright.bin", "rb");
	if(fp != NULL) {
		fread(&Sensor_Lens_Map_Bright[0][0][0], sizeof(Sensor_Lens_Map_Bright), 1, fp);
		fclose(fp);
	}
	else return 0;

	return 1;
}

int doSensorLensEn = 0;
void doSensorLensAdj(int s_id, int mode)
{
	int i, j, addr, size, tmp_s, FPGA_ID, MT;
	unsigned char *buf;
	FILE *fp;
	char path[128];
	struct stat sti;
	int sned_cmd_flag = 0;
	unsigned s_addr, t_addr, cmd_mode, id;
	int Input_Scale;
	int f2_addr, fx_addr;

	size = 64 * 512;	//sizeof(Sensor_Lens_Map_Cmd[i]);
	tmp_s = 2048;
	for(Input_Scale = 1; Input_Scale <= 3; Input_Scale++) {
		switch(Input_Scale) {
		case 1:
			f2_addr = F2_LC_BUF_FS_Addr;
			fx_addr = FX_LC_FS_BUF_Addr;
			break;
		case 2:
			f2_addr = F2_LC_BUF_D2_Addr;
			fx_addr = FX_LC_D2_BUF_Addr;
			break;
		case 3:
			f2_addr = F2_LC_BUF_D3_Addr;
			fx_addr = FX_LC_D3_BUF_Addr;
			break;
		}
		Sensor_Lens_Adj(Input_Scale);

		/*
		 *	Write Command
		 */
		Get_FId_MT(s_id, &FPGA_ID, &MT);
		buf = &Sensor_Lens_Map_Cmd[s_id][0][0];
		addr = f2_addr + s_id * 0x8000;
		for(j = 0; j < size; j += tmp_s) {
			ua360_spi_ddr_write(addr + j, &buf[j], tmp_s);

			/*
			 *   Command is Translated to FPGA0 and FPGA1 by SPI
			 */
			s_addr = (addr + j);
			t_addr = (fx_addr + MT * 0x8000 + j);
			cmd_mode = 1;
			if(FPGA_ID == 0) id = AS2_MSPI_F0_ID;
			else             id = AS2_MSPI_F1_ID;
			AS2_Write_F2_to_FX_CMD(s_addr, t_addr, tmp_s, cmd_mode, id, 1);
		}
		Sen_Lens_Adj_Cal_CheckSum[Input_Scale-1][s_id] = Cal_DDR_CheckSum(size, buf);
	}

	doSensorLensEn = mode;
}
void SetdoSensorLensAdj(int mode)
{
	int i;
	for(i = 0; i < 5; i++)
		doSensorLensAdj(i, mode);
}

void SetColorSTSW(int en)
{
	ColorST_SW = (en & 0x1);
}

int GetColorSTSW(void)
{
	return ColorST_SW;
}

void SetAutoSTSW(int en)
{
	AutoST_SW = (en & 0x1);
}

int GetAutoSTSW(void)
{
	return AutoST_SW;
}

/*
 * 本機先寫檔給TestTool讀取, 以同步參數
 */
void WriteSensorAdjFile(void)
{
	FILE *fp=NULL;

	Test_Tool_Adj.checksum = SENSOR_ADJ_CHECK_SUM;
	memcpy(&Test_Tool_Adj.Adj_Sensor[0], &Adj_Sensor[LensCode][0], sizeof(struct Adj_Sensor_Struct)*5);
	memcpy(&Test_Tool_Adj.Test_Block_Table[0], &Test_Block_Table[0], sizeof(struct Test_Block_Table_Struct)*Test_Block_Table_MAX);
	Test_Tool_Adj.SensorZoom = Adj_Sensor[LensCode][0].Zoom_X;

	fp = fopen("/mnt/sdcard/US360/Test/Sensor_Adj.bin", "wb");
	if(fp != NULL) {
		fwrite(&Test_Tool_Adj, sizeof(Test_Tool_Adj), 1, fp);
		fclose(fp);
	}
}

/*
 * TestTool校正完給本機讀取最後的校正值
 */
void ReadSensorAdjFile(void)
{
	int i, size;
	int Top, Btm, Top_Idx, Btm_Idx;
	FILE *fp=NULL;
	struct stat sti;

	fp = fopen("/mnt/sdcard/US360/Test/Sensor_Adj.bin", "rb");
	if(fp != NULL) {
		stat("/mnt/sdcard/US360/Test/Sensor_Adj.bin", &sti);
		size = sti.st_size;
		if(size <= 0) {
			db_error("Sensor_Adj.bin not found !\n");
			fclose(fp);
			return;
		}
		if(size <= sizeof(Test_Tool_Adj) )
			fread(&Test_Tool_Adj, size, 1, fp);
		else
			fread(&Test_Tool_Adj, sizeof(Test_Tool_Adj), 1, fp);
		fclose(fp);
	}
	memcpy(&Adj_Sensor[LensCode][0], &Test_Tool_Adj.Adj_Sensor[0], sizeof(struct Adj_Sensor_Struct)*5);
	memcpy(&Test_Block_Table[0], &Test_Tool_Adj.Test_Block_Table[0], sizeof(struct Test_Block_Table_Struct)*Test_Block_Table_MAX);
	SensorZoom[LensCode] = Adj_Sensor[LensCode][0].Zoom_X;

	for (i = 0; i < Adjust_Line_I2_MAX; i++) {
		Top        = A_L_I2[i].Top_Sensor;
		Btm        = A_L_I2[i].Bottom_Sensor;
		Top_Idx    = A_L_I2[i].Top_A_L_S_Idx;
		Btm_Idx    = A_L_I2[i].Bottom_A_L_S_Idx;
	}

	for(i = 0; i < 5; i++)
		Adj_Sensor_Command[i] = Adj_Sensor[LensCode][i];

	SaveConfig(1);
	LoadConfig(1);
}

void ReadAdjSensorLensFile() {
	int i, size;
	int Top, Btm, Top_Idx, Btm_Idx;
	FILE *fp=NULL;
	struct stat sti;

	fp = fopen("/mnt/sdcard/US360/Test/Adj_Sensor_Lens.bin", "rb");
	if(fp != NULL) {
		stat("/mnt/sdcard/US360/Test/Adj_Sensor_Lens.bin", &sti);
		size = sti.st_size;
		if(size <= 0) db_error("Adj_Sensor_Lens.bin not found !\n");
		if(size <= sizeof(Adj_Sensor_Lens_S) )
			fread(&Adj_Sensor_Lens_S, size, 1, fp);
		else
			fread(&Adj_Sensor_Lens_S, sizeof(Adj_Sensor_Lens_S), 1, fp);
		fclose(fp);
	}

	memcpy(&Sensor_Lens_Map_Th[0][0][0], &Adj_Sensor_Lens_S.Sensor_Lens_Map_Th[0][0][0], sizeof(Sensor_Lens_Map_Th) );
	memcpy(&Sensor_Lens_Map_Bright[0][0][0], &Adj_Sensor_Lens_S.Sensor_Lens_Map_Bright[0][0][0], sizeof(Sensor_Lens_Map_Bright) );

	fp = fopen("/mnt/sdcard/US360/Sensor_Lens_Map_Th.bin", "wb");
	if(fp != NULL) {
		fwrite(&Sensor_Lens_Map_Th[0][0][0], sizeof(Sensor_Lens_Map_Th), 1, fp);
		fclose(fp);
	}

	fp = fopen("/mnt/sdcard/US360/Sensor_Lens_Map_Bright.bin", "wb");
	if(fp != NULL) {
		fwrite(&Sensor_Lens_Map_Bright[0][0][0], sizeof(Sensor_Lens_Map_Bright), 1, fp);
		fclose(fp);
	}
}

//void ReadSensorLensTableFile(void)
//{
//	int i, size;
//	int Top, Btm, Top_Idx, Btm_Idx;
//	FILE *fp=NULL;
//	struct stat sti;
//
//	fp = fopen("/mnt/sdcard/US360/Test/Sensor_Lens_Table.bin", "rb");
//	if(fp != NULL) {
//		stat("/mnt/sdcard/US360/Test/Sensor_Lens_Table.bin", &sti);
//		size = sti.st_size;
//		if(size <= 0) db_error("Sensor_Lens_Table.bin not found !\n");
//		if(size <= sizeof(Sensor_Lens_Table_S) )
//			fread(&Sensor_Lens_Table_S, size, 1, fp);
//		else
//			fread(&Sensor_Lens_Table_S, sizeof(Sensor_Lens_Table_S), 1, fp);
//		fclose(fp);
//	}
//
//	Sensor_Lens_Gain = Sensor_Lens_Table_S.gain;
//	memcpy(&Sensor_Lens_Table[LensCode][0], &Sensor_Lens_Table_S.table[0], sizeof(Sensor_Lens_Table[LensCode]) );
//}

int send_Sitching_Cmd(int f_id)
{
	int i, k, tmpaddr;
	int size, total_size;
	unsigned char *st_p;
	int start_idx;
	int mode, size_tmp;
	AS2_SPI_Cmd_struct SPI_Cmd_P;

	total_size = Stitch_Block_Max * 32;
	size_tmp = total_size;

	st_p = &ST_I[f_id][0];
	for(i = 0; i < size_tmp; i += 4000) {
		//write to f2
		if(f_id == 0) tmpaddr = (F2_0_ST_CMD_ADDR) + i;
		else 	   	  tmpaddr = (F2_1_ST_CMD_ADDR) + i;
		ua360_spi_ddr_write(tmpaddr,  (st_p+i), 4000);

		//f2 write to fx
		size = 4000 / 32;
		if(f_id == 0)
			SPI_Cmd_P.S_DDR_P = (F2_0_ST_CMD_ADDR + i) >> 5;
		else
			SPI_Cmd_P.S_DDR_P = (F2_1_ST_CMD_ADDR + i) >> 5;
		SPI_Cmd_P.T_DDR_P = (FX_ST_CMD_ADDR + i) >> 5;
		SPI_Cmd_P.Size = size;
		SPI_Cmd_P.Mode = 1;
		if(f_id == 0)
			SPI_Cmd_P.F_ID = AS2_MSPI_F0_ID;
		else
			SPI_Cmd_P.F_ID = AS2_MSPI_F1_ID;
		SPI_Cmd_P.Check_ID = 0xD5;
		SPI_Cmd_P.Rev = 0;

		if(SPI_Cmd_P.Size != 0)
			AS2_Write_F2_to_FX_CMD( (SPI_Cmd_P.S_DDR_P << 5), (SPI_Cmd_P.T_DDR_P << 5), (SPI_Cmd_P.Size << 5), SPI_Cmd_P.Mode, SPI_Cmd_P.F_ID, 1);
	}

	ST_Cmd_Cal_CheckSum[f_id] = Cal_DDR_CheckSum(size_tmp, st_p);
}

int send_Sitching_Sensor_Cmd(int f_id)
{
	int i, k, tmpaddr;
	int size, sum, total_size;
	unsigned char *st_p;
	int size_tmp;
	AS2_SPI_Cmd_struct SPI_Cmd_P;

	total_size = Stitch_Sesnor_Max * 32;                // Stitch_Sesnor_Max = 8192, total_size = 0x40000
	size_tmp = total_size;

	st_p = &ST_S[0];
	for(i = 0; i < size_tmp; i += 4000) {
		if(f_id == 0) tmpaddr = (F2_0_ST_S_CMD_ADDR) + i;
		else 	   	  tmpaddr = (F2_1_ST_S_CMD_ADDR) + i;
		ua360_spi_ddr_write(tmpaddr,  (st_p+i), 4000);

		size = 4000 / 32;
		if(f_id == 0)
			SPI_Cmd_P.S_DDR_P = (F2_0_ST_S_CMD_ADDR + i) >> 5;
		else
			SPI_Cmd_P.S_DDR_P = (F2_1_ST_S_CMD_ADDR + i) >> 5;
		SPI_Cmd_P.T_DDR_P = (FX_ST_S_CMD_ADDR + i) >> 5;
		SPI_Cmd_P.Size = size;
		SPI_Cmd_P.Mode = 1;
		if(f_id == 0)
			SPI_Cmd_P.F_ID = AS2_MSPI_F0_ID;
		else
			SPI_Cmd_P.F_ID = AS2_MSPI_F1_ID;
		SPI_Cmd_P.Check_ID = 0xD5;
		SPI_Cmd_P.Rev = 0;

		if(SPI_Cmd_P.Size != 0)
			AS2_Write_F2_to_FX_CMD( (SPI_Cmd_P.S_DDR_P << 5), (SPI_Cmd_P.T_DDR_P << 5), (SPI_Cmd_P.Size << 5), SPI_Cmd_P.Mode, SPI_Cmd_P.F_ID, 1);
	}

	ST_Sen_Cmd_Cal_CheckSum[f_id] = Cal_DDR_CheckSum(size_tmp, st_p);
}

int Send_MP_B_Table(int f_id, int idx)
{
	int i, tmpaddr;
	int size, total_size;
	unsigned char *st_p;
	int start_idx;
	int mode, size_tmp;
	AS2_SPI_Cmd_struct SPI_Cmd_P;

  	total_size = Stitch_Block_Max * 64;
	size_tmp = total_size;

	if(idx == 0) st_p = &MP_B[f_id][0];
	else		 st_p = &MP_B_2[f_id][0];
	for(i = 0; i < size_tmp; i += 4000) {
		if(idx == 0) {
			if(f_id == 0) tmpaddr = F2_0_MP_GAIN_ADDR + i;
			else          tmpaddr = F2_1_MP_GAIN_ADDR + i;
		}
		else {
			if(f_id == 0) tmpaddr = F2_0_MP_GAIN_2_ADDR + i;
			else          tmpaddr = F2_1_MP_GAIN_2_ADDR + i;
		}
		ua360_spi_ddr_write(tmpaddr,  (st_p+i), 4000);

  		size = 4000 / 32;
  		if(idx == 0) {
			if(f_id == 0) SPI_Cmd_P.S_DDR_P = (F2_0_MP_GAIN_ADDR + i) >> 5;
			else	      SPI_Cmd_P.S_DDR_P = (F2_1_MP_GAIN_ADDR + i) >> 5;
			SPI_Cmd_P.T_DDR_P = (FX_MP_GAIN_ADDR + i) >> 5;
  		}
  		else {
			if(f_id == 0) SPI_Cmd_P.S_DDR_P = (F2_0_MP_GAIN_2_ADDR + i) >> 5;
			else	      SPI_Cmd_P.S_DDR_P = (F2_1_MP_GAIN_2_ADDR + i) >> 5;
			SPI_Cmd_P.T_DDR_P = (FX_MP_GAIN_2_ADDR + i) >> 5;
  		}
  		SPI_Cmd_P.Size = size;
  		SPI_Cmd_P.Mode = 1;
  		if(f_id == 0)
  			SPI_Cmd_P.F_ID = AS2_MSPI_F0_ID;
  		else
  			SPI_Cmd_P.F_ID = AS2_MSPI_F1_ID;
  		SPI_Cmd_P.Check_ID = 0xD5;
  		SPI_Cmd_P.Rev = 0;

		if(SPI_Cmd_P.Size != 0)
			AS2_Write_F2_to_FX_CMD( (SPI_Cmd_P.S_DDR_P << 5), (SPI_Cmd_P.T_DDR_P << 5), (SPI_Cmd_P.Size << 5), SPI_Cmd_P.Mode, SPI_Cmd_P.F_ID, 1);
	}

	MP_B_Table_Cal_CheckSum[f_id][idx] = Cal_DDR_CheckSum(size_tmp, st_p);
}

int send_Sitching_Tran_Cmd(int f_id)
{
	int i, k, tmpaddr, idx/*=Sitching_Idx*/;
	int size, total_size;
	unsigned char *st_p;
	int start_idx;
	int mode, size_tmp;
	AS2_SPI_Cmd_struct SPI_Cmd_P;

	idx = 0;
	total_size = Stitch_Block_Max * 4;
	size_tmp = total_size;

	st_p = &Transparent_B[f_id][0][0];
	for(i = 0; i < size_tmp; i += 4000) {
		if(f_id == 0) tmpaddr = (F2_0_ST_TRAN_CMD_ADDR + idx * 0x80000) + i;
		else 	      tmpaddr = (F2_1_ST_TRAN_CMD_ADDR + idx * 0x80000) + i;
		ua360_spi_ddr_write(tmpaddr,  (st_p+i), 4000);

		size = 4000 / 32;
		if(f_id == 0)
			SPI_Cmd_P.S_DDR_P = (F2_0_ST_TRAN_CMD_ADDR + idx * 0x80000 + i) >> 5;
		else
			SPI_Cmd_P.S_DDR_P = (F2_1_ST_TRAN_CMD_ADDR + idx * 0x80000 + i) >> 5;
		SPI_Cmd_P.T_DDR_P = (FX_ST_TRAN_CMD_ADDR + idx * 0x80000 + i) >> 5;
		SPI_Cmd_P.Size = size;
		SPI_Cmd_P.Mode = 1;
		if(f_id == 0)
			SPI_Cmd_P.F_ID = AS2_MSPI_F0_ID;
		else
			SPI_Cmd_P.F_ID = AS2_MSPI_F1_ID;
		SPI_Cmd_P.Check_ID = 0xD5;
		SPI_Cmd_P.Rev = 0;

		if(SPI_Cmd_P.Size != 0)
			AS2_Write_F2_to_FX_CMD( (SPI_Cmd_P.S_DDR_P << 5), (SPI_Cmd_P.T_DDR_P << 5), (SPI_Cmd_P.Size << 5), SPI_Cmd_P.Mode, SPI_Cmd_P.F_ID, 1);
	}

	ST_Tran_Cmd_Cal_CheckSum[f_id] = Cal_DDR_CheckSum(size_tmp, st_p);
}

int send_Sitching_Cmd_3DModel(int f_id, int isInit)
{
	int i, j, tmpaddr;
	int size, total_size;
	unsigned char *st_p;
	int start_idx;
	int mode, size_tmp;
	int page, cnt=1;
	int offset = PLANT_ST_CMD_PAGE_OFFSET;
	int block_size;
	AS2_SPI_Cmd_struct SPI_Cmd_P;

	//if(isInit == 1) cnt = 1;
	//else			cnt = 2;		//往後多寫一個Page

	block_size = ST_3DModel_Header[ST_3DModel_Idx].Sum[ST_H_Img][f_id] << 5;
	total_size = (((block_size + 2047) >> 11) << 11);		//對齊2048 Byte
	if(total_size > (Stitch_3DModel_Max << 5) ) total_size = (Stitch_3DModel_Max << 5);
	size_tmp = 2048;

	for(j = 0; j < cnt; j++) {
		st_p = &ST_I_3DModel[ST_3DModel_Idx][f_id][0];
		page = ((ST_3DModel_Idx+j)&0x7);
		for(i = 0; i < total_size; i += size_tmp) {
			//write to f2
			if(f_id == 0) tmpaddr = F2_0_PLANT_ST_CMD_ADDR + page * offset + i;
			else 	   	  tmpaddr = F2_1_PLANT_ST_CMD_ADDR + page * offset + i;
			ua360_spi_ddr_write(tmpaddr,  (st_p+i), size_tmp);

			//f2 write to fx
			size = (size_tmp >> 5);
			if(f_id == 0)
				SPI_Cmd_P.S_DDR_P = (F2_0_PLANT_ST_CMD_ADDR + page * offset + i) >> 5;
			else
				SPI_Cmd_P.S_DDR_P = (F2_1_PLANT_ST_CMD_ADDR + page * offset + i) >> 5;
			SPI_Cmd_P.T_DDR_P = (FX_PLANT_ST_CMD_ADDR + page * offset + i) >> 5;
			SPI_Cmd_P.Size = size;
			SPI_Cmd_P.Mode = 1;
			if(f_id == 0)
				SPI_Cmd_P.F_ID = AS2_MSPI_F0_ID;
			else
				SPI_Cmd_P.F_ID = AS2_MSPI_F1_ID;
			SPI_Cmd_P.Check_ID = 0xD5;
			SPI_Cmd_P.Rev = 0;

			if(SPI_Cmd_P.Size != 0)
				AS2_Write_F2_to_FX_CMD( (SPI_Cmd_P.S_DDR_P << 5), (SPI_Cmd_P.T_DDR_P << 5), (SPI_Cmd_P.Size << 5), SPI_Cmd_P.Mode, SPI_Cmd_P.F_ID, 1);
		}
	}
}

int Send_MP_B_Table_3DModel(int f_id, int idx)
{
	int i, tmpaddr;
	int size, total_size;
	unsigned char *st_p;
	int start_idx;
	int mode, size_tmp;
	int page = ST_3DModel_Idx;
	int offset = PLANT_MP_GAIN_PAGE_OFFSET;
	AS2_SPI_Cmd_struct SPI_Cmd_P;

  	total_size = Stitch_3DModel_Max * 64;
	size_tmp = total_size;

	if(idx == 0) st_p = &MP_B_3DModel[page][f_id][0];
	else		 st_p = &MP_B_2_3DModel[page][f_id][0];
	for(i = 0; i < size_tmp; i += 2048) {
		if(idx == 0) {
			if(f_id == 0) tmpaddr = F2_0_PLANT_MP_GAIN_ADDR + page * offset + i;
			else          tmpaddr = F2_1_PLANT_MP_GAIN_ADDR + page * offset + i;
		}
		else {
			if(f_id == 0) tmpaddr = F2_0_PLANT_MP_GAIN_2_ADDR + page * offset + i;
			else          tmpaddr = F2_1_PLANT_MP_GAIN_2_ADDR + page * offset + i;
		}
		ua360_spi_ddr_write(tmpaddr,  (st_p+i), 2048);

  		size = 2048 / 32;
  		if(idx == 0) {
			if(f_id == 0) SPI_Cmd_P.S_DDR_P = (F2_0_PLANT_MP_GAIN_ADDR + page * offset + i) >> 5;
			else	      SPI_Cmd_P.S_DDR_P = (F2_1_PLANT_MP_GAIN_ADDR + page * offset + i) >> 5;
			SPI_Cmd_P.T_DDR_P = (FX_PLANT_MP_GAIN_ADDR + page * offset + i) >> 5;
  		}
  		else {
			if(f_id == 0) SPI_Cmd_P.S_DDR_P = (F2_0_PLANT_MP_GAIN_2_ADDR + page * offset + i) >> 5;
			else	      SPI_Cmd_P.S_DDR_P = (F2_1_PLANT_MP_GAIN_2_ADDR + page * offset + i) >> 5;
			SPI_Cmd_P.T_DDR_P = (FX_PLANT_MP_GAIN_2_ADDR + page * offset + i) >> 5;
  		}
  		SPI_Cmd_P.Size = size;
  		SPI_Cmd_P.Mode = 1;
  		if(f_id == 0)
  			SPI_Cmd_P.F_ID = AS2_MSPI_F0_ID;
  		else
  			SPI_Cmd_P.F_ID = AS2_MSPI_F1_ID;
  		SPI_Cmd_P.Check_ID = 0xD5;
  		SPI_Cmd_P.Rev = 0;

		if(SPI_Cmd_P.Size != 0)
			AS2_Write_F2_to_FX_CMD( (SPI_Cmd_P.S_DDR_P << 5), (SPI_Cmd_P.T_DDR_P << 5), (SPI_Cmd_P.Size << 5), SPI_Cmd_P.Mode, SPI_Cmd_P.F_ID, 1);
	}
}

int send_Sitching_Tran_Cmd_3DModel(int f_id)
{
	int i, k, tmpaddr;
	int size, total_size;
	unsigned char *st_p;
	int start_idx;
	int mode, size_tmp;
	int page = ST_3DModel_Idx;
	int offset = PLANT_TRAN_PAGE_OFFSET;
	AS2_SPI_Cmd_struct SPI_Cmd_P;

	total_size = Stitch_3DModel_Max * 4;
	size_tmp = total_size;

	st_p = &Transparent_B_3DModel[page][f_id][0][0];
	for(i = 0; i < size_tmp; i += 2048) {
		if(f_id == 0) tmpaddr = F2_0_PLANT_ST_TRAN_CMD_ADDR + page * offset + i;
		else 	      tmpaddr = F2_1_PLANT_ST_TRAN_CMD_ADDR + page * offset + i;
		ua360_spi_ddr_write(tmpaddr,  (st_p+i), 2048);

		size = 2048 / 32;
		if(f_id == 0)
			SPI_Cmd_P.S_DDR_P = (F2_0_PLANT_ST_TRAN_CMD_ADDR + page * offset + i) >> 5;
		else
			SPI_Cmd_P.S_DDR_P = (F2_1_PLANT_ST_TRAN_CMD_ADDR + page * offset + i) >> 5;
		SPI_Cmd_P.T_DDR_P = (FX_PLANT_ST_TRAN_CMD_ADDR + page * offset + i) >> 5;
		SPI_Cmd_P.Size = size;
		SPI_Cmd_P.Mode = 1;
		if(f_id == 0)
			SPI_Cmd_P.F_ID = AS2_MSPI_F0_ID;
		else
			SPI_Cmd_P.F_ID = AS2_MSPI_F1_ID;
		SPI_Cmd_P.Check_ID = 0xD5;
		SPI_Cmd_P.Rev = 0;

		if(SPI_Cmd_P.Size != 0)
			AS2_Write_F2_to_FX_CMD( (SPI_Cmd_P.S_DDR_P << 5), (SPI_Cmd_P.T_DDR_P << 5), (SPI_Cmd_P.Size << 5), SPI_Cmd_P.Mode, SPI_Cmd_P.F_ID, 1);
	}
}

short WDR_Live_Diffusion_Table[256];
float WDR_Live_Target_Table[1024];
char WDR_Diffusion_Table[4][256];   // [0][0] -> zero table
                                    // [1][0] -> function table, 透明度設定, =0:不透明, =255:全透明
                                    // [2][0] -> last table, PI0=64
									// [3][0] -> 0~255
    //
    //            _______ PV1 (透明)	255
    //           /
    //      ____/         PV0 (不透明)	0
    //          | |
    //         PI0|
    //           PI1 (亮度)
    //         0 ~ +63
    //
int WDRTablePixel = 15;		//30;     // 12K=30(預設), 8K=20, 6K=15, 擴散範圍
int WDR1PI0 = 0;                // 180921, 64改1
int WDR1PI1 = 159;              // 190422, 127改159
int WDR1PV0 = 64;
int WDR1PV1 = 255;
int WDR2PI0 = 96;               // 190422, 128改96
int WDR2PI1 = 255;
int WDR2PV0 = 255;
int WDR2PV1 = 64;
//int WDR_EXP_Mode = 3;       // 0:中-暗-更暗, 1:中-暗-亮, 2:亮-中-暗, 3:暗-中-亮

extern int hdrLevel;
void send_WDR_Diffusion_Table(int c_mode)
{
    int i, j, fid, i2;
    AS2_SPI_Cmd_struct SPI_Cmd_P;
    int diffx1024;
    int pi0=255, pi1=240, pv0=255, pv1=1;
    int hdr_strength;
    
    memset(&SPI_Cmd_P, 0, sizeof(SPI_Cmd_P));
    memset(&WDR_Diffusion_Table[0][0], 0, sizeof(WDR_Diffusion_Table[0]) );
    
    if(c_mode == 13) hdr_strength = get_Removal_HDR_Strength();
    else			 hdr_strength = get_HDR7P_Strength();
    // 真正有參數的WDR Table//
    for(j = 1; j < 4; j++){
        //if(WDR_EXP_Mode == 3){                         // 暗-中-亮 模式
            //if(j == 1){ pv0 = 64; pv1 = 255; pi0 = 0; pi1 = 127; }            // 長曝取暗部資料
            //if(j == 2){ pv0 = 255; pv1 = 64; pi0 = 128; pi1 = 255; }          // 短曝取亮部資料
            if(j == 1){
                pv1 = WDR1PV1; pi0 = WDR1PI0; pi1 = WDR1PI1;      // 長曝取暗部資料
                pv0 = 255 - hdr_strength * 255 / 100 ;
                //pi0 = WDR1PI0; pi1 = WDR1PI1;
                //pv0 = WDR1PV1; 
                //pv1 = 255 - hdr_strength * 255 / 100 ;
                /*
                pv0 = WDR1PV0;
                if(hdrLevel == 2){ pv0 = pv0 *2; }  // WDR弱
                if(hdrLevel == 8){ pv0 = 32; }      // WDR強
                */
            }
            else if(j == 2){
                pv0 = WDR2PV0; pi0 = WDR2PI0; pi1 = WDR2PI1;      // 短曝取亮部資料
                pv1 = 255 - hdr_strength * 255 / 100 ;
                /*
                pv1 = WDR2PV1;
                if(hdrLevel == 2){ pv1 = pv1 *2; }  // WDR弱
                if(hdrLevel == 8){ pv1 = 32; }      // WDR強
                */
            }
            if(pv0 > 255) pv0 = 255;
            if(pv1 > 255) pv1 = 255;
            if(pv0 < 0) pv0 = 0;
            if(pv1 < 0) pv1 = 0;
        //}
        diffx1024 = ((pv1<<10)-(pv0<<10)) / (pi1-pi0+1);
        
        for(i = 0; i < 256; i++){
        	if(j == 3)
        		WDR_Diffusion_Table[j][i] = i;
        	else {
				if     (i < pi0){ WDR_Diffusion_Table[j][i] = pv0; }
				else if(i > pi1){ WDR_Diffusion_Table[j][i] = pv1; }
				else            { WDR_Diffusion_Table[j][i] = pv0 + ((diffx1024 * (i-pi0)) >> 10); }
        	}
        }
    }
    ua360_spi_ddr_write(F2_WDR_TABLE_ADDR,  &WDR_Diffusion_Table[0], 2048);
    for(i = 0; i < 2; i++){
        if(i == 0) fid = AS2_MSPI_F0_ID;
        else       fid = AS2_MSPI_F1_ID;
        AS2_Write_F2_to_FX_CMD( F2_WDR_TABLE_ADDR, FX_WDR_TABLE_ADDR, 1024, 1, fid, 1);
    }
}
int WDR_Live_Strength = 0;	//0:強 	1:中 	2:弱
void SetWDRLiveStrength(int value)
{
	WDR_Live_Strength = value;
	send_WDR_Live_Diffusion_Table();
}

/*
 *  tb size -> char [256]
 */
/*void Make_WDR_Live_Table(short *tb, int mode)
{
	int i;
	int T_Table[256];
	int L_Table[256];
	int temp;
	int LV,HV,Step;
	switch(mode){
		case 0: LV = 70; HV = 150; break;	//Strong
		case 1: LV = 48; HV = 180; break;       //Medium
		case 2: LV = 32; HV = 200; break;       //Week
	}
	T_Table[0] = (LV << 8);
	T_Table[255] = (HV << 8);
	Step = HV - LV;
	for(i = 1 ; i < 255 ; i++){
		T_Table[i] = T_Table[i - 1] + Step;
	}
	for(i = 0 ; i < 256 ; i++){
		L_Table[i] = T_Table[i] - (i << 8) ;
	}
	for(i = 0 ; i < 256 ; i++){
		temp = (L_Table[i] >> 4) + 0x800;     // *16/256, 0x800 = 0, 0xFFF = +127, 0x000 = -128
		tb[i] = temp;
		if(temp > 4095) tb[i] = 4095;
		if(temp < 0   ) tb[i] = 0;
	}
}*/
int Send_WDR_Live_Table_Flag = 0;
void Gamma_2_WDR_Table(float *sb, short *tb)
{
	int i, j, j2;
	float s1, s2, tv;
	float sub_rate;
	int gamma_max = GetGammaMax();

	tb[0] = 0;
	for(i = 1; i < 256; i++) {
		if(i >= gamma_max) {
			tb[i] = tb[gamma_max-1];
			//db_debug("Gamma_2_WDR_Table() 00 gamma_max=%d tb[max]=0x%x  i=%d tb=0x%x\n", gamma_max, tb[gamma_max], i, tb[i]);
			continue;
		}
		for(j = 0; j < 1024; j++) {
			s1 = sb[j];
			if(s1 >= i) {
				if(j == 0) j2 = 0;
				else       j2 = j-1;
				s2 = sb[j2];
				if(s1 == s2)
					sub_rate = 0;
				else
					sub_rate = (s1 - (float)i) / (s1 - s2);
				tv = sub_rate * j2 + (1.0 - sub_rate) * j;
				tb[i] = i * 0x1000 / tv / 2;
				if(tb[i] < 0x200) tb[i] = 0x200;		//Gamma不可小餘1倍(0x1000), 反算回去WDR Table不可小於0x200, 0x200 = 0.25 * 0x1000 / 2

				if(i > (gamma_max-5) )
					db_debug("Gamma_2_WDR_Table() 01 i=%d j=%d s1=%f s2=%f sub_rate=%f tv=%f tb=0x%x\n",
							i, j, s1, s2, sub_rate, tv, tb[i]);
				break;
			}
		}
	}
}
void send_WDR_Live_Diffusion_Table(void)
{
    int i, fid;

    memset(&WDR_Live_Diffusion_Table[0], 0, sizeof(WDR_Live_Diffusion_Table[0]));

    //Make_WDR_Live_Table(&WDR_Live_Diffusion_Table[0], WDR_Live_Strength);
    Gamma_2_WDR_Table(&WDR_Live_Target_Table[0], &WDR_Live_Diffusion_Table[0]);

    ua360_spi_ddr_write(F2_WDR_LIVE_TABLE_ADDR,  &WDR_Live_Diffusion_Table[0], 2048);
    for(i = 0; i < 2; i++){
        if(i == 0) fid = AS2_MSPI_F0_ID;
        else       fid = AS2_MSPI_F1_ID;
        AS2_Write_F2_to_FX_CMD( F2_WDR_LIVE_TABLE_ADDR, FX_WDR_LIVE_TABLE_ADDR, 512, 1, fid, 1);
    }
}
// call from: k_spi_cmd.c & StateTool.java
int getWDRTablePixel(int m_mode, int rec_mode)
{
    int pixel, pixel_tmp = WDRTablePixel;
    //if(rec_mode == 7) pixel_tmp = (WDRTablePixel >> 1);   // 30 -> 15 // rex- 181221, 日夜WDR擴散範圍都改設30
    //else              pixel_tmp = WDRTablePixel;

    switch(m_mode){
    default: pixel = pixel_tmp; break;
    case 1: pixel = (pixel_tmp/3)*2; break;    // 8K
    case 2: pixel = (pixel_tmp>>1); break;     // 6K
    }
    if(pixel < 5) pixel = 5;
    return pixel;
}
#ifdef ANDROID_CODE
void setWDRTablePixel(int pix){ WDRTablePixel = pix; }   // 145
int doGetWDRTablePixel(){ return getWDRTablePixel(0, 5); }
void setWDR1PI0(int pi0){ WDR1PI0 = pi0; }       // 146
void setWDR1PI1(int pi1){ WDR1PI1 = pi1; }       // 147
void setWDR1PV0(int pv0){ WDR1PV0 = pv0; }       // 148
void setWDR1PV1(int pv1){ WDR1PV1 = pv1; }       // 149
int getWDR1PI0(){ return WDR1PI0; }
int getWDR1PI1(){ return WDR1PI1; }
int getWDR1PV0(){ return WDR1PV0; }
int getWDR1PV1(){ return WDR1PV1; }
void setWDR2PI0(int pi0){ WDR2PI0 = pi0; }       // 146
void setWDR2PI1(int pi1){ WDR2PI1 = pi1; }       // 147
void setWDR2PV0(int pv0){ WDR2PV0 = pv0; }       // 148
void setWDR2PV1(int pv1){ WDR2PV1 = pv1; }       // 149
int getWDR2PI0(){ return WDR2PI0; }
int getWDR2PI1(){ return WDR2PI1; }
int getWDR2PV0(){ return WDR2PV0; }
int getWDR2PV1(){ return WDR2PV1; }

#endif


int do_ST_Table_Mix_En = 1;
void setSTMixEn(int en)
{
	do_ST_Table_Mix_En = en;
}
int getSTMixEn(void)
{
	return do_ST_Table_Mix_En;
}

/*
 * 下一次執行一次
 */
void do_ST_Table_Mix(int M_Mode)
{
	int i, j;
	int buf[12][2] = {0};		//[12][2], 寫DDR需對齊32
	int idx1, idx2;

	memset(&buf[0][0], 0, sizeof(buf) );

	if(do_ST_Table_Mix_En == 0) return;

	if(M_Mode == 5) {
		idx1 = ST_Header[M_Mode].Start_Idx[ST_H_Img];
		idx2 = Stitch_Block_Max;
	}
	else {
		idx1 = ST_Header[M_Mode].Start_Idx[ST_H_Img];
		idx2 = ST_Header[M_Mode+1].Start_Idx[ST_H_Img];
	}

	for(i = 0; i < 11; i++) {
		switch(i) {
		//mix 1
		case 0: buf[i][0] = SPI_IO_SMOOTH_PHI_ADDR;
				buf[i][1] = ( (FX_SPI_IO_SMOOTH_PHI_DATA + (M_Mode << 11) ) >> 5);	//(M_Mode * 512 * 4)
				break;
		case 1: buf[i][0] = SPI_IO_MP_BASE_ADDR;
				buf[i][1] = ( (SPI_IO_MP_BASE_DATA + (idx1 << 5) ) >> 5);
				break;
		case 2: buf[i][0] = SPI_IO_MP_GAIN_ADDR;
				buf[i][1] = ( (SPI_IO_MP_GAIN_DATA + (idx1 << 6) ) >> 5);
				break;
		case 3: buf[i][0] = SPI_IO_MP_SIZE_ADDR;
				buf[i][1] = ( (idx2 - idx1) >> 2);
				break;
		case 4: buf[i][0] = SPI_IO_MP_ST_O_ADDR;
				buf[i][1] = ( (SPI_IO_MP_ST_O_P0_DATA + (idx1 << 5) ) >> 5);
				break;
		//mix 2
		case 5: buf[i][0] = SPI_IO_SMOOTH_PHI_2_ADDR;
				buf[i][1] = ( (FX_SPI_IO_SMOOTH_PHI_DATA + (M_Mode << 11) ) >> 5);
				break;
		case 6: buf[i][0] = SPI_IO_MP_BASE_2_ADDR;
				buf[i][1] = ( (SPI_IO_MP_BASE_2_DATA + (idx1 << 5) ) >> 5);
				break;
		case 7: buf[i][0] = SPI_IO_MP_GAIN_2_ADDR;
				buf[i][1] = ( (SPI_IO_MP_GAIN_2_DATA + (idx1 << 6) ) >> 5);
				break;
		case 8: buf[i][0] = SPI_IO_MP_SIZE_2_ADDR;
				buf[i][1] = ( (idx2 - idx1) >> 2);
				break;
		case 9: buf[i][0] = SPI_IO_MP_ST_O_2_ADDR;
				buf[i][1] = ( (SPI_IO_MP_ST_O_2_P0_DATA + (idx1 << 5) ) >> 5);
				break;
		case 10:buf[i][0] = SPI_IO_MP_ST_O_EN_ADDR;
				buf[i][1] = 0;
				break;
		}
	}

	int Addr, size, s_addr, t_addr, spi_mode, id;
	int ret;
    Addr = ST_CMD_MIX_ADDR;
	ret = ua360_spi_ddr_write(Addr, &buf[0][0], sizeof(buf) );

	for(i = 0; i < 2; i++) {
		s_addr = ST_CMD_MIX_ADDR;
		t_addr = 0;
		size = sizeof(buf);
		spi_mode = 0;
		if(i == 0) id = AS2_MSPI_F0_ID;
		else       id = AS2_MSPI_F1_ID;
		AS2_Write_F2_to_FX_CMD(s_addr, t_addr, size, spi_mode, id, 1);
	}

}

int send_Sitching_Cmd_Test(void)
{
	int i, j, tmpaddr, idx;
	int size, sum, Background_P=128;

	idx = 0;
	for(j = 0; j < 2; j++) {
		if(ST_Sum_Test[j] == 0) {
			db_debug("send_Sitching_Cmd_Test() Sum==0 (j=%d)\n", j);
			continue;
		}
		size = (ST_Sum_Test[j] * 32);
		for(i = 0; i < size; i += 2048) {
			if(j == 0) tmpaddr = (F2_0_TEST_ST_CMD_ADDR + idx * 0x80000) + i;
			else 	   tmpaddr = (F2_1_TEST_ST_CMD_ADDR + idx * 0x80000) + i;
			ua360_spi_ddr_write(tmpaddr,  (int *)&ST_Test[j][i / 32], 2048);
		}
	}

	AS2_SPI_Cmd_struct SPI_Cmd_P;
	unsigned *Pxx;
  	unsigned MSPI_D[10][3];

  	int tmp, size_t, size_s;
  	Pxx = (unsigned *) &SPI_Cmd_P;
  	for(i = 0; i < 2; i++) {
  		size_t = 0;
  		size_s = ( (ST_Sum_Test[i] / 5) & 0xFFFFFFF0);
  		for(j = 0; j < 5; j++) {
  			tmp = (i*5+j);
  			if(j == 4) size = ST_Sum_Test[i] - size_t;
  			else       size = size_s;

  			if(i == 0)
  				SPI_Cmd_P.S_DDR_P = (F2_0_TEST_ST_CMD_ADDR + idx * 0x80000 + size_t * 32) >> 5;
  			else
  				SPI_Cmd_P.S_DDR_P = (F2_1_TEST_ST_CMD_ADDR + idx * 0x80000 + size_t * 32) >> 5;
  			SPI_Cmd_P.T_DDR_P = (FX_TEST_ST_CMD_ADDR + idx * 0x80000 + size_t * 32) >> 5;
  			SPI_Cmd_P.Size = size;
  			SPI_Cmd_P.Mode = 1;
  			if(i == 0)
  				SPI_Cmd_P.F_ID = AS2_MSPI_F0_ID;
  			else
  				SPI_Cmd_P.F_ID = AS2_MSPI_F1_ID;
  			if(ST_Sum_Test[i] == 0)
  				SPI_Cmd_P.Check_ID = 0x00;
  			else
  				SPI_Cmd_P.Check_ID = 0xD5;
  			SPI_Cmd_P.Rev = 0;

  			MSPI_D[tmp][0] = Pxx[0];
  			MSPI_D[tmp][1] = Pxx[1];
  			MSPI_D[tmp][2] = Pxx[2];

  			if(SPI_Cmd_P.Size != 0)
  				AS2_Write_F2_to_FX_CMD( (SPI_Cmd_P.S_DDR_P << 5), (SPI_Cmd_P.T_DDR_P << 5), (SPI_Cmd_P.Size << 5), SPI_Cmd_P.Mode, SPI_Cmd_P.F_ID, 1);

  			size_t += size;
  		}
  	}
}

#endif

//-------------------------Miller 2017/01/10---------------------------------


struct Resolution_Mode_Struct  Resolution_Table[12] = {
  {  1920, 1080,  6000000 },
  { 15360, 7680, 48000000 },
  {  3840, 1920,  8400000 },
  {  1920, 1080,  6000000 },
  {  1280,  720,  4000000 },
  {  3840, 2160,  2000000 },
  {  7680, 1920,  8400000 },
  {  7680, 3840, 12900000 },
  {  7680, 4320, 18000000 },
  { 15360, 3840, 48000000 },
  {   960,  704,  3000000 },
  {  3072,  800,  8400000 }
};

struct Resolution_Mode_Struct View_Resolution;

//Model View Project
//float View_Phi[49][97];
//float View_Thita[49][97];

//void MVP_Proc(int M_Mode, int SX, int SY,  unsigned short I_thita0, unsigned short I_phi0, unsigned short I_rotate0)
//{
//  unsigned short I_thita35, I_phi35;
//  int i,j,k;
//  float thita1, phi1;
//
//  for (i =  0; i <= SY; i ++) {
//    for (j = 0 ; j <= SX ; j ++) {
//      phi1   = View_Phi[i][j];
//      thita1 = View_Thita[i][j];
//      Map_3D_Rotate( phi1, thita1, I_phi0, I_thita0, I_rotate0, &I_phi35, &I_thita35);
//      Map_Posi_Tmp[i][j].I_phi   = I_phi35;
//      Map_Posi_Tmp[i][j].I_thita = I_thita35;
//      Map_To_Sensor_Trans(0,&Map_Posi_Tmp[i][j]);
//    }
//  }
//}



void IniSensorAdj(void)
{
	int i, size;
		//int Top, Btm, Top_Idx, Btm_Idx;
		FILE *fp=NULL;
		struct stat sti;

		fp = fopen("/mnt/sdcard/US360/Test/A_L_I2_D.bin", "rb");
		if(fp != NULL) {
			stat("/mnt/sdcard/US360/Test/A_L_I2_D.bin", &sti);
			size = sti.st_size;
			//if(size <= 0) db_error("A_L_I2_D.bin not found !\n");
			if(size <= sizeof(A_L_I2) )
				fread(&A_L_I2, size, 1, fp);
			else
				fread(&A_L_I2, sizeof(A_L_I2), 1, fp);
			fclose(fp);
		}
	memset(&Adj_Sensor_Command,0,sizeof(Adj_Sensor_Command));
	memset(&Lens_Rate_Line,0,sizeof(Lens_Rate_Line));
	memset(&Trans_ZY_thita,0,sizeof(Trans_ZY_thita));
	memset(&Trans_ZY_phi,0,sizeof(Trans_ZY_phi));
	memset(&Trans_Sin,0,sizeof(Trans_Sin));
	memset(&PP_Temp,0,sizeof(PP_Temp));
	memset(&SL,0,sizeof(SL));
	memset(&ISP2_Image_Table,0,sizeof(ISP2_Image_Table));
	memset(&ST_I,0,sizeof(ST_I));
	memset(&SLT,0,sizeof(SLT));
	memset(&A_L_S2,0,sizeof(A_L_S2));
	memset(&ST_Sum,0,sizeof(ST_Sum));
}


int Map_sensor_id[6][30][60];
int Map_sensor_trans[5][31][61];

void Make_Map_sensor_transparent1(void)
{

  int i,j,k;
  for (i = 0; i < 30; i++) {
    for (j = 0; j < 60; j++) {
      for (k = 0; k < 5 ; k++) {
	Map_sensor_id[k][i][j] = 0;
	Map_sensor_trans[k][i][j] = 0;
      }
    }
  }

  for (i = 0; i < 30; i++) {
    for (j = 0; j < 60; j++) {
       k = Map_sensor_id[5][i][j];
       Map_sensor_id[k][i][j] = 1;
    }
  }

  for (i = 0; i < 30; i++) {
    for (j = 0; j < 60; j++) {
      for (k = 0; k < 5 ; k++) {
	if ( Map_sensor_id[k][i][j] == 1) {
	  Map_sensor_trans[k][i  ][j  ] =  9;
	  Map_sensor_trans[k][i+1][j  ] =  9;
	  Map_sensor_trans[k][i  ][j+1] =  9;
	  Map_sensor_trans[k][i+1][j+1] =  9;
	}
      }
    }
  }
}


void Make_Map_sensor_id(void)
{
    int i,j;
    // Sensor 0;
    for (i = 0; i < 30; i++) {
      for (j = 0; j < 60; j++) {
	Map_sensor_id[5][i  ][j  ] = 0;
      }
    }
    for (i = 10; i < 30; i++) {
      for (j = 0; j < 30; j++) {
	Map_sensor_id[5][i  ][j   ] = 4;
	Map_sensor_id[5][i  ][j+30] = 2;
      }
    }
    for (i = 7; i < 26; i++) {
      for (j = 0; j < 15; j++) {
	Map_sensor_id[5][i][j+23       ] = 1;
	Map_sensor_id[5][i][(j+53) % 60] =  3;
      }
    }

     Make_Map_sensor_transparent1();
}


// 20170708 Miller

int  Map_Transparent[5][91][181];

// Map Line
#define ML_gap     0x10


void Make_Transparent(int idx, struct XY_Posi * XY_P)
{
 short  I_phi; unsigned I_thita; int i; int j;
 int ML_phi0, ML_phi1, ML_phi2;
 int ML_thita0, ML_thita1, ML_thita2, ML_thita3, ML_thita4, ML_thita5;

 //水平縫合線
 ML_phi0 = A_L_I3_Header[idx].Phi_P[0] * 0x10000 /  A_L_I3_Header[idx].H_Blocks;
 ML_phi1 = A_L_I3_Header[idx].Phi_P[1] * 0x10000 /  A_L_I3_Header[idx].H_Blocks;
 ML_phi2 = A_L_I3_Header[idx].Phi_P[2] * 0x10000 /  A_L_I3_Header[idx].H_Blocks;

 //垂直縫合線, 0x8000=180, 0x10000=360
 ML_thita0 = A_L_I3_Header[idx].Thita_P[0] * 0x10000 /  A_L_I3_Header[idx].H_Blocks;
 ML_thita1 = A_L_I3_Header[idx].Thita_P[1] * 0x10000 /  A_L_I3_Header[idx].H_Blocks;
 ML_thita2 = 0x8000;
 ML_thita3 = A_L_I3_Header[idx].Thita_P[2] * 0x10000 /  A_L_I3_Header[idx].H_Blocks;
 ML_thita4 = A_L_I3_Header[idx].Thita_P[3] * 0x10000 /  A_L_I3_Header[idx].H_Blocks;
 ML_thita5 = 0x10000;

 I_phi   = (0x4000 - XY_P->I_phi) & 0xFFFF;
 I_thita = (XY_P->I_thita - 0x8000) & 0xFFFF;

 for (i = 0; i < 5; i++)  XY_P->Transparent[i] = 0;
// Sensor 0
 if (I_phi < ML_phi0 + ML_gap) XY_P->Transparent[0] = 1;
 if (I_phi < ML_phi1 + ML_gap) {
   if ((I_thita > ML_thita0 - ML_gap) && (I_thita < ML_thita1 + ML_gap)) XY_P->Transparent[0] = 1;
   if ((I_thita > ML_thita3 - ML_gap) && (I_thita < ML_thita4 + ML_gap)) XY_P->Transparent[0] = 1;
 }
// Sensor 1,3
 if ((I_phi > ML_phi0 - ML_gap) && (I_phi < ML_phi2 + ML_gap)) {
   if ((I_thita > ML_thita1 - ML_gap) && (I_thita < ML_thita3 + ML_gap)) XY_P->Transparent[1] = 1;
   if ((I_thita > ML_thita4 - ML_gap) || (I_thita < ML_thita0 + ML_gap)) XY_P->Transparent[3] = 1;
 }

// Sensor 2,4
 if (I_phi > ML_phi1 - ML_gap)  {
   if ((I_thita > ML_thita0 - ML_gap) && (I_thita < ML_thita1 + ML_gap)) XY_P->Transparent[2] = 1;
   if ((I_thita > ML_thita3 - ML_gap) && (I_thita < ML_thita4 + ML_gap)) XY_P->Transparent[4] = 1;
 }
 if (I_phi > ML_phi2 - ML_gap)  {

   if (I_thita < ML_thita2 + ML_gap)  XY_P->Transparent[2] = 1;
   if (I_thita > ML_thita2 - ML_gap)  XY_P->Transparent[4] = 1;

   if (I_thita < 0x1000) {
     I_thita += 0x10000;
     if ((I_thita < ML_thita5 + ML_gap) && (I_thita > ML_thita5 - ML_gap)) {
       XY_P->Transparent[2] = 1;  XY_P->Transparent[4] = 1;
     }
   }
  }
}



void Make_ALI2(void)
{
  int i,j; int S_Len;
  int S_Id; int S_X; int S_Y;
  struct Adjust_Line_I2_Struct   *p1;
  struct Adjust_Line_I2_Struct   *p2;
  struct XY_Posi                 *p3a, *p3b;
  int Delta_phi,Delta_thita;
  int phi_F,thita_F;

  memcpy(&A_L_I2[0], &A_L_I2_Default[0], sizeof(A_L_I2) );
#ifndef ANDROID_CODE
  for(i = 0; i < Adjust_Line_I2_MAX; i++) {
	A_L_I2[i].Distance   = A_L_I2_File[i].Distance;
	A_L_I2[i].Height     = A_L_I2_File[i].Height;
	A_L_I2[i].XY_P[0][0] = A_L_I2_File[i].XY_P[0][0];
	A_L_I2[i].XY_P[0][1] = A_L_I2_File[i].XY_P[0][1];
	A_L_I2[i].XY_P[1][0] = A_L_I2_File[i].XY_P[1][0];
	A_L_I2[i].XY_P[1][1] = A_L_I2_File[i].XY_P[1][1];
  }
#endif

  for (i = 0; i < Adjust_Line_I2_MAX; i++) {
    p1  = &A_L_I2[i];
    for (j = 0; j < 2; j++) {
      p3a = &p1->DP[0][j];
      p3a->I_phi   = p1->F_phi;
      p3a->I_thita = p1->F_thita;
    }
  }

  int Input_Scale = Get_Input_Scale();
  Delta_phi   = 0x8000 / (180 / Input_Scale);
  Delta_thita = 0x8000 / (180 / Input_Scale);

  for (i = 0; i < Adjust_Line_I2_MAX; i++) {
    p1  = &A_L_I2[i];
    p2  = &A_L_I2[i];
    p1->Top_Sensor       = p2->S_Id[0];
    p1->Bottom_Sensor    = p2->S_Id[1];
    p1->Top_A_L_S_Idx    = p2->A_L_S_Idx[0];
    p1->Bottom_A_L_S_Idx = p2->A_L_S_Idx[1];

    p1->G_t_p[4].phi     = (short)p2->F_phi;
    p1->G_t_p[4].thita   = (short)p2->F_thita;


    for (j = 0; j <  4; j++) {
	phi_F = (j & 2) - 1;  thita_F = ((j  << 1) & 2) - 1;
	A_L_I2[i].G_t_p[j].phi   = A_L_I2[i].G_t_p[4].phi   + phi_F * Delta_phi ;
	A_L_I2[i].G_t_p[j].thita = A_L_I2[i].G_t_p[4].thita + thita_F * Delta_thita;
    }
  }

  S_Rotate_R_Init(3);
  for (i = 0; i < Adjust_Line_I2_MAX; i++) {
	for (j = 0; j < 5 ; j++) {
			A_L_I2[i].S_XY[j].I_phi   =  A_L_I2[i].G_t_p[j].phi;
			A_L_I2[i].S_XY[j].I_thita =  A_L_I2[i].G_t_p[j].thita;

  			for (S_Id = 0; S_Id < 5; S_Id++) {
  				Global2Sensor(3,S_Id, A_L_I2[i].S_XY[j].I_phi, A_L_I2[i].S_XY[j].I_thita, G_Degree);
				A_L_I2[i].S_XY[j].S[S_Id] = PP_Temp.S[S_Id];
			}

   }
	A_L_I2[i].S_t_p1 = PP_Temp.S_t_p[ A_L_I2[i].Top_Sensor] ;
	A_L_I2[i].S_t_p2 = PP_Temp.S_t_p[ A_L_I2[i].Bottom_Sensor] ;
 }

}

//void cal_center_y()
//{
//	int i;
//	int sensor, num, type;
//	struct Adjust_Line_I2_Struct   *p1;
//
//	int idx1, idx2;
//	int s1, s2;
//	int y1, y2;
//	int y1_t, y2_t;
//	for(i = 0; i < Adjust_Line_I2_MAX; i++) {
//		p1 = &A_L_I2[i];
//
//		s1 = p1->S_Id[0];
//		y1 = p1->Sum_Y[0];
//		s2 = p1->S_Id[1];
//		y2 = p1->Sum_Y[1];
//
//		idx1 = p1->A_L_S_Idx[0];
//		idx2 = p1->A_L_S_Idx[1];
//
//#ifdef S_LENS_USER_128
//		y1_t = Brightness256to1024( (y1 / 128 / 128) );
//		y2_t = Brightness256to1024( (y2 / 128 / 128) );
//#else
//		y1_t = Brightness256to1024( (y1 / 256) );
//		y2_t = Brightness256to1024( (y2 / 256) );
//#endif
//
//		A_L_S2[s1].Test_Block_Y[idx1] = ( (float)y2_t / (float)y1_t - 1.0) / 2.0;
//		A_L_S2[s2].Test_Block_Y[idx2] = ( (float)y1_t / (float)y2_t - 1.0) / 2.0;
//	}
//}

#ifdef ANDROID_CODE
int doAutoStitch(void)
{
	int i, j, size;
	FILE *fp=NULL;
	char path[128];
	struct stat sti;

	Set_Calibration_Flag(1);
//tmp	paintOLEDStitching(1);

	//00.Check File
	sprintf(path, "/mnt/sdcard/US360/Test/ALI2_Result.bin\0");
	if(stat(path, &sti) != 0) { goto error; }


	//01.Init
	for (i = 0; i < 5; i++){
		Adj_Sensor_Command[i] = Adj_Sensor_Default[LensCode][i];
		Adj_Sensor[LensCode][i] = Adj_Sensor_Default[LensCode][i];
	}
	Stitch_Init_All();


	//02.Read File: ALI2_Result.bin
	fp = fopen(path, "rb");
	if(fp != NULL) {
		size = sti.st_size;
		if(size == sizeof(struct Adjust_Line_I2_Struct )*Adjust_Line_I2_MAX ){
			fread(&A_L_I2[0], sizeof(struct Adjust_Line_I2_Struct )*Adjust_Line_I2_MAX, 1, fp);
			fclose(fp);
		}
		else{
			db_error("ALI2_Result.bin size error !\n");
			fclose(fp);
			goto error;
		}
	}
	else { goto error; }


	//03.Calibration
//tmp	paintOLEDStitching(2);
	Camera_Calibration();


//tmp	paintOLEDStitching(3);
	Set_Calibration_Flag(0);
	db_debug("doAutoStitch success !\n");
	return 0;

error:
//tmp	paintOLEDStitching(0);
	Set_Calibration_Flag(0);
	db_error("doAutoStitch error !\n");
	return -1;
}
#endif


void setSmooth(int idx, int yuvz, int value, int f_id)
{
	int v_tmp, M_Mode, S_Mode;

	Get_M_Mode(&M_Mode, &S_Mode);
	switch(yuvz) {
	case 0: 	//y
		if(value < 0) {
			Smooth_O[f_id][S_Mode][idx].F_Y = 1;
			Smooth_O[f_id][S_Mode][idx].Y = -(value);
		}
		else {
			Smooth_O[f_id][S_Mode][idx].F_Y = 0;
			Smooth_O[f_id][S_Mode][idx].Y = value;
		}
		break;
	case 1:		//u
		if(value < 0) {
			Smooth_O[f_id][S_Mode][idx].F_U = 1;
			Smooth_O[f_id][S_Mode][idx].U = -(value);
		}
		else {
			Smooth_O[f_id][S_Mode][idx].F_U = 0;
			Smooth_O[f_id][S_Mode][idx].U = value;
		}
		break;
	case 2:		//v
		if(value < 0) {
			Smooth_O[f_id][S_Mode][idx].F_V = 1;
			Smooth_O[f_id][S_Mode][idx].V = -(value);
		}
		else {
			Smooth_O[f_id][S_Mode][idx].F_V = 0;
			Smooth_O[f_id][S_Mode][idx].V = value;
		}
		break;
	case 3:		//z
		if(value < 0) {
			Smooth_O[f_id][S_Mode][idx].F_Z = 1;
			Smooth_O[f_id][S_Mode][idx].Z = -(value);
		}
		else {
			Smooth_O[f_id][S_Mode][idx].F_Z = 0;
			Smooth_O[f_id][S_Mode][idx].Z = value;
		}
		break;
	}
}

void getSmooth(int idx, int yuvz, int *val, int f_id)
{
	int v_tmp, M_Mode, S_Mode;

	Get_M_Mode(&M_Mode, &S_Mode);
	switch(yuvz){
	case 0:		//y
		if(Smooth_O[f_id][S_Mode][idx].F_Y == 1)
			v_tmp = -(Smooth_O[f_id][S_Mode][idx].Y);
		else
			v_tmp = Smooth_O[f_id][S_Mode][idx].Y;
		break;
	case 1:		//u
		if(Smooth_O[f_id][S_Mode][idx].F_U == 1)
			v_tmp = -(Smooth_O[f_id][S_Mode][idx].U);
		else
			v_tmp = Smooth_O[f_id][S_Mode][idx].U;
		break;
	case 2:		//v
		if(Smooth_O[f_id][S_Mode][idx].F_V == 1)
			v_tmp = -(Smooth_O[f_id][S_Mode][idx].V);
		else
			v_tmp = Smooth_O[f_id][S_Mode][idx].V;
		break;
	case 3:		//z
		if(Smooth_O[f_id][S_Mode][idx].F_Z == 1)
			v_tmp = -(Smooth_O[f_id][S_Mode][idx].Z);
		else
			v_tmp = Smooth_O[f_id][S_Mode][idx].Z;
		break;
	}
	*val = v_tmp;
}

int ST_Table_Mix(int SF_A, int S_A, int F_A, int V_A, int SF_B, int S_B, int F_B, int V_B)
{
	int Value_A, Value_B, Value;

	if(SF_A == F_A) Value_A = S_A * V_A;
	else            Value_A = -(S_A * V_A);

	if(SF_B == F_B) Value_B = S_B * V_B;
	else            Value_B = -(S_B * V_B);

	Value = ( (Value_A + Value_B) / 0x4000);

	return Value;
}

void Add_ST_O_Table_Proc(void)
{
/*	int i, j;
	int f_id, m_mode, start_idx, idx_size;
	int SFZ_A, SZ_A, SFY_A, SY_A, SFU_A, SU_A, SFV_A, SV_A;
	int SFZ_B, SZ_B, SFY_B, SY_B, SFU_B, SU_B, SFV_B, SV_B;
	int F_A, V_A, P_A;
	int F_B, V_B, P_B;
	int YUV_Tmp;

	for(f_id = 0; f_id < 2; f_id++) {
		for(m_mode = 0; m_mode < 6; m_mode++) {

			start_idx = ST_Header[m_mode].Start_Idx[ST_H_Img];
			idx_size  = ST_Header[m_mode].Start_Idx[ST_H_Trans];	// - ST_Header[m_mode].Start_Idx[0];
			for(i = start_idx; i < idx_size; i++) {

				//XY0
				P_A = MP_B[f_id][i].MP_A0.P;
				P_B = MP_B[f_id][i].MP_B0.P;
				SFZ_A = Smooth_O[m_mode][P_A].F_Z;
				SZ_A  = Smooth_O[m_mode][P_A].Z;
				SFZ_B = Smooth_O[m_mode][P_B].F_Z;
				SZ_B  = Smooth_O[m_mode][P_B].Z;

				F_A = MP_B[f_id][i].MP_A0.F_X;
				V_A = MP_B[f_id][i].MP_A0.V_X;
				F_B = MP_B[f_id][i].MP_B0.F_X;
				V_B = MP_B[f_id][i].MP_B0.V_X;
				ST_O[f_id][i].CB_Posi_X0 = ST_I[f_id][i].CB_Posi_X0 + ST_Table_Mix(SFZ_A, SZ_A, F_A, V_A, SFZ_B, SZ_B, F_B, V_B);

				F_A = MP_B[f_id][i].MP_A0.F_Y;
				V_A = MP_B[f_id][i].MP_A0.V_Y;
				F_B = MP_B[f_id][i].MP_B0.F_Y;
				V_B = MP_B[f_id][i].MP_B0.V_Y;
				ST_O[f_id][i].CB_Posi_Y0 = ST_I[f_id][i].CB_Posi_Y0 + ST_Table_Mix(SFZ_A, SZ_A, F_A, V_A, SFZ_B, SZ_B, F_B, V_B);

				//XY1
				P_A = MP_B[f_id][i].MP_A1.P;
				P_B = MP_B[f_id][i].MP_B1.P;
				SFZ_A = Smooth_O[m_mode][P_A].F_Z;
				SZ_A  = Smooth_O[m_mode][P_A].Z;
				SFZ_B = Smooth_O[m_mode][P_B].F_Z;
				SZ_B  = Smooth_O[m_mode][P_B].Z;

				F_A = MP_B[f_id][i].MP_A1.F_X;
				V_A = MP_B[f_id][i].MP_A1.V_X;
				F_B = MP_B[f_id][i].MP_B1.F_X;
				V_B = MP_B[f_id][i].MP_B1.V_X;
				ST_O[f_id][i].CB_Posi_X1 = ST_I[f_id][i].CB_Posi_X1 + ST_Table_Mix(SFZ_A, SZ_A, F_A, V_A, SFZ_B, SZ_B, F_B, V_B);

				F_A = MP_B[f_id][i].MP_A1.F_Y;
				V_A = MP_B[f_id][i].MP_A1.V_Y;
				F_B = MP_B[f_id][i].MP_B1.F_Y;
				V_B = MP_B[f_id][i].MP_B1.V_Y;
				ST_O[f_id][i].CB_Posi_Y1 = ST_I[f_id][i].CB_Posi_Y1 + ST_Table_Mix(SFZ_A, SZ_A, F_A, V_A, SFZ_B, SZ_B, F_B, V_B);

				//XY2
				P_A = MP_B[f_id][i].MP_A2.P;
				P_B = MP_B[f_id][i].MP_B2.P;
				SFZ_A = Smooth_O[m_mode][P_A].F_Z;
				SZ_A  = Smooth_O[m_mode][P_A].Z;
				SFZ_B = Smooth_O[m_mode][P_B].F_Z;
				SZ_B  = Smooth_O[m_mode][P_B].Z;

				F_A = MP_B[f_id][i].MP_A2.F_X;
				V_A = MP_B[f_id][i].MP_A2.V_X;
				F_B = MP_B[f_id][i].MP_B2.F_X;
				V_B = MP_B[f_id][i].MP_B2.V_X;
				ST_O[f_id][i].CB_Posi_X2 = ST_I[f_id][i].CB_Posi_X2 + ST_Table_Mix(SFZ_A, SZ_A, F_A, V_A, SFZ_B, SZ_B, F_B, V_B);

				F_A = MP_B[f_id][i].MP_A2.F_Y;
				V_A = MP_B[f_id][i].MP_A2.V_Y;
				F_B = MP_B[f_id][i].MP_B2.F_Y;
				V_B = MP_B[f_id][i].MP_B2.V_Y;
				ST_O[f_id][i].CB_Posi_Y2 = ST_I[f_id][i].CB_Posi_Y2 + ST_Table_Mix(SFZ_A, SZ_A, F_A, V_A, SFZ_B, SZ_B, F_B, V_B);

				//XY3
				P_A = MP_B[f_id][i].MP_A3.P;
				P_B = MP_B[f_id][i].MP_B3.P;
				SFZ_A = Smooth_O[m_mode][P_A].F_Z;
				SZ_A  = Smooth_O[m_mode][P_A].Z;
				SFZ_B = Smooth_O[m_mode][P_B].F_Z;
				SZ_B  = Smooth_O[m_mode][P_B].Z;

				F_A = MP_B[f_id][i].MP_A3.F_X;
				V_A = MP_B[f_id][i].MP_A3.V_X;
				F_B = MP_B[f_id][i].MP_B3.F_X;
				V_B = MP_B[f_id][i].MP_B3.V_X;
				ST_O[f_id][i].CB_Posi_X3 = ST_I[f_id][i].CB_Posi_X3 + ST_Table_Mix(SFZ_A, SZ_A, F_A, V_A, SFZ_B, SZ_B, F_B, V_B);

				F_A = MP_B[f_id][i].MP_A3.F_Y;
				V_A = MP_B[f_id][i].MP_A3.V_Y;
				F_B = MP_B[f_id][i].MP_B3.F_Y;
				V_B = MP_B[f_id][i].MP_B3.V_Y;
				ST_O[f_id][i].CB_Posi_Y3 = ST_I[f_id][i].CB_Posi_Y3 + ST_Table_Mix(SFZ_A, SZ_A, F_A, V_A, SFZ_B, SZ_B, F_B, V_B);


				//YUV0
				P_A = MP_B[f_id][i].MP_A0.P;
				P_B = MP_B[f_id][i].MP_B0.P;
				SFY_A = Smooth_O[m_mode][P_A].F_Y;
				SY_A  = Smooth_O[m_mode][P_A].Y;
				SFU_A = Smooth_O[m_mode][P_A].F_U;
				SU_A  = Smooth_O[m_mode][P_A].U;
				SFV_A = Smooth_O[m_mode][P_A].F_V;
				SV_A  = Smooth_O[m_mode][P_A].V;
				SFY_B = Smooth_O[m_mode][P_B].F_Y;
				SY_B  = Smooth_O[m_mode][P_B].Y;
				SFU_B = Smooth_O[m_mode][P_B].F_U;
				SU_B  = Smooth_O[m_mode][P_B].U;
				SFV_B = Smooth_O[m_mode][P_B].F_V;
				SV_B  = Smooth_O[m_mode][P_B].V;

				F_A = MP_B[f_id][i].MP_A0.F_YUV;
				V_A = MP_B[f_id][i].MP_A0.V_YUV;
				F_B = MP_B[f_id][i].MP_B0.F_YUV;
				V_B = MP_B[f_id][i].MP_B0.V_YUV;
				YUV_Tmp = ST_Table_Mix(SFY_A, SY_A, F_A, V_A, SFY_B, SY_B, F_B, V_B);
				ST_O[f_id][i].CB_Adj_Y0 = 0x80 + YUV_Tmp;
				YUV_Tmp = ST_Table_Mix(SFU_A, SU_A, F_A, V_A, SFU_B, SU_B, F_B, V_B);
				ST_O[f_id][i].CB_Adj_U0 = 0x80 + YUV_Tmp;
				YUV_Tmp = ST_Table_Mix(SFV_A, SV_A, F_A, V_A, SFV_B, SV_B, F_B, V_B);
				ST_O[f_id][i].CB_Adj_V0 = 0x80 + YUV_Tmp;

				//YUV1
				P_A = MP_B[f_id][i].MP_A1.P;
				P_B = MP_B[f_id][i].MP_B1.P;
				SFY_A = Smooth_O[m_mode][P_A].F_Y;
				SY_A  = Smooth_O[m_mode][P_A].Y;
				SFU_A = Smooth_O[m_mode][P_A].F_U;
				SU_A  = Smooth_O[m_mode][P_A].U;
				SFV_A = Smooth_O[m_mode][P_A].F_V;
				SV_A  = Smooth_O[m_mode][P_A].V;
				SFY_B = Smooth_O[m_mode][P_B].F_Y;
				SY_B  = Smooth_O[m_mode][P_B].Y;
				SFU_B = Smooth_O[m_mode][P_B].F_U;
				SU_B  = Smooth_O[m_mode][P_B].U;
				SFV_B = Smooth_O[m_mode][P_B].F_V;
				SV_B  = Smooth_O[m_mode][P_B].V;

				F_A = MP_B[f_id][i].MP_A1.F_YUV;
				V_A = MP_B[f_id][i].MP_A1.V_YUV;
				F_B = MP_B[f_id][i].MP_B1.F_YUV;
				V_B = MP_B[f_id][i].MP_B1.V_YUV;
				YUV_Tmp = ST_Table_Mix(SFY_A, SY_A, F_A, V_A, SFY_B, SY_B, F_B, V_B);
				ST_O[f_id][i].CB_Adj_Y1 = 0x80 + YUV_Tmp;
				YUV_Tmp = ST_Table_Mix(SFU_A, SU_A, F_A, V_A, SFU_B, SU_B, F_B, V_B);
				ST_O[f_id][i].CB_Adj_U1 = 0x80 + YUV_Tmp;
				YUV_Tmp = ST_Table_Mix(SFV_A, SV_A, F_A, V_A, SFV_B, SV_B, F_B, V_B);
				ST_O[f_id][i].CB_Adj_V1 = 0x80 + YUV_Tmp;

				//YUV2
				P_A = MP_B[f_id][i].MP_A2.P;
				P_B = MP_B[f_id][i].MP_B2.P;
				SFY_A = Smooth_O[m_mode][P_A].F_Y;
				SY_A  = Smooth_O[m_mode][P_A].Y;
				SFU_A = Smooth_O[m_mode][P_A].F_U;
				SU_A  = Smooth_O[m_mode][P_A].U;
				SFV_A = Smooth_O[m_mode][P_A].F_V;
				SV_A  = Smooth_O[m_mode][P_A].V;
				SFY_B = Smooth_O[m_mode][P_B].F_Y;
				SY_B  = Smooth_O[m_mode][P_B].Y;
				SFU_B = Smooth_O[m_mode][P_B].F_U;
				SU_B  = Smooth_O[m_mode][P_B].U;
				SFV_B = Smooth_O[m_mode][P_B].F_V;
				SV_B  = Smooth_O[m_mode][P_B].V;

				F_A = MP_B[f_id][i].MP_A2.F_YUV;
				V_A = MP_B[f_id][i].MP_A2.V_YUV;
				F_B = MP_B[f_id][i].MP_B2.F_YUV;
				V_B = MP_B[f_id][i].MP_B2.V_YUV;
				YUV_Tmp = ST_Table_Mix(SFY_A, SY_A, F_A, V_A, SFY_B, SY_B, F_B, V_B);
				ST_O[f_id][i].CB_Adj_Y2 = 0x80 + YUV_Tmp;
				YUV_Tmp = ST_Table_Mix(SFU_A, SU_A, F_A, V_A, SFU_B, SU_B, F_B, V_B);
				ST_O[f_id][i].CB_Adj_U2 = 0x80 + YUV_Tmp;
				YUV_Tmp = ST_Table_Mix(SFV_A, SV_A, F_A, V_A, SFV_B, SV_B, F_B, V_B);
				ST_O[f_id][i].CB_Adj_V2 = 0x80 + YUV_Tmp;

				//YUV3
				P_A = MP_B[f_id][i].MP_A3.P;
				P_B = MP_B[f_id][i].MP_B3.P;
				SFY_A = Smooth_O[m_mode][P_A].F_Y;
				SY_A  = Smooth_O[m_mode][P_A].Y;
				SFU_A = Smooth_O[m_mode][P_A].F_U;
				SU_A  = Smooth_O[m_mode][P_A].U;
				SFV_A = Smooth_O[m_mode][P_A].F_V;
				SV_A  = Smooth_O[m_mode][P_A].V;
				SFY_B = Smooth_O[m_mode][P_B].F_Y;
				SY_B  = Smooth_O[m_mode][P_B].Y;
				SFU_B = Smooth_O[m_mode][P_B].F_U;
				SU_B  = Smooth_O[m_mode][P_B].U;
				SFV_B = Smooth_O[m_mode][P_B].F_V;
				SV_B  = Smooth_O[m_mode][P_B].V;

				F_A = MP_B[f_id][i].MP_A3.F_YUV;
				V_A = MP_B[f_id][i].MP_A3.V_YUV;
				F_B = MP_B[f_id][i].MP_B3.F_YUV;
				V_B = MP_B[f_id][i].MP_B3.V_YUV;
				YUV_Tmp = ST_Table_Mix(SFY_A, SY_A, F_A, V_A, SFY_B, SY_B, F_B, V_B);
				ST_O[f_id][i].CB_Adj_Y3 = 0x80 + YUV_Tmp;
				YUV_Tmp = ST_Table_Mix(SFU_A, SU_A, F_A, V_A, SFU_B, SU_B, F_B, V_B);
				ST_O[f_id][i].CB_Adj_U3 = 0x80 + YUV_Tmp;
				YUV_Tmp = ST_Table_Mix(SFV_A, SV_A, F_A, V_A, SFV_B, SV_B, F_B, V_B);
				ST_O[f_id][i].CB_Adj_V3 = 0x80 + YUV_Tmp;

			}
		}
	}*/
}

int ST_Line_Offset = 0;
void do_ST_Line_Offset(void)
{
	int i;
	int zoom;
	int S1_Y, S2_Y, S3_Y, S4_Y;
	float S1_A, S2_A, S3_A, S4_A;

	zoom = Adj_Sensor[LensCode][0].Zoom_X;

	S1_Y = Adj_Sensor[LensCode][1].S[4].Y;
	S2_Y = Adj_Sensor[LensCode][2].S[4].Y;
	S3_Y = Adj_Sensor[LensCode][3].S[4].Y;
	S4_Y = Adj_Sensor[LensCode][4].S[4].Y;

	S1_A = (float)S1_Y * 180.0 / (float)zoom *  1.0;
	S2_A = (float)S2_Y * 180.0 / (float)zoom * -1.0;
	S3_A = (float)S3_Y * 180.0 / (float)zoom *  1.0;
	S4_A = (float)S4_Y * 180.0 / (float)zoom * -1.0;
	ST_Line_Offset = (S1_A + S2_A + S3_A + S4_A) * 100 / 4;

	for(i = 0; i < 5; i++) {
		switch(i) {
		case 0: Adj_Sensor_Command[0].Rotate_R3 = Adj_Sensor[LensCode][0].Rotate_R3 - ST_Line_Offset; break;
		case 1: Adj_Sensor_Command[1].Rotate_R1 = Adj_Sensor[LensCode][1].Rotate_R1 - ST_Line_Offset; break;
		case 2: Adj_Sensor_Command[2].Rotate_R1 = Adj_Sensor[LensCode][2].Rotate_R1 - ST_Line_Offset; break;
		case 3: Adj_Sensor_Command[3].Rotate_R1 = Adj_Sensor[LensCode][3].Rotate_R1 - ST_Line_Offset; break;
		case 4: Adj_Sensor_Command[4].Rotate_R1 = Adj_Sensor[LensCode][4].Rotate_R1 - ST_Line_Offset; break;
		}
	}
}

int getSTLineOffset(void)
{
	return ST_Line_Offset;
}


/*
 * 	取得PaintData的值
 */
int PaintData[16][64];
int ShowSmoothIdx = 0, ShowSmoothMode = 0, setShowSmoothType = 0;
void setShowSmoothIdx(int idx, int value)
{
	switch(idx) {
	case 0: ShowSmoothMode    = value; break;
	case 1: ShowSmoothIdx     = value; break;
	case 2: setShowSmoothType = value; break;
	}
}

void getShowSmoothIdx(int *val)
{
	*val      = ShowSmoothMode;
	*(val+1)  = ShowSmoothIdx;
	*(val+2)  = setShowSmoothType;
}

void setPaintData(int point)
{
	int i, j;
	int mode, idx, type;
	mode = ShowSmoothMode;
	idx = ( (ShowSmoothIdx + point) & 0x1FF);
	type = setShowSmoothType;

	switch(type) {
	case 0:
		for(i = 0; i < 64; i++)
			PaintData[point][i] = Smooth_I_Buf[mode][i][idx] / 8;
		break;
	case 1:
		for(i = 0; i < 64; i++)
			PaintData[point][i] = Smooth_I_Weight[mode][i][idx] / 8;
		break;
	case 2:
		for(i = 0; i < 64; i++)
			PaintData[point][i] = Smooth_I_Data[mode][i][idx] / 8;
		break;
	case 3:
		for(i = 0; i < 64; i++) {
			if(i < 32)
				PaintData[point][i] = Smooth_Z_V_I[mode][idx][i].Weight / 8;
			else
				PaintData[point][i] = Smooth_Z_H_I[mode][idx][i-32].Weight / 8;
		}
		break;
	case 4:
		for(i = 0; i < 64; i++) {
			if(i < 32)
				PaintData[point][i] = Smooth_Z_V_I[mode][idx][i].Value0 / 8;
			else
				PaintData[point][i] = Smooth_Z_H_I[mode][idx][i-32].Value0 / 8;
		}
		break;
	case 5:
		for(i = 0; i < 64; i++) {
			if(i < 32)
				PaintData[point][i] = Smooth_Z_V_I[mode][idx][i].Count0 / 2;
			else
				PaintData[point][i] = Smooth_Z_H_I[mode][idx][i-32].Count0 / 2;
		}
		break;
	case 6:
		for(i = 0; i < 64; i++)
			PaintData[point][i] = Smooth_I_Temp[i][idx] / 8;
		break;
	}

}
int getPaintData(int *value, int point)
{
	int i;
	setPaintData(point);
	for(i = 0; i < 64; i++)
		*(value + i) = PaintData[point][i];
	return 256;		// 256*4
}
int getStitchNumber(void)
{
    int n, mode = ShowSmoothMode;
    /*switch(mode){
        default:
        case 0: n = 136*3;      break;  // 12K
        case 1: n = 136*2;      break;  // 8K
        case 2: n = (136>>2)*6; break;  // 6K
        case 3: n = 136;        break;  // 4K
        case 4: n = (136>>2)*3; break;  // 3K
        case 5: n = (136>>1);   break;  // 2K
    }*/
    n = A_L_I3_Header[mode].Sum;
    return n;
}
int getStitchLocation(int *value)
{
	struct Adjust_Line_I3_Struct *I3_p;
	struct thita_phi_Struct Temp_t_p;
	int i, y = -1;
	int n = getStitchNumber();
	for(i = 0; i < n; i++){
		I3_p = &A_L_I3[ShowSmoothMode][i];
		Temp_t_p.phi   = (I3_p->G_t_p.phi   ) * 64 / 1092;
		Temp_t_p.thita = (I3_p->G_t_p.thita ) * 64 / 1092;
		//db_debug("G_T_P: %d,%d  temp: %d,%d\n",I3_p->G_t_p.phi,I3_p->G_t_p.thita,Temp_t_p.phi,Temp_t_p.thita);
		*(value + i) = Temp_t_p.thita;
		*(value + n + i) = Temp_t_p.phi;
		if(y == Temp_t_p.phi){
			if(I3_p->Line_Mode == 0){ *(value + n + i) += 17; } //顯示不下，調整位置
			y = -1;
		}
		else
			y = Temp_t_p.phi;
	}
	return n;		// total
}

//儲存離廠後Sensor的校正值
Sensor_Calibration_Live_Struct S_Cal_Live;
int Write_S_Cal_Live_File(void)
{
    FILE *fp;
    int size;
    char path[128];

#ifdef ANDROID_CODE
    size = sizeof(Sensor_Calibration_Live_Struct);
    sprintf(path, "/mnt/sdcard/US360/CalibrationLive.bin\0");

    fp = fopen(path, "wb");
    if(fp != NULL) {
    	fwrite(&S_Cal_Live, size, 1, fp);
    	fclose(fp);
    }
    return 0;
#else
    return 0;
#endif
}

int Read_S_Cal_Live_File(void)
{
    FILE *fp;
    int size, state;
    char path[128];
    struct stat sti;
    unsigned char buf[sizeof(Sensor_Calibration_Live_Struct)];

#ifdef ANDROID_CODE
    size = sizeof(Sensor_Calibration_Live_Struct);
    sprintf(path, "/mnt/sdcard/US360/CalibrationLive.bin\0");
    state = stat("/mnt/sdcard/US360Config.bin", &sti);
    if(state != 0) {
        db_error("CalibrationLive.bin not exist!\n");
        return -1;
    }
    else if(size != sti.st_size) {
        db_error("CalibrationLive.bin size not same!\n");
        return -2;
    }

    fp = fopen(path, "rb");
    if(fp == NULL) {
        db_error("Read CalibrationLive.bin err!\n");
        return -3;
    }
    else {
    	fread(&buf[0], size, 1, fp);
    	memcpy(&S_Cal_Live, &buf[0], size);
    	fclose(fp);
    }
    return 0;
#else
    return 0;
#endif
}


// rex+ 180213
void get_Stitching_Out(int *mode, int *res)
{
    *mode = Stitching_Out.Output_Mode;
    if(*mode >= 0 && *mode < 32)
        *res = Stitching_Out.OC[*mode].Resolution_Mode;
	else {
#ifdef ANDROID_CODE
        db_error("get_Stitching_Out: mode=%d\n", *mode);
#endif
	}
}
// rex+ 180213
void get_GPadj_WidthHeight(int *width, int *height)
{
    *width = GPadj_Width;
    *height = GPadj_Height;
}

int Transparent_En = 1;		//0:off 1:on 2:auto
int Transparent_Auto_Table[6] = { 1, 1, 1, 0, 0, 0 };
void SetTransparentEn(int en)
{
	Transparent_En = en;
    set_A2K_Stitch_CMD_v(en);
}

int GetTransparentEn(int mode)
{
    int en = Transparent_En;    // 0:off, 1:on
    if(en == 2){                // 2:auto
        en = Transparent_Auto_Table[mode];
    }
    return(en&0x1);
}

int LensCode = 0;	//0:Old Lens	1:New Lens
int ReadLensCode() {
	FILE *fp;
	char path[128];

	sprintf(path, "/mnt/sdcard/US360/Lens.bin\0");
	fp = fopen(path, "rb");
	if(fp != NULL) {
		fread(&LensCode, sizeof(LensCode), 1, fp);
		fclose(fp);
	}
	else
		LensCode = 0;
db_debug("getLensCode() LensCode=%d\n", LensCode);
	set_A2K_LensCode(LensCode);
	return LensCode;
}

/*
 * Removal sizeX = 576
 *         sizeY = 432
 */
#define SSX_8  576      /*(4608 / 8)*/
#define SSY_8  432      /*(3456 / 8)*/
unsigned Image_Buf[SSY_8][SSX_8];
/*
char Image_In[4][SSY_8][SSX_8];         // 
void Set_Removal(void)
{
	int x,y;
	int i,j,k,l;

  for (i = 0; i <= 3; i++) {
	 for (y = 0; y < SSY_8; y++) {
		 for (x = 0; x < SSX_8; x++) {
			 Image_In[i][y][x] = 0;
		 }
	 }
  }
  Image_In[1][0][0] = 20;

  for (i = 0; i <= 3; i++) {
	 Image_In[i][3][3] = i * 20;
	 Image_In[i][3][4] = i * 20;
  }
  Image_In[0][3][5] = i * 20;


  Image_In[1][5][5] = 20;
  Image_In[3][5][5] = 20;
}
*/
extern int Removal_Compare;
/*
 * sensor_id = 0,1,2,3,4
 * frame0 = (short *)addr = 2byte = 1pixel
 */
void Obj_Removal(char *fr_in, int fr_ost, int ln_ost, int *tbl_out)
{
	int x,y;
	int i,j,k,l;
	int P_to_P;
	char *P1,*P2;
	int flag;
	int edge_Count[4];
	int Max_Count, Max_I, Max_Code;

  //Set_Removal();
  //for (i = 0; i <= 3; i++) {
  	 for (y = 0; y < SSY_8; y++) {
  		 for (x = 0; x < SSX_8; x++) {
  			 Image_Buf[y][x] = 0;
  		 }
  	 }
  //}
  char *yu0, *yu1, *yv0, *yv1;

  for (i = 0; i < 3; i++) {
	  for (j = i+1; j <= 3; j++) {
		 P_to_P = (1 << i * 4) | (1 << j * 4);
         
		 for (y = 0; y < SSY_8; y++) {
			 yu0 = fr_in + ((i * fr_ost) + (y * ln_ost));
             yu1 = fr_in + ((j * fr_ost) + (y * ln_ost));
             yv0 = fr_in + ((i * fr_ost) + (y * ln_ost) + 1);
             yv1 = fr_in + ((j * fr_ost) + (y * ln_ost) + 1);
             
             for (x = 0; x < SSX_8; x++) {
				 //P1 = &Image_In[i][y][x];
				 //P2 = &Image_In[j][y][x];
				 //if (abs(*P1 - *P2) < 10) {
                 if((abs(*yu0 - *yu1) < Removal_Compare) && (abs(*yv0 - *yv1) < Removal_Compare)){
				   Image_Buf[y][x] += P_to_P;
				 }
                 yu0 += 2;
                 yu1 += 2;
                 yv0 += 2;
                 yv1 += 2;
			 }
		 }
	  }
  }

  for (y = 0; y < SSY_8; y++) {
	 for (x = 0; x < SSX_8; x++) {
		 if ( Image_Buf[y][x] == 0x1111) Image_Buf[y][x] = 0;
	 }
  }

  for ( i= 0; i < 4; i++) edge_Count[i] = 0;

  for (y = 1; y < SSY_8-1; y++) {
	 for (x = 1; x < SSX_8-1; x++) {
		 if ( Image_Buf[y][x] != 0) {
			  flag = 0;
			 if ( Image_Buf[y][x-1] == 0) flag = 1;
			 if ( Image_Buf[y][x+1] == 0) flag = 1;
			 if ( Image_Buf[y-1][x] == 0) flag = 1;
			 if ( Image_Buf[y+1][x] == 0) flag = 1;

			 if (flag) {
				for (i = 0; i < 4; i++) {
				   if (((Image_Buf[y][x] >> (i *4)) & 0xf)  != 0)
				       edge_Count[i]++;
				}
			 }
		 }
	 }
  }

  Max_Count = 0; Max_I = 0;
  for ( i= 0; i < 4; i++)
	  if (edge_Count[i] >= Max_Count)
		  { Max_Count = edge_Count[i]; Max_I = i;};
  Max_Code = 5 << (Max_I * 4);

  for (y = 0; y < SSY_8; y++) {
	 for (x = 0; x < SSX_8; x++) {
		//if (Image_Buf[y][x] == 0) Image_Buf[y][x] = Max_Code;
        *tbl_out = Image_Buf[y][x];
        tbl_out++;
	 }
  }


}
