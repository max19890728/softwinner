/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/
 
#ifndef __ALETAS3_CMD_STRUCT_H__
#define __ALETAS3_CMD_STRUCT_H__

#ifdef __cplusplus
extern "C" {
#endif

int AS3_iQY[64] = {
	16,	12,	14,	14,	18,	24,	49,	72,
	11,	12,	13,	17,	22,	35,	64,	92,
	10,	14,	16,	22,	37,	55,	78,	95,
	16,	19,	24,	29,	56,	64,	87,	98,
	24,	26,	40,	51,	68,	81,	103,	112,
	40,	58,	57,	87,	109,	104,	121,	100,
	51,	60,	69,	80,	103,	113,	120,	103,
	61,	55,	56,	62,	77,	92,	101,	99
};
int AS3_iQC[64] = {
	17,	18,	24,	47,	99,	99,	99,	99,
	18,	21,	26,	66,	99,	99,	99,	99,
	24,	26,	56,	99,	99,	99,	99,	99,
	47,	66,	99,	99,	99,	99,	99,	99,
	99,	99,	99,	99,	99,	99,	99,	99,
	99,	99,	99,	99,	99,	99,	99,	99,
	99,	99,	99,	99,	99,	99,	99,	99,
	99,	99,	99,	99,	99,	99,	99,	99
};
int AS3_Zigzag[64]={
	0,	1,	5,	6,	14,	15,	27,	28,
	2,	4,	7,	13,	16,	26,	29,	42,
	3,	8,	12,	17,	25,	30,	41,	43,
	9,	11,	18,	24,	31,	40,	44,	53,
	10,	19,	23,	32,	39,	45,	52,	54,
	20,	22,	33,	38,	46,	51,	55,	60,
	21,	34,	37,	47,	50,	56,	59,	61,
	35,	36,	48,	49,	57,	58,	62,	63
};
char AS3_HT_Y_DC[28] = {
/*  8*/	0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01,
/*  8*/	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*  8*/	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
/*  8*/	0x08, 0x09, 0x0a, 0x0b
};
char AS3_HT_Y_AC[178] = {
/*  0*/	0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03,
/*  8*/	0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7d,
/* 16*/	0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
/* 24*/	0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
/* 32*/	0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
/* 40*/	0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
/* 48*/	0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
/* 56*/	0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
/* 64*/	0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
/* 72*/	0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
/* 80*/	0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
/* 88*/	0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
/* 96*/	0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
/*104*/	0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
/*112*/	0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
/*120*/	0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
/*128*/	0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
/*136*/	0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
/*144*/	0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
/*152*/	0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
/*160*/	0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
/*168*/	0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
/*176*/	0xf9, 0xfa
};
char AS3_HT_C_DC[28] = {
/*  0*/	0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
/*  8*/	0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 16*/	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
/* 24*/	0x08, 0x09, 0x0a, 0x0b
};
char AS3_HT_C_AC[178] = {
/*  0*/	0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04,
/*  8*/	0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77,
/* 16*/	0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
/* 24*/	0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
/* 32*/	0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
/* 40*/	0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
/* 48*/	0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
/* 56*/	0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
/* 64*/	0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
/* 72*/	0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
/* 80*/	0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
/* 88*/	0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
/* 96*/	0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
/*104*/	0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
/*112*/	0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
/*120*/	0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
/*128*/	0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
/*136*/	0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
/*144*/	0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
/*152*/	0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
/*160*/	0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
/*168*/	0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
/*176*/	0xf9, 0xfa
};
              

typedef struct{
	char Label_H;
	char Label_L;
}AS3_J_SOI_Struct;
typedef struct{
	char Label_H;
	char Label_L;
	char Length_H;
	char Length_L;
	char  Data[14];
}AS3_J_APP0_Struct;
typedef struct{
	char Label_H;
	char Label_L;
	char Length_H;
	char Length_L;
	char Data[128];
}AS3_J_APP1_Struct;
typedef struct{
	char Label_H;
	char Label_L;
	char Length_H;
	char Length_L;
	char  QT_ID;
	char  Data[64];
}AS3_J_DQT_Struct;
typedef struct{
	char Label_H;
	char Label_L;
	char Length_H;
	char Length_L;
	char  Precision;
	char Image_H_H;
	char Image_H_L;
	char Image_W_H;
	char Image_W_L;
	char  N_Components;
	char  Component_ID0;
	char  VH_Factor0;
	char  QT_ID0;
	char  Component_ID1;
	char  VH_Factor1;
	char  QT_ID1;
	char  Component_ID2;
	char  VH_Factor2;
	char  QT_ID2;
}AS3_J_SOF0_Struct;
typedef struct{
	char Label_H;
	char Label_L;
	char Length_H;
	char Length_L;
	char  HT_ID;
	char  Data[28];
}AS3_J_DHT_DC_Struct;
typedef struct{
	char Label_H;
	char Label_L;
	char Length_H;
	char Length_L;
	char  HT_ID;
	char  Data[178];
}AS3_J_DHT_AC_Struct;
typedef struct{
	char Label_H;
	char Label_L;
	char Length_H;
	char Length_L;
	char  N_Components;
	char  Component_ID0;
	char  HT_ID0;
	char  Component_ID1;
	char  HT_ID1;
	char  Component_ID2;
	char  HT_ID2;
	char  Data[3];
}AS3_J_SOS_Struct;

typedef struct{
	AS3_J_SOI_Struct SOI;
	AS3_J_APP0_Struct APP0;
	AS3_J_APP1_Struct APP1;
	AS3_J_DQT_Struct DQT_Y;
	AS3_J_DQT_Struct DQT_C;
	AS3_J_SOF0_Struct SOF0;
	AS3_J_DHT_DC_Struct DHT_Y_DC;
	AS3_J_DHT_AC_Struct DHT_Y_AC;
	AS3_J_DHT_DC_Struct DHT_C_DC;
	AS3_J_DHT_AC_Struct DHT_C_AC;
	AS3_J_SOS_Struct SOS;
}AS3_J_Hder_Struct;
typedef struct  {
	unsigned Address;
	unsigned Data;
}AS3_CMD_IO_struct;

typedef struct{
  /* 0*/AS3_CMD_IO_struct  R_GAIN_I;
  /* 1*/AS3_CMD_IO_struct  G_GAIN_I;	
  /* 2*/AS3_CMD_IO_struct  B_GAIN_I;	
  /* 3*/AS3_CMD_IO_struct  R_GAIN_0;	
  /* 4*/AS3_CMD_IO_struct  G_GAIN_0;	
  /* 5*/AS3_CMD_IO_struct  B_GAIN_0;	
  /* 6*/AS3_CMD_IO_struct  VH_Start_Mode;	
  /* 7*/AS3_CMD_IO_struct  V_H_Start;	
  /* 8*/AS3_CMD_IO_struct  H_PIX_Size;	
  /* 9*/AS3_CMD_IO_struct  B_Mode;	
  /*10*/AS3_CMD_IO_struct  Gm_R_DDR_Addr_R3;	
  /*11*/AS3_CMD_IO_struct  RGB_Offset;	
  /*12*/AS3_CMD_IO_struct  AWB_TH;	
  /*13*/AS3_CMD_IO_struct  WDR_Start;	
  /*14*/AS3_CMD_IO_struct  Mo_Par;	
  /*15*/AS3_CMD_IO_struct  Mo_En;	
  /*16*/AS3_CMD_IO_struct  Noise_TH;	
  /*17*/AS3_CMD_IO_struct  SA_F_DDR_Addr_R3;	
  /*18*/AS3_CMD_IO_struct  SB_F_DDR_Addr_R3;	
  /*19*/AS3_CMD_IO_struct  SC_F_DDR_Addr_R3;	
  /*20*/AS3_CMD_IO_struct  SD_F_DDR_Addr_R3;	
  /*21*/AS3_CMD_IO_struct  SE_F_DDR_Addr_R3;	
  /*22*/AS3_CMD_IO_struct  SF_F_DDR_Addr_R3;	
  /*23*/AS3_CMD_IO_struct  SA_LC_DDR_Addr;	
  /*24*/AS3_CMD_IO_struct  SB_LC_DDR_Addr;	
  /*25*/AS3_CMD_IO_struct  SC_LC_DDR_Addr;	
  /*26*/AS3_CMD_IO_struct  SD_LC_DDR_Addr;	
  /*27*/AS3_CMD_IO_struct  SE_LC_DDR_Addr;	
  /*28*/AS3_CMD_IO_struct  SF_LC_DDR_Addr;	
  /*29*/AS3_CMD_IO_struct  SA_B_DDR_Addr_R3;	
  /*30*/AS3_CMD_IO_struct  SB_B_DDR_Addr_R3;	
  /*31*/AS3_CMD_IO_struct  SC_B_DDR_Addr_R3;	
  /*32*/AS3_CMD_IO_struct  SD_B_DDR_Addr_R3;	
  /*33*/AS3_CMD_IO_struct  SE_B_DDR_Addr_R3;	
  /*34*/AS3_CMD_IO_struct  SF_B_DDR_Addr_R3;	
  /*35*/AS3_CMD_IO_struct  SA_WD_DDR_Addr;	
  /*36*/AS3_CMD_IO_struct  SB_WD_DDR_Addr;	
  /*37*/AS3_CMD_IO_struct  SC_WD_DDR_Addr;	
  /*38*/AS3_CMD_IO_struct  SD_WD_DDR_Addr;	
  /*39*/AS3_CMD_IO_struct  SE_WD_DDR_Addr;	
  /*40*/AS3_CMD_IO_struct  SF_WD_DDR_Addr;	
  /*41*/AS3_CMD_IO_struct  SA_WB_DDR_Addr;	
  /*42*/AS3_CMD_IO_struct  SB_WB_DDR_Addr;	
  /*43*/AS3_CMD_IO_struct  SC_WB_DDR_Addr;	
  /*44*/AS3_CMD_IO_struct  SD_WB_DDR_Addr;	
  /*45*/AS3_CMD_IO_struct  SE_WB_DDR_Addr;	
  /*46*/AS3_CMD_IO_struct  SF_WB_DDR_Addr;	
  /*47*/AS3_CMD_IO_struct  SA_ISP1_En;	
  /*48*/AS3_CMD_IO_struct  SB_ISP1_En;	
  /*49*/AS3_CMD_IO_struct  SC_ISP1_En;	
  /*50*/AS3_CMD_IO_struct  SD_ISP1_En;	
  /*51*/AS3_CMD_IO_struct  SE_ISP1_En;	
  /*52*/AS3_CMD_IO_struct  SF_ISP1_En;	
  /*53*/AS3_CMD_IO_struct  SA_Binn_Cnt;	
  /*54*/AS3_CMD_IO_struct  SB_Binn_Cnt;	
  /*55*/AS3_CMD_IO_struct  SC_Binn_Cnt;	
  /*56*/AS3_CMD_IO_struct  SD_Binn_Cnt;	
  /*57*/AS3_CMD_IO_struct  SE_Binn_Cnt;	
  /*58*/AS3_CMD_IO_struct  SF_Binn_Cnt;	
  /*59*/AS3_CMD_IO_struct  ISP1_LH;
  /*60~63*/AS3_CMD_IO_struct	REV[4];
}AS3_ISP1_CMD_struct;


typedef struct{
  /* 0*/AS3_CMD_IO_struct  Now_Offset;	          
  /* 1*/AS3_CMD_IO_struct  Pre_Offset;	          
  /* 2*/AS3_CMD_IO_struct  Next_Offset;	          
  /* 3*/AS3_CMD_IO_struct  SFull_Offset;	  
  /* 4*/AS3_CMD_IO_struct  SH_Mode;	          
  /* 5*/AS3_CMD_IO_struct  BW_EN;	          
  /* 6*/AS3_CMD_IO_struct  Set_Y_RG_mulV;	  
  /* 7*/AS3_CMD_IO_struct  Set_Y_B_mulV;	  
  /* 8*/AS3_CMD_IO_struct  Set_U_RG_mulV;	  
  /* 9*/AS3_CMD_IO_struct  Set_UV_BR_mulV;        
  /*10*/AS3_CMD_IO_struct  Set_V_GB_mulV;         
  /*11*/AS3_CMD_IO_struct  Now_Addr_0;            
  /*12*/AS3_CMD_IO_struct  Pre_Addr_0;            
  /*13*/AS3_CMD_IO_struct  Next_Addr_0;           
  /*14*/AS3_CMD_IO_struct  Sfull_Addr_0;          
  /*15*/AS3_CMD_IO_struct  Col_Size_0;            
  /*16*/AS3_CMD_IO_struct  Size_Y_0;              
  /*17*/AS3_CMD_IO_struct  RB_Offset_0;           
  /*18*/AS3_CMD_IO_struct  Start_En_0;            
  /*19*/AS3_CMD_IO_struct  Now_Addr_1;            
  /*20*/AS3_CMD_IO_struct  Pre_Addr_1;            
  /*21*/AS3_CMD_IO_struct  Next_Addr_1;           
  /*22*/AS3_CMD_IO_struct  Sfull_Addr_1;          
  /*23*/AS3_CMD_IO_struct  Col_Size_1;            
  /*24*/AS3_CMD_IO_struct  Size_Y_1;              
  /*25*/AS3_CMD_IO_struct  RB_Offset_1;           
  /*26*/AS3_CMD_IO_struct  Start_En_1;            
  /*27*/AS3_CMD_IO_struct  Now_Addr_2;            
  /*28*/AS3_CMD_IO_struct  Pre_Addr_2;            
  /*29*/AS3_CMD_IO_struct  Next_Addr_2;           
  /*30*/AS3_CMD_IO_struct  Sfull_Addr_2;          
  /*31*/AS3_CMD_IO_struct  Col_Size_2;            
  /*32*/AS3_CMD_IO_struct  Size_Y_2;              
  /*33*/AS3_CMD_IO_struct  RB_Offset_2;           
  /*34*/AS3_CMD_IO_struct  Start_En_2;            
  /*35*/AS3_CMD_IO_struct  NR3D_Level;            
  /*36*/AS3_CMD_IO_struct  NR3D_Rate;             
  /*37*/AS3_CMD_IO_struct  WDR_Addr_0;            
  /*38*/AS3_CMD_IO_struct  WDR_Addr_1;            
  /*39*/AS3_CMD_IO_struct  WDR_Addr_2;            
  /*40*/AS3_CMD_IO_struct  WDR_Offset;            
  /*41*/AS3_CMD_IO_struct  BD_TH;                 
  /*42*/AS3_CMD_IO_struct  ND_Addr_0;             
  /*43*/AS3_CMD_IO_struct  PD_Addr_0;             
  /*44*/AS3_CMD_IO_struct  MD_Addr_0;             
  /*45*/AS3_CMD_IO_struct  ND_Addr_1;             
  /*46*/AS3_CMD_IO_struct  PD_Addr_1;             
  /*47*/AS3_CMD_IO_struct  MD_Addr_1;             
  /*48*/AS3_CMD_IO_struct  ND_Addr_2;             
  /*49*/AS3_CMD_IO_struct  PD_Addr_2;             
  /*50*/AS3_CMD_IO_struct  MD_Addr_2;             
  /*51*/AS3_CMD_IO_struct  OV_Mul;                
  /*52*/AS3_CMD_IO_struct  NR3D_Disable_0;        
  /*53*/AS3_CMD_IO_struct  NR3D_Disable_1;        
  /*54*/AS3_CMD_IO_struct  NR3D_Disable_2;        
  /*55*/AS3_CMD_IO_struct  Defect_Addr_0;         
  /*56*/AS3_CMD_IO_struct  Defect_Addr_1;         
  /*57*/AS3_CMD_IO_struct  Defect_Addr_2;         
  /*58*/AS3_CMD_IO_struct  Scaler_Addr_0;         
  /*59*/AS3_CMD_IO_struct  Scaler_Addr_1;         
  /*60*/AS3_CMD_IO_struct  Scaler_Addr_2;         
  /*61*/AS3_CMD_IO_struct  DG_Offset;             
  /*62*/AS3_CMD_IO_struct  DG_TH;                 
  /*63~63*/AS3_CMD_IO_struct	REV[1];
}AS3_ISP2_CMD_struct;

typedef struct{
  /* 0*/AS3_CMD_IO_struct  S_DDR_P0;
  /* 1*/AS3_CMD_IO_struct  Block_Size0;	
  /* 2*/AS3_CMD_IO_struct  Comm_P0;	
  /* 3*/AS3_CMD_IO_struct  Start_En0;	
  /* 4*/AS3_CMD_IO_struct  T_Offset_0;	
  /* 5*/AS3_CMD_IO_struct  YUVC_P0;	
  /* 6*/AS3_CMD_IO_struct  S_MASK_1_P0;	
  /* 7*/AS3_CMD_IO_struct  S_MASK_2_P0;	
  /* 8*/AS3_CMD_IO_struct  S_MASK_3_P0;	
  /* 9*/AS3_CMD_IO_struct  CP_En0;	
  /*10*/AS3_CMD_IO_struct  Alpha_DDR_P0;	
  /*11*/AS3_CMD_IO_struct  S_DDR_P1;	
  /*12*/AS3_CMD_IO_struct  Block_Size1;	
  /*13*/AS3_CMD_IO_struct  Comm_P1;	
  /*14*/AS3_CMD_IO_struct  Start_En1;	
  /*15*/AS3_CMD_IO_struct  T_Offset_1;	
  /*16*/AS3_CMD_IO_struct  YUVC_P1;	
  /*17*/AS3_CMD_IO_struct  S_MASK_1_P1;	
  /*18*/AS3_CMD_IO_struct  S_MASK_2_P1;	
  /*19*/AS3_CMD_IO_struct  S_MASK_3_P1;	
  /*20*/AS3_CMD_IO_struct  CP_En1;	
  /*21*/AS3_CMD_IO_struct  Alpha_DDR_P1;	
  /*22*/AS3_CMD_IO_struct  S_DDR_P2;	
  /*23*/AS3_CMD_IO_struct  Block_Size2;	
  /*24*/AS3_CMD_IO_struct  Comm_P2;	
  /*25*/AS3_CMD_IO_struct  Start_En2;	
  /*26*/AS3_CMD_IO_struct  T_Offset_2;	
  /*27*/AS3_CMD_IO_struct  YUVC_P2;	
  /*28*/AS3_CMD_IO_struct  S_MASK_1_P2;	
  /*29*/AS3_CMD_IO_struct  S_MASK_2_P2;	
  /*30*/AS3_CMD_IO_struct  S_MASK_3_P2;	
  /*21*/AS3_CMD_IO_struct  CP_En2;	
  /*22*/AS3_CMD_IO_struct  Alpha_DDR_P2;	
  /*33*/AS3_CMD_IO_struct  S_DDR_P3;	
  /*34*/AS3_CMD_IO_struct  Block_Size3;	
  /*35*/AS3_CMD_IO_struct  Comm_P3;	
  /*36*/AS3_CMD_IO_struct  Start_En3;	
  /*37*/AS3_CMD_IO_struct  T_Offset_3;	
  /*38*/AS3_CMD_IO_struct  YUVC_P3;	
  /*39*/AS3_CMD_IO_struct  S_MASK_1_P3;	
  /*40*/AS3_CMD_IO_struct  S_MASK_2_P3;	
  /*41*/AS3_CMD_IO_struct  S_MASK_3_P3;
  /*42*/AS3_CMD_IO_struct  CP_En3;
  /*43*/AS3_CMD_IO_struct  Alpha_DDR_P3;
  /*44*/AS3_CMD_IO_struct  SML_XY_Offset1;
  /*45*/AS3_CMD_IO_struct  SML_XY_Offset2;
  /*46*/AS3_CMD_IO_struct  SML_XY_Offset3;
  /*47~63*/AS3_CMD_IO_struct	REV[17];
}AS3_Stitch_CMD_struct;


typedef struct{
  /* 0*/AS3_CMD_IO_struct   R_DDR_ADDR_0;	
  /* 1*/AS3_CMD_IO_struct   W_DDR_ADDR_0;	
  /* 2*/AS3_CMD_IO_struct   Hder_Size_0;	
  /* 3*/AS3_CMD_IO_struct   Y_Size_0;	
  /* 4*/AS3_CMD_IO_struct   X_Size_0;	
  /* 5*/AS3_CMD_IO_struct   Page_sel_0;	
  /* 6*/AS3_CMD_IO_struct   Start_En_0;	
  /* 7*/AS3_CMD_IO_struct   H_B_Addr_0;	
  /* 8*/AS3_CMD_IO_struct   H_B_Size_0;	
  /* 9*/AS3_CMD_IO_struct   NoPic_En_0;	
  /*10*/AS3_CMD_IO_struct   S_B_Addr_0;	
  /*11*/AS3_CMD_IO_struct   S_B_Size_0;	
  /*12*/AS3_CMD_IO_struct   S_B_En_0;	
  /*13*/AS3_CMD_IO_struct   E_B_Addr_0;	
  /*14*/AS3_CMD_IO_struct   E_B_Size_0;	
  /*15*/AS3_CMD_IO_struct   Q_table_sel_0;	
  /*16*/AS3_CMD_IO_struct   E_B_En_0;	
  /*17*/AS3_CMD_IO_struct   R_DDR_ADDR_1;	
  /*18*/AS3_CMD_IO_struct   W_DDR_ADDR_1;	
  /*19*/AS3_CMD_IO_struct   Hder_Size_1;	
  /*20*/AS3_CMD_IO_struct   Y_Size_1;	
  /*21*/AS3_CMD_IO_struct   X_Size_1;	
  /*22*/AS3_CMD_IO_struct   Page_sel_1;	
  /*23*/AS3_CMD_IO_struct   Start_En_1;	
  /*24*/AS3_CMD_IO_struct   H_B_Addr_1;	
  /*25*/AS3_CMD_IO_struct   H_B_Size_1;	
  /*26*/AS3_CMD_IO_struct   NoPic_En_1;	
  /*27*/AS3_CMD_IO_struct   S_B_Addr_1;	
  /*28*/AS3_CMD_IO_struct   S_B_Size_1;	
  /*29*/AS3_CMD_IO_struct   S_B_En_1;	
  /*30*/AS3_CMD_IO_struct   E_B_Addr_1;	
  /*31*/AS3_CMD_IO_struct   E_B_Size_1;
  /*32*/AS3_CMD_IO_struct   Q_table_sel_1;
  /*33*/AS3_CMD_IO_struct   E_B_En_1;
  /*34 ~ 63*/AS3_CMD_IO_struct	REV[30];
}AS3_JPEG_CMD_struct;




/*
 *	DMA
 */
typedef struct{
  /* 0*/AS3_CMD_IO_struct   DMA_CMD_DW0_0;
  /* 1*/AS3_CMD_IO_struct   DMA_CMD_DW1_0;
  /* 2*/AS3_CMD_IO_struct   DMA_CMD_DW2_0;
  /* 3*/AS3_CMD_IO_struct   DMA_CMD_DW3_0;
  /* 4*/AS3_CMD_IO_struct   DMA_CMD_DW0_1;
  /* 5*/AS3_CMD_IO_struct   DMA_CMD_DW1_1;
  /* 6*/AS3_CMD_IO_struct   DMA_CMD_DW2_1;
  /* 7*/AS3_CMD_IO_struct   DMA_CMD_DW3_1;
  /* 8*/AS3_CMD_IO_struct   DMA_CMD_DW0_2;
  /* 9*/AS3_CMD_IO_struct   DMA_CMD_DW1_2;
  /*10*/AS3_CMD_IO_struct   DMA_CMD_DW2_2;
  /*11*/AS3_CMD_IO_struct   DMA_CMD_DW3_2;
  /*12*/AS3_CMD_IO_struct   DMA_CMD_DW0_3;
  /*13*/AS3_CMD_IO_struct   DMA_CMD_DW1_3;
  /*14*/AS3_CMD_IO_struct   DMA_CMD_DW2_3;
  /*15*/AS3_CMD_IO_struct   DMA_CMD_DW3_3;
  /*16*/AS3_CMD_IO_struct   DMA_CMD_DW0_4;
  /*17*/AS3_CMD_IO_struct   DMA_CMD_DW1_4;
  /*18*/AS3_CMD_IO_struct   DMA_CMD_DW2_4;
  /*19*/AS3_CMD_IO_struct   DMA_CMD_DW3_4;
  /*20*/AS3_CMD_IO_struct   DMA_CMD_DW0_5;
  /*21*/AS3_CMD_IO_struct   DMA_CMD_DW1_5;
  /*22*/AS3_CMD_IO_struct   DMA_CMD_DW2_5;
  /*23*/AS3_CMD_IO_struct   DMA_CMD_DW3_5;
  /*24*/AS3_CMD_IO_struct   DMA_CMD_DW0_6;
  /*25*/AS3_CMD_IO_struct   DMA_CMD_DW1_6;
  /*26*/AS3_CMD_IO_struct   DMA_CMD_DW2_6;
  /*27*/AS3_CMD_IO_struct   DMA_CMD_DW3_6;
  /*28*/AS3_CMD_IO_struct   DMA_CMD_DW0_7;
  /*29*/AS3_CMD_IO_struct   DMA_CMD_DW1_7;
  /*30*/AS3_CMD_IO_struct   DMA_CMD_DW2_7;
  /*31*/AS3_CMD_IO_struct   DMA_CMD_DW3_7;
  /*32*/AS3_CMD_IO_struct   DMA_CMD_DW0_8;
  /*33*/AS3_CMD_IO_struct   DMA_CMD_DW1_8;
  /*34*/AS3_CMD_IO_struct   DMA_CMD_DW2_8;
  /*35*/AS3_CMD_IO_struct   DMA_CMD_DW3_8;
  /*36*/AS3_CMD_IO_struct   DMA_CMD_DW0_9;
  /*37*/AS3_CMD_IO_struct   DMA_CMD_DW1_9;
  /*38*/AS3_CMD_IO_struct   DMA_CMD_DW2_9;
  /*39*/AS3_CMD_IO_struct   DMA_CMD_DW3_9;
  /*40*/AS3_CMD_IO_struct   DMA_CMD_DW0_10;
  /*41*/AS3_CMD_IO_struct   DMA_CMD_DW1_10;
  /*42*/AS3_CMD_IO_struct   DMA_CMD_DW2_10;
  /*43*/AS3_CMD_IO_struct   DMA_CMD_DW3_10;
  /*44*/AS3_CMD_IO_struct   DMA_CMD_DW0_11;
  /*45*/AS3_CMD_IO_struct   DMA_CMD_DW1_11;
  /*46*/AS3_CMD_IO_struct   DMA_CMD_DW2_11;
  /*47*/AS3_CMD_IO_struct   DMA_CMD_DW3_11;
  /*48*/AS3_CMD_IO_struct   DMA_CMD_DW0_12;
  /*49*/AS3_CMD_IO_struct   DMA_CMD_DW1_12;
  /*50*/AS3_CMD_IO_struct   DMA_CMD_DW2_12;
  /*51*/AS3_CMD_IO_struct   DMA_CMD_DW3_12;
  /*52*/AS3_CMD_IO_struct   DMA_CMD_DW0_13;
  /*53*/AS3_CMD_IO_struct   DMA_CMD_DW1_13;
  /*54*/AS3_CMD_IO_struct   DMA_CMD_DW2_13;
  /*55*/AS3_CMD_IO_struct   DMA_CMD_DW3_13;
  /*56*/AS3_CMD_IO_struct   DMA_CMD_DW0_14;
  /*57*/AS3_CMD_IO_struct   DMA_CMD_DW1_14;
  /*58*/AS3_CMD_IO_struct   DMA_CMD_DW2_14;
  /*59*/AS3_CMD_IO_struct   DMA_CMD_DW3_14;
  /*60~63*/AS3_CMD_IO_struct	REV[4];
}AS3_DMA_CMD_struct;

typedef struct{
  /*17*/AS3_CMD_IO_struct		Reset_Record;
  /* 0*/AS3_CMD_IO_struct		S_Addr;
  /* 1*/AS3_CMD_IO_struct		SM_Addr;
  /* 2*/AS3_CMD_IO_struct		T_Addr;
  /* 3*/AS3_CMD_IO_struct		TM_Addr;                       	//0-23 = SM_Addr
  /* 4*/AS3_CMD_IO_struct		Motion_Addr;                   	//0-23
  /* 5*/AS3_CMD_IO_struct		size_v;                        	//0-6 burst
  /* 6*/AS3_CMD_IO_struct		size_h;                        	//0-6
  /* 7*/AS3_CMD_IO_struct		XY_Rate;    					//0 Div_Mode 1-2 Div_field 3
  /* 8*/AS3_CMD_IO_struct		NewTempCheck;      				//0-15       NewTempLine 16 - 31
  /* 9*/AS3_CMD_IO_struct		B_Mask_Addr;                   	//0-23    B_Mask_Addr_Size 28-30 B_Mask_Addr_En 31
  /*10*/AS3_CMD_IO_struct		QML0;                    		//     13107
  /*11*/AS3_CMD_IO_struct		QSL;                     		//     4
  /*12*/AS3_CMD_IO_struct		DeQML0;                  		//     10
  /*13*/AS3_CMD_IO_struct		QML1;                    		//     8066
  /*14*/AS3_CMD_IO_struct		DeQML1;                  		//     13
  /*15*/AS3_CMD_IO_struct		QML2;                    		//     5423
  /*16*/AS3_CMD_IO_struct		DeQML2;                  		//     16
  /*18*/AS3_CMD_IO_struct		Size_Y;
  /*19*/AS3_CMD_IO_struct		Size_X;                     		//0-6 Bytes
  /*20*/AS3_CMD_IO_struct		I_P_Mode;                   		// I:0   P:1
  /*21~59*/AS3_CMD_IO_struct		Add_Byte[39];        			// Header
  /*60*/AS3_CMD_IO_struct		Add_Last_Byte;       			// 0-7 data 8-11 bit  0011 00000111
  /*61*/AS3_CMD_IO_struct		H264_en;             			//
  /*62*/AS3_CMD_IO_struct		USB_Buffer_Sel;
  /*63*/AS3_CMD_IO_struct  		rev;
}AS3_H264_CMD_struct;
                     
typedef struct  {
  /* 0*/AS3_CMD_IO_struct	S_DDR_ADDR_0;
  /* 1*/AS3_CMD_IO_struct	JPEG_SEL_0;
  /* 2*/AS3_CMD_IO_struct	Start_En_0;
  /* 3*/AS3_CMD_IO_struct	S_DDR_ADDR_1;
  /* 4*/AS3_CMD_IO_struct	JPEG_SEL_1;
  /* 5*/AS3_CMD_IO_struct	Start_En_1;
  /* 6*/AS3_CMD_IO_struct	S_DDR_ADDR_2;
  /* 7*/AS3_CMD_IO_struct	JPEG_SEL_2;
  /* 8*/AS3_CMD_IO_struct	Start_En_2;
  /* 9*/AS3_CMD_IO_struct	S_DDR_ADDR_3;
  /*10*/AS3_CMD_IO_struct	JPEG_SEL_3;
  /*11*/AS3_CMD_IO_struct	Start_En_3;
  /*12~63*/AS3_CMD_IO_struct	REV[52];
}AS3_USB_CMD_struct;


typedef struct{
/* 0*/AS3_CMD_IO_struct	Size;			// 0xCCAA03C1, (Y << 16 | (X / 192)), EX: Y = 5760, X = 11520, Size = ((5760 << 16) | 60)
/* 1*/AS3_CMD_IO_struct	S_DDR_P;		// 0xCCAA03C2, burst
/* 2*/AS3_CMD_IO_struct	T_DDR_P;        // 0xCCAA03C3, burst
/* 3*/AS3_CMD_IO_struct	Start_En;       // 0xCCAA03C0, '1' = Enable, '0' = disable
/* 4 ~ 63*/AS3_CMD_IO_struct	REV[60];
}AS3_SMOOTH_CMD_struct;
 
     
typedef struct{
  /* 0*/AS3_CMD_IO_struct	Start_En_0;
  /* 1*/AS3_CMD_IO_struct	CMD_DW0_0;
  /* 2*/AS3_CMD_IO_struct	SIZE_0;
  /* 3*/AS3_CMD_IO_struct	TAB_ADDR_P_0;
  /* 4*/AS3_CMD_IO_struct	S0_DDR_P_0;
  /* 5*/AS3_CMD_IO_struct	S1_DDR_P_0;
  /* 6*/AS3_CMD_IO_struct	T_DDR_P_0;
  /* 7*/AS3_CMD_IO_struct	S0_DDR_Offset_0;
  /* 8*/AS3_CMD_IO_struct	S1_DDR_Offset_0;
  /* 9*/AS3_CMD_IO_struct	T_DDR_Offset_0;
  /*10*/AS3_CMD_IO_struct	Start_En_1;
  /*11*/AS3_CMD_IO_struct	CMD_DW0_1;
  /*12*/AS3_CMD_IO_struct	SIZE_1;
  /*13*/AS3_CMD_IO_struct	TAB_ADDR_P_1;
  /*14*/AS3_CMD_IO_struct	S0_DDR_P_1;
  /*15*/AS3_CMD_IO_struct	S1_DDR_P_1;
  /*16*/AS3_CMD_IO_struct	T_DDR_P_1;
  /*17*/AS3_CMD_IO_struct	S0_DDR_Offset_1;
  /*18*/AS3_CMD_IO_struct	S1_DDR_Offset_1;
  /*19*/AS3_CMD_IO_struct	T_DDR_Offset_1;
  /*20*/AS3_CMD_IO_struct	Start_En_2;
  /*21*/AS3_CMD_IO_struct	CMD_DW0_2;
  /*22*/AS3_CMD_IO_struct	SIZE_2;
  /*23*/AS3_CMD_IO_struct	TAB_ADDR_P_2;
  /*24*/AS3_CMD_IO_struct	S0_DDR_P_2;
  /*25*/AS3_CMD_IO_struct	S1_DDR_P_2;
  /*26*/AS3_CMD_IO_struct	T_DDR_P_2;
  /*27*/AS3_CMD_IO_struct	S0_DDR_Offset_2;
  /*28*/AS3_CMD_IO_struct	S1_DDR_Offset_2;
  /*29*/AS3_CMD_IO_struct	T_DDR_Offset_2;
  /*30*/AS3_CMD_IO_struct	Start_En_3;
  /*31*/AS3_CMD_IO_struct	CMD_DW0_3;
  /*32*/AS3_CMD_IO_struct	SIZE_3;
  /*33*/AS3_CMD_IO_struct	TAB_ADDR_P_3;
  /*34*/AS3_CMD_IO_struct	S0_DDR_P_3;
  /*35*/AS3_CMD_IO_struct	S1_DDR_P_3;
  /*36*/AS3_CMD_IO_struct	T_DDR_P_3;
  /*37*/AS3_CMD_IO_struct	S0_DDR_Offset_3;
  /*38*/AS3_CMD_IO_struct	S1_DDR_Offset_3;
  /*39*/AS3_CMD_IO_struct	T_DDR_Offset_3;
  /*40*/AS3_CMD_IO_struct	Start_En_4;
  /*41*/AS3_CMD_IO_struct	CMD_DW0_4;
  /*42*/AS3_CMD_IO_struct	SIZE_4;
  /*43*/AS3_CMD_IO_struct	TAB_ADDR_P_4;
  /*44*/AS3_CMD_IO_struct	S0_DDR_P_4;
  /*45*/AS3_CMD_IO_struct	S1_DDR_P_4;
  /*46*/AS3_CMD_IO_struct	T_DDR_P_4;
  /*47*/AS3_CMD_IO_struct	S0_DDR_Offset_4;
  /*48*/AS3_CMD_IO_struct	S1_DDR_Offset_4;
  /*49*/AS3_CMD_IO_struct	T_DDR_Offset_4;
  /*50*/AS3_CMD_IO_struct	Start_En_5;
  /*51*/AS3_CMD_IO_struct	CMD_DW0_5;
  /*52*/AS3_CMD_IO_struct	SIZE_5;
  /*53*/AS3_CMD_IO_struct	TAB_ADDR_P_5;
  /*54*/AS3_CMD_IO_struct	S0_DDR_P_5;
  /*55*/AS3_CMD_IO_struct	S1_DDR_P_5;
  /*56*/AS3_CMD_IO_struct	T_DDR_P_5;
  /*57*/AS3_CMD_IO_struct	S0_DDR_Offset_5;
  /*58*/AS3_CMD_IO_struct	S1_DDR_Offset_5;
  /*59*/AS3_CMD_IO_struct	T_DDR_Offset_5;
  /*60*/AS3_CMD_IO_struct	Mo_Mul_D;
  /*61~63*/AS3_CMD_IO_struct	REV[3];
}AS3_Diffusion_CMD_struct;

typedef struct  {
  /* 0*/AS3_CMD_IO_struct Data_Delay;
  /* 1*/AS3_CMD_IO_struct FS_Delay;
  /* 2*/AS3_CMD_IO_struct Idle_Delay;
  /* 3*/AS3_CMD_IO_struct T_STR_END;
  /* 4*/AS3_CMD_IO_struct T_HS_PAR;
  /* 5*/AS3_CMD_IO_struct V_LP_Cnt;
  /* 6*/AS3_CMD_IO_struct D_Size;
  /* 7*/AS3_CMD_IO_struct SOF_PH;
  /* 8*/AS3_CMD_IO_struct EOF_PH;
  /* 9*/AS3_CMD_IO_struct DATA_PH;
  /*10*/AS3_CMD_IO_struct Mode;
  /*11*/AS3_CMD_IO_struct SDDR_ADDR;  
  /*12*/AS3_CMD_IO_struct REV0;
  /*13*/AS3_CMD_IO_struct REV1;
  /*14*/AS3_CMD_IO_struct REV2;
  /*15*/AS3_CMD_IO_struct MIPI_TX_Delay_Code;
  /*16*/AS3_CMD_IO_struct LDO_TRIM;
  /*17*/AS3_CMD_IO_struct MIPI_TX_En;
  /*18~63*/AS3_CMD_IO_struct	REV[46];
}AS3_MTX_CMD_struct;


typedef struct {
	AS3_ISP1_CMD_struct   		ISP1_CMD;
	AS3_ISP2_CMD_struct   		ISP2_CMD;
	AS3_Stitch_CMD_struct 		STIH_CMD;
	AS3_Diffusion_CMD_struct   	Diff_CMD;
	AS3_JPEG_CMD_struct   		JPEG0_CMD;
	AS3_JPEG_CMD_struct   		JPEG1_CMD;
	AS3_DMA_CMD_struct    		DMA_CMD;
	AS3_H264_CMD_struct    		H264_CMD;
	AS3_MTX_CMD_struct    		MTX_CMD;
	AS3_SMOOTH_CMD_struct    	SMOOTH_CMD;
}AS3_MAIN_CMD_struct;


typedef struct {
	AS3_CMD_IO_struct     SPDDR;
	AS3_CMD_IO_struct     MDDR;
	AS3_CMD_IO_struct     VST;
	AS3_CMD_IO_struct     VMAX;
	AS3_CMD_IO_struct     HMAX;
	AS3_CMD_IO_struct     SOFTNUM;
	AS3_CMD_IO_struct     SRST;
	AS3_CMD_IO_struct     VLWIDTH;
	AS3_CMD_IO_struct     HLWIDTH;
	AS3_CMD_IO_struct     CISPSEL;
	AS3_CMD_IO_struct     MSTART;
	AS3_CMD_IO_struct     CISCMDW;
}AS3_Reg_MAP_struct;

AS3_Reg_MAP_struct  Reg_MAP_P;

#ifdef __cplusplus
}   // extern "C"
#endif

#endif    // __ALETAS3_CMD_STRUCT_H__
