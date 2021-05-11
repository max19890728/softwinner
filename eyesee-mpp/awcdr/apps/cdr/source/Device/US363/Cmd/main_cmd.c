/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Cmd/main_cmd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <malloc.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "common/app_log.h"
#include "Device/US363/Cmd/us363_spi.h"
#include "Device/US363/Cmd/us360_define.h"
#include "Device/US363/Cmd/AletaS2_CMD_Struct.h"
#include "Device/US363/Cmd/jpeg_header.h"
#include "Device/US363/Cmd/variable.h"
#include "Device/US363/Kernel/k_spi_cmd.h"

#undef LOG_TAG
#define LOG_TAG "US363::MainCmd"

AS2_F0_MAIN_CMD_struct FX_MAIN_CMD_P[2];
AS2_F0_MAIN_CMD_struct FX_MAIN_CMD_Q[64];
AS2_F2_MAIN_CMD_struct F2_MAIN_CMD_P[2];
AS2_F2_MAIN_CMD_struct F2_MAIN_CMD_Q[64];

int ISP1_RGB_Offset_I = 0;
int ISP1_RGB_Offset_O = 0;

//debug
void setISP1RGBOffset(int idx, int value) {
	if(idx == 0) ISP1_RGB_Offset_I = (value & 0xF);
	else         ISP1_RGB_Offset_O = (value & 0x3F);
	set_A2K_ISP1_RGB(ISP1_RGB_Offset_I, ISP1_RGB_Offset_O);
}

//debug
void getISP1RGBOffset(int *val) {
	*val     = ISP1_RGB_Offset_I;
	*(val+1) = ISP1_RGB_Offset_O;
}

void AS2_F0_DS_CMD_Init() {
    AS2_F0_MAIN_CMD_struct *F_M_P;
    int V_Start,H_Start;
    int TimeOut_CLK;
    int TimeOut_Time;
    int TimeOut_Value;
    int TimeOut_En;

    TimeOut_CLK   = 100;        // MHz
    TimeOut_Time  = 33333;      // ISP1_TIME_OUT; //us
    TimeOut_Value = 0x4000000 - TimeOut_CLK * TimeOut_Time;
    TimeOut_En    = 1;

    V_Start = AS2_S3_V_START;
    H_Start = AS2_S3_H_START;

    memset(&FX_MAIN_CMD_P[0], 0, sizeof(FX_MAIN_CMD_P) );

    F_M_P = &FX_MAIN_CMD_P[0];
    //============ ISP1 ==============
    F_M_P->F0_ISP1_CMD.R_GainI.Address              = 0xCCAA02D1;    F_M_P->F0_ISP1_CMD.R_GainI.Data                 = 0x40000000;
    F_M_P->F0_ISP1_CMD.G_GainI.Address              = 0xCCAA02D2;    F_M_P->F0_ISP1_CMD.G_GainI.Data                 = 0x40000000;
    F_M_P->F0_ISP1_CMD.B_GainI.Address              = 0xCCAA02D3;    F_M_P->F0_ISP1_CMD.B_GainI.Data                 = 0x40000000;
    F_M_P->F0_ISP1_CMD.R_Gain0.Address              = 0xCCAA02DE;    F_M_P->F0_ISP1_CMD.R_Gain0.Data                 = 0x60000000;
    F_M_P->F0_ISP1_CMD.G_Gain0.Address              = 0xCCAA02DF;    F_M_P->F0_ISP1_CMD.G_Gain0.Data                 = 0x40000000;
    F_M_P->F0_ISP1_CMD.B_Gain0.Address              = 0xCCAA02E0;    F_M_P->F0_ISP1_CMD.B_Gain0.Data                 = 0x60000000;
//    F_M_P->F0_ISP1_CMD.M_00.Address                 = 0xCCAA02C8;    F_M_P->F0_ISP1_CMD.M_00.Data                    = 0x2347;
//    F_M_P->F0_ISP1_CMD.M_01.Address                 = 0xCCAA02C9;    F_M_P->F0_ISP1_CMD.M_01.Data                    = 0x84B8;
//    F_M_P->F0_ISP1_CMD.M_02.Address                 = 0xCCAA02CA;    F_M_P->F0_ISP1_CMD.M_02.Data                    = 0x0170;
//    F_M_P->F0_ISP1_CMD.M_10.Address                 = 0xCCAA02CB;    F_M_P->F0_ISP1_CMD.M_10.Data                    = 0x82E1;
//    F_M_P->F0_ISP1_CMD.M_11.Address                 = 0xCCAA02CC;    F_M_P->F0_ISP1_CMD.M_11.Data                    = 0x21EB;
//    F_M_P->F0_ISP1_CMD.M_12.Address                 = 0xCCAA02CD;    F_M_P->F0_ISP1_CMD.M_12.Data                    = 0x00F5;
//    F_M_P->F0_ISP1_CMD.M_20.Address                 = 0xCCAA02CE;    F_M_P->F0_ISP1_CMD.M_20.Data                    = 0x00F5;
//    F_M_P->F0_ISP1_CMD.M_21.Address                 = 0xCCAA02CF;    F_M_P->F0_ISP1_CMD.M_21.Data                    = 0x89C2;
//    F_M_P->F0_ISP1_CMD.M_22.Address                 = 0xCCAA02D0;    F_M_P->F0_ISP1_CMD.M_22.Data                    = 0x28CC;
    F_M_P->F0_ISP1_CMD.VH_Start_Mode.Address        = 0xCCAA02E1;    F_M_P->F0_ISP1_CMD.VH_Start_Mode.Data           = FX_START_MODE;
    F_M_P->F0_ISP1_CMD.S0_F_DDR_Addr.Address        = 0xCCAA0201;    F_M_P->F0_ISP1_CMD.S0_F_DDR_Addr.Data           = (ISP1_STM1_T_P0_A_BUF_ADDR + 32) >> 5;
    F_M_P->F0_ISP1_CMD.S1_F_DDR_Addr.Address        = 0xCCAA0206;    F_M_P->F0_ISP1_CMD.S1_F_DDR_Addr.Data           = (ISP1_STM1_T_P0_B_BUF_ADDR + 32) >> 5;
    F_M_P->F0_ISP1_CMD.S2_F_DDR_Addr.Address        = 0xCCAA020B;    F_M_P->F0_ISP1_CMD.S2_F_DDR_Addr.Data           = (ISP1_STM1_T_P0_C_BUF_ADDR + 32) >> 5;
    F_M_P->F0_ISP1_CMD.S0_LC_DDR_Addr.Address       = 0xCCAA02E8;    F_M_P->F0_ISP1_CMD.S0_LC_DDR_Addr.Data          = (FX_LC_FS_A_BUF_Addr >> 5) + 0x80000000;
    F_M_P->F0_ISP1_CMD.S1_LC_DDR_Addr.Address       = 0xCCAA02E9;    F_M_P->F0_ISP1_CMD.S1_LC_DDR_Addr.Data          = (FX_LC_FS_B_BUF_Addr >> 5) + 0x80000000;
    F_M_P->F0_ISP1_CMD.S2_LC_DDR_Addr.Address       = 0xCCAA02EA;    F_M_P->F0_ISP1_CMD.S2_LC_DDR_Addr.Data          = (FX_LC_FS_C_BUF_Addr >> 5) + 0x80000000;
    F_M_P->F0_ISP1_CMD.S0_H_PIX_Size.Address        = 0xCCAA02F0;    F_M_P->F0_ISP1_CMD.S0_H_PIX_Size.Data           = DS_H_PIXEL_SIZE_D3;
    F_M_P->F0_ISP1_CMD.S1_H_PIX_Size.Address        = 0xCCAA02F1;    F_M_P->F0_ISP1_CMD.S1_H_PIX_Size.Data           = DS_H_PIXEL_SIZE_D3;
    F_M_P->F0_ISP1_CMD.S2_H_PIX_Size.Address        = 0xCCAA02F2;    F_M_P->F0_ISP1_CMD.S2_H_PIX_Size.Data           = DS_H_PIXEL_SIZE_D3;
    F_M_P->F0_ISP1_CMD.S0_TimeOut_Set.Address       = 0x00000000;    F_M_P->F0_ISP1_CMD.S0_TimeOut_Set.Data          = ((TimeOut_En << 31) | TimeOut_Value);
    F_M_P->F0_ISP1_CMD.S1_TimeOut_Set.Address       = 0x00000000;    F_M_P->F0_ISP1_CMD.S1_TimeOut_Set.Data          = ((TimeOut_En << 31) | TimeOut_Value);
    F_M_P->F0_ISP1_CMD.S2_TimeOut_Set.Address       = 0x00000000;    F_M_P->F0_ISP1_CMD.S2_TimeOut_Set.Data          = ((TimeOut_En << 31) | (0x4000000 - TimeOut_CLK * 5000) );
    F_M_P->F0_ISP1_CMD.V_H_Start.Address            = 0xCCAA02B8;    F_M_P->F0_ISP1_CMD.V_H_Start.Data               = ((V_Start << 8) | H_Start);
    F_M_P->F0_ISP1_CMD.S0_ISP1_START.Address        = 0x00000000;    F_M_P->F0_ISP1_CMD.S0_ISP1_START.Data           = 0x0;    //0x000002B9
    F_M_P->F0_ISP1_CMD.S1_ISP1_START.Address        = 0x00000000;    F_M_P->F0_ISP1_CMD.S1_ISP1_START.Data           = 0x0;    //0x000002BA
    F_M_P->F0_ISP1_CMD.S2_ISP1_START.Address        = 0x00000000;    F_M_P->F0_ISP1_CMD.S2_ISP1_START.Data           = 0x0;    //0x000002BB
    F_M_P->F0_ISP1_CMD.B_Mode.Address               = 0xCCAA02E4;    F_M_P->F0_ISP1_CMD.B_Mode.Data                  = DS_ISP1_BINN_OFF;
    F_M_P->F0_ISP1_CMD.GAMMA_PARM0.Address          = 0xCCAA02E5;    F_M_P->F0_ISP1_CMD.GAMMA_PARM0.Data             = 0xA8D12C48;
    F_M_P->F0_ISP1_CMD.GAMMA_PARM1.Address          = 0xCCAA02E6;    F_M_P->F0_ISP1_CMD.GAMMA_PARM1.Data             = 0xA66F2C22;
    F_M_P->F0_ISP1_CMD.GAMMA_PARM2.Address          = 0xCCAA02E7;    F_M_P->F0_ISP1_CMD.GAMMA_PARM2.Data             = 0x4D6A2101;
    F_M_P->F0_ISP1_CMD.GAMMA_PARM3.Address          = 0xCCAA02EB;    F_M_P->F0_ISP1_CMD.GAMMA_PARM3.Data             = 0x280E1A00;
    F_M_P->F0_ISP1_CMD.GAMMA_DDR_Addr.Address       = 0xCCAA02F8;    F_M_P->F0_ISP1_CMD.GAMMA_DDR_Addr.Data          = FX_GAMMA_ADDR >> 5;
    F_M_P->F0_ISP1_CMD.RGB_offset.Address           = 0xCCAA02EC;    F_M_P->F0_ISP1_CMD.RGB_offset.Data              = ( (ISP1_RGB_Offset_O << 4) | ISP1_RGB_Offset_I);
    F_M_P->F0_ISP1_CMD.BN_Offset.Address            = 0xCCAA0205;    F_M_P->F0_ISP1_CMD.BN_Offset.Data               = 0x18;
    F_M_P->F0_ISP1_CMD.F_Offset.Address             = 0xCCAA0207;    F_M_P->F0_ISP1_CMD.F_Offset.Data                = 0x18;    //小圖:0x18   大圖:0x48
    F_M_P->F0_ISP1_CMD.S0_FXY_Offset.Address        = 0xCCAA02F3;    F_M_P->F0_ISP1_CMD.S0_FXY_Offset.Data           = 0x00010001;    //0x00010001 = 0xBXBYRXRY    Sensor_Binn
    F_M_P->F0_ISP1_CMD.S1_FXY_Offset.Address        = 0xCCAA02F4;    F_M_P->F0_ISP1_CMD.S1_FXY_Offset.Data           = 0x00010001;    //0x00010001 = 0xBXBYRXRY    Sensor_Binn
    F_M_P->F0_ISP1_CMD.S2_FXY_Offset.Address        = 0xCCAA02F5;    F_M_P->F0_ISP1_CMD.S2_FXY_Offset.Data           = 0x00010001;    //0x00010001 = 0xBXBYRXRY    Sensor_Binn
    F_M_P->F0_ISP1_CMD.S01_BXY_Offset.Address       = 0xCCAA02F6;    F_M_P->F0_ISP1_CMD.S01_BXY_Offset.Data          = 0x01010101;    //0x01010101 = 0xS1BYRY S0BYRY    FPGA_Binn
    F_M_P->F0_ISP1_CMD.S2_BXY_Offset.Address        = 0xCCAA02F7;    F_M_P->F0_ISP1_CMD.S2_BXY_Offset.Data           = 0x01010101;    //0x01010101 = 0x       S2BYRY    FPGA_Binn
    F_M_P->F0_ISP1_CMD.S0_B_DDR_Addr.Address        = 0xCCAA0202;    F_M_P->F0_ISP1_CMD.S0_B_DDR_Addr.Data           = (ISP1_STM2_T_P0_A_BUF_ADDR + 32) >> 5;
    F_M_P->F0_ISP1_CMD.S1_B_DDR_Addr.Address        = 0xCCAA0203;    F_M_P->F0_ISP1_CMD.S1_B_DDR_Addr.Data           = (ISP1_STM2_T_P0_B_BUF_ADDR + 32) >> 5;
    F_M_P->F0_ISP1_CMD.S2_B_DDR_Addr.Address        = 0xCCAA0204;    F_M_P->F0_ISP1_CMD.S2_B_DDR_Addr.Data           = (ISP1_STM2_T_P0_C_BUF_ADDR + 32) >> 5;
    F_M_P->F0_ISP1_CMD.S0_Binn_START.Address        = 0xCCAA02BC;    F_M_P->F0_ISP1_CMD.S0_Binn_START.Data           = 0x0;
    F_M_P->F0_ISP1_CMD.S1_Binn_START.Address        = 0xCCAA02BD;    F_M_P->F0_ISP1_CMD.S1_Binn_START.Data           = 0x0;
    F_M_P->F0_ISP1_CMD.S2_Binn_START.Address        = 0xCCAA02BE;    F_M_P->F0_ISP1_CMD.S2_Binn_START.Data           = 0x0;
    F_M_P->F0_ISP1_CMD.AWB_TH.Address               = 0xCCAA02ED;    F_M_P->F0_ISP1_CMD.AWB_TH.Data                  = 0x1001;  //0x2004;
    F_M_P->F0_ISP1_CMD.SA_WD_DDR_Addr.Address       = 0xCCAA0261;    F_M_P->F0_ISP1_CMD.SA_WD_DDR_Addr.Data          = ISP1_WD_P0_A_ADDR >> 5;
    F_M_P->F0_ISP1_CMD.SB_WD_DDR_Addr.Address       = 0xCCAA0262;    F_M_P->F0_ISP1_CMD.SB_WD_DDR_Addr.Data          = ISP1_WD_P0_B_ADDR >> 5;
    F_M_P->F0_ISP1_CMD.SC_WD_DDR_Addr.Address       = 0xCCAA0263;    F_M_P->F0_ISP1_CMD.SC_WD_DDR_Addr.Data          = ISP1_WD_P0_C_ADDR >> 5;
    F_M_P->F0_ISP1_CMD.SA_WB_DDR_Addr.Address       = 0xCCAA0264;    F_M_P->F0_ISP1_CMD.SA_WB_DDR_Addr.Data          = ISP1_WB_P0_A_ADDR >> 5;
    F_M_P->F0_ISP1_CMD.SB_WB_DDR_Addr.Address       = 0xCCAA0265;    F_M_P->F0_ISP1_CMD.SB_WB_DDR_Addr.Data          = ISP1_WB_P0_B_ADDR >> 5;
    F_M_P->F0_ISP1_CMD.SC_WB_DDR_Addr.Address       = 0xCCAA0266;    F_M_P->F0_ISP1_CMD.SC_WB_DDR_Addr.Data          = ISP1_WB_P0_C_ADDR >> 5;
    F_M_P->F0_ISP1_CMD.WDR_Start_En.Address         = 0xCCAA0260;    F_M_P->F0_ISP1_CMD.WDR_Start_En.Data            = 0;
    F_M_P->F0_ISP1_CMD.Mo_Mul.Address         		= 0xCCAA02EF;    F_M_P->F0_ISP1_CMD.Mo_Mul.Data            		 = 0;
    F_M_P->F0_ISP1_CMD.Mo_En.Address         		= 0xCCAA02F9;    F_M_P->F0_ISP1_CMD.Mo_En.Data            		 = 0;
    F_M_P->F0_ISP1_CMD.Noise_TH.Address         	= 0xCCAA02FA;    F_M_P->F0_ISP1_CMD.Noise_TH.Data            	 = 0;
    F_M_P->F0_ISP1_CMD.REV[0].Address               = 0xCCAA03F0;

    //============ ISP2 ==============
    F_M_P->F0_ISP2_CMD.Now_Offset.Address           = 0xCCAAA031;    F_M_P->F0_ISP2_CMD.Now_Offset.Data              = 0x0;
    F_M_P->F0_ISP2_CMD.Pre_Offset.Address           = 0xCCAAA032;    F_M_P->F0_ISP2_CMD.Pre_Offset.Data              = 0x0;
    F_M_P->F0_ISP2_CMD.Next_Offset.Address          = 0xCCAAA033;    F_M_P->F0_ISP2_CMD.Next_Offset.Data             = 0x0;
    F_M_P->F0_ISP2_CMD.SFull_Offset.Address         = 0xCCAAA034;    F_M_P->F0_ISP2_CMD.SFull_Offset.Data            = 0x0;
    F_M_P->F0_ISP2_CMD.SH_Mode.Address              = 0xCCAAA027;    F_M_P->F0_ISP2_CMD.SH_Mode.Data                 = 0x10;
    F_M_P->F0_ISP2_CMD.NR3D_Level.Address           = 0xCCAAA029;    F_M_P->F0_ISP2_CMD.NR3D_Level.Data              = 0x0;
    F_M_P->F0_ISP2_CMD.NR3D_Rate.Address            = 0xCCAAA02A;    F_M_P->F0_ISP2_CMD.NR3D_Rate.Data               = 0x00406070;    //0x81cd66b4;
    F_M_P->F0_ISP2_CMD.BW_EN.Address                = 0xCCAAA006;    F_M_P->F0_ISP2_CMD.BW_EN.Data                   = 0x0;
    F_M_P->F0_ISP2_CMD.Set_Y_RG_mulV.Address        = 0xCCAAA013;    F_M_P->F0_ISP2_CMD.Set_Y_RG_mulV.Data           = 0x10001000;    // (UG << 16) | UR
    F_M_P->F0_ISP2_CMD.Set_Y_B_mulV.Address         = 0xCCAAA014;    F_M_P->F0_ISP2_CMD.Set_Y_B_mulV.Data            = 0x10001000;    // (VR << 16) | UB
    F_M_P->F0_ISP2_CMD.Set_U_RG_mulV.Address        = 0xCCAAA015;    F_M_P->F0_ISP2_CMD.Set_U_RG_mulV.Data           = 0x874883b7;    // (UG << 16) | UR
    F_M_P->F0_ISP2_CMD.Set_UV_BR_mulV.Address       = 0xCCAAA016;    F_M_P->F0_ISP2_CMD.Set_UV_BR_mulV.Data          = 0x0b000b00;    // (VR << 16) | UB
    F_M_P->F0_ISP2_CMD.Set_V_GB_mulV.Address        = 0xCCAAA017;    F_M_P->F0_ISP2_CMD.Set_V_GB_mulV.Data           = 0x81c88937;    // (VB << 16) | VG
    F_M_P->F0_ISP2_CMD.Now_Addr_0.Address           = 0xCCAAA040;    F_M_P->F0_ISP2_CMD.Now_Addr_0.Data              = (ISP2_STM2_S_P0_BUF_ADDR >> 5);
    F_M_P->F0_ISP2_CMD.Pre_Addr_0.Address           = 0xCCAAA041;    F_M_P->F0_ISP2_CMD.Pre_Addr_0.Data              = (NR3D_STM2_A_BUF_ADDR >> 5);
    F_M_P->F0_ISP2_CMD.Next_Addr_0.Address          = 0xCCAAA042;    F_M_P->F0_ISP2_CMD.Next_Addr_0.Data             = (NR3D_STM2_A_BUF_ADDR >> 5);
    F_M_P->F0_ISP2_CMD.Sfull_Addr_0.Address         = 0xCCAAA043;    F_M_P->F0_ISP2_CMD.Sfull_Addr_0.Data            = (ISP2_STM2_T_P0_A_BUF_ADDR >> 5);
    F_M_P->F0_ISP2_CMD.Col_Size_0.Address           = 0xCCAAA044;    F_M_P->F0_ISP2_CMD.Col_Size_0.Data              = DS_ISP2_Column_A;
    F_M_P->F0_ISP2_CMD.Size_Y_0.Address             = 0xCCAAA045;    F_M_P->F0_ISP2_CMD.Size_Y_0.Data                = DS_ISP2_Line_A;
    F_M_P->F0_ISP2_CMD.RB_Offset_0.Address          = 0xCCAAA047;    F_M_P->F0_ISP2_CMD.RB_Offset_0.Data             = 0x18;    //小圖 : 0x18 , 大圖 0x48
    F_M_P->F0_ISP2_CMD.Now_Addr_1.Address           = 0xCCAAA050;    F_M_P->F0_ISP2_CMD.Now_Addr_1.Data              = 0;
    F_M_P->F0_ISP2_CMD.Pre_Addr_1.Address           = 0xCCAAA051;    F_M_P->F0_ISP2_CMD.Pre_Addr_1.Data              = 0;
    F_M_P->F0_ISP2_CMD.Next_Addr_1.Address          = 0xCCAAA052;    F_M_P->F0_ISP2_CMD.Next_Addr_1.Data             = 0;
    F_M_P->F0_ISP2_CMD.Sfull_Addr_1.Address         = 0xCCAAA053;    F_M_P->F0_ISP2_CMD.Sfull_Addr_1.Data            = 0;
    F_M_P->F0_ISP2_CMD.Col_Size_1.Address           = 0xCCAAA054;    F_M_P->F0_ISP2_CMD.Col_Size_1.Data              = 0;
    F_M_P->F0_ISP2_CMD.Size_Y_1.Address             = 0xCCAAA055;    F_M_P->F0_ISP2_CMD.Size_Y_1.Data                = 0;
    F_M_P->F0_ISP2_CMD.RB_Offset_1.Address          = 0xCCAAA057;    F_M_P->F0_ISP2_CMD.RB_Offset_1.Data             = 0;
    F_M_P->F0_ISP2_CMD.Now_Addr_2.Address           = 0xCCAAA060;    F_M_P->F0_ISP2_CMD.Now_Addr_2.Data              = 0;
    F_M_P->F0_ISP2_CMD.Pre_Addr_2.Address           = 0xCCAAA061;    F_M_P->F0_ISP2_CMD.Pre_Addr_2.Data              = 0;
    F_M_P->F0_ISP2_CMD.Next_Addr_2.Address          = 0xCCAAA062;    F_M_P->F0_ISP2_CMD.Next_Addr_2.Data             = 0;
    F_M_P->F0_ISP2_CMD.Sfull_Addr_2.Address         = 0xCCAAA063;    F_M_P->F0_ISP2_CMD.Sfull_Addr_2.Data            = 0;
    F_M_P->F0_ISP2_CMD.Col_Size_2.Address           = 0xCCAAA064;    F_M_P->F0_ISP2_CMD.Col_Size_2.Data              = 0;
    F_M_P->F0_ISP2_CMD.Size_Y_2.Address             = 0xCCAAA065;    F_M_P->F0_ISP2_CMD.Size_Y_2.Data                = 0;
    F_M_P->F0_ISP2_CMD.RB_Offset_2.Address          = 0xCCAAA067;    F_M_P->F0_ISP2_CMD.RB_Offset_2.Data             = 0;
    F_M_P->F0_ISP2_CMD.Start_En_0.Address           = 0xCCAAA046;    F_M_P->F0_ISP2_CMD.Start_En_0.Data              = ISP2_A_ENABLE;
    F_M_P->F0_ISP2_CMD.Start_En_1.Address           = 0xCCAAA056;    F_M_P->F0_ISP2_CMD.Start_En_1.Data              = 0;    //ISP2_B_ENABLE;
    F_M_P->F0_ISP2_CMD.Start_En_2.Address           = 0xCCAAA066;    F_M_P->F0_ISP2_CMD.Start_En_2.Data              = 0;    //ISP2_C_ENABLE;
    F_M_P->F0_ISP2_CMD.WDR_Offset.Address           = 0xCCAAA035;    F_M_P->F0_ISP2_CMD.WDR_Offset.Data              = 0x7C;
    F_M_P->F0_ISP2_CMD.BD_TH.Address                = 0xCCAAA018;    F_M_P->F0_ISP2_CMD.BD_TH.Data                   = 0;
    F_M_P->F0_ISP2_CMD.ND_Addr_0.Address            = 0xCCAAA04C;    F_M_P->F0_ISP2_CMD.ND_Addr_0.Data               = 0;
    F_M_P->F0_ISP2_CMD.PD_Addr_0.Address            = 0xCCAAA04D;    F_M_P->F0_ISP2_CMD.PD_Addr_0.Data               = 0;
    F_M_P->F0_ISP2_CMD.MD_Addr_0.Address            = 0xCCAAA04E;    F_M_P->F0_ISP2_CMD.MD_Addr_0.Data               = 0;
    F_M_P->F0_ISP2_CMD.ND_Addr_1.Address            = 0xCCAAA05C;    F_M_P->F0_ISP2_CMD.ND_Addr_1.Data               = 0;
    F_M_P->F0_ISP2_CMD.PD_Addr_1.Address            = 0xCCAAA05D;    F_M_P->F0_ISP2_CMD.PD_Addr_1.Data               = 0;
    F_M_P->F0_ISP2_CMD.MD_Addr_1.Address            = 0xCCAAA05E;    F_M_P->F0_ISP2_CMD.MD_Addr_1.Data               = 0;
    F_M_P->F0_ISP2_CMD.ND_Addr_2.Address            = 0xCCAAA06C;    F_M_P->F0_ISP2_CMD.ND_Addr_2.Data               = 0;
    F_M_P->F0_ISP2_CMD.PD_Addr_2.Address            = 0xCCAAA06D;    F_M_P->F0_ISP2_CMD.PD_Addr_2.Data               = 0;
    F_M_P->F0_ISP2_CMD.MD_Addr_2.Address            = 0xCCAAA06E;    F_M_P->F0_ISP2_CMD.MD_Addr_2.Data               = 0;
    F_M_P->F0_ISP2_CMD.OV_Mul.Address               = 0xCCAAA02B;    F_M_P->F0_ISP2_CMD.OV_Mul.Data                  = 11;		// = Overlay_Mul
    F_M_P->F0_ISP2_CMD.NR3D_Disable_0.Address       = 0xCCAAA04A;    F_M_P->F0_ISP2_CMD.NR3D_Disable_0.Data          = 0;
    F_M_P->F0_ISP2_CMD.NR3D_Disable_1.Address       = 0xCCAAA05A;    F_M_P->F0_ISP2_CMD.NR3D_Disable_1.Data          = 0;
    F_M_P->F0_ISP2_CMD.NR3D_Disable_2.Address       = 0xCCAAA06A;    F_M_P->F0_ISP2_CMD.NR3D_Disable_2.Data          = 0;
    F_M_P->F0_ISP2_CMD.Defect_Addr_0.Address        = 0xCCAAA049;    F_M_P->F0_ISP2_CMD.Defect_Addr_0.Data           = 0;			//壞點tbale addr: bit31=En  bit30~0=Addr
    F_M_P->F0_ISP2_CMD.Defect_Addr_1.Address        = 0xCCAAA059;    F_M_P->F0_ISP2_CMD.Defect_Addr_1.Data           = 0;			//壞點tbale addr: bit31=En  bit30~0=Addr
    F_M_P->F0_ISP2_CMD.Defect_Addr_2.Address        = 0xCCAAA069;    F_M_P->F0_ISP2_CMD.Defect_Addr_2.Data           = 0;			//壞點tbale addr: bit31=En  bit30~0=Addr
    F_M_P->F0_ISP2_CMD.Scaler_Addr_0.Address        = 0xCCAAA04B;    F_M_P->F0_ISP2_CMD.Scaler_Addr_0.Data           = 0;
    F_M_P->F0_ISP2_CMD.Scaler_Addr_1.Address        = 0xCCAAA05B;    F_M_P->F0_ISP2_CMD.Scaler_Addr_1.Data           = 0;
    F_M_P->F0_ISP2_CMD.Scaler_Addr_2.Address        = 0xCCAAA06B;    F_M_P->F0_ISP2_CMD.Scaler_Addr_2.Data           = 0;
    F_M_P->F0_ISP2_CMD.DG_Offset.Address        	= 0xCCAAA02C;    F_M_P->F0_ISP2_CMD.DG_Offset.Data           	 = 0;
    F_M_P->F0_ISP2_CMD.DG_TH.Address        		= 0xCCAAA02D;    F_M_P->F0_ISP2_CMD.DG_TH.Data           	 	 = 32;
    F_M_P->F0_ISP2_CMD.REV[0].Address               = 0xCCAA03F1;

    //============ Stitch ==============
    F_M_P->F0_Stitch_CMD.SML_XY_Offset1.Address     = 0xCCAA0140;    F_M_P->F0_Stitch_CMD.SML_XY_Offset1.Data        = ( (130 >> 1) << 16) | (138 >> 1);    //1 = 2 pix
    F_M_P->F0_Stitch_CMD.SML_XY_Offset2.Address     = 0xCCAA0141;    F_M_P->F0_Stitch_CMD.SML_XY_Offset2.Data        = ( (130 >> 1) << 16) | (138 >> 1);        //1 = 2 pix
    F_M_P->F0_Stitch_CMD.SML_XY_Offset3.Address     = 0xCCAA0142;    F_M_P->F0_Stitch_CMD.SML_XY_Offset3.Data        = ( (130 >> 1) << 16) | (138 >> 1);
    F_M_P->F0_Stitch_CMD.S_DDR_P0.Address           = 0xCCAA0101;    F_M_P->F0_Stitch_CMD.S_DDR_P0.Data              = (ST_STM2_P0_S_ADDR >> 5);
    F_M_P->F0_Stitch_CMD.Block_Size0.Address        = 0xCCAA0102;    F_M_P->F0_Stitch_CMD.Block_Size0.Data           = 0;    //DS_ST_BSIZE;
    F_M_P->F0_Stitch_CMD.Comm_P0.Address            = 0xCCAA0103;    F_M_P->F0_Stitch_CMD.Comm_P0.Data               = (FX_ST_CMD_ADDR >> 5);
    F_M_P->F0_Stitch_CMD.T_Offset_0.Address         = 0xCCAA0105;    F_M_P->F0_Stitch_CMD.T_Offset_0.Data            = 0x000;
    F_M_P->F0_Stitch_CMD.YUVC_P0.Address            = 0xCCAA0106;    F_M_P->F0_Stitch_CMD.YUVC_P0.Data               = (F0_ST_YUV_ADDR >> 5);
    F_M_P->F0_Stitch_CMD.S_MASK_1_P0.Address        = 0xCCAA0107;    F_M_P->F0_Stitch_CMD.S_MASK_1_P0.Data           = (547 << 16) | 727;
    F_M_P->F0_Stitch_CMD.S_MASK_2_P0.Address        = 0xCCAA0108;    F_M_P->F0_Stitch_CMD.S_MASK_2_P0.Data           = (547 << 16) | 727;
    F_M_P->F0_Stitch_CMD.S_MASK_3_P0.Address        = 0xCCAA0109;    F_M_P->F0_Stitch_CMD.S_MASK_3_P0.Data           = (547 << 16) | 727;
    F_M_P->F0_Stitch_CMD.CP_En0.Address             = 0xCCAA010A;    F_M_P->F0_Stitch_CMD.CP_En0.Data                = 0;
    F_M_P->F0_Stitch_CMD.Alpha_DDR_P0.Address       = 0xCCAA010B;    F_M_P->F0_Stitch_CMD.Alpha_DDR_P0.Data          = 0;
    F_M_P->F0_Stitch_CMD.S_DDR_P1.Address           = 0xCCAA0111;    F_M_P->F0_Stitch_CMD.S_DDR_P1.Data              = (ST_STM2_P0_S_ADDR >> 5);
    F_M_P->F0_Stitch_CMD.Block_Size1.Address        = 0xCCAA0112;    F_M_P->F0_Stitch_CMD.Block_Size1.Data           = 0;    //DS_ST_BSIZE;
    F_M_P->F0_Stitch_CMD.Comm_P1.Address            = 0xCCAA0113;    F_M_P->F0_Stitch_CMD.Comm_P1.Data               = (FX_ST_CMD_ADDR >> 5);
    F_M_P->F0_Stitch_CMD.T_Offset_1.Address         = 0xCCAA0115;    F_M_P->F0_Stitch_CMD.T_Offset_1.Data            = 0x000;
    F_M_P->F0_Stitch_CMD.YUVC_P1.Address            = 0xCCAA0116;    F_M_P->F0_Stitch_CMD.YUVC_P1.Data               = (F0_ST_YUV_ADDR >> 5);
    F_M_P->F0_Stitch_CMD.S_MASK_1_P1.Address        = 0xCCAA0117;    F_M_P->F0_Stitch_CMD.S_MASK_1_P1.Data           = (547 << 16) | 727;
    F_M_P->F0_Stitch_CMD.S_MASK_2_P1.Address        = 0xCCAA0118;    F_M_P->F0_Stitch_CMD.S_MASK_2_P1.Data           = (547 << 16) | 727;
    F_M_P->F0_Stitch_CMD.S_MASK_3_P1.Address        = 0xCCAA0119;    F_M_P->F0_Stitch_CMD.S_MASK_3_P1.Data           = (547 << 16) | 727;
    F_M_P->F0_Stitch_CMD.CP_En1.Address             = 0xCCAA011A;    F_M_P->F0_Stitch_CMD.CP_En1.Data                = 0;
    F_M_P->F0_Stitch_CMD.Alpha_DDR_P1.Address       = 0xCCAA011B;    F_M_P->F0_Stitch_CMD.Alpha_DDR_P1.Data          = 0;
    F_M_P->F0_Stitch_CMD.S_DDR_P2.Address           = 0xCCAA0121;    F_M_P->F0_Stitch_CMD.S_DDR_P2.Data              = 0;
    F_M_P->F0_Stitch_CMD.Block_Size2.Address        = 0xCCAA0122;    F_M_P->F0_Stitch_CMD.Block_Size2.Data           = 0;
    F_M_P->F0_Stitch_CMD.Comm_P2.Address            = 0xCCAA0123;    F_M_P->F0_Stitch_CMD.Comm_P2.Data               = 0;
    F_M_P->F0_Stitch_CMD.T_Offset_2.Address         = 0xCCAA0125;    F_M_P->F0_Stitch_CMD.T_Offset_2.Data            = 0;
    F_M_P->F0_Stitch_CMD.YUVC_P2.Address            = 0xCCAA0126;    F_M_P->F0_Stitch_CMD.YUVC_P2.Data               = 0;
    F_M_P->F0_Stitch_CMD.S_MASK_1_P2.Address        = 0xCCAA0127;    F_M_P->F0_Stitch_CMD.S_MASK_1_P2.Data           = 0;
    F_M_P->F0_Stitch_CMD.S_MASK_2_P2.Address        = 0xCCAA0128;    F_M_P->F0_Stitch_CMD.S_MASK_2_P2.Data           = 0;
    F_M_P->F0_Stitch_CMD.S_MASK_3_P2.Address        = 0xCCAA0129;    F_M_P->F0_Stitch_CMD.S_MASK_3_P2.Data           = 0;
    F_M_P->F0_Stitch_CMD.CP_En2.Address             = 0xCCAA012A;    F_M_P->F0_Stitch_CMD.CP_En2.Data                = 0;
    F_M_P->F0_Stitch_CMD.Alpha_DDR_P2.Address       = 0xCCAA012B;    F_M_P->F0_Stitch_CMD.Alpha_DDR_P2.Data          = 0;
    F_M_P->F0_Stitch_CMD.S_DDR_P3.Address           = 0xCCAA0131;    F_M_P->F0_Stitch_CMD.S_DDR_P3.Data              = 0;
    F_M_P->F0_Stitch_CMD.Block_Size3.Address        = 0xCCAA0132;    F_M_P->F0_Stitch_CMD.Block_Size3.Data           = 0;
    F_M_P->F0_Stitch_CMD.Comm_P3.Address            = 0xCCAA0133;    F_M_P->F0_Stitch_CMD.Comm_P3.Data               = 0;
    F_M_P->F0_Stitch_CMD.T_Offset_3.Address         = 0xCCAA0135;    F_M_P->F0_Stitch_CMD.T_Offset_3.Data            = 0;
    F_M_P->F0_Stitch_CMD.YUVC_P3.Address            = 0xCCAA0136;    F_M_P->F0_Stitch_CMD.YUVC_P3.Data               = 0;
    F_M_P->F0_Stitch_CMD.S_MASK_1_P3.Address        = 0xCCAA0137;    F_M_P->F0_Stitch_CMD.S_MASK_1_P3.Data           = 0;
    F_M_P->F0_Stitch_CMD.S_MASK_2_P3.Address        = 0xCCAA0138;    F_M_P->F0_Stitch_CMD.S_MASK_2_P3.Data           = 0;
    F_M_P->F0_Stitch_CMD.S_MASK_3_P3.Address        = 0xCCAA0139;    F_M_P->F0_Stitch_CMD.S_MASK_3_P3.Data           = 0;
    F_M_P->F0_Stitch_CMD.CP_En3.Address             = 0xCCAA013A;    F_M_P->F0_Stitch_CMD.CP_En3.Data                = 0;
    F_M_P->F0_Stitch_CMD.Alpha_DDR_P3.Address       = 0xCCAA013B;    F_M_P->F0_Stitch_CMD.Alpha_DDR_P3.Data          = 0;
    F_M_P->F0_Stitch_CMD.Start_En0.Address          = 0xCCAA0104;    F_M_P->F0_Stitch_CMD.Start_En0.Data             = 0;
    F_M_P->F0_Stitch_CMD.Start_En1.Address          = 0xCCAA0114;    F_M_P->F0_Stitch_CMD.Start_En1.Data             = 0;
    F_M_P->F0_Stitch_CMD.Start_En2.Address          = 0xCCAA0124;    F_M_P->F0_Stitch_CMD.Start_En2.Data             = 0;
    F_M_P->F0_Stitch_CMD.Start_En3.Address          = 0xCCAA0134;    F_M_P->F0_Stitch_CMD.Start_En3.Data             = 0;
    F_M_P->F0_Stitch_CMD.REV[0].Address             = 0xCCAA03F2;

    //============ Diffusion ==============
    int diff_Pix = 5;       // 擴散範圍
    int times = (1024*16/((2*diff_Pix + 1)*(2*diff_Pix + 1)));
/*
    F_M_P->F0_Diffusion_CMD.Start_En0.Address       = 0xCCAAC00D;    F_M_P->F0_Diffusion_CMD.Start_En0.Data          = 0x0;
    F_M_P->F0_Diffusion_CMD.S_DDR_P0.Address        = 0xCCAAC002;    F_M_P->F0_Diffusion_CMD.S_DDR_P0.Data           = FX_WDR_IMG_A_P0_ADDR >> 5;
    F_M_P->F0_Diffusion_CMD.T_DDR_P0.Address        = 0xCCAAC003;    F_M_P->F0_Diffusion_CMD.T_DDR_P0.Data           = FX_WDR_DIF_A_P0_ADDR >> 5;
    F_M_P->F0_Diffusion_CMD.Size_Y0.Address         = 0xCCAAC004;    F_M_P->F0_Diffusion_CMD.Size_Y0.Data            = 409;         // 180731 jay, 口頭更新
    F_M_P->F0_Diffusion_CMD.Size_X0.Address         = 0xCCAAC005;    F_M_P->F0_Diffusion_CMD.Size_X0.Data            = 544;
    F_M_P->F0_Diffusion_CMD.Diff_pix0.Address       = 0xCCAAC006;    F_M_P->F0_Diffusion_CMD.Diff_pix0.Data          = diff_Pix;
    F_M_P->F0_Diffusion_CMD.Divisor0.Address        = 0xCCAAC007;    F_M_P->F0_Diffusion_CMD.Divisor0.Data           = times;
    F_M_P->F0_Diffusion_CMD.Table_DDR_P0.Address    = 0xCCAAC008;    F_M_P->F0_Diffusion_CMD.Table_DDR_P0.Data       = FX_WDR_TABLE_ADDR >> 5;
    F_M_P->F0_Diffusion_CMD.Bin64x64_Flag_0.Address = 0xCCAAC009;    F_M_P->F0_Diffusion_CMD.Bin64x64_Flag_0.Data    = 0;
    F_M_P->F0_Diffusion_CMD.Start_En1.Address       = 0xCCAAC01D;    F_M_P->F0_Diffusion_CMD.Start_En1.Data          = 0x0;
    F_M_P->F0_Diffusion_CMD.S_DDR_P1.Address        = 0xCCAAC012;    F_M_P->F0_Diffusion_CMD.S_DDR_P1.Data           = FX_WDR_IMG_B_P0_ADDR >> 5;
    F_M_P->F0_Diffusion_CMD.T_DDR_P1.Address        = 0xCCAAC013;    F_M_P->F0_Diffusion_CMD.T_DDR_P1.Data           = FX_WDR_DIF_B_P0_ADDR >> 5;
    F_M_P->F0_Diffusion_CMD.Size_Y1.Address         = 0xCCAAC014;    F_M_P->F0_Diffusion_CMD.Size_Y1.Data            = 409;
    F_M_P->F0_Diffusion_CMD.Size_X1.Address         = 0xCCAAC015;    F_M_P->F0_Diffusion_CMD.Size_X1.Data            = 544;
    F_M_P->F0_Diffusion_CMD.Diff_pix1.Address       = 0xCCAAC016;    F_M_P->F0_Diffusion_CMD.Diff_pix1.Data          = diff_Pix;
    F_M_P->F0_Diffusion_CMD.Divisor1.Address        = 0xCCAAC017;    F_M_P->F0_Diffusion_CMD.Divisor1.Data           = times;
    F_M_P->F0_Diffusion_CMD.Table_DDR_P1.Address    = 0xCCAAC018;    F_M_P->F0_Diffusion_CMD.Table_DDR_P1.Data       = FX_WDR_TABLE_ADDR >> 5;
    F_M_P->F0_Diffusion_CMD.Bin64x64_Flag_1.Address = 0xCCAAC019;    F_M_P->F0_Diffusion_CMD.Bin64x64_Flag_1.Data    = 0;
    F_M_P->F0_Diffusion_CMD.Start_En2.Address       = 0xCCAAC02D;    F_M_P->F0_Diffusion_CMD.Start_En2.Data          = 0x0;
    F_M_P->F0_Diffusion_CMD.S_DDR_P2.Address        = 0xCCAAC022;    F_M_P->F0_Diffusion_CMD.S_DDR_P2.Data           = FX_WDR_IMG_C_P0_ADDR >> 5;
    F_M_P->F0_Diffusion_CMD.T_DDR_P2.Address        = 0xCCAAC023;    F_M_P->F0_Diffusion_CMD.T_DDR_P2.Data           = FX_WDR_DIF_C_P0_ADDR >> 5;
    F_M_P->F0_Diffusion_CMD.Size_Y2.Address         = 0xCCAAC024;    F_M_P->F0_Diffusion_CMD.Size_Y2.Data            = 409;
    F_M_P->F0_Diffusion_CMD.Size_X2.Address         = 0xCCAAC025;    F_M_P->F0_Diffusion_CMD.Size_X2.Data            = 544;
    F_M_P->F0_Diffusion_CMD.Diff_pix2.Address       = 0xCCAAC026;    F_M_P->F0_Diffusion_CMD.Diff_pix2.Data          = diff_Pix;
    F_M_P->F0_Diffusion_CMD.Divisor2.Address        = 0xCCAAC027;    F_M_P->F0_Diffusion_CMD.Divisor2.Data           = times;
    F_M_P->F0_Diffusion_CMD.Table_DDR_P2.Address    = 0xCCAAC028;    F_M_P->F0_Diffusion_CMD.Table_DDR_P2.Data       = FX_WDR_TABLE_ADDR >> 5;
    F_M_P->F0_Diffusion_CMD.Bin64x64_Flag_2.Address = 0xCCAAC029;    F_M_P->F0_Diffusion_CMD.Bin64x64_Flag_2.Data    = 0;
    F_M_P->F0_Diffusion_CMD.Start_En3.Address       = 0xCCAAC03D;    F_M_P->F0_Diffusion_CMD.Start_En3.Data          = 0x0;
    F_M_P->F0_Diffusion_CMD.S_DDR_P3.Address        = 0xCCAAC032;    F_M_P->F0_Diffusion_CMD.S_DDR_P3.Data           = FX_WDR_IMG_A_P0_ADDR >> 5;
    F_M_P->F0_Diffusion_CMD.T_DDR_P3.Address        = 0xCCAAC033;    F_M_P->F0_Diffusion_CMD.T_DDR_P3.Data           = FX_WDR_DIF_A_P0_ADDR >> 5;
    F_M_P->F0_Diffusion_CMD.Size_Y3.Address         = 0xCCAAC034;    F_M_P->F0_Diffusion_CMD.Size_Y3.Data            = 409;
    F_M_P->F0_Diffusion_CMD.Size_X3.Address         = 0xCCAAC035;    F_M_P->F0_Diffusion_CMD.Size_X3.Data            = 544;
    F_M_P->F0_Diffusion_CMD.Diff_pix3.Address       = 0xCCAAC036;    F_M_P->F0_Diffusion_CMD.Diff_pix3.Data          = diff_Pix;
    F_M_P->F0_Diffusion_CMD.Divisor3.Address        = 0xCCAAC037;    F_M_P->F0_Diffusion_CMD.Divisor3.Data           = times;
    F_M_P->F0_Diffusion_CMD.Table_DDR_P3.Address    = 0xCCAAC038;    F_M_P->F0_Diffusion_CMD.Table_DDR_P3.Data       = FX_WDR_TABLE_ADDR >> 5;
    F_M_P->F0_Diffusion_CMD.Bin64x64_Flag_3.Address = 0xCCAAC039;    F_M_P->F0_Diffusion_CMD.Bin64x64_Flag_3.Data    = 0;
    F_M_P->F0_Diffusion_CMD.Start_En4.Address       = 0xCCAAC04D;    F_M_P->F0_Diffusion_CMD.Start_En4.Data          = 0x0;
    F_M_P->F0_Diffusion_CMD.S_DDR_P4.Address        = 0xCCAAC042;    F_M_P->F0_Diffusion_CMD.S_DDR_P4.Data           = FX_WDR_IMG_B_P0_ADDR >> 5;
    F_M_P->F0_Diffusion_CMD.T_DDR_P4.Address        = 0xCCAAC043;    F_M_P->F0_Diffusion_CMD.T_DDR_P4.Data           = FX_WDR_DIF_B_P0_ADDR >> 5;
    F_M_P->F0_Diffusion_CMD.Size_Y4.Address         = 0xCCAAC044;    F_M_P->F0_Diffusion_CMD.Size_Y4.Data            = 409;
    F_M_P->F0_Diffusion_CMD.Size_X4.Address         = 0xCCAAC045;    F_M_P->F0_Diffusion_CMD.Size_X4.Data            = 544;
    F_M_P->F0_Diffusion_CMD.Diff_pix4.Address       = 0xCCAAC046;    F_M_P->F0_Diffusion_CMD.Diff_pix4.Data          = diff_Pix;
    F_M_P->F0_Diffusion_CMD.Divisor4.Address        = 0xCCAAC047;    F_M_P->F0_Diffusion_CMD.Divisor4.Data           = times;
    F_M_P->F0_Diffusion_CMD.Table_DDR_P4.Address    = 0xCCAAC048;    F_M_P->F0_Diffusion_CMD.Table_DDR_P4.Data       = FX_WDR_TABLE_ADDR >> 5;
    F_M_P->F0_Diffusion_CMD.Bin64x64_Flag_4.Address = 0xCCAAC049;    F_M_P->F0_Diffusion_CMD.Bin64x64_Flag_4.Data    = 0;
    F_M_P->F0_Diffusion_CMD.Start_En5.Address       = 0xCCAAC05D;    F_M_P->F0_Diffusion_CMD.Start_En5.Data          = 0x0;
    F_M_P->F0_Diffusion_CMD.S_DDR_P5.Address        = 0xCCAAC052;    F_M_P->F0_Diffusion_CMD.S_DDR_P5.Data           = FX_WDR_IMG_C_P0_ADDR >> 5;
    F_M_P->F0_Diffusion_CMD.T_DDR_P5.Address        = 0xCCAAC053;    F_M_P->F0_Diffusion_CMD.T_DDR_P5.Data           = FX_WDR_DIF_C_P0_ADDR >> 5;
    F_M_P->F0_Diffusion_CMD.Size_Y5.Address         = 0xCCAAC054;    F_M_P->F0_Diffusion_CMD.Size_Y5.Data            = 409;
    F_M_P->F0_Diffusion_CMD.Size_X5.Address         = 0xCCAAC055;    F_M_P->F0_Diffusion_CMD.Size_X5.Data            = 544;
    F_M_P->F0_Diffusion_CMD.Diff_pix5.Address       = 0xCCAAC056;    F_M_P->F0_Diffusion_CMD.Diff_pix5.Data          = diff_Pix;
    F_M_P->F0_Diffusion_CMD.Divisor5.Address        = 0xCCAAC057;    F_M_P->F0_Diffusion_CMD.Divisor5.Data           = times;
    F_M_P->F0_Diffusion_CMD.Table_DDR_P5.Address    = 0xCCAAC058;    F_M_P->F0_Diffusion_CMD.Table_DDR_P5.Data       = FX_WDR_TABLE_ADDR >> 5;
    F_M_P->F0_Diffusion_CMD.Bin64x64_Flag_5.Address = 0xCCAAC059;    F_M_P->F0_Diffusion_CMD.Bin64x64_Flag_5.Data    = 0;
    F_M_P->F0_Diffusion_CMD.Offset_Diff_P0.Address    = 0xCCAAC001;    F_M_P->F0_Diffusion_CMD.Offset_Diff_P0.Data       = 10;
    F_M_P->F0_Diffusion_CMD.Offset_Diff_P1.Address    = 0xCCAAC011;    F_M_P->F0_Diffusion_CMD.Offset_Diff_P1.Data       = 10;
    F_M_P->F0_Diffusion_CMD.Offset_Diff_P2.Address    = 0xCCAAC021;    F_M_P->F0_Diffusion_CMD.Offset_Diff_P2.Data       = 10;
    F_M_P->F0_Diffusion_CMD.Offset_Diff_P3.Address    = 0xCCAAC031;    F_M_P->F0_Diffusion_CMD.Offset_Diff_P2.Data       = 0;
    F_M_P->F0_Diffusion_CMD.Offset_Diff_P4.Address    = 0xCCAAC041;    F_M_P->F0_Diffusion_CMD.Offset_Diff_P2.Data       = 0;
    F_M_P->F0_Diffusion_CMD.Offset_Diff_P5.Address    = 0xCCAAC051;    F_M_P->F0_Diffusion_CMD.Offset_Diff_P2.Data       = 0;
    F_M_P->F0_Diffusion_CMD.REV[0].Address          = 0xCCAA03F3;
*/
    AS2_Diffusion_Address_Set(&F_M_P->F0_Diffusion_CMD);
    AS2_F1_DS_CMD_Init(F_M_P);

    //Page 1
    memcpy(&FX_MAIN_CMD_P[1], &FX_MAIN_CMD_P[0], sizeof(AS2_F0_MAIN_CMD_struct));
    F_M_P = &FX_MAIN_CMD_P[1];
    F_M_P->F0_Stitch_CMD.T_Offset_0.Address         = 0xCCAA0105;    F_M_P->F0_Stitch_CMD.T_Offset_0.Data            = 0x200;
    F_M_P->F1_Stitch_CMD.T_Offset_0.Address         = 0xCCAA0105;    F_M_P->F1_Stitch_CMD.T_Offset_0.Data            = 0x200;
}

void AS2_F1_DS_CMD_Init(AS2_F0_MAIN_CMD_struct *F_M_P) {
    int V_Start,H_Start;
    int TimeOut_CLK;
    int TimeOut_Time;
    int TimeOut_Value;
    int TimeOut_En;

    TimeOut_CLK   = 100;        // MHz
    TimeOut_Time  = 33333;      // ISP1_TIME_OUT; //us
    TimeOut_Value = 0x4000000 - TimeOut_CLK * TimeOut_Time;
    TimeOut_En    = 1;

    V_Start = AS2_S3_V_START;
    H_Start = AS2_S3_H_START;

    //============ ISP1 ==============
    memcpy(&F_M_P->F1_ISP1_CMD, &F_M_P->F0_ISP1_CMD, sizeof(F_M_P->F1_ISP1_CMD));
    F_M_P->F1_ISP1_CMD.S0_F_DDR_Addr.Address        = 0xCCAA0201;    F_M_P->F1_ISP1_CMD.S0_F_DDR_Addr.Data           = ISP1_STM1_T_P0_C_BUF_ADDR >> 5;
    F_M_P->F1_ISP1_CMD.S1_F_DDR_Addr.Address        = 0xCCAA0206;    F_M_P->F1_ISP1_CMD.S1_F_DDR_Addr.Data           = ISP1_STM1_T_P0_B_BUF_ADDR >> 5;
    F_M_P->F1_ISP1_CMD.S2_F_DDR_Addr.Address        = 0xCCAA020B;    F_M_P->F1_ISP1_CMD.S2_F_DDR_Addr.Data           = ISP1_STM1_T_P0_A_BUF_ADDR >> 5;
    F_M_P->F1_ISP1_CMD.S0_LC_DDR_Addr.Address       = 0xCCAA02E8;    F_M_P->F1_ISP1_CMD.S0_LC_DDR_Addr.Data          = (FX_LC_FS_C_BUF_Addr >> 5) + 0x80000000;
    F_M_P->F1_ISP1_CMD.S1_LC_DDR_Addr.Address       = 0xCCAA02E9;    F_M_P->F1_ISP1_CMD.S1_LC_DDR_Addr.Data          = (FX_LC_FS_B_BUF_Addr >> 5) + 0x80000000;
    F_M_P->F1_ISP1_CMD.S2_LC_DDR_Addr.Address       = 0xCCAA02EA;    F_M_P->F1_ISP1_CMD.S2_LC_DDR_Addr.Data          = (FX_LC_FS_A_BUF_Addr >> 5) + 0x80000000;
    F_M_P->F1_ISP1_CMD.S2_TimeOut_Set.Address       = 0x00000000;    F_M_P->F1_ISP1_CMD.S2_TimeOut_Set.Data          = ((TimeOut_En << 31) | TimeOut_Value);
    F_M_P->F1_ISP1_CMD.S0_B_DDR_Addr.Address        = 0xCCAA0202;    F_M_P->F1_ISP1_CMD.S0_B_DDR_Addr.Data           = ISP1_STM2_T_P0_C_BUF_ADDR;
    F_M_P->F1_ISP1_CMD.S1_B_DDR_Addr.Address        = 0xCCAA0203;    F_M_P->F1_ISP1_CMD.S1_B_DDR_Addr.Data           = ISP1_STM2_T_P0_B_BUF_ADDR;
    F_M_P->F1_ISP1_CMD.S2_B_DDR_Addr.Address        = 0xCCAA0204;    F_M_P->F1_ISP1_CMD.S2_B_DDR_Addr.Data           = ISP1_STM2_T_P0_A_BUF_ADDR;

    //============ ISP2 ==============
    memcpy(&F_M_P->F1_ISP2_CMD, &F_M_P->F0_ISP2_CMD, sizeof(F_M_P->F1_ISP2_CMD));
    F_M_P->F1_ISP2_CMD.Now_Addr_0.Address           = 0xCCAAA040;    F_M_P->F1_ISP2_CMD.Now_Addr_0.Data              = (ISP2_STM2_S_P1_BUF_ADDR >> 5);

    //============ Stitch ==============
    memcpy(&F_M_P->F1_Stitch_CMD, &F_M_P->F0_Stitch_CMD, sizeof(F_M_P->F1_Stitch_CMD));
    F_M_P->F1_Stitch_CMD.YUVC_P0.Address	    	= 0xCCAA0106;    F_M_P->F1_Stitch_CMD.YUVC_P0.Data	             = (F1_ST_YUV_ADDR >> 5);
    F_M_P->F1_Stitch_CMD.YUVC_P1.Address            = 0xCCAA0116;    F_M_P->F1_Stitch_CMD.YUVC_P1.Data               = (F1_ST_YUV_ADDR >> 5);

    //============ Diffusion ==============
    memcpy(&F_M_P->F1_Diffusion_CMD, &F_M_P->F0_Diffusion_CMD, sizeof(F_M_P->F1_Diffusion_CMD));
}

void AS2_F2_DS_CMD_Init() {
	int i, j, idx;
	AS2_F2_MAIN_CMD_struct *F_M_P;
	AS2_SPI_Cmd_struct SPI_Cmd_P;
	unsigned *uTX_B;
	unsigned *Pxx;
  	unsigned MSPI_D[10][3];
	int MSPI_SEL_D;

	memset(&F2_MAIN_CMD_P[0], 0, sizeof(F2_MAIN_CMD_P) );

		F_M_P = &F2_MAIN_CMD_P[0];

		//============ SMOOTH ==============

		F_M_P->SMOOTH_CMD.Size.Address            			= 0xCCAA03C1;           F_M_P->SMOOTH_CMD.Size.Data			= 0;	// (Y << 16 | (X / 192) )
		F_M_P->SMOOTH_CMD.S_DDR_P.Address            		= 0xCCAA03C2;           F_M_P->SMOOTH_CMD.S_DDR_P.Data   	= 0;
		F_M_P->SMOOTH_CMD.T_DDR_P.Address             		= 0xCCAA03C3;           F_M_P->SMOOTH_CMD.T_DDR_P.Data   	= 0;
		F_M_P->SMOOTH_CMD.Start_En.Address                	= 0xCCAA03C0;           F_M_P->SMOOTH_CMD.Start_En.Data   	= 0;
		F_M_P->SMOOTH_CMD.REV[0].Address                  	= 0xA5C35A3C;

		//============ DMA ==============

		F_M_P->DMA_CMD.DMA_CMD_DW0_0.Address            	= 0xCCAAB004;           F_M_P->DMA_CMD.DMA_CMD_DW0_0.Data	= 0;
		F_M_P->DMA_CMD.DMA_CMD_DW1_0.Address            	= 0xCCAAB005;           F_M_P->DMA_CMD.DMA_CMD_DW1_0.Data   = 0;
		F_M_P->DMA_CMD.DMA_CMD_DW2_0.Address             	= 0xCCAAB006;           F_M_P->DMA_CMD.DMA_CMD_DW2_0.Data   = 0;
		F_M_P->DMA_CMD.DMA_CMD_DW3_0.Address                = 0xCCAAB007;           F_M_P->DMA_CMD.DMA_CMD_DW3_0.Data   = 0;
		F_M_P->DMA_CMD.DMA_CMD_DW0_1.Address                = 0xCCAAB008;           F_M_P->DMA_CMD.DMA_CMD_DW0_1.Data   = 0;
		F_M_P->DMA_CMD.DMA_CMD_DW1_1.Address              	= 0xCCAAB009;           F_M_P->DMA_CMD.DMA_CMD_DW1_1.Data   = 0;
		F_M_P->DMA_CMD.DMA_CMD_DW2_1.Address              	= 0xCCAAB00A;           F_M_P->DMA_CMD.DMA_CMD_DW2_1.Data   = 0;
		F_M_P->DMA_CMD.DMA_CMD_DW3_1.Address              	= 0xCCAAB00B;           F_M_P->DMA_CMD.DMA_CMD_DW3_1.Data   = 0;
		F_M_P->DMA_CMD.DMA_CMD_DW0_2.Address              	= 0xCCAAB00C;           F_M_P->DMA_CMD.DMA_CMD_DW0_2.Data   = 0;
		F_M_P->DMA_CMD.DMA_CMD_DW1_2.Address              	= 0xCCAAB00D;           F_M_P->DMA_CMD.DMA_CMD_DW1_2.Data   = 0;
		F_M_P->DMA_CMD.DMA_CMD_DW2_2.Address                = 0xCCAAB00E;           F_M_P->DMA_CMD.DMA_CMD_DW2_2.Data   = 0;
		F_M_P->DMA_CMD.DMA_CMD_DW3_2.Address              	= 0xCCAAB00F;           F_M_P->DMA_CMD.DMA_CMD_DW3_2.Data   = 0;
		F_M_P->DMA_CMD.DMA_CMD_DW0_3.Address              	= 0xCCAAB010;           F_M_P->DMA_CMD.DMA_CMD_DW0_3.Data   = 0;
		F_M_P->DMA_CMD.DMA_CMD_DW1_3.Address                = 0xCCAAB011;           F_M_P->DMA_CMD.DMA_CMD_DW1_3.Data   = 0;
		F_M_P->DMA_CMD.DMA_CMD_DW2_3.Address              	= 0xCCAAB012;           F_M_P->DMA_CMD.DMA_CMD_DW2_3.Data   = 0;
		F_M_P->DMA_CMD.DMA_CMD_DW3_3.Address              	= 0xCCAAB013;           F_M_P->DMA_CMD.DMA_CMD_DW3_3.Data   = 0;
		F_M_P->DMA_CMD.REV[0].Address                  		= 0xA5C35A3C;

		//============ JPEG0 ==============
		F_M_P->JPEG0_CMD.R_DDR_ADDR_0.Address            	= 0xCCAA0181;           F_M_P->JPEG0_CMD.R_DDR_ADDR_0.Data         = (JPEG_STM2_P0_S_BUF_ADDR >> 5);//0x00000;
		F_M_P->JPEG0_CMD.W_DDR_ADDR_0.Address            	= 0xCCAA0182;           F_M_P->JPEG0_CMD.W_DDR_ADDR_0.Data         = (JPEG_STM2_P0_T_BUF_ADDR >> 5);//0x400000
		F_M_P->JPEG0_CMD.Hder_Size_0.Address             	= 0xCCAA0183;           F_M_P->JPEG0_CMD.Hder_Size_0.Data          = sizeof(J_Hder_Struct);
		F_M_P->JPEG0_CMD.Y_Size_0.Address                	= 0xCCAA0184;           F_M_P->JPEG0_CMD.Y_Size_0.Data             = F2_J_PIXEL_Y;
		F_M_P->JPEG0_CMD.X_Size_0.Address                	= 0xCCAA0185;           F_M_P->JPEG0_CMD.X_Size_0.Data             = F2_J_PIXEL_X;
		F_M_P->JPEG0_CMD.H_B_Addr_0.Address              	= 0xCCAA0187;           F_M_P->JPEG0_CMD.H_B_Addr_0.Data           = (JPEG_HEADER_4K_ADDR >> 5);
		F_M_P->JPEG0_CMD.H_B_Size_0.Address              	= 0xCCAA0188;           F_M_P->JPEG0_CMD.H_B_Size_0.Data           = (sizeof(J_Hder_Struct) << 3);
		F_M_P->JPEG0_CMD.NoPic_En_0.Address              	= 0xCCAA0189;           F_M_P->JPEG0_CMD.NoPic_En_0.Data           = 1;
		F_M_P->JPEG0_CMD.S_B_Addr_0.Address              	= 0xCCAA018A;           F_M_P->JPEG0_CMD.S_B_Addr_0.Data           = 0;
		F_M_P->JPEG0_CMD.S_B_Size_0.Address              	= 0xCCAA018B;           F_M_P->JPEG0_CMD.S_B_Size_0.Data           = 0;//64 << 5;
		F_M_P->JPEG0_CMD.S_B_En_0.Address                	= 0xCCAA018C;           F_M_P->JPEG0_CMD.S_B_En_0.Data             = 0;
		F_M_P->JPEG0_CMD.E_B_Addr_0.Address              	= 0xCCAA018D;           F_M_P->JPEG0_CMD.E_B_Addr_0.Data           = 0;
		F_M_P->JPEG0_CMD.E_B_Size_0.Address              	= 0xCCAA018E;           F_M_P->JPEG0_CMD.E_B_Size_0.Data           = 0;//63 << 5;
		F_M_P->JPEG0_CMD.E_B_En_0.Address                	= 0xCCAA018F;           F_M_P->JPEG0_CMD.E_B_En_0.Data             = 0;
		F_M_P->JPEG0_CMD.Page_sel_0.Address              	= 0xCCAA0190;           F_M_P->JPEG0_CMD.Page_sel_0.Data           = 0;
		F_M_P->JPEG0_CMD.Q_table_sel_0.Address              = 0xCCAA01C1;           F_M_P->JPEG0_CMD.Q_table_sel_0.Data        = 0;
		F_M_P->JPEG0_CMD.Start_En_0.Address              	= 0xCCAA0186;           F_M_P->JPEG0_CMD.Start_En_0.Data           = 0x1;
/*		F_M_P->JPEG0_CMD.R_DDR_ADDR_1.Address            	= 0xCCAA0191;           F_M_P->JPEG0_CMD.R_DDR_ADDR_1.Data         = (JPEG_STM2_P0_S_BUF_ADDR >> 5);//0x00000;
		F_M_P->JPEG0_CMD.W_DDR_ADDR_1.Address            	= 0xCCAA0192;           F_M_P->JPEG0_CMD.W_DDR_ADDR_1.Data         = (JPEG_STM2_P0_T_BUF_ADDR >> 5);//0x400000
		F_M_P->JPEG0_CMD.Hder_Size_1.Address             	= 0xCCAA0193;           F_M_P->JPEG0_CMD.Hder_Size_1.Data          = sizeof(J_Hder_Struct);
		F_M_P->JPEG0_CMD.Y_Size_1.Address                	= 0xCCAA0194;           F_M_P->JPEG0_CMD.Y_Size_1.Data             = F2_J_PIXEL_Y;
		F_M_P->JPEG0_CMD.X_Size_1.Address                	= 0xCCAA0195;           F_M_P->JPEG0_CMD.X_Size_1.Data             = F2_J_PIXEL_X;
		F_M_P->JPEG0_CMD.H_B_Addr_1.Address              	= 0xCCAA0197;           F_M_P->JPEG0_CMD.H_B_Addr_1.Data           = (JPEG_HEADER_4K_ADDR >> 5);
		F_M_P->JPEG0_CMD.H_B_Size_1.Address              	= 0xCCAA0198;           F_M_P->JPEG0_CMD.H_B_Size_1.Data           = (sizeof(J_Hder_Struct) << 3);
		F_M_P->JPEG0_CMD.NoPic_En_1.Address              	= 0xCCAA0199;           F_M_P->JPEG0_CMD.NoPic_En_1.Data           = 1;
		F_M_P->JPEG0_CMD.S_B_Addr_1.Address              	= 0xCCAA019A;           F_M_P->JPEG0_CMD.S_B_Addr_1.Data           = 0;
		F_M_P->JPEG0_CMD.S_B_Size_1.Address              	= 0xCCAA019B;           F_M_P->JPEG0_CMD.S_B_Size_1.Data           = 0;//64 << 5;
		F_M_P->JPEG0_CMD.S_B_En_1.Address                	= 0xCCAA019C;           F_M_P->JPEG0_CMD.S_B_En_1.Data             = 0;
		F_M_P->JPEG0_CMD.E_B_Addr_1.Address              	= 0xCCAA019D;           F_M_P->JPEG0_CMD.E_B_Addr_1.Data           = 0;
		F_M_P->JPEG0_CMD.E_B_Size_1.Address              	= 0xCCAA019E;           F_M_P->JPEG0_CMD.E_B_Size_1.Data           = 0;//63 << 5;
		F_M_P->JPEG0_CMD.E_B_En_1.Address                	= 0xCCAA019F;           F_M_P->JPEG0_CMD.E_B_En_1.Data             = 0;
		F_M_P->JPEG0_CMD.Page_sel_1.Address              	= 0xCCAA01A0;           F_M_P->JPEG0_CMD.Page_sel_1.Data           = 0;
		F_M_P->JPEG0_CMD.Q_table_sel_1.Address              = 0xCCAA01C2;           F_M_P->JPEG0_CMD.Q_table_sel_1.Data        = 0;
		F_M_P->JPEG0_CMD.Start_En_1.Address              	= 0xCCAA0196;           F_M_P->JPEG0_CMD.Start_En_1.Data           = 0;*/
		//F_M_P->JPEG0_CMD.REV[0].Address                  	= 0xA5C35A3C;

		//============ JPEG1 ==============
		F_M_P->JPEG1_CMD.R_DDR_ADDR_0.Address            	= 0xCCAA01A1;           F_M_P->JPEG1_CMD.R_DDR_ADDR_0.Data         = (JPEG_STM2_P0_S_BUF_ADDR >> 5);//0x00000;
		F_M_P->JPEG1_CMD.W_DDR_ADDR_0.Address            	= 0xCCAA01A2;           F_M_P->JPEG1_CMD.W_DDR_ADDR_0.Data         = (JPEG_STM2_P0_T_BUF_ADDR >> 5);//0x400000
		F_M_P->JPEG1_CMD.Hder_Size_0.Address             	= 0xCCAA01A3;           F_M_P->JPEG1_CMD.Hder_Size_0.Data          = sizeof(J_Hder_Struct);
		F_M_P->JPEG1_CMD.Y_Size_0.Address                	= 0xCCAA01A4;           F_M_P->JPEG1_CMD.Y_Size_0.Data             = F2_J_PIXEL_Y;
		F_M_P->JPEG1_CMD.X_Size_0.Address                	= 0xCCAA01A5;           F_M_P->JPEG1_CMD.X_Size_0.Data             = F2_J_PIXEL_X;
		F_M_P->JPEG1_CMD.H_B_Addr_0.Address              	= 0xCCAA01A7;           F_M_P->JPEG1_CMD.H_B_Addr_0.Data           = (JPEG_HEADER_4K_ADDR >> 5);
		F_M_P->JPEG1_CMD.H_B_Size_0.Address              	= 0xCCAA01A8;           F_M_P->JPEG1_CMD.H_B_Size_0.Data           = (sizeof(J_Hder_Struct) << 3);
		F_M_P->JPEG1_CMD.NoPic_En_0.Address              	= 0xCCAA01A9;           F_M_P->JPEG1_CMD.NoPic_En_0.Data           = 1;
		F_M_P->JPEG1_CMD.S_B_Addr_0.Address              	= 0xCCAA01AA;           F_M_P->JPEG1_CMD.S_B_Addr_0.Data           = 0;
		F_M_P->JPEG1_CMD.S_B_Size_0.Address              	= 0xCCAA01AB;           F_M_P->JPEG1_CMD.S_B_Size_0.Data           = 0;//64 << 5;
		F_M_P->JPEG1_CMD.S_B_En_0.Address                	= 0xCCAA01AC;           F_M_P->JPEG1_CMD.S_B_En_0.Data             = 0;
		F_M_P->JPEG1_CMD.E_B_Addr_0.Address              	= 0xCCAA01AD;           F_M_P->JPEG1_CMD.E_B_Addr_0.Data           = 0;
		F_M_P->JPEG1_CMD.E_B_Size_0.Address              	= 0xCCAA01AE;           F_M_P->JPEG1_CMD.E_B_Size_0.Data           = 0;//63 << 5;
		F_M_P->JPEG1_CMD.E_B_En_0.Address                	= 0xCCAA01AF;           F_M_P->JPEG1_CMD.E_B_En_0.Data             = 0;
		F_M_P->JPEG1_CMD.Page_sel_0.Address              	= 0xCCAA01B0;           F_M_P->JPEG1_CMD.Page_sel_0.Data           = 0;
		F_M_P->JPEG1_CMD.Q_table_sel_0.Address              = 0xCCAA01C3;           F_M_P->JPEG1_CMD.Q_table_sel_0.Data        = 0;
		F_M_P->JPEG1_CMD.Start_En_0.Address              	= 0xCCAA01A6;           F_M_P->JPEG1_CMD.Start_En_0.Data           = 0;
/*		F_M_P->JPEG1_CMD.R_DDR_ADDR_1.Address            	= 0xCCAA01B1;           F_M_P->JPEG1_CMD.R_DDR_ADDR_1.Data         = (JPEG_STM2_P0_S_BUF_ADDR >> 5);//0x00000;
		F_M_P->JPEG1_CMD.W_DDR_ADDR_1.Address            	= 0xCCAA01B2;           F_M_P->JPEG1_CMD.W_DDR_ADDR_1.Data         = (JPEG_STM2_P0_T_BUF_ADDR >> 5);//0x400000
		F_M_P->JPEG1_CMD.Hder_Size_1.Address             	= 0xCCAA01B3;           F_M_P->JPEG1_CMD.Hder_Size_1.Data          = sizeof(J_Hder_Struct);
		F_M_P->JPEG1_CMD.Y_Size_1.Address                	= 0xCCAA01B4;           F_M_P->JPEG1_CMD.Y_Size_1.Data             = F2_J_PIXEL_Y;
		F_M_P->JPEG1_CMD.X_Size_1.Address                	= 0xCCAA01B5;           F_M_P->JPEG1_CMD.X_Size_1.Data             = F2_J_PIXEL_X;
		F_M_P->JPEG1_CMD.H_B_Addr_1.Address              	= 0xCCAA01B7;           F_M_P->JPEG1_CMD.H_B_Addr_1.Data           = (JPEG_HEADER_4K_ADDR >> 5);
		F_M_P->JPEG1_CMD.H_B_Size_1.Address              	= 0xCCAA01B8;           F_M_P->JPEG1_CMD.H_B_Size_1.Data           = (sizeof(J_Hder_Struct) << 3);
		F_M_P->JPEG1_CMD.NoPic_En_1.Address              	= 0xCCAA01B9;           F_M_P->JPEG1_CMD.NoPic_En_1.Data           = 1;
		F_M_P->JPEG1_CMD.S_B_Addr_1.Address              	= 0xCCAA01BA;           F_M_P->JPEG1_CMD.S_B_Addr_1.Data           = 0;
		F_M_P->JPEG1_CMD.S_B_Size_1.Address              	= 0xCCAA01BB;           F_M_P->JPEG1_CMD.S_B_Size_1.Data           = 0;//64 << 5;
		F_M_P->JPEG1_CMD.S_B_En_1.Address                	= 0xCCAA01BC;           F_M_P->JPEG1_CMD.S_B_En_1.Data             = 0;
		F_M_P->JPEG1_CMD.E_B_Addr_1.Address              	= 0xCCAA01BD;           F_M_P->JPEG1_CMD.E_B_Addr_1.Data           = 0;
		F_M_P->JPEG1_CMD.E_B_Size_1.Address              	= 0xCCAA01BE;           F_M_P->JPEG1_CMD.E_B_Size_1.Data           = 0;//63 << 5;
		F_M_P->JPEG1_CMD.E_B_En_1.Address                	= 0xCCAA01BF;           F_M_P->JPEG1_CMD.E_B_En_1.Data             = 0;
		F_M_P->JPEG1_CMD.Page_sel_1.Address              	= 0xCCAA01C0;           F_M_P->JPEG1_CMD.Page_sel_1.Data           = 0;
		F_M_P->JPEG1_CMD.Q_table_sel_1.Address              = 0xCCAA01C4;           F_M_P->JPEG1_CMD.Q_table_sel_1.Data        = 0;
		F_M_P->JPEG1_CMD.Start_En_1.Address              	= 0xCCAA01B6;           F_M_P->JPEG1_CMD.Start_En_1.Data           = 0;*/
		//F_M_P->JPEG1_CMD.REV[0].Address                  	= 0xA5C35A3C;

		//============ H264 ==============
		F_M_P->H264_CMD.Reset_Record.Address  				= 0xCCAA8011;    		F_M_P->H264_CMD.Reset_Record.Data  			= 0x00000;				//Address = 0xCCAA8011: reset
		F_M_P->H264_CMD.S_Addr.Address        				= 0xCCAA8000;    		F_M_P->H264_CMD.S_Addr.Data        			= 0x5A0000;
		F_M_P->H264_CMD.SM_Addr.Address       				= 0xCCAA8001;    		F_M_P->H264_CMD.SM_Addr.Data       			= 0xA38000+16*0x400;	//須對齊DDR最左邊
		F_M_P->H264_CMD.T_Addr.Address        				= 0xCCAA8002;    		F_M_P->H264_CMD.T_Addr.Data        			= 0xA38000;				//須對齊DDR最左邊
		F_M_P->H264_CMD.TM_Addr.Address       				= 0xCCAA8003;    		F_M_P->H264_CMD.TM_Addr.Data       			= 0xA38000+16*0x400;	//須對齊DDR最左邊
		F_M_P->H264_CMD.Motion_Addr.Address   				= 0xCCAA8004;    		F_M_P->H264_CMD.Motion_Addr.Data   			= (H264_MOTION_BUF_ADDR >> 5);			//一塊垃圾記憶體, Size=X*Y/16
		F_M_P->H264_CMD.size_v.Address        				= 0xCCAA8005;    		F_M_P->H264_CMD.size_v.Data        			= 64;					//1024*2/32
		F_M_P->H264_CMD.size_h.Address        				= 0xCCAA8006;    		F_M_P->H264_CMD.size_h.Data        			= 128;					//2048*2/32
		F_M_P->H264_CMD.XY_Rate.Address       				= 0xCCAA8007;    		F_M_P->H264_CMD.XY_Rate.Data       			= 0x0000;
		F_M_P->H264_CMD.NewTempCheck.Address  				= 0xCCAA8008;    		F_M_P->H264_CMD.NewTempCheck.Data  			= 0x0000;
		F_M_P->H264_CMD.B_Mask_Addr.Address   				= 0xCCAA8009;    		F_M_P->H264_CMD.B_Mask_Addr.Data   			= 0x0000;

		F_M_P->H264_CMD.QSL.Address           				= 0xCCAA800B;    		F_M_P->H264_CMD.QSL.Data           			= 4;					//24/6=4	32/6=5
		F_M_P->H264_CMD.QML0.Address          				= 0xCCAA800A;    		F_M_P->H264_CMD.QML0.Data          			= 13107;				//24%6=table[0]	32%6=table[2]
		F_M_P->H264_CMD.DeQML0.Address        				= 0xCCAA800C;    		F_M_P->H264_CMD.DeQML0.Data        			= 10;					//24%6=table[0]	32%6=table[2]
		F_M_P->H264_CMD.QML1.Address          				= 0xCCAA800D;    		F_M_P->H264_CMD.QML1.Data          			= 8066;
		F_M_P->H264_CMD.DeQML1.Address        				= 0xCCAA800E;    		F_M_P->H264_CMD.DeQML1.Data       			= 13;
		F_M_P->H264_CMD.QML2.Address          				= 0xCCAA800F;    		F_M_P->H264_CMD.QML2.Data         			= 5423;
		F_M_P->H264_CMD.DeQML2.Address        				= 0xCCAA8010;    		F_M_P->H264_CMD.DeQML2.Data        			= 16;

		F_M_P->H264_CMD.Size_Y.Address        				= 0xCCAA8012;    		F_M_P->H264_CMD.Size_Y.Data        			= 64;					//1024*2/32
		F_M_P->H264_CMD.Size_X.Address        				= 0xCCAA8013;    		F_M_P->H264_CMD.Size_X.Data        			= 128;					//2048*2/32
		F_M_P->H264_CMD.I_P_Mode.Address      				= 0xCCAA8014;    		F_M_P->H264_CMD.I_P_Mode.Data      			= 0 & 0x000000FF;		//(Slice & 0x000000FF);	Slice:0:I_Frame 1:P_Frame
		F_M_P->H264_CMD.H264_en.Address       				= 0xCCAA8015;    		F_M_P->H264_CMD.H264_en.Data       			= 0x0000;
	    //F_M_P->H264_CMD.Add_Byte.Address      				= 0xCCAA8018;    		F_M_P->H264_CMD.Add_Byte.Data      			= 0x0000;
	    //F_M_P->H264_CMD.Add_Last_Byte.Address 				= 0xCCAA8019;    		F_M_P->H264_CMD.Add_Last_Byte.Data 			= 0x0000;
		F_M_P->H264_CMD.USB_Buffer_Sel.Address 				= 0xCCAA8017;    		F_M_P->H264_CMD.USB_Buffer_Sel.Data 		= 0x0000;				//0~3	USB:8~11
		F_M_P->H264_CMD.rev.Address       	  				= 0xCCAA03F5;    		F_M_P->H264_CMD.rev.Data           			= 0x0000;

		//============ USB ==============
		F_M_P->USB_CMD.S_DDR_ADDR_0.Address               	= 0xCCAA0301;			F_M_P->USB_CMD.S_DDR_ADDR_0.Data        	= (JPEG_STM2_P0_T_BUF_ADDR >> 5);
		F_M_P->USB_CMD.JPEG_SEL_0.Address               	= 0xCCAA0302;			F_M_P->USB_CMD.JPEG_SEL_0.Data          	= 0;
		F_M_P->USB_CMD.Start_En_0.Address               	= 0xCCAA0303;			F_M_P->USB_CMD.Start_En_0.Data              = 1;
/*		F_M_P->USB_CMD.S_DDR_ADDR_1.Address               	= 0xCCAA0311;			F_M_P->USB_CMD.S_DDR_ADDR_1.Data        	= 0;
		F_M_P->USB_CMD.JPEG_SEL_1.Address               	= 0xCCAA0312;			F_M_P->USB_CMD.JPEG_SEL_1.Data          	= 0;
		F_M_P->USB_CMD.Start_En_1.Address               	= 0xCCAA0313;			F_M_P->USB_CMD.Start_En_1.Data              = 0;*/
		F_M_P->USB_CMD.REV[0].Address                  		= 0xA5C35A3C;


		//Page 1
		memcpy(&F2_MAIN_CMD_P[1],&F2_MAIN_CMD_P[0],sizeof(AS2_F2_MAIN_CMD_struct));
		F_M_P = &F2_MAIN_CMD_P[1];
		F_M_P->JPEG0_CMD.R_DDR_ADDR_0.Address            	= 0xCCAA0181;           F_M_P->JPEG0_CMD.R_DDR_ADDR_0.Data         = JPEG_STM2_P1_S_BUF_ADDR >> 5;
		F_M_P->JPEG0_CMD.W_DDR_ADDR_0.Address            	= 0xCCAA0182;           F_M_P->JPEG0_CMD.W_DDR_ADDR_0.Data         = JPEG_STM2_P1_T_BUF_ADDR >> 5;
	//	F_M_P->USB_P.S_DDR_ADDR_0.Address               = 0xCCAA0301;           F_M_P->USB_P.S_DDR_ADDR_0.Data          = (DS_USB_BUF_ADDR >> 5) + 0x60000 ;

		//F_M_P->JPEG_P.R_DDR_ADDR_1.Address            	= 0xCCAA0191;           F_M_P->JPEG_P.R_DDR_ADDR_1.Data         = (JPEG_STM2_P0_S_BUF_ADDR >> 5) + 0x200;
		F_M_P->JPEG0_CMD.W_DDR_ADDR_1.Address            	= 0xCCAA0192;           F_M_P->JPEG0_CMD.W_DDR_ADDR_1.Data         = JPEG_STM2_P1_T_BUF_ADDR >> 5;
	//	F_M_P->USB_P.S_DDR_ADDR_1.Address               = 0xCCAA0311;           F_M_P->USB_P.S_DDR_ADDR_1.Data          = (DS_USB_BUF_S2_ADDR >> 5) + 0x18000;
}

void AS2MainCMDInit() {
	MainCmdInit();
    AS2_F0_DS_CMD_Init();
    AS2_F2_DS_CMD_Init();
    do_sc_cmd_Tx_func("S2F0", "CMDP");      // 更新資料到kernel
    do_sc_cmd_Tx_func("S2F2", "CMDP");
}

void MainCmdStart() {
	AS2_CMD_Start();
}