/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/
 
#include "Device/US363/Cmd/Smooth.h"
 
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

#include "Device/US363/us363_para.h"
#include "Device/US363/Cmd/us360_func.h"
#include "Device/US363/Cmd/us360_define.h"
#include "Device/US363/Cmd/AletaS2_CMD_Struct.h"
//#ifdef ANDROID_CODE
#include "Device/US363/Cmd/spi_cmd.h"
#include "Device/US363/Test/test.h"
#include "Device/US363/Cmd/fpga_driver.h"
//  #include "us360.h"
//#else
#include "Device/US363/Kernel/FPGA_Pipe.h"
//#endif
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::Smooth"

struct Smooth_Com_Struct Smooth_Com;

struct Smooth_Z_I_Struct Smooth_Z_V_I[6][512][32];
struct Smooth_Z_I_Struct Smooth_Z_H_I[6][512][32];

int  Smooth_YUV_Temp[3][512][2];
int  Smooth_YUV_Data[6][3][512][2]; // Smooth_YUV_Data[Mode][YUV][Idx][Top/Bottom]
int  Smooth_I_Weight[6][67][512];   // V: 0~31  H: 32~63
int  Smooth_I_Data[6][67][512];
int  Smooth_I_Temp[64][512];
int  Smooth_T_Temp[6][70][512];     // Smooth Transfer Temp
int  Smooth_T_Temp_En = 0;
int  Smooth_I_Buf[6][70][512];      // Smooth_I_Buf[Mode][0~63:ZD  64:Y 65:U 66:V 67:ZP 68:ZV 69:Manual][Idx]
int  Smooth_O_Buf[6][4][4][512];    // Smooth_O_Buf[Mode][Cnt][YUVZ][Idx]
int  Smooth_Debug_Buf[6][3][512];
int  Smooth_Z_Temp_Buf[512];
struct Smooth_O_Struct  Smooth_O[2][6][512];        //Smooth_O[F_ID][Mode][Idx]

int  Smooth_Init = -1;              // 0xaaaa6666



// 0~63:ZD  64:Y 65:U 66:V 67:ZP 68:ZV 69:Manual

void Do_Manual_Smooth_Z(int M_Mode)
{
  float Global_Phi;
  short Global_temp_phi;
  float Global_Phi_rate;
  int i,j;
  int I3_Sum;
  struct Adjust_Line_I3_Struct *I3_p0;

  I3_Sum = A_L_I3_Header[M_Mode].Sum;

  for (j = 0; j < I3_Sum; j++) {
      I3_p0 = &A_L_I3[M_Mode][j];
      Global_temp_phi = 0x4000 - I3_p0->G_t_p.phi;
      Global_Phi_rate = 0.0;
      if (Global_temp_phi > 0x4000) {
          Global_temp_phi = 0x8000 - Global_temp_phi;
          if(Global_temp_phi < 0xD00)       Global_Phi = Smooth_Com.Global_phi_Btm;
          else if(Global_temp_phi > 0x3000) Global_Phi = Smooth_Com.Global_phi_Mid;
            else {
            Global_Phi_rate = (float)(Global_temp_phi - 0xD00)/ (0x3000 - 0xD00);
            Global_Phi = Global_Phi_rate * Smooth_Com.Global_phi_Mid + (1-Global_Phi_rate) * Smooth_Com.Global_phi_Btm;
          }
      }
      else {
          if(Global_temp_phi < 0x2000)       Global_Phi = Smooth_Com.Global_phi_Top;
          else if(Global_temp_phi > 0x3000)  Global_Phi = Smooth_Com.Global_phi_Mid;
            else {
              Global_Phi_rate = (float)(Global_temp_phi - 0x2000)/ (0x3000 - 0x2000);
              Global_Phi = Global_Phi_rate * Smooth_Com.Global_phi_Mid + (1-Global_Phi_rate) * Smooth_Com.Global_phi_Top;
            }
      }
      Smooth_I_Buf[M_Mode][69][j] = Global_Phi;
  }
}

int ColorST_SW = 1, AutoST_SW = 1;
int Smooth_YUV_Rate = 128;
int Smooth_D_Max = 63;
void Do_Auto_Smooth_YUV(int M_Mode)
{
    int i,j,k,L,M,M2;
    int I3_Sum, I3_Smooth_Sum, I3_Smooth_Total;
    int Total;
//    struct Adjust_Line_I3_Struct *I3_p0;
    int Smooth_D, Smooth_V, Smooth_P;
    int Min_V, Min_P;
    int Z_Value;

    I3_Sum = A_L_I3_Header[M_Mode].Sum;

#ifndef ANDROID_CODE
    for (i = 0; i <= 2; i++) {
        for (j = 0; j < I3_Sum; j++) {
            for (k = 0; k < 2; k++) Smooth_YUV_Data[M_Mode][i][j][k] = 0;
        }
        Smooth_YUV_Data[M_Mode][i][5][0] = 100;
    }
#endif
    int Len;
    unsigned Data_Sum, Data_Total;
    int I3_S_Idx, I3_TB_Idx;
    int I3_S_Idx0, I3_TB_Idx0;
    struct Adjust_Line_S3_Struct *S3_p;
    struct Adjust_Line_I3_Line_Struct *I3_Lp, *I3_Lp0, *I3_Lp1;
    int i2, AVG; int P0_Data, P1_Data;
    int Y_Sum;

    for (L = 0; L <= 2; L++) {                      // Y、U、V, 3種數值
        for (i = 1; i < 5; i++) {                   // 5顆sensor
            S3_p = &A_L_S3[M_Mode][i];
            Len = (S3_p->Sum >> 4);                                                                        //S3_p->Sum / 16;
            for (j = 0; j < S3_p->Sum; j++) {
                Data_Sum   = 0;
                Data_Total = 0;
                for (k = -Len; k <= Len; k++) {
                    M = j + k; M2 = (M + S3_p->Sum) % S3_p->Sum;
                    if ((M >= 0) && (M < S3_p->Sum)) {
                        I3_S_Idx  = S3_p->Source_Idx[M2];
                        I3_TB_Idx = S3_p->Top_Bottom[M2];
                        Data_Sum   += Smooth_YUV_Data[M_Mode][L][I3_S_Idx][I3_TB_Idx];
                        Data_Total ++;
                    }
                }
                I3_S_Idx0  = S3_p->Source_Idx[j];
                I3_TB_Idx0 = S3_p->Top_Bottom[j];

                if (Data_Total > 0) Smooth_YUV_Temp[L][I3_S_Idx0][I3_TB_Idx0] = Data_Sum / Data_Total;
                else                Smooth_YUV_Temp[L][I3_S_Idx0][I3_TB_Idx0] = 0;
            }
        }

        int Point;
        for (i = 0; i < 4; i++) {                   // 取水平縫合線, 算平均
            I3_Lp = &A_L_I3_Line[i+4];
            Len = (I3_Lp->SL_Sum[M_Mode] >> 2);
            Point = I3_Lp->SL_Point[M_Mode];
            for (j = 0; j < I3_Lp->SL_Sum[M_Mode]; j++) {
                Data_Sum   = 0;
                Data_Total = 0;
                for (k = -Len; k <= Len; k++) {
                    M = j + k;
                    if ((M >= 0) && (M < I3_Lp->SL_Sum[M_Mode])) {
                        Data_Sum   += Smooth_YUV_Data[M_Mode][L][M + Point][0];
                        Data_Total ++;
                    }
                }
                if (Data_Total > 0) Smooth_YUV_Temp[L][j + Point][0] = Data_Sum / Data_Total;
                else                Smooth_YUV_Temp[L][j + Point][0] = 0;
                Data_Sum   = 0;
                Data_Total = 0;
                for (k = -Len; k <= Len; k++) {
                    M = j + k;
                    if ((M >= 0) && (M < I3_Lp->SL_Sum[M_Mode])) {
                        Data_Sum   += Smooth_YUV_Data[M_Mode][L][M + Point][1];
                        Data_Total ++;
                    }
                }
                if (Data_Total > 0) Smooth_YUV_Temp[L][j + Point][1] = Data_Sum / Data_Total;
                else                Smooth_YUV_Temp[L][j + Point][1] = 0;
            }
        }
//        for (i = 0; i < 4; i++) {                   // 取垂直縫合線(0,2,4,6), 算平均
//            I3_Lp = & A_L_I3_Line[i<<1];
//            Point = I3_Lp->SL_Point[M_Mode];
//            for (j = 0; j < I3_Lp->SL_Sum[M_Mode]; j++) {
//                Smooth_YUV_Temp[L][j + Point][0] = Smooth_YUV_Data[M_Mode][L][j + Point][0];
//                Smooth_YUV_Temp[L][j + Point][1] = Smooth_YUV_Data[M_Mode][L][j + Point][1];
//            }
//        }

        for (j = 0; j < I3_Sum; j++) {
            Smooth_I_Buf[M_Mode][64+L][j] = (Smooth_YUV_Temp[L][j][0] - Smooth_YUV_Temp[L][j][1]) / Smooth_YUV_Rate;
        }
    }    //L

}

void setSmoothParameter(int idx, int value)
{
    switch(idx) {
    case 0: Smooth_Com.Smooth_Debug_Flag    = value; break;         // CMD=86，開啟Debug模式
    case 1: Smooth_Com.Smooth_Base_Level    = value; break;         // CMD=94
    case 2: Smooth_Com.Smooth_Avg_Weight    = value; break;         // CMD=87，平均加權比例
    case 3: Smooth_Com.Smooth_Auto_Rate     = value; break;         // UI設定，自動縫合比例，(手動)0~100(全自動)
    case 4: Smooth_Com.Smooth_Manual_Weight = value; break;         // CMD=93
    case 5: Smooth_Com.Smooth_Low_Level     = value; break;         // CMD=96，縫合點門檻
    case 6: Smooth_Com.Smooth_Weight_Th     = value; break;         // CMD=95，強度門檻
    }
    set_A2K_JPEG_Smooth_Auto(Smooth_Com.Smooth_Auto_Rate);
}

void getSmoothParameter(int *val)
{
	*val      = Smooth_Com.Smooth_Debug_Flag;       // CMD=86
    *(val+1)  = Smooth_Com.Smooth_Base_Level;
    *(val+2)  = Smooth_Com.Smooth_Avg_Weight;       // CMD=87
    *(val+3)  = Smooth_Com.Smooth_Auto_Rate;
    *(val+4)  = Smooth_Com.Smooth_Manual_Weight;
    *(val+5)  = Smooth_Com.Smooth_Low_Level;
    *(val+6)  = Smooth_Com.Smooth_Weight_Th;
}

void clean_Smooth_T_Temp(void)
{
    if(Smooth_T_Temp_En == 1){
        Smooth_T_Temp_En = -1;
        memset(Smooth_T_Temp, 0, sizeof(Smooth_T_Temp));
    }
}
void Smooth_Line_Trans_One(int S_Mode, int S_Start, int S_Stop, int T_Mode, int T_Start, int T_Stop)
{
    int i,i2, j, k;
    struct Smooth_O_Struct *p_S, *p_T;
    int S_Len, T_Len;
    int P, Sub;
    int P0, P1, P2;
    int I3_p_SP0, I3_p_SP1;
    int Scale;  int Gap;
    int s3en = chk_Line_YUV_Offset_Step_S3();
    int run_big_smooth = get_run_big_smooth();
    int skip_j = (run_big_smooth == 1 && s3en > 0)? 3: 0;

    S_Len = S_Stop - S_Start;
    T_Len = T_Stop - T_Start;

    Scale = T_Len / S_Len;
    Gap =   (T_Len - S_Len * Scale) / 2;
    for (i = 0; i <= T_Len; i++) {
        i2 = i - Gap;
        P  = i2 * 256 / Scale ;
        if (P < 0) P2 = 0;
        else       P2 = P;
        P0 = P2 / 256;
        if (P0  > S_Len) P0 = S_Len;
        P1 = P0+1;
        if (P1  > S_Len) P1 = S_Len;
        Sub = P2 & 0xff;
        for (j = skip_j; j < 4; j++) {        //YUVZ
            I3_p_SP0 = Smooth_O_Buf[S_Mode][3][j][P0 + S_Start];
            I3_p_SP1 = Smooth_O_Buf[S_Mode][3][j][P1 + S_Start];
            Smooth_O_Buf[T_Mode][3][j][i+T_Start] = (I3_p_SP0 * (256 - Sub) + I3_p_SP1 * Sub) / 256;
        }
        // rex+ 181023
        //Smooth_O_Buf[6][4][4][512]
        //Smooth_I_Buf[6][70][512]
        for(k = 0; k < 64; k++){
            I3_p_SP0 = Smooth_I_Buf[S_Mode][k][P0 + S_Start];
            I3_p_SP1 = Smooth_I_Buf[S_Mode][k][P1 + S_Start];
            Smooth_T_Temp[T_Mode][k][i+T_Start] = (I3_p_SP0 * (256 - Sub) + I3_p_SP1 * Sub) / 256;
        }
    }
    Smooth_T_Temp_En = 1;
}

void Smooth_Line_Trans(int S_Mode, int T_Mode)
{
    int i, j, k;
    int S_Start;
    int S_Stop;
    int T_Start;
    int T_Stop;
    // for smooth +- 2 point
    for (i = 0; i < 4; i++) {        //垂直縫合線
        S_Start = A_L_I3_Line[i].SL_Point[S_Mode];
        S_Stop  = A_L_I3_Line[i + 1].SL_Point[S_Mode] - 1;
        T_Start = A_L_I3_Line[i].SL_Point[T_Mode];
        T_Stop  = A_L_I3_Line[i + 1].SL_Point[T_Mode] - 1;
        Smooth_Line_Trans_One( S_Mode, S_Start, S_Stop, T_Mode, T_Start, T_Stop);
    }
    for (i = 4; i < 8; i++) {        //水平縫合線
        S_Start = A_L_I3_Line[i].SL_Point[S_Mode];
        S_Stop  = A_L_I3_Line[i].SL_Point[S_Mode] + A_L_I3_Line[i].SL_Sum[S_Mode] - 1;
        T_Start = A_L_I3_Line[i].SL_Point[T_Mode];
        T_Stop  = A_L_I3_Line[i].SL_Point[T_Mode] + A_L_I3_Line[i].SL_Sum[T_Mode] - 1;
        Smooth_Line_Trans_One( S_Mode, S_Start, S_Stop, T_Mode, T_Start, T_Stop);
    }
}
// rex+ 180927, 大圖縫合參數
int BSmooth_Set = 0;            // 0:Normal, 1:若有新設定，不要再參考舊資料加權
int BSmooth_XY_Space = 1;       // 2; 擴散範圍，4K=2，12K=6
int BSmooth_FarWeight = 3;      // 中央強度加權
int BSmooth_DelSlope = 3640;    // 2184;	//1092;	// 728(會裂開) // 1200(會扭曲), 1360;
int BSmooth_Function = 4;       // 縫合參數演算法選擇 0:舊的 1:第一版 4:加入小圖轉大圖參數

int getBSmooth_Function(void)
{
    return BSmooth_Function; 
}
int Smooth_I_AVG[3][512][64];   // 記錄每個縫合點的高度, [0][0:511][0]=代表高度Y
                                //                       [0][0:511][1]=maxP
                                //                       [0][0:511][2]=maxV
                                //                       [1][0:511][0:63]=距離總和
                                //                       [2][0:511][0:63]=數量

void Make_Smooth_I_Buf(int M_Mode)
{
	int i, j, k, py;
    int I3_Sum;
    int Temp_Sum;
    int Smooth_P, Smooth_V, Weight;
    struct Adjust_Line_I3_Struct *I3_p0;
    int d_cnt = read_F_Com_In_Capture_D_Cnt();

    I3_Sum = A_L_I3_Header[M_Mode].Sum;
    if(I3_Sum > 512) I3_Sum = 512;

    // for smooth +- 2 point
    for (i = 0; i < 64; i++) {                  // 64個距離相似數值(水平32+垂直32)
        I3_p0 = &A_L_I3[M_Mode][0];
        for (j = 0; j < I3_Sum; j++, I3_p0++) {          // 全部縫合點
            Temp_Sum = 0;
            for (k = 0; k < I3_p0->Smooth_Total; k++) {
              Smooth_P = I3_p0->Smooth_P[k];    // 縫合點
              Smooth_V = I3_p0->Smooth_V[k];    // 相似度0~1023

              Weight = Smooth_I_Weight[M_Mode][i][Smooth_P] ;
              if (Weight > 64) Weight = 64;
              Temp_Sum += Smooth_I_Data[M_Mode][i][Smooth_P] * Smooth_V * Weight / 256;     // 避免overflow
            }
            Smooth_I_Temp[i][j] = Temp_Sum / I3_p0->Smooth_Sum;     // (不像)0 ~ 255(像)

            //if(d_cnt != 0){
                py = (I3_p0->G_t_p.phi) * 64 / 1092;                    // 計算相同高度(y)的平均距離
                if(py > 960){ py = 960 + (3840 - py); }                 // .java 算式
                else        { py = 960 - py; }
                if(i == 0) Smooth_I_AVG[0][j][0] = py;
            //}
        }
     }

     int Len; int Total, Avg, i2;
     int Sum[512];
     Len = 4;
     for (j = 0; j < I3_Sum; j++) {
        for (i = 0; i < 32; i++) {
            Smooth_I_Buf[M_Mode][i*2  ][j] = (Smooth_I_Temp[i][j] + Smooth_I_Temp[i+32][j]) >> 1;
            Smooth_I_Buf[M_Mode][i*2+1][j] = (Smooth_I_Temp[i][j] + Smooth_I_Temp[i+32][j]) >> 1;
        }

        Sum[j] = 0;
        for (i = 0; i < 64; i++) Sum[j] += Smooth_I_Buf[M_Mode][i][j];      // Smooth_I_Buf = 0~255
        Sum[j] += 256;
     }
     
     for (j = 0; j < I3_Sum; j++){      // 最多512個縫合點
        for (i = 0; i < 64; i++){       // 64個距離相似數值(水平32+垂直32)
            Smooth_I_Buf[M_Mode][i][j] = Smooth_I_Buf[M_Mode][i][j] * 128 * 64 / Sum[j];    // 轉換為倍率，中間值128
        }
     }
}

int Smooth_YUV_Speed = 128, Smooth_Z_Speed = 64;	//32;
void setSmoothSpeed(int idx, int value)
{
    if(idx == 0)
        Smooth_YUV_Speed = value;
    else
        Smooth_Z_Speed   = value;
}

void getSmoothSpeed(int *val)
{
    *val      = Smooth_YUV_Speed;
    *(val+1)  = Smooth_Z_Speed;
}

int Smooth_Speed_Mode = 0;
void Set_Smooth_Speed_Mode(int mode)
{
    Smooth_Speed_Mode = mode;
}

int Get_Smooth_Speed_Mode(void)
{
    return Smooth_Speed_Mode;
}

int Smooth_YUV[2][3][512];        //[F_Id][YUV][Idx]
void Do_Smooth_YUV(int M_Mode)
{
    int i, j, k, i2;
    int S3_Sum_h;
    int L, Len, M, M2, line, line0, line1;
    int Data_Sum, Data_Total;
    int I3_S_Idx, I3_TB_Idx;
    int I3_S_Idx0, I3_TB_Idx0;
    struct Adjust_Line_S3_Struct *S3_p;
    int s_id0, s_id1, f_id, f_id0, f_id1;
    struct Adjust_Line_I3_Line_Struct *I3_Lp, *I3_Lp0;
    int I3_p0, I3_p1;
    struct Adjust_Line_I3_Struct *I3_p;
    int Point, Point0, sub0, sub1;

#ifndef ANDROID_CODE
    memset(&Smooth_O_Buf[0][0][0][0], 0, sizeof(Smooth_O_Buf) );
    memset(&Smooth_YUV[0][0][0], 0, sizeof(Smooth_YUV) );
    for(i = 0; i < 512; i++) {
        if(i >= 122 && i <= 135)
            Smooth_O_Buf[M_Mode][3][0][i] = 20;
        else if(i >= 80 && i <= 93)
            Smooth_O_Buf[M_Mode][3][0][i] = 10;
        else if(i >= 3 && i <= 19)
            Smooth_O_Buf[M_Mode][3][0][i] = -10;
        else if(i >= 0 && i <= 2)
            Smooth_O_Buf[M_Mode][3][0][i] = 15;
    }
#endif

    memcpy(&Smooth_YUV[0][0][0], &Smooth_O_Buf[M_Mode][3][0][0], sizeof(Smooth_YUV[0]) );
    memcpy(&Smooth_YUV[1][0][0], &Smooth_O_Buf[M_Mode][3][0][0], sizeof(Smooth_YUV[1]) );
    for (L = 0; L <= 2; L++) {        //YUV
        //Sensor 0 平滑化
        S3_p = &A_L_S3[M_Mode][0];
        Len = (S3_p->Sum >> 4);
        for (j = 0; j < S3_p->Sum; j++) {
            Data_Sum   = 0;
            Data_Total = 0;
            for (k = -Len; k <= Len; k++) {
                M = j + k; M2 = (M + S3_p->Sum) % S3_p->Sum;
                if( (M2 >= 0) && (M2 < S3_p->Sum) ) {
                    I3_S_Idx0  = S3_p->Source_Idx[M2];
                    Data_Sum   += Smooth_O_Buf[M_Mode][3][L][I3_S_Idx0];
                    Data_Total ++;
                }
            }
            I3_S_Idx  = S3_p->Source_Idx[j];
            if(Data_Total > 0) {
                Smooth_YUV[0][L][I3_S_Idx] = Data_Sum / Data_Total;
                Smooth_YUV[1][L][I3_S_Idx] = Data_Sum / Data_Total;
            }
            else {
                Smooth_YUV[0][L][I3_S_Idx] = 0;
                Smooth_YUV[1][L][I3_S_Idx] = 0;
            }
        }

        //垂直邊平滑化 (Line: 1 3 5 7)
        for(i = 1; i < 5; i++) {        //sensor
            f_id = (i & 0x1);
            S3_p = &A_L_S3[M_Mode][i];
            Len = (S3_p->Sum >> 4);
            for (j = 0; j < S3_p->Sum; j++) {
                I3_S_Idx  = S3_p->Source_Idx[j];
                I3_TB_Idx = S3_p->Top_Bottom[j];
                I3_p = &A_L_I3[M_Mode][I3_S_Idx];
                line = I3_p->Line_No;
                if(line >= 0 && line <= 3) {
                    Data_Sum   = 0;
                    Data_Total = 0;
                    for (k = -Len; k <= Len; k++) {
                        M = j + k; /*M2 = (M + S3_p->Sum) % S3_p->Sum;*/
                        if( (M >= 0) && (M < S3_p->Sum) ) {
                            I3_S_Idx0  = S3_p->Source_Idx[M];
                            I3_TB_Idx0 = S3_p->Top_Bottom[M];
                            if(I3_TB_Idx != I3_TB_Idx0)
                                Data_Sum   += -(Smooth_O_Buf[M_Mode][3][L][I3_S_Idx0]);
                            else
                                Data_Sum   += Smooth_O_Buf[M_Mode][3][L][I3_S_Idx0];
                            Data_Total ++;
                        }
                    }
                    if(Data_Total > 0) Smooth_YUV[f_id][L][I3_S_Idx] = Data_Sum / Data_Total;
                    else               Smooth_YUV[f_id][L][I3_S_Idx] = 0;

                    //垂直邊部分線性差補(Line: 1 3 5 7)
                    /*if(f_id == 0) {
                        line0 = (line / 4) * 2 + 8;
                    }
                    else {
                        line0 = line - 1;
                        I3_Lp = &A_L_I3_Line[line];
                        Point = I3_Lp->SL_Point[M_Mode];
                        Len = (I3_Lp->SL_Sum[M_Mode] >> 1);

                        I3_Lp0 = &A_L_I3_Line[line0];
                        Point0 = I3_Lp0->SL_Point[M_Mode];
                        I3_p0 = Point0 +;
                        I3_p1 = Point + Len;
                    }*/

                }
            }
        }
    }
}
int Smooth_Min_Lv = 15;
void Make_Smooth_I_Buf_67(int I3_Sum, int M_Mode, int C_Mode, int Mix_En)
{
    int i, j; unsigned tot1 = 0, tot2 = 0;
    int Max_Level;
    int Max_V1, Max_P1;
    int Max_V2, Max_P2;
    
    for (j = 0; j < I3_Sum; j++) {
        
        if(Mix_En == 1 && Smooth_T_Temp_En == 1 && BSmooth_Function == 4 && M_Mode < 3 && C_Mode != 2 && C_Mode != 11){       // M_Mode < 3 --> 6K 8K 12K, C_Mode != 2 --> 縮時除外
            for (i = 0; i < 64; i++){
                tot1 += Smooth_I_Buf[M_Mode][i][j];
                tot2 += Smooth_T_Temp[M_Mode][i][j];
                Smooth_I_Buf[M_Mode][i][j] = (Smooth_I_Buf[M_Mode][i][j] + Smooth_T_Temp[M_Mode][i][j]) >> 1;
            }
            if(j == (I3_Sum-1))
                db_debug("Make_Smooth_I_Buf: M_Mode=%d tot1=%d tot2=%d\n", M_Mode, tot1, tot2);
        }
        
        Max_V2 = 1; Max_P2 = -1;
        Max_V1 = 1; Max_P1 = -1;
        for (i = 0; i < 64; i++) {
          if (Smooth_I_Buf[M_Mode][i][j] > Max_V1) {
            Max_V1 = Smooth_I_Buf[M_Mode][i][j];
            Max_P1 = i;
          }
        }
        for (i = 0; i < 64; i++) {
          if ((Smooth_I_Buf[M_Mode][i][j] > Max_V2) && (abs(Max_P1 - i) > 3)) {
            Max_V2 = Smooth_I_Buf[M_Mode][i][j];
            Max_P2 = i;
          }
        }

        Max_Level = Max_V1 - Max_V2;        // 取最高數值和次高數值的差值，判斷是否為正確縫合點
        Smooth_I_Buf[M_Mode][68][j] = Max_Level;
        if (Max_Level > Smooth_Min_Lv) Smooth_I_Buf[M_Mode][67][j] = Max_P1;
        else               Smooth_I_Buf[M_Mode][67][j] = -1;

        if(Mix_En == 0){
            Smooth_Debug_Buf[M_Mode][0][j] = Smooth_I_Buf[M_Mode][67][j];           // 縫合點取最高數值做設定, getStitchData()
        
            //rex+ 181025
            Max_V2 = 1; Max_P2 = -1;
            Max_V1 = 1; Max_P1 = -1;
            for (i = 0; i < 64; i++) {
              if (Smooth_T_Temp[M_Mode][i][j] > Max_V1) {
                Max_V1 = Smooth_T_Temp[M_Mode][i][j];
                Max_P1 = i;
              }
            }
            for (i = 0; i < 64; i++) {
              if ((Smooth_T_Temp[M_Mode][i][j] > Max_V2) && (abs(Max_P1 - i) > 3)) {
                Max_V2 = Smooth_T_Temp[M_Mode][i][j];
                Max_P2 = i;
              }
            }
    
            Max_Level = Max_V1 - Max_V2;        // 取最高數值和次高數值的差值，判斷是否為正確縫合點
            Smooth_T_Temp[M_Mode][68][j] = Max_Level;
            if (Max_Level > Smooth_Min_Lv) Smooth_T_Temp[M_Mode][67][j] = Max_P1;
            else               Smooth_T_Temp[M_Mode][67][j] = -1;
            Smooth_Debug_Buf[M_Mode][1][j] = Smooth_T_Temp[M_Mode][67][j];
        }
    }
}

void Make_Smooth_I_AVG(int M_Mode)
{
    int i, j, k, idx, I3_Sum;
    int h1, h2, dif;
    int *v1, *p1, *v2, *p2;

    if(M_Mode != 0) return;         // only測試大畫面

    db_debug("Make_Smooth_I_AVG: in~\n");
    I3_Sum = A_L_I3_Header[M_Mode].Sum;
    if(I3_Sum > 512) I3_Sum = 512;

    for(j = 0; j < I3_Sum; j++){
        memset(&Smooth_I_AVG[1][j][0], 0, sizeof(Smooth_I_AVG[1][j]));
        memset(&Smooth_I_AVG[2][j][0], 0, sizeof(Smooth_I_AVG[2][j]));
        h1 = Smooth_I_AVG[0][j][0];                 // 取得高度pY1
        v1 = &Smooth_I_AVG[1][j][0];                // [1]設定總和v
        p1 = &Smooth_I_AVG[2][j][0];                // [2]設定數量p
        for(k = 0; k < I3_Sum; k++){
            h2 = Smooth_I_AVG[0][k][0];             // [0]取得高度h
            if(h2 > (h1-50) && h2 < (h1+50)){
                if(h1 > h2) dif = h1-h2;
                else        dif = h2-h1;
                for(i = 0; i < 64; i++){
                    v1[i] += (Smooth_I_Buf[M_Mode][i][k]*(50-dif));
                    p1[i] += (50-dif);
                }
            }
        }
        //db_debug("j=%d i=%d v1=%d p1=%d h1=%d h2=%d\n", j, i, *v1, *p1, h1, h2);
    }
    int v1bas, v2bas, n1, n2, maxV, maxP;
    for(j = 0; j < I3_Sum; j++){
        v1 = &Smooth_I_AVG[1][j][0];
        p1 = &Smooth_I_AVG[2][j][0];
        v1bas = 0;
        v2bas = 0;
        for(i = 0; i < 64; i++){
            if(*p1 > 0)
                v1bas += (*v1 / *p1);       // 計算基礎值
            v2bas += Smooth_I_Buf[M_Mode][i][j];
            v1 ++;
            p1 ++;
        }
        v1bas = v1bas >> 8;     // n/64, 取得平均基礎值
        v2bas = v2bas >> 8;     //
        //db_debug("j=%d v1b=%d v2b=%d\n", j, v1bas, v2bas);

        maxV = 0;
        maxP = -1;
        for(i = 0; i < 64; i++){
            v1 = &Smooth_I_AVG[1][j][0];
            p1 = &Smooth_I_AVG[2][j][0];
            n1 = 0;
            if(*p1 > 0){
                // Smooth_Com.Smooth_Avg_Weight = 25
                n1 = ((*v1 / *p1) - v1bas) * Smooth_Com.Smooth_Avg_Weight / 100;
                if(n1 < 0) n1 = 0;
                if(n1 > maxV){ maxV = n1; maxP = i; }
            }
            n2 = Smooth_I_Buf[M_Mode][i][j] - v2bas;    // 減掉基礎值
            if(n2 < 0) n2 = 0;
            Smooth_I_Buf[M_Mode][i][j] = n2 + n1;
            //if(j == 100)
            //    db_debug("i=%d n1=%d n2=%d v1=%d p1=%d v1b=%d mP=%d mV=%d\n", i, n1, n2, *v1, *p1, v1bas, maxP, maxV);

            v1 ++;
            p1 ++;
        }
        //Smooth_I_AVG[0][j][0] = 高度
        Smooth_I_AVG[0][j][1] = maxP;       // debug, type=11
        Smooth_I_AVG[0][j][2] = maxV;       // debug, type=12
    }//*/
    db_debug("Make_Smooth_I_AVG: out~\n");
}
//tmp extern int cap_file_cnt;
// int get_A2K_Cap_Cnt();
void Do_Auto_Smooth_Z(int M_Mode, int C_Mode)
{
    int i,j,k,i2, f_id;
    int I3_Sum, I3_Smooth_Sum, I3_Smooth_Total;
    int Smooth_P, Smooth_V;
    int ValueV, ValueH, ValueV1, ValueH1, CountV, CountH, idx, Weight, WeightV, WeightH;
    int Smooth_Manual_Rate;
    struct Adjust_Line_S3_Struct *S3_p;
    int Speed;
    int smooth_by_pass_flag = 0;        //手動調整直接調至目標值
    static int phi_top_lst = -1, phi_mid_lst = -1, auto_rate_lst = -1;
    int S3_Sum;
    int d_cnt = read_F_Com_In_Capture_D_Cnt();       // !=0: 拍照中

    if (Smooth_Com.Smooth_Auto_Rate > 100) Smooth_Com.Smooth_Auto_Rate = 100;
    if (Smooth_Com.Smooth_Auto_Rate <   0) Smooth_Com.Smooth_Auto_Rate = 0;

    Smooth_Manual_Rate = 100 - Smooth_Com.Smooth_Auto_Rate;

    I3_Sum = A_L_I3_Header[M_Mode].Sum;
    if(I3_Sum <= 0 || I3_Sum >= 512) return;

    for(j = 0; j < I3_Sum; j++) {
        for(k = 0; k < 64; k++)
          Smooth_I_Data[M_Mode][k][j] = 0;
    }

    for (i = 0; i < 32; i++) {
        for (j = 0; j < I3_Sum; j++) {
           // V/H 水平/垂直軸
           CountV  = (Smooth_Z_V_I[M_Mode][j][i].Count0 + 1);               // 比較數目
           CountH  = (Smooth_Z_H_I[M_Mode][j][i].Count0 + 1);
           WeightV = (Smooth_Z_V_I[M_Mode][j][i].Weight << 6) / CountV;     // 比較強度
           WeightH = (Smooth_Z_H_I[M_Mode][j][i].Weight << 6) / CountH;
           ValueV  = (Smooth_Z_V_I[M_Mode][j][i].Value0 << 6) / CountV;     // 比較相似，(像)0~1024(不像)
           ValueH  = (Smooth_Z_H_I[M_Mode][j][i].Value0 << 6) / CountH;
           if(WeightV < Smooth_Com.Smooth_Weight_Th) WeightV = 0;           // 強度門檻
           else                                      WeightV -= Smooth_Com.Smooth_Weight_Th;
           if(WeightH < Smooth_Com.Smooth_Weight_Th) WeightH = 0; 
           else                                      WeightH -= Smooth_Com.Smooth_Weight_Th;
           ValueV1 = Smooth_Com.Smooth_Base_Level - ValueV;                 // Smooth_Base_Level = 600，倒數運算 (不像)0 ~ 1024 (像)
           if (ValueV1 < 0) ValueV1 = 0;
           ValueH1 = Smooth_Com.Smooth_Base_Level - ValueH;
           if (ValueH1 < 0) ValueH1 = 0;
           Smooth_I_Data[M_Mode][i  ][j]    = ValueV1 * Smooth_Com.Smooth_Auto_Rate / 50;
           Smooth_I_Data[M_Mode][i+32][j]   = ValueH1 * Smooth_Com.Smooth_Auto_Rate / 50;
           Smooth_I_Weight[M_Mode][i][j]    = WeightV;
           Smooth_I_Weight[M_Mode][i+32][j] = WeightH;
        };
    }
    Make_Smooth_I_Buf(M_Mode);
    if(Smooth_Com.Smooth_Debug_Flag == 1){
//tmp        if((getCapFileCnt() & 0x1) == 1)          // 測試差異, 1=new avg, 0=normal
            Make_Smooth_I_AVG(M_Mode);
    }
    Make_Smooth_I_Buf_67(I3_Sum, M_Mode, C_Mode, 0);                // 計算縫合距離, 0:原始數值

    // Add Pre Smooth_O for stable，前一次資料加權，防抖動
    int pre_w;
    if(d_cnt == 0 && I3_Sum > 0){
        pre_w = (Smooth_Com.Smooth_Pre_Weight * 120  + (I3_Sum - 1)) / I3_Sum;
        for (j = 0; j < I3_Sum; j++) {
            for (k = -Smooth_Com.Smooth_Pre_Space; k <= Smooth_Com.Smooth_Pre_Space; k++) {
                idx = Smooth_O[0][M_Mode][j].Z + k;
                if ((idx >=0) && (idx < 64)){
                    // Smooth_Com.Smooth_Pre_Space = 3
                    // Smooth_Com.Smooth_Pre_Weight = 10;
                    // k = -3 ~ +3
                    // 2 3 4 5 4 3 2 -> 20 30 40 50 40 30 20
                    Smooth_I_Buf[M_Mode][idx][j] += (Smooth_Com.Smooth_Pre_Space + 2 - abs(k)) * pre_w * 
                                                     Smooth_Com.Smooth_Auto_Rate / 100;
                }
            }
        }
    }

    if(Smooth_Manual_Rate != 0){
        Do_Manual_Smooth_Z( M_Mode);
        // Add Manual Smooth_O for stable
        for (j = 0; j < I3_Sum; j++) {
            for (k = -Smooth_Com.Smooth_Manual_Space; k <= Smooth_Com.Smooth_Manual_Space; k++) {
                idx = Smooth_I_Buf[M_Mode][69][j] + k;
                if ((idx >=0) && (idx < 64)){
                    // Smooth_Manual_Rate = 100 - Smooth_Com.Smooth_Auto_Rate;
                    // Smooth_Com.Smooth_Manual_Space = 8 (實驗結果)，手調數值加權
                    // Smooth_Com.Smooth_Manual_Weight = 4
                    // k = -8 ~ +8
                    // (8+2-8)=2 (8+2-7)=3 (8+2-6)=4 ... (8+2-0)=10 (8+2-1)=9 -> 2 3 4 5 6 7 8 9 10 9 8 7 6 5 4 3 2
                    Smooth_I_Buf[M_Mode][idx][j] += (Smooth_Com.Smooth_Manual_Space + 2 - abs(k)) * Smooth_Com.Smooth_Manual_Weight * 
                                                     Smooth_Manual_Rate / 50;
                }
            }
        }
    }
    Make_Smooth_I_Buf_67(I3_Sum, M_Mode, C_Mode, 1);                // 計算縫合距離, 1:原始和加權數值混合

    int Max_P1;
    for (j = 0; j < I3_Sum; j++) {
        Max_P1 = Smooth_I_Buf[M_Mode][67][j];
        // rex+ 181011, 增加相似度係數
        Smooth_I_Weight[M_Mode][64][j] = 0; // 總和
        Smooth_I_Weight[M_Mode][65][j] = 1; // 平均
        Smooth_I_Weight[M_Mode][66][j] = 0; // 倍率
        if(Max_P1 != -1){
            for(i = 0; i < 64; i++) Smooth_I_Weight[M_Mode][64][j] += Smooth_I_Buf[M_Mode][i][j];       // 總和
            Smooth_I_Weight[M_Mode][65][j] = Smooth_I_Weight[M_Mode][64][j] >> 6;                       // 平均
            if(Smooth_I_Weight[M_Mode][65][j] > 0)                                                      // 倍率
                Smooth_I_Weight[M_Mode][66][j] = (Smooth_I_Buf[M_Mode][Max_P1][j]*64) / Smooth_I_Weight[M_Mode][65][j];
            
            if(M_Mode == 0){
                if(BSmooth_Function == 3) Smooth_I_Weight[M_Mode][66][j] = 100; // 測試加權比例
                //中央加權
                if(j >= 0 && j <= 10)    // 右一直線
                    Smooth_I_Weight[M_Mode][66][j] = Smooth_I_Weight[M_Mode][66][j] + ((Smooth_I_Weight[M_Mode][66][j] * (10-j) * BSmooth_FarWeight) / 100);     // 0~10%
                if(j >= 47 && j <= 57)   // 右二直線
                    Smooth_I_Weight[M_Mode][66][j] = Smooth_I_Weight[M_Mode][66][j] + ((Smooth_I_Weight[M_Mode][66][j] * (57-j) * BSmooth_FarWeight) / 100);
                if(j >= 94 && j <= 104)  // 左二直線
                    Smooth_I_Weight[M_Mode][66][j] = Smooth_I_Weight[M_Mode][66][j] + ((Smooth_I_Weight[M_Mode][66][j] * (104-j) * BSmooth_FarWeight) / 100);
                if(j >= 141 && j <= 151) // 左一直線
                    Smooth_I_Weight[M_Mode][66][j] = Smooth_I_Weight[M_Mode][66][j] + ((Smooth_I_Weight[M_Mode][66][j] * (151-j) * BSmooth_FarWeight) / 100);
                if(j >= 267 && j <= 275)// 斜線左二
                    Smooth_I_Weight[M_Mode][66][j] = Smooth_I_Weight[M_Mode][66][j] + ((Smooth_I_Weight[M_Mode][66][j] * (j-267) * BSmooth_FarWeight) / 100);
                if(j >= 320 && j <= 328)// 斜線左一
                    Smooth_I_Weight[M_Mode][66][j] = Smooth_I_Weight[M_Mode][66][j] + ((Smooth_I_Weight[M_Mode][66][j] * (328-j) * BSmooth_FarWeight) / 100);
                if(j >= 355 && j <= 363)// 斜線右一
                    Smooth_I_Weight[M_Mode][66][j] = Smooth_I_Weight[M_Mode][66][j] + ((Smooth_I_Weight[M_Mode][66][j] * (j-355) * BSmooth_FarWeight) / 100);
                if(j >= 232 && j <= 240)// 斜線右二
                    Smooth_I_Weight[M_Mode][66][j] = Smooth_I_Weight[M_Mode][66][j] + ((Smooth_I_Weight[M_Mode][66][j] * (240-j) * BSmooth_FarWeight) / 100);
            }
        }
    }

    //去除比對失誤(縫合點打掉)
    int idx1, idx2, idx3, idx4;
    int tmp1, tmp2, tmp3, tmp4;
    int a, b, c, d, m, n;
    int slope=10;
    //I3_Sum = 120(4K), 364(12K)
    if(I3_Sum != 0) slope = (BSmooth_DelSlope + (I3_Sum - 1)) / I3_Sum;        //BSmooth_DelSlope = 1360, I3_Sum = 136~408

    if(BSmooth_Function == 255){}       //不做任何處理
    else{
        //Smooth_Z_Temp_Buf[j] = Smooth_I_Buf[M_Mode][67][j];
        memcpy(&Smooth_Z_Temp_Buf[0], &Smooth_I_Buf[M_Mode][67][0], sizeof(Smooth_Z_Temp_Buf));
        
        for(i2 = 0; i2 < 4; i2++) {        //舊的打掉機制
            switch (i2) {
                case 0: i = 2; break;     // i=2, 左一
                case 1: i = 4; break;     // i=4, 右一
                case 2: i = 1; break;     // i=1, 中
                case 3: i = 3; break;     // i=3, 邊
            }
            S3_Sum = A_L_S3[M_Mode][i].Sum;
            for(j = 0; j < (S3_Sum-1); j++) {
                idx1 = A_L_S3[M_Mode][i].Source_Idx[j]&0x1ff;
                if( (j+1) < S3_Sum) idx2 = A_L_S3[M_Mode][i].Source_Idx[j+1]&0x1ff;
                else                idx2 = A_L_S3[M_Mode][i].Source_Idx[S3_Sum-1]&0x1ff;
                if( (j+2) < S3_Sum) idx3 = A_L_S3[M_Mode][i].Source_Idx[j+2]&0x1ff;
                else                idx3 = A_L_S3[M_Mode][i].Source_Idx[S3_Sum-1]&0x1ff;
                tmp1 = Smooth_I_Buf[M_Mode][67][idx1];
                tmp2 = Smooth_I_Buf[M_Mode][67][idx2];
                tmp3 = Smooth_I_Buf[M_Mode][67][idx3];
    
                if(tmp1 != -1 && tmp2 != -1) {                      //相鄰兩點相差15以上, 兩點設為-1
                    if(abs(tmp1 - tmp2) > 15) {
                        Smooth_Z_Temp_Buf[idx1] = -1;
                        Smooth_Z_Temp_Buf[idx2] = -1;
                    }
                }
    
                if(tmp1 != -1 && tmp2 == -1 && tmp3 != -1) {        //間隔1點相差30以上, 且中間為-1, 兩點設為-1
                    if(abs(tmp1 - tmp3) > 30) {
                        Smooth_Z_Temp_Buf[idx1] = -1;
                        Smooth_Z_Temp_Buf[idx3] = -1;
                    }
                }//*/
            }
        }
        if(M_Mode == 0){    // rex+ 181029, debug
            memset(&Smooth_Debug_Buf[1][0][0], 0, sizeof(Smooth_Debug_Buf[1]));
        }

        // 2.比較數值大(近距離)的刪掉
        for(i2 = 0; i2 < 4; i2++) {
            switch (i2) {
                case 0: i = 2; break;     // i=2, 左一
                case 1: i = 4; break;     // i=4, 右一
                case 2: i = 1; break;     // i=1, 中
                case 3: i = 3; break;     // i=3, 邊
            }
            S3_Sum = A_L_S3[M_Mode][i].Sum;

            for(j = 0; j < (S3_Sum-1); j++){        // 順時針運算
last1:
                idx1 = A_L_S3[M_Mode][i].Source_Idx[j]&0x1ff; tmp1 = Smooth_I_Buf[M_Mode][67][idx1];
                if(tmp1 == -1 || Smooth_Z_Temp_Buf[idx1] == -1){
                    Smooth_Z_Temp_Buf[idx1] = -1; 
                    //if(M_Mode == 0 && i == 1) db_debug("1) j=%d %d %d\n", j, idx1, tmp1);
                    continue; 
                }
                for(k = j+1; k < S3_Sum; k++){
                    idx2 = A_L_S3[M_Mode][i].Source_Idx[k]&0x1ff; tmp2 = Smooth_I_Buf[M_Mode][67][idx2];
                    if(tmp2 == -1 || Smooth_Z_Temp_Buf[idx2] == -1){
                        Smooth_Z_Temp_Buf[idx2] = -1; 
                        //if(M_Mode == 0 && i == 1) db_debug("2) j=%d %d %d k=%d %d %d\n", j, idx1, tmp1, k, idx2, tmp2);
                        continue; 
                    }
                    c = (tmp2 + tmp1) >> 1;
                    if(c < 40) d = 0;
                    else if(c < 48) d = 1;
                    else if(c < 56) d = 2;
                    else d = 3;
                    if(abs(tmp2 - tmp1) > (slope+d*2)*(k-j)){       // slope+d，d數字越大(距離越近)，可容忍範圍越大(63->+3)
                        a = Smooth_I_Weight[M_Mode][66][idx1];
                        b = Smooth_I_Weight[M_Mode][66][idx2];
                        //拆強度低的點
                        if(a >= b){ 
                            Smooth_Z_Temp_Buf[idx2] = -1; 
                            //if(M_Mode == 0 && i == 1) db_debug("3) j=%d %d %d k=%d %d %d\n", j, idx1, tmp1, k, idx2, tmp2); 
                            continue; 
                        }
                        else{ 
                            Smooth_Z_Temp_Buf[idx1] = -1; 
                            //if(M_Mode == 0 && i == 1) db_debug("4) j=%d %d %d k=%d %d %d\n", j, idx1, tmp1, k, idx2, tmp2); 
                            for(m = j; m > 0; m--){ // 回到上一點
                                idx3 = A_L_S3[M_Mode][i].Source_Idx[m]&0x1ff; tmp3 = Smooth_I_Buf[M_Mode][67][idx3];
                                if(tmp3 != -1 && Smooth_Z_Temp_Buf[idx3] != -1){
                                    j = m;
                                    goto last1;
                                }
                            }
                        }
                    }
                    //if(M_Mode == 0 && i == 1) db_debug("5) j=%d %d %d k=%d %d %d\n", j, idx1, tmp1, k, idx2, tmp2); 
                    j = k-1; break;
                }
            }
        }
        memcpy(&Smooth_I_Buf[M_Mode][67][0], &Smooth_Z_Temp_Buf[0], sizeof(Smooth_Z_Temp_Buf));
    }


    // 縫合點判斷是否有低於門檻的點，避免扭曲
    int flag_L, flag_R, Len; int P1,V1,P2,D2;
    int v1_min = 30, d2_min = 6;
    if(I3_Sum > 0){
        v1_min = (Smooth_Com.Smooth_Low_Level * 120 + (I3_Sum - 1)) / I3_Sum;   // 強度，[68]
        d2_min = (6 * 120 + (I3_Sum - 1)) / I3_Sum;                             // 距離，[67]
    }
    for (j = 0; j < I3_Sum; j++) {
        Len = 1;
        P1 = Smooth_I_Buf[M_Mode][67][j];
        V1 = Smooth_I_Buf[M_Mode][68][j];
        if ((P1 != -1) && (V1 < v1_min)) {                 // Smooth_Com.Smooth_Low_Level = 30, rex+s 181017
                flag_L = 1; flag_R = 1;
                for (i = 1; i <= Len; i++) {
                       i2 = j + i;
                       if ((i2 >= 0) && (i2 < 64)) {
                               P2 = Smooth_I_Buf[M_Mode][67][i2];
                               D2 = abs(P1 - P2);
                               if (P2 == -1)  flag_L = 0;
                               if (D2 < d2_min) flag_L = 0;
                       }
                       i2 = j - i;
                       if ((i2 >= 0) && (i2 < 64)) {
                               P2 = Smooth_I_Buf[M_Mode][67][i2];
                               D2 = abs(P1 - P2);
                               if (P2 == -1)  flag_R = 0;
                               if (D2 < d2_min) flag_R = 0;
                       }
                }
                if ((flag_L == 0) && (flag_R == 0)) Smooth_I_Buf[M_Mode][67][j] = -1;
        }
        Smooth_Debug_Buf[M_Mode][2][j] = Smooth_I_Buf[M_Mode][67][j];         // 縫合點判斷是否有低於門檻的點, getStitchData()
    }

    // debug    
    if(BSmooth_Function == 2){ // 看數值順序 -> A_L_S3[M_Mode][i].Source_Idx[j]
        for(i2 = 0; i2 < 5; i2++) {
            switch (i2) {
                case 0: i = 2; break;     // i=2, 左一
                case 1: i = 4; break;     // i=4, 右一
                case 2: i = 1; break;     // i=1, 中
                case 3: i = 3; break;     // i=3, 邊
                case 4: i = 0; break;     // i=0, 上
            }
            S3_Sum = A_L_S3[M_Mode][i].Sum;
            for(j = 0; j < S3_Sum; j++){
                idx1 = A_L_S3[M_Mode][i].Source_Idx[j]&0x1ff;
                Smooth_Debug_Buf[M_Mode][2][idx1] = j;
            }
        }
    }
    else if(BSmooth_Function == 3){ } //看加權數值 -> Smooth_I_Weight[]


    int L, I3_Idx0_S, I3_Idx1_S, I3_Idx2_S;
    int PI0, PV0;
    int PI1, PV1;
    for (i2 = 0; i2 < 4; i2++) {
          switch (i2) {
                  case 0: i = 2; break;     // i=2, 左一
                  case 1: i = 4; break;     // i=4, 右一
                  case 2: i = 1; break;     // i=1, 中
                  case 3: i = 3; break;     // i=3, 邊
          }
          PI0 = -1;
          S3_p = &A_L_S3[M_Mode][i];
          for (j = 0; j < S3_p->Sum; j++) {
                  I3_Idx0_S = S3_p->Source_Idx[j]&0x1ff;
                  if (Smooth_I_Buf[M_Mode][67][I3_Idx0_S] < 0) {
                          for (k = j; k < S3_p->Sum; k++) {
                                  I3_Idx1_S = S3_p->Source_Idx[k]&0x1ff;
                                  if (Smooth_I_Buf[M_Mode][67][I3_Idx1_S] >= 0) {
                                          PV1 = Smooth_I_Buf[M_Mode][67][I3_Idx1_S];
                                          PI1 = k;
                                          break;
                                  }
                          }
                          if (PI0 == -1) { PI0 = -1; PV0 = PV1;};
                          if (k == S3_p->Sum) { PI1 = S3_p->Sum; PV1 = PV0; };

                          if (PV1 < 0) PV1 = PV0;
                          if (PV0 < 0) PV0 = PV1;
                          if ((PV0 < 0) && (PV1 < 0)) {
                                  PV0 = 32; PV1 = 32;
                          };

                          Len = PI1 - PI0;
                          if (Len > 1) {
                                  for (L = PI0 + 1; L < PI1; L++) {
                                          I3_Idx2_S = S3_p->Source_Idx[L]&0x1ff;
                                          Smooth_I_Buf[M_Mode][67][I3_Idx2_S] =  (PV1 * (L - PI0) +  PV0 * (PI1 - L)) / Len;
                                  }
                                  j = PI1 - 1;
                          };
                  }
                  else {
                          PV0 = Smooth_I_Buf[M_Mode][67][I3_Idx0_S];
                          PI0 = j;
                  }
          }
    }

   int O_Speed,Now, Target;
   if( Smooth_Com.Smooth_Auto_Rate < 100 && (phi_top_lst != Smooth_Com.Global_phi_Top || phi_mid_lst != Smooth_Com.Global_phi_Mid /*|| auto_rate_lst != Smooth_Com.Smooth_Auto_Rate*/) ) {
       phi_top_lst = Smooth_Com.Global_phi_Top;
       phi_mid_lst = Smooth_Com.Global_phi_Mid;
       auto_rate_lst = Smooth_Com.Smooth_Auto_Rate;
       smooth_by_pass_flag = 1;
   }
   int run_big_smooth = get_run_big_smooth();
   int cap_en=0; int Delta;
   int Z_Max, Z_Min, YUV_Max;
   Z_Min = (0 << 4);
   Z_Max = (63 << 4);
   YUV_Max = (127 << 4);
   for (j = 0; j < 4; j++) {        //YUVZ
       for (i = 0; i < A_L_I3_Header[M_Mode].Sum; i++) {
           if (j == 3) {
              if (Smooth_O_Buf[M_Mode][1][j][i] < Z_Min) Smooth_O_Buf[M_Mode][1][j][i] = Z_Min;
              if (Smooth_O_Buf[M_Mode][1][j][i] > Z_Max) Smooth_O_Buf[M_Mode][1][j][i] = Z_Max;                //if (Smooth_O_Buf[M_Mode][1][j][i] > 63*16) Smooth_O_Buf[M_Mode][1][j][i] = 63 * 16;
           }
           else {
               if (Smooth_O_Buf[M_Mode][1][j][i] < -YUV_Max) Smooth_O_Buf[M_Mode][1][j][i] = -YUV_Max;        //if (Smooth_O_Buf[M_Mode][1][j][i] < -127 * 16) Smooth_O_Buf[M_Mode][1][j][i] = -127 * 16;
               if (Smooth_O_Buf[M_Mode][1][j][i] >  YUV_Max) Smooth_O_Buf[M_Mode][1][j][i] =  YUV_Max;        //if (Smooth_O_Buf[M_Mode][1][j][i] >  127 * 16) Smooth_O_Buf[M_Mode][1][j][i] =  127 * 16;
           }

           Target = (Smooth_I_Buf[M_Mode][64+j][i] << 4);                                                    //Target = Smooth_I_Buf[M_Mode][64+j][i] * 16;
           Smooth_O_Buf[M_Mode][0][j][i] = Target;
           Now    = Smooth_O_Buf[M_Mode][1][j][i];
           Delta = abs(Target - Now);

           if (j == 3) {
               if(Get_Smooth_Speed_Mode() != 0) Speed = ((Smooth_O_Buf[M_Mode][1][j][i] / 32) + Smooth_Z_Speed) / 6;
               else                             Speed = ((Smooth_O_Buf[M_Mode][1][j][i] / 32) + Smooth_Z_Speed) / 2;
           }
           else {
               if(Get_Smooth_Speed_Mode() != 0) Speed = Smooth_YUV_Speed / 3;
               else                             Speed = Smooth_YUV_Speed;
           }


           if (Delta > Speed) O_Speed = Speed;
           else               O_Speed = Delta;

           if (Target > Now) Smooth_O_Buf[M_Mode][1][j][i] += O_Speed;
           else              Smooth_O_Buf[M_Mode][1][j][i] -= O_Speed;

           if(run_big_smooth == 1){         // 0->Cap, 3->HDR, /*CameraMode == 3 || CameraMode == 0*/
               if(d_cnt != 0){
                   cap_en = 1;              // 拍照中
                   if(j == 3 && BSmooth_Function == 0) break;        // rex- 181008, 大圖縫合, 不做Z軸運算, 使用小圖轉大圖的數值
               }
           }
//tmp           if(j == 3 && (C_Mode == 2 || C_Mode == 11) && rec_state == -2){       // 縮時模式加速反應
		   if(j == 3 && (C_Mode == 2 || C_Mode == 11) /*&& rec_state == -2*/){       // 縮時模式加速反應
               Smooth_O_Buf[M_Mode][1][j][i] = (Smooth_O_Buf[M_Mode][3][j][i] << 3) + (Smooth_I_Buf[M_Mode][64+j][i] << 3);
           }
           if((j == 3 && BSmooth_Set == 1) ||       // 縫合若有新設定，不要再參考舊資料平均
              (smooth_by_pass_flag == 1) || 
              (cap_en == 1))
           {
               Smooth_O_Buf[M_Mode][1][j][i] = Smooth_I_Buf[M_Mode][64+j][i] << 4;      // 強制套用新參數
           }

           if(j == 3 && C_Mode == 3 && cap_en == 1){                                    // AEB關掉縫合動作
               int s3_en = chk_Line_YUV_Offset_Step_S3();
               int st_en = read_AEB_ST_Enable();
               if((s3_en != 0) && (st_en == 1)){                                        // 只讓第一張做Z縫合
                   set_AEB_ST_Enable(0);
                   Smooth_O_Buf[M_Mode][1][j][i] = Smooth_I_Buf[M_Mode][64+j][i] << 4;  // 強制套用新參數
                   Smooth_O_Buf[M_Mode][3][j][i] = Smooth_O_Buf[M_Mode][1][j][i] >> 4;
               }
           }
           else{
               Smooth_O_Buf[M_Mode][3][j][i] = Smooth_O_Buf[M_Mode][1][j][i] >> 4;
           }
       }
    }
    int s3en = chk_Line_YUV_Offset_Step_S3();
    if(s3en != 0){
        run_Line_YUV_Offset_Step_S3();              // Smooth_O_Buf[M_Mode][3]
        //db_debug("Do_Auto_Smooth_Z: s3en=%d sum=%d\n", s3en, A_L_I3_Header[M_Mode].Sum);
    }
    BSmooth_Set = 0;
}

//Debug
int Save_Smooth_File_En = 0;
int Save_Smooth_File_Flag = 0;
struct Smooth_Debug_Struct Smooth_Debug;
void setSaveSmoothEn(int en)
{
    Save_Smooth_File_En = en;
}

int getSaveSmoothEn(void)
{
    return Save_Smooth_File_En;
}

//debug
void checkSaveSmoothBin() {
	FILE *fp;
	struct stat sti;
    char fileStr[64] = "/mnt/sdcard/US360/SaveSmoothEn.bin\0";
	if (-1 == stat(fileStr, &sti) )
        return;
    else
		setSaveSmoothEn(1);
}
	
void Set_Smooth_Debug(void)
{
    if(Save_Smooth_File_En == 1) {
        memset(&Smooth_Debug, 0, sizeof(Smooth_Debug) );
        memcpy(&Smooth_Debug.Smooth_Z_V_I[0][0][0], &Smooth_Z_V_I[0][0][0], sizeof(Smooth_Z_V_I) );
        memcpy(&Smooth_Debug.Smooth_Z_H_I[0][0][0], &Smooth_Z_H_I[0][0][0], sizeof(Smooth_Z_H_I) );
        memcpy(&Smooth_Debug.Smooth_I_Buf[0][0][0], &Smooth_I_Buf[0][0][0], sizeof(Smooth_I_Buf) );
        memcpy(&Smooth_Debug.Smooth_O_Buf[0][0][0][0], &Smooth_O_Buf[0][0][0][0], sizeof(Smooth_O_Buf) );
        memcpy(&Smooth_Debug.Smooth_O[0][0][0], &Smooth_O[0][0][0], sizeof(Smooth_O) );
        memcpy(&Smooth_Debug.Smooth_Debug_Buf[0][0][0], &Smooth_Debug_Buf[0][0][0], sizeof(Smooth_Debug_Buf) );
        Save_Smooth_File_Flag = 1;
    }
}
void SaveSmoothFile(char *c_path)
{
    int i;
    FILE *fp;
    char *ptr, path_tmp[128], path[128];

    //db_debug("SaveSmoothFile: 00 cap_path=%s en=%d flag=%d\n", c_path, Save_Smooth_File_En, Save_Smooth_File_Flag);
    if(Save_Smooth_File_En == 1 && Save_Smooth_File_Flag == 1) {
        Save_Smooth_File_Flag = 0;

        ptr = c_path;
        for(i = 0; i < 128; i++) {
            if(*ptr == '.' && *(ptr+1) == 'j' && *(ptr+2) == 'p' && *(ptr+3) == 'g')
                break;
            ptr++;
        }
        if(i >= 128) return;

        memcpy(&path_tmp[0], c_path, i);
        sprintf(path, "%s.bin\0", path_tmp);
        db_debug("SaveSmoothFile: 01 path_tmp=%s path=%s i=%d\n", path_tmp, path, i);
        fp = fopen(path, "wb");
        if(fp != NULL) {
            fwrite(&Smooth_Debug, sizeof(Smooth_Debug), 1, fp);
            fclose(fp);
        }
    }
}
int ReadSmoothFile(char *path)
{
	FILE *fp;

	fp = fopen(path, "rb");
	if(fp != NULL) {
		fread(&Smooth_Debug, sizeof(Smooth_Debug), 1, fp);

//        memcpy(&Smooth_Z_V_I[0][0][0], &Smooth_Debug.Smooth_Z_V_I[0][0][0], sizeof(Smooth_Z_V_I) );
//        memcpy(&Smooth_Z_H_I[0][0][0], &Smooth_Debug.Smooth_Z_H_I[0][0][0], sizeof(Smooth_Z_H_I) );
//        memcpy(&Smooth_I_Buf[0][0][0], &Smooth_Debug.Smooth_I_Buf[0][0][0], sizeof(Smooth_I_Buf) );
//        memcpy(&Smooth_O_Buf[0][0][0][0], &Smooth_Debug.Smooth_O_Buf[0][0][0][0], sizeof(Smooth_O_Buf) );
//        memcpy(&Smooth_O[0][0][0], &Smooth_Debug.Smooth_O[0][0][0], sizeof(Smooth_O) );

		fclose(fp);
	}
	else return -1;

	return 0;
}

int Debug_Smooth_O_Idx = -1, Debug_Smooth_O_Value = 0;
void SetSmoothOIdx(int idx)
{
    Debug_Smooth_O_Idx = idx;
}
int GetSmoothOIdx(void)
{
    return Debug_Smooth_O_Idx;
}

void SetSmoothOValue(int value)
{
    Debug_Smooth_O_Value = (value & 0x3F);
}
int GetSmoothOValue(void)
{
    return Debug_Smooth_O_Value;
}


void Smooth_I_to_O_Proc(int M_Mode)
{
	int i, j, k;
    int I3_Sum;
    int Sum, Z_Sum, YUV_Sum[3];
	int I3_Idx, S_Idx;

    Do_Smooth_YUV(M_Mode);

    I3_Sum = A_L_I3_Header[M_Mode].Sum;
    for(i = 0; i < 2; i++) {
    	// Miller 20181024, 內圈Smooth
    	for (k = 0; k < 5; k++) {
    		Sum = A_L_S3[M_Mode][k].Sum;
    		Z_Sum = 0;
    		I3_Idx = A_L_I3_Header[M_Mode].Sum;
    		for (j = 0; j < Sum; j++) {
    			S_Idx = A_L_S3[M_Mode][k].Source_Idx[j];
    			Z_Sum += Smooth_O_Buf[M_Mode][3][3][S_Idx];
    		};
    		Smooth_O_Buf[M_Mode][3][3][I3_Idx + k] = Z_Sum / Sum;
    		Smooth_YUV[i][0][I3_Idx + k] = 0;
    		Smooth_YUV[i][1][I3_Idx + k] = 0;
    		Smooth_YUV[i][2][I3_Idx + k] = 0;
    	}

    	// 底部4點平均
    	Z_Sum = 0;
    	YUV_Sum[0]=0; YUV_Sum[1]=0; YUV_Sum[2]=0;
    	for(k = 0; k < 4; k++) {
    		I3_Idx = A_L_I3_Line[k].SL_Point[M_Mode] + A_L_I3_Line[k].SL_Sum[M_Mode] - 1;
    		Z_Sum += Smooth_O_Buf[M_Mode][3][3][I3_Idx];

    		YUV_Sum[0] += Smooth_YUV[i][0][I3_Idx];
    		YUV_Sum[1] += Smooth_YUV[i][1][I3_Idx];
    		YUV_Sum[2] += Smooth_YUV[i][2][I3_Idx];
    	}
		Smooth_O_Buf[M_Mode][3][3][I3_Sum + 5] = Z_Sum >> 2;
		Smooth_YUV[i][0][I3_Sum + 5] = YUV_Sum[0] >> 2;
		Smooth_YUV[i][1][I3_Sum + 5] = YUV_Sum[1] >> 2;
		Smooth_YUV[i][2][I3_Sum + 5] = YUV_Sum[2] >> 2;

		for (j = 0; j < (I3_Sum + 6); j++) {
            if(ColorST_SW == 1) {
                    //Y
                    if(Smooth_YUV[i][0][j] >= 0) {
                      Smooth_O[i][M_Mode][j].F_Y = 0;
                      if(Smooth_YUV[i][0][j] > 127 || Smooth_YUV[i][0][j] < -127)
                          Smooth_O[i][M_Mode][j].Y = 127;
                        else
                          Smooth_O[i][M_Mode][j].Y = Smooth_YUV[i][0][j];
                    }
                    else {
                      Smooth_O[i][M_Mode][j].F_Y = 1;
                      if(Smooth_YUV[i][0][j] > 127 || Smooth_YUV[i][0][j] < -127)
                          Smooth_O[i][M_Mode][j].Y = 127;
                        else
                          Smooth_O[i][M_Mode][j].Y = -(Smooth_YUV[i][0][j]);
                    }

                    //U
                    if(Smooth_YUV[i][1][j] >= 0) {
                      Smooth_O[i][M_Mode][j].F_U = 0;
                      if(Smooth_YUV[i][1][j] > 127 || Smooth_YUV[i][1][j] < -127)
                          Smooth_O[i][M_Mode][j].U = 127;
                        else
                          Smooth_O[i][M_Mode][j].U = Smooth_YUV[i][1][j];
                    }
                    else {
                      Smooth_O[i][M_Mode][j].F_U = 1;
                      if(Smooth_YUV[i][1][j] > 127 || Smooth_YUV[i][1][j] < -127)
                          Smooth_O[i][M_Mode][j].U = 127;
                        else
                          Smooth_O[i][M_Mode][j].U = -(Smooth_YUV[i][1][j]);
                    }

                    //V
                    if(Smooth_YUV[i][2][j] >= 0) {
                      Smooth_O[i][M_Mode][j].F_V = 0;
                      if(Smooth_YUV[i][2][j] > 127 || Smooth_YUV[i][2][j] < -127)
                          Smooth_O[i][M_Mode][j].V = 127;
                      else
                          Smooth_O[i][M_Mode][j].V = Smooth_YUV[i][2][j];
                    }
                    else {
                      Smooth_O[i][M_Mode][j].F_V = 1;
                      if(Smooth_YUV[i][2][j] > 127 || Smooth_YUV[i][2][j] < -127)
                          Smooth_O[i][M_Mode][j].V = 127;
                      else
                          Smooth_O[i][M_Mode][j].V = -(Smooth_YUV[i][2][j]);
                    }
                }
                else {
                  //Y
                  Smooth_O[i][M_Mode][j].F_Y = 0;
                  Smooth_O[i][M_Mode][j].Y = 0;

                  //U
                  Smooth_O[i][M_Mode][j].F_U = 0;
                  Smooth_O[i][M_Mode][j].U = 0;

                  //V
                  Smooth_O[i][M_Mode][j].F_V = 0;
                  Smooth_O[i][M_Mode][j].V = 0;
                }

            	//Z
                Smooth_O[i][M_Mode][j].F_Z = 0;
                if(Debug_Smooth_O_Idx != -1 && j == Debug_Smooth_O_Idx) {
                    Smooth_O[i][M_Mode][j].Z = Debug_Smooth_O_Value;
                }
                else {
                  if(Smooth_O_Buf[M_Mode][3][3][j] > 63)
                      Smooth_O[i][M_Mode][j].Z = 63;
                  else if(Smooth_O_Buf[M_Mode][3][3][j] < 0)
                      Smooth_O[i][M_Mode][j].Z = 0;
                  else
                      Smooth_O[i][M_Mode][j].Z = Smooth_O_Buf[M_Mode][3][3][j];
              }
          }
      }
}

#ifdef ANDROID_CODE
int Send_Smooth_Table(int M_Mode, int idx)
{
    int i, tmpaddr, f_id;
    int size, size_tmp;
    unsigned char *st_p;
    AS2_SPI_Cmd_struct SPI_Cmd_P;

    size = 4000 / 32;
    for(f_id = 0; f_id < 2; f_id++) {
        size_tmp = sizeof(Smooth_O[f_id][M_Mode]);
        st_p = &Smooth_O[f_id][M_Mode][0];
        for(i = 0; i < size_tmp; i += 4000) {
			if(f_id == 0) tmpaddr = F2_0_SPI_IO_SMOOTH_PHI_DATA + (M_Mode << 11) + i;        //(M_Mode * 512 * 4)
			else          tmpaddr = F2_1_SPI_IO_SMOOTH_PHI_DATA + (M_Mode << 11) + i;        //(M_Mode * 512 * 4)
            ua360_spi_ddr_write(tmpaddr,  (st_p+i), 4000);

			if(f_id == 0) SPI_Cmd_P.S_DDR_P = (F2_0_SPI_IO_SMOOTH_PHI_DATA + (M_Mode << 11) + i) >> 5;        //(M_Mode * 512 * 4)
			else          SPI_Cmd_P.S_DDR_P = (F2_1_SPI_IO_SMOOTH_PHI_DATA + (M_Mode << 11) + i) >> 5;        //(M_Mode * 512 * 4)
			SPI_Cmd_P.T_DDR_P = (FX_SPI_IO_SMOOTH_PHI_DATA + (M_Mode << 11) + i) >> 5;        //(M_Mode * 512 * 4)

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
#endif

void Make_Smooth_table(void)
{
    int i,j,k, L;
    int S_Idx;
    int I3_Idx_S, I3_Idx2, Source_Idx;
    struct Adjust_Line_I3_Struct *I3_p0;
    struct Adjust_Line_S3_Struct *S3_p;
    int Value;
    int M_Mode, XY_Space;

    for (M_Mode = 0; M_Mode < 6; M_Mode++) {
       for (j = 0; j < A_L_I3_Header[M_Mode].Sum; j++) {
           I3_p0 = &A_L_I3[M_Mode][j];
           I3_p0->Smooth_Sum   = 0;
           I3_p0->Smooth_Total = 0;
           for (L = 0; L < 32; L++) {
               I3_p0->Smooth_P[L]  = -1;
               I3_p0->Smooth_V[L]  = 0;
           }
       };
       switch(M_Mode){
           default:
           case 0: XY_Space = Smooth_Com.Smooth_XY_Space * 3; break;    // 12K
           case 1: XY_Space = Smooth_Com.Smooth_XY_Space * 2; break;    // 8K
           case 2: XY_Space = Smooth_Com.Smooth_XY_Space * 2; break;    // 6K
           case 3: XY_Space = Smooth_Com.Smooth_XY_Space; break;        // 4K
           case 4: XY_Space = Smooth_Com.Smooth_XY_Space; break;        // 3K
           case 5: XY_Space = Smooth_Com.Smooth_XY_Space>>1; break;     // 2K
       }

       for (i = 0; i < 5; i++) {
           S3_p = &A_L_S3[M_Mode][i];
           for (j = 0; j < S3_p->Sum; j++) {
               I3_Idx_S = S3_p->Source_Idx[j];
               I3_p0 = &A_L_I3[M_Mode][I3_Idx_S];
               for (k = -XY_Space; k <= XY_Space; k++) {
                   Source_Idx = j + k;
                   Value = XY_Space + 2 - abs(k);
                   //     if (k == 0) Value *= 2;
                   if (((Source_Idx >= 0) && (Source_Idx < S3_p->Sum)) || (i == 0)) {
                       Source_Idx = (Source_Idx + S3_p->Sum) % S3_p->Sum;
                       I3_Idx2 = S3_p->Source_Idx[Source_Idx];
                       for (L = 0; L < I3_p0->Smooth_Total; L++) {
                           if (I3_Idx2 == I3_p0->Smooth_P[L]) {
                               I3_p0->Smooth_V[L]  += Value;
                               I3_p0->Smooth_Sum   += Value;
                               break;
                           }
                       }
                       if (L == I3_p0->Smooth_Total) {
                           I3_p0->Smooth_P[L]  = I3_Idx2;
                           I3_p0->Smooth_V[L]  = Value;
                           I3_p0->Smooth_Sum   += Value;
                           I3_p0->Smooth_Total++;
                       }
                   }
              }
           }
       }
   }
}

int Smooth_XY_Space = 2;
void Smooth_I_Init(void)
{
    int i, j, k;
    memset(&Smooth_I_Data[0][0], 0, sizeof(Smooth_I_Data) );

    for(i = 0; i < 6; i++) {
      for(k = 0; k < 32; k++) {
        for(j = 0; j < 512; j++) {
//              Smooth_Z_I[i][j][k].Value0 = 1000;
//              Smooth_Z_I[i][j][k].Count0 = 63;
//              Smooth_Z_I[i][j][k].Weight = 0;

              Smooth_Z_V_I[i][j][k].Value0 = 1000;
              Smooth_Z_V_I[i][j][k].Count0 = 63;
              Smooth_Z_V_I[i][j][k].Weight = 0;

              Smooth_Z_H_I[i][j][k].Value0 = 1000;
              Smooth_Z_H_I[i][j][k].Count0 = 63;
              Smooth_Z_H_I[i][j][k].Weight = 0;
        }
      }

      for(k = 64; k < 67; k++) {
        for(j = 0; j < 512; j++) Smooth_I_Data[i][k][j] = 0;
      }
    }

#ifndef ANDROID_CODE
    Smooth_Com.Global_phi_Top       = 0;    //A_L_I_Global_phi2_Default;
    Smooth_Com.Global_phi_Mid       = 0;    //A_L_I_Global_phi_Default;
    Smooth_Com.Smooth_Auto_Rate     = 100;  // 100 : Auto 0: Manual
#endif
    Smooth_Com.Global_phi_Btm       = (50.0 * (float)RULE_UNIT[LensCode] / 64.0) / 4;
    if(Smooth_Com.Global_phi_Btm > 63) Smooth_Com.Global_phi_Btm  = 63;
    Smooth_Com.Smooth_Debug_Flag    = 0;                        // rex+ 191014
    Smooth_Com.Smooth_Avg_Weight    = 25;                       // rex+ 191014
    Smooth_Com.Smooth_Base_Level    = 600;
    Smooth_Com.Smooth_Pre_Weight    = 10;
    Smooth_Com.Smooth_Low_Level     = 30;
    Smooth_Com.Smooth_XY_Space      = BSmooth_XY_Space;        // rex+ 180921, 2->適合小圖, 6->適合大圖
    Smooth_Com.Smooth_Pre_Space     = 3;
    Smooth_Com.Smooth_Manual_Space  = 8;
    Smooth_Com.Smooth_Manual_Weight = 4;
    Smooth_Com.Smooth_Weight_Th     = 4;

#ifndef ANDROID_CODE
//    Smooth_Z_I[3][11][ 8].Value0 = 400;
//    Smooth_Z_I[3][9 ][ 9].Value0 = 500;
//
//    Smooth_Z_I[3][8 ][ 7].Value0 = 400;
//
//    Smooth_Z_I[3][16][31].Value0 = 340;
//
//    Smooth_Z_I[3][52][25].Value0 = 340;

    for(i = 0; i < 2; i++) {
        Smooth_O[i][3][10].Z = 15;
        Smooth_O[i][3][11].Z = 17;
        Smooth_O[i][3][20].Z = 18;
        Smooth_O[i][3][21].Z = 19;
    }
#endif
    memset(Smooth_T_Temp, 0, sizeof(Smooth_T_Temp));
}


void Smooth_O_Init(void)
{
    int i, j, k, f_id; int Z;
    memset(&Smooth_O[0][0][0], 0, sizeof(Smooth_O) );

#ifdef ANDROID_CODE
   Z = 30;
#else
   Z = 4;
#endif

   for(f_id = 0; f_id < 2; f_id++) {
    for(i = 0; i < 6; i++) {
        for(j = 0; j < 512; j++) {
               Smooth_O[f_id][i][j].Z = Z;
        }
        for(k = 64; k < 67; k++) {
            for(j = 0; j < 512; j++) Smooth_I_Data[i][k][j] = 0;
        }
    }
   }
}

void Smooth_Run(int Img_Mode, int C_Mode)
{
    int i;
    if (check_F_Com_In() == 1) {
        if (Smooth_Init != 0xaaaa6666) {
            Smooth_I_Init();
            Smooth_O_Init();
            Smooth_Init = 0xaaaa6666;
        }
        Do_Auto_Smooth_YUV(Img_Mode);
        Do_Auto_Smooth_Z(Img_Mode, C_Mode);
    }
}
extern int getStitchNumber(void);
extern int ShowSmoothMode;
int getStitchData(int *value,int type, int f_id)
{
    int i = 0;
    int idx = 0, tmp = 0;
    int n = getStitchNumber();

    for(i = 0; i < n; i++){
        switch(type) {
        case 0:
            tmp = Smooth_O[f_id][3][i].Y;
            if(Smooth_O[f_id][3][i].F_Y == 1)
                tmp = tmp * -1;
            break;
        case 1:
            tmp = Smooth_O[f_id][3][i].U;
            if(Smooth_O[f_id][3][i].F_U == 1)
                tmp = tmp * -1;
            break;
        case 2:
            tmp = Smooth_O[f_id][3][i].V;
            if(Smooth_O[f_id][3][i].F_V == 1)
                tmp = tmp * -1;
            break;
        case 3: tmp = Smooth_Debug_Buf[ShowSmoothMode][2][i]; break;
        case 4: tmp = Smooth_Debug_Buf[ShowSmoothMode][1][i]; break;
        case 5: tmp = Smooth_Debug_Buf[ShowSmoothMode][0][i]; break;
        case 6: tmp = Smooth_I_Weight[ShowSmoothMode][66][i]; break;
        case 7: tmp = Smooth_I_Buf[ShowSmoothMode][68][i]; break;
        case 8: idx = Smooth_I_Buf[ShowSmoothMode][67][i]; tmp = Smooth_I_Buf[ShowSmoothMode][idx][i]; break;
        case 10: tmp = Smooth_I_AVG[0][i][0]; break;
        case 11: tmp = Smooth_I_AVG[0][i][1]; break;
        case 12: tmp = Smooth_I_AVG[0][i][2]; break;
        case 255: tmp = i; break;
        }
        *(value + i) = tmp;
    }
    return 256;        // 256*4
}

//----------------- Smooth Debug ---------------------------------

#ifdef ANDROID_CODE
void setSmoothXYSpace(int space){ 
	BSmooth_XY_Space = space; 
	BSmooth_Set = 1; 
	Smooth_Init = -1; 
}     // 100
int getSmoothXYSpace(){ 
	return BSmooth_XY_Space; 
}
void setSmoothFarWeight(int weight){ 
    BSmooth_FarWeight = weight; BSmooth_Set = 1; 
}   // 101
int getSmoothFarWeight(){
	return BSmooth_FarWeight; 
}
void setSmoothDelSlope(int slope){
	BSmooth_DelSlope = slope; BSmooth_Set = 1; 
}       // 102
int getSmoothDelSlope(){
	return BSmooth_DelSlope; 
}
void setSmoothFunction(int func){
	BSmooth_Function = func; 
	BSmooth_Set = 1;
}         // 103
int getSmoothFunction(){
	return BSmooth_Function; 
}
#endif
//-----------------------------------------------------------------------------------

