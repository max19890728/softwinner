/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/
 
#include "Device/US363/Kernel/k_test.h"

#include "Device/US363/Kernel/us360_define.h"
#include "Device/US363/Kernel/AletaS2_CMD_Struct.h"
#include "Device/US363/Kernel/k_spi_cmd.h"
#include "Device/US363/Kernel/variable.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::KTest"

static struct US360_Stitching_Table ST_Test[2][16384];			//測試程式 & debug程式使用
static int ST_Sum_Test[2];

/*
 * 	mode: 0:DDR_to_IO  1:DDR_to_DDR
 */
static void AS2_Write_F2_to_FX_CMD(unsigned S_Addr, unsigned T_Addr, unsigned size, unsigned mode, unsigned id, unsigned en)
{
	int i;
	unsigned Addr;
	AS2_SPI_Cmd_struct TP[2];
	AS2_CMD_IO_struct IOP[7];
	unsigned *uTP;

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
//	IOP[3].Address = Addr + 0xF00;
//	IOP[3].Data    = 0;
	K_SPI_Write_IO_S2( 0x9  , (unsigned *) &IOP[0] , 24);		//SPI_Write_IO_S2( 0x9  , (unsigned *) &IOP[0] , 8 * 4);
}

static int send_Sitching_Cmd_Test(void)
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
			k_ua360_spi_ddr_write(tmpaddr,  (int *)&ST_Test[j][i / 32], 2048);
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
int *get_ST_Sum_Test_p(void)
{
    return(&ST_Sum_Test[0]);
}
int AS2_F0_ST_Test2(int s_id, int f_id)
{
	int i,j,idx,*data,Addr;
	int T_DDR_P,offset_y,offset_x,offset;
	int base_x,base_y;
	int st_size=0, size=0;
	int size_x = DS_ST_XSIZE, size_y = DS_ST_YSIZE;
	struct US360_Stitching_Table *ST_P;

	T_DDR_P = ST_STM2_P0_T_ADDR >> 5;
	offset_y = 64 * 0x400;
	offset_x = 64 * 2 / 32;
	offset = 0x100;
	base_x = 512;
	base_y = 512;

	int Input_Scale = get_Sensor_Input_Scale();
    switch(Input_Scale) {
	//case 1: size_x = 70; size_y = 53; break;
	case 1: size_x = 24; size_y = 18; break;
	case 2: size_x = 36; size_y = 27; break;
	case 3: size_x = 24; size_y = 18; break;
	}

	ST_Sum_Test[f_id] = 0;
    for(i = 0 ; i < size_y ; i ++){
		for(j = 0 ; j < size_x ; j ++){
			idx = i * size_x + j;
			//ST_P = &AS2_F0_ST_CMD[idx];
			ST_P  = &ST_Test[f_id][ ST_Sum_Test[f_id] ];
			ST_P->CB_DDR_P = T_DDR_P + i * offset_y + j * offset_x;
			ST_P->CB_Sensor_ID = 4;
			ST_P->CB_Alpha_Flag = 0;
			ST_P->CB_Block_ID = 1;
			ST_P->CB_Mask = 0;
			ST_P->CB_Posi_X0 = base_x +       j * offset + (1536 * s_id * 4);
			ST_P->CB_Posi_Y0 = base_y +       i * offset;
			ST_P->CB_Posi_X1 = base_x + (j + 1) * offset + (1536 * s_id * 4);
			ST_P->CB_Posi_Y1 = base_y +       i * offset;
			ST_P->CB_Posi_X2 = base_x +       j * offset + (1536 * s_id * 4);
			ST_P->CB_Posi_Y2 = base_y + (i + 1) * offset;
			ST_P->CB_Posi_X3 = base_x + (j + 1) * offset + (1536 * s_id * 4);
			ST_P->CB_Posi_Y3 = base_y + (i + 1) * offset;
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

			ST_Sum_Test[f_id]++;
		}
	}

	return 0;
}

int changeST(int f_id, int MT)
{
    AS2_F0_ST_Test2(MT, f_id);
	send_Sitching_Cmd_Test();
	return 0;
}

int AS2_FX_ST_Test_Cmd(int f_id)
{
	int i,j,idx,*data,Addr;
	int T_DDR_P,offset_y,offset_x,offset;
	int base_x,base_y;
	int st_size=0, size;
	int size_x = DS_ST_XSIZE, size_y = DS_ST_YSIZE;
	struct US360_Stitching_Table *ST_P;

	ST_Sum_Test[0] = 0;
	ST_Sum_Test[1] = 0;

	T_DDR_P = ST_STM2_P0_T_ADDR >> 5;
	offset_y = 64 * 0x400;
	offset_x = 64 * 2 / 32;
	offset = 0x100;
	base_x = 0;
	base_y = 0;

	size_x = (1536 / 64);
	size_y = (1152 / 64);

	for(i = 0 ; i < size_y ; i ++){
		for(j = 0 ; j < size_x ; j ++){
			idx = i * size_x + j;
			//ST_P = &AS2_F1_ST_CMD[idx];
			ST_P  = &ST_Test[f_id][ ST_Sum_Test[f_id] ];
			ST_P->CB_DDR_P = T_DDR_P + i * offset_y + j * offset_x;
			ST_P->CB_Sensor_ID = 4;
			ST_P->CB_Alpha_Flag = 0;
			ST_P->CB_Block_ID = 1;
			ST_P->CB_Mask = 0;
			ST_P->CB_Posi_X0 = base_x +       j * offset;
			ST_P->CB_Posi_Y0 = base_y +       i * offset;
			ST_P->CB_Posi_X1 = base_x + (j + 1) * offset;
			ST_P->CB_Posi_Y1 = base_y +       i * offset;
			ST_P->CB_Posi_X2 = base_x +       j * offset;
			ST_P->CB_Posi_Y2 = base_y + (i + 1) * offset;
			ST_P->CB_Posi_X3 = base_x + (j + 1) * offset;
			ST_P->CB_Posi_Y3 = base_y + (i + 1) * offset;
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

			ST_Sum_Test[f_id]++;
		}
	}

	return 0;
}
int do_FX_ST_Test(int f_id)
{
	AS2_FX_ST_Test_Cmd(f_id);
	send_Sitching_Cmd_Test();
	return 0;
}

void Add_ST_Table_Proc_Test_S2_Focus(int M_Mode, struct Adjust_Line_I3_Header_Struct *TP, int s_id)
{
	int tmpaddr = 0;
	int offsetX = 0, offsetY = 0;
	int i = 0, j = 0, k = 0;
	int MT = 0, MT2 = 0;
	int page = 0;
	int sx, sy, tx, ty, sx2, sy2;
    int FPGA_ID;
    int T_DDR_P;
    struct US360_Stitching_Table *ST_P;

	ST_Sum_Test[0] = 0;
	ST_Sum_Test[1] = 0;

    T_DDR_P = ST_STM1_P0_T_ADDR >> 5;

    offsetX = 512;
    offsetY = 512;

	Get_FId_MT(s_id, &FPGA_ID, &MT);
    for(i = 0; i < 9; i++) {
    	for (j = 0; j < 5; j++) {
    		for(k = 0; k < 10; k++) {
    			//4364(68) * 3282(51)	FS:實際影像範圍
    			switch(i) {
    			case 0: sx = 0;  sy = 0;  tx = 0;  ty = 0;  break;
    			case 1: sx = 29; sy = 0;  tx = 10; ty = 0;  break;
    			case 2: sx = 58; sy = 0;  tx = 20; ty = 0;  break;

    			case 3: sx = 0;  sy = 22; tx = 0;  ty = 5;  break;
    			case 4: sx = 29; sy = 22; tx = 10; ty = 5;  break;
    			case 5: sx = 58; sy = 22; tx = 20; ty = 5;  break;

    			case 6: sx = 0;  sy = 46; tx = 0;  ty = 10; break;
    			case 7: sx = 29; sy = 46; tx = 10; ty = 10; break;
    			case 8: sx = 58; sy = 46; tx = 20; ty = 10; break;
    			}

    			ST_P  = &ST_Test[FPGA_ID][ ST_Sum_Test[FPGA_ID] ];

    			int  V_Sensor_X_Step =  4608;	//Sensor_X_Step_debug;	//Sensor_X_Step/2;

    			ST_P->CB_Block_ID = 1;	//(ST_Idx + ST_Sum_Test[FPGA_ID]) & 7;
    			ST_P->CB_DDR_P   = ( ( ( ( (ty+j) * 64 * 16384) + ( (tx+k) * 64) ) * 2) / 32) + T_DDR_P;
    			ST_P->CB_Sensor_ID = 4;
    			ST_P->CB_Alpha_Flag = 0;	//TP->Source_Scale;
    			ST_P->CB_Mask   = 0;

    			ST_P->CB_Posi_X0 = ( (sx+k)   * 64 +MT * V_Sensor_X_Step/TP->Binn ) * 4 + offsetX;
    			ST_P->CB_Posi_Y0 = ( (sy+j)   * 64                                ) * 4 + offsetY;
    			ST_P->CB_Posi_X1 = ( (sx+k+1) * 64 +MT * V_Sensor_X_Step/TP->Binn ) * 4 + offsetX;
    			ST_P->CB_Posi_Y1 = ( (sy+j)   * 64                                ) * 4 + offsetY;
    			ST_P->CB_Posi_X2 = ( (sx+k)   * 64 +MT * V_Sensor_X_Step/TP->Binn ) * 4 + offsetX;
    			ST_P->CB_Posi_Y2 = ( (sy+j+1) * 64                                ) * 4 + offsetY;
    			ST_P->CB_Posi_X3 = ( (sx+k+1) * 64 +MT * V_Sensor_X_Step/TP->Binn ) * 4 + offsetX;
    			ST_P->CB_Posi_Y3 = ( (sy+j+1) * 64                                ) * 4 + offsetY;

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

    			ST_Sum_Test[FPGA_ID]++;
    		}
    	}
    }

}
int do_Focus_ST_Test(int s_id)
{
	int M_Mode = 0;
	Add_ST_Table_Proc_Test_S2_Focus(M_Mode, &A_L_I3_Header[M_Mode], s_id);
	send_Sitching_Cmd_Test();
	return 0;
}

struct S2_Focus_XY_Struct Foucs_XY[4][5][4] = {				//Foucs_XY[tool_id][sensor][idx]
	  { { { 460, 2145}, {3015, 2122}, { 472,  722}, {3005,  690} },
		{ {2435, -155}, {2435, 2385}, { 280,  230}, { 225, 2033} },
		{ { 915, 2385}, { 842,  -40}, {3103, 2085}, {3092,  302} },
		{ {2533, -103}, {2545, 2480}, { 283,  250}, { 273, 2080} },
		{ { 830, 2440}, { 820,  -80}, {3095, 2050}, {3053,  258} } },

	  { { { 459, 2158}, {3041, 2200}, { 462,  704}, {3018,  688} },
		{ {2409, -156}, {2421, 2418}, { 251,  236}, { 203, 2027} },
		{ { 936, 2480}, { 939,  -87}, {3140, 2095}, {3122,  312} },
		{ {2521, -165}, {2511, 2435}, { 238,  295}, { 224, 1990} },
		{ { 835, 2467}, { 865, -109}, {3133, 1962}, {3090,  300} } },

	  { { { 460, 2145}, {3015, 2122}, { 472,  722}, {3005,  690} },
		{ {2435, -155}, {2435, 2385}, { 280,  230}, { 225, 2033} },
		{ { 915, 2385}, { 842,  -40}, {3103, 2085}, {3092,  302} },
		{ {2533, -103}, {2545, 2480}, { 283,  250}, { 273, 2080} },
		{ { 830, 2440}, { 820,  -80}, {3095, 2050}, {3053,  258} } },

	  { { { 460, 2145}, {3015, 2122}, { 472,  722}, {3005,  690} },
		{ {2435, -155}, {2435, 2385}, { 280,  230}, { 225, 2033} },
		{ { 915, 2385}, { 842,  -40}, {3103, 2085}, {3092,  302} },
		{ {2533, -103}, {2545, 2480}, { 283,  250}, { 273, 2080} },
		{ { 830, 2440}, { 820,  -80}, {3095, 2050}, {3053,  258} } }
};
void Set_Focus_XY(int tool_id, int s_id, int idx, int X, int Y)
{
	Foucs_XY[tool_id-1][s_id][idx].X = X;
	Foucs_XY[tool_id-1][s_id][idx].Y = Y;
	db_debug("Set_Focus_XY() tool_id=%d s_id=%d idx=%d X=%d Y=%d\n",
			tool_id, s_id, idx, Foucs_XY[tool_id-1][s_id][idx].X, Foucs_XY[tool_id-1][s_id][idx].Y);
}
void Get_Focus_XY(int tool_id, int s_id, int idx, int *X, int *Y)
{
	*X = Foucs_XY[tool_id-1][s_id][idx].X;
	*Y = Foucs_XY[tool_id-1][s_id][idx].Y;
}

struct S2_Focus_XY_Struct Foucs_XY_Offset[5] = {0};			//Foucs_XY_Offset[sensor]
void Set_Focus_XY_Offset(int s_id, int xy, int offset)
{
	if(xy == 0) Foucs_XY_Offset[s_id].X = offset;
	else		Foucs_XY_Offset[s_id].Y = offset;
}
void Get_Focus_XY_Offset(int s_id, int *offsetX, int *offsetY)
{
	*offsetX = Foucs_XY_Offset[s_id].X;
	*offsetY = Foucs_XY_Offset[s_id].Y;
}

int Focus_Degree[5] = {0, -90, 90, -90, 90};
void Get_Focus_Degree(int s_id, int *degree)
{
	*degree = Focus_Degree[s_id];
}

void Add_ST_Table_Proc_Test_S2_Focus2(int M_Mode, struct Adjust_Line_I3_Header_Struct *TP, int tool_id, int s_id)
{
	int tmpaddr = 0;
	int offsetX = 0, offsetY = 0;
	int i = 0, j = 0, k = 0;
	int MT = 0, MT2 = 0;
	int page = 0;
	int sx, sy, tx, ty, sx2, sy2;
    int FPGA_ID;
    int T_DDR_P;
    struct US360_Stitching_Table *ST_P;

	ST_Sum_Test[0] = 0;
	ST_Sum_Test[1] = 0;

    T_DDR_P = ST_STM1_P0_T_ADDR >> 5;

    offsetX = 512;
    offsetY = 512;

    // 1920 * 1024 -> 分4格
	Get_FId_MT(s_id, &FPGA_ID, &MT);
    for(i = 0; i < 4; i++) {
    	for (j = 0; j < 8; j++) {		//1024 / 2 / 64
    		for(k = 0; k < 15; k++) {	//1920 / 2 / 64
    			//4364(68) * 3282(51)	FS:實際影像範圍
    			sx = Foucs_XY[tool_id-1][s_id][i].X + Foucs_XY_Offset[s_id].X;
    			sy = Foucs_XY[tool_id-1][s_id][i].Y + Foucs_XY_Offset[s_id].Y;
    			tx = (i % 2) * 15;
    			ty = (i / 2) * 8;

    			ST_P  = &ST_Test[FPGA_ID][ ST_Sum_Test[FPGA_ID] ];

    			int V_Sensor_X_Step = 4608;

    			ST_P->CB_Block_ID   = 1;
    			ST_P->CB_DDR_P   = ( ( (ty+j) * 64 * 32768 + (tx+k) * 64 * 2) / 32) + T_DDR_P;
    			ST_P->CB_Sensor_ID = 4;
    			ST_P->CB_Alpha_Flag = 0;
    			ST_P->CB_Mask   = 0;

    			//將Sensor影像擺正
    			if(Focus_Degree[s_id] == 0) {
        			ST_P->CB_Posi_X0 = ( sx+k     		* 64 +MT * V_Sensor_X_Step/TP->Binn ) * 4 + offsetX;
        			ST_P->CB_Posi_Y0 = ( sy+j     		* 64                                ) * 4 + offsetY;
        			ST_P->CB_Posi_X1 = ( sx+(k+1) 		* 64 +MT * V_Sensor_X_Step/TP->Binn ) * 4 + offsetX;
        			ST_P->CB_Posi_Y1 = ( sy+j     		* 64                                ) * 4 + offsetY;
        			ST_P->CB_Posi_X2 = ( sx+k     		* 64 +MT * V_Sensor_X_Step/TP->Binn ) * 4 + offsetX;
        			ST_P->CB_Posi_Y2 = ( sy+(j+1) 		* 64                                ) * 4 + offsetY;
        			ST_P->CB_Posi_X3 = ( sx+(k+1) 		* 64 +MT * V_Sensor_X_Step/TP->Binn ) * 4 + offsetX;
        			ST_P->CB_Posi_Y3 = ( sy+(j+1) 		* 64                                ) * 4 + offsetY;
    			}
    			else if(Focus_Degree[s_id] == -90) {
        			ST_P->CB_Posi_X0 = ( sx+(7-j+1)    	* 64 +MT * V_Sensor_X_Step/TP->Binn ) * 4 + offsetX;
        			ST_P->CB_Posi_Y0 = ( sy+k     		* 64                                ) * 4 + offsetY;
        			ST_P->CB_Posi_X1 = ( sx+(7-j+1) 	* 64 +MT * V_Sensor_X_Step/TP->Binn ) * 4 + offsetX;
        			ST_P->CB_Posi_Y1 = ( sy+(k+1)     	* 64                                ) * 4 + offsetY;
        			ST_P->CB_Posi_X2 = ( sx+(7-j)     	* 64 +MT * V_Sensor_X_Step/TP->Binn ) * 4 + offsetX;
        			ST_P->CB_Posi_Y2 = ( sy+k 			* 64                                ) * 4 + offsetY;
        			ST_P->CB_Posi_X3 = ( sx+(7-j) 		* 64 +MT * V_Sensor_X_Step/TP->Binn ) * 4 + offsetX;
        			ST_P->CB_Posi_Y3 = ( sy+(k+1) 		* 64                                ) * 4 + offsetY;
    			}
    			else if(Focus_Degree[s_id] == 90) {
        			ST_P->CB_Posi_X0 = ( sx+j     		* 64 +MT * V_Sensor_X_Step/TP->Binn ) * 4 + offsetX;
        			ST_P->CB_Posi_Y0 = ( sy+(14-k+1)	* 64                                ) * 4 + offsetY;
        			ST_P->CB_Posi_X1 = ( sx+j 			* 64 +MT * V_Sensor_X_Step/TP->Binn ) * 4 + offsetX;
        			ST_P->CB_Posi_Y1 = ( sy+(14-k)     	* 64                                ) * 4 + offsetY;
        			ST_P->CB_Posi_X2 = ( sx+(j+1)     	* 64 +MT * V_Sensor_X_Step/TP->Binn ) * 4 + offsetX;
        			ST_P->CB_Posi_Y2 = ( sy+(14-k+1) 	* 64                                ) * 4 + offsetY;
        			ST_P->CB_Posi_X3 = ( sx+(j+1) 		* 64 +MT * V_Sensor_X_Step/TP->Binn ) * 4 + offsetX;
        			ST_P->CB_Posi_Y3 = ( sy+(14-k) 		* 64                                ) * 4 + offsetY;
    			}


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

    			ST_Sum_Test[FPGA_ID]++;
    		}
    	}
    }
}
int do_Focus_ST_Test2(int tool_id, int s_id)
{
	int M_Mode = 0;
	Add_ST_Table_Proc_Test_S2_Focus2(M_Mode, &A_L_I3_Header[M_Mode], tool_id, s_id);
	send_Sitching_Cmd_Test();
	return 0;
}

void Add_ST_Table_Proc_Test_S2(int M_Mode, struct Adjust_Line_I3_Header_Struct *TP)
{
	int tmpaddr = 0;
	int offsetX = 0, offsetY = 0;
	int i = 0, j = 0, k = 0;
	int MT = 0, MT2 = 0;
	int page = 0;
	int Background_P;
	int sx, sy, tx, ty, sx2, sy2, sy3;
    int FPGA_ID;
    int T_DDR_P;
    struct US360_Stitching_Table *ST_P;

	ST_Sum_Test[0] = 0;
	ST_Sum_Test[1] = 0;

    T_DDR_P = ST_STM2_P0_T_ADDR >> 5;

    offsetX = 512;
    offsetY = 512;

    int E_cnt =0;
    for(i = 1; i < 5; i++) {
		for (j = 0; j <  4; j++) {
			for(k = 0; k < 24; k++) {
				Get_FId_MT(i, &FPGA_ID, &MT);

				switch(i) {
				  case 0: break;
				  case 1:
					  if(j < 2) { sx = 0; sy = 0; 		tx = 4;	ty = 0; }
					  else 		{ sx = 0; sy = 15;		tx = 4;	ty = 14; }
					  break;
				  case 2:
					  if(j < 2) { sx = 0; sy = 0;		tx = 2;	ty = 2; }
					  else      { sx = 0; sy = 15;		tx = 2;	ty = 4; }
					  break;
				  case 3:
					  if(j < 2) { sx = 0; sy = 15;		tx = 4;	ty = 6; }
					  else      { sx = 0; sy = 0;		tx = 4;	ty = 8; }
					  break;
				  case 4:
					  if(j < 2) { sx = 0; sy = 0;		tx = 2;	ty = 10; }
					  else      { sx = 0; sy = 15;		tx = 2;	ty = 12; }
					  break;
				}

				if(FPGA_ID == 0){
					sx2 = sx + (23-k);
					sy2 = sy+(j&1);
				}
				else{
					sx2 = sx+k;
					sy2 = sy+(((j&1)+1)%2);
				}

				sy3 = sy2 * 64 + 12;


				ST_P = &ST_Test[FPGA_ID][ST_Sum_Test[FPGA_ID] ];

				int  V_Sensor_X_Step =  4608;	//Sensor_X_Step_debug;	//Sensor_X_Step/2;

				ST_P->CB_Block_ID = 1;	//(ST_Idx + ST_Sum_Test[FPGA_ID]) & 7;
				ST_P->CB_DDR_P   = ( ( ( ( (ty+(j&1) ) << 20) + ( (tx+k) << 6) ) << 1) >> 5) + T_DDR_P;
				ST_P->CB_Sensor_ID = 4;
				ST_P->CB_Alpha_Flag = 0;	//TP->Source_Scale;
				ST_P->CB_Mask   = 0;
				if(FPGA_ID == 0){
					if(sy == 0){
						ST_P->CB_Posi_X0 = ( (sx2+1)*64 +MT * V_Sensor_X_Step/TP->Binn )*4 + offsetX;
						ST_P->CB_Posi_Y0 = ( sy2*64                                    )*4 + offsetY;
						ST_P->CB_Posi_X1 = ( sx2*64     +MT * V_Sensor_X_Step/TP->Binn )*4 + offsetX;
						ST_P->CB_Posi_Y1 = ( sy2*64                                    )*4 + offsetY;
						ST_P->CB_Posi_X2 = ( (sx2+1)*64 +MT * V_Sensor_X_Step/TP->Binn )*4 + offsetX;
						ST_P->CB_Posi_Y2 = ( (sy2+1)*64                                )*4 + offsetY;
						ST_P->CB_Posi_X3 = ( sx2*64     +MT * V_Sensor_X_Step/TP->Binn )*4 + offsetX;
						ST_P->CB_Posi_Y3 = ( (sy2+1)*64                                )*4 + offsetY;
					}
					else{
						ST_P->CB_Posi_X0 = ( (sx2+1)*64 +MT * V_Sensor_X_Step/TP->Binn )*4 + offsetX;
						ST_P->CB_Posi_Y0 = ( sy3                                       )*4 + offsetY;
						ST_P->CB_Posi_X1 = ( sx2*64     +MT * V_Sensor_X_Step/TP->Binn )*4 + offsetX;
						ST_P->CB_Posi_Y1 = ( sy3                                       )*4 + offsetY;
						ST_P->CB_Posi_X2 = ( (sx2+1)*64 +MT * V_Sensor_X_Step/TP->Binn )*4 + offsetX;
						ST_P->CB_Posi_Y2 = ( (sy3+64)                                  )*4 + offsetY;
						ST_P->CB_Posi_X3 = ( sx2*64     +MT * V_Sensor_X_Step/TP->Binn )*4 + offsetX;
						ST_P->CB_Posi_Y3 = ( (sy3+64)                                  )*4 + offsetY;
					}
				}
				else{
					if(sy == 0){
						ST_P->CB_Posi_X0 = ( sx2*64     +MT * V_Sensor_X_Step/TP->Binn )*4 + offsetX;
						ST_P->CB_Posi_Y0 = ( (sy2+1)*64                                )*4 + offsetY;
						ST_P->CB_Posi_X1 = ( (sx2+1)*64 +MT * V_Sensor_X_Step/TP->Binn )*4 + offsetX;
						ST_P->CB_Posi_Y1 = ( (sy2+1)*64                                )*4 + offsetY;
						ST_P->CB_Posi_X2 = ( sx2*64     +MT * V_Sensor_X_Step/TP->Binn )*4 + offsetX;
						ST_P->CB_Posi_Y2 = ( sy2*64                                    )*4 + offsetY;
						ST_P->CB_Posi_X3 = ( (sx2+1)*64 +MT * V_Sensor_X_Step/TP->Binn )*4 + offsetX;
						ST_P->CB_Posi_Y3 = ( sy2*64                                    )*4 + offsetY;
					}
					else{
						ST_P->CB_Posi_X0 = ( sx2*64     +MT * V_Sensor_X_Step/TP->Binn )*4 + offsetX;
						ST_P->CB_Posi_Y0 = ( (sy3+64)                                  )*4 + offsetY;
						ST_P->CB_Posi_X1 = ( (sx2+1)*64 +MT * V_Sensor_X_Step/TP->Binn )*4 + offsetX;
						ST_P->CB_Posi_Y1 = ( (sy3+64)                                  )*4 + offsetY;
						ST_P->CB_Posi_X2 = ( sx2*64     +MT * V_Sensor_X_Step/TP->Binn )*4 + offsetX;
						ST_P->CB_Posi_Y2 = ( sy3                                       )*4 + offsetY;
						ST_P->CB_Posi_X3 = ( (sx2+1)*64 +MT * V_Sensor_X_Step/TP->Binn )*4 + offsetX;
						ST_P->CB_Posi_Y3 = ( sy3                                       )*4 + offsetY;
					}
				}

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

				ST_Sum_Test[FPGA_ID]++;
			}
		}
    }
}
int do_ST_Test_S2(int s_id)
{
	int M_Mode = 3;
	Add_ST_Table_Proc_Test_S2(M_Mode, &A_L_I3_Header[M_Mode]);
	send_Sitching_Cmd_Test();
	return 0;
}
