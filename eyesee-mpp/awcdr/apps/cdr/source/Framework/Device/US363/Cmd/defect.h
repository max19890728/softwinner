/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __DEFECT_H__
#define __DEFECT_H__

#ifdef __cplusplus
extern "C" {
#endif

#define DEFECT_IMG_X			4320
#define DEFECT_IMG_Y			3280
#define DEFECT_CMD_TABLE_X		( ( (DEFECT_IMG_X + 191) / 192) * 48)			/* 1152 */	// 48 = 192 / 8 * 2
#define DEFECT_CMD_TABLE_Y		(DEFECT_IMG_Y >> 4)							/* 205 */	// DEFECT_IMG_Y / 8 / 2

#define DEFECT_AVGY_X			( (DEFECT_IMG_X+7) >> 3)
#define DEFECT_AVGY_Y			( (DEFECT_IMG_Y+7) >> 3)

#define DEFECT_COPY_BUF_MAX		0x200000

typedef struct Defect_Avg_Y_Struct_h {
	unsigned Sum;
	unsigned Cnt;
	unsigned Avg;
} Defect_Avg_Y_Struct;
  
typedef struct Defect_Posi_Cmd_Struct_h {
	unsigned char X		:4;
	unsigned char Y		:4;			//高位元
} Defect_Posi_Cmd_Struct;
  
/*
 *  一個8X8最多2個點
 */
typedef struct Defect_Cmd_Struct_h {
	Defect_Posi_Cmd_Struct P0;
	Defect_Posi_Cmd_Struct P1;		//高位元
} Defect_Cmd_Struct;
  
typedef struct Defect_Posi_Struct_h {
	unsigned short x;
	unsigned short y;
	unsigned short value		:14;
	unsigned short flag			:2;
} Defect_Posi_Struct;
  
typedef struct Defect_8x8_Struct_h {
	Defect_Posi_Struct P0[DEFECT_IMG_Y >> 3][DEFECT_IMG_X >> 3];
	Defect_Posi_Struct P1[DEFECT_IMG_Y >> 3][DEFECT_IMG_X >> 3];
	Defect_Cmd_Struct  Table[DEFECT_IMG_Y >> 3][DEFECT_IMG_X >> 3];
} Defect_8x8_Struct;

void SetDefectType(int type);
int GetDefectType();
void SetDefectStep(int step);
int GetDefectStep();
void SetDefectTh(int th);
int GetDefectTh();
void SetDefectOffsetX(int offset);
int GetDefectOffsetX();
void SetDefectOffsetY(int offset);
int GetDefectOffsetY();
int DefectInit();
void SetDefectDebugEn(int en);
void SetDefectState(int state);
int GetDefectState();
int do_Defect_Func();

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__DEFECT_H__