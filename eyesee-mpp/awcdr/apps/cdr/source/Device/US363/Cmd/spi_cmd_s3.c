/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Cmd/spi_cmd_s3.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <malloc.h>
#include <pthread.h>

//#include "Device/spi.h"
//#include "Device/US363/us360.h"
//#include "Device/US363/us363_para.h"
#include "Device/US363/Cmd/us363_spi.h"
#include "Device/US363/Cmd/us360_define.h"
#include "Device/US363/Cmd/AletaS3_CMD_Struct.h"
#include "Device/US363/Cmd/CIS_Table.h"
#include "Device/US363/Cmd/AletaS3_Register_MAP.h"
#include "Device/US363/Cmd/CIS_Table.h"
//#include "Device/US363/Cmd/us360_define.h"
//#include "Device/US363/Cmd/variable.h"
//#include "Device/US363/Cmd/Smooth.h"
//#include "Device/US363/Cmd/us360_func.h"
//#include "Device/US363/Kernel/FPGA_Pipe.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::SPICmdS3"

//------------------------- Alets S3 Test -------------------------

#define JPEG_QUALITY		50
#define JPEG_HEADER_ADDR    0x0900000
#define JPEG_X_PIXEL		2048
#define JPEG_Y_PIXEL		1024

#define JPEG0_S_ADDR		0
#define JPEG0_T_ADDR_P0		0x1000000
#define JPEG0_T_ADDR_P1		0x1500000

//#define MTX_S_ADDR		    0x10000000      	//Byte

typedef struct {
	unsigned Addr;
	unsigned Data;
} IO_CMD_Struct;

typedef struct {
	IO_CMD_Struct Data_Delay;   
	IO_CMD_Struct FS_Delay;    
	IO_CMD_Struct Idle_Delay;  
	IO_CMD_Struct T_STR_END;   
	IO_CMD_Struct T_HS_PAR;
	IO_CMD_Struct V_LP_Cnt;  
	IO_CMD_Struct D_Size;    
	IO_CMD_Struct SOF_PH;   
	IO_CMD_Struct EOF_PH;     
	IO_CMD_Struct DATA_PH;
	IO_CMD_Struct Mode;  
	IO_CMD_Struct MIPI_TX_En;
} MIPI_TX_Set_Struct;


typedef struct {                      
	unsigned DataID		:8;
	unsigned WC		:16;   
	unsigned ECC		:8;
} PH_Struct;

typedef struct {
	char 		T_STR;
	char	  	T_END;
	char 		T_LPX;  
	char 		T_HS_PREPARE;
	char	  	T_HS_ZERO;
	char 		T_HS_TRAIL;      
	char 		T_HS_PAR;
} MIPI_Timing_Struct;

typedef struct {
	unsigned DLY_D0		:5;
	unsigned DLY_D1		:5;
	unsigned DLY_D2		:5;
	unsigned DLY_D3		:5;
	unsigned DLY_D4		:5;
	unsigned REV0		:7;
	unsigned DLY_D5		:5;
	unsigned DLY_D6		:5;
	unsigned DLY_D7		:5;
	unsigned DLY_D8		:5;
	unsigned DLY_D9		:5;
	unsigned REV1		:7;
	unsigned DLY_D10	:5;
	unsigned DLY_D11	:5;
	unsigned DLY_D12	:5;
	unsigned DLY_D13	:5;
	unsigned DLY_D14	:5;
	unsigned DLY_D15	:5;
	unsigned REV2		:2;
} DDR_DLY_Struct;
                  
typedef struct {
	unsigned DLY0		:6;
	unsigned DLY1		:6;
	unsigned DLY2		:6;
	unsigned DLY3		:6;
	unsigned DLY4		:6;
	unsigned DLY5L		:2;
	unsigned DLY5H		:4;   
	unsigned DLY6		:6;   
	unsigned DLY7		:6;
	unsigned DLY8		:6;
	unsigned DLY9		:6;
	unsigned DLY10L		:4; 
	unsigned DLY10H		:2;  
	unsigned DLY11		:6;  
	unsigned DLY12		:6;
	unsigned DLY13		:6;
	unsigned DLY14		:6;
	unsigned DLY15		:6;  
	unsigned DLY16		:6;
	unsigned DLY17		:6;
	unsigned DLY18		:6;
	unsigned DLY19		:6;
	unsigned DLY20		:6;
	unsigned DLY21L		:2;
	unsigned DLY21H		:4;
	unsigned DLY22		:6;
	unsigned DLY23		:6;
	unsigned DLY24		:6;
	unsigned DLY25		:6;
	unsigned DLY26L		:4;
	unsigned DLY26H		:2;
	unsigned DLY27		:6;
	unsigned DLY28		:6;
	unsigned DLY29		:6;
	unsigned DLY30		:6;
	unsigned DLY31		:6;
} IDLY_DQ_TEMP_Struct;

AS3_J_Hder_Struct AS3_J_Hder_P;
AS3_MAIN_CMD_struct Init_CMD[2];

char AS3_tzQY[64], AS3_tzQC[64];
unsigned AS3_QY[64], AS3_QC[64];


void AS3_CMD_Init(AS3_MAIN_CMD_struct *F_M_P) {
    unsigned *P;
    int i, j;
    //int sz[10] = {60,63,47,61,34,34,60,63,12,4};
    for(i = 0; i < 10; i++) {
	    switch(i) {
		    case 0: P = (unsigned *) &F_M_P->ISP1_CMD;   break;
		    case 1: P = (unsigned *) &F_M_P->ISP2_CMD;   break;
		    case 2: P = (unsigned *) &F_M_P->STIH_CMD;   break;
		    case 3: P = (unsigned *) &F_M_P->Diff_CMD;   break;
		    case 4: P = (unsigned *) &F_M_P->JPEG0_CMD;  break;
		    case 5: P = (unsigned *) &F_M_P->JPEG1_CMD;  break;
		    case 6: P = (unsigned *) &F_M_P->DMA_CMD;    break;
		    case 7: P = (unsigned *) &F_M_P->H264_CMD;   break;
		    case 8: P = (unsigned *) &F_M_P->MTX_CMD;    break;
		    case 9: P = (unsigned *) &F_M_P->SMOOTH_CMD; break;
	    }
	    for(j = 0; j < 64; j++) {
		    //if(j == sz[i]) break;
        	P[j*2] = 0xCCAA0001 + i * 256 +	j;
	    }
    }
}

void AS3_Make_JPEG_QTable(int Quality) {
    int tQY[64],tQC[64];
    int i,j,scale;
   
    if(Quality <= 50) scale = 5000 / Quality;
    else              scale = 200 - 2 * Quality;

    for(i = 0; i < 64 ; i ++) {
	    tQY[i] = (AS3_iQY[i] * scale + 50) / 100;
	    tQC[i] = (AS3_iQC[i] * scale + 50) / 100;
    }
    for(i = 0; i < 8 ; i ++) {
	    for(j = 0; j < 8 ; j ++){
		    if(tQY[i * 8 + j] > 1) AS3_QY[i + j * 8] = 65536 / tQY[i * 8 + j];
		    else           	       AS3_QY[i + j * 8] = 65535;
		    if(tQC[i * 8 + j] > 1) AS3_QC[i + j * 8] = 65536 / tQC[i * 8 + j];
		    else           	       AS3_QC[i + j * 8] = 65535;
	    }
    }

    for(i = 0; i < 64 ; i ++) {
	    AS3_tzQY[AS3_Zigzag[i]] = tQY[i] & 0xFF;
	    AS3_tzQC[AS3_Zigzag[i]] = tQC[i] & 0xFF;
//	    if(tQY[i] < 1) AS3_tzQY[AS3_Zigzag[i]] = 1;
//	    if(tQC[i] < 1) AS3_tzQC[AS3_Zigzag[i]] = 1;
    }
} 

void AS3_Make_JPEG_Header(AS3_J_Hder_Struct *P1, short IH, short IW) {
	memset(P1,0x00,sizeof(AS3_J_Hder_Struct));
	P1->SOI.Label_H 	    = 0xFF;	// Start Of Image
	P1->SOI.Label_L 	    = 0xD8;
	P1->APP0.Label_H 	    = 0xFF; // Marker App0
	P1->APP0.Label_L 	    = 0xE0;
	P1->APP0.Length_H 	    = 0x00; // Length of App0 : 0x0010;
	P1->APP0.Length_L 	    = 0x10;
	P1->APP0.Data[0]        = 0x4A; //JFIF String
	P1->APP0.Data[1]        = 0x46;
	P1->APP0.Data[2]        = 0x49;
	P1->APP0.Data[3]        = 0x46;
	P1->APP0.Data[4]        = 0x00;
	P1->APP0.Data[5]        = 0x01;
	P1->APP0.Data[6]        = 0x01;
	P1->APP0.Data[7]        = 0x01;
	P1->APP0.Data[8]        = 0x00;
	P1->APP0.Data[9]        = 0x60;
	P1->APP0.Data[10]       = 0x00;
	P1->APP0.Data[11]       = 0x60;
	P1->APP0.Data[12]       = 0x00;
	P1->APP0.Data[13]       = 0x00;
	P1->APP1.Label_H 	    = 0xFF; // Marker App1 (REV 128 bytes)
	P1->APP1.Label_L 	    = 0xE1;
	P1->APP1.Length_H 	    = 0x00; // Length of App1 : 0x0082;
	P1->APP1.Length_L 	    = 0x82; 
	//P1->APP1.Data[0]              // Data of App1 (REV)
	P1->DQT_Y.Label_H	    = 0xFF;	// Define Quantization Table : Y
	P1->DQT_Y.Label_L	    = 0xDB;
	P1->DQT_Y.Length_H      = 0x00; // Length of DQT : 0x0043
	P1->DQT_Y.Length_L      = 0x43;
	P1->DQT_Y.QT_ID		    = 0x00; // Y - Q Table ID : 0x00
	memcpy(&P1->DQT_Y.Data[0],&AS3_tzQY[0],sizeof(AS3_tzQY));
	P1->DQT_C.Label_H	    = 0xFF; // Define Quantization Table : C
	P1->DQT_C.Label_L	    = 0xDB;
	P1->DQT_C.Length_H      = 0x00; // Length of DQT : 0x0043
	P1->DQT_C.Length_L      = 0x43;
	P1->DQT_C.QT_ID      	= 0x01;	// C - Q Table ID : 0x01
	memcpy(&P1->DQT_C.Data[0],&AS3_tzQC[0],sizeof(AS3_tzQC));
	P1->SOF0.Label_H        = 0xFF;	// Start Of Frame : 0xFFC0
	P1->SOF0.Label_L        = 0xC0;
	P1->SOF0.Length_H       = 0x00; // Length of SOF : 0x0011
	P1->SOF0.Length_L       = 0x11;
	P1->SOF0.Precision      = 0x08;			// 數據精度 (每個pixel的位元數)
	P1->SOF0.Image_H_H      = (IH >> 8) & 0xFF;	// 圖片的高度
	P1->SOF0.Image_H_L      =  IH       & 0xFF;	// 圖片的高度
	P1->SOF0.Image_W_H      = (IW >> 8) & 0xFF;	// 圖片的寬度
	P1->SOF0.Image_W_L      =  IW       & 0xFF;	// 圖片的寬度
	P1->SOF0.N_Components   = 0x03;
	P1->SOF0.Component_ID0	= 0x01; // 1 -> Y ;
	P1->SOF0.VH_Factor0     = 0x22; // YUV取樣係數 ; bit 0~3 -> Vertical ; bit 4 ~7 -> Horizontal;
	P1->SOF0.QT_ID0		    = 0x00; // 使用的量化表編號, 這邊是Y, 採用00號量化表;
	P1->SOF0.Component_ID1  = 0x02; // 2 -> U ;
	P1->SOF0.VH_Factor1     = 0x11; // YUV取樣係數 ; bit 0~3 -> Vertical ; bit 4 ~7 -> Horizontal;
	P1->SOF0.QT_ID1      	= 0x01; // 使用的量化表編號, 這邊是C, 採用01號量化表;
	P1->SOF0.Component_ID2  = 0x03; // 3 -> V ;
	P1->SOF0.VH_Factor2     = 0x11; // YUV取樣係數 ; bit 0~3 -> Vertical ; bit 4 ~7 -> Horizontal;
	P1->SOF0.QT_ID2      	= 0x01; // 使用的量化表編號, 這邊是C, 採用01號量化表;
	P1->DHT_Y_DC.Label_H    = 0xFF; // Define Huffman Table DC : Y : 0xFFC4
	P1->DHT_Y_DC.Label_L    = 0xC4;
	P1->DHT_Y_DC.Length_H   = 0x00; // Length of DHT : 0x001F
	P1->DHT_Y_DC.Length_L   = 0x1F;
	P1->DHT_Y_DC.HT_ID   	= 0x00;
	memcpy(&P1->DHT_Y_DC.Data[0],&AS3_HT_Y_DC[0],sizeof(AS3_HT_Y_DC));
	P1->DHT_Y_AC.Label_H    = 0xFF; // Define Huffman Table AC : Y : 0xFFC4
	P1->DHT_Y_AC.Label_L    = 0xC4;
	P1->DHT_Y_AC.Length_H   = 0x00; // Length of DHT : 0x00B5
	P1->DHT_Y_AC.Length_L   = 0xB5;
	P1->DHT_Y_AC.HT_ID   	= 0x10;
	memcpy(&P1->DHT_Y_AC.Data[0],&AS3_HT_Y_AC[0],sizeof(AS3_HT_Y_AC));
	P1->DHT_C_DC.Label_H    = 0xFF; // Define Huffman Table DC : C : 0xFFC4
	P1->DHT_C_DC.Label_L    = 0xC4;
	P1->DHT_C_DC.Length_H   = 0x00; // Length of DHT : 0x001F
	P1->DHT_C_DC.Length_L   = 0x1F;
	P1->DHT_C_DC.HT_ID   	= 0x01;
	memcpy(&P1->DHT_C_DC.Data[0],&AS3_HT_C_DC[0],sizeof(AS3_HT_C_DC));
	P1->DHT_C_AC.Label_H    = 0xFF; // Define Huffman Table AC : C : 0xFFC4
	P1->DHT_C_AC.Label_L    = 0xC4;
	P1->DHT_C_AC.Length_H   = 0x00; // Length of DHT : 0x00B5
	P1->DHT_C_AC.Length_L   = 0xB5;
	P1->DHT_C_AC.HT_ID      = 0x11;
	memcpy(&P1->DHT_C_AC.Data[0],&AS3_HT_C_AC[0],sizeof(AS3_HT_C_AC));
	P1->SOS.Label_H 	    = 0xFF; // Start Of Scam
	P1->SOS.Label_L 	    = 0xDA;
	P1->SOS.Length_H        = 0x00; // Length of SOS : 0x000C
	P1->SOS.Length_L        = 0x0C;                       
	P1->SOS.N_Components    = 0x03; // Number of Components 3 = YUV
	P1->SOS.Component_ID0   = 0x01; // Component ID, 01 = Y
	P1->SOS.HT_ID0       	= 0x00; // Huffman Table ID (Used)
	P1->SOS.Component_ID1   = 0x02; // Component ID, 02 = U
	P1->SOS.HT_ID1       	= 0x11; // Huffman Table ID (Used)
	P1->SOS.Component_ID2   = 0x03; // Component ID, 03 = V
	P1->SOS.HT_ID2       	= 0x11; // Huffman Table ID (Used)
	P1->SOS.Data[0]		    = 0x00;
	P1->SOS.Data[1]		    = 0x3f;
	P1->SOS.Data[2]		    = 0x00;
}

void AS3_Make_JPEG_CMD() {
   int Quality;
   int Addr,i;
   int *data;
   unsigned *Data;
   AS3_J_Hder_Struct *P1;
   AS3_CMD_IO_struct Cbuf[128];
   
   P1 = &AS3_J_Hder_P;
   data = (int *) &AS3_J_Hder_P;
   Quality = JPEG_QUALITY;
   Addr = JPEG_HEADER_ADDR;

   AS3_Make_JPEG_QTable(Quality);
   AS3_Make_JPEG_Header(P1, JPEG_Y_PIXEL, JPEG_X_PIXEL);
   ua360_spi_ddr_write(Addr, data, 1024);
   Data = (unsigned *) &Cbuf[0];
   Addr = 0x90000;
   for(i = 0 ; i < 128 ; i ++) {
	  Cbuf[i].Address = Addr + (i << 2);
	  if(i < 64) Cbuf[i].Data = AS3_QY[i];
	  else       Cbuf[i].Data = AS3_QC[i & 0x3F];
   }
   Data = (unsigned *) &Cbuf[0];
   SPI_Write_IO_S2(0x9, (int *) &Data[0], sizeof(Cbuf)/4);
   Data = (unsigned *) &Cbuf[32];
   SPI_Write_IO_S2(0x9, (int *) &Data[0], sizeof(Cbuf)/4);
   Data = (unsigned *) &Cbuf[64];
   SPI_Write_IO_S2(0x9, (int *) &Data[0], sizeof(Cbuf)/4);
   Data = (unsigned *) &Cbuf[96];
   SPI_Write_IO_S2(0x9, (int *) &Data[0], sizeof(Cbuf)/4);
}

int ECC_Gen(int temp) {
    int i, j, ECC[6];
    int pow;
    int ECCR; 

    int Parity[6][24] = {
	    {0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,1,1,1},
	    {0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,1,1,1,1,1,0,1,1},
	    {0,1,1,1,0,0,0,1,1,1,0,0,0,1,1,1,0,0,0,1,1,1,0,1},
	    {1,0,1,1,0,1,1,0,0,1,0,1,1,0,0,1,0,0,1,0,1,1,1,0},
	    {1,1,0,1,1,0,1,0,1,0,1,0,1,0,1,0,0,1,0,0,1,1,1,1},
	    {1,1,1,0,1,1,0,1,0,0,1,1,0,1,0,0,1,0,0,0,1,1,1,1}
    };
  
    pow = 1;
    ECCR = 0;
    for(i = 0; i < 6; i++) {
        ECC[i] = ((temp >> 0) & 1) * Parity[5-i][0];
	    for(j = 1; j < 24; j++){
		    ECC[i] = ECC[i] ^ (((temp >> (j)) & 1) * Parity[5-i][j]);
	    }
	    ECCR += ECC[i] * pow;
	    pow *= 2;
    }  
    return ECCR;
}

void mipi_tx_cmd_gen(MIPI_TX_Set_Struct *mipi_tx_p, MIPI_Timing_Struct *mipi_timing_p, 
		int h, int v, int fps, int format, int n_lane, int Enable) {
	unsigned dly;
	unsigned T_STR, T_END;
	unsigned T_LPX, T_HS_PREPARE, T_HS_ZERO, T_HS_TRAIL, T_HS_PAR;
	unsigned *SOF_PH, *EOF_PH, *DATA_PH;
	int *data_p;
	int DataType, Mode, t1sec, t1fps;
	float pxbt;

	t1sec = 100000000;	// 100M => 10ns
	PH_Struct SOF_Packet;
	PH_Struct EOF_Packet;
	PH_Struct Data_Packet;
	t1fps = t1sec / fps;
	dly = t1fps / (v + 3);
	T_STR = mipi_timing_p->T_STR;			//62 - 3 	620ns
	T_END = mipi_timing_p->T_END;    		//62 - 3        620ns
	T_LPX = mipi_timing_p->T_LPX;              	// 8 - 3         80ns
	T_HS_PREPARE = mipi_timing_p->T_HS_PREPARE;     // 8 - 3         80ns
	T_HS_ZERO = mipi_timing_p->T_HS_ZERO;         	// 18 - 3       180ns
	T_HS_TRAIL = mipi_timing_p->T_HS_TRAIL;        	// 10 - 3       100ns

	T_HS_PAR = ((T_HS_TRAIL & 0xFF) << 24) | ((T_HS_ZERO & 0xFF) << 16) | ((T_HS_PREPARE & 0xFF) << 8) | (T_LPX & 0xFF);

    DataType = 0x2A;
    switch(format) {
		case 0: DataType = 0x2A; pxbt = 1.0; break; //raw08b
	}
	Mode = 0;
	switch(n_lane) {
		case 1: Mode = 1; break;
		case 2: Mode = 2; break;
		case 4: Mode = 3; break;
	}
	SOF_PH = (unsigned *) &SOF_Packet;
	SOF_Packet.DataID = 0x00;
	SOF_Packet.WC = v;
	SOF_Packet.ECC = ECC_Gen((SOF_Packet.WC << 8) | SOF_Packet.DataID);

	EOF_PH = (unsigned *) &EOF_Packet;
	EOF_Packet.DataID = 0x01;
	EOF_Packet.WC = v;
	EOF_Packet.ECC = ECC_Gen((EOF_Packet.WC << 8) | EOF_Packet.DataID);
                                           
	DATA_PH = (unsigned *) &Data_Packet;
	Data_Packet.DataID = DataType;
	Data_Packet.WC = h * pxbt;
	Data_Packet.ECC = ECC_Gen((Data_Packet.WC << 8) | Data_Packet.DataID);

	mipi_tx_p->Data_Delay.Addr 	= 0x00000101;	mipi_tx_p->Data_Delay.Data 	= dly;
	mipi_tx_p->FS_Delay.Addr 	= 0x00000102;	mipi_tx_p->FS_Delay.Data 	= dly;
	mipi_tx_p->Idle_Delay.Addr 	= 0x00000103;	mipi_tx_p->Idle_Delay.Data 	= (t1fps - dly * (v + 2));
	mipi_tx_p->T_STR_END.Addr 	= 0x00000104;	mipi_tx_p->T_STR_END.Data 	= (T_END << 8) | T_STR;
	mipi_tx_p->T_HS_PAR.Addr 	= 0x00000105;	mipi_tx_p->T_HS_PAR.Data 	= T_HS_PAR;
	mipi_tx_p->V_LP_Cnt.Addr 	= 0x00000106;	mipi_tx_p->V_LP_Cnt.Data 	= (v | 0x4000);		//0x4000: 使用二維記憶體空間
	mipi_tx_p->D_Size.Addr 		= 0x00000107;	mipi_tx_p->D_Size.Data 		= Data_Packet.WC + 4;
	mipi_tx_p->SOF_PH.Addr 		= 0x00000108;	mipi_tx_p->SOF_PH.Data 		= *SOF_PH;     
	mipi_tx_p->EOF_PH.Addr 		= 0x00000109;	mipi_tx_p->EOF_PH.Data 		= *EOF_PH;
	mipi_tx_p->DATA_PH.Addr 	= 0x0000010A;	mipi_tx_p->DATA_PH.Data 	= *DATA_PH;
	mipi_tx_p->Mode.Addr 		= 0x0000010B;	mipi_tx_p->Mode.Data 		= Mode;
	mipi_tx_p->MIPI_TX_En.Addr 	= 0x0000010F;	mipi_tx_p->MIPI_TX_En.Data 	= Enable;
}

void mipi_tx_init(AS3_MTX_CMD_struct* MTX_P) {
    MIPI_TX_Set_Struct mipi_tx;
	MIPI_Timing_Struct mipi_timing;
	int i;
	unsigned *mipi_p;
	int h,v,fps,n_lane,format;
	
	mipi_p = (unsigned *) &mipi_tx;

	mipi_timing.T_STR = 59;		//62 - 3 	620ns
	mipi_timing.T_END = 59;    	//62 - 3        620ns
	mipi_timing.T_LPX = 5;          // 8 - 3         80ns
	mipi_timing.T_HS_PREPARE = 5;   // 8 - 3         80ns
	mipi_timing.T_HS_ZERO = 15;     // 18 - 3       180ns
	mipi_timing.T_HS_TRAIL = 7;     // 10 - 3       100ns

	h 	    = 3840;		//StrToInt(Form1->AS3_MIPI_TX_ReSolution_H_Edit->Text);
	v 	    = 1920;		//StrToInt(Form1->AS3_MIPI_TX_ReSolution_V_Edit->Text);
	fps 	= 15;		//StrToInt(Form1->AS3_MIPI_TX_FPS_Edit->Text);
	format 	= 0;		//StrToInt(Form1->AS3_MIPI_TX_Image_Mode_Edit->Text);
	n_lane 	= 4;		//StrToInt(Form1->AS3_MIPI_TX_Lane_Edit->Text);

//	h = 3840;  //AS3_MIPI_TX_ReSolution_H_Edit
//	v = 1920;  //AS3_MIPI_TX_ReSolution_V_Edit
//	fps = 15;  //AS3_MIPI_TX_FPS_Edit
//	format = 0;//AS3_MIPI_TX_Image_Mode_Edit
//	n_lane = 4;//AS3_MIPI_TX_Lane_Edit

//  mipi_tx_test_pat_write();
	
	mipi_tx_cmd_gen(&mipi_tx, &mipi_timing, h, v, fps, format,n_lane, 1);
	MTX_P->Data_Delay.Data 	= mipi_tx.Data_Delay.Data;
	MTX_P->FS_Delay.Data 	= mipi_tx.FS_Delay.Data;
	MTX_P->Idle_Delay.Data 	= mipi_tx.Idle_Delay.Data;
	MTX_P->T_STR_END.Data 	= mipi_tx.T_STR_END.Data;
	MTX_P->T_HS_PAR.Data 	= mipi_tx.T_HS_PAR.Data;
	MTX_P->V_LP_Cnt.Data 	= mipi_tx.V_LP_Cnt.Data;
	MTX_P->D_Size.Data  	= mipi_tx.D_Size.Data;
	MTX_P->SOF_PH.Data 	    = mipi_tx.SOF_PH.Data;
	MTX_P->EOF_PH.Data 	    = mipi_tx.EOF_PH.Data;   
	MTX_P->DATA_PH.Data 	= mipi_tx.DATA_PH.Data;
	MTX_P->Mode.Data 	    = mipi_tx.Mode.Data;
	MTX_P->MIPI_TX_En.Data 	= mipi_tx.MIPI_TX_En.Data;
	
	MTX_P->SDDR_ADDR.Data 	= (MTX_S_ADDR >> 5);
db_debug("max+ mipi_tx_init: SDDR_ADDR=0x%x", MTX_P->SDDR_ADDR.Data);	
	MTX_P->MIPI_TX_Delay_Code.Data 	= 0;
	MTX_P->LDO_TRIM.Data 	= 0;

	//SPI_Write_IO_S2(0x9, (int *) mipi_p, sizeof(MIPI_TX_Set_Struct) );
}

void as3_main_cmd_init() {
	AS3_MAIN_CMD_struct *F_M_P;
	int Addr, i, j;
	int t_size, size;
	char *Data;
	unsigned DBT[2];
	
	AS3_CMD_Init(&Init_CMD[0]); 
	AS3_CMD_Init(&Init_CMD[1]);

    AS3_Make_JPEG_CMD();

	F_M_P = &Init_CMD[0];
	mipi_tx_init(&F_M_P->MTX_CMD);

	//DBT Start 
//	DBT[0]= 0xF10;
//	DBT[1]= 0x10000000;
//	SPI_Write_IO_S2(0x9, (int *) &DBT[0], 8);
	//DBT
/*
	F_M_P->JPEG0_CMD.R_DDR_ADDR_0.Data = JPEG0_S_ADDR >> 5;
	F_M_P->JPEG0_CMD.W_DDR_ADDR_0.Data = JPEG0_T_ADDR_P0 >> 5;
	F_M_P->JPEG0_CMD.Hder_Size_0.Data = sizeof(AS3_J_Hder_P);
	F_M_P->JPEG0_CMD.Y_Size_0.Data = JPEG_Y_PIXEL;       
	F_M_P->JPEG0_CMD.X_Size_0.Data = JPEG_X_PIXEL;      
	F_M_P->JPEG0_CMD.Page_sel_0.Data = 0;
	F_M_P->JPEG0_CMD.Start_En_0.Data = 1;          
	F_M_P->JPEG0_CMD.S_B_En_0.Data = 0;         
	F_M_P->JPEG0_CMD.E_B_En_0.Data = 0;
	F_M_P->JPEG0_CMD.Q_table_sel_0.Data = 0;     
	F_M_P->JPEG0_CMD.H_B_Addr_0.Data = JPEG_HEADER_ADDR >> 5;

	F_M_P->USB_CMD.S_DDR_ADDR_0.Data = JPEG0_T_ADDR_P1 >> 5;
	F_M_P->USB_CMD.JPEG_SEL_0.Data = 0;
	F_M_P->USB_CMD.Start_En_0.Data = 1;
 */
	                       
	F_M_P = &Init_CMD[1];   
	mipi_tx_init(&F_M_P->MTX_CMD); 
/*
	F_M_P->JPEG0_CMD.R_DDR_ADDR_0.Data = JPEG0_S_ADDR >> 5;
	F_M_P->JPEG0_CMD.W_DDR_ADDR_0.Data = JPEG0_T_ADDR_P1 >> 5;
	F_M_P->JPEG0_CMD.Hder_Size_0.Data = sizeof(AS3_J_Hder_P); 
	F_M_P->JPEG0_CMD.Y_Size_0.Data = JPEG_Y_PIXEL;       
	F_M_P->JPEG0_CMD.X_Size_0.Data = JPEG_X_PIXEL;      
	F_M_P->JPEG0_CMD.Page_sel_0.Data = 0;
	F_M_P->JPEG0_CMD.Start_En_0.Data = 1;          
	F_M_P->JPEG0_CMD.S_B_En_0.Data = 0;         
	F_M_P->JPEG0_CMD.E_B_En_0.Data = 0;          
	F_M_P->JPEG0_CMD.Q_table_sel_0.Data = 0;   
	F_M_P->JPEG0_CMD.H_B_Addr_0.Data = JPEG_HEADER_ADDR >> 5;
      
	F_M_P->USB_CMD.S_DDR_ADDR_0.Data = JPEG0_T_ADDR_P0 >> 5;    
	F_M_P->USB_CMD.JPEG_SEL_0.Data = 0;                   
	F_M_P->USB_CMD.Start_En_0.Data = 1;
*/	
	for(i = 0 ; i < (CMD_MAX + 1) ; i ++) {
		Data = &Init_CMD[i&1];
		t_size = sizeof(Init_CMD[i&1]);
		for(j = 0; j < t_size; j += 3584) {
			if((t_size-j) >= 3584) size = 3584;
			else				   size = t_size - j;
			Addr = (MAIN_ADDR) + i * 8192 + j;    			
			ua360_spi_ddr_write(Addr, (int *) Data, size);
			Data += size;
		}
	}
}

void as3_reg_addr_init() {
    unsigned *P, Data[2];
    int i;
    AS3_Reg_MAP_struct *R_M_P;
	
    R_M_P = &Reg_MAP_P;
    P = (unsigned *) &Reg_MAP_P;
    R_M_P->SPDDR.Address 	=  SPDDR_ADDR;		    R_M_P->SPDDR.Data 	=  SPDDR_DATA;
    R_M_P->MDDR.Address 	=  MDDR_ADDR;           R_M_P->MDDR.Data 	=  MDDR_DATA;
    R_M_P->VST.Address 		=  VST_ADDR;            R_M_P->VST.Data 	=  VST_DATA;
    R_M_P->VMAX.Address 	=  VMAX_ADDR;           R_M_P->VMAX.Data 	=  VMAX_DATA;
    R_M_P->HMAX.Address 	=  HMAX_ADDR;           R_M_P->HMAX.Data 	=  HMAX_DATA;
    R_M_P->SOFTNUM.Address 	=  SOFTNUM_ADDR;        R_M_P->SOFTNUM.Data =  SOFTNUM_DATA;
    R_M_P->SRST.Address 	=  SRST_ADDR;           R_M_P->SRST.Data 	=  SRST_DATA;
    R_M_P->VLWIDTH.Address 	=  VLWIDTH_ADDR;        R_M_P->VLWIDTH.Data =  VLWIDTH_DATA;
    R_M_P->HLWIDTH.Address 	=  HLWIDTH_ADDR;        R_M_P->HLWIDTH.Data =  HLWIDTH_DATA;
    R_M_P->CISPSEL.Address 	=  CISPSEL_ADDR;        R_M_P->CISPSEL.Data =  CISPSEL_DATA;
    R_M_P->MSTART.Address 	=  MSTART_ADDR;         R_M_P->MSTART.Data 	=  MSTART_DATA;

    as3_main_cmd_init();
    
    for(i = 0; i < (sizeof(Reg_MAP_P) / 8 - 1); i++) {
	    Data[0]= P[i * 2];
	    Data[1]= P[i * 2 + 1];
	    SPI_Write_IO_S2(0x9, (int *) &Data[0], 8);
    }
}

void DDR_Reset() {
	unsigned Data[2];
    Data[0] = 0x920;
    Data[1] = 2;
    SPI_Write_IO_S2(0x9, (int *) &Data[0], 8);	
	
	usleep(100000);
	Data[0] = 0x920;
    Data[1] = 0;
    SPI_Write_IO_S2(0x9, (int *) &Data[0], 8);	
}

//------------------------- Alets S3 Test -------------------------