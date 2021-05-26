/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Cmd/spi_cmd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <malloc.h>
#include <pthread.h>

#include "Device/spi.h"
#include "Device/US363/us360.h"
#include "Device/US363/us363_para.h"
#include "Device/US363/Cmd/us363_spi.h"
#include "Device/US363/Cmd/us360_define.h"
#include "Device/US363/Cmd/AletaS2_CMD_Struct.h"
#include "Device/US363/Cmd/variable.h"
#include "Device/US363/Cmd/Smooth.h"
#include "Device/US363/Cmd/us360_func.h"
#include "Device/US363/Kernel/FPGA_Pipe.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::SPICmd"

int F0_Speed = FX_SPEED_DEFAULT;
int F1_Speed = FX_SPEED_DEFAULT;
int F2_Speed = F2_SPEED_DEFAULT;

float Saturation_C  = 1;
float Saturation_Ku = 0;
float Saturation_Kv = 0;
float Saturation_Th = 256;

unsigned Read_FX_HDR_X = 288;
unsigned Read_FX_HDR_Y = 400;
//unsigned HDR_Img_Avg[7];		//8x8 Avg
//unsigned HDR_Mo_Avg[7];		//8x8 Avg
unsigned char HDR_Img_Buf[7][2][1024];
unsigned char HDR_Mo_Buf[7][2][1024];

int FPGA_Sleep_En = 0;

unsigned ST_Cmd_Cal_CheckSum[2] = {0};
unsigned ST_Sen_Cmd_Cal_CheckSum[2] = {0};
unsigned ST_Tran_Cmd_Cal_CheckSum[2] = {0};
unsigned MP_B_Table_Cal_CheckSum[2][2] = {0};		//[f_id][idx]
unsigned Sen_Lens_Adj_Cal_CheckSum[3][5] = {0};		//[binn][s_id]

void writeSPIUSB() {
    int ret;
    unsigned Data[2];
    db_debug("writeSPIUSB: ...\n");

    Data[0] = 0xae4;
    Data[1] = 1;
    ret = SPI_Write_IO_S2(0x9, (int *) &Data[0], 8);    // writeSPIUSB
    usleep(200000);

    Data[0] = 0xadc;
    Data[1] = 1;
    ret = SPI_Write_IO_S2(0x9, (int *) &Data[0], 8);    // writeSPIUSB
    usleep(800000);        // 延遲800ms, 避免 writeSPIUSB() 重新連線未完成, 導致找不到 Video 0

    return;
}

int readCmdIdx() {
    int i, idx = 0, addr = 0;
    addr = 0xBE8;
    for(i = 0; i < 3; i ++){
        spi_read_io_porcess_S2(addr, &idx, 4);
        if(idx < M_CMD_PAGE_N) break;
    }
    if(idx >= M_CMD_PAGE_N) 
		db_error("readCmdIdx: idx=%d\n", idx);
    return(idx&(M_CMD_PAGE_N-1) );
}

int AS2_RX_Delay_Cnt(int Add, int ChSel, int En) {
	int INC,DEC;
	unsigned cmd, Data[2];
	INC = 0;
	DEC  = 0;
	if(Add == 1) INC = En;
	else         DEC = En;

	cmd =  (INC << 8) | DEC;

	if(ChSel == 1) Data[1] = cmd | 0x1000000;
	else           Data[1] = cmd;

	Data[0] = 0xD10;
	SPI_Write_IO_S2(0x9, (int *) &Data[0], 8);     // AS2_RX_Delay_Cnt

	spi_read_io_porcess_S2(Data[0], (int *) &Data[1], 4);

	return Data[1];
}

void RXDelaySet(int TValue, int ChSel, int PSel) {
	int Value,RValue;
	int TimeOut;
	int sel,i;
	unsigned temp;
	RValue = AS2_RX_Delay_Cnt(0,ChSel,0);

	sel = 0;
	for(i = 0 ; i < 5 ; i ++){
        if(((PSel >> (4 - i)) & 1) == 1){
			sel = (4 - i);
			break;
		}
	}
	TimeOut = 33;
	while(TValue != RValue){
		TimeOut--;
		if(TimeOut == 0) break;
		Value = TValue - RValue;
		if(Value > 0) temp = AS2_RX_Delay_Cnt(1,ChSel,PSel);
		else          temp = AS2_RX_Delay_Cnt(0,ChSel,PSel);
		switch(sel){
			case 0: RValue = ((temp >>  0) & 0x1F); break;
			case 1: RValue = ((temp >>  5) & 0x1F); break;
			case 2: RValue = ((temp >> 10) & 0x1F); break;
			case 3: RValue = ((temp >> 15) & 0x1F); break;
			case 4: RValue = ((temp >> 20) & 0x1F); break;
			default: RValue = 0;
		}
	}
}

/*
 * 	mode: 0:DDR_to_IO  1:DDR_to_DDR
 */
void AS2_Write_F2_to_FX_CMD(unsigned S_Addr, unsigned T_Addr, unsigned size, unsigned mode, unsigned id, unsigned en) {
	int i;
	unsigned Addr;
	AS2_SPI_Cmd_struct TP[2];
	AS2_CMD_IO_struct IOP[7];
	unsigned *uTP;

	//F2 to FX DDR, Size至少128 Byte
	if(mode == 1 && size < 128)
		db_error("AS2_Write_F2_to_FX_CMD() Size Err! (mode=%d size=%d)\n", mode, size);

	TP[0].S_DDR_P = S_Addr >> 5;
	TP[0].T_DDR_P = T_Addr >> 5;
	if(mode == 0)
		TP[0].Size = (size >> 2);
	else
		TP[0].Size = (size >> 5);
	TP[0].Mode = mode;
	TP[0].F_ID = id;
	if(en == 0)
		TP[0].Check_ID = 0x00;
	else
		TP[0].Check_ID = 0xD5;
	TP[0].Rev = 0;
	uTP = (unsigned *) &TP[0];

	Addr = 0x20000;
	for(i = 0 ; i < 3 ; i++){
		IOP[i].Address = Addr + (i << 2);
		IOP[i].Data    = uTP[i];
	}

	SPI_Write_IO_S2(0x9, (int *) &IOP[0], 24);		// AS2_Write_F2_to_FX_CMD
}

void readIMCPdata(int M_Mode, int IMCP_Idx) {
	int i, j;
	int addr, size, idx = 0;
	char *buf_p;
	int Sum;

	idx = (IMCP_Idx - 1) & 0x3;
	Sum = (A_L_I3_Header[M_Mode].Sum << 5);
	//Z_V
	buf_p = &Smooth_Z_V_I[M_Mode][0][0];
	addr = IMCP_Z_V_LINE_P0_T_ADDR;
	size = sizeof(struct Smooth_Z_I_Struct) * Sum + 3072;							//sizeof(struct Smooth_Z_I_Struct) * A_L_I3_Header[M_Mode].Sum * 32 + 3072;
	for(i = 0; i < size; i += 3072) {
		ua360_spi_ddr_read(addr + i, (int *) (buf_p + i), 3072, 2, 0);
	}
	//Z_H
	buf_p = &Smooth_Z_H_I[M_Mode][0][0];
	addr = IMCP_Z_V_LINE_P0_T_ADDR + sizeof(struct Smooth_Z_I_Struct) * Sum;		//IMCP_Z_V_LINE_P0_T_ADDR + sizeof(struct Smooth_Z_I_Struct) * A_L_I3_Header[M_Mode].Sum * 32;
	size = sizeof(struct Smooth_Z_I_Struct) * Sum + 3072;							//sizeof(struct Smooth_Z_I_Struct) * A_L_I3_Header[M_Mode].Sum * 32 + 3072;
	for(i = 0; i < size; i += 3072) {
		ua360_spi_ddr_read(addr + i, (int *) (buf_p + i), 3072, 2, 0);
	}
}

int readSensorState() {
    int state = 0, addr = 0;
    int d_cnt = read_F_Com_In_Capture_D_Cnt();  // !=0: 拍照中
    if(d_cnt != 0) return 0;                    // 拍照中不檢查

    addr = 0xF0C;
    spi_read_io_porcess_S2(addr, &state, 4);
    if(state == 1) 
		db_error("readSensorState: state=%d d_cnt=%d\n", state, d_cnt);
	return state;
}

void SetFXSpeed(int f_id) {
	int Addr, s_addr, t_addr, size, id, mode;
	unsigned Data[64];
	memset(&Data[0], 0, sizeof(Data) );

	Data[0] = 0xCCA00924;
    if(f_id == 0) Data[1] = (F0_Speed | 0x10000);
    else		  Data[1] = (F1_Speed | 0x10000);

    if(f_id == 0) Addr = F2_F0_SPEED_ADDR;
    else		  Addr = F2_F1_SPEED_ADDR;
	ua360_spi_ddr_write(Addr, (int *) &Data[0], sizeof(Data) );

    s_addr = Addr;
    t_addr = 0;
    size = sizeof(Data);
    mode = 0;
    if(f_id == 0) id = AS2_MSPI_F0_ID;
    else		  id = AS2_MSPI_F1_ID;
    AS2_Write_F2_to_FX_CMD(s_addr, t_addr, size, mode, id, 1);

    db_debug("SetFXSpeed() f_id=%d Data[1]=0x%x size=%d Addr=0x%x\n", f_id, Data[1], size, Addr);
}

void SetF2Speed() {
	unsigned Data[2];
    Data[0] = 0x924;
    Data[1] = (F2_Speed | 0x10000);
    SPI_Write_IO_S2(0x9, (int *) &Data[0], 8);

    db_debug("SetF2Speed() Data[1]=0x%x\n", Data[1]);
}

void setFPGASpeed(int f_id, int speed) {
	switch(f_id) {
	case 0: F0_Speed = speed; SetFXSpeed(f_id); break;
	case 1: F1_Speed = speed; SetFXSpeed(f_id); break;
	case 2: F2_Speed = speed; SetF2Speed();     break;
	}
}

int getFPGASpeed(int f_id) {
	int speed;
	switch(f_id) {
	case 0: speed = F0_Speed; break;
	case 1: speed = F1_Speed; break;
	case 2: speed = F2_Speed; break;
	}
	return speed;
}

void ReadFPGASpeed() {
	int addr;
	unsigned F0_Speed, F1_Speed, F2_Speed;

	addr = 0x600D0;
	spi_read_io_porcess_S2(addr, (int *) &F0_Speed, 4);

	addr = 0x600D4;
	spi_read_io_porcess_S2(addr, (int *) &F1_Speed, 4);

	addr = 0x920;
	spi_read_io_porcess_S2(addr, (int *) &F2_Speed, 4);

	db_debug("ReadFPGASpeed() F0=0x%x F1=0x%x F2=0x%x\n", F0_Speed, F1_Speed, F2_Speed);
}

void SetSaturationUV() {
	int IO_Addr, DDR_Addr, i;
	int U_Tmp, V_Tmp;
	int U_Tmp2, V_Tmp2;
	unsigned short U_Table[256];		//x1:0x4000  MAX:0xFFFF
	unsigned short V_Table[256];		//x1:0x4000  MAX:0xFFFF
	unsigned *pv, *pu, *data;
	AS2_CMD_IO_struct IOP[256];

	for(i = 0; i < 256; i++) {
		U_Tmp = (Saturation_C * (1.0 + Saturation_Ku * ( (float)i-128.0)/128.0) ) * 16384.0;
		if(i >= Saturation_Th) U_Tmp2 = U_Tmp * (255 - i) / (255 - Saturation_Th);
		else				   U_Tmp2 = U_Tmp;
		if(U_Tmp2 < 0)      U_Tmp2 = 0;
		if(U_Tmp2 > 0xFFFF) U_Tmp2 = 0xFFFF;
		U_Table[i] = U_Tmp2;

		V_Tmp = (Saturation_C * (1.0 + Saturation_Kv * ( (float)i-128.0)/128.0) ) * 16384.0;
		if(i >= Saturation_Th) V_Tmp2 = V_Tmp * (255 - i) / (255 - Saturation_Th);
		else				   V_Tmp2 = V_Tmp;
		if(V_Tmp2 < 0)      V_Tmp2 = 0;
		if(V_Tmp2 > 0xFFFF) V_Tmp2 = 0xFFFF;
		V_Table[i] = V_Tmp2;
	}

	IO_Addr = 0xCCADD000;
	DDR_Addr = F2_SATURATION_UV_ADDR;
	pu = (unsigned *) U_Table;
	pv = (unsigned *) V_Table;
	for(i = 0 ; i < sizeof(IOP) / 8 ; i ++){
		 IOP[i].Address = IO_Addr + (i << 2);
		 if(i < sizeof(IOP) / 16) IOP[i].Data = *pu++;
		 else                     IOP[i].Data = *pv++;
	}
	data = (unsigned *) IOP;
	ua360_spi_ddr_write(DDR_Addr, (int *) data, sizeof(IOP) );


	int s_addr, t_addr;
	int size, mode, id;
	for(i = 0; i < 2; i++) {	//f_id
		s_addr = DDR_Addr;
		t_addr = 0;
		size = sizeof(IOP);
		mode = 0;
		if(i == 0) id = AS2_MSPI_F0_ID;
		else	   id = AS2_MSPI_F1_ID;
		AS2_Write_F2_to_FX_CMD(s_addr, t_addr, size, mode, id, 1);
	}

	db_debug("SetSaturationUV() C=%f Ku=%f Kv=%f (U[0]=%d U[128]=%d U[255]=%d, V[0]=%d V[128]=%d V[255]=%d)  sizeof(IOP)=%d\n",
			Saturation_C, Saturation_Ku, Saturation_Kv, U_Table[0], U_Table[128], U_Table[255],
			V_Table[0], V_Table[128], V_Table[255], sizeof(IOP) );
}

void SetSaturationValue(int idx, int value) {
	int c_idx;
	float add;
	switch(idx) {
	case 0:
		Saturation_C  = (float)value / 1000.0;
		if(Saturation_C >= 1.0) add = 0.5;
		else				    add = 0.0;
		c_idx = Saturation_C * 7.0 - 7.0 + add;
		set_A2K_JPEG_Saturation_C(c_idx);
		break;
	case 1: Saturation_Ku = (float)value / 1000.0; break;
	case 2: Saturation_Kv = (float)value / 1000.0; break;
	case 3: Saturation_Th = value; 				   break;
	}
	SetSaturationUV();
}

int GetSaturationParameter(int idx) {
	int value;
	switch(idx) {
	case 0: value = (int)(Saturation_C  * 1000); break;
	case 1: value = (int)(Saturation_Ku * 1000); break;
	case 2: value = (int)(Saturation_Kv * 1000); break;
	case 3: value = Saturation_Th; 			     break;
	}
	return value;
}

void setSensorPowerOff() {
	unsigned Data[2];
    Data[0] = 0xBF0;
    Data[1] = 0;
    SPI_Write_IO_S2(0x9, (int *) &Data[0], 8);
    db_debug("SetSensorPowerOff() 00\n");
}

void Read_HDR_Img(int cap_cnt) {
	int i, j, idx;
	int size, fx_x, f_id, addr, I_Sum, M_Sum, cnt, x;
	//int sx = 432, sy = 312;
	int sx = Read_FX_HDR_X, sy = Read_FX_HDR_Y;
	int img_s_addr, mo_s_addr, f_cnt = 5;
	unsigned char buf[2048];

	memset(&buf[0], 0, sizeof(buf) );
	memset(&HDR_Img_Buf[0][0], 0, sizeof(HDR_Img_Buf) );

	size = 2048;	//sizeof(buf);
	f_id = 1;
	fx_x = 1;		//1024 * 2
	for(idx = 0; idx < f_cnt; idx++) {
		//Read Data
		switch(idx) {
		case 0: img_s_addr = FX_WDR_IMG_P0_ADDR + sy * 0x8000 + 32;
				mo_s_addr = FX_WDR_MO_P0_ADDR + sy * 0x8000;
				break;
		case 1: img_s_addr = FX_WDR_IMG_P1_ADDR + sy * 0x8000 + 32;
				mo_s_addr = FX_WDR_MO_P1_ADDR + sy * 0x8000;
				break;
		case 2: img_s_addr = FX_WDR_IMG_P2_ADDR + sy * 0x8000 + 32;
				mo_s_addr = FX_WDR_MO_P2_ADDR + sy * 0x8000;
				break;
		case 3: img_s_addr = FX_WDR_IMG_P3_ADDR + sy * 0x8000 + 32;
				mo_s_addr = FX_WDR_MO_P3_ADDR + sy * 0x8000;
				break;
		case 4: img_s_addr = FX_WDR_IMG_P4_ADDR + sy * 0x8000 + 32;
				mo_s_addr = FX_WDR_MO_P4_ADDR + sy * 0x8000;
				break;
		case 5: img_s_addr = FX_WDR_IMG_P5_ADDR + sy * 0x8000 + 32;
				mo_s_addr = FX_WDR_MO_P5_ADDR + sy * 0x8000;
				break;
		case 6: img_s_addr = FX_WDR_IMG_P6_ADDR + sy * 0x8000 + 32;
				mo_s_addr = FX_WDR_MO_P6_ADDR + sy * 0x8000;
				break;
		}
		for(i = 0; i < 1; i++) {
			addr = img_s_addr + i * 2 * 0x8000;
			ua360_spi_ddr_read(addr, (int *) &buf[0], size, f_id, fx_x);
			memcpy(&HDR_Img_Buf[idx][i*2][0], &buf[0], HDR_IMG_SIZE_X);
			memcpy(&HDR_Img_Buf[idx][i*2+1][0], &buf[1024], HDR_IMG_SIZE_X);

			addr = mo_s_addr + i * 2 * 0x8000;
			ua360_spi_ddr_read(addr, (int *) &buf[0], size, f_id, fx_x);
			memcpy(&HDR_Mo_Buf[idx][i*2][0], &buf[0], HDR_IMG_SIZE_X);
			memcpy(&HDR_Mo_Buf[idx][i*2+1][0], &buf[1024], HDR_IMG_SIZE_X);
		}

		//8x8 Avg
		/*I_Sum = 0; M_Sum = 0; cnt = 0;
		HDR_Img_Avg[idx] = 0, HDR_Mo_Avg[idx] = 0;
		for(i = 0; i < 8; i++) {
			for(j = 0; j < 8; j++) {
				x = sx+j;
				if(x < HDR_IMG_SIZE_X) {
					I_Sum += HDR_Img_Buf[idx][i][x];
					M_Sum += HDR_Mo_Buf[idx][i][x];
					cnt++;
					//db_debug("Read_HDR_Img() idx=%d i=%d j=%d I_Buf=0x%02x M_Buf=0x%02x\n", idx, i, j, HDR_Img_Buf[idx][i][x], HDR_Mo_Buf[idx][i][x]);
				}
			}
		}
		HDR_Img_Avg[idx] = I_Sum / cnt;
		HDR_Mo_Avg[idx] = M_Sum / cnt;
		db_debug("Read_HDR_Img() idx=%d I_Avg=%d M_Avg=%d\n", idx, HDR_Img_Avg[idx], HDR_Mo_Avg[idx]);*/
	}


	//Write File
	FILE *rfx_fp;
	char path[128], str[512];
	int sub;
	sprintf(path, "/mnt/sdcard/Read_FX_%d.txt\0", cap_cnt);	//tmp
	rfx_fp = fopen(path, "wt");
	if(rfx_fp != NULL) {
		for(idx = 0; idx < f_cnt; idx++) {
			for(i = 0; i < 2; i++) {
				for(j = 0; j < HDR_IMG_SIZE_X; j++) {
					sub = HDR_Img_Buf[idx][i][j] - HDR_Mo_Buf[idx][i][j];
					sprintf(str, "Read_HDR_Img() idx=%d i=%d j=%d I_Buf=%d M_Buf=%d Sub=%d\n\0", idx, i, j, HDR_Img_Buf[idx][i][j], HDR_Mo_Buf[idx][i][j], sub);
					fwrite(&str[0], strlen(str), 1, rfx_fp);
				}
			}
		}
		fclose(rfx_fp);
	}
}

void ReadFXDDR(unsigned f_id, unsigned addr) {
	int i, size, fx_x;
	unsigned char buf[2048];

	memset(&buf[0], 0, sizeof(buf) );
	size = 1024;	//sizeof(buf);
	if(size <= 512)       fx_x = 0;		// 512 * 4
	else if(size <= 1024) fx_x = 1;		//1024 * 2
	else				  fx_x = 3;		//2048 * 1
	ua360_spi_ddr_read(addr, (int *) &buf[0], size, f_id, fx_x);

	for(i = 0; i < 2048; i++)
		db_debug("ReadFXDDR() i=%d buf=0x%x\n", i, buf[i]);
}

/*
 * en: 0=Off, 1=On
 */
void SetFPGASleepEn(int en) {
    int c_mode = getCameraMode();
	unsigned Data[2];
	Data[0] = 0x708;
	if(en == 0) Data[1] = 2;
	else		Data[1] = 1;
	SPI_Write_IO_S2(0x9, (int *) &Data[0], 8);

	if(en == 0) {		//Power Saving Off
		usleep(1000000);
		MainCmdInit();
		FPGA_Pipe_Init(1);
		Set_FPGA_Pipe_Idx(0);
		AS2_CMD_Start2();
//tmp		if(c_mode == CAMERA_MODE_NIGHT || c_mode == CAMERA_MODE_NIGHT_HDR || c_mode == CAMERA_MODE_M_MODE)		//Night / NightHDR / M-Mode
//tmp			Set_Skip_Frame_Cnt(3);
//tmp		else
//tmp			Set_Skip_Frame_Cnt(6);
	}
	FPGA_Sleep_En = en;
}

int GetFPGASleepEn() {
	return FPGA_Sleep_En;
}

/*
 * 要求FPGA計算CheckSum
 */
void Make_DDR_CheckSum(unsigned f_id, unsigned size, unsigned addr) {
	unsigned cmd_addr, Data[64];
	unsigned s_addr, t_addr;
	unsigned cmd_size, mode, id;

	memset(&Data[0], 0, sizeof(Data) );
	Data[0] = 0xCCA00CD4;
	Data[1] = size;

	Data[2] = 0xCCA00CD5;
	Data[3] = (addr >> 5);

	if(f_id == 0) cmd_addr = F2_0_DDR_CHECKSUM_ADDR;
	else		  cmd_addr = F2_1_DDR_CHECKSUM_ADDR;
	ua360_spi_ddr_write(cmd_addr, (int *) &Data[0], sizeof(Data) );

	s_addr = cmd_addr;
	t_addr = 0;
	cmd_size = sizeof(Data);
	mode = 0;
	if(f_id == 0) id = AS2_MSPI_F0_ID;
	else		  id = AS2_MSPI_F1_ID;
	AS2_Write_F2_to_FX_CMD(s_addr, t_addr, cmd_size, mode, id, 1);
}

unsigned Get_DDR_CheckSum(unsigned f_id) {
	unsigned addr, value;
	unsigned state=0, checksum=0;

	if(f_id == 0) addr = F0_DDR_CHECKSUM_IO_ADDR;
	else		  addr = F1_DDR_CHECKSUM_IO_ADDR;
	spi_read_io_porcess_S2(addr, (int *) &value, sizeof(value) );

	checksum = (value & 0x7FFFFFFF);	//31 bit
	state = (value >> 31) & 0x1;		//狀態改變位元, 0101切換, 表示FPGA是否執行checksum計算
	db_debug("Get_DDR_CheckSum() f_id=%d value=0x%x state=%d checksum=0x%x(%d)\n", f_id, value, state, checksum, checksum);
	return checksum;
}

unsigned Cal_DDR_CheckSum(unsigned size, unsigned char *data) {
	unsigned i, checksum=0;
	for(i = 0; i < size; i++)
		checksum += (unsigned)data[i];
	return checksum;
}

int Check_DDR_CheckSum(unsigned f_id, unsigned size, unsigned addr, unsigned ck_cal) {
	int ck_new;
	Make_DDR_CheckSum(f_id, size, addr);		//要求FPGA計算CheckSum
	usleep(20000);
	ck_new = Get_DDR_CheckSum(f_id);			//讀取新的CheckSum, 與自行計算的CheckSum比較
	db_debug("Check_DDR_CheckSum() f_id=%d size=%d addr=0x%x ck_cal=%d ck_new=%d\n", f_id, size, addr, ck_cal, ck_new);
	if(ck_cal == ck_new)
		return 1;
	else
		return 0;
}

void Check_ST_Cmd_DDR_CheckSum() {
	unsigned f_id, addr, size, timeout;
	unsigned check=0;

	addr = FX_ST_CMD_ADDR;
	size = (Stitch_Block_Max * 32);
	for(f_id = 0; f_id < 2; f_id++) {
		check = 0;
		timeout = DDR_CHECKSUM_TIMEOUT;
		while(check == 0) {
			check = Check_DDR_CheckSum(f_id, size, addr, ST_Cmd_Cal_CheckSum[f_id]);
			//db_debug("Check_ST_Cmd_DDR_CheckSum() f_id=%d sum=%d check=%d\n", f_id, ST_Cmd_Cal_CheckSum[f_id], check);
			if(check == 0) {
				db_error("Check_ST_Cmd_DDR_CheckSum() Err! f_id=%d timeout=%d\n", f_id, timeout);
				send_Sitching_Cmd(f_id);
				timeout--;
				if(timeout <= 0) {
//tmp					paintOLEDDDRError(1);
					break;
				}
			}
			else
				db_debug("Check_ST_Cmd_DDR_CheckSum() OK!\n");
		}
	}
	return;
}

void Check_ST_Sensor_Cmd_DDR_CheckSum() {
	unsigned f_id, addr, size, timeout;
	unsigned check=0;

	addr = FX_ST_S_CMD_ADDR;
//tmp	size = (Stitch_Sesnor_Max * 32);
	for(f_id = 0; f_id < 2; f_id++) {
		check = 0;
		timeout = DDR_CHECKSUM_TIMEOUT;
		while(check == 0) {
			check = Check_DDR_CheckSum(f_id, size, addr, ST_Sen_Cmd_Cal_CheckSum[f_id]);
			//db_debug("Check_ST_Sensor_Cmd_DDR_CheckSum() f_id=%d sum=%d check=%d\n", f_id, ST_Sen_Cmd_Cal_CheckSum[f_id], check);
			if(check == 0) {
				db_error("Check_ST_Sensor_Cmd_DDR_CheckSum() Err! f_id=%d timeout=%d\n", f_id, timeout);
//tmp				send_Sitching_Sensor_Cmd(f_id);
				timeout--;
				if(timeout <= 0) {
//tmp					paintOLEDDDRError(1);
					break;
				}
			}
			else
				db_debug("Check_ST_Sensor_Cmd_DDR_CheckSum() OK!\n");
		}
	}
	return;
}

void Check_ST_Tran_Cmd_DDR_CheckSum() {
	unsigned f_id, addr, size, timeout;
	unsigned check=0;

	addr = FX_ST_TRAN_CMD_ADDR;
	size = (Stitch_Block_Max * 4);
	for(f_id = 0; f_id < 2; f_id++) {
		check = 0;
		timeout = DDR_CHECKSUM_TIMEOUT;
		while(check == 0) {
			check = Check_DDR_CheckSum(f_id, size, addr, ST_Tran_Cmd_Cal_CheckSum[f_id]);
			//db_debug("Check_ST_Tran_Cmd_DDR_CheckSum() f_id=%d sum=%d check=%d\n", f_id, ST_Tran_Cmd_Cal_CheckSum[f_id], check);
			if(check == 0) {
				db_error("Check_ST_Tran_Cmd_DDR_CheckSum() Err! f_id=%d timeout=%d\n", f_id, timeout);
				send_Sitching_Tran_Cmd(f_id);
				timeout--;
				if(timeout <= 0) {
//tmp					paintOLEDDDRError(1);
					break;
				}
			}
			else
				db_debug("Check_ST_Tran_Cmd_DDR_CheckSum() OK!\n");
		}
	}
	return;
}

void Check_MP_B_Table_DDR_CheckSum(int idx) {
	unsigned f_id, addr, size, timeout;
	unsigned check=0;

	if(idx == 0) addr = FX_MP_GAIN_ADDR;
	else		 addr = FX_MP_GAIN_2_ADDR;
	size = (Stitch_Block_Max * 64);
	for(f_id = 0; f_id < 2; f_id++) {
		check = 0;
		timeout = DDR_CHECKSUM_TIMEOUT;
		while(check == 0) {
			check = Check_DDR_CheckSum(f_id, size, addr, MP_B_Table_Cal_CheckSum[f_id][idx]);
			//db_debug("Check_MP_B_Table_DDR_CheckSum() f_id=%d sum=%d check=%d\n", f_id, MP_B_Table_Cal_CheckSum[f_id], check);
			if(check == 0) {
				db_error("Check_MP_B_Table_DDR_CheckSum() Err! f_id=%d timeout=%d\n", f_id, timeout);
				Send_MP_B_Table(f_id, idx);
				timeout--;
				if(timeout <= 0) {
//tmp					paintOLEDDDRError(1);
					break;
				}
			}
			else
				db_debug("Check_MP_B_Table_DDR_CheckSum() OK!\n");
		}
	}
	return;
}

void Check_Sen_Lens_Adj_DDR_CheckSum(int s_id) {
	unsigned scale, addr, size, timeout;
	unsigned check=0;
	unsigned FPGA_ID, MT;

	Get_FId_MT(s_id, &FPGA_ID, &MT);

	for(scale = 1; scale <= 3; scale++) {
		switch(scale) {
		case 1: addr = FX_LC_FS_BUF_Addr; break;
		case 2: addr = FX_LC_D2_BUF_Addr; break;
		case 3: addr = FX_LC_D3_BUF_Addr; break;
		}
		addr += (MT * 0x8000);

		size = (64 * 512);
		check = 0;
		timeout = DDR_CHECKSUM_TIMEOUT;
		while(check == 0) {
			check = Check_DDR_CheckSum(FPGA_ID, size, addr, Sen_Lens_Adj_Cal_CheckSum[scale-1][s_id]);
			//db_debug("Check_Sen_Lens_Adj_DDR_CheckSum() scale=%d s_id=%d f_id=%d sum=%d check=%d\n", scale, s_id, FPGA_ID, Sen_Lens_Adj_Cal_CheckSum[scale-1][s_id], check);
			if(check == 0) {
				db_error("Check_Sen_Lens_Adj_DDR_CheckSum() Err! f_id=%d timeout=%d\n", FPGA_ID, timeout);
				doSensorLensAdj(s_id, doSensorLensEn);
				timeout--;
				if(timeout <= 0) {
//tmp					paintOLEDDDRError(1);
					break;
				}
			}
			else
				db_debug("Check_Sen_Lens_Adj_DDR_CheckSum() OK!\n");
		}
	}

	return;
}

void CheckSTCmdDDRCheckSum() {
	int i;
	Check_ST_Cmd_DDR_CheckSum();
	Check_ST_Sensor_Cmd_DDR_CheckSum();
	Check_ST_Tran_Cmd_DDR_CheckSum();
	for(i = 0; i < 2; i++)
		Check_MP_B_Table_DDR_CheckSum(i);
	for(i = 0; i < 5; i++)
		Check_Sen_Lens_Adj_DDR_CheckSum(i);
}

unsigned Read_FX_Cnt(void) {
	unsigned cnt=0, addr;
    addr = F0_CNT_IO_ADDR;
    spi_read_io_porcess_S2(addr, &cnt, 4);
    return cnt;
}

int CheckFXCnt(void) {
	unsigned cnt=0;
	unsigned fps_t = 1000000000 / getFPS();		//1000000000 / (FPS / 10) / 10ns;
	unsigned fps_t_max = (fps_t << 1);
	static int err_cnt=0;
	int chBinn = Get_FPGA_Com_In_Sensor_Change_Binn();

//tmp	if(getImgReadyFlag() == 1 && get_rec_state() == -2 && Capture_Is_Finish() == 1 && chBinn == 0) {
	if(chBinn == 0) {
		cnt = Read_FX_Cnt();		// 1 = 10ns
		if(cnt > fps_t_max) {
			db_error("CheckFXCnt() FX: cnt err! cnt=%d fps_t=%d max=%d\n", cnt, fps_t, fps_t_max);
			err_cnt++;
		}
		else
			err_cnt = 0;
		if(err_cnt > 8) {		//8s
			db_error("CheckFXCnt() FX: err_cnt > 8!\n");
			return -1;
		}
	}
	return 0;
}