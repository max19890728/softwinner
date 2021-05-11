/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Cmd/variable.h"

#ifdef __KERNEL__
  #include <linux/module.h>
  #include <linux/kthread.h>
  #include <linux/string.h>
#else
	
  #include <string.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <assert.h>
  #include <fcntl.h>              /* low-level i/o */
#endif

#include "Device/US363/Cmd/us360_define.h"
#include "Device/US363/Cmd/AletaS2_CMD_Struct.h"
#include "Device/US363/Kernel/k_spi_cmd.h"
#include "Device/US363/Kernel/k_variable.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::Variable"

#define __APK_LIB_SIM__    1

/*
struct FPGA_Speed_Struct {
  int ISP2;
  int Stitch_Img;
  int Stitch_ZYUV;
  int Jpeg;
  int H264;
  int DMA;
  int USB;
  int USB_H264;
};*/
// *** share variable ***
// WaveForm 量測結果 (us)
struct FPGA_Speed_Struct FPGA_Speed[6] = {
		{469893, 713822, 26570, 3800000, 724069, 1524069, 663602,  PIPE_USB_SPEED_12K,160291},		//ST:396964		USB:800095		//ST=613822, 因為JPEG雙引擎, 太短可能會有ST與JPEG使用同一塊DDR的狀況
		{125971, 317563, 17735, 1700000, 325985,  932042, 311962,  PIPE_USB_SPEED_8K, 121582},		//ST:213399		USB:283834		//ST=317563, 解HDR加底圖, BUF使用同一塊問題
		{125971, 255890, 14281, 1100000, 215985,  602447, 188793,  PIPE_USB_SPEED_6K,  97828},		//ST:149852		USB:194686		//ST=255890, 解HDR加底圖, BUF使用同一塊問題
		{ 53469,  73809,  8768,  500000,  80484,  235144,  73778,  PIPE_USB_SPEED_4K,  50547},		//ST:73809		USB:63124
		{ 30164,  33333,  6974,  300000,  51524,  200069,  47235,  PIPE_USB_SPEED_3K,  30547},		//ST:52571		USB:44759
		{ 30182,  13949,  4583,  200000,  22919,  100069,  21021,  PIPE_USB_SPEED_2K,  20547}			//ST:31395		USB:22718
};
int JPEG_Size[6] = {0};

struct Adjust_Line_I3_Header_Struct   A_L_I3_Header[6] = {
  { 11520 ,  3, 1,1,3},     //  12K   F4K  // 10 FPS      1   3
  {  7680 ,  0, 2,2,2},     //   8K   F3K  // 10 FPS      2   4
  {  6144 ,  0, 2,2,2},     //   6K   F3K  // 10 FPS      2   4
  {  3840 ,  3, 3,3,1},     //   4K   X    // 30 FPS      3F
  {  3072 ,  0, 4,2,2},     //   X    F3K  // 30 FPS          4
  {  2048 ,  0, 6,2,3}      //   X    F2K  // 30 FPS          6
};
struct Adjust_Line_I3_Line_Struct  A_L_I3_Line[8] = {
  { 1, 1 , 4 },
  { 1, 3 , 4 },
  { 1, 3 , 2 },
  { 1, 1 , 2 },
  { 0, 0 , 4 },
  { 0, 0 , 3 },
  { 0, 0 , 2 },
  { 0, 0 , 1 },
};
struct Adjust_Line_S2_Struct A_L_S2[5];
struct Adjust_Line_S3_Struct A_L_S3[6][5];
struct Adjust_Line_I3_Struct A_L_I3[6][512];

extern CIS_CMD_struct FS_TABLE[CIS_TAB_N];
extern CIS_CMD_struct D2_TABLE[CIS_TAB_N];
extern CIS_CMD_struct D3_TABLE[CIS_TAB_N];

extern struct ST_Header_Struct  ST_Header[6];                      // rex+, 180315
extern struct ST_Header_Struct  ST_S_Header[5];
extern struct ST_Header_Struct  ST_3DModel_Header[8];

extern AS2_F0_MAIN_CMD_struct FX_MAIN_CMD_P[2];
extern AS2_F0_MAIN_CMD_struct FX_MAIN_CMD_Q[64];
extern AS2_F2_MAIN_CMD_struct F2_MAIN_CMD_P[2];
extern AS2_F2_MAIN_CMD_struct F2_MAIN_CMD_Q[64];


// *** function ***
static void null_Rx(void *data){ return; }
static void null_Tx(void *data, int dmax){ return; }

#define SC_CHECK_V        0x20180302
#define SC_CT_SIZE        0x100
/*
 * SC_CMD = stream control command
 */
typedef struct sc_cmd_table_struct_H
{
    unsigned    check_V;            // check version, 0x20180302
    char        cmd_M[4];           // command main
    char        cmd_S[4];           // command sub
    unsigned    data_S;             // data size

    char        *data_P;            // data point

    void        (*do_Tx)(void *, int);
    void        (*do_Rx)(void *);
    unsigned    rev;
} sc_cmd_table_struct;

//SC_CT_SIZE =256
sc_cmd_table_struct sc_cmd_txTable[SC_CT_SIZE] = {
//    Start_Code          , CMD_S[4],               , *Data_P,                       , (*Rx_Func_P)(void *)
//                , CMD_M[4]   , Len_Max                              , (*Tx_Func_P)(void *, int) 
    { SC_CHECK_V, "FPGA", "SYNC", sizeof(FPGA_Speed)      , (char *)FPGA_Speed    , null_Tx, null_Rx, 0},
    { SC_CHECK_V, "JPEG", "SYNC", sizeof(JPEG_Size)       , (char *)JPEG_Size     , null_Tx, null_Rx, 0},
    { SC_CHECK_V, "I3HE", "SYNC", sizeof(A_L_I3_Header)   , (char *)A_L_I3_Header , null_Tx, null_Rx, 0},
    { SC_CHECK_V, "I3LI", "SYNC", sizeof(A_L_I3_Line)     , (char *)A_L_I3_Line   , null_Tx, null_Rx, 0},
    { SC_CHECK_V, "ALI3", "SYNC", sizeof(A_L_I3)          , (char *)A_L_I3        , null_Tx, null_Rx, 0},
    { SC_CHECK_V, "ALS2", "SYNC", sizeof(A_L_S2)          , (char *)A_L_S2        , null_Tx, null_Rx, 0},
    { SC_CHECK_V, "ALS3", "SYNC", sizeof(A_L_S3)          , (char *)A_L_S3        , null_Tx, null_Rx, 0},
//    { SC_CHECK_V, "CIST", "CPDS", sizeof(DS_TABLE)        , (char *)DS_TABLE      , null_Tx, null_Rx, 0},
    { SC_CHECK_V, "CIST", "CPFS", sizeof(FS_TABLE)        , (char *)FS_TABLE      , null_Tx, null_Rx, 0},
    { SC_CHECK_V, "CIST", "CPD2", sizeof(D2_TABLE)        , (char *)D2_TABLE      , null_Tx, null_Rx, 0},
    { SC_CHECK_V, "CIST", "CPD3", sizeof(D3_TABLE)        , (char *)D3_TABLE      , null_Tx, null_Rx, 0},
    { SC_CHECK_V, "MKST", "HEAD", sizeof(ST_Header)       , (char *)ST_Header     , null_Tx, null_Rx, 0},    // 384(bytes)
    { SC_CHECK_V, "S2F0", "CMDP", sizeof(FX_MAIN_CMD_P)   , (char *)FX_MAIN_CMD_P , null_Tx, null_Rx, 0},
    { SC_CHECK_V, "S2F0", "CMDQ", sizeof(FX_MAIN_CMD_Q)   , (char *)FX_MAIN_CMD_Q , null_Tx, null_Rx, 0},
    { SC_CHECK_V, "S2F2", "CMDP", sizeof(F2_MAIN_CMD_P)   , (char *)F2_MAIN_CMD_P , null_Tx, null_Rx, 0},
    { SC_CHECK_V, "S2F2", "CMDQ", sizeof(F2_MAIN_CMD_Q)   , (char *)F2_MAIN_CMD_Q , null_Tx, null_Rx, 0},
    0
};


void write_cmd_app2kerel(char *data, int size)
{
    FILE *fp;
    fp = fopen("/proc/s2/cmd", "wb");
    if(fp >= 0){
        fwrite(data, size, 1, fp);
        fclose(fp);
    }
    else{
        db_error("write_cmd_app2kerel: err.1 fp=%d\n", (int)fp);
    }
}

char sc_cmd_Tx_buf[0x100000];
void do_sc_cmd_Tx_func(char *cmd_M, char *cmd_S)
{
    int i;
    sc_cmd_table_struct *sc_cmd = &sc_cmd_txTable[0];
    for(i = 0; i < SC_CT_SIZE; i++, sc_cmd++){
        if((*(int *)sc_cmd->cmd_M == *(int *)cmd_M) &&
           (*(int *)sc_cmd->cmd_S == *(int *)cmd_S))
        {
            if((sc_cmd->data_S+16) < sizeof(sc_cmd_Tx_buf)){
                memcpy(&sc_cmd_Tx_buf[0], sc_cmd, 16);
                memcpy(&sc_cmd_Tx_buf[16], sc_cmd->data_P, sc_cmd->data_S);
                #ifdef __APK_LIB_SIM__
                write_cmd_app2lib(sc_cmd_Tx_buf, sc_cmd->data_S+16);
                #else
                write_cmd_app2kerel(sc_cmd_Tx_buf, sc_cmd->data_S+16);
                #endif
            }
            else{
                db_error("do_sc_cmd_Tx_func: overflow! size=%d\n", (sc_cmd->data_S + 12));
            }
            return;
        }
    }
}

