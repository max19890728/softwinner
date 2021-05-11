/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Kernel/jpeg_header.h"

#ifdef __KERNEL__
  #include <linux/string.h>
  #include <linux/time.h>
#else
  #include <string.h>
  #include <stdio.h>
  #include <stdlib.h>
#endif

#include "Device/US363/Kernel/k_spi_cmd.h"
#include "Device/US363/Kernel/us360_define.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::JPEGHeader"

//----- S2 Standard Quality Table -----

//Quantization Table
//Y
int iQY[64] = {
16,	12,	14,	14,	18,	24,	49,	72,
11,	12,	13,	17,	22,	35,	64,	92,
10,	14,	16,	22,	37,	55,	78,	95,
16,	19,	24,	29,	56,	64,	87,	98,
24,	26,	40,	51,	68,	81,	103,	112,
40,	58,	57,	87,	109,	104,	121,	100,
51,	60,	69,	80,	103,	113,	120,	103,
61,	55,	56,	62,	77,	92,	101,	99
};
//C
int iQC[64] = {
17,	18,	24,	47,	99,	99,	99,	99,
18,	21,	26,	66,	99,	99,	99,	99,
24,	26,	56,	99,	99,	99,	99,	99,
47,	66,	99,	99,	99,	99,	99,	99,
99,	99,	99,	99,	99,	99,	99,	99,
99,	99,	99,	99,	99,	99,	99,	99,
99,	99,	99,	99,	99,	99,	99,	99,
99,	99,	99,	99,	99,	99,	99,	99
};
//Standard Y
unsigned QY[64]={
0x2000,	0x2AAB,	0x2492,	0x2492,	0x1C72,	0x1555,	0x0A3D,	0x071C,
0x2AAB,	0x2AAB,	0x2492,	0x1C72,	0x1746,	0x0E39,	0x0800,	0x0591,
0x3333,	0x2492,	0x2000,	0x1746,	0x0D79,	0x0925,	0x0690,	0x0690,
0x2000,	0x199A,	0x1555,	0x1111,	0x0925,	0x0800,	0x047E,	0x0539,
0x1555,	0x13B1,	0x0CCD,	0x09D9,	0x0788,	0x0432,	0x04EC,	0x0492,
0x0CCD,	0x08D4,	0x08D4,	0x05D1,	0x0492,	0x04EC,	0x0432,	0x051F,
0x09D9,	0x0889,	0x0750,	0x051F,	0x04EC,	0x047E,	0x0444,	0x04EC,
0x0842,	0x0925,	0x0444,	0x0842,	0x0690,	0x0591,	0x0505,	0x051F
};
//Standard C
unsigned QC[64]={
0x1C71,	0x1C71,	0x1555,	0x051E,	0x051E,	0x051E,	0x051E,	0x051E,
0x1C71,	0x1745,	0x0AAA,	0x07C1,	0x051E,	0x051E,	0x051E,	0x051E,
0x1555,	0x13B1,	0x0924,	0x051E,	0x051E,	0x051E,	0x051E,	0x051E,
0x0AAA,	0x07C1,	0x051E,	0x051E,	0x051E,	0x051E,	0x051E,	0x051E,
0x051E,	0x051E,	0x051E,	0x051E,	0x051E,	0x051E,	0x051E,	0x051E,
0x051E,	0x051E,	0x051E,	0x051E,	0x051E,	0x051E,	0x051E,	0x051E,
0x051E,	0x051E,	0x051E,	0x051E,	0x051E,	0x051E,	0x051E,	0x051E,
0x051E,	0x051E,	0x051E,	0x051E,	0x051E,	0x051E,	0x051E,	0x051E
};

char tzQY[4][64], tzQC[4][64];
void Quality_Set(int Quality)
{
	int tQY[64],tQC[64];
	unsigned uQY[64],uQC[64];
	unsigned zQY[64],zQC[64];
	int Zigzag[64]={
		0,	1,	5,	6,	14,	15,	27,	28,
		2,	4,	7,	13,	16,	26,	29,	42,
		3,	8,	12,	17,	25,	30,	41,	43,
		9,	11,	18,	24,	31,	40,	44,	53,
		10,	19,	23,	32,	39,	45,	52,	54,
		20,	22,	33,	38,	46,	51,	55,	60,
		21,	34,	37,	47,	50,	56,	59,	61,
		35,	36,	48,	49,	57,	58,	62,	63
	};
	int i, j, scale;
	int Q_Sel = Get_JPEG_Quality_Sel(Quality);
	if(Quality == 0) return;
	if(Quality <= 50){
		scale = 5000 / Quality;
	}else{
		scale = 200 - 2 * Quality;
	}

	for(i = 0; i < 64 ; i ++){
		tQY[i] = (iQY[i] * scale + 50) / 100;
		tQC[i] = (iQC[i] * scale + 50) / 100;
	}
	//取倒數後寫至FPGA Quantization Table
	for(i = 0; i < 8 ; i ++){
		for(j = 0; j < 8; j++) {
			if(tQY[i*8+j] > 1) QY[i+j*8] = 65536 / tQY[i*8+j];
			else               QY[i+j*8] = 65535;
			if(tQC[i*8+j] > 1) QC[i+j*8] = 65536 / tQC[i*8+j];
			else               QC[i+j*8] = 65535;
		}
	}
	for(i = 0; i < 64 ; i ++){
		if(tQY[i] == 0) tzQY[Q_Sel][Zigzag[i]] = (1 & 0xFF);
		else            tzQY[Q_Sel][Zigzag[i]] = (tQY[i] & 0xFF);
		if(tQC[i] == 0) tzQC[Q_Sel][Zigzag[i]] = (1 & 0xFF);
		else            tzQC[Q_Sel][Zigzag[i]] = (tQC[i] & 0xFF);
	}
	db_debug("Quality_Set: Quality=%d Q_Sel=%d\n", Quality, Q_Sel);
	SPI_W(Q_Sel);
}

int JPEG_Battery_State = 0;
int JPEG_Battery_Life = 0;
int JPEG_Battery_Voltage = 0;
void Set_JPEG_Battery(int state, int life, int voltage)
{
	JPEG_Battery_State   = state;
	JPEG_Battery_Life    = life;
	JPEG_Battery_Voltage = voltage;
}

J_Hder_Struct jpeg_header;
unsigned char header_buf[2048];
void Set_JPEG_Header(int doQuality, int Quality, unsigned short Y, unsigned short X, unsigned addr, int hdr_3s, unsigned long long time,
		             unsigned c_mode, int exp_n, int exp_m, int iso, int deGhost_en, int smooth_en, JPEG_HDR_AEB_Info_Struct info, JPEG_UI_Info_Struct ui_info)
{
	int b_state   = JPEG_Battery_State;
	int b_life    = JPEG_Battery_Life;
	int b_voltage = JPEG_Battery_Voltage;

	if(doQuality == 1)
		Quality_Set(Quality);

	// rex+ 180319, 需解決rtsp設定的問題
	#ifndef __KERNEL__
//tmp	set_mjpg_rtsp_quatily(Quality);         // rex+ 170616
	#endif

	if(b_state == 2) b_life = 100;		//充電
	set_app1_data(time, c_mode, deGhost_en, b_life, smooth_en, info, ui_info);
	Make_JPEG_Exif_Header(X, Y, hdr_3s, exp_n, exp_m, iso);
	Make_JPEG_Header(&jpeg_header, Y, X, Quality);

	//spi write ddr
	memcpy(&header_buf[0], &jpeg_header, sizeof(J_Hder_Struct) );
	k_ua360_spi_ddr_write(addr, &header_buf[0], sizeof(header_buf) );
}

/*
 * Write Quantization Table to FPGA
 */
void SPI_W(int Q_Sel){
	unsigned Data[2];
	unsigned Addr = 0x90000;
	int ret,i;

	switch(Q_Sel) {
	case 0: Addr = 0x90000;         break;
	case 1: Addr = 0x90000 + 0x200; break;
	case 2: Addr = 0x90000 + 0x400; break;
	case 3: Addr = 0x90000 + 0x600; break;
	default: Addr = 0x90000;        break;
	}

	for(i = 0 ; i < 64 ; i ++){
		Data[0] = Addr + ((i) << 2);
		Data[1] = QY[i];
		ret = K_SPI_Write_IO_S2(0x9, &Data[0], 8);    // SPI_W
	}
	for(i = 0 ; i < 64 ; i ++){
		Data[0] = Addr + ((i+64) << 2);
		Data[1] = QC[i];
		ret = K_SPI_Write_IO_S2(0x9, &Data[0], 8);    // SPI_W
	}
}


char HT_Y_DC[28] = {
/*  8*/	0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01,
/*  8*/	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
/*  8*/	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
/*  8*/	0x08, 0x09, 0x0a, 0x0b
};
char HT_Y_AC[178] = {
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
char HT_C_DC[28] = {
/*  0*/	0x00, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
/*  8*/	0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
/* 16*/	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
/* 24*/	0x08, 0x09, 0x0a, 0x0b
};
char HT_C_AC[178] = {
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

J_APP1_Data_Struct app1_data;
J_APP2_Data_Struct app2_data;
void set_app1_data(unsigned long long time, unsigned c_mode, unsigned deGhost_en, unsigned battery, unsigned smooth_en, JPEG_HDR_AEB_Info_Struct info, JPEG_UI_Info_Struct ui_info)
{
	memset(&app1_data, 0, sizeof(J_APP1_Data_Struct) );
	app1_data.Time   = time;
	app1_data.C_Mode = c_mode;

	app1_data.HDR_Manual = info.hdr_manual;
	app1_data.HDR_nP = info.hdr_np;
	app1_data.AEB_nP = info.aeb_np;

	app1_data.HDR_Auto_EV[0] = 0;
	app1_data.HDR_Auto_EV[1] = 0;
	if(c_mode == 3)
		app1_data.IncEV = info.aeb_ev;
	else if(c_mode == 5 || c_mode == 7 || c_mode == 13) {
		app1_data.IncEV = info.hdr_ev;
		if(Check_Is_HDR_Auto(app1_data.C_Mode, app1_data.HDR_Manual, app1_data.HDR_Manual) == 1) {
			app1_data.HDR_Auto_EV[0] = info.hdr_auto_ev[0];
			app1_data.HDR_Auto_EV[1] = info.hdr_auto_ev[1];
		}
	}
	else
		app1_data.IncEV = 0;
	app1_data.HDR_Strength = info.strength;

	app1_data.Battery    = battery;
	app1_data.deGhost_en = deGhost_en;
	app1_data.smooth_en  = smooth_en;

	app1_data.WB_Mode      	   	= ui_info.wb_mode;
	app1_data.Color      	   	= ui_info.color;
	app1_data.Tint      	   	= (float)ui_info.tint / 1000.0;
	app1_data.Smooth_Auto_Rate 	= ui_info.smooth;
	app1_data.Sharpness        	= ui_info.sharpness;
	app1_data.Tone 				= ui_info.tone;
	app1_data.Contrast 			= ui_info.contrast;
	app1_data.Saturation 		= ui_info.saturation;
}

J_Exif_Header1_Struct J_Exif_Header1;
J_Exif_Header2_Struct J_Exif_Header2;
J_Exif_GPS_Header_Struct J_Exif_GPS_Header;
int Get_Exif_IFD_Data_Size(uint16_t type, uint32_t cnt)
{
	int size=0;
	switch(type) {
	case 1:  size = EXIF_SIZE_TYPE_1;  break;       //1
	case 2:  size = EXIF_SIZE_TYPE_2;  break;       //1
	case 3:  size = EXIF_SIZE_TYPE_3;  break;       //2
	case 4:  size = EXIF_SIZE_TYPE_4;  break;       //4
	case 5:  size = EXIF_SIZE_TYPE_5;  break;       //8
	case 6:  size = EXIF_SIZE_TYPE_6;  break;       //1
	case 7:  size = EXIF_SIZE_TYPE_7;  break;       //1
	case 8:  size = EXIF_SIZE_TYPE_8;  break;       //2
	case 9:  size = EXIF_SIZE_TYPE_9;  break;       //4
	case 10: size = EXIF_SIZE_TYPE_10; break;       //8
	case 11: size = EXIF_SIZE_TYPE_11; break;       //4
	case 12: size = EXIF_SIZE_TYPE_12; break;       //8
	}
	return (size*cnt);
}

void Make_Exif_Header1(int base_addr, char *header_buf, unsigned long long time)
{
	int size=0, data_size=0, offset=0, header_offset=0;
	uint16_t size_type=0;
	uint32_t cnt=0, data1=0, data2=0;
	char buf[128], tmp_buf[80];
	char *ptr;
	struct tm *timeinfo;
	unsigned long long time_sec;
	char version[128] = "v0.00.00\0";
	char model[128] = "AletaS2\0";

	memset(&buf[0], 0, sizeof(buf) );
	memset(&J_Exif_Header1, 0, sizeof(J_Exif_Header1) );

	ptr = &buf[0];
	header_offset = base_addr;	// + sizeof(J_Exif_Header1) - 6;

	time_sec = time / 1000000;
	#ifndef __KERNEL__
	timeinfo = localtime(&time_sec);
	#else
	struct tm tm_tmp;
	timeinfo = &tm_tmp;
	time_to_tm(time_sec, 0, timeinfo);
	#endif

	J_Exif_Header1.Exif_str  						= CC("Exif");
	J_Exif_Header1.rev1		 						= 0;
	J_Exif_Header1.TIFF_type 						= CC("MM");
	J_Exif_Header1.TIFF_header1						= SWAP16(0x002A);
	J_Exif_Header1.TIFF_header2						= SWAP32(0x00000008);
	J_Exif_Header1.IFD_cnt							= SWAP16(0x000B);		//cnt


	size_type = 2; cnt = 10;
	J_Exif_Header1.IFD_Make.tag						= SWAP16(0x010F);
	J_Exif_Header1.IFD_Make.size_type				= SWAP16(size_type);
	J_Exif_Header1.IFD_Make.cnt						= SWAP32(cnt);
		offset = header_offset + size;
	J_Exif_Header1.IFD_Make.data					= SWAP32(offset);		//addr offset	//AletaS1	Ultracker
		data_size = Get_Exif_IFD_Data_Size(size_type, cnt);
		memset(&tmp_buf[0], 0, sizeof(tmp_buf) );                           //tmp_buf[80]
		//sprintf(tmp_buf, "Ultracker");
		memcpy(ptr, &tmp_buf[0], data_size);
		ptr += data_size;	size += data_size;


	size_type = 2; cnt = 10;
	J_Exif_Header1.IFD_Model.tag					= SWAP16(0x0110);
	J_Exif_Header1.IFD_Model.size_type				= SWAP16(size_type);
	J_Exif_Header1.IFD_Model.cnt					= SWAP32(cnt);
		offset = header_offset + size;
	J_Exif_Header1.IFD_Model.data					= SWAP32(offset);		//addr offset	//AletaS1
		data_size = Get_Exif_IFD_Data_Size(size_type, cnt);
		memset(&tmp_buf[0], 0, sizeof(tmp_buf) );
		get_A2K_Model(&model[0]);
		sprintf(tmp_buf, "%s", model);
		memcpy(ptr, &tmp_buf[0], data_size);
		ptr += data_size;	size += data_size;


	size_type = 3; cnt = 1;
	J_Exif_Header1.IFD_Orientation.tag				= SWAP16(0x0112);
	J_Exif_Header1.IFD_Orientation.size_type		= SWAP16(size_type);
	J_Exif_Header1.IFD_Orientation.cnt				= SWAP32(cnt);
	J_Exif_Header1.IFD_Orientation.data				= 0;


	size_type = 5; cnt = 1;
	J_Exif_Header1.IFD_XResolution.tag				= SWAP16(0x011A);
	J_Exif_Header1.IFD_XResolution.size_type		= SWAP16(size_type);
	J_Exif_Header1.IFD_XResolution.cnt				= SWAP32(cnt);
		offset = header_offset + size;
	J_Exif_Header1.IFD_XResolution.data				= SWAP32(offset);		//addr offset	//00 00 00 48 00 00 00 01
		data_size = Get_Exif_IFD_Data_Size(size_type, cnt);
		data1 = SWAP32(0x00000048); memcpy(ptr, &data1, 4);
		ptr += 4;
		data1 = SWAP32(0x00000001);	memcpy(ptr, &data1, 4);
		ptr += 4;
		size += data_size;


	size_type = 5; cnt = 1;
	J_Exif_Header1.IFD_YResolution.tag				= SWAP16(0x011B);
	J_Exif_Header1.IFD_YResolution.size_type		= SWAP16(size_type);
	J_Exif_Header1.IFD_YResolution.cnt				= SWAP32(cnt);
		offset = header_offset + size;
	J_Exif_Header1.IFD_YResolution.data				= SWAP32(offset);		//addr offset	//00 00 00 48 00 00 00 01
		data_size = Get_Exif_IFD_Data_Size(size_type, cnt);
		data1 = SWAP32(0x00000048);	memcpy(ptr, &data1, 4);
		ptr += 4;
		data1 = SWAP32(0x00000001);	memcpy(ptr, &data1, 4);
		ptr += 4;
		size += data_size;


	size_type = 3; cnt = 1;
	J_Exif_Header1.IFD_ResolutionUnit.tag			= SWAP16(0x0128);
	J_Exif_Header1.IFD_ResolutionUnit.size_type		= SWAP16(size_type);
	J_Exif_Header1.IFD_ResolutionUnit.cnt			= SWAP32(cnt);
	J_Exif_Header1.IFD_ResolutionUnit.data			= SWAP32(0x00020000);


	size_type = 2; cnt = 32;
	J_Exif_Header1.IFD_SoftwareVersion.tag			= SWAP16(0x0131);
	J_Exif_Header1.IFD_SoftwareVersion.size_type	= SWAP16(size_type);
	J_Exif_Header1.IFD_SoftwareVersion.cnt			= SWAP32(cnt);
		offset = header_offset + size;
	J_Exif_Header1.IFD_SoftwareVersion.data			= SWAP32(offset);		//addr offset	//"v1.00.00"
		data_size = Get_Exif_IFD_Data_Size(size_type, cnt);
		memset(&tmp_buf[0], 0, sizeof(tmp_buf) );
		get_A2K_Softwave_Version(&version[0]);
		sprintf(tmp_buf, "%s", version);
		memcpy(ptr, &tmp_buf[0], data_size);
		ptr += data_size;	size += data_size;


	size_type = 2; cnt = 20;
	J_Exif_Header1.IFD_DateTime.tag					= SWAP16(0x0132);
	J_Exif_Header1.IFD_DateTime.size_type			= SWAP16(size_type);
	J_Exif_Header1.IFD_DateTime.cnt					= SWAP32(cnt);
		offset = header_offset + size;
	J_Exif_Header1.IFD_DateTime.data				= SWAP32(offset);		//addr offset	//"YYYY:MM:DD HH:MM:SS"+0x00
		data_size = Get_Exif_IFD_Data_Size(size_type, cnt);
		memset(&tmp_buf[0], 0, sizeof(tmp_buf) );
		#ifndef __KERNEL__
		strftime (tmp_buf, 80, "%Y:%m:%d %H:%M:%S", timeinfo);
		#else
		sprintf(tmp_buf, "%Y:%m:%d %H:%M:%S", timeinfo->tm_year, timeinfo->tm_mon, timeinfo->tm_mday,
		                                      timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
		#endif
		memcpy(ptr, &tmp_buf[0], data_size);
		ptr += data_size;	size += data_size;


	size_type = 3; cnt = 1;
	J_Exif_Header1.IFD_YCbCrPositioning.tag			= SWAP16(0x0213);
	J_Exif_Header1.IFD_YCbCrPositioning.size_type	= SWAP16(size_type);
	J_Exif_Header1.IFD_YCbCrPositioning.cnt			= SWAP32(cnt);
	J_Exif_Header1.IFD_YCbCrPositioning.data		= SWAP32(0x00010000);


	size_type = 4; cnt = 1;
	J_Exif_Header1.IFD_ExifOffset.tag				= SWAP16(0x8769);
	J_Exif_Header1.IFD_ExifOffset.size_type			= SWAP16(size_type);
	J_Exif_Header1.IFD_ExifOffset.cnt				= SWAP32(cnt);
		offset = header_offset + size;
	J_Exif_Header1.IFD_ExifOffset.data				= SWAP32(offset);		//data offset


	size_type = 4; cnt = 1;
	J_Exif_Header1.IFD_GPS.tag						= SWAP16(0x8825);
	J_Exif_Header1.IFD_GPS.size_type				= SWAP16(size_type);
	J_Exif_Header1.IFD_GPS.cnt						= SWAP32(cnt);
		offset = sizeof(J_Exif_Header1) - 6 + EXIF_H_BUF1_MAX + sizeof(J_Exif_Header2) + EXIF_H_BUF2_MAX;
	J_Exif_Header1.IFD_GPS.data						= SWAP32(offset);		//data


	memcpy(header_buf, &buf[0], size);
}

void Make_Exif_Header2(int base_addr, char *header_buf, unsigned long long time, int width, int height,
		int hdr_3s, int exp_n, int exp_m, int iso)
{
    int size=0, data_size=0, offset=0, header_offset=0;
    uint16_t size_type=0;
    uint32_t cnt=0, data1=0, data2=0;
    char buf[256], tmp_buf[80];
    char *ptr;
    //float f_num = 2.1, f_length = 2.8;
    int f_num_tmp = get_A2K_FocalNumber();           // 2.1 x 1000
    int f_length_tmp = get_A2K_FocalLength();        // 2.8 x 1000
    struct tm *timeinfo;
    unsigned long long time_sec;
    int time_ms=0;
    //int exp_n, exp_m, iso;
    int aeb_f_cnt = get_AEB_Frame_Cnt();
    int aeb_inc_ev = get_AEBEv();
    int aeb_ev = 0, aeb_f_mid;
    float exp_ms;
    int c_mode = get_Camera_Mode();
    

    if(exp_n == -1 || exp_m == -1 || iso == -1){
        get_A2K_JPEG_EP_v(&exp_n, &exp_m, &iso);                // 透過 ISP_AEG_IMX222 取得參數
    }
    
    int hdr_level = get_A2K_Sensor_HDR_Level();
    memset(&buf[0], 0, sizeof(buf) );
    memset(&J_Exif_Header2, 0, sizeof(J_Exif_Header2) );

    ptr = &buf[0];
    header_offset = base_addr + sizeof(J_Exif_Header2);

    time_sec = time / 1000000;
    #ifndef __KERNEL__
    timeinfo = localtime(&time_sec);
    #else
    struct tm tm_tmp;
    timeinfo = &tm_tmp;
    time_to_tm(time_sec, 0, timeinfo);
    #endif

    J_Exif_Header2.IFD_cnt                                  = SWAP16(16);       //SWAP16(0x0023);        //cnt

    // EP
    size_type = 5; cnt = 1;
    J_Exif_Header2.IFD_ExposureTime.tag                     = SWAP16(0x829A);
    J_Exif_Header2.IFD_ExposureTime.size_type               = SWAP16(size_type);
    J_Exif_Header2.IFD_ExposureTime.cnt                     = SWAP32(cnt);
        offset = header_offset + size;
    J_Exif_Header2.IFD_ExposureTime.data                    = SWAP32(offset);   //addr offset    // 00 00 00 01 00 00 00 1E = 1/30
        data_size = Get_Exif_IFD_Data_Size(size_type, cnt);
        data1 = SWAP32(exp_n); memcpy(ptr, &data1, 4);
        ptr += 4;//4
        data1 = SWAP32(exp_m); memcpy(ptr, &data1, 4);
        ptr += 4;//8
        size += data_size;

    size_type = 5; cnt = 1;
    J_Exif_Header2.IFD_FNumber.tag                          = SWAP16(0x829D);
    J_Exif_Header2.IFD_FNumber.size_type                    = SWAP16(size_type);
    J_Exif_Header2.IFD_FNumber.cnt                          = SWAP32(cnt);
        offset = header_offset + size;
    J_Exif_Header2.IFD_FNumber.data                         = SWAP32(offset);   //addr offset    // 00 20 0B 20 00 0F 42 40 = F2.1
        data_size = Get_Exif_IFD_Data_Size(size_type, cnt);
        //f_num_tmp = f_num * 1000000;
        data1 = SWAP32(f_num_tmp*1000); memcpy(ptr, &data1, 4);         //2100000
        ptr += 4;//12
        data1 = SWAP32(0x000F4240);    memcpy(ptr, &data1, 4);          //1000000    2100000/1000000=F2.1
        ptr += 4;//16
        size += data_size;

    size_type = 3; cnt = 1;
    J_Exif_Header2.IFD_ExposureProgram.tag                  = SWAP16(0x8822);
    J_Exif_Header2.IFD_ExposureProgram.size_type            = SWAP16(size_type);
    J_Exif_Header2.IFD_ExposureProgram.cnt                  = SWAP32(cnt);
    J_Exif_Header2.IFD_ExposureProgram.data                 = SWAP32(0x00020000);    // 0:未知 1:手動 2:一般 3:光圈先決 4:快門先決 5:景深優先 6:快門優先

    // ISO
    size_type = 3; cnt = 1;
    J_Exif_Header2.IFD_ISOSpeedRatings.tag                  = SWAP16(0x8827);
    J_Exif_Header2.IFD_ISOSpeedRatings.size_type            = SWAP16(size_type);
    J_Exif_Header2.IFD_ISOSpeedRatings.cnt                  = SWAP32(cnt);
    J_Exif_Header2.IFD_ISOSpeedRatings.data                 = SWAP32(iso<<16);     // 7D=125


    size_type = 7; cnt = 4;
    J_Exif_Header2.IFD_ExifVersion.tag                      = SWAP16(0x9000);
    J_Exif_Header2.IFD_ExifVersion.size_type                = SWAP16(size_type);
    J_Exif_Header2.IFD_ExifVersion.cnt                      = SWAP32(cnt);
    J_Exif_Header2.IFD_ExifVersion.data                     = SWAP32(0x30323230);   // "0220"


    size_type = 2; cnt = 20;
    J_Exif_Header2.IFD_DateTimeOriginal.tag                 = SWAP16(0x9003);
    J_Exif_Header2.IFD_DateTimeOriginal.size_type           = SWAP16(size_type);
    J_Exif_Header2.IFD_DateTimeOriginal.cnt                 = SWAP32(cnt);
        offset = header_offset + size;
    J_Exif_Header2.IFD_DateTimeOriginal.data                = SWAP32(offset);       //addr offset    //"YYYY:MM:DD HH:MM:SS"+0x00
        data_size = Get_Exif_IFD_Data_Size(size_type, cnt);
        memset(&tmp_buf[0], 0, sizeof(tmp_buf) );
        #ifndef __KERNEL__
        strftime (tmp_buf, 80, "%Y:%m:%d %H:%M:%S", timeinfo);
        #else
        sprintf(tmp_buf, "%Y:%m:%d %H:%M:%S", timeinfo->tm_year, timeinfo->tm_mon, timeinfo->tm_mday,
                                              timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
        #endif
        memcpy(ptr, &tmp_buf[0], data_size);//16+20=36
        ptr += data_size;    size += data_size;


    size_type = 2; cnt = 20;
    J_Exif_Header2.IFD_DateTimeDigitized.tag                = SWAP16(0x9004);
    J_Exif_Header2.IFD_DateTimeDigitized.size_type          = SWAP16(size_type);
    J_Exif_Header2.IFD_DateTimeDigitized.cnt                = SWAP32(cnt);
        offset = header_offset + size;
    J_Exif_Header2.IFD_DateTimeDigitized.data               = SWAP32(offset);       //addr offset    //"YYYY:MM:DD HH:MM:SS"+0x00
        data_size = Get_Exif_IFD_Data_Size(size_type, cnt);
        memset(&tmp_buf[0], 0, sizeof(tmp_buf) );
        #ifndef __KERNEL__
        strftime (tmp_buf, 80, "%Y:%m:%d %H:%M:%S", timeinfo);
        #else
        sprintf(tmp_buf, "%Y:%m:%d %H:%M:%S", timeinfo->tm_year, timeinfo->tm_mon, timeinfo->tm_mday,
                                              timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
        #endif
        memcpy(ptr, &tmp_buf[0], data_size);//36+20=56
        ptr += data_size;    size += data_size;

//****
/*    size_type = 7; cnt = 4;
    J_Exif_Header2.IFD_ComponentsConfiguration.tag          = SWAP16(0x9101);
    J_Exif_Header2.IFD_ComponentsConfiguration.size_type    = SWAP16(size_type);
    J_Exif_Header2.IFD_ComponentsConfiguration.cnt          = SWAP32(cnt);
    J_Exif_Header2.IFD_ComponentsConfiguration.data         = SWAP32(0x01020300);   //RGB:04 05 06 00    YCbCr:01 02 03 00
*/
//****
/*    size_type = 5; cnt = 1;
    J_Exif_Header2.IFD_CompressedBitsPerPixel.tag           = SWAP16(0x9102);
    J_Exif_Header2.IFD_CompressedBitsPerPixel.size_type     = SWAP16(size_type);
    J_Exif_Header2.IFD_CompressedBitsPerPixel.cnt           = SWAP32(cnt);
        offset = header_offset + size;
    J_Exif_Header2.IFD_CompressedBitsPerPixel.data          = SWAP32(offset);       //addr offset    //00 00 0F A0 00 00 03 E8
        data_size = Get_Exif_IFD_Data_Size(size_type, cnt);
        data1 = 0x00000000;    memcpy(ptr, data1, 4);
        ptr += 4;
        data1 = 0x00000000;    memcpy(ptr, data1, 4);
        ptr += 4;
        size += data_size;
*/
    // EP
/*  size_type = 10; cnt = 1;
    J_Exif_Header2.IFD_ShutterSpeedValue.tag                = SWAP16(0x9201);
    J_Exif_Header2.IFD_ShutterSpeedValue.size_type          = SWAP16(size_type);
    J_Exif_Header2.IFD_ShutterSpeedValue.cnt                = SWAP32(cnt);
        offset = header_offset + size;
    J_Exif_Header2.IFD_ShutterSpeedValue.data               = SWAP32(offset);       //addr offset    //00 00 13 2B 00 00 03 E8  4907/1000=4.907 -> 1/(2^4.907) = 1/30
        data_size = Get_Exif_IFD_Data_Size(size_type, cnt);
        //ep_apex = log2f(ep) * 1000;
        data1 = SWAP32(ep_apex);    memcpy(ptr, &data1, 4);
        ptr += 4;//60
        data1 = SWAP32(0x000003E8);    memcpy(ptr, &data1, 4);
        ptr += 4;//64
        size += data_size;
*/

    size_type = 5; cnt = 1;
    J_Exif_Header2.IFD_ApertureValue.tag                    = SWAP16(0x9202);
    J_Exif_Header2.IFD_ApertureValue.size_type              = SWAP16(size_type);
    J_Exif_Header2.IFD_ApertureValue.cnt                    = SWAP32(cnt);
        offset = header_offset + size;
    J_Exif_Header2.IFD_ApertureValue.data                   = SWAP32(offset);       //addr offset    //00 00 00 D2 00 00 00 64  210/100=F2.1
        data_size = Get_Exif_IFD_Data_Size(size_type, cnt);
        //f_num_tmp = f_num * 100;
        data1 = SWAP32(f_num_tmp/10); memcpy(ptr, &data1, 4);
        ptr += 4;//68
        data1 = SWAP32(0x00000064);     memcpy(ptr, &data1, 4);
        ptr += 4;//72
        size += data_size;

//****
/*    size_type = 10; cnt = 1;
    J_Exif_Header2.IFD_BrightnessValue.tag                  = SWAP16(0x9203);
    J_Exif_Header2.IFD_BrightnessValue.size_type            = SWAP16(size_type);
    J_Exif_Header2.IFD_BrightnessValue.cnt                  = SWAP32(cnt);
        offset = header_offset + size;
    J_Exif_Header2.IFD_BrightnessValue.data                 = SWAP32(offset);       //addr offset    //00 18 30 0C 00 0F 42 40    ???
        data_size = Get_Exif_IFD_Data_Size(size_type, cnt);
        data1 = 0x0018300C; memcpy(ptr, data1, 4);
        ptr += 4;
        data1 = 0x000F4240;    memcpy(ptr, data1, 4);
        ptr += 4;
        size += data_size;
*/
//****
    // -2, -1.6, -1.3, -1, -0.6, -0.3, 0, +0.3 +0.6, +1, +1.3, +1.6, +2
    int ev_tbl[13]={-20, -16, -13, -10, -6, -3, 0, 3, 6, 10, 13, 16, 20};
    int ev_idx = getEVValue() + 6;
    int ev_value = 0;
    if(ev_idx < 0) ev_idx = 0;
    if(ev_idx > 12) ev_idx = 12;
    int hdr_3s_tmp;

    if(hdr_3s > 0 && hdr_3s <= aeb_f_cnt) {		//AEB
    	aeb_f_mid = (aeb_f_cnt >> 1);
    	hdr_3s_tmp = (hdr_3s - 1 + aeb_f_mid) % aeb_f_cnt;
    	aeb_ev = ( ( (hdr_3s_tmp - aeb_f_mid) * aeb_inc_ev) >> 1);
    	ev_value = ev_tbl[ev_idx] + aeb_ev;
//db_debug("Make_Exif_Header2() hdr_3s=%d aeb_ev=%d IncEv=%d ev_idx=%d ev_tbl=%d ev_value=%d\n", hdr_3s, aeb_ev, aeb_inc_ev, ev_idx, ev_tbl[ev_idx], ev_value);
    }
    else
    	ev_value = ev_tbl[ev_idx];

    size_type = 10; cnt = 1;
    J_Exif_Header2.IFD_ExposureBiasValue.tag                = SWAP16(0x9204);
    J_Exif_Header2.IFD_ExposureBiasValue.size_type          = SWAP16(size_type);
    J_Exif_Header2.IFD_ExposureBiasValue.cnt                = SWAP32(cnt);
        offset = header_offset + size;
    J_Exif_Header2.IFD_ExposureBiasValue.data               = SWAP32(offset);       //addr offset    //00 00 00 00 00 00 00 06    ???
        data_size = Get_Exif_IFD_Data_Size(size_type, cnt);
        data1 = SWAP32( (unsigned int)ev_value); memcpy(ptr, &data1, 4);
        ptr += 4;//76
        data1 = SWAP32(0x0000000A);    memcpy(ptr, &data1, 4);
        ptr += 4;//80
        size += data_size;


    size_type = 5; cnt = 1;
    J_Exif_Header2.IFD_MaxApertureValue.tag                 = SWAP16(0x9205);
    J_Exif_Header2.IFD_MaxApertureValue.size_type           = SWAP16(size_type);
    J_Exif_Header2.IFD_MaxApertureValue.cnt                 = SWAP32(cnt);
        offset = header_offset + size;
    J_Exif_Header2.IFD_MaxApertureValue.data                = SWAP32(offset);       //addr offset    //00 00 00 D2 00 00 00 64    210/100=F2.1
        data_size = Get_Exif_IFD_Data_Size(size_type, cnt);
        //f_num_tmp = f_num * 100;
        data1 = SWAP32(f_num_tmp/10); memcpy(ptr, &data1, 4);
        ptr += 4;//84
        data1 = SWAP32(0x00000064);     memcpy(ptr, &data1, 4);
        ptr += 4;//88
        size += data_size;


    size_type = 3; cnt = 1;
    J_Exif_Header2.IFD_MeteringMode.tag                     = SWAP16(0x9207);
    J_Exif_Header2.IFD_MeteringMode.size_type               = SWAP16(size_type);
    J_Exif_Header2.IFD_MeteringMode.cnt                     = SWAP32(cnt);
    J_Exif_Header2.IFD_MeteringMode.data                    = SWAP32(0x00010000);   //0=Unknown 1=平均測光 2=中央加權平均 3=點測光 4=多點測光 5=分區測光 6=局部測光


    size_type = 3; cnt = 1;
    J_Exif_Header2.IFD_Flash.tag                            = SWAP16(0x9209);
    J_Exif_Header2.IFD_Flash.size_type                      = SWAP16(size_type);
    J_Exif_Header2.IFD_Flash.cnt                            = SWAP32(cnt);
    J_Exif_Header2.IFD_Flash.data                           = SWAP32(0x00000000);


    size_type = 5; cnt = 1;
    J_Exif_Header2.IFD_FocalLength.tag                      = SWAP16(0x920A);
    J_Exif_Header2.IFD_FocalLength.size_type                = SWAP16(size_type);
    J_Exif_Header2.IFD_FocalLength.cnt                      = SWAP32(cnt);
        offset = header_offset + size;
    J_Exif_Header2.IFD_FocalLength.data                     = SWAP32(offset);       //addr offset    //00 00 08 FC 00 00 03 E8    2300/1000=2.3mm
        data_size = Get_Exif_IFD_Data_Size(size_type, cnt);
        //f_length_tmp = f_length * 1000;
        data1 = SWAP32(f_length_tmp); memcpy(ptr, &data1, 4);
        ptr += 4;//92
        data1 = SWAP32(0x000003E8);    memcpy(ptr, &data1, 4);
        ptr += 4;//96
        size += data_size;


    int luxValue = get_A2K_LuxValue();
	size_type = 4; cnt = 1;
	J_Exif_Header2.IFD_ImageNumber.tag                   = SWAP16(0x9211);
	J_Exif_Header2.IFD_ImageNumber.size_type             = SWAP16(size_type);
	J_Exif_Header2.IFD_ImageNumber.cnt                   = SWAP32(cnt);
	J_Exif_Header2.IFD_ImageNumber.data                  = SWAP32(luxValue / 2);		//替換為照度值(顯示的照度值減半)

//****
/*    size_type = 7; cnt = 4;
    J_Exif_Header2.IFD_MakerNote.tag                        = SWAP16(0x927C);
    J_Exif_Header2.IFD_MakerNote.size_type                  = SWAP16(size_type);
    J_Exif_Header2.IFD_MakerNote.cnt                        = SWAP32(cnt);
        offset = header_offset + size;
    J_Exif_Header2.IFD_MakerNote.data                       = SWAP32(offset);       //addr offset    //68 74 63 49 00 00 00 00    ......    ???
        data_size = Get_Exif_IFD_Data_Size(size_type, cnt);
        memset(&tmp_buf[0], 0, sizeof(tmp_buf) );
        memcpy(ptr, &tmp_buf[0], data_size);
        ptr += data_size; size += data_size;*/
    size_type = 7; cnt = 4;
    J_Exif_Header2.IFD_MakerNote.tag                        = SWAP16(0x927C);
    J_Exif_Header2.IFD_MakerNote.size_type                  = SWAP16(size_type);
    J_Exif_Header2.IFD_MakerNote.cnt                        = SWAP32(cnt);
    J_Exif_Header2.IFD_MakerNote.data                       = SWAP32(0);       //68 74 63 49 00 00 00 00    ......    ???


    size_type = 2; cnt = 4;
    J_Exif_Header2.IFD_SubsecTimeOriginal.tag               = SWAP16(0x9291);
    J_Exif_Header2.IFD_SubsecTimeOriginal.size_type         = SWAP16(size_type);
    J_Exif_Header2.IFD_SubsecTimeOriginal.cnt               = SWAP32(cnt);
    time_ms = (time / 1000) % 1000;
    memset(&tmp_buf[0], 0, sizeof(tmp_buf) );
    sprintf(tmp_buf, "%d\0", time_ms);
    J_Exif_Header2.IFD_SubsecTimeOriginal.data              = CC(tmp_buf);          //ms    "130 "

//****
/*    size_type = 7; cnt = 4;
    J_Exif_Header2.IFD_FlashPixVersion.tag                  = SWAP16(0xA000);
    J_Exif_Header2.IFD_FlashPixVersion.size_type            = SWAP16(size_type);
    J_Exif_Header2.IFD_FlashPixVersion.cnt                  = SWAP32(cnt);
    J_Exif_Header2.IFD_FlashPixVersion.data                 = SWAP32(0x30313030);   //"0100"
*/
//****
/*    size_type = 3; cnt = 1;
    J_Exif_Header2.IFD_FlashPixVersion.tag                  = SWAP16(0xA001);
    J_Exif_Header2.IFD_FlashPixVersion.size_type            = SWAP16(size_type);
    J_Exif_Header2.IFD_FlashPixVersion.cnt                  = SWAP32(cnt);
    J_Exif_Header2.IFD_FlashPixVersion.data                 = SWAP32(0x00010000);   //1:sRGB
*/
    // width
    size_type = 4; cnt = 1;
    J_Exif_Header2.IFD_ExifImageWidth.tag                   = SWAP16(0xA002);
    J_Exif_Header2.IFD_ExifImageWidth.size_type             = SWAP16(size_type);
    J_Exif_Header2.IFD_ExifImageWidth.cnt                   = SWAP32(cnt);
    J_Exif_Header2.IFD_ExifImageWidth.data                  = SWAP32(width);        //1920

    // height
    size_type = 4; cnt = 1;
    J_Exif_Header2.IFD_ExifImageHeight.tag                  = SWAP16(0xA003);
    J_Exif_Header2.IFD_ExifImageHeight.size_type            = SWAP16(size_type);
    J_Exif_Header2.IFD_ExifImageHeight.cnt                  = SWAP32(cnt);
    J_Exif_Header2.IFD_ExifImageHeight.data                 = SWAP32(height);       //1080

//****
/*    size_type = 4; cnt = 1;
    J_Exif_Header2.IFD_ExifInteroperabilityOffset.tag       = SWAP16(0xA005);
    J_Exif_Header2.IFD_ExifInteroperabilityOffset.size_type = SWAP16(size_type);
    J_Exif_Header2.IFD_ExifInteroperabilityOffset.cnt       = SWAP32(cnt);
    J_Exif_Header2.IFD_ExifInteroperabilityOffset.data      = SWAP32(0x00001710);   // ???
*/
    size_type = 3; cnt = 1;
    J_Exif_Header2.IFD_CustomRendered.tag                   = SWAP16(0xA401);
    J_Exif_Header2.IFD_CustomRendered.size_type             = SWAP16(size_type);
    J_Exif_Header2.IFD_CustomRendered.cnt                   = SWAP32(cnt);
    if(c_mode == 3 || c_mode == 5) data1 = (0x0003<<16) | hdr_level;                // 3=HDR(16bit)
    else                           data1 = 0;
    J_Exif_Header2.IFD_CustomRendered.data                  = SWAP32(data1);        // 0=Normal 1=Custom 3=HDR 6=Panorama

/*
    size_type = 3; cnt = 1;
    J_Exif_Header2.IFD_ExposureMode.tag                     = SWAP16(0xA402);
    J_Exif_Header2.IFD_ExposureMode.size_type               = SWAP16(size_type);
    J_Exif_Header2.IFD_ExposureMode.cnt                     = SWAP32(cnt);
    J_Exif_Header2.IFD_ExposureMode.data                    = SWAP32(0x00000000);

    size_type = 3; cnt = 1;
    J_Exif_Header2.IFD_WhiteBalance.tag                     = SWAP16(0xA403);
    J_Exif_Header2.IFD_WhiteBalance.size_type               = SWAP16(size_type);
    J_Exif_Header2.IFD_WhiteBalance.cnt                     = SWAP32(cnt);
    J_Exif_Header2.IFD_WhiteBalance.data                    = SWAP32(0x00000000);

    size_type = 5; cnt = 1;
    J_Exif_Header2.IFD_DigitalZoomRatio.tag                 = SWAP16(0xA404);
    J_Exif_Header2.IFD_DigitalZoomRatio.size_type           = SWAP16(size_type);
    J_Exif_Header2.IFD_DigitalZoomRatio.cnt                 = SWAP32(cnt);
    J_Exif_Header2.IFD_DigitalZoomRatio.data                = SWAP32(offset);       //addr offset    //..... ????

    size_type = 3; cnt = 1;
    J_Exif_Header2.IFD_FocalLengthIn35mmFilm.tag            = SWAP16(0xA405);
    J_Exif_Header2.IFD_FocalLengthIn35mmFilm.size_type      = SWAP16(size_type);
    J_Exif_Header2.IFD_FocalLengthIn35mmFilm.cnt            = SWAP32(cnt);
    J_Exif_Header2.IFD_FocalLengthIn35mmFilm.data           = SWAP32(0x00000000);

    size_type = 3; cnt = 1;
    J_Exif_Header2.IFD_SceneCaptureType.tag                 = SWAP16(0xA406);
    J_Exif_Header2.IFD_SceneCaptureType.size_type           = SWAP16(size_type);
    J_Exif_Header2.IFD_SceneCaptureType.cnt                 = SWAP32(cnt);
    J_Exif_Header2.IFD_SceneCaptureType.data                = SWAP32(0x00000000);

    size_type = 3; cnt = 1;
    J_Exif_Header2.IFD_GainControl.tag                      = SWAP16(0xA407);
    J_Exif_Header2.IFD_GainControl.size_type                = SWAP16(size_type);
    J_Exif_Header2.IFD_GainControl.cnt                      = SWAP32(cnt);
    J_Exif_Header2.IFD_GainControl.data                     = SWAP32(0x00000000);

    size_type = 3; cnt = 1;
    J_Exif_Header2.IFD_Contrast.tag                         = SWAP16(0xA408);
    J_Exif_Header2.IFD_Contrast.size_type                   = SWAP16(size_type);
    J_Exif_Header2.IFD_Contrast.cnt                         = SWAP32(cnt);
    J_Exif_Header2.IFD_Contrast.data                        = SWAP32(0x00000000);

    size_type = 3; cnt = 1;
    J_Exif_Header2.IFD_Saturation.tag                       = SWAP16(0xA409);
    J_Exif_Header2.IFD_Saturation.size_type                 = SWAP16(size_type);
    J_Exif_Header2.IFD_Saturation.cnt                       = SWAP32(cnt);
    J_Exif_Header2.IFD_Saturation.data                      = SWAP32(0x00000000);

    // 銳利度
    size_type = 3; cnt = 1;
    J_Exif_Header2.IFD_Sharpness.tag                        = SWAP16(0xA40A);
    J_Exif_Header2.IFD_Sharpness.size_type                  = SWAP16(size_type);
    J_Exif_Header2.IFD_Sharpness.cnt                        = SWAP32(cnt);
    J_Exif_Header2.IFD_Sharpness.data                       = SWAP32(0x00000000);   // 銳利度, 0:一般 1:模糊 2:銳利
*/
    if(size > EXIF_H_BUF2_MAX)
        db_error("Make_Exif_Header2: err! size=%d max=%d\n", size, EXIF_H_BUF2_MAX);
    else
        memcpy(header_buf, &buf[0], size);
        
}

//void Make_Exif_GPS_Header(int base_addr, char *header_buf, double latitude, double longitude, double altitude)
void Make_Exif_GPS_Header(int base_addr, char *header_buf)
{
	int size=0, data_size=0, offset=0, header_offset=0;
	uint16_t size_type=0;
	uint32_t cnt=0, data1=0;//, data2=0;
	char buf[128];//, tmp_buf[32];
	char *ptr;
	//struct tm *timeinfo;
	//int tmp1;
	//double latitude_tmp=0, longitude_tmp=0;
	//static double latitude_lst=0, longitude_lst=0;
	//static int latitude_tmp1=0, latitude_tmp2=0, latitude_tmp3=0;
	//static int longitude_tmp1=0, longitude_tmp2=0, longitude_tmp3=0;

	memset(&buf[0], 0, sizeof(buf) );
	memset(&J_Exif_GPS_Header, 0, sizeof(J_Exif_GPS_Header) );

	ptr = &buf[0];
	header_offset = base_addr + sizeof(J_Exif_GPS_Header);

	J_Exif_GPS_Header.IFD_cnt						= SWAP16(0x0006);		//cnt

	size_type = 2; cnt = 2;
	J_Exif_GPS_Header.IFD_GPSLatitudeRef.tag					= SWAP16(0x0001);
	J_Exif_GPS_Header.IFD_GPSLatitudeRef.size_type				= SWAP16(size_type);
	J_Exif_GPS_Header.IFD_GPSLatitudeRef.cnt					= SWAP32(cnt);

	int la[3], la_CC, lo[3], lo_CC, alt;
	get_A2K_JPEG_GPS_v(la, sizeof(la), &la_CC, lo, sizeof(lo), &lo_CC, &alt);
    
	//if(latitude > 0) {
	//	J_Exif_GPS_Header.IFD_GPSLatitudeRef.data				= CC("N   ");
	//	latitude_tmp = latitude;
	//}
	//else {
	//	J_Exif_GPS_Header.IFD_GPSLatitudeRef.data				= CC("S   ");
	//	latitude_tmp = -(latitude);
	//}
	J_Exif_GPS_Header.IFD_GPSLatitudeRef.data					= la_CC;

	size_type = 5; cnt = 3;
	J_Exif_GPS_Header.IFD_GPSLatitude.tag						= SWAP16(0x0002);
	J_Exif_GPS_Header.IFD_GPSLatitude.size_type					= SWAP16(size_type);
	J_Exif_GPS_Header.IFD_GPSLatitude.cnt						= SWAP32(cnt);
		offset = header_offset + size;
	J_Exif_GPS_Header.IFD_GPSLatitude.data						= SWAP32(offset);		//addr offset
		data_size = Get_Exif_IFD_Data_Size(size_type, cnt);
		//if(latitude_lst != latitude) {
		//	latitude_tmp1 = latitude_tmp;
		//	latitude_tmp2 = (latitude_tmp - (double)latitude_tmp1) * 60.0;
		//	latitude_tmp3 = ( ( (latitude_tmp - (double)latitude_tmp1) * 60.0) - (double)latitude_tmp2) * 60.0 * 10000.0;
		//	latitude_lst = latitude;
		//}
		data1 = SWAP32(la[0]); memcpy(ptr, &data1, 4);
		ptr += 4;
		data1 = SWAP32(0x00000001);	memcpy(ptr, &data1, 4);
		ptr += 4;
		data1 = SWAP32(la[1]); memcpy(ptr, &data1, 4);
		ptr += 4;
		data1 = SWAP32(0x00000001);	memcpy(ptr, &data1, 4);
		ptr += 4;
		data1 = SWAP32(la[2]); memcpy(ptr, &data1, 4);
		ptr += 4;
		//data1 = SWAP32(0x00002710);	memcpy(ptr, &data1, 4);
		data1 = SWAP32((int)JPEG_GPS_MUL);	memcpy(ptr, &data1, 4);
		ptr += 4;
		size += data_size;


	size_type = 2; cnt = 2;
	J_Exif_GPS_Header.IFD_GPSLongitudeRef.tag					= SWAP16(0x0003);
	J_Exif_GPS_Header.IFD_GPSLongitudeRef.size_type				= SWAP16(size_type);
	J_Exif_GPS_Header.IFD_GPSLongitudeRef.cnt					= SWAP32(cnt);
	//if(longitude > 0) {
	//	J_Exif_GPS_Header.IFD_GPSLongitudeRef.data				= CC("E   ");
	//	longitude_tmp = longitude;
	//}
	//else {
	//	J_Exif_GPS_Header.IFD_GPSLongitudeRef.data				= CC("W   ");
	//	longitude_tmp = -(longitude);
	//}
	J_Exif_GPS_Header.IFD_GPSLongitudeRef.data					= lo_CC;

	size_type = 5; cnt = 3;
	J_Exif_GPS_Header.IFD_GPSLongitude.tag						= SWAP16(0x0004);
	J_Exif_GPS_Header.IFD_GPSLongitude.size_type				= SWAP16(size_type);
	J_Exif_GPS_Header.IFD_GPSLongitude.cnt						= SWAP32(cnt);
		offset = header_offset + size;
	J_Exif_GPS_Header.IFD_GPSLongitude.data						= SWAP32(offset);		//addr offset
		data_size = Get_Exif_IFD_Data_Size(size_type, cnt);
		//if(longitude_lst != longitude) {
		//	longitude_tmp1 = longitude_tmp;
		//	longitude_tmp2 = (longitude_tmp - (double)longitude_tmp1) * 60.0;
		//	longitude_tmp3 = ( ( (longitude_tmp - (double)longitude_tmp1) * 60.0) - (double)longitude_tmp2) * 60.0 * 10000.0;
		//	longitude_lst = longitude;
		//}
		data1 = SWAP32(lo[0]); memcpy(ptr, &data1, 4);
		ptr += 4;
		data1 = SWAP32(0x00000001);	memcpy(ptr, &data1, 4);
		ptr += 4;
		data1 = SWAP32(lo[1]); memcpy(ptr, &data1, 4);
		ptr += 4;
		data1 = SWAP32(0x00000001);	memcpy(ptr, &data1, 4);
		ptr += 4;
		data1 = SWAP32(lo[2]); memcpy(ptr, &data1, 4);
		ptr += 4;
		//data1 = SWAP32(0x00002710);	memcpy(ptr, &data1, 4);
		data1 = SWAP32((int)JPEG_GPS_MUL);	memcpy(ptr, &data1, 4);
		ptr += 4;
		size += data_size;


	size_type = 1; cnt = 1;
	J_Exif_GPS_Header.IFD_GPSAltitudeRef.tag					= SWAP16(0x0005);
	J_Exif_GPS_Header.IFD_GPSAltitudeRef.size_type				= SWAP16(size_type);
	J_Exif_GPS_Header.IFD_GPSAltitudeRef.cnt					= SWAP32(cnt);
	J_Exif_GPS_Header.IFD_GPSAltitudeRef.data					= 0;					//SWAP32(0x00000000);


	size_type = 5; cnt = 1;
	J_Exif_GPS_Header.IFD_GPSAltitude.tag						= SWAP16(0x0006);
	J_Exif_GPS_Header.IFD_GPSAltitude.size_type					= SWAP16(size_type);
	J_Exif_GPS_Header.IFD_GPSAltitude.cnt						= SWAP32(cnt);
		offset = header_offset + size;
	J_Exif_GPS_Header.IFD_GPSAltitude.data						= SWAP32(offset);		//addr offset
		data_size = Get_Exif_IFD_Data_Size(size_type, cnt);
		//tmp1 = altitude * 1000;
		data1 = SWAP32(alt); memcpy(ptr, &data1, 4);
		ptr += 4;
		data1 = SWAP32(0x000003E8);	memcpy(ptr, &data1, 4);
		ptr += 4;
		size += data_size;


	memcpy(header_buf, &buf[0], size);
}

void Make_JPEG_Exif_Header(int width, int height, int hdr_3s, int exp_n, int exp_m, int iso)
{
	int addr_tmp;
	unsigned long long now_time;

	#ifndef __KERNEL__
//tmp	get_current_usec(&now_time);
	#else
	now_time = get_seconds();
	#endif
    
	addr_tmp = sizeof(J_Exif_Header1) - 6;
	Make_Exif_Header1(addr_tmp, &app2_data.exif_buf1[0], now_time);
	app2_data.exif_h1 = J_Exif_Header1;

	addr_tmp = sizeof(J_Exif_Header1) - 6 + EXIF_H_BUF1_MAX;
	Make_Exif_Header2(addr_tmp, &app2_data.exif_buf2[0], now_time, width, height, hdr_3s, exp_n, exp_m, iso);
	app2_data.exif_h2 = J_Exif_Header2;

	if(get_A2K_JPEG_GPS_En() == 1) {
		addr_tmp = sizeof(J_Exif_Header1) - 6 + EXIF_H_BUF1_MAX + sizeof(J_Exif_Header2) + EXIF_H_BUF2_MAX;
		Make_Exif_GPS_Header(addr_tmp, &app2_data.exif_gps_buf[0]);
		app2_data.exif_gps_h = J_Exif_GPS_Header;
	}
}

void Get_Camera_Mode_Str(int c_mode, int aeb_np, int hdr_manual, char *str)
{
    switch(c_mode) {
    case 0:  sprintf(str, "Capture\0");      		break;
    case 1:  sprintf(str, "Record\0");       		break;
    case 2:  sprintf(str, "TimeLapse\0");    		break;
    case 3:  sprintf(str, "AEB(%dP)\0", aeb_np);	break;
    case 4:  sprintf(str, "RAW(5P)\0");      		break;
    case 5:
    	if(hdr_manual == 2) sprintf(str, "Capture HDR(Auto)\0");
    	else			    sprintf(str, "Capture HDR\0");
    	break;
    case 6:  sprintf(str, "Night\0");        		break;
    case 7:
    	if(hdr_manual == 2) sprintf(str, "Night HDR(Auto)\0");
    	else				sprintf(str, "Night HDR\0");
    	break;
    case 8:  sprintf(str, "Sport\0");        		break;
    case 9:  sprintf(str, "Sport HDR\0");     		break;
    case 10: sprintf(str, "Record WDR\0");    		break;
    case 11: sprintf(str, "Time-Lapse WDR\0"); 		break;
    case 12: sprintf(str, "M-Mode\0");       		break;
    case 13:
    	if(hdr_manual == 1) sprintf(str, "Removal(Auto)\0");
    	else				sprintf(str, "Removal\0");
    	break;
    case 14: sprintf(str, "3D-Model\0");       		break;
    default: sprintf(str, "None\0");         		break;
    }
}

void Get_On_Off_Str(int en, char *str)
{
	if(en == 0) sprintf(str, "Off\0");
	else		sprintf(str, "On\0");
}

void Get_Auto_Stitching_Str(int rate, char *str)
{
	if(rate == 0) 		 sprintf(str, "Manual\0");
	else if(rate == 100) sprintf(str, "Auto\0");
	else				 sprintf(str, "Semi-Auto\0");
}

void Get_Color_Temperature_Str(int mode, int temp, char *str)
{
	if(mode == 0) 		 sprintf(str, "%dK(Auto)\0", temp);
	else				 sprintf(str, "%dK(Manual)\0", temp);
}

void Make_JPEG_Header(J_Hder_Struct *P1, short IH, short IW, short Quality)
{
    short Q_Sel = Get_JPEG_Quality_Sel(Quality);
    char m_str[32], dg_str[8], smooth_str[8], st_str[32], color_str[16];
    float inc_ev = 0, auto_inc_ev[2] = {0};

    memset(P1, 0, sizeof(J_Hder_Struct));
    P1->SOI.Label_H         = 0xFF; // Start Of Image
    P1->SOI.Label_L         = 0xD8;

    P1->APP0.Label_H        = 0xFF; // Marker App0
    P1->APP0.Label_L        = 0xE0;
    P1->APP0.Length_H       = 0x00; // Length of App0 : 0x0010;
    P1->APP0.Length_L       = 0x10;
    P1->APP0.Data[0]        = 0x4A; // JFIF String
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

    P1->APP1.Label_H        = 0xFF; // Marker App1 (REV 128 bits)
    P1->APP1.Label_L        = 0xE1;
    P1->APP1.Length_H       = ((sizeof(P1->APP1.Data) + 2) >> 8) & 0xFF;     //0x00; // Length of App1 : 0x0082;
    P1->APP1.Length_L       = (sizeof(P1->APP1.Data) + 2) & 0xFF;             //0x42;    //0x82;
    Get_Camera_Mode_Str(app1_data.C_Mode, app1_data.AEB_nP, app1_data.HDR_Manual, &m_str[0]);
    Get_On_Off_Str(app1_data.smooth_en, &smooth_str[0]);
    Get_Auto_Stitching_Str(app1_data.Smooth_Auto_Rate, &st_str[0]);
    Get_Color_Temperature_Str(app1_data.WB_Mode, app1_data.Color, &color_str[0]);
    inc_ev = (float)app1_data.IncEV / 20.0;
    if(app1_data.C_Mode == 5 || app1_data.C_Mode == 7 || app1_data.C_Mode == 13) {		//HDR
    	Get_On_Off_Str(app1_data.deGhost_en, &dg_str[0]);
    	if(Check_Is_HDR_Auto(app1_data.C_Mode, app1_data.HDR_Manual, app1_data.HDR_Manual) == 1) {		//Auto
    	    auto_inc_ev[0] = (float)app1_data.HDR_Auto_EV[0] / 20.0;		//EV+0
    	    auto_inc_ev[1] = (float)app1_data.HDR_Auto_EV[1] / 20.0;		//EV-5
    		sprintf(&P1->APP1.Data[0], "S2IF{\"Mode\": \"%s\", \"Time\": %lld, \"Color Temperature\": \"%s\", \"Tint\": \"%.2f%%\", \"Battery\": \"%d%%\", \"Anti-Aliasing\": \"%s\",	\
    				\"Stitching\": \"%s\", \"Sharpness\": %d, \"Tone\": %d, \"Contrast\": %d, \"Saturation\": %d,																\
    				\"Number of Brackets\": %d, \"Bracketing Increment\": \"-%.1f +%.1f\", \"Strength\": %d, \"DeGhost\": \"%s\"}\0",
    				m_str, app1_data.Time, color_str, app1_data.Tint, app1_data.Battery, smooth_str,
    				st_str, app1_data.Sharpness, app1_data.Tone, app1_data.Contrast, app1_data.Saturation,
    				app1_data.HDR_nP, auto_inc_ev[1], auto_inc_ev[0], app1_data.HDR_Strength, dg_str);
    	}
    	else {
    		sprintf(&P1->APP1.Data[0], "S2IF{\"Mode\": \"%s\", \"Time\": %lld, \"Color Temperature\": \"%s\", \"Tint\": \"%.2f%%\", \"Battery\": \"%d%%\", \"Anti-Aliasing\": \"%s\",	\
    				\"Stitching\": \"%s\", \"Sharpness\": %d, \"Tone\": %d, \"Contrast\": %d, \"Saturation\": %d,																\
    				\"Number of Brackets\": %d, \"Bracketing Increment\": \"+-%.1f\", \"Strength\": %d, \"DeGhost\": \"%s\"}\0",
    				m_str, app1_data.Time, color_str, app1_data.Tint, app1_data.Battery, smooth_str,
    				st_str, app1_data.Sharpness, app1_data.Tone, app1_data.Contrast, app1_data.Saturation,
    				app1_data.HDR_nP, inc_ev, app1_data.HDR_Strength, dg_str);
    	}
    }
    else if(app1_data.C_Mode == 3) {		//AEB
    	sprintf(&P1->APP1.Data[0], "S2IF{\"Mode\": \"%s\", \"Time\": %lld, \"Color Temperature\": \"%s\", \"Tint\": \"%.2f%%\", \"Battery\": \"%d%%\", \"Anti-Aliasing\": \"%s\",		\
    			\"Stitching\": \"%s\", \"Sharpness\": %d, \"Tone\": %d, \"Contrast\": %d, \"Saturation\": %d,																	\
    			\"Bracketing Increment\": \"+-%.1f\"}\0",
    			m_str, app1_data.Time, color_str, app1_data.Tint, app1_data.Battery, smooth_str,
    			st_str, app1_data.Sharpness, app1_data.Tone, app1_data.Contrast, app1_data.Saturation,
    			inc_ev);
    }
    else {
    	sprintf(&P1->APP1.Data[0], "S2IF{\"Mode\": \"%s\", \"Time\": %lld, \"Color Temperature\": \"%s\", \"Tint\": \"%.2f%%\", \"Battery\": \"%d%%\", \"Anti-Aliasing\": \"%s\",		\
    			\"Stitching\": \"%s\", \"Sharpness\": %d, \"Tone\": %d, \"Contrast\": %d, \"Saturation\": %d}\0",
    			m_str, app1_data.Time, color_str, app1_data.Tint, app1_data.Battery, smooth_str,
    			st_str, app1_data.Sharpness, app1_data.Tone, app1_data.Contrast, app1_data.Saturation);
    }

    P1->APP2.Label_H        = 0xFF;
    P1->APP2.Label_L        = 0xE1;
    P1->APP2.Length_H       = ((sizeof(P1->APP2.Data) + 2) >> 8) & 0xFF;     //0x00; // Length of App1 : 0x0082;
    P1->APP2.Length_L       = (sizeof(P1->APP2.Data) + 2) & 0xFF;             //0x42;    //0x82;
    if( sizeof(P1->APP2.Data) >= sizeof(J_APP2_Data_Struct) )
        memcpy(&P1->APP2.Data[0], &app2_data, sizeof(J_APP2_Data_Struct) );
    else
    	db_error("Make_JPEG_Header() APP2 size err! (%d:%d)\n", sizeof(P1->APP2.Data), sizeof(J_APP2_Data_Struct) );

    P1->DQT_Y.Label_H       = 0xFF;    // Define Quantization Table : Y
    P1->DQT_Y.Label_L       = 0xDB;
    P1->DQT_Y.Length_H      = 0x00; // Length of DQT : 0x0043
    P1->DQT_Y.Length_L      = 0x43;
    P1->DQT_Y.QT_ID         = 0x00; // Y - Q Table ID : 0x00
    memcpy(&P1->DQT_Y.Data[0],&tzQY[Q_Sel][0],sizeof(tzQY[Q_Sel]));

    P1->DQT_C.Label_H       = 0xFF; // Define Quantization Table : C
    P1->DQT_C.Label_L       = 0xDB;
    P1->DQT_C.Length_H      = 0x00; // Length of DQT : 0x0043
    P1->DQT_C.Length_L      = 0x43;
    P1->DQT_C.QT_ID         = 0x01;    // C - Q Table ID : 0x01
    memcpy(&P1->DQT_C.Data[0],&tzQC[Q_Sel][0],sizeof(tzQC[Q_Sel]));

    P1->SOF0.Label_H        = 0xFF;    // Start Of Frame : 0xFFC0
    P1->SOF0.Label_L        = 0xC0;
    P1->SOF0.Length_H       = 0x00; // Length of SOF : 0x0011
    P1->SOF0.Length_L       = 0x11;
    P1->SOF0.Precision      = 0x08;            // 數據精度 (每個pixel的位元數)
    P1->SOF0.Image_H_H      = (IH >> 8) & 0xFF;    // 圖片的高度
    P1->SOF0.Image_H_L      =  IH       & 0xFF;    // 圖片的高度
    P1->SOF0.Image_W_H      = (IW >> 8) & 0xFF;    // 圖片的寬度
    P1->SOF0.Image_W_L      =  IW       & 0xFF;    // 圖片的寬度
    P1->SOF0.N_Components   = 0x03;
    P1->SOF0.Component_ID0  = 0x01; // 1 -> Y ;
    P1->SOF0.VH_Factor0     = 0x22; // YUV取樣係數 ; bit 0~3 -> Vertical ; bit 4 ~7 -> Horizontal;
    P1->SOF0.QT_ID0         = 0x00; // 使用的量化表編號, 這邊是Y, 採用00號量化表;
    P1->SOF0.Component_ID1  = 0x02; // 2 -> U ;
    P1->SOF0.VH_Factor1     = 0x11; // YUV取樣係數 ; bit 0~3 -> Vertical ; bit 4 ~7 -> Horizontal;
    P1->SOF0.QT_ID1         = 0x01; // 使用的量化表編號, 這邊是C, 採用01號量化表;
    P1->SOF0.Component_ID2  = 0x03; // 3 -> V ;
    P1->SOF0.VH_Factor2     = 0x11; // YUV取樣係數 ; bit 0~3 -> Vertical ; bit 4 ~7 -> Horizontal;
    P1->SOF0.QT_ID2         = 0x01; // 使用的量化表編號, 這邊是C, 採用01號量化表;
    P1->DHT_Y_DC.Label_H    = 0xFF; // Define Huffman Table DC : Y : 0xFFC4
    P1->DHT_Y_DC.Label_L    = 0xC4;
    P1->DHT_Y_DC.Length_H   = 0x00; // Length of DHT : 0x001F
    P1->DHT_Y_DC.Length_L   = 0x1F;
    P1->DHT_Y_DC.HT_ID      = 0x00;
    memcpy(&P1->DHT_Y_DC.Data[0],&HT_Y_DC[0],sizeof(HT_Y_DC));

    P1->DHT_Y_AC.Label_H    = 0xFF; // Define Huffman Table AC : Y : 0xFFC4
    P1->DHT_Y_AC.Label_L    = 0xC4;
    P1->DHT_Y_AC.Length_H   = 0x00; // Length of DHT : 0x00B5
    P1->DHT_Y_AC.Length_L   = 0xB5;
    P1->DHT_Y_AC.HT_ID      = 0x10;
    memcpy(&P1->DHT_Y_AC.Data[0],&HT_Y_AC[0],sizeof(HT_Y_AC));

    P1->DHT_C_DC.Label_H    = 0xFF; // Define Huffman Table DC : C : 0xFFC4
    P1->DHT_C_DC.Label_L    = 0xC4;
    P1->DHT_C_DC.Length_H   = 0x00; // Length of DHT : 0x001F
    P1->DHT_C_DC.Length_L   = 0x1F;
    P1->DHT_C_DC.HT_ID      = 0x01;
    memcpy(&P1->DHT_C_DC.Data[0],&HT_C_DC[0],sizeof(HT_C_DC));

    P1->DHT_C_AC.Label_H    = 0xFF; // Define Huffman Table AC : C : 0xFFC4
    P1->DHT_C_AC.Label_L    = 0xC4;
    P1->DHT_C_AC.Length_H   = 0x00; // Length of DHT : 0x00B5
    P1->DHT_C_AC.Length_L   = 0xB5;
    P1->DHT_C_AC.HT_ID      = 0x11;
    memcpy(&P1->DHT_C_AC.Data[0],&HT_C_AC[0],sizeof(HT_C_AC));

    P1->SOS.Label_H         = 0xFF; // Start Of Scam
    P1->SOS.Label_L         = 0xDA;
    P1->SOS.Length_H        = 0x00; // Length of SOS : 0x000C
    P1->SOS.Length_L        = 0x0C;
    P1->SOS.N_Components    = 0x03; // Number of Components 3 = YUV
    P1->SOS.Component_ID0   = 0x01; // Component ID, 01 = Y
    P1->SOS.HT_ID0          = 0x00; // Huffman Table ID (Used)
    P1->SOS.Component_ID1   = 0x02; // Component ID, 02 = U
    P1->SOS.HT_ID1          = 0x11; // Huffman Table ID (Used)
    P1->SOS.Component_ID2   = 0x03; // Component ID, 03 = V
    P1->SOS.HT_ID2          = 0x11; // Huffman Table ID (Used)
    P1->SOS.Data[0]         = 0x00;
    P1->SOS.Data[1]         = 0x3f;
    P1->SOS.Data[2]         = 0x00;
}

struct Jpeg_sensor_data tmp_sensor_data[10];
void Set_Jpeg_Sensor_Head(int idx, int value) {
	tmp_sensor_data[idx].PoseHeadingDegrees = value;
}

void Set_Jpeg_Sensor_Pitch(int idx, int value) {
	tmp_sensor_data[idx].PosePitchDegrees = value;
}

void Set_Jpeg_Sensor_Roll(int idx, int value) {
	tmp_sensor_data[idx].PoseRollDegrees = value;
}

//PS. "http://ns.adobe.com/xap/1.0/'x'< ..... " 裡的'x'為故意改的, 實際為 ' '( 0x00), 直接使用 ' '(0x00)會有問題
char panorama[0x69E];
char panorama_head_str[0x69E] =
"http://ns.adobe.com/xap/1.0/x<x:xmpmeta xmlns:x=\"adobe:ns:meta/\" x:xmptk=\"Adobe XMP Core 5.1.0-jc003\">\n\
  <rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">\n\
    <rdf:Description rdf:about=\"\" xmlns:GPano=\"http://ns.google.com/photos/1.0/panorama/\">\n\
      <GPano:UsePanoramaViewer>True</GPano:UsePanoramaViewer>\n\
      <GPano:ProjectionType>equirectangular</GPano:ProjectionType>\n\
      <GPano:CroppedAreaImageHeightPixels>1792</GPano:CroppedAreaImageHeightPixels>\n\
      <GPano:CroppedAreaImageWidthPixels>06144</GPano:CroppedAreaImageWidthPixels>\n\
      <GPano:FullPanoHeightPixels>3072</GPano:FullPanoHeightPixels>\n\
      <GPano:FullPanoWidthPixels>06144</GPano:FullPanoWidthPixels>\n\
      <GPano:CroppedAreaTopPixels>0000</GPano:CroppedAreaTopPixels>\n\
      <GPano:CroppedAreaLeftPixels>0000</GPano:CroppedAreaLeftPixels>\n\
      <GPano:PoseRollDegrees>0000.0</GPano:PoseRollDegrees>\n\
      <GPano:PosePitchDegrees>000.0</GPano:PosePitchDegrees>\n\
      <GPano:PoseHeadingDegrees>301.0</GPano:PoseHeadingDegrees>\n\
      <GPano:InitialViewHeadingDegrees>301</GPano:InitialViewHeadingDegrees>\n\
    </rdf:Description>\n\
  </rdf:RDF>\n\
</x:xmpmeta>\n\0";

void set_panorama_head(int width, int height, int Top) {
    int width2, height2;
    int posi = 0;
    char *str = NULL;
    char wstr1[6], wstr2[6], hstr1[5], hstr2[5], topstr[5], leftstr[5], headstr[5], pitchstr[5], rollstr[6],
	initialStr[3];

    memset(panorama, 0, 0x510);
    sprintf(panorama, "%s", panorama_head_str);

    // 實際影像區寬
    sprintf(wstr1, "%05d\0", width);
    // 實際影像區高
    sprintf(hstr1, "%04d\0", height);

    //total 寬
    width2 = width;
    sprintf(wstr2, "%05d\0", width2);

    //total 高
    height2 = (width >> 1);        // google 寬高比似乎需 2:1
    /*if( (width == 3072 && height == 576) || (width == 6144 && height == 1152) ) {
        //height2 = height * 3;
        height2 = (width >> 1);        // google 寬高比似乎需 2:1
    }
    else                                height2 = 360.0 / 210.0 * (float)height;*/
    //height2 = height;
    sprintf(hstr2, "%04d\0", height2);

    // 影像起始位址
    posi = get_CameraPositionMode();
    if(posi == 1) {
        if( (width == 3072 && height == 576) || (width == 6144 && height == 1152) )
            sprintf(topstr, "%04d\0", (int)( (height2 / 2) - ( (15.0 / 180.0) * height2) ) );
        else
            sprintf(topstr, "%04d\0", (height2 - height) );
    }
    else {
        if( (width == 3072 && height == 576) || (width == 6144 && height == 1152) )
            sprintf(topstr, "%04d\0", (int)( (height2 / 2) - ( (52.5 / 180.0) * height2) ) );
        else
            sprintf(topstr, "0000\0");
    }
    sprintf(leftstr, "0000\0");

    int senNum = 0;	//read_sensor_data();
    //if(senNum != -1){			//Auto
    	int initial = tmp_sensor_data[senNum].PoseHeadingDegrees;
    	sprintf(headstr, "%03d.0\0", initial);
    	sprintf(pitchstr, "%05.1f\0", tmp_sensor_data[senNum].PosePitchDegrees);
    		sprintf(rollstr, "%06.1f\0", tmp_sensor_data[senNum].PoseRollDegrees);
    	sprintf(initialStr, "%03d\0", 180);
    /*}else{						//手動
    	sprintf(headstr, "%03d.0\0", 0);
    	sprintf(pitchstr, "%05d.0\0", 0);
    	sprintf(rollstr, "%06d.0\0", 0);
    	sprintf(initialStr, "%03d\0", 0);
    }*/

    //db_debug("set_panorama_head: height2 %d  hstr2 %s\n", height2, hstr2);
    str = strstr(panorama, "ImageWidth");
    if(str != NULL) {
        *(str + 17) = wstr1[0];
        *(str + 18) = wstr1[1];
        *(str + 19) = wstr1[2];
        *(str + 20) = wstr1[3];
        *(str + 21) = wstr1[4];
        //db_debug("CroppedAreaImageWidthPixels str %c\n", *(str+18));
        str = NULL;
    }

    str = strstr(panorama, "ImageHeight");
    if(str != NULL) {
        *(str + 18) = hstr1[0];
        *(str + 19) = hstr1[1];
        *(str + 20) = hstr1[2];
        *(str + 21) = hstr1[3];
        //db_debug("CroppedAreaImageHeightPixels str %c\n", *(str+19));
        str = NULL;
    }

    str = strstr(panorama, "PanoWidth");
    if(str != NULL) {
        *(str + 16) = wstr2[0];
        *(str + 17) = wstr2[1];
        *(str + 18) = wstr2[2];
        *(str + 19) = wstr2[3];
        *(str + 20) = wstr2[4];
        //db_debug("FullPanoWidthPixels str %c\n", *(str+17));
        str = NULL;
    }

    str = strstr(panorama, "PanoHeight");
    if(str != NULL) {
        *(str + 17) = hstr2[0];
        *(str + 18) = hstr2[1];
        *(str + 19) = hstr2[2];
        *(str + 20) = hstr2[3];
        //db_debug("FullPanoHeightPixels str %c\n", *(str+18));
        str = NULL;
    }

    str = strstr(panorama, "TopPixels");
    if(str != NULL) {
        *(str + 10) = topstr[0];
        *(str + 11) = topstr[1];
        *(str + 12) = topstr[2];
        *(str + 13) = topstr[3];
        //db_debug("CroppedAreaTopPixels str %c\n", *(str+11));
        str = NULL;
    }

    str = strstr(panorama, "LeftPixels");
    if(str != NULL) {
        *(str + 11) = leftstr[0];
        *(str + 12) = leftstr[1];
        *(str + 13) = leftstr[2];
        *(str + 14) = leftstr[3];
        //db_debug("CroppedAreaLeftPixels str %c\n", *(str+11));
        str = NULL;
    }

    /*str = strstr(panorama, "RectTop");
    if(str != NULL) {
        *(str + 8)  = topstr[0];
        *(str + 9) = topstr[1];
        *(str + 10) = topstr[2];
        *(str + 11) = topstr[3];
        //db_debug("LargestValidInteriorRectTop str %c\n", *(str+9));
        str = NULL;
    }*/

    /*str = strstr(panorama, "RectLeft");
    if(str != NULL) {
        *(str + 9) = leftstr[0];
        *(str + 10) = leftstr[1];
        *(str + 11) = leftstr[2];
        *(str + 12) = leftstr[3];
        //db_debug("LargestValidInteriorRectLeft str %c\n", *(str+9));
        str = NULL;
    }*/

    /*str = strstr(panorama, "RectWidth");
    if(str != NULL) {
        *(str + 10) = wstr1[0];
        *(str + 11) = wstr1[1];
        *(str + 12) = wstr1[2];
        *(str + 13) = wstr1[3];
        *(str + 14) = wstr1[4];
        //db_debug("LargestValidInteriorRectWidth str %c\n", *(str+11));
        str = NULL;
    }*/

    /*str = strstr(panorama, "RectHeight");
    if(str != NULL) {
        *(str + 11) = hstr1[0];
        *(str + 12) = hstr1[1];
        *(str + 13) = hstr1[2];
        *(str + 14) = hstr1[3];
        //db_debug("LargestValidInteriorRectHeight str %c\n", *(str+12));
        str = NULL;
    }*/

    str = strstr(panorama, "PoseHeading");
	if(str != NULL) {
		*(str + 19) = headstr[0];
		*(str + 20) = headstr[1];
		*(str + 21) = headstr[2];
		*(str + 22) = headstr[3];
		*(str + 23) = headstr[4];
		//db_debug("CroppedAreaImageWidthPixels str %c\n", *(str+18));
		str = NULL;
	}
	str = strstr(panorama, "PosePitch");
	if(str != NULL) {
		*(str + 17) = pitchstr[0];
		*(str + 18) = pitchstr[1];
		*(str + 19) = pitchstr[2];
		*(str + 20) = pitchstr[3];
		*(str + 21) = pitchstr[4];
		//db_debug("CroppedAreaImageWidthPixels str %c\n", *(str+18));
		str = NULL;
	}

	str = strstr(panorama, "PoseRollDegrees");
	if(str != NULL) {
		*(str + 16) = rollstr[0];
		*(str + 17) = rollstr[1];
		*(str + 18) = rollstr[2];
		*(str + 19) = rollstr[3];
		*(str + 20) = rollstr[4];
		*(str + 21) = rollstr[5];
		//db_debug("CroppedAreaImageWidthPixels str %c\n", *(str+18));
		str = NULL;
	}

	str = strstr(panorama, "InitialViewHeading");
	if(str != NULL) {
		*(str + 26) = initialStr[0];
		*(str + 27) = initialStr[1];
		*(str + 28) = initialStr[2];
		//db_debug("CroppedAreaImageWidthPixels str %c\n", *(str+18));
		str = NULL;
	}

    str = strstr(panorama, "/1.0/");
    if(str != NULL) {
        *(str + 5) = 0x00;
        db_debug("<x: str %c\n", *str);
        str = NULL;
    }
}

/*
 * 	加GOOGLE環景檔頭
 */
int Add_Panorama_Header(int width, int height, char *img, FILE *fp) {
	int i, j, idx=0, len, ret;
	char *tmp;
	unsigned char panorama_flag[4] = {0xFF, 0xE1, 0x03, 0xF6};

    if     (width == S2_RES_12K_WIDTH && height == S2_RES_12K_HEIGHT) idx = 1;    //12K    11520
    else if(width == S2_RES_4K_WIDTH  && height == S2_RES_4K_HEIGHT)  idx = 2;    //4K
    else if(width == S2_RES_8K_WIDTH  && height == S2_RES_8K_HEIGHT)  idx = 3;    //8K
    else if(width == S2_RES_6K_WIDTH  && height == S2_RES_6K_HEIGHT)  idx = 6;    //6K
    else if(width == S2_RES_3K_WIDTH  && height == S2_RES_3K_HEIGHT)  idx = 7;    //3K
    else if(width == S2_RES_2K_WIDTH  && height == S2_RES_2K_HEIGHT)  idx = 8;    //2K

    if(idx != 0) {
        tmp = img;
        for(i = 0; i < 2048; i++) {
            if(*tmp == 0xFF && *(tmp+1) == 0xDB)
                break;
            tmp++;
        }
        tmp = img;
        if(fp != NULL)
            fwrite(&tmp[0], i, 1, fp);

        len = sizeof(panorama_head_str);
        for(j = 0; j < len; j++) {
            if(panorama_head_str[j] == '\n' && panorama_head_str[j+1] == '\0') {
                j++;
                break;
            }
        }
        panorama_flag[2] = ((j+2) >> 8) & 0xff;
        panorama_flag[3] = (j+2) & 0xff;

        if     (idx == 1) set_panorama_head(S2_RES_12K_WIDTH, S2_RES_12K_HEIGHT, 0);   // 12K    11520
        else if(idx == 2) set_panorama_head( S2_RES_4K_WIDTH,  S2_RES_4K_HEIGHT, 0);   // 4K
        else if(idx == 3) set_panorama_head( S2_RES_8K_WIDTH,  S2_RES_8K_HEIGHT, 0);   // 8K
        else if(idx == 6) set_panorama_head( S2_RES_6K_WIDTH,  S2_RES_6K_HEIGHT, 0);   // 6K
        else if(idx == 7) set_panorama_head( S2_RES_3K_WIDTH,  S2_RES_3K_HEIGHT, 0);   // 3K
        else if(idx == 8) set_panorama_head( S2_RES_2K_WIDTH,  S2_RES_2K_HEIGHT, 0);   // 2K

        if(fp != NULL) {
        	ret = fwrite(&panorama_flag[0], 4, 1, fp);
        	ret = fwrite(&panorama[0], j, 1, fp);
        }
        //checkW = 1;
        if(ret == 0)
        	return -1;
        else
        	return i;
    }
    else
    	return 0;
}

//刪除0xFF 0xD9前的 0xFF或 0x00
void del_jpeg_error_code(unsigned char *img, int size, int *error_cnt, int *len) {
	int i, cnt=0, step=0;
    unsigned char *ptr;

    ptr = &img[size-1];
    for(i = 0; i < 64; i++) {
    	if(step == 0) {			//找檔尾 0xFF 0xD9
    		if(*ptr == 0xFF && *(ptr+1) == 0xD9)
    			step = 1;
    		ptr--;
    	}
    	else if(step == 1) {	//計數 0xFF 0x00
    		if(*ptr == 0xFF) {
    			cnt++;
    			ptr--;
    		}
    		else if(*ptr == 0x00 && *(ptr-1) == 0xFF) {
    			cnt += 2;
    			ptr -= 2;
    			i++;
    		}
    		else
    			break;
    	}
    }
    *error_cnt = cnt;
    *len = i;
}

int Write_JPEG_Real_Size(char *img, int r_size) {
	int i, step = 0;
	char *ptr;

	ptr = img;
	for(i = 0; i < 1024; i++) {
		if(step == 0 && *ptr == 0xFF && *(ptr+1) == 0xE1 &&
			*(ptr+4) == 0x45 && *(ptr+5) == 0x78 && *(ptr+6) == 0x69 && *(ptr+7) == 0x66) {			//0xFF 0xE1 Size(2Byte) "Exif"
			step = 1;
		}
		else if(step == 1 && *ptr == 0x92 && *(ptr+1) == 0x7C) {	//IFD_MakerNote: 0x927C
			*(ptr+8)  = (r_size >> 24) & 0xFF;
			*(ptr+9)  = (r_size >> 16) & 0xFF;
			*(ptr+10) = (r_size >> 8) & 0xFF;
			*(ptr+11) = r_size & 0xFF;
			step = 2;
			return 1;
		}
		ptr++;
	}
	return 0;
}