/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/
 
#ifndef __K_SPI_CMD_H__
#define __K_SPI_CMD_H__

#include "Device/US363/Kernel/AletaS2_CMD_Struct.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
// DW 0
   unsigned     Com_Size       :18;  //Target  //Data base 512Byte   //Video base 32Byte(Burst)    0x200C0
   unsigned     Com_I_M        :5;   // M   //Max 15 burst                                         0xC
   unsigned     Com_O_N        :5;   // N   //Max 16 burst                                         0x6
   unsigned     Com_Size_H     :1;   //2017.12  為了輸出可以到4K +1bit                             1
   unsigned     Com_S_IM_DDR   :1;   //0:Use DW1   1:Use from Read DDR                             1
   unsigned     Com_XI_Size_H   :2;                                                              //1
// DW 1
   unsigned     Com_S_DDR_P    :24;  //source DDR address   (22~0  23bit  256MByte)         9B8000
   unsigned     Com_XI_Size    :8;   //source X size(burst)                                 80

// DW 2
   unsigned     Com_T_DDR_P    :24;  //target DDR address   (22~0  23bit  256MByte)         5A0200
   unsigned     Com_Check      :7;   //check same run                                       0x15
   unsigned     Com_Data_Video :1;   //0:Data Mode  1:Video Mode                            0x1

// DW 3
   unsigned     Com_X_Step     :16;   // 4.12                                               2000
   unsigned     Com_Y_Step     :15;   //7.8                                                   // 4000
   unsigned     Anti_Flag      : 1;
}Com_Video_struct;

typedef struct {    //16 * 8 = 128 Bytes
    AS2_CMD_IO_struct     S_DDR_P;        //   0
    AS2_CMD_IO_struct     T_DDR_P0;       //   1
    AS2_CMD_IO_struct     T_DDR_P1;       //   2
    AS2_CMD_IO_struct     T_DDR_P2;       //   3
    AS2_CMD_IO_struct     T_DDR_P3;       //   4
    AS2_CMD_IO_struct     Col_Size;       //   5
    AS2_CMD_IO_struct     HeightC01;      //   6
    AS2_CMD_IO_struct     HeightC23;      //   7
    AS2_CMD_IO_struct     HeightC45;      //   8
    AS2_CMD_IO_struct     HeightC67;      //   9
    AS2_CMD_IO_struct     HeightC89;      //  10
    AS2_CMD_IO_struct     HeightCAB;      //  11
    AS2_CMD_IO_struct     HeightCCD;      //  12
    AS2_CMD_IO_struct     HeightCEF;      //  13
    AS2_CMD_IO_struct     Rev;            //  14
    AS2_CMD_IO_struct     Start_En;       //  15
}IMCP_struct;        //128 Byte

#ifdef __KERNEL__
typedef struct dev_fpga_cmd_struct_h
{
    unsigned char   rw;                 // 0xF0 -> write, 0xF1 -> read
    unsigned char   cmd;
    unsigned char   size[2];            // 0x0123 -> size[0]=0x01, size[1]=0x23
} dev_fpga_cmd_struct;

typedef struct F2_DATA_HI_LO_STRUCT_H
{
   // F2: data_lo, (temp_y & temp_x) = ddr_addr (23 downto 3), DW
   unsigned temp_x            : 7;
   unsigned temp_y            : 14;
   unsigned offset_y          : 4;    //110103
   unsigned y14               : 3;
   unsigned rev1              : 4;    //110103

   // F2: data_hi,
   unsigned x_start_addr      : 7;
   unsigned x_stop_addr       : 8;
   unsigned y_counter         : 11;
   unsigned rev2              : 1;
   unsigned burst_mode        : 1;
   unsigned addr_mask_mode    : 4;   // 4 bytes
}F2_DATA_HI_LO_STRUCT;
#endif	//__KERNEL__

void AS2_CMD_Start(void);
int get_Sensor_Input_Scale(void);
int run_Line_YUV_Offset_Step_S2(int *yuv_ost);
int set_A2K_Shuter_Speed(int shuter, int exp_long);
void set_AEG_EP_Var(int en, int frm, int time, int gain);
void set_A2K_JPEG_EP_v(int exp_n, int exp_m, int iso);
void set_A2K_ISP1_Timeout(int f0s0, int f0s1, int f0s2, int f1s0, int f1s1, int f1s2);
void set_A2K_ISP2_NR3D(int level, int rate);
int get_run_big_smooth(void);
void set_A2K_ISP2_UVtoRGB(int idx, int value);
void get_A2K_ISP2_UVtoRGB(int *value);
void set_A2K_ISP1_RGB_Gain(int MR, int MG, int MB, int GR, int GG, int GB);
void set_A2K_JPEG_Color(int color);
void set_A2K_JPEG_Tint(int tint);
void set_A2K_JPEG_WB_Mode(int mode);
void set_A2K_JPEG_Sharpness(int sharpness);
void set_AEG_System_Freq_NP(int np);
void set_A2K_JPEG_Tone(int tone);
void set_A2K_JPEG_Contrast(int contrast);
int set_A2K_LuxValue(int val);
void set_A2K_JPEG_Smooth_Auto(int rate);
int chk_Line_YUV_Offset_Step_S3(void);
void run_Line_YUV_Offset_Step_S3(void);
int get_A2K_JPEG_3D_Res_Mode(void);
void set_A2K_DMA_CameraPosition(int cp_mode);
void set_A2K_JPEG_GPS_v(int en, int *la, int la_CC, int *lo, int lo_CC, int alt);
void init_k_spi_proc(void);
void set_A2K_Live_CMD(int c_mode, int t_mode, int m_mode, int s_mode);
void do_A2K_Live_CMD(int enc_type);
void do_A2K_Sensor_CMD(void);
void set_A2K_Debug_Mode(int debug, int addr, int sensor);
void set_A2K_Sensor_CMD(int sync_mode, int ep_time, int mainfps, int hdrLevel);
void set_A2K_JPEG_Quality_lst(int Quality_lst);
void Set_ST_S_Cmd_Debug_Flag(int value);
int Get_ST_S_Cmd_Debug_Flag(void);
void set_A2K_ISP2_Defect_En(int en);
void check_spi_irq_state(char *str, int input);
void set_A2K_ISP1_RGB(int ost_I, int ost_O);
void AS2_Diffusion_Address_Set(AS2_Diffusion_CMD_struct *SP);
void MainCmdInit(void);
void set_A2K_JPEG_Saturation_C(int saturation_c);
void Set_FPGA_Pipe_Idx(int idx);
void AS2_CMD_Start2(void);
int get_A2K_ST_3DModel_Idx(int idx);
void SetWaveDebug(int id, int data);
void set_A2K_Stitch_CMD_v(int en);
int set_A2K_LensCode(int code);
void set_A2K_Smooth_En(int en);
void get_A2K_DMA_BottomMode(int *mode, int *size);
void get_A2K_DMA_BottomTextMode(int *mode);
void get_A2K_DMA_CameraPosition(int *cp_mode);
void get_AEG_EP_Var(int *frm, int *time, int *gain);
int get_AEG_System_Freq_NP(void);
int get_A2K_Shuter_Speed(void);
int get_A2K_Sensor_HDR_Level(void);
void get_wdr_dif_addr(int i_page, int *addr_a, int *addr_b, int *addr_c);
void get_A2K_Live_CMD(int *c_mode, int *t_mode, int *m_mode, int *s_mode);
void get_A2K_H264_Init(int *fs_init);
void set_A2K_H264_Init(int fs_init);
void run_Line_YUV_Offset_Step_S1S4(int idx);
int read_Sensor_Busy(void);
int get_A2K_Debug_Defect_Step(void);
void Make_Main_Cmd(int idx, int *bulb_st);
int K_SPI_Write_IO_S2( int io_idx, int *data , int size);
int K_SPI_Read_IO_S2( int io_idx, int *data, int size);
void k_ua360_spi_ddr_read(unsigned read_addr, char *read_buf, int read_size, int f_id, int fx_x);
int k_ua360_spi_ddr_write(unsigned write_addr, unsigned char *write_buff, unsigned write_size);
void set_A2K_Debug_Focus(int sensor);
void set_A2K_Debug_Focus_Tool(int num);
void get_A2K_JPEG_EP_v(int *exp_n, int *exp_m, int *iso);
void get_A2K_JPEG_GPS_v(int *la, int la_S, int *la_CC, 
                        int *lo, int lo_S, int *lo_CC, int *alt);
int get_A2K_JPEG_GPS_En(void);						

#ifdef __KERNEL__
int k_spi_fpga_cmd(int rw, int cmd, int *data, int size);
void k_ua360_spi_ddr_read(unsigned read_addr, char *read_buf, int read_size, int f_id);
int k_ua360_spi_ddr_write(unsigned write_addr, char *write_buff, int write_size);
#endif

#ifdef __cplusplus
}   // extern "C"
#endif

#endif    // __K_SPI_CMD_H__
