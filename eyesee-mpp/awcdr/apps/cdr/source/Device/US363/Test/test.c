/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>          /* for videodev2.h */
//#include <linux/videodev2.h>
//#include <linux/usbdevice_fs.h>

#include "Device/US363/Test/test.h"
#include "Device/US363/Cmd/spi_cmd.h"
#include "Device/US363/Cmd/us363_spi.h"
#include "Device/US363/Cmd/us360_func.h"
#include "Device/US363/Kernel/k_spi_cmd.h"
//#include "us360.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::Test"

FILE *block_fp[3];
int Block_File_idx = 0;
struct Test_Tool_Cmd_Struct_H TestToolCmd;
int testtool_set_AEG = -1000, testtool_set_gain = -1000;

void ReadTestToolCmd(void) {
	FILE *fp = NULL;
	char path[128];
	int size;

	memset(path, 0, sizeof(path));
	sprintf(path, "/mnt/sdcard/US360/Test/TestToolCmd.bin\0");
	fp = fopen(path, "rb");
	if(fp != NULL) {
//tmp		size = GetFileLength(path);
		if(size == 0) db_error("TestToolCmd.bin not found !\n");
		if(size <= sizeof(TestToolCmd) )
			fread(&TestToolCmd, 1, size, fp);
		else
			fread(&TestToolCmd, 1, sizeof(TestToolCmd), fp);
		fclose(fp);
	}
	testtool_set_AEG = getTestToolAEG();
	testtool_set_gain = getTestToolgain();
	db_debug("TestTool set AEG=%d, gain=%d\n", testtool_set_AEG, testtool_set_gain);

	Block_File_idx = 0;
	clean_tmp_file();
	if(TestToolCmd.MainCmd == 5)
		Sensor_Lens_Map_Cnt = 0;
	else if(TestToolCmd.MainCmd == 6 && TestToolCmd.SubCmd == 0)
		Sensor_Lens_Map_Cnt = 0;
	else if(TestToolCmd.MainCmd == 6 && TestToolCmd.SubCmd == 10)
		Sensor_Lens_Map_Cnt++;
}
void GetTestToolCmd(int *cmd) {
	*cmd     = TestToolCmd.State;
	*(cmd+1) = TestToolCmd.MainCmd;
	*(cmd+2) = TestToolCmd.SubCmd;
}
void doReadTestToolCmd(int *cmd) {
	ReadTestToolCmd();
	GetTestToolCmd(cmd);
}

void TestToolCmdInit(void) {
	TestToolCmd.State   = -1;
	TestToolCmd.MainCmd = -1;
	TestToolCmd.SubCmd  = -1;
	TestToolCmd.Step    = 0;
}

void SetTestToolStep(int step) {
	TestToolCmd.Step = step;
}
int GetTestToolStep(void) {
	return TestToolCmd.Step;
}

int GetTestToolState(void) {
	return TestToolCmd.State;
}


void ReadTestBlockTable()
{
	FILE *fp = NULL;
	char path[128];
	int size;

	memset(path, 0, sizeof(path));
	sprintf(path, "/mnt/sdcard/US360/Test/TestBlockTable.bin\0");

	fp = fopen(path, "rb");
	if(fp != NULL) {
//tmp		size = GetFileLength(path);
		if(size == 0) db_error("TestBlockTable.bin not found !\n");
		if(size <= sizeof(struct Test_Block_Table_Struct)*Test_Block_Table_MAX)
			fread(&Test_Block_Table[0], 1, size, fp);
		else
			fread(&Test_Block_Table[0], 1, sizeof(struct Test_Block_Table_Struct)*Test_Block_Table_MAX, fp);
		fclose(fp);
	}

	return 0;
}

void clean_tmp_file(void)
{
	int i;
	char path[128];
	struct stat sti;

	for(i = 0; i < 3; i++) {
		memset(path, 0, sizeof(path));
		sprintf(path, "/mnt/sdcard/US360/Test/BlockFileIdx_%d.bin\0", i);
		if(stat(path,&sti) == 0) {	//檔案存在
			db_debug("clean_tmp_file() bin %d\n", i);
			remove(path);
		}

		memset(path, 0, sizeof(path));
		sprintf(path, "/mnt/sdcard/US360/Test/Block_%d.jpg\0", i);
		if(stat(path,&sti) == 0) {	//檔案存在
			db_debug("clean_tmp_file() jpg %d\n", i);
			remove(path);
		}
	}
}

int ST_Idx = 0;
void STTableTestS2Focus(int s_id)
{
	set_A2K_Debug_Focus(s_id);
}

void Add_ST_Table_Proc_Test_S2_Show_ST_Line(int M_Mode, struct Adjust_Line_I3_Header_Struct *TP)
{
	int tmpaddr = 0;
	int offsetX = 0, offsetY = 0;
	int i = 0, j = 0, k = 0;
	int MT = 0, MT2 = 0;
	int page = 0;
	int sx, sy, tx, ty, sx2, sy2;
    int FPGA_ID;

	ST_Sum_Test[0] = 0;
	ST_Sum_Test[1] = 0;

    //if (TP->Source_Scale == 0) {
    	offsetX = 0;	//512;
    	offsetY = 512;
    //}
    //else {
    //	offsetX = 512 - Source_X_Delta; //Sitching_offsetX;
    //	offsetY = (START_SENSOR_Y << 2) - Source_Y_Delta; //256 ; //Sitching_offsetY;
    //}

    int E_cnt =0;

	for (i = 0; i < 4; i++) {
	    tx = i*7+(i/2);         ty = 0;
	    switch(M_Mode) {
	    case 0: sx = 20 + (i*45); sy = 18; break;
	    case 1: sx = 13 + (i*30); sy = 12; break;
	    case 3: sx = 0;           sy = 0;  break;
	    }
		for (j = 0; j <  17; j++) {
			for(k = 0; k < (7+(i%2) ); k++) {
				MT2 = Map_Posi_Tmp[sy+j  ][sx+k  ].Sensor_Sel;
				Get_FId_MT(MT2, &FPGA_ID, &MT);

				ST_P  = &ST_Test[FPGA_ID][ST_Sum_Test[FPGA_ID] ];
				SLT_P = &SLT_Test[FPGA_ID][ST_Sum_Test[FPGA_ID] ];


				int  V_Sensor_X_Step =  Sensor_X_Step_debug;	//Sensor_X_Step/2;

				if (MT2 > 4)
				  E_cnt++;

				if (MT2 <= 4) {

					ST_P->CB_Block_ID = (ST_Idx + ST_Sum_Test[FPGA_ID]) & 7;
					ST_P->CB_DDR_P   = ( ( ( ( (ty+j) << 20) + ( (tx+k) << 6) ) << 1) >> 5) + TP->Target_P;		//( (i * 64 * 16384 + j * 64) * 2) / 32 + TP->Target_P;
					ST_P->CB_Sensor_ID = 4;
					ST_P->CB_Alpha_Flag = 0;	//TP->Source_Scale;
					ST_P->CB_Mask   = 0;
					ST_P->CB_Posi_X0 = (Map_Posi_Tmp[sy+j  ][sx+k  ].S[MT2].X +MT * V_Sensor_X_Step ) * 4/TP->Binn  + offsetX;
					ST_P->CB_Posi_Y0 = (Map_Posi_Tmp[sy+j  ][sx+k  ].S[MT2].Y                       ) * 4/TP->Binn  + offsetY;
					ST_P->CB_Posi_X1 = (Map_Posi_Tmp[sy+j  ][sx+k+1].S[MT2].X +MT * V_Sensor_X_Step ) * 4/TP->Binn  + offsetX;
					ST_P->CB_Posi_Y1 = (Map_Posi_Tmp[sy+j  ][sx+k+1].S[MT2].Y                       ) * 4/TP->Binn  + offsetY;
					ST_P->CB_Posi_X2 = (Map_Posi_Tmp[sy+j+1][sx+k  ].S[MT2].X +MT * V_Sensor_X_Step ) * 4/TP->Binn  + offsetX;
					ST_P->CB_Posi_Y2 = (Map_Posi_Tmp[sy+j+1][sx+k  ].S[MT2].Y                       ) * 4/TP->Binn  + offsetY;
					ST_P->CB_Posi_X3 = (Map_Posi_Tmp[sy+j+1][sx+k+1].S[MT2].X +MT * V_Sensor_X_Step ) * 4/TP->Binn  + offsetX;
					ST_P->CB_Posi_Y3 = (Map_Posi_Tmp[sy+j+1][sx+k+1].S[MT2].Y                       ) * 4/TP->Binn  + offsetY;

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

					SLT_P->CB_Mode     = 0;
					SLT_P->CB_Idx   = 0;
					SLT_P->X_Posi   = j;
					SLT_P->Y_Posi   = i;

					ST_Sum_Test[FPGA_ID]++;
				}
			}
		}
	}


	int sum;
	if(TP->Binn == 1) {
		for(i = 0; i < 2; i++) {
			FPGA_ID = i;
			MT = 0;
			sum = 9720 - ST_Sum_Test[FPGA_ID];
			for(j = 0; j < sum; j++) {
				tx = 60+i; ty = 0;
				sx = 0; sy = 0;

				ST_P  = &ST_Test[FPGA_ID][ST_Sum_Test[FPGA_ID] ];
				SLT_P = &SLT_Test[FPGA_ID][ST_Sum_Test[FPGA_ID] ];


				int  V_Sensor_X_Step =  Sensor_X_Step_debug;	//Sensor_X_Step/2;

				if (MT2 > 4)
				  E_cnt++;

				if (MT2 <= 4) {

					ST_P->CB_Block_ID = (ST_Idx + ST_Sum_Test[FPGA_ID]) & 7;
					ST_P->CB_DDR_P   = ( ( ( (ty << 20) + (tx << 6) ) << 1) >> 5) + TP->Target_P;		//( (i * 64 * 16384 + j * 64) * 2) / 32 + TP->Target_P;
					ST_P->CB_Sensor_ID = 4;
					ST_P->CB_Alpha_Flag = 0;	//TP->Source_Scale;
					ST_P->CB_Mask   = 0;
					ST_P->CB_Posi_X0 = ( 0 +MT * V_Sensor_X_Step ) * 4/TP->Binn  + offsetX;
					ST_P->CB_Posi_Y0 = ( 0                       ) * 4/TP->Binn  + offsetY;
					ST_P->CB_Posi_X1 = (64 +MT * V_Sensor_X_Step ) * 4/TP->Binn  + offsetX;
					ST_P->CB_Posi_Y1 = ( 0                       ) * 4/TP->Binn  + offsetY;
					ST_P->CB_Posi_X2 = ( 0 +MT * V_Sensor_X_Step ) * 4/TP->Binn  + offsetX;
					ST_P->CB_Posi_Y2 = (64                       ) * 4/TP->Binn  + offsetY;
					ST_P->CB_Posi_X3 = (64 +MT * V_Sensor_X_Step ) * 4/TP->Binn  + offsetX;
					ST_P->CB_Posi_Y3 = (64                       ) * 4/TP->Binn  + offsetY;

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

					SLT_P->CB_Mode  = 0;
					SLT_P->CB_Idx   = 0;
					SLT_P->X_Posi   = 0;
					SLT_P->Y_Posi   = 0;

					ST_Sum_Test[FPGA_ID]++;
				}
			}
		}
	}
}
void STTableTestS2ShowSTLine(void) {
	int M_Mode, S_Mode;

	Get_M_Mode(&M_Mode, &S_Mode);
	Add_ST_Table_Proc_Test_S2_Show_ST_Line(M_Mode, &A_L_I3_Header[M_Mode]);

	send_Sitching_Cmd_Test();

	//Sitching_Idx++;
	//if(Sitching_Idx > 3) Sitching_Idx = 0;
}

void Add_ST_Table_Proc_Test_S2_All_Senosr(int M_Mode, struct Adjust_Line_I3_Header_Struct *TP)
{
//#ifdef NEW_US360_CODE_FLAG
	int tmpaddr = 0;
	int offsetX = 0, offsetY = 0;
	int i = 0, j = 0, k = 0;
	int MT = 0, MT2 = 0;
	int page = 0;
	int sx, sy, tx, ty, sx2, sy2;
    int FPGA_ID;

	ST_Sum_Test[0] = 0;
	ST_Sum_Test[1] = 0;

    //if (TP->Source_Scale == 0) {
    	offsetX = 0;	//512;
    	offsetY = 512;
    //}
    //else {
    //	offsetX = 512 - Source_X_Delta; //Sitching_offsetX;
    //	offsetY = (START_SENSOR_Y << 2) - Source_Y_Delta; //256 ; //Sitching_offsetY;
    //}

    int E_cnt =0;

    for(i = 0; i < 6; i++) {
    	for (j = 0; j < 15; j++) {
    		for(k = 0; k < 20; k++) {
    			switch (i) {
    			  case 0: tx =  0; ty =  0;		break;
    			  case 1: tx = 20; ty =  0;		break;
    			  case 2: tx = 40; ty =  0;		break;
    			  case 3: tx =  0; ty = 15;		break;
    			  case 4: tx = 20; ty = 15;		break;
    			  case 5: tx = 40; ty = 15;		break;
    			}
    			Get_FId_MT(i, &FPGA_ID, &MT);

    			sx = 26;
    			sy = 15;

    			if(i == 5){
    				sx2 = 52;
    				sy2 = 69;
    			}
    			else if(FPGA_ID == 0){
    				sx2 = sx + j;
    				sy2 = sy + (20-k);
    			}
    			else{
    				sx2 = sx + (15-j);
    				sy2 = sy + k;
    			}

    			ST_P  = &ST_Test[FPGA_ID][ST_Sum_Test[FPGA_ID] ];
    			SLT_P = &SLT_Test[FPGA_ID][ST_Sum_Test[FPGA_ID] ];

    			int  V_Sensor_X_Step =  Sensor_X_Step_debug;	//Sensor_X_Step/2;

    			ST_P->CB_Block_ID = (ST_Idx + ST_Sum_Test[FPGA_ID]) & 7;
    			ST_P->CB_DDR_P   = ( ( ( ( (ty+j) << 20) + ( (tx+k) << 6) ) << 1) >> 5) + TP->Target_P;		//( (i * 64 * 16384 + j * 64) * 2) / 32 + TP->Target_P;
    			ST_P->CB_Sensor_ID = 4;
    			ST_P->CB_Alpha_Flag = 0;	//TP->Source_Scale;
    			ST_P->CB_Mask   = 0;

    			if(FPGA_ID == 0){
    				ST_P->CB_Posi_X0 = ( sx2*64         +MT * V_Sensor_X_Step/TP->Binn )*4 + offsetX;
    				ST_P->CB_Posi_Y0 = ( (sy2+1)*64                                    )*4 + offsetY;
    				ST_P->CB_Posi_X1 = ( sx2*64         +MT * V_Sensor_X_Step/TP->Binn )*4 + offsetX;
    				ST_P->CB_Posi_Y1 = ( sy2*64                                        )*4 + offsetY;
    				ST_P->CB_Posi_X2 = ( (sx2+1)*64     +MT * V_Sensor_X_Step/TP->Binn )*4 + offsetX;
    				ST_P->CB_Posi_Y2 = ( (sy2+1)*64                                    )*4 + offsetY;
    				ST_P->CB_Posi_X3 = ( (sx2+1)*64     +MT * V_Sensor_X_Step/TP->Binn )*4 + offsetX;
    				ST_P->CB_Posi_Y3 = ( sy2*64                                        )*4 + offsetY;
    			}
    			else{
    				ST_P->CB_Posi_X0 = ( (sx2+1)*64     +MT * V_Sensor_X_Step/TP->Binn )*4 + offsetX;
    				ST_P->CB_Posi_Y0 = ( sy2*64                                        )*4 + offsetY;
    				ST_P->CB_Posi_X1 = ( (sx2+1)*64     +MT * V_Sensor_X_Step/TP->Binn )*4 + offsetX;
   					ST_P->CB_Posi_Y1 = ( (sy2+1)*64                                    )*4 + offsetY;
   					ST_P->CB_Posi_X2 = ( sx2*64         +MT * V_Sensor_X_Step/TP->Binn )*4 + offsetX;
   					ST_P->CB_Posi_Y2 = ( sy2*64                                        )*4 + offsetY;
   					ST_P->CB_Posi_X3 = ( sx2*64         +MT * V_Sensor_X_Step/TP->Binn )*4 + offsetX;
   					ST_P->CB_Posi_Y3 = ( (sy2+1)*64                                    )*4 + offsetY;
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

   				SLT_P->CB_Mode  = 0;
   				SLT_P->CB_Idx   = 0;
   				SLT_P->X_Posi   = j;
   				SLT_P->Y_Posi   = i;
    			ST_Sum_Test[FPGA_ID]++;
    		}
    	}
    }
//#endif
}
void STTableTestS2AllSensor(void) {
//#ifdef NEW_US360_CODE_FLAG
	int M_Mode, S_Mode;
	Get_M_Mode(&M_Mode, &S_Mode);
	Add_ST_Table_Proc_Test_S2_All_Senosr(M_Mode, &A_L_I3_Header[M_Mode]);

	send_Sitching_Cmd_Test();

	//Sitching_Idx++;
	//if(Sitching_Idx > 3) Sitching_Idx = 0;
//#endif
}

void S2_Add_ST_Table_Proc_Test(int M_Mode, struct Adjust_Line_I3_Header_Struct *TP)
{
	int tmpaddr = 0;
	int offsetX = 0, offsetY = 0;
	int i = 0, j = 0, k = 0;
	int MT = 0, MT2 = 0;
	int page = 0;
	int sx, sy, tx, ty, sx2, sy2;
    int FPGA_ID;

	ST_Sum_Test[0] = 0;
	ST_Sum_Test[1] = 0;

    offsetX = 0;	//512;
    offsetY = 512;

    int E_cnt =0;

    //垂直縫合線, 每條4*16個block, 分4次
	for (i = 0; i < 4; i++) {
	    tx = 0 + (i*4);  ty = 0;
		sx = 6 + (i*15); sy = 10;
		for (j = 0; j <  16; j++) {
			for(k = 0; k < 4; k++) {
				MT2 = Map_Posi_Tmp[sy+j  ][sx+k  ].Sensor_Sel;
				Get_FId_MT(MT2, &FPGA_ID, &MT);

				ST_P  = &ST_Test[FPGA_ID][ST_Sum_Test[FPGA_ID] ];
				SLT_P = &SLT_Test[FPGA_ID][ST_Sum_Test[FPGA_ID] ];

				int  V_Sensor_X_Step =  Sensor_X_Step_debug;	//Sensor_X_Step/2;

				if (MT2 > 4)
				  E_cnt++;

				if (MT2 <= 4) {

					ST_P->CB_Block_ID = (ST_Idx + ST_Sum_Test[FPGA_ID]) & 7;
					ST_P->CB_DDR_P   = ( ( ( ( (ty+j) << 20) + ( (tx+k) << 6) ) << 1) >> 5) + TP->Target_P;		//( (i * 64 * 16384 + j * 64) * 2) / 32 + TP->Target_P;
					ST_P->CB_Sensor_ID = 4;
					ST_P->CB_Alpha_Flag = 0;	//TP->Source_Scale;
					ST_P->CB_Mask   = 0;
					ST_P->CB_Posi_X0 = (Map_Posi_Tmp[sy+j  ][sx+k  ].S[MT2].X +MT * V_Sensor_X_Step ) * 4/TP->Binn  + offsetX;
					ST_P->CB_Posi_Y0 = (Map_Posi_Tmp[sy+j  ][sx+k  ].S[MT2].Y                       ) * 4/TP->Binn  + offsetY;
					ST_P->CB_Posi_X1 = (Map_Posi_Tmp[sy+j  ][sx+k+1].S[MT2].X +MT * V_Sensor_X_Step ) * 4/TP->Binn  + offsetX;
					ST_P->CB_Posi_Y1 = (Map_Posi_Tmp[sy+j  ][sx+k+1].S[MT2].Y                       ) * 4/TP->Binn  + offsetY;
					ST_P->CB_Posi_X2 = (Map_Posi_Tmp[sy+j+1][sx+k  ].S[MT2].X +MT * V_Sensor_X_Step ) * 4/TP->Binn  + offsetX;
					ST_P->CB_Posi_Y2 = (Map_Posi_Tmp[sy+j+1][sx+k  ].S[MT2].Y                       ) * 4/TP->Binn  + offsetY;
					ST_P->CB_Posi_X3 = (Map_Posi_Tmp[sy+j+1][sx+k+1].S[MT2].X +MT * V_Sensor_X_Step ) * 4/TP->Binn  + offsetX;
					ST_P->CB_Posi_Y3 = (Map_Posi_Tmp[sy+j+1][sx+k+1].S[MT2].Y                       ) * 4/TP->Binn  + offsetY;

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

					SLT_P->CB_Mode     = 0;
					SLT_P->CB_Idx   = 0;
					SLT_P->X_Posi   = j;
					SLT_P->Y_Posi   = i;

					ST_Sum_Test[FPGA_ID]++;
				}
			}
		}
	}


    //水平縫合線, 每條13*4個block, 分4次
	for (i = 0; i < 4; i++) {
	    tx = 17;  ty = (i*4);
	    switch(i) {
	    case 0: sx = 24; sy = 5; break;
	    case 1: sx = 9;  sy = 8; break;
	    case 2: sx = 54; sy = 5; break;
	    case 3: sx = 39; sy = 8; break;
	    }

		for (j = 0; j <  4; j++) {
			for(k = 0; k < 14; k++) {
				if(i == 2) {
					if(k < 6)  { sx2 = sx+k; sy2 = sy+j; }
					else       { sx2 = k-6;  sy2 = sy+j; }
				}
				else {
					sx2 = sx+k;
					sy2 = sy+j;
				}
				MT2 = Map_Posi_Tmp[sy2  ][sx2  ].Sensor_Sel;
				Get_FId_MT(MT2, &FPGA_ID, &MT);

				ST_P  = &ST_Test[FPGA_ID][ST_Sum_Test[FPGA_ID] ];
				SLT_P = &SLT_Test[FPGA_ID][ST_Sum_Test[FPGA_ID] ];


				int  V_Sensor_X_Step =  Sensor_X_Step_debug;	//Sensor_X_Step/2;

				if (MT2 > 4)
				  E_cnt++;

				if (MT2 <= 4) {

					ST_P->CB_Block_ID = (ST_Idx + ST_Sum_Test[FPGA_ID]) & 7;
					ST_P->CB_DDR_P   = ( ( ( ( (ty+j) << 20) + ( (tx+k) << 6) ) << 1) >> 5) + TP->Target_P;		//( (i * 64 * 16384 + j * 64) * 2) / 32 + TP->Target_P;
					ST_P->CB_Sensor_ID = 4;
					ST_P->CB_Alpha_Flag = 0;	//TP->Source_Scale;
					ST_P->CB_Mask   = 0;
					ST_P->CB_Posi_X0 = (Map_Posi_Tmp[sy2  ][sx2  ].S[MT2].X +MT * V_Sensor_X_Step ) * 4/TP->Binn  + offsetX;
					ST_P->CB_Posi_Y0 = (Map_Posi_Tmp[sy2  ][sx2  ].S[MT2].Y                       ) * 4/TP->Binn  + offsetY;
					ST_P->CB_Posi_X1 = (Map_Posi_Tmp[sy2  ][sx2+1].S[MT2].X +MT * V_Sensor_X_Step ) * 4/TP->Binn  + offsetX;
					ST_P->CB_Posi_Y1 = (Map_Posi_Tmp[sy2  ][sx2+1].S[MT2].Y                       ) * 4/TP->Binn  + offsetY;
					ST_P->CB_Posi_X2 = (Map_Posi_Tmp[sy2+1][sx2  ].S[MT2].X +MT * V_Sensor_X_Step ) * 4/TP->Binn  + offsetX;
					ST_P->CB_Posi_Y2 = (Map_Posi_Tmp[sy2+1][sx2  ].S[MT2].Y                       ) * 4/TP->Binn  + offsetY;
					ST_P->CB_Posi_X3 = (Map_Posi_Tmp[sy2+1][sx2+1].S[MT2].X +MT * V_Sensor_X_Step ) * 4/TP->Binn  + offsetX;
					ST_P->CB_Posi_Y3 = (Map_Posi_Tmp[sy2+1][sx2+1].S[MT2].Y                       ) * 4/TP->Binn  + offsetY;

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

					SLT_P->CB_Mode     = 0;
					SLT_P->CB_Idx   = 0;
					SLT_P->X_Posi   = j;
					SLT_P->Y_Posi   = i;

					ST_Sum_Test[FPGA_ID]++;
				}
			}
		}
	}
}
void S2AddSTTableProcTest(void) {
	int M_Mode, S_Mode;

	Get_M_Mode(&M_Mode, &S_Mode);
	S2_Add_ST_Table_Proc_Test(M_Mode, &A_L_I3_Header[M_Mode]);
	send_Sitching_Cmd_Test();

	//Sitching_Idx++;
	//if(Sitching_Idx > 3) Sitching_Idx = 0;
}

//mmap err
void creat_block_file(char *path, unsigned char *data, int size)
{
	db_debug("creat_block_file() 00 path %s\n", path);
	int idx;
	char idx_str, block_str[10] = "Block_";
	struct stat sti;
	unsigned char *addr;

	idx = strstr(path, block_str);
	//idx_str = path[idx + 6];
	db_debug("creat_block_file() 00-1\n");
	/*switch(idx_str) {
	case '0':
		if(block_fp[0] == NULL || stat(path,&sti) != 0) {
			block_fp[0] = fopen(path, "wb");
			fwrite(&data[0], size, 1, block_fp[0]);
		}
		else {
			addr = mmap(NULL, 0x32000, PROT_READ || PROT_WRITE, MAP_SHARED, block_fp[0], 0);
			memcpy(addr, data, size);
		}
		break;
	case '1':
		break;
	case '2':
		break;
	}*/

	if(block_fp[0] == NULL || stat(path,&sti) != 0) {
		db_debug("creat_block_file() 01 idx_str %c data %x %x size %d\n", idx_str, data[0], data[1], size);
		block_fp[0] = fopen(path, "wb");
		fwrite(&data[0], size, 1, block_fp[0]);
	}
	else {
		db_debug("creat_block_file() 02 idx_str %c data %x %x size %d\n", idx_str, data[0], data[1], size);
		addr = mmap(NULL, 0x32000, PROT_READ || PROT_WRITE, MAP_SHARED, block_fp[0], 0);
		db_debug("creat_block_file() 02-1 addr 0x%x\n", addr);
		memcpy(addr, data, size);
	}
}

/*void Add_SL_Table_Proc_Test(struct Trans_par  *TP)
{
	int i,j;

	for (i = 0; i < 10; i++) {
		for (j = 0; j < 8; j++) {
			ST_P = &ST_I[ST_Idx][][ST_Sum[ST_Idx] ];
			SLT_P = &SLT[ST_Idx[]][ST_Sum[ST_Idx] ];
			ST_I[ST_Idx][][ST_Sum[ST_Idx] ] = SL[(i * 8) + j];

			ST_P->CB_Block_ID = (ST_Idx + ST_Sum[ST_Idx]) & 7;
			ST_P->CB_DDR_P   = ((i * 64 * 16384 + j * 64) * 2) / 32 + TP->Target_P;

			SLT_P->CB_Mode  = 0;
			SLT_P->CB_Idx   = 0;
			SLT_P->X_Posi   = 0;
			SLT_P->Y_Posi   = 0;

			ST_Sum[ST_Idx]++;
		}
	}
}*/

void SLTableTest()
{
/*	int mode, res;
	struct Trans_par  TP;

	mode = Stitching_Out.Output_Mode;
	res = Stitching_Out.OC[mode].Resolution_Mode;
	switch(mode) {
	case 0:
		if(res == 1)      TP = Out_Mode_G_0.TP;
		else if(res == 2) TP = Out_Mode_G_1.TP;
		else if(res == 7) TP = Out_Mode_G_2.TP;
		break;
	case 1: TP = Out_Mode_Plane_0.TP;      break;
	case 2: TP = Out_Mode_Pillar_360_0.TP; break;
	case 3:                                break;
	case 4: TP = Out_Mode_Pillar_180_0.TP; break;
	case 5: TP = Out_Mode_Plane_4_0.TP;    break;
	case 6:                                break;
	default: TP = Out_Mode_G_0.TP;         break;
	}

	ST_Sum[ST_Idx] = 0;
	Add_SL_Table_Proc_Test(&TP);

	send_Sitching_Cmd(&TP);

//	ST_Idx++;
//	if(ST_Idx > 7) ST_Idx = 0;

	//Sitching_Idx++;
	//if(Sitching_Idx > 3) Sitching_Idx = 0;*/
}


int getBatteryNowCapacity(){
	char buf[16];
	int fd;
	fd = open("/sys/class/power_supply/battery/capacity\0", O_RDONLY);
	if(fd < 0){
		db_error("getBatteryNowCapacity: open '/sys/class/power_supply/battery/capacity' Err. fd=%d\n", fd);
		return -1;
	}

	memset(&buf[0], 0, 16);
	read(fd, &buf[0], 16);
	close(fd);

	int capacity = -1;
	sscanf(buf, "%d", &capacity);

	return capacity;
}

int getBatteryNowStatus(){
	char buf[16];
	int fd;
	fd = open("/sys/class/power_supply/battery/status\0", O_RDONLY);
	if(fd < 0){
		db_error("getBatteryNowCapacity: open '/sys/class/power_supply/battery/status' Err. fd=%d\n", fd);
		return -1;
	}

	memset(&buf[0], 0, 16);
	read(fd, &buf[0], 16);
	close(fd);

	int status = -1;

	switch(buf[0]){
		case 'U':			// unknow
			status = 1;
			break;
		case 'C':			// charging
			status = 2;
			break;
		case 'D':			// discharging
			status = 3;
			break;
		case 'N':			// not charging
			status = 4;
			break;
		case 'F':			// full
			status = 5;
			break;
		default:
			status = -1;
			break;
	}

	return status;
}

int getBatteryNowVoltage(){
	char buf[16];
	int fd;
	fd = open("/sys/class/power_supply/battery/voltage_now\0", O_RDONLY);
	if(fd < 0){
		db_error("getBatteryNowCapacity: open '/sys/class/power_supply/battery/voltage_now' Err. fd=%d\n", fd);
		return -1;
	}

	memset(&buf[0], 0, 16);
	read(fd, &buf[0], 16);
	close(fd);

	int voltage = -1;
	sscanf(buf, "%d", &voltage);

	return voltage;
}

int getBatteryNowCurrent(){
	char buf[16];
	int fd;
	fd = open("/sys/class/power_supply/battery/current_now\0", O_RDONLY);
	if(fd < 0){
		db_error("getBatteryNowCapacity: open '/sys/class/power_supply/battery/current_now' Err. fd=%d\n", fd);
		return -1;
	}

	memset(&buf[0], 0, 16);
	read(fd, &buf[0], 16);
	close(fd);

	int current = -1;
	sscanf(buf, "%d", &current);

	return current;
}

char VersionStrJNI[32];
void setVersionJNI(char *ver, int len)
{
	memcpy(&VersionStrJNI[0], ver, len);
	VersionStrJNI[len] = '\0';
}

char T_mSSID[16];
//Test_Tool_Result_Struct Test_Tool_Result;
/*
 * type: 0:本機縫合	1:製具縫合
 */
void WriteTestResult(int type, int debug)
{
	FILE *fp;
	char path[128];
    char us363Ver[64];

    getUS363Version(&us363Ver[0]);
	Test_Tool_Result.CheckSum          = TEST_RESULT_CHECK_SUM;
	Test_Tool_Result.TestBlockTableNum = Test_Block_Table_MAX;
	sprintf(Test_Tool_Result.SSID, "%s\0", T_mSSID);
	sprintf(Test_Tool_Result.AletaVer, "%s\0", VersionStrJNI);
	if(type == 0) {
		if(debug == 1)
			Test_Tool_Result.StitchVer = S2_STITCH_VERSION_DEBUG;
		else
			Test_Tool_Result.StitchVer = S2_STITCH_VERSION;
	}
	else if(type == 1) {
		if(Test_Tool_Adj.checksum >= SENSOR_ADJ_CHECK_SUM)
			Test_Tool_Result.StitchVer = Test_Tool_Adj.stitch_ver;
		else
			Test_Tool_Result.StitchVer = 0;
		sprintf(Test_Tool_Result.TestToolVer, "%s\0", Test_Tool_Adj.test_tool_ver);
	}

	sprintf(path, "/mnt/sdcard/US360/TestResult.bin\0");
	fp = fopen(path, "wb");
	if(fp != NULL) {
		fwrite(&Test_Tool_Result, sizeof(Test_Tool_Result), 1, fp);
		fclose(fp);
	}
}

int ReadTestResult(void)
{
	FILE *fp;
	char path[128];
	struct stat sti;

	memset(&Test_Tool_Result, 0, sizeof(Test_Tool_Result) );
	sprintf(path, "/mnt/sdcard/US360/TestResult.bin\0");
	if(stat(path, &sti) == 0) {
		fp = fopen(path, "rb");
		if(fp != NULL) {
			fread(&Test_Tool_Result, sizeof(Test_Tool_Result), 1, fp);
			fclose(fp);

			db_debug("ReadTestResult() checksum=0x%x SSID=%s ToolVer=%s AletaVer=%s BlockNum=%d StitchVer=0x%x\n",
					Test_Tool_Result.CheckSum, Test_Tool_Result.SSID, Test_Tool_Result.TestToolVer,
					Test_Tool_Result.AletaVer, Test_Tool_Result.TestBlockTableNum, Test_Tool_Result.StitchVer);

			if(Test_Tool_Result.StitchVer != S2_STITCH_VERSION)
				return 1;
		}
		else { return -1; }
	}
	else { return -2; }

	return 0;
}

int getTestBlockNum(void)
{
	return Test_Tool_Result.TestBlockTableNum;
}

int getTestToolAEG(){
	char buf[16];
	int fd;
	fd = open("/mnt/sdcard/US360/Test/TestToolAEG.bin\0", O_RDONLY);
	if(fd < 0){
		return -1000;
	}

	memset(&buf[0], 0, 16);
	read(fd, &buf[0], 16);
	close(fd);

	int aeg = -1000;
	sscanf(buf, "%d", &aeg);

	return aeg;
}

int getTestToolgain(){
	char buf[16];
	int fd;
	fd = open("/mnt/sdcard/US360/Test/TestToolgain.bin\0", O_RDONLY);
	if(fd < 0){
		return -1000;
	}

	memset(&buf[0], 0, 16);
	read(fd, &buf[0], 16);
	close(fd);

	int gain = -1000;
	sscanf(buf, "%d", &gain);

	return gain;
}

// Main.java
//testtool.State = 1
//testtool.MainCmd = 2:         // Focus
//                 = 5;         // take picture & writeSensorLine();
//                 = 6;         // take picture & doSensorLensEn(2)
//                 = 7;         // focus 
//                 = 100;       // S2 sensor*5 auto st
/*
 * return = 0: test tool disable
 *        > 0: test tool enable
 */
int get_TestToolCmd(void)
{
    int state = TestToolCmd.State;
    int main_cmd = TestToolCmd.MainCmd;
    int sub_cmd = TestToolCmd.SubCmd;
    int ret = 0;

    if(state == 0 || state == -1){
        return 0;                   // disable
    }
    else{
        //if(main_cmd == 2 || main_cmd == 5 || main_cmd == 6 || main_cmd == 7 || main_cmd == 100)
        
        ret = ( ( (state&0xff)<<16) | ( (sub_cmd&0xff)<<8) | (main_cmd&0xff) );
        return ret;                 // enable
    }
    return ret;
}

// rex+ 180214
void set_T_mSSID(char *ssid)
{
    memcpy(T_mSSID, ssid, sizeof(T_mSSID));
}
int get_Sensor_Lens_Map_Cnt(void)
{
    return Sensor_Lens_Map_Cnt;
}



/* Test Mode retrun files ---------------------------------------------------------------------------------------------------------------------------------------------------*/

void remove_file(char *path) {
	struct stat sti;
    if(stat(path, &sti) == 0) {
        remove(path);
    }
}

void create_file(char *dir, char *path) {
	FILE *fp = NULL;
	struct stat sti;
	int value = 1;
	if(stat(dir, &sti) != 0) {
        if(mkdir(dir, S_IRWXU) != 0)
            db_error("create_file() create %s folder fail\n", dir);
	}
	if(stat(path, &sti) != 0) {
		fp = fopen(path, "wb");
		if(fp != NULL) {
			fwrite(&value, sizeof(value), 1, fp);
			fclose(fp);
		}
		else
			db_error("create_file() create %s file fail\n", path);
	}
}

// oled
void deleteTestModeOledFile(void) {
   	char fileStr[64] = "/mnt/sdcard/US360/testModeOled.bin\0";
   	remove_file(&fileStr[0]);
}

void createTestModeOledFile(void) {
	char dirStr[64]  = "/mnt/sdcard/US360\0";
	char fileStr[64] = "/mnt/sdcard/US360/testModeOled.bin\0";
	create_file(&dirStr[0], &fileStr[0]);
}

// led
void deleteTestModeLedFile(void) {
   	char fileStr[64] = "/mnt/sdcard/US360/testModeLed.bin\0";
   	remove_file(&fileStr[0]);
}

void createTestModeLedFile(void) {
   	char dirStr[64]  = "/mnt/sdcard/US360\0";
	char fileStr[64] = "/mnt/sdcard/US360/testModeLed.bin\0";
	create_file(&dirStr[0], &fileStr[0]);
}

// gsensor
void deleteTestModeGsensorFile(void) {
   	char fileStr[64] = "/mnt/sdcard/US360/testModeGsensor.bin\0";
   	remove_file(&fileStr[0]);
}

void createTestModeGsensorFile(void) {
   	char dirStr[64]  = "/mnt/sdcard/US360\0";
	char fileStr[64] = "/mnt/sdcard/US360/testModeGsensor.bin\0";
	create_file(&dirStr[0], &fileStr[0]);
}

void deleteTestModeGsensorErrorFile(void) {
	char fileStr[64] = "/mnt/sdcard/US360/testModeGsensorError.bin\0";
	remove_file(&fileStr[0]);
}

void createTestModeGsensorErrorFile(void) {
    char dirStr[64]  = "/mnt/sdcard/US360\0";
    char fileStr[64] = "/mnt/sdcard/US360/testModeGsensorError.bin\0";
    create_file(&dirStr[0], &fileStr[0]);
}

// sdcard
void deleteTestModeSdcardFile(void) {
	char fileStr[64] = "/mnt/sdcard/US360/testModeSdcard.bin\0";
	remove_file(&fileStr[0]);
}

void createTestModeSdcardFile(void) {
	char dirStr[64]  = "/mnt/sdcard/US360\0";
	char fileStr[64] = "/mnt/sdcard/US360/testModeSdcard.bin\0";
	create_file(&dirStr[0], &fileStr[0]);
}

void deleteTestModeSdcardSkipFile(void) {
	char fileStr[64] = "/mnt/sdcard/US360/testModeSdcardSkip.bin\0";
	remove_file(&fileStr[0]);
}

void createTestModeSdcardSkipFile(void) {
	char dirStr[64]  = "/mnt/sdcard/US360\0";
	char fileStr[64] = "/mnt/sdcard/US360/testModeSdcardSkip.bin\0";
	create_file(&dirStr[0], &fileStr[0]);
}

// focus
void deleteTestModeFocusFile(void) {
	char fileStr[64] = "/mnt/sdcard/US360/testModeFocus.bin\0";
	remove_file(&fileStr[0]);
}

void createTestModeFocusFile(void) {
	char dirStr[64]  = "/mnt/sdcard/US360\0";
	char fileStr[64] = "/mnt/sdcard/US360/testModeFocus.bin\0";
	create_file(&dirStr[0], &fileStr[0]);
}

void deleteGetFocusFile(void) {
	char fileStr[64] = "/mnt/sdcard/US360/Test/get_focus.bin\0";
	remove_file(&fileStr[0]);
}

int checkGetFocusFile(void) {
	char fileStr[64] = "/mnt/sdcard/US360/Test/get_focus.bin\0";
	struct stat sti;
	if(stat(fileStr, &sti) == 0)
		return 1;
	else
		return 0;
}

void deleteAdjFocusRAW(void) {
	int i;
	char fileStr[64];
	for(i = 0; i < 5; i++) {
    	sprintf(fileStr, "/mnt/sdcard/US360/Test/RAW_S%d.jpg\0", i);
    	remove_file(&fileStr[0]);
	}
}

void deleteAdjFocusPosi(void) {
	char fileStr[64] = "/mnt/sdcard/US360/Test/Adj_Foucs_Posi.bin\0";
	remove_file(&fileStr[0]);
}

// buttom
void deleteTestModeButtonActionFile(void) {
	char fileStr[64] = "/mnt/sdcard/US360/testModeButtonAction.bin\0";
	remove_file(&fileStr[0]);
}

void createTestModeButtonActionFile(void) {
	char dirStr[64]  = "/mnt/sdcard/US360\0";
	char fileStr[64] = "/mnt/sdcard/US360/testModeButtonAction.bin\0";
	create_file(&dirStr[0], &fileStr[0]);
}

void deleteTestModeButtonPowerFile(void) {
	char fileStr[64] = "/mnt/sdcard/US360/testModeButtonPower.bin\0";
	remove_file(&fileStr[0]);
}

void createTestModeButtonPowerFile(void) {
	char dirStr[64]  = "/mnt/sdcard/US360\0";
	char fileStr[64] = "/mnt/sdcard/US360/testModeButtonPower.bin\0";
	create_file(&dirStr[0], &fileStr[0]);
}

// fan
void deleteTestModeFanFile(void) {
	char fileStr[64] = "/mnt/sdcard/US360/testModeFan.bin\0";
	remove_file(&fileStr[0]);
}

void createTestModeFanFile(void) {
	char dirStr[64]  = "/mnt/sdcard/US360\0";
	char fileStr[64] = "/mnt/sdcard/US360/testModeFan.bin\0";
	create_file(&dirStr[0], &fileStr[0]);
}

// battery
void deleteTestModeBatteryFile(void) {
	char fileStr[64] = "/mnt/sdcard/US360/testModeBattery.bin\0";
	remove_file(&fileStr[0]);
}

void createTestModeBatteryFile(void) {
	char dirStr[64]  = "/mnt/sdcard/US360\0";
	char fileStr[64] = "/mnt/sdcard/US360/testModeBattery.bin\0";
	create_file(&dirStr[0], &fileStr[0]);
}

void deleteTestModeBatteryErrorFile(void) {
	char fileStr[64] = "/mnt/sdcard/US360/testModeBatteryError.bin\0";
	remove_file(&fileStr[0]);
}

void createTestModeBatteryErrorFile(void) {
	char dirStr[64]  = "/mnt/sdcard/US360\0";
	char fileStr[64] = "/mnt/sdcard/US360/testModeBatteryError.bin\0";
	create_file(&dirStr[0], &fileStr[0]);
}

// hdmi
void deleteTestModeHDMIFile(void) {
	char fileStr[64] = "/mnt/sdcard/US360/testModeHDMI.bin\0";
	remove_file(&fileStr[0]);
}

void createTestModeHDMIFile(void) {
	char dirStr[64]  = "/mnt/sdcard/US360\0";
	char fileStr[64] = "/mnt/sdcard/US360/testModeHDMI.bin\0";
	create_file(&dirStr[0], &fileStr[0]);
}

void deleteTestModeHDMISkipFile(void) {
	char fileStr[64] = "/mnt/sdcard/US360/testModeHDMISkip.bin\0";
	remove_file(&fileStr[0]);
}

void createTestModeHDMISkipFile(void) {
	char dirStr[64]  = "/mnt/sdcard/US360\0";
	char fileStr[64] = "/mnt/sdcard/US360/testModeHDMISkip.bin\0";
	create_file(&dirStr[0], &fileStr[0]);
}

// wifi
void deleteTestModeWifiFile(void) {
	char fileStr[64] = "/mnt/sdcard/US360/testModeWifi.bin\0";
	remove_file(&fileStr[0]);
}

void createTestModeWifiFile(void) {
	char dirStr[64]  = "/mnt/sdcard/US360\0";
	char fileStr[64] = "/mnt/sdcard/US360/testModeWifi.bin\0";
	create_file(&dirStr[0], &fileStr[0]);
}

void deleteTestModeWifiErrorFile(void) {
	char fileStr[64] = "/mnt/sdcard/US360/testModeWifiError.bin\0";
	remove_file(&fileStr[0]);
}

void createTestModeWifiErrorFile(void) {
	char dirStr[64]  = "/mnt/sdcard/US360\0";
	char fileStr[64] = "/mnt/sdcard/US360/testModeWifiError.bin\0";
	create_file(&dirStr[0], &fileStr[0]);
}

// audio
void deleteTestModeAudioFile(void) {
	char fileStr[64] = "/mnt/sdcard/US360/testModeAudio.bin\0";
	remove_file(&fileStr[0]);
}

void createTestModeAudioFile(void) {
	char dirStr[64]  = "/mnt/sdcard/US360\0";
	char fileStr[64] = "/mnt/sdcard/US360/testModeAudio.bin\0";
	create_file(&dirStr[0], &fileStr[0]);
}

void deleteTestModeAudioDoneFile(void) {
	char fileStr[64] = "/mnt/sdcard/US360/testModeAudioDone.bin\0";
	remove_file(&fileStr[0]);
}

void createTestModeAudioDoneFile(void) {
	char dirStr[64]  = "/mnt/sdcard/US360\0";
	char fileStr[64] = "/mnt/sdcard/US360/testModeAudioDone.bin\0";
	create_file(&dirStr[0], &fileStr[0]);
}

// dna
void deleteTestModeDna(void) {
	char fileStr[64] = "/mnt/sdcard/US360/read_dna.bin\0";
	remove_file(&fileStr[0]);
}

void deleteTestModeDnaDone(void) {
	char fileStr[64] = "/mnt/sdcard/US360/testModeDnaDone.bin\0";
	remove_file(&fileStr[0]);
}

void createTestModeDnaDone(void) {
	char dirStr[64]  = "/mnt/sdcard/US360\0";
	char fileStr[64] = "/mnt/sdcard/US360/testModeDnaDone.bin\0";
	create_file(&dirStr[0], &fileStr[0]);
}

void deleteTestModeDnaError(void) {
	char fileStr[64] = "/mnt/sdcard/US360/testModeDnaError.bin\0";
	remove_file(&fileStr[0]);
}

void createTestModeDnaError(void) {
	char dirStr[64]  = "/mnt/sdcard/US360\0";
	char fileStr[64] = "/mnt/sdcard/US360/testModeDnaError.bin\0";
	create_file(&dirStr[0], &fileStr[0]);
}

// auto stitch
void deleteAutoStitchFinishFile(void) {
	char fileStr[64] = "/mnt/sdcard/US360/AutoStitchFinish.bin\0";
	remove_file(&fileStr[0]);
}

void createAutoStitchFinishFile(void) {
	char dirStr[64]  = "/mnt/sdcard/US360\0";
	char fileStr[64] = "/mnt/sdcard/US360/AutoStitchFinish.bin\0";
	create_file(&dirStr[0], &fileStr[0]);
}

void createAutoStitchRestartFile(void) {
	char dirStr[64]  = "/mnt/sdcard/US360/Test\0";
	char fileStr[64] = "/mnt/sdcard/US360/Test/AutoStitchRestart.bin\0";
	create_file(&dirStr[0], &fileStr[0]);
}

void createAutoStitchErrorFile(void) {
	char dirStr[64]  = "/mnt/sdcard/US360\0";
	char fileStr[64] = "/mnt/sdcard/US360/AutoStitchError.bin\0";
	create_file(&dirStr[0], &fileStr[0]);
}

int checkAutoStitchFile(void) {
	char fileStr1[64] = "/mnt/sdcard/US360Config.bin\0";
	char fileStr2[64] = "/mnt/sdcard/US360/Test/ALI2_Result.bin\0";
	struct stat sti1, sti2;
	if(stat(fileStr1, &sti1) == 0 && stat(fileStr2, &sti2) == 0)
		return 1;
	else
		return 0;
}

void deleteGetStJpgFile(void) {
	char fileStr[64] = "/mnt/sdcard/US360/Test/get_st_jpg.bin\0";
	remove_file(&fileStr[0]);
}

int checkGetStJpgFile(void) {
	char fileStr[64] = "/mnt/sdcard/US360/Test/get_st_jpg.bin\0";
	struct stat sti;
	if(stat(fileStr, &stat) == 0)
		return 1;
	else
		return 0;
}

//Background Bottom User
void deleteBackgroundBottomUserFile(void) {
	char fileStr[64];
//tmp BOTTOM_FILE_NAME_USER	sprintf(fileStr, "/mnt/sdcard/US360/Background/%s.jpg\0", BOTTOM_FILE_NAME_USER);
	remove_file(&fileStr[0]);
}

// stop
void deleteTestModeFiles(void) {
	deleteTestModeOledFile();

	deleteTestModeLedFile();

	deleteTestModeGsensorFile();
	deleteTestModeGsensorErrorFile();

	deleteTestModeSdcardFile();
	deleteTestModeSdcardSkipFile();

	deleteTestModeFocusFile();

	deleteTestModeButtonActionFile();
	deleteTestModeButtonPowerFile();

	deleteTestModeFanFile();

	deleteTestModeBatteryFile();
	deleteTestModeBatteryErrorFile();

	deleteTestModeHDMIFile();
	deleteTestModeHDMISkipFile();

	deleteTestModeWifiFile();
	deleteTestModeWifiErrorFile();

	deleteTestModeAudioFile();
	deleteTestModeAudioDoneFile();

	deleteTestModeDna();
	deleteTestModeDnaDone();
	deleteTestModeDnaError();
}

/* Test Mode retrun files ---------------------------------------------------------------------------------------------------------------------------------------------------*/
