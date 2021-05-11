/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/
 
#include "Device/US363/Kernel/k_variable.h"
 
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

#include "Device/US363/Kernel/variable.h"
#include "Device/US363/Kernel/us360_define.h"               // rex+ 180307
#include "Device/US363/Kernel/AletaS2_CMD_Struct.h"         // rex+ 180315
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::KVariable"

#define __APK_LIB_SIM__    1

// *** share variable ***
struct FPGA_Speed_Struct FPGA_Speed[6];
//int JPEG_Size[6];

struct Adjust_Line_I3_Header_Struct   A_L_I3_Header[6];
struct Adjust_Line_I3_Line_Struct  A_L_I3_Line[8];
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
sc_cmd_table_struct sc_cmd_rxTable[SC_CT_SIZE] = {
//    Start_Code          , CMD_S[4],               , *Data_P,                       , (*Rx_Func_P)(void *)
//                , CMD_M[4]   , Len_Max                              , (*Tx_Func_P)(void *, int) 
    { SC_CHECK_V, "FPGA", "SYNC", sizeof(FPGA_Speed)      , (char *)FPGA_Speed    , null_Tx, null_Rx, 0},
    //{ SC_CHECK_V, "JPEG", "SYNC", sizeof(JPEG_Size)       , (char *)JPEG_Size     , null_Tx, null_Rx, 0},
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

void do_sc_cmd_Rx_func(char *rx_buf)
{
    int i;
    sc_cmd_table_struct *rx_cmd = (sc_cmd_table_struct *)&rx_buf[0];
    sc_cmd_table_struct *sc_cmd = &sc_cmd_rxTable[0];

    if(rx_cmd->check_V != SC_CHECK_V){
        db_error("do_sc_cmd_Rx_func: err! rx_cmd->check_V=%x\n", rx_cmd->check_V);
        return;
    }

    //db_debug("do_sc_cmd_Rx_func: cmd_M=%c%c%c%c cmd_S=%c%c%c%c data_S=%d data_P=%x\n", 
    //    rx_cmd->cmd_M[0], rx_cmd->cmd_M[1], rx_cmd->cmd_M[2], rx_cmd->cmd_M[3], 
    //    rx_cmd->cmd_S[0], rx_cmd->cmd_S[1], rx_cmd->cmd_S[2], rx_cmd->cmd_S[3], 
    //    rx_cmd->data_S, rx_cmd->data_P);

    for(i = 0; i < SC_CT_SIZE-1; i++, sc_cmd++){
        if(sc_cmd->check_V != SC_CHECK_V){
            return;
        }
        if((*(int *)sc_cmd->cmd_M == *(int *)rx_cmd->cmd_M) &&
           (*(int *)sc_cmd->cmd_S == *(int *)rx_cmd->cmd_S))
        {
            if(sc_cmd->data_S == rx_cmd->data_S){
                memcpy((char *)sc_cmd->data_P, (char *)&rx_buf[16], sc_cmd->data_S);
            }
            else{
                db_error("do_sc_cmd_Rx_func: err! size=%d(!=%d)\n", rx_cmd->data_S, sc_cmd->data_S);
            }
            return;
        }
    }
}

char sc_cmd_Rx_buf[0x100000];
void write_cmd_app2lib(char *data, int size)
{
    if(size > sizeof(sc_cmd_Rx_buf)) return;

    memcpy(sc_cmd_Rx_buf, data, size);
    do_sc_cmd_Rx_func(sc_cmd_Rx_buf);
}

