/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/
 
#ifndef __ALETAS2_CMD_STRUCT_H__
#define __ALETAS2_CMD_STRUCT_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct  {
	unsigned Address;
	unsigned Data;
}AS2_CMD_IO_struct;

/*
 *	ISP1
 */
typedef struct{
  /* 0*/AS2_CMD_IO_struct	R_GainI;
  /* 1*/AS2_CMD_IO_struct	G_GainI;
  /* 2*/AS2_CMD_IO_struct	B_GainI;
  /* 3*/AS2_CMD_IO_struct	R_Gain0;
  /* 4*/AS2_CMD_IO_struct	G_Gain0;
  /* 5*/AS2_CMD_IO_struct	B_Gain0;
  /* 6*/AS2_CMD_IO_struct	M_00;				//190308 S2 Color Matrix 沒在用
  /* 7*/AS2_CMD_IO_struct	M_01;
  /* 8*/AS2_CMD_IO_struct	M_02;
  /* 9*/AS2_CMD_IO_struct	M_10;
  /*10*/AS2_CMD_IO_struct	M_11;
  /*11*/AS2_CMD_IO_struct	M_12;
  /*12*/AS2_CMD_IO_struct	M_20;
  /*13*/AS2_CMD_IO_struct	M_21;
  /*14*/AS2_CMD_IO_struct	M_22;
  /*15*/AS2_CMD_IO_struct	VH_Start_Mode;
  /*16*/AS2_CMD_IO_struct	V_H_Start;
  /*17*/AS2_CMD_IO_struct	S0_F_DDR_Addr;
  /*18*/AS2_CMD_IO_struct	S0_LC_DDR_Addr;
  /*19*/AS2_CMD_IO_struct	S0_TimeOut_Set;
  /*20*/AS2_CMD_IO_struct	S0_H_PIX_Size;
  /*21*/AS2_CMD_IO_struct	S1_F_DDR_Addr;
  /*22*/AS2_CMD_IO_struct	S1_LC_DDR_Addr;
  /*23*/AS2_CMD_IO_struct	S1_TimeOut_Set;
  /*24*/AS2_CMD_IO_struct	S1_H_PIX_Size;
  /*25*/AS2_CMD_IO_struct	S2_F_DDR_Addr;
  /*26*/AS2_CMD_IO_struct	S2_LC_DDR_Addr;
  /*27*/AS2_CMD_IO_struct	S2_TimeOut_Set;
  /*28*/AS2_CMD_IO_struct	S2_H_PIX_Size;   
  /*29*/AS2_CMD_IO_struct	S0_B_DDR_Addr;
  /*30*/AS2_CMD_IO_struct	S1_B_DDR_Addr;
  /*31*/AS2_CMD_IO_struct	S2_B_DDR_Addr;
  /*32*/AS2_CMD_IO_struct	B_Mode;
  /*33*/AS2_CMD_IO_struct	GAMMA_PARM0;
  /*34*/AS2_CMD_IO_struct	GAMMA_PARM1;
  /*35*/AS2_CMD_IO_struct	GAMMA_PARM2;
  /*36*/AS2_CMD_IO_struct	GAMMA_PARM3;
  /*37*/AS2_CMD_IO_struct	GAMMA_DDR_Addr;         //rex+ 180913, 0x02F8  [F0 , F1 Gamma Table位置]
  /*38*/AS2_CMD_IO_struct	RGB_offset;
  /*39*/AS2_CMD_IO_struct	BN_Offset;    			//0xCCAA0205	     小圖 : 0x18
  /*40*/AS2_CMD_IO_struct	F_Offset;               //0xCCAA0207         大圖 : 0x48
  /*41*/AS2_CMD_IO_struct	S0_FXY_Offset;          //0xCCAA02f3         0x00010001	=	0xBXBYRXRY			Sensor_Binn
  /*42*/AS2_CMD_IO_struct	S1_FXY_Offset;          //0xCCAA02f4         0x00010001	=						Sensor_Binn
  /*43*/AS2_CMD_IO_struct	S2_FXY_Offset;          //0xCCAA02f5         0x00010001	=						Sensor_Binn
  /*44*/AS2_CMD_IO_struct	S01_BXY_Offset;         //0xCCAA02f6         0x01010101	=	0xS1BYRY S0BYRY		FPGA_Binn
  /*45*/AS2_CMD_IO_struct	S2_BXY_Offset;          //0xCCAA02f7         0x01010101	=	0x		 S2BYRY		FPGA_Binn
  /*46*/AS2_CMD_IO_struct	S0_ISP1_START;
  /*47*/AS2_CMD_IO_struct	S1_ISP1_START;
  /*48*/AS2_CMD_IO_struct	S2_ISP1_START;
  /*49*/AS2_CMD_IO_struct	S0_Binn_START;
  /*50*/AS2_CMD_IO_struct	S1_Binn_START;
  /*51*/AS2_CMD_IO_struct	S2_Binn_START;
  /*52*/AS2_CMD_IO_struct	AWB_TH;					//0x2004: (Max << 8) | Min
  /*53*/AS2_CMD_IO_struct	SA_WD_DDR_Addr;			//0xCCAA0261
  /*54*/AS2_CMD_IO_struct	SB_WD_DDR_Addr;         //0xCCAA0262
  /*55*/AS2_CMD_IO_struct	SC_WD_DDR_Addr;         //0xCCAA0263
  /*56*/AS2_CMD_IO_struct	SA_WB_DDR_Addr;         //0xCCAA0264
  /*57*/AS2_CMD_IO_struct	SB_WB_DDR_Addr;         //0xCCAA0265
  /*58*/AS2_CMD_IO_struct	SC_WB_DDR_Addr;         //0xCCAA0266
  /*59*/AS2_CMD_IO_struct	WDR_Start_En;           //0xCCAA0260
  /*60*/AS2_CMD_IO_struct	Mo_Mul;           		//0xCCAA02EF    2
  /*61*/AS2_CMD_IO_struct	Mo_En;            		//0xCCAA02F9	0, 1
  /*62*/AS2_CMD_IO_struct	Noise_TH;            	//0xCCAA02FA	, max:1023, th/4096, th以下皆為th亮度, 解因Gamma倍率過高低亮度雜訊過重問題
  /*63~63*/AS2_CMD_IO_struct	REV[1];
}AS2_ISP1_CMD_struct;

/*
 *	ISP2
 */
typedef struct{
  /* 0*/AS2_CMD_IO_struct	Now_Offset;
  /* 1*/AS2_CMD_IO_struct	Pre_Offset;
  /* 2*/AS2_CMD_IO_struct	Next_Offset;
  /* 3*/AS2_CMD_IO_struct	SFull_Offset;
  /* 4*/AS2_CMD_IO_struct	SH_Mode;
  /* 5*/AS2_CMD_IO_struct	BW_EN;
  /* 6*/AS2_CMD_IO_struct	Set_Y_RG_mulV;		//0x1000 = 1倍		0xA013
  /* 7*/AS2_CMD_IO_struct	Set_Y_B_mulV;		//0x1000 = 1倍		0xA014
  /* 8*/AS2_CMD_IO_struct	Set_U_RG_mulV;		//0x1000 = 1倍		負號		負數:15bit = 1, 數值為絕對值+正負號
  /* 9*/AS2_CMD_IO_struct	Set_UV_BR_mulV;		//0x1000 = 1倍		正號
  /*10*/AS2_CMD_IO_struct	Set_V_GB_mulV;		//0x1000 = 1倍		負號
  /*11*/AS2_CMD_IO_struct	Now_Addr_0;		//不要減 1條
  /*12*/AS2_CMD_IO_struct	Pre_Addr_0;
  /*13*/AS2_CMD_IO_struct	Next_Addr_0;
  /*14*/AS2_CMD_IO_struct	Sfull_Addr_0;
  /*15*/AS2_CMD_IO_struct	Col_Size_0;
  /*16*/AS2_CMD_IO_struct	Size_Y_0;
  /*17*/AS2_CMD_IO_struct	RB_Offset_0;		//0xCCAAA047	小圖 : 0x18 , 大圖 0x48
  /*18*/AS2_CMD_IO_struct	Start_En_0;
  /*19*/AS2_CMD_IO_struct	Now_Addr_1;             //不要減 1條
  /*20*/AS2_CMD_IO_struct	Pre_Addr_1;
  /*21*/AS2_CMD_IO_struct	Next_Addr_1;
  /*22*/AS2_CMD_IO_struct	Sfull_Addr_1;
  /*23*/AS2_CMD_IO_struct	Col_Size_1;
  /*24*/AS2_CMD_IO_struct	Size_Y_1;
  /*25*/AS2_CMD_IO_struct	RB_Offset_1;            //0xCCAAA057    小圖 : 0x18 , 大圖 0x48
  /*26*/AS2_CMD_IO_struct	Start_En_1;
  /*27*/AS2_CMD_IO_struct	Now_Addr_2;             //不要減 1條
  /*28*/AS2_CMD_IO_struct	Pre_Addr_2;
  /*29*/AS2_CMD_IO_struct	Next_Addr_2;
  /*30*/AS2_CMD_IO_struct	Sfull_Addr_2;
  /*31*/AS2_CMD_IO_struct	Col_Size_2;
  /*32*/AS2_CMD_IO_struct	Size_Y_2;
  /*33*/AS2_CMD_IO_struct	RB_Offset_2;           //0xCCAAA067    小圖 : 0x18 , 大圖 0x48
  /*34*/AS2_CMD_IO_struct	Start_En_2;
  /*35*/AS2_CMD_IO_struct	NR3D_Level;
  /*36*/AS2_CMD_IO_struct	NR3D_Rate;
  /*37*/AS2_CMD_IO_struct	WDR_Addr_0;				//0xCCAAA048
  /*38*/AS2_CMD_IO_struct	WDR_Addr_1;				//0xCCAAA058
  /*39*/AS2_CMD_IO_struct	WDR_Addr_2;				//0xCCAAA068
  /*40*/AS2_CMD_IO_struct	WDR_Offset;				//0xCCAAA035	0x80=0, 0x7A=-6		1=8byte
  /*41*/AS2_CMD_IO_struct	BD_TH;
  /*42*/AS2_CMD_IO_struct	ND_Addr_0;
  /*43*/AS2_CMD_IO_struct	PD_Addr_0;
  /*44*/AS2_CMD_IO_struct	MD_Addr_0;
  /*45*/AS2_CMD_IO_struct	ND_Addr_1;
  /*46*/AS2_CMD_IO_struct	PD_Addr_1;
  /*47*/AS2_CMD_IO_struct	MD_Addr_1;
  /*48*/AS2_CMD_IO_struct	ND_Addr_2;
  /*49*/AS2_CMD_IO_struct	PD_Addr_2;
  /*50*/AS2_CMD_IO_struct	MD_Addr_2;
  /*51*/AS2_CMD_IO_struct	OV_Mul;
  /*52*/AS2_CMD_IO_struct	NR3D_Disable_0;         // 0xCCAAA04A   // 0:normal(enable), 3:disable(AEB used DDR)
  /*53*/AS2_CMD_IO_struct	NR3D_Disable_1;         // 0xCCAAA05A
  /*54*/AS2_CMD_IO_struct	NR3D_Disable_2;         // 0xCCAAA06A
  /*55*/AS2_CMD_IO_struct	Defect_Addr_0;			//壞點tbale addr: bit31=En  bit30~0=Addr
  /*56*/AS2_CMD_IO_struct	Defect_Addr_1;			//壞點tbale addr: bit31=En  bit30~0=Addr
  /*57*/AS2_CMD_IO_struct	Defect_Addr_2;			//壞點tbale addr: bit31=En  bit30~0=Addr
  /*58*/AS2_CMD_IO_struct	Scaler_Addr_0;          // 0xCCAAA04B   // Removal used. Binning output address, 0xCCAA027 = 0x40 enable
  /*59*/AS2_CMD_IO_struct	Scaler_Addr_1;          // 0xCCAAA05B
  /*60*/AS2_CMD_IO_struct	Scaler_Addr_2;          // 0xCCAAA06B
  /*61*/AS2_CMD_IO_struct	DG_Offset;
  /*62*/AS2_CMD_IO_struct	DG_TH;
  /*63~63*/AS2_CMD_IO_struct	REV[1];
}AS2_ISP2_CMD_struct;

/*
 *	Stitch
 */
typedef struct{
  /* 0*/AS2_CMD_IO_struct	S_DDR_P0;
  /* 1*/AS2_CMD_IO_struct	Block_Size0;
  /* 2*/AS2_CMD_IO_struct	Comm_P0;
  /* 3*/AS2_CMD_IO_struct	Start_En0;
  /* 4*/AS2_CMD_IO_struct	T_Offset_0;
  /* 5*/AS2_CMD_IO_struct	YUVC_P0;
  /* 6*/AS2_CMD_IO_struct	S_MASK_1_P0;
  /* 7*/AS2_CMD_IO_struct	S_MASK_2_P0;
  /* 8*/AS2_CMD_IO_struct	S_MASK_3_P0;
  /* 9*/AS2_CMD_IO_struct	CP_En0;
  /*42*/AS2_CMD_IO_struct	Alpha_DDR_P0;		//0xCCAA010B
  /*10*/AS2_CMD_IO_struct	S_DDR_P1;
  /*11*/AS2_CMD_IO_struct	Block_Size1;
  /*12*/AS2_CMD_IO_struct	Comm_P1;
  /*13*/AS2_CMD_IO_struct	Start_En1;
  /*14*/AS2_CMD_IO_struct	T_Offset_1;
  /*15*/AS2_CMD_IO_struct	YUVC_P1;
  /*16*/AS2_CMD_IO_struct	S_MASK_1_P1;
  /*17*/AS2_CMD_IO_struct	S_MASK_2_P1;
  /*18*/AS2_CMD_IO_struct	S_MASK_3_P1;
  /*19*/AS2_CMD_IO_struct	CP_En1;
  /*43*/AS2_CMD_IO_struct	Alpha_DDR_P1;
  /*20*/AS2_CMD_IO_struct	S_DDR_P2;
  /*21*/AS2_CMD_IO_struct	Block_Size2;
  /*22*/AS2_CMD_IO_struct	Comm_P2;
  /*23*/AS2_CMD_IO_struct	Start_En2;
  /*24*/AS2_CMD_IO_struct	T_Offset_2;
  /*25*/AS2_CMD_IO_struct	YUVC_P2;
  /*26*/AS2_CMD_IO_struct	S_MASK_1_P2;
  /*27*/AS2_CMD_IO_struct	S_MASK_2_P2;
  /*28*/AS2_CMD_IO_struct	S_MASK_3_P2;
  /*29*/AS2_CMD_IO_struct	CP_En2;
  /*44*/AS2_CMD_IO_struct	Alpha_DDR_P2;
  /*30*/AS2_CMD_IO_struct	S_DDR_P3;
  /*21*/AS2_CMD_IO_struct	Block_Size3;
  /*22*/AS2_CMD_IO_struct	Comm_P3;
  /*33*/AS2_CMD_IO_struct	Start_En3;
  /*34*/AS2_CMD_IO_struct	T_Offset_3;
  /*35*/AS2_CMD_IO_struct	YUVC_P3;
  /*36*/AS2_CMD_IO_struct	S_MASK_1_P3;
  /*37*/AS2_CMD_IO_struct	S_MASK_2_P3;
  /*38*/AS2_CMD_IO_struct	S_MASK_3_P3;
  /*39*/AS2_CMD_IO_struct	CP_En3;
  /*45*/AS2_CMD_IO_struct	Alpha_DDR_P3;
  /*40*/AS2_CMD_IO_struct	SML_XY_Offset1;
  /*41*/AS2_CMD_IO_struct	SML_XY_Offset2;
  /*46*/AS2_CMD_IO_struct	SML_XY_Offset3;
  /*47~63*/AS2_CMD_IO_struct	REV[17];
}AS2_Stitch_CMD_struct;
     
typedef struct{
  /* 0*/AS2_CMD_IO_struct	Start_En_0;
  /* 1*/AS2_CMD_IO_struct	CMD_DW0_0;
  /* 2*/AS2_CMD_IO_struct	SIZE_0;
  /* 3*/AS2_CMD_IO_struct	TAB_ADDR_P_0;
  /* 4*/AS2_CMD_IO_struct	S0_DDR_P_0;
  /* 5*/AS2_CMD_IO_struct	S1_DDR_P_0;
  /* 6*/AS2_CMD_IO_struct	T_DDR_P_0;
  /* 7*/AS2_CMD_IO_struct	S0_DDR_Offset_0;
  /* 8*/AS2_CMD_IO_struct	S1_DDR_Offset_0;
  /* 9*/AS2_CMD_IO_struct	T_DDR_Offset_0;
  /*10*/AS2_CMD_IO_struct	Start_En_1;
  /*11*/AS2_CMD_IO_struct	CMD_DW0_1;
  /*12*/AS2_CMD_IO_struct	SIZE_1;
  /*13*/AS2_CMD_IO_struct	TAB_ADDR_P_1;
  /*14*/AS2_CMD_IO_struct	S0_DDR_P_1;
  /*15*/AS2_CMD_IO_struct	S1_DDR_P_1;
  /*16*/AS2_CMD_IO_struct	T_DDR_P_1;
  /*17*/AS2_CMD_IO_struct	S0_DDR_Offset_1;
  /*18*/AS2_CMD_IO_struct	S1_DDR_Offset_1;
  /*19*/AS2_CMD_IO_struct	T_DDR_Offset_1;
  /*20*/AS2_CMD_IO_struct	Start_En_2;
  /*21*/AS2_CMD_IO_struct	CMD_DW0_2;
  /*22*/AS2_CMD_IO_struct	SIZE_2;
  /*23*/AS2_CMD_IO_struct	TAB_ADDR_P_2;
  /*24*/AS2_CMD_IO_struct	S0_DDR_P_2;
  /*25*/AS2_CMD_IO_struct	S1_DDR_P_2;
  /*26*/AS2_CMD_IO_struct	T_DDR_P_2;
  /*27*/AS2_CMD_IO_struct	S0_DDR_Offset_2;
  /*28*/AS2_CMD_IO_struct	S1_DDR_Offset_2;
  /*29*/AS2_CMD_IO_struct	T_DDR_Offset_2;
  /*30*/AS2_CMD_IO_struct	Start_En_3;
  /*31*/AS2_CMD_IO_struct	CMD_DW0_3;
  /*32*/AS2_CMD_IO_struct	SIZE_3;
  /*33*/AS2_CMD_IO_struct	TAB_ADDR_P_3;
  /*34*/AS2_CMD_IO_struct	S0_DDR_P_3;
  /*35*/AS2_CMD_IO_struct	S1_DDR_P_3;
  /*36*/AS2_CMD_IO_struct	T_DDR_P_3;
  /*37*/AS2_CMD_IO_struct	S0_DDR_Offset_3;
  /*38*/AS2_CMD_IO_struct	S1_DDR_Offset_3;
  /*39*/AS2_CMD_IO_struct	T_DDR_Offset_3;
  /*40*/AS2_CMD_IO_struct	Start_En_4;
  /*41*/AS2_CMD_IO_struct	CMD_DW0_4;
  /*42*/AS2_CMD_IO_struct	SIZE_4;
  /*43*/AS2_CMD_IO_struct	TAB_ADDR_P_4;
  /*44*/AS2_CMD_IO_struct	S0_DDR_P_4;
  /*45*/AS2_CMD_IO_struct	S1_DDR_P_4;
  /*46*/AS2_CMD_IO_struct	T_DDR_P_4;
  /*47*/AS2_CMD_IO_struct	S0_DDR_Offset_4;
  /*48*/AS2_CMD_IO_struct	S1_DDR_Offset_4;
  /*49*/AS2_CMD_IO_struct	T_DDR_Offset_4;
  /*50*/AS2_CMD_IO_struct	Start_En_5;
  /*51*/AS2_CMD_IO_struct	CMD_DW0_5;
  /*52*/AS2_CMD_IO_struct	SIZE_5;
  /*53*/AS2_CMD_IO_struct	TAB_ADDR_P_5;
  /*54*/AS2_CMD_IO_struct	S0_DDR_P_5;
  /*55*/AS2_CMD_IO_struct	S1_DDR_P_5;
  /*56*/AS2_CMD_IO_struct	T_DDR_P_5;
  /*57*/AS2_CMD_IO_struct	S0_DDR_Offset_5;
  /*58*/AS2_CMD_IO_struct	S1_DDR_Offset_5;
  /*59*/AS2_CMD_IO_struct	T_DDR_Offset_5;
  /*60*/AS2_CMD_IO_struct	Mo_Mul_D;
  /*61~63*/AS2_CMD_IO_struct	REV[3];
}AS2_Diffusion_CMD_struct;

typedef struct{
	unsigned SA_Start_En	:1;
	unsigned SB_Start_En	:1;
	unsigned SC_Start_En	:1;
	unsigned I_TH_En	:1;
	unsigned Divisor	:14;
	unsigned rev		:6;
	unsigned I_TH		:8;
} __attribute__((packed)) Mo_Diff_Start_struct;

typedef struct{
	unsigned Offset_Bytes	:5;
	unsigned diff_pix	:5;
	unsigned bin_64_F	:1;
	unsigned F_2nd		:1;
	unsigned Mo_D_Mode	:2;
	unsigned rev		:2;
	unsigned Mo_TH		:8;
	unsigned Mo_Mul_D	:8;
} __attribute__((packed)) Mo_Diff_DW0_struct;

typedef struct{
	unsigned Size_Y		:12;
	unsigned rev0		:4;
	unsigned Size_X		:10;
	unsigned rev1		:6;
} __attribute__((packed)) Mo_Diff_Size_struct;

typedef struct{
        Mo_Diff_Start_struct    START_EN;
        Mo_Diff_DW0_struct    	DW0;
        Mo_Diff_Size_struct    	SIZE;
	unsigned 		TAB_DDR_P;
    unsigned 		S0_DDR_P;
    unsigned 		S1_DDR_P;
    unsigned 		T_DDR_P;
    unsigned 		S0_DDR_Offset;
    unsigned 		S1_DDR_Offset;
    unsigned 		T_DDR_Offset;
} __attribute__((packed)) Diff_CMD_struct;

/*
 * Smooth
 */
typedef struct{
/* 0*/AS2_CMD_IO_struct	Size;			// 0xCCAA03C1, (Y << 16 | (X / 192)), EX: Y = 5760, X = 11520, Size = ((5760 << 16) | 60)
/* 1*/AS2_CMD_IO_struct	S_DDR_P;		// 0xCCAA03C2, burst
/* 2*/AS2_CMD_IO_struct	T_DDR_P;        // 0xCCAA03C3, burst
/* 3*/AS2_CMD_IO_struct	Start_En;       // 0xCCAA03C0, '1' = Enable, '0' = disable
/* 4 ~ 63*/AS2_CMD_IO_struct	REV[60];
}AS2_SMOOTH_CMD_struct;

/*
 *	DMA
 */
typedef struct{
  /* 0*/AS2_CMD_IO_struct   DMA_CMD_DW0_0;
  /* 1*/AS2_CMD_IO_struct   DMA_CMD_DW1_0;
  /* 2*/AS2_CMD_IO_struct   DMA_CMD_DW2_0;
  /* 3*/AS2_CMD_IO_struct   DMA_CMD_DW3_0;
  /* 4*/AS2_CMD_IO_struct   DMA_CMD_DW0_1;
  /* 5*/AS2_CMD_IO_struct   DMA_CMD_DW1_1;
  /* 6*/AS2_CMD_IO_struct   DMA_CMD_DW2_1;
  /* 7*/AS2_CMD_IO_struct   DMA_CMD_DW3_1;
  /* 8*/AS2_CMD_IO_struct   DMA_CMD_DW0_2;
  /* 9*/AS2_CMD_IO_struct   DMA_CMD_DW1_2;
  /*10*/AS2_CMD_IO_struct   DMA_CMD_DW2_2;
  /*11*/AS2_CMD_IO_struct   DMA_CMD_DW3_2;
  /*12*/AS2_CMD_IO_struct   DMA_CMD_DW0_3;
  /*13*/AS2_CMD_IO_struct   DMA_CMD_DW1_3;
  /*14*/AS2_CMD_IO_struct   DMA_CMD_DW2_3;
  /*15*/AS2_CMD_IO_struct   DMA_CMD_DW3_3;
  /*16*/AS2_CMD_IO_struct   DMA_CMD_DW0_4;
  /*17*/AS2_CMD_IO_struct   DMA_CMD_DW1_4;
  /*18*/AS2_CMD_IO_struct   DMA_CMD_DW2_4;
  /*19*/AS2_CMD_IO_struct   DMA_CMD_DW3_4;
  /*20*/AS2_CMD_IO_struct   DMA_CMD_DW0_5;
  /*21*/AS2_CMD_IO_struct   DMA_CMD_DW1_5;
  /*22*/AS2_CMD_IO_struct   DMA_CMD_DW2_5;
  /*23*/AS2_CMD_IO_struct   DMA_CMD_DW3_5;
  /*24*/AS2_CMD_IO_struct   DMA_CMD_DW0_6;
  /*25*/AS2_CMD_IO_struct   DMA_CMD_DW1_6;
  /*26*/AS2_CMD_IO_struct   DMA_CMD_DW2_6;
  /*27*/AS2_CMD_IO_struct   DMA_CMD_DW3_6;
  /*28*/AS2_CMD_IO_struct   DMA_CMD_DW0_7;
  /*29*/AS2_CMD_IO_struct   DMA_CMD_DW1_7;
  /*30*/AS2_CMD_IO_struct   DMA_CMD_DW2_7;
  /*31*/AS2_CMD_IO_struct   DMA_CMD_DW3_7;
  /*32*/AS2_CMD_IO_struct   DMA_CMD_DW0_8;
  /*33*/AS2_CMD_IO_struct   DMA_CMD_DW1_8;
  /*34*/AS2_CMD_IO_struct   DMA_CMD_DW2_8;
  /*35*/AS2_CMD_IO_struct   DMA_CMD_DW3_8;
  /*36*/AS2_CMD_IO_struct   DMA_CMD_DW0_9;
  /*37*/AS2_CMD_IO_struct   DMA_CMD_DW1_9;
  /*38*/AS2_CMD_IO_struct   DMA_CMD_DW2_9;
  /*39*/AS2_CMD_IO_struct   DMA_CMD_DW3_9;
  /*40*/AS2_CMD_IO_struct   DMA_CMD_DW0_10;
  /*41*/AS2_CMD_IO_struct   DMA_CMD_DW1_10;
  /*42*/AS2_CMD_IO_struct   DMA_CMD_DW2_10;
  /*43*/AS2_CMD_IO_struct   DMA_CMD_DW3_10;
  /*44*/AS2_CMD_IO_struct   DMA_CMD_DW0_11;
  /*45*/AS2_CMD_IO_struct   DMA_CMD_DW1_11;
  /*46*/AS2_CMD_IO_struct   DMA_CMD_DW2_11;
  /*47*/AS2_CMD_IO_struct   DMA_CMD_DW3_11;
  /*48*/AS2_CMD_IO_struct   DMA_CMD_DW0_12;
  /*49*/AS2_CMD_IO_struct   DMA_CMD_DW1_12;
  /*50*/AS2_CMD_IO_struct   DMA_CMD_DW2_12;
  /*51*/AS2_CMD_IO_struct   DMA_CMD_DW3_12;
  /*52*/AS2_CMD_IO_struct   DMA_CMD_DW0_13;
  /*53*/AS2_CMD_IO_struct   DMA_CMD_DW1_13;
  /*54*/AS2_CMD_IO_struct   DMA_CMD_DW2_13;
  /*55*/AS2_CMD_IO_struct   DMA_CMD_DW3_13;
  /*56*/AS2_CMD_IO_struct   DMA_CMD_DW0_14;
  /*57*/AS2_CMD_IO_struct   DMA_CMD_DW1_14;
  /*58*/AS2_CMD_IO_struct   DMA_CMD_DW2_14;
  /*59*/AS2_CMD_IO_struct   DMA_CMD_DW3_14;
  /*60~63*/AS2_CMD_IO_struct	REV[4];
}AS2_DMA_CMD_struct;
     
/*
 *	JPEG
 */
typedef struct{
  /* 0*/AS2_CMD_IO_struct	R_DDR_ADDR_0;
  /* 1*/AS2_CMD_IO_struct	W_DDR_ADDR_0;
  /* 2*/AS2_CMD_IO_struct	Hder_Size_0;
  /* 3*/AS2_CMD_IO_struct	Y_Size_0;
  /* 4*/AS2_CMD_IO_struct	X_Size_0;
  /* 5*/AS2_CMD_IO_struct	Page_sel_0;
  /* 6*/AS2_CMD_IO_struct	Start_En_0;
  /* 7*/AS2_CMD_IO_struct	H_B_Addr_0;
  /* 8*/AS2_CMD_IO_struct	H_B_Size_0;
  /* 9*/AS2_CMD_IO_struct	NoPic_En_0;
  /*10*/AS2_CMD_IO_struct	S_B_Addr_0;
  /*11*/AS2_CMD_IO_struct	S_B_Size_0;
  /*12*/AS2_CMD_IO_struct	S_B_En_0;
  /*13*/AS2_CMD_IO_struct	E_B_Addr_0;
  /*14*/AS2_CMD_IO_struct	E_B_Size_0;
  /*15*/AS2_CMD_IO_struct   Q_table_sel_0;
  /*16*/AS2_CMD_IO_struct	E_B_En_0;
  /*17*/AS2_CMD_IO_struct	R_DDR_ADDR_1;
  /*18*/AS2_CMD_IO_struct	W_DDR_ADDR_1;
  /*19*/AS2_CMD_IO_struct	Hder_Size_1;
  /*20*/AS2_CMD_IO_struct	Y_Size_1;
  /*21*/AS2_CMD_IO_struct	X_Size_1;
  /*22*/AS2_CMD_IO_struct	Page_sel_1;
  /*23*/AS2_CMD_IO_struct	Start_En_1;
  /*24*/AS2_CMD_IO_struct	H_B_Addr_1;
  /*25*/AS2_CMD_IO_struct	H_B_Size_1;
  /*26*/AS2_CMD_IO_struct	NoPic_En_1;
  /*27*/AS2_CMD_IO_struct	S_B_Addr_1;
  /*28*/AS2_CMD_IO_struct	S_B_Size_1;
  /*29*/AS2_CMD_IO_struct	S_B_En_1;
  /*30*/AS2_CMD_IO_struct	E_B_Addr_1;
  /*31*/AS2_CMD_IO_struct	E_B_Size_1;
  /*32*/AS2_CMD_IO_struct   Q_table_sel_1;
  /*33*/AS2_CMD_IO_struct	E_B_En_1;
//  /*30*/AS2_CMD_IO_struct	R_DDR_ADDR_2;
//  /*31*/AS2_CMD_IO_struct	W_DDR_ADDR_2;
//  /*32*/AS2_CMD_IO_struct	Hder_Size_2;
//  /*33*/AS2_CMD_IO_struct	Y_Size_2;
//  /*34*/AS2_CMD_IO_struct	X_Size_2;
//  /*62*/AS2_CMD_IO_struct	Page_sel_2;
//  /*35*/AS2_CMD_IO_struct	Start_En_2;
//  /*36*/AS2_CMD_IO_struct	H_B_Addr_2;
//  /*37*/AS2_CMD_IO_struct	H_B_Size_2;
//  /*38*/AS2_CMD_IO_struct	NoPic_En_2;
//  /*39*/AS2_CMD_IO_struct	S_B_Addr_2;
//  /*40*/AS2_CMD_IO_struct	S_B_Size_2;
//  /*41*/AS2_CMD_IO_struct	S_B_En_2;
//  /*42*/AS2_CMD_IO_struct	E_B_Addr_2;
//  /*43*/AS2_CMD_IO_struct	E_B_Size_2;
//  /*44*/AS2_CMD_IO_struct	E_B_En_2;
//  /*45*/AS2_CMD_IO_struct	R_DDR_ADDR_3;
//  /*46*/AS2_CMD_IO_struct	W_DDR_ADDR_3;
//  /*47*/AS2_CMD_IO_struct	Hder_Size_3;
//  /*48*/AS2_CMD_IO_struct	Y_Size_3;
//  /*49*/AS2_CMD_IO_struct	X_Size_3;
//  /*63*/AS2_CMD_IO_struct	Page_sel_3;
//  /*50*/AS2_CMD_IO_struct	Start_En_3;
//  /*51*/AS2_CMD_IO_struct	H_B_Addr_3;
//  /*52*/AS2_CMD_IO_struct	H_B_Size_3;
//  /*53*/AS2_CMD_IO_struct	NoPic_En_3;
//  /*54*/AS2_CMD_IO_struct	S_B_Addr_3;
//  /*55*/AS2_CMD_IO_struct	S_B_Size_3;
//  /*56*/AS2_CMD_IO_struct	S_B_En_3;
//  /*57*/AS2_CMD_IO_struct	E_B_Addr_3;
//  /*58*/AS2_CMD_IO_struct	E_B_Size_3;
//  /*59*/AS2_CMD_IO_struct	E_B_En_3;
    /*34 ~ 63*/AS2_CMD_IO_struct	REV[30];
}AS2_JPEG_CMD_struct;

typedef struct{
  /*17*/AS2_CMD_IO_struct		Reset_Record;
  /* 0*/AS2_CMD_IO_struct		S_Addr;
  /* 1*/AS2_CMD_IO_struct		SM_Addr;
  /* 2*/AS2_CMD_IO_struct		T_Addr;
  /* 3*/AS2_CMD_IO_struct		TM_Addr;                       	//0-23 = SM_Addr
  /* 4*/AS2_CMD_IO_struct		Motion_Addr;                   	//0-23
  /* 5*/AS2_CMD_IO_struct		size_v;                        	//0-6 burst
  /* 6*/AS2_CMD_IO_struct		size_h;                        	//0-6
  /* 7*/AS2_CMD_IO_struct		XY_Rate;    					//0 Div_Mode 1-2 Div_field 3
  /* 8*/AS2_CMD_IO_struct		NewTempCheck;      				//0-15       NewTempLine 16 - 31
  /* 9*/AS2_CMD_IO_struct		B_Mask_Addr;                   	//0-23    B_Mask_Addr_Size 28-30 B_Mask_Addr_En 31
  /*10*/AS2_CMD_IO_struct		QML0;                    		//     13107
  /*11*/AS2_CMD_IO_struct		QSL;                     		//     4
  /*12*/AS2_CMD_IO_struct		DeQML0;                  		//     10
  /*13*/AS2_CMD_IO_struct		QML1;                    		//     8066
  /*14*/AS2_CMD_IO_struct		DeQML1;                  		//     13
  /*15*/AS2_CMD_IO_struct		QML2;                    		//     5423
  /*16*/AS2_CMD_IO_struct		DeQML2;                  		//     16

  /*18*/AS2_CMD_IO_struct		Size_Y;
  /*19*/AS2_CMD_IO_struct		Size_X;                     	//0-6 Bytes
  /*20*/AS2_CMD_IO_struct		I_P_Mode;                   	// I:0   P:1
  /*21~59*/AS2_CMD_IO_struct	Add_Byte[39];        			// Header
  /*60*/AS2_CMD_IO_struct		Add_Last_Byte;       			// 0-7 data 8-11 bit  0011 00000111
  /*61*/AS2_CMD_IO_struct		H264_en;             			//
  /*62*/AS2_CMD_IO_struct		USB_Buffer_Sel;
  /*63*/AS2_CMD_IO_struct  		rev;
}AS2_H264_CMD_struct;

typedef struct  {
  /* 0*/AS2_CMD_IO_struct	S_DDR_ADDR_0;
  /* 1*/AS2_CMD_IO_struct	JPEG_SEL_0;
  /* 2*/AS2_CMD_IO_struct	Start_En_0;
  /* 3*/AS2_CMD_IO_struct	S_DDR_ADDR_1;
  /* 4*/AS2_CMD_IO_struct	JPEG_SEL_1;
  /* 5*/AS2_CMD_IO_struct	Start_En_1;
  /* 6*/AS2_CMD_IO_struct	S_DDR_ADDR_2;
  /* 7*/AS2_CMD_IO_struct	JPEG_SEL_2;
  /* 8*/AS2_CMD_IO_struct	Start_En_2;
  /* 9*/AS2_CMD_IO_struct	S_DDR_ADDR_3;
  /*10*/AS2_CMD_IO_struct	JPEG_SEL_3;
  /*11*/AS2_CMD_IO_struct	Start_En_3;
  /*12~63*/AS2_CMD_IO_struct	REV[52];
}AS2_USB_CMD_struct;
 
/*
 *     State Table
 */
typedef struct {
	unsigned  Test_Cnt0;	      //test  0x60000
	unsigned  F0_S0_R_Cnt;        //test  0x60004
	unsigned  F0_S0_G_Cnt;        //test  0x60008
	unsigned  F0_S0_B_Cnt;        //test  0x6000C
	unsigned  F0_S1_R_Cnt;        //test  0x60010
	unsigned  F0_S1_G_Cnt;        //test  0x60014
	unsigned  F0_S1_B_Cnt;        //test  0x60018
	unsigned  F1_S0_R_Cnt;        //test  0x6001C
	unsigned  F1_S0_G_Cnt;        //test  0x60020
	unsigned  F1_S0_B_Cnt;        //test  0x60024
	unsigned  F1_S1_R_Cnt;        //test  0x60028
	unsigned  F1_S1_G_Cnt;        //test  0x6002C
	unsigned  F1_S1_B_Cnt;        //test  0x60030
	unsigned  F1_S2_R_Cnt;        //test  0x60034
	unsigned  F1_S2_G_Cnt;        //test  0x60038
	unsigned  F1_S2_B_Cnt;        //test  0x6003C
	unsigned  AS2_Sync_Idx;	     //Command Sync Index	//6 bits
	unsigned  IMCP_Idx;          // Image Compare Index     //2 bits
	unsigned  Vsync_Cnt;         // V Sync Counter  --   100MHz/256   =  2.56us
	unsigned  Free_Run_Cnt;      // Free Run Counter  --   100MHz/256    =  2.56us
	unsigned  Vsync_Cnt_MAX;     //
	unsigned  F0_S0_offset;      // offset cnt from F0_S0_Sync correct lock time
	unsigned  F0_S1_offset;      //
	unsigned  F1_S0_offset;      //
	unsigned  F1_S1_offset;      //
	unsigned  F1_S2_offset;      //
	unsigned  REV[5];
	unsigned  Test_Cnt1;	      //test
}AS2_State_Table_struct;

typedef struct{
	unsigned size		:7;  //0~126
	unsigned enable		:1;
	unsigned reset		:1;
	unsigned rev		:7;
	unsigned checksum	:16; //0xD55C
}S_HEADER_struct;

typedef struct{
   unsigned	Addr	:12;         //
   unsigned S_ID    :3;          // 0->F1_S0 1->F1_S1 2->F0_S0 3->F1_S2  4->F0_S1  5->all
   unsigned	RW	    :1;          // 0 read   1 write
   unsigned	Data	:16;         //
}S_CMD_struct;

typedef struct{
   S_HEADER_struct 	S_HEADER;
   unsigned		    AS2_Sync_MAX;
   S_CMD_struct		S_Data[126];
}S_M_CMD_struct;

typedef struct{
   unsigned	Data	:20;
   unsigned	Rev	    :8;
   unsigned ID      :4;      	//0~14
}WAVE_CMD_struct;


/*
 *     F0 Main Command
 */
typedef struct {
	AS2_ISP1_CMD_struct         F0_ISP1_CMD;        //0, 64*8=512
	AS2_ISP2_CMD_struct         F0_ISP2_CMD;        //1, 64*8=512
	AS2_Stitch_CMD_struct       F0_Stitch_CMD;      //2, 64*8=512
	AS2_ISP1_CMD_struct         F1_ISP1_CMD;        //3, 64*8=512
	AS2_ISP2_CMD_struct         F1_ISP2_CMD;        //4, 64*8=512
	AS2_Stitch_CMD_struct       F1_Stitch_CMD;      //5, 64*8=512
	AS2_Diffusion_CMD_struct    F0_Diffusion_CMD;   //6
	AS2_Diffusion_CMD_struct    F1_Diffusion_CMD;   //7
}AS2_F0_MAIN_CMD_struct;                            // 4KB

/*
 *     F2 Main Command
 */
typedef struct {
	S_M_CMD_struct        SENSOR_CMD;     	//0, 512
	AS2_JPEG_CMD_struct   JPEG0_CMD;        //1, 512
	AS2_JPEG_CMD_struct   JPEG1_CMD;        //2, 512
	AS2_DMA_CMD_struct    DMA_CMD;          //3, 512
	AS2_USB_CMD_struct    USB_CMD;          //4, 512
	AS2_H264_CMD_struct   H264_CMD;         //5
	AS2_SMOOTH_CMD_struct SMOOTH_CMD;
	unsigned char         rev[512];
}AS2_F2_MAIN_CMD_struct;                    // 4KB


typedef struct{
	unsigned        S_DDR_P;
	unsigned        T_DDR_P;
	unsigned        Size    :16;
	unsigned        Mode    :1;
	unsigned        F_ID    :3;
	unsigned        Rev     :4;
	unsigned        Check_ID:8;
}AS2_SPI_Cmd_struct;

#ifdef __cplusplus
}   // extern "C"
#endif

#endif    // __ALETAS2_CMD_STRUCT_H__
