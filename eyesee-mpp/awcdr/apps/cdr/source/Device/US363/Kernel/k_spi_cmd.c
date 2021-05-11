/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/
 
#include "Device/US363/Kernel/k_spi_cmd.h"

#ifdef __KERNEL__
  #include <linux/init.h>
  #include <linux/module.h>
  #include <linux/kernel.h>
  #include <linux/unistd.h>

  #include <linux/kthread.h>    // rex+ 180316
#else
  #include <pthread.h>
#endif
#include <stdio.h>

#ifndef __KERNEL__
  #include "Device/US363/Cmd/us363_spi.h"
#endif	//__KERNEL__

#include "Device/US363/Kernel/us360_define.h"
#include "Device/US363/Kernel/AletaS2_CMD_Struct.h"
#include "Device/US363/Kernel/FPGA_Pipe.h"
#include "Device/US363/Kernel/jpeg_header.h"        // make jpeg cmd
#include "Device/US363/Kernel/variable.h"           // make stitch cmd
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::KSPICmd"


AS2_F0_MAIN_CMD_struct FX_MAIN_CMD_P[2];
AS2_F0_MAIN_CMD_struct FX_MAIN_CMD_Q[FPGA_ENGINE_P_MAX];
AS2_F2_MAIN_CMD_struct F2_MAIN_CMD_P[2];
AS2_F2_MAIN_CMD_struct F2_MAIN_CMD_Q[FPGA_ENGINE_P_MAX];

struct ST_Header_Struct  ST_Header[6];
struct ST_Header_Struct  ST_S_Header[5];        // RAW / Removal
struct ST_Header_Struct  ST_3DModel_Header[8];

#define swap4(x) ( (x & 0xFF) << 24) | ( (x & 0xFF00) << 8) | ( (x >> 8) & 0xFF00) | (( x >> 24) & 0xFF)
int run_big_smooth = 0;          // 拍照做大圖縫合
int set_run_big_smooth(int run)
{
    run_big_smooth = (run&0x1);
}
int get_run_big_smooth(void)
{
    return run_big_smooth;
}
static int check_spi_irq_cnt=0;
inline void check_spi_irq_state(char *str, int input)
{
    if(input == 1){
        check_spi_irq_cnt ++;
        if(check_spi_irq_cnt != 1){
            db_error("%s: err.1! cnt=%d\n", str, check_spi_irq_cnt);
        }
    }
    else{
        check_spi_irq_cnt --;
        if(check_spi_irq_cnt != 0){
            db_error("%s: err.2! cnt=%d\n", str, check_spi_irq_cnt);
        }
    }
}

/*void ua360_spi_ddr_read_S2( int read_addr, int *read_buf, int read_size, int f_id, int fx_x)
{
#ifdef __KERNEL__
    k_ua360_spi_ddr_read(read_addr, read_buf, read_size, f_id);
#else
    ua360_spi_ddr_read(read_addr, read_buf, read_size, f_id, fx_x);
#endif
}
int ua360_spi_ddr_write_S2( int write_addr, int *write_buff, int write_size)
{
    if(write_size > 4000) printk("ua360_spi_ddr_write_S2: size overflow!");
#ifdef __KERNEL__
    return k_ua360_spi_ddr_write(write_addr, write_buff, write_size);
#else
    return ua360_spi_ddr_write(write_addr, write_buff, write_size);
#endif
}*/

#ifdef __KERNEL__
void make_F2_Addr_Cmd(void *data, unsigned addr, unsigned size ,unsigned char mode)
{
    F2_DATA_HI_LO_STRUCT *dp;

    dp                 = (F2_DATA_HI_LO_STRUCT *)data;

    dp->temp_x         = addr >> 5;         // 1 burst = 32 bytes
    dp->temp_y         = addr >> 12;// + mode ; //( Form1->RadioGroup_mem_ctl2->ItemIndex == 0 ) ? addr >> 12 : (addr >> 12) + 1;        // 1 line = 4096 bytes
    dp->offset_y       = 1;
    dp->y14            = ( addr >> 26 ) & 0x7;
    dp->rev1           = 0;

    dp->x_start_addr   = addr >> 5;
    dp->x_stop_addr    = (addr + size) > 4096 ?  (addr + size - dp->temp_y * 0x1000) >> 5 : (addr + size ) >> 5 ;
    dp->y_counter      = ((addr - (dp->temp_y << 12)) + size + 4095) >> 12 ;
    dp->burst_mode       = 0;//mode ; //( Form1->RadioGroup_mem_ctl2->ItemIndex == 0 ) ? 0 : 1;
    dp->addr_mask_mode = 0;
    dp->rev2           = 0;
}

void make_ddr_read_cmd(unsigned char *cbuf, unsigned addr, unsigned burst ,unsigned char mode, unsigned f_id)
{
    unsigned cmd_len, size;
    unsigned *uP1;
    dev_fpga_cmd_struct fpga_cmd;

    size         = burst << 5;             //For DDR2 size   ,32 bytes?1??

    // FPGA cmd
    cmd_len          = sizeof(dev_fpga_cmd_struct);
    if(f_id == 0) 	   fpga_cmd.rw = 0xF2;
    else if(f_id == 1) fpga_cmd.rw = 0xF3;
    else if(f_id == 2) fpga_cmd.rw = 0xF1;
    fpga_cmd.cmd     = 0x01;
    fpga_cmd.size[0] = ((burst>>8)&0xff);
    fpga_cmd.size[1] = (burst&0xff);
    memcpy(cbuf, &fpga_cmd, cmd_len);
    cbuf += cmd_len;

    // DDR HI LO command
    make_F2_Addr_Cmd((void *)cbuf, addr, size, mode);

    uP1  = (unsigned *)cbuf;
    *uP1 = swap4(*uP1);
    uP1  ++;
    *uP1 = swap4(*uP1);
    cbuf += sizeof(F2_DATA_HI_LO_STRUCT);
}

void make_ddr_write_cmd(unsigned char *cbuf, unsigned char *data, unsigned addr, unsigned burst ,unsigned char mode)
{
    unsigned i, cmd_len, size;
    unsigned *uP1;
    dev_fpga_cmd_struct fpga_cmd;

    size = burst << 5;

    // FPGA command
    cmd_len          = sizeof(dev_fpga_cmd_struct);
    fpga_cmd.rw      = 0xF0;
    fpga_cmd.cmd     = 0x01;
    fpga_cmd.size[0] = ((burst>>8)&0xff);
    fpga_cmd.size[1] = (burst&0xff);
    memcpy(cbuf, &fpga_cmd, cmd_len);
    cbuf += cmd_len;

    // DDR HI LO command
    make_F2_Addr_Cmd((void *)cbuf, addr, size, mode);

    uP1  =  (unsigned *)cbuf;
    *uP1 =  swap4(*uP1);
    uP1  ++;
    *uP1 =  swap4(*uP1);
    cbuf += sizeof(F2_DATA_HI_LO_STRUCT);
    memcpy(cbuf, data, size);
}

char spi_fpga_cmd_buf[4100];
char spi_ddr_read_buf[5120];
char spi_ddr_write_buf[5120];
extern int spidev_360_write_and_read(char *wptr, int wlen, char *rptr, int rlen);

#define DDR_ON_DVR_SIZE		0x20000000	// 512MB
int k_spi_fpga_cmd(int rw, int cmd, int *data, int size)
{
    int len=4, dw, ret=1, status;
    dev_fpga_cmd_struct cmd_fpga;
    //int file = spi_init();

    // check
    if((size > 4096) || ((size & 0x3) != 0))
    {    // 1次最多 512bytes
        db_error("k_spi_fpga_cmd: (size > 512) || ((size & 0x3)  err!\n");
        return -6;
    }
    //pthread_mutex_lock(&pthread_spi_mutex);
    check_spi_irq_state("k_spi_fpga_cmd", 1);
    memset(&spi_fpga_cmd_buf[0], 0, sizeof(spi_fpga_cmd_buf));

    if(rw == 0) cmd_fpga.rw = 0xF1;             // read
    else        cmd_fpga.rw = 0xF0;             // write

    cmd_fpga.cmd     = cmd;                     // 0x09 -> MIPS I/O
    dw = ((size+3)>>2);                         // FPGA I/O 以 4bytes 為單位
    cmd_fpga.size[0] = ((dw>>8)&0xff);          // 高低位元對調
    cmd_fpga.size[1] = (dw&0xff);

    memcpy(&spi_fpga_cmd_buf[0], &cmd_fpga, len);       // cmd
    if(rw == 1){
        memcpy(&spi_fpga_cmd_buf[len], data, size);     // data
        status = spidev_360_write_and_read(spi_fpga_cmd_buf, len+size, NULL, 0);
    }
    else if(rw == 0){
        status = spidev_360_write_and_read(spi_fpga_cmd_buf, len, (char *)data, size);
    }
    if (status < 0){
        db_error("k_spi_fpga_cmd: status < 0  err!\n");
        check_spi_irq_state("k_spi_fpga_cmd", 0);
        return -7;        // "SPI Write Err
    }
    check_spi_irq_state("k_spi_fpga_cmd", 0);
    //pthread_mutex_unlock(&pthread_spi_mutex);
    return ret;
}
#endif    // __KERNEL__

void k_ua360_spi_ddr_read(unsigned read_addr, char *read_buf, int read_size, int f_id, int fx_x)
{
#ifdef __KERNEL__
    unsigned char cmd_buf[128], mode='\0';
    unsigned cmd_len=0, i, num=0; int status;

    cmd_len = sizeof(dev_fpga_cmd_struct) + sizeof(F2_DATA_HI_LO_STRUCT);
    if((read_size <= 0) || ((read_size % 32) != 0) || ((read_addr % 4) != 0) ||
       (cmd_len > sizeof(cmd_buf)) || ((read_size+20) > sizeof(spi_ddr_read_buf)))
    {
        db_error("k_ua360_spi_ddr_read: err!\n");
        return -3;
    }

    if(f_id < 2 && read_size > 2048) {
    	db_error("k_ua360_spi_ddr_read: f_id=%d size=%d\n", f_id, read_size);
    	return -3;
    }

    //Write IO to Read F0/F1
    unsigned Data[2];
    if(f_id < 2) {
		Data[0] = 0xF10;
		Data[1] = (read_addr >> 5);
		K_SPI_Write_IO_S2(0x9, &Data[0], 8);    // SPI_W
    }

    //pthread_mutex_lock(&pthread_spi_mutex);
    check_spi_irq_state("k_ua360_spi_ddr_read", 1);
    make_ddr_read_cmd(cmd_buf, read_addr, read_size>>5, mode, f_id);
    status = spidev_360_write_and_read((char *)&cmd_buf[0], cmd_len, (char *)spi_ddr_read_buf, read_size);
    memcpy(read_buf, spi_ddr_read_buf, read_size);
    
    if (status < 0) {
        db_debug("k_ua360_spi_ddr_read: SPI_Read Fail/n");
        check_spi_irq_state("k_ua360_spi_ddr_read", 0);
        return;
    }
    check_spi_irq_state("k_ua360_spi_ddr_read", 0);
    //pthread_mutex_unlock(&pthread_spi_mutex);
#else
    ua360_spi_ddr_read(read_addr, read_buf, read_size, f_id, fx_x);
#endif	//__KERNEL__
}

int k_ua360_spi_ddr_write(unsigned write_addr, unsigned char *write_buff, unsigned write_size)
{
#ifdef __KERNEL__	
    int status;
    unsigned char mode='\0';        // cmd_buf[5120]
    unsigned cmd_len=0, write_len=0, i, num;

    cmd_len = sizeof(dev_fpga_cmd_struct) + sizeof(F2_DATA_HI_LO_STRUCT);
    if((write_size <= 0) || ((write_size % 32) != 0) || ((cmd_len + write_size) > sizeof(spi_ddr_write_buf)) ||
       ((write_addr % 4) != 0) || ((write_addr + write_size ) > DDR_ON_DVR_SIZE))
    {
        db_error("k_ua360_spi_ddr_write: err! addr=%x size=%x\n", write_addr, write_size);
        return -3;
    }
    //pthread_mutex_lock(&pthread_spi_mutex);
    check_spi_irq_state("k_ua360_spi_ddr_write", 1);
    make_ddr_write_cmd(spi_ddr_write_buf, write_buff, write_addr, write_size>>5, mode);
    status = spidev_360_write_and_read((char *)&spi_ddr_write_buf[0], cmd_len+write_size, NULL, 0);

    if (status < 0) {
        db_error("k_ua360_spi_ddr_write: err! size=%d\n", write_size);
        check_spi_irq_state("k_ua360_spi_ddr_write", 0);
        return -4;        // "SPI Write Err
    }
    check_spi_irq_state("k_ua360_spi_ddr_write", 0);
    //pthread_mutex_unlock(&pthread_spi_mutex);
    return 1;
#else
    return ua360_spi_ddr_write(write_addr, write_buff, write_size);
#endif	//__KERNEL__
}

int K_SPI_Write_IO_S2( int io_idx, int *data , int size)
{
#ifdef __KERNEL__
    return k_spi_fpga_cmd( 1 , (int)io_idx , (int *)data , (int)size);
#else
    return spi_fpga_cmd( 1 , (int)io_idx , (int *)data , (int)size);
#endif
}

int K_SPI_Read_IO_S2( int io_idx, int *data, int size)
{
#ifdef __KERNEL__
    return k_spi_fpga_cmd( 0, io_idx, data, size);
#else
    return spi_fpga_cmd( 0, io_idx, data, size);
#endif
}
void SetWaveDebug(int id, int data)
{
	unsigned *ptr;
	unsigned Data[2], Addr;
	WAVE_CMD_struct wave_s;

	wave_s.ID   = id;
	wave_s.Rev  = 0;
	wave_s.Data = data;
	ptr = &wave_s;

	Addr = 0xF14;
	Data[0] = Addr;
	Data[1] = *ptr;
	K_SPI_Write_IO_S2( 0x9  , &Data[0] , 8 );     // waveform
}

#define CLOSE_USB_CNT	5
typedef struct A2K_Variable_Struct_H
{
    // A2K_Sensor_CMD
    int         EP_Time;        // 單位:us
    int         Shuter_Speed;
    int         FPS_x10;
    int         Sensor_Reset;
    int         Sensor_Adjust;
    int         Change_Mode;
    int	        HDR_Level;
    // A2K_Live_CMD
    int         Camera_Mode;
    int         Time_Lapse;
    int         B_RUL_Mode;     // big mode
    int         S_RUL_Mode;     // small mode
    // A2K_ISP1_CMD
    int         ISP1_RGB_Offset_I;
    int         ISP1_RGB_Offset_O;
    int         ISP1_Timeout_F0[3];
    int         ISP1_Timeout_F1[3];
    //int         ISP1_Color_Matrix[9];
    int         ISP1_RGB_Gain[6];      // {MR, MG, MB, GR, GG, GB}, ISP_Block1.MR & ISP_Block1.Gain_R
    int 		ISP1_Gamma_Rate[4];
    int			ISP1_Gamma_Point[4];
    int			ISP1_Skip_XY_Offset[6][6];		//[M_Mode][RX/RY/BX/BY/B_RY/B_BY]
    int         ISP1_Skip_H_Offset[5][2];	//0:B 1:R
    int         ISP1_Skip_V_Offset[5][2];	//0:B 1:R
    int 		ISP1_Noise_Th;
    // A2K_Debug
    int         DebugJPEGMode;
    int         DebugJPEGaddr;
    int         ISP2_Sensor;
    int         Focus_Tool;
    int         Focus_Sensor;
    int			Defect_Step;
    // A2K_ISP2_CMD
    int         ISP2_NR3D_Level;
    int         ISP2_NR3D_Rate;
    int 		ISP2_YR;
    int 		ISP2_YG;
    int 		ISP2_YB;
    int 		ISP2_UR;
    int 		ISP2_UG;
    int 		ISP2_UB;
    int 		ISP2_VR;
    int 		ISP2_VG;
    int 		ISP2_VB;
    int   		ISP2_Defect_En;
    int 		ISP2_DG_Offset[6];
    // A2K_JPEG_CMD
    int         JPEG_Quality_Mode;
    int         JPEG_Quality_lst;
    int 		JPEG_Live_Quality_Mode;
    int         JPEG_GPS_En;
    int         JPEG_GPS_Latitude[3];
    int         JPEG_GPS_Latitude_CC;
    int         JPEG_GPS_Longitude[3];
    int         JPEG_GPS_Longitude_CC;
    int         JPEG_GPS_Altitude;
    int         JPEG_EXP_N;         // 分子, 1,2,4,8,15,30,60...
    int         JPEG_EXP_M;         // 分母, 1,2,4,8...1000...32000
    int         JPEG_ISO;           // 100,200,400...
    int			JPEG_Color;
    int			JPEG_Saturation_C;
    int			JPEG_Contrast;
    int			JPEG_Tone;
    int			JPEG_Sharpness;
    int			JPEG_Smooth_Auto;
    int			JPEG_Tint;
    int			JPEG_WB_Mode;
    int			JPEG_3D_Res_Mode;	// 0:1152*1152 1:3840*64
    // A2K_Stitch_CMD
    int         ST_Trans_En;        // 0:off, 1:on, 2:auto
    int         ST_Trans_Tbl[6];
    int			ST_3DModel_Idx;
    // A2K_Smooth_CMD
    int         Smooth_En;        // 0:off, 1:on
    // A2K_DMA_CMD
    int 		BottomMode;			// 底圖設定				0:關, 1:延伸, 2:加底圖(default), 3:鏡像, 4:加底圖(user)
    int 		BottomSize;			// 底圖大小				10 ~ 100
    int 		BottomTextMode;		// 底圖文字設定
    int 		CameraPosition;		// 正放/倒放
    // A2K_H264_CMD
    int 		Frame_Stamp_Init;
    // A2K_USB_CMD
    int 		USB_Close_No[CLOSE_USB_CNT];	//切換mode時, 紀錄需要關掉usb的pipe cmd
    // A2K_Cap_Cnt
    int 		cap_file_cnt;
    // A2K_Rec_State
    int 		do_rec_thm;
    int         rec_state;
    // A2K_Diff
    int			WDR_Pixel;
    int			DeGhostEn;
    // A2K_Software_Version
    char		SoftVer[128];
    // A2K_Model
    char		Model[128];
    // A2K_LemsCpde
    int			lensCode;
    // A2K_LuxValue
    int			luxValue;
    // ...
} A2K_Variable_Struct;
//{0, 0, 2, 5, 0, 6}, { {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} }, { {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} }, // A2K_ISP1		//REC 4K
//{0, 0, 0, 5, 0, 6}, { {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} }, { {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} }, // A2K_ISP1		//REC 2K 3K
//{0, 0, 0, 1, 0, 6}, { {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} }, { {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} }, // A2K_ISP1		//Cap 12K
A2K_Variable_Struct A2K = {
    100000, 100000, 100, 0, 0, 0, 4,    // A2K_Sensor_CMD
    0, 0, 0, 0,                 // A2K_Live_CMD

    0, 0, {0, 0, 0}, {0, 0, 0}, /*{0, 0, 0, 0, 0, 0, 0, 0, 0},*/ {0x4000, 0x4000, 0x4000, 0x6000, 0x4000, 0x6000},	// A2K_ISP1
    {0xA8D1, 0xA66F, 0x4D6A, 0x280E}, {0x2C48, 0x2C22, 0x2101, 0x1A00}, 										// A2K_ISP1
    { {0, 0, 0, 1, 0, 4}, {0, 0, 0, 2, 0, 4}, {0, 0, 0, 2, 0, 4}, {0, 0, 2, 5, 0, 0}, {0, 0, 0, 2, 0, 4}, {0, 0, 0, 1, 0, 6} },		// A2K_ISP1
    { {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} }, { {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} }, 24,										// A2K_ISP1

    0, 0, -1, 1, -1, 0,                // A2K Debug
    0, 0x00406070,   0x04C9, 0x0964, 0x01D3, 0x83B7, 0x8748, 0x0B00, 0x0B00, 0x8937, 0x81C8, 0, {0, 0, 0, 0, 0, 0},		// A2K_ISP2
    0, 0, 1, 0, {0,0,0}, 0, {0,0,0}, 0, 0, 0, 0, 0, 5000, 0, 0, 0, 0, 100, 0, 0, 1,         // A2K_JPEG
    1, {1,1,1,0,0,0}, 0,               // A2K_Stitch_CMD
    0,								// A2K_Smooth_CMD
    0, 100, 1, 0,					// A2K_DMA_CMD
    0,							// A2K_H264_CMD
    {-1, -1, -1, -1, -1},		// A2K_USB_CMD
    0,							// A2K_Cap_Cnt
    0, -2,						// A2K_Rec_State
	30, 0,						// A2K_Diff
    0,							// A2K_Software_Version
	0,							// A2K_Model
	0,							// lensCode
	0							// luxValue
};

static int AEG_System_Freq_NP = 0;       // 0:60Hz, 1:50Hz, ISP_AEG_EP_IMX222_Freq
int set_A2K_Shuter_Speed(int shuter, int exp_long)
{
    int sel = exp_long >> 6;
    if     (sel >= 180) shuter = 4000000;  //(180*64), 4s
    else if(sel >= 160) shuter = 2000000;  //(160*64), 2s
    else if(sel >= 140) shuter = 1000000;  //(140*64), 1s
    else if(sel >= 120) shuter = (AEG_System_Freq_NP == 0)? 533334: 640000;  //(120*64), 1/2s, 500000
    else if(sel >= 100) shuter = (AEG_System_Freq_NP == 0)? 266667: 320000;  //(100*64), 1/4s, 250000
    else if(sel >=  80) shuter = (AEG_System_Freq_NP == 0)? 133334: 160000;  // (80*64), 1/8s, 125000
    A2K.Shuter_Speed = shuter;
}
int get_A2K_Shuter_Speed(void)
{
    //db_debug("get_A2K_Shuter_Speed: old=%d new=%d\n", old_shuter_speed, A2K.Shuter_Speed);
    return A2K.Shuter_Speed;
}
void set_A2K_Stitch_CMD_v(int en)
{
    A2K.ST_Trans_En = en;
}
/*
 * 透明度
 *   同 us360_func.c .. GetTransparentEn(int mode)
 */
int get_A2K_doTrans(int mode)
{
	int en = A2K.ST_Trans_En;   // 0:off, 1:on
	if(en == 2){                // 2:auto
        en = A2K.ST_Trans_Tbl[mode];
    }
	return(en&0x1);
}
void set_A2K_ST_3DModel_Idx(int idx)
{
    A2K.ST_3DModel_Idx = idx;
}
int get_A2K_ST_3DModel_Idx(int idx)
{
    return A2K.ST_3DModel_Idx;
}
void set_A2K_Smooth_En(int en)
{
    A2K.Smooth_En = en;
}
int get_A2K_Smooth_En(void)
{
    return A2K.Smooth_En;
}
void set_A2K_JPEG_EP_v(int exp_n, int exp_m, int iso)
{
    A2K.JPEG_EXP_N   = exp_n;
    A2K.JPEG_EXP_M   = exp_m;
    A2K.JPEG_ISO     = iso;
}
void get_A2K_JPEG_EP_v(int *exp_n, int *exp_m, int *iso)
{
    *exp_n      = A2K.JPEG_EXP_N;
    *exp_m      = A2K.JPEG_EXP_M;
    *iso        = A2K.JPEG_ISO;
}
void set_A2K_JPEG_Color(int color)
{
    A2K.JPEG_Color = color;
}
void set_A2K_JPEG_Tint(int tint)
{
    A2K.JPEG_Tint = tint;
}
void set_A2K_JPEG_WB_Mode(int mode)
{
    A2K.JPEG_WB_Mode = mode;
}
void set_A2K_JPEG_Saturation_C(int saturation_c)
{
    A2K.JPEG_Saturation_C = saturation_c;
db_debug("set_A2K_JPEG_Saturation_C() saturation_c=%d\n", saturation_c);
}
void set_A2K_JPEG_Contrast(int contrast)
{
    A2K.JPEG_Contrast = contrast;
}
void set_A2K_JPEG_Tone(int tone)
{
    A2K.JPEG_Tone = tone;
}
void set_A2K_JPEG_Sharpness(int sharpness)
{
    A2K.JPEG_Sharpness = sharpness;
}
void set_A2K_JPEG_Smooth_Auto(int rate)
{
    A2K.JPEG_Smooth_Auto = rate;
}
void set_A2K_JPEG_GPS_v(int en, int *la, int la_CC, int *lo, int lo_CC, int alt)                    
{
    A2K.JPEG_GPS_En = en;
    memcpy(A2K.JPEG_GPS_Latitude, la, sizeof(A2K.JPEG_GPS_Latitude));   // 12(B)
    A2K.JPEG_GPS_Latitude_CC = la_CC;        // 4(B)
    memcpy(A2K.JPEG_GPS_Longitude,lo, sizeof(A2K.JPEG_GPS_Longitude));  // 12(B)
    A2K.JPEG_GPS_Longitude_CC = lo_CC;      // 4(B)
    A2K.JPEG_GPS_Altitude = alt;
}
int get_A2K_JPEG_GPS_En(void)
{
    return A2K.JPEG_GPS_En;
}
void get_A2K_JPEG_GPS_v(int *la, int la_S, int *la_CC, 
                        int *lo, int lo_S, int *lo_CC, int *alt)
{
    memcpy(la, (int *)A2K.JPEG_GPS_Latitude, la_S);
    *la_CC = A2K.JPEG_GPS_Latitude_CC;
    memcpy(lo, (int *)A2K.JPEG_GPS_Longitude, lo_S);
    *lo_CC = A2K.JPEG_GPS_Longitude_CC;
    *alt = A2K.JPEG_GPS_Altitude;
}
void set_A2K_JPEG_Quality_Mode(int mode)
{
    A2K.JPEG_Quality_Mode     = mode;
}
//void get_A2K_JPEG_Quality_Mode(int *mode)
//{
//    *mode = A2K.JPEG_Quality_Mode;
//}

void set_A2K_JPEG_Quality_lst(int Quality_lst)
{
    A2K.JPEG_Quality_lst = Quality_lst;
}
//void get_A2K_JPEG_Quality_lst(int *Quality_lst)
//{
//    *Quality_lst = A2K.JPEG_Quality_lst;
//}
void set_A2K_JPEG_Live_Quality_Mode(int mode)
{
    A2K.JPEG_Live_Quality_Mode = mode;
}
int get_A2K_JPEG_Live_Quality_Mode(void)
{
    return A2K.JPEG_Live_Quality_Mode;
}

void set_A2K_JPEG_3D_Res_Mode(int mode)
{
    A2K.JPEG_3D_Res_Mode = mode;
}

int get_A2K_JPEG_3D_Res_Mode(void)
{
    return A2K.JPEG_3D_Res_Mode;
}

void set_A2K_ISP2_NR3D(int level, int rate)
{
    A2K.ISP2_NR3D_Level = level;
    A2K.ISP2_NR3D_Rate = rate;
}
void set_A2K_ISP2_UVtoRGB(int idx, int value)
{
	switch(idx) {
	case 0: A2K.ISP2_YR = value; break;
	case 1: A2K.ISP2_YG = value; break;
	case 2: A2K.ISP2_YB = value; break;
	case 3: A2K.ISP2_UR = value; break;
	case 4: A2K.ISP2_UG = value; break;
	case 5: A2K.ISP2_UB = value; break;
	case 6: A2K.ISP2_VR = value; break;
	case 7: A2K.ISP2_VG = value; break;
	case 8: A2K.ISP2_VB = value; break;
	}
}
void get_A2K_ISP2_UVtoRGB(int *value)
{
	*value     = A2K.ISP2_YR;
	*(value+1) = A2K.ISP2_YG;
	*(value+2) = A2K.ISP2_YB;
	*(value+3) = A2K.ISP2_UR;
	*(value+4) = A2K.ISP2_UG;
	*(value+5) = A2K.ISP2_UB;
	*(value+6) = A2K.ISP2_VR;
	*(value+7) = A2K.ISP2_VG;
	*(value+8) = A2K.ISP2_VB;
}
void set_A2K_ISP2_Defect_En(int en)
{
	A2K.ISP2_Defect_En = en & 0x1;
}

int get_A2K_ISP2_Defect_En(void)
{
	return A2K.ISP2_Defect_En;
}

void set_A2K_ISP2_DG_Offset(int idx, int offset)
{
	A2K.ISP2_DG_Offset[idx] = offset;
}
int get_A2K_ISP2_DG_Offset(int idx)
{
	return A2K.ISP2_DG_Offset[idx];
}

void set_A2K_Debug_Mode(int debug, int addr, int sensor)
{
    A2K.DebugJPEGMode = debug;
    A2K.DebugJPEGaddr = addr;
    A2K.ISP2_Sensor = sensor;
}
void set_A2K_Debug_Focus(int sensor)
{
    A2K.Focus_Sensor = sensor;
}
int get_A2K_Debug_Focus(void)
{
    return A2K.Focus_Sensor;
}
void set_A2K_Debug_Focus_Tool(int num)
{
    A2K.Focus_Tool = num;
}
int get_A2K_Debug_Focus_Tool(void)
{
    return A2K.Focus_Tool;
}
void set_A2K_Debug_Defect_Step(int step)
{
    A2K.Defect_Step = step;
}
int get_A2K_Debug_Defect_Step(void)
{
    return A2K.Defect_Step;
}
void set_A2K_DMA_BottomMode(int mode, int size)
{
    A2K.BottomMode = mode;
    A2K.BottomSize = size;
}
void get_A2K_DMA_BottomMode(int *mode, int *size)
{
    *mode = A2K.BottomMode;
    *size = A2K.BottomSize;
}
void set_A2K_DMA_BottomTextMode(int mode)
{
    A2K.BottomTextMode = mode;
}
void get_A2K_DMA_BottomTextMode(int *mode)
{
    *mode = A2K.BottomTextMode;
}
void set_A2K_DMA_CameraPosition(int cp_mode)
{
    A2K.CameraPosition = cp_mode;
}
void get_A2K_DMA_CameraPosition(int *cp_mode)
{
	*cp_mode = A2K.CameraPosition;
}
void set_A2K_H264_Init(int fs_init)
{
    A2K.Frame_Stamp_Init = fs_init;
}
void get_A2K_H264_Init(int *fs_init)
{
    *fs_init = A2K.Frame_Stamp_Init;
}
/*
 * MR/MG/MB Gain1
 * GR/GG/GB Gain0
 */
void set_A2K_ISP1_RGB_Gain(int MR, int MG, int MB, int GR, int GG, int GB)
{
    A2K.ISP1_RGB_Gain[0] = MR;
    A2K.ISP1_RGB_Gain[1] = MG;
    A2K.ISP1_RGB_Gain[2] = MB;
    A2K.ISP1_RGB_Gain[3] = GR;
    A2K.ISP1_RGB_Gain[4] = GG;
    A2K.ISP1_RGB_Gain[5] = GB;
}
//int S2_R_Gain = 0x7000, S2_G_Gain = 0x4000, S2_B_Gain = 0x7000;
//void set_A2K_ISP1_Color_Matrix(int *matrix)
//{
//    int i;
//    for(i = 0; i < 9; i++){
//        A2K.ISP1_Color_Matrix[i] = matrix[i];
//    }
//}
void set_A2K_ISP1_Timeout(int f0s0, int f0s1, int f0s2, int f1s0, int f1s1, int f1s2)
{
    A2K.ISP1_Timeout_F0[0] = f0s0;
    A2K.ISP1_Timeout_F0[1] = f0s1;
    A2K.ISP1_Timeout_F0[2] = f0s2;
    A2K.ISP1_Timeout_F1[0] = f1s0;
    A2K.ISP1_Timeout_F1[1] = f1s1;
    A2K.ISP1_Timeout_F1[2] = f1s2;
}
void set_A2K_ISP1_RGB(int ost_I, int ost_O)
{
    A2K.ISP1_RGB_Offset_I = ost_I;
    A2K.ISP1_RGB_Offset_O = ost_O;
}

//void set_A2K_ISP1_Gamma_Rate(int idx, int rate)
//{
//	A2K.ISP1_Gamma_Rate[idx] = rate;
//}
//int get_A2K_ISP1_Gamma_Rate(int idx)
//{
//	return A2K.ISP1_Gamma_Rate[idx];
//}
//void set_A2K_ISP1_Gamma_Point(int idx, int point)
//{
//	A2K.ISP1_Gamma_Point[idx] = point;
//}
//int get_A2K_ISP1_Gamma_Point(int idx)
//{
//	return A2K.ISP1_Gamma_Point[idx];
//}

void set_A2K_ISP1_Skip_Offset(int M_Mode, int idx, int offset)
{
	A2K.ISP1_Skip_XY_Offset[M_Mode][idx] = offset;
}
int get_A2K_ISP1_Skip_Offset(int M_Mode, int idx)
{
	int offset;
	offset = A2K.ISP1_Skip_XY_Offset[M_Mode][idx];
	return offset;
}

void set_A2K_ISP1_Skip_H_Offset(int s_id, int idx, int offset)
{
	A2K.ISP1_Skip_H_Offset[s_id][idx] = offset;
}
int get_A2K_ISP1_Skip_H_Offset(int s_id, int idx)
{
	return A2K.ISP1_Skip_H_Offset[s_id][idx];
}

void set_A2K_ISP1_Skip_V_Offset(int s_id, int idx, int offset)
{
	A2K.ISP1_Skip_V_Offset[s_id][idx] = offset;
}
int get_A2K_ISP1_Skip_V_Offset(int s_id, int idx)
{
	return A2K.ISP1_Skip_V_Offset[s_id][idx];
}

void set_A2K_ISP1_Noise_Th(int value) {
	A2K.ISP1_Noise_Th = value;
	if(A2K.ISP1_Noise_Th < 0)
		A2K.ISP1_Noise_Th = 0;
	else if(A2K.ISP1_Noise_Th > 1023)
		A2K.ISP1_Noise_Th = 1023;
}
int get_A2K_ISP1_Noise_Th(void) {
	return A2K.ISP1_Noise_Th;
}

void set_A2K_Cap_Cnt(int cnt)
{
    A2K.cap_file_cnt = cnt;
}
int get_A2K_Cap_Cnt(void)
{
    return A2K.cap_file_cnt;
}

int get_A2K_FPS(void)
{
    return A2K.FPS_x10;
}
/*
 * chmode : 1->sensor reset
 *          2->sensor adjust
 */
void set_A2K_Sensor_CMD(int sync_mode, int ep_time, int mainfps, int hdrLevel)
{
    A2K.EP_Time = ep_time;
    A2K.FPS_x10 = mainfps;
    if(sync_mode == 1) A2K.Sensor_Reset = 1;
    if(sync_mode == 2) A2K.Sensor_Adjust = 1;
    if(sync_mode == 3) A2K.Change_Mode = 1;
    A2K.HDR_Level = hdrLevel;
}
void do_A2K_Sensor_CMD(void)
{
    int r = 0;
    int ep_time = A2K.EP_Time;
    int fps = A2K.FPS_x10 / 10;
    int d_cnt = read_F_Com_In_Capture_D_Cnt();

    if(A2K.Sensor_Reset == 1){      // sensor reset, change mode
        A2K.Sensor_Reset = 0;
        A2K.Change_Mode = 0;
        A2K.Sensor_Adjust = 0;
        if(d_cnt == 0)
            FPGA_Com_In_Sensor_Reset(ep_time, fps);
    }
    if(A2K.Change_Mode == 1){
    	A2K.Change_Mode = 0;
    	A2K.Sensor_Adjust = 0;
        if(d_cnt == 0)
    	    FPGA_Com_In_Change_Mode(ep_time, fps);
    }
    if(A2K.Sensor_Adjust == 1){     // sensor adjust, cycle 10s
        if(d_cnt == 0){
            r = FPGA_Com_In_Sensor_Adjust(ep_time, fps);
            if(r == 0) A2K.Sensor_Adjust = 0;
        }
        else{
            A2K.Sensor_Adjust = 0;
        }
    }
}

/*
 * return 1 = busy
 */
int adjust_State = 0;
int read_Sensor_Busy(void)
{
    int busy = 0;
    if(adjust_State != 0){
        busy = 1;
        db_debug("**** Sensor: reset=%d change=%d adjust=%d status=%d\n", A2K.Sensor_Reset, A2K.Change_Mode, A2K.Sensor_Adjust, adjust_State);
    }
    return busy;
}

void set_A2K_do_Rec_Thm_CMD(int thm_en)
{
    A2K.do_rec_thm = thm_en;
}

void set_A2K_Rec_State_CMD(int state)
{
    A2K.rec_state = state;
}
int get_A2K_Rec_State_CMD(void)
{
    return A2K.rec_state;
}

int get_A2K_Sensor_HDR_Level(void)
{
    int level = A2K.HDR_Level;
    if     (level <= 2)             level = 3;  // 0918, level由2,4,8改成3,4,6
    else if(level > 2 && level < 8) level = 4;
    else if(level >= 8)             level = 6;
    return level;
}

int set_A2K_Softwave_Version(char *ver)
{
	memcpy(&A2K.SoftVer[0], ver, sizeof(A2K.SoftVer) );
}
int get_A2K_Softwave_Version(char *ver)
{
    memcpy(ver, &A2K.SoftVer[0], sizeof(A2K.SoftVer) );
}
int set_A2K_Model(char *model)
{
	memcpy(&A2K.Model[0], model, sizeof(A2K.Model) );
}
int get_A2K_Model(char *model)
{
	memcpy(model, &A2K.Model[0], sizeof(A2K.Model) );
}
int set_A2K_LensCode(int code)
{
	A2K.lensCode = code;
}
int get_A2K_LensCode(char *ver)
{
    return A2K.lensCode;
}
int get_A2K_FocalLength(){
	if(A2K.lensCode == 0){
		return 2600;
	}else{
		return 2580;
	}
}
int get_A2K_FocalNumber(){
	if(A2K.lensCode == 0){
		return 2000;
	}else{
		return 2200;
	}
}
int set_A2K_LuxValue(int val)
{
	A2K.luxValue = val;
	db_debug("set luxValue: %d\n",val);
}
int get_A2K_LuxValue()
{
	//db_debug("get luxValue: %d\n",A2K.luxValue);
    return A2K.luxValue;
}

int set_A2K_WDRPixel(int val)
{
	A2K.WDR_Pixel = val;
	if(A2K.WDR_Pixel < 1)  A2K.WDR_Pixel = 1;
	if(A2K.WDR_Pixel > 31) A2K.WDR_Pixel = 31;
	db_debug("set_A2K_WDRPixel() WDR_Pixel=%d\n", A2K.WDR_Pixel);
}

int get_A2K_WDRPixel()
{
    return A2K.WDR_Pixel;
}

int set_A2K_DeGhostEn(int en)
{
	A2K.DeGhostEn = en & 0x1;
db_debug("set_A2K_DeGhostEn() en=%d\n", A2K.DeGhostEn);
}

int readIMCPIdx(void)
{
    int i, idx = 0, addr = 0;
    addr = IMCP_IDX_IO_ADDR;
    for(i = 0; i < 3; i ++){
        spi_read_io_porcess_S2(addr, &idx, 4);
        if(idx < 64) break;
    }
    if(idx >= 64) db_error("readIMCPIdx: idx=%d\n", idx);
    return(idx&0x3F);    //IMCP目標BUF只有4組
}
void AS2_Image_Compare_Test(int M_Mode, int IMCP_Idx)
{
	int Addr,i;
	unsigned SP = IMCP_Z_V_LINE_STM1_P0_S_ADDR;
	unsigned TP = IMCP_Z_V_LINE_P0_T_ADDR;
	unsigned *P;
	IMCP_struct IMCP_P;
	int Size1=20, Size2=14;
	int page=0;
	int Size1_64,Size2_64;

	page = (IMCP_Idx - 1) & 0x1;
	if(M_Mode == 3 || M_Mode == 4 || M_Mode == 5) {
		if(page == 0) SP = IMCP_Z_V_LINE_STM2_P0_S_ADDR;
		else          SP = IMCP_Z_V_LINE_STM2_P1_S_ADDR;
	}
	else {
		if(page == 0) SP = IMCP_Z_V_LINE_STM1_P0_S_ADDR;
		else          SP = IMCP_Z_V_LINE_STM1_P1_S_ADDR;
	}
	Size1 = A_L_I3_Line[0].SL_Sum[M_Mode];		//垂直縫合線
	Size2 = A_L_I3_Line[4].SL_Sum[M_Mode];		//水平縫合線
	Size1_64 = (Size1 << 6); Size2_64 = (Size2 << 6);

	memset(&IMCP_P ,0 ,sizeof(IMCP_struct));
	Addr = 0xA0000;
	IMCP_P.S_DDR_P.Address 	    = Addr + (0x0 << 2);	IMCP_P.S_DDR_P.Data 		= SP >> 5;
	IMCP_P.T_DDR_P0.Address 	= Addr + (0x1 << 2);	IMCP_P.T_DDR_P0.Data 		= IMCP_Z_V_LINE_P0_T_ADDR >> 5;
	IMCP_P.T_DDR_P1.Address     = Addr + (0x2 << 2);	IMCP_P.T_DDR_P1.Data 		= IMCP_Z_V_LINE_P0_T_ADDR >> 5;
	IMCP_P.T_DDR_P2.Address     = Addr + (0x3 << 2);	IMCP_P.T_DDR_P2.Data 		= IMCP_Z_V_LINE_P0_T_ADDR >> 5;
	IMCP_P.T_DDR_P3.Address 	= Addr + (0x4 << 2);	IMCP_P.T_DDR_P3.Data 		= IMCP_Z_V_LINE_P0_T_ADDR >> 5;
	IMCP_P.Col_Size.Address 	= Addr + (0x5 << 2);	IMCP_P.Col_Size.Data 		= 16;
	IMCP_P.HeightC01.Address 	= Addr + (0x6 << 2);	IMCP_P.HeightC01.Data 		= (Size1_64 << 16) | (Size1_64);	// 1 / 0		//(64 * Size1 << 16) | (64 * Size1);
	IMCP_P.HeightC23.Address 	= Addr + (0x7 << 2);	IMCP_P.HeightC23.Data 		= (Size1_64 << 16) | (Size1_64);	// 3 / 2
	IMCP_P.HeightC45.Address 	= Addr + (0x8 << 2);	IMCP_P.HeightC45.Data 		= (Size2_64 << 16) | (Size2_64);
	IMCP_P.HeightC67.Address 	= Addr + (0x9 << 2);	IMCP_P.HeightC67.Data 		= (Size2_64 << 16) | (Size2_64);
	IMCP_P.HeightC89.Address 	= Addr + (0xA << 2);	IMCP_P.HeightC89.Data 		= (Size1_64 << 16) | (Size1_64);
	IMCP_P.HeightCAB.Address 	= Addr + (0xB << 2);	IMCP_P.HeightCAB.Data 		= (Size1_64 << 16) | (Size1_64);
	IMCP_P.HeightCCD.Address 	= Addr + (0xC << 2);	IMCP_P.HeightCCD.Data 		= (Size2_64 << 16) | (Size2_64);
	IMCP_P.HeightCEF.Address 	= Addr + (0xD << 2);	IMCP_P.HeightCEF.Data 		= (Size2_64 << 16) | (Size2_64);
	IMCP_P.Start_En.Address 	= Addr + (0xF << 2);	IMCP_P.Start_En.Data 		= 0;	//1;

	P = (unsigned *)&IMCP_P;
	K_SPI_Write_IO_S2( 0x9, P, sizeof(IMCP_P) );      // AS2_Image_Compare_Test
}

void Smooth_CMD_Proc(int mode01, int mode02, int c_mode)
{
	int i;
    int IMCP_Idx = readIMCPIdx();
    static int IMCP_Idx_lst = -1;
    int s3en = chk_Line_YUV_Offset_Step_S3();
    
    if(mode02 == -1){
        clean_Smooth_T_Temp();
    }
    if(run_big_smooth == 1 && mode02 != -1){
        if(s3en > 0){
            Smooth_Line_Trans(mode01, mode02);      // to Smooth_O_Buf[], to Smooth_T_Temp[]
        }
        mode01 = mode02;
        mode02 = -1;
    }
#ifndef __KERNEL__
    if(IMCP_Idx_lst != IMCP_Idx || s3en > 0) {
        IMCP_Idx_lst = IMCP_Idx;
        //if(mode01 == 0)
        //    db_error("Smooth_CMD_Proc: IMCP_Idx=%d\n", IMCP_Idx);   // rex+ 181017
        AS2_Image_Compare_Test(mode01, IMCP_Idx);
        readIMCPdata(mode01, IMCP_Idx);
        Smooth_Run(mode01, c_mode);
        Smooth_I_to_O_Proc(mode01);
        //for(i = 0; i < 2; i++)
        	Send_Smooth_Table(mode01, 0);
        do_ST_Table_Mix(mode01);
    }
    
    if(mode02 != -1){                           // 小圖轉大圖smooth運算
        Smooth_Line_Trans(mode01, mode02);
        Smooth_I_to_O_Proc(mode02);
        //for(i = 0; i < 2; i++)
        	Send_Smooth_Table(mode02, 0);
        do_ST_Table_Mix(mode02);
    }
#endif
}
void set_A2K_Live_CMD(int c_mode, int t_mode, int m_mode, int s_mode)
{
    A2K.Camera_Mode = c_mode;
    A2K.Time_Lapse = t_mode;
    A2K.B_RUL_Mode = m_mode;
    A2K.S_RUL_Mode = s_mode;
}
void get_A2K_Live_CMD(int *c_mode, int *t_mode, int *m_mode, int *s_mode)
{
     *c_mode = A2K.Camera_Mode;
     *t_mode = A2K.Time_Lapse;
     *m_mode = A2K.B_RUL_Mode;
     *s_mode = A2K.S_RUL_Mode;
}
int get_Camera_Mode(void)
{
    return A2K.Camera_Mode;
}
extern int check_F_Com_In(void);                // FPGA_Pipe.c
extern int read_F_Com_In_Capture_D_Cnt(void);   // FPGA_Pipe.c
void do_A2K_Live_CMD(int enc_type)
{
    int c_mode = A2K.Camera_Mode;
    int t_mode = A2K.Time_Lapse;
    int m_mode = A2K.B_RUL_Mode;
    int s_mode = A2K.S_RUL_Mode;
    int ep_time = A2K.EP_Time;
    int fps = A2K.FPS_x10 / 10;
    int d_cnt = 0;
    int cmd_ok = 0;
    int state = A2K.rec_state;
    
    if(check_F_Com_In() == -1) return;
    
    if(c_mode == 1 || c_mode == 10) {               // 1:REC, 10:REC+WDR
        if(m_mode >= 3 && m_mode <= 5) {
            Smooth_CMD_Proc(m_mode, -1, c_mode);
            FPGA_Com_In_Set_Record(m_mode, ep_time, fps, &A2K.do_rec_thm, enc_type, c_mode);
            cmd_ok = 1;
        }
    }
    else if(c_mode == 2 || c_mode == 11) {          // 2:Timelapse, 11:Timelapse+WDR
        if(m_mode >= 0 && m_mode <= 3) {
        	if(state == -2)		//錄影不做調整
            	Smooth_CMD_Proc(m_mode, -1, c_mode);
            FPGA_Com_In_Set_Timelapse(m_mode, ep_time, fps, enc_type, c_mode);
            cmd_ok = 1;
        }
    }
    else if(c_mode == 4) {							// 4:RAW(5 Sensor)
        if(m_mode >= 0 && m_mode <= 2) {
            Smooth_CMD_Proc(s_mode, -1, c_mode);
            FPGA_Com_In_Set_RAW(m_mode, ep_time, fps, enc_type);
            cmd_ok = 1;
        }
    }
    else{
        // c_mode=0,3,5,6,7,8,9,12,13
        //   0:Capture, 3:AEB, 5:HDR, 6:Night, 7:N-HDR
        //   8:Sport, 9:S+WDR, 12:M-Mode, 13:Removal
        if(m_mode >= 0 && m_mode <= 2) {
            d_cnt = read_F_Com_In_Capture_D_Cnt();
            if(d_cnt != 0) {                            // != 0, 執行中
                Smooth_CMD_Proc(s_mode, m_mode, c_mode);        // 小圖轉大圖smooth運算
                Set_Smooth_Debug();
                cmd_ok = 1;
            }
            else {                                      // == 0, 執行完畢
                Smooth_CMD_Proc(s_mode, -1, c_mode);
                FPGA_Com_In_Set_CAP(m_mode, ep_time, fps, enc_type, c_mode);
                cmd_ok = 1;
            }
        }
    }
    if(cmd_ok != 1){
        //do_A2K_Live_CMD: err! c_mode=1 m_mode=0 s_mode=3
        db_error("do_A2K_Live_CMD: err! c_mode=%d m_mode=%d s_mode=%d\n", c_mode, m_mode, s_mode);
    }
}
// rex+ 180319, 最後移植
#ifndef __KERNEL__
static pthread_mutex_t pthread_k_spi_mutex;
pthread_t pthread_k_spi_id = 0;
void *pthread_k_spi_proc()
{
    int ret;
    unsigned long long lstTime, curTime, runTime;
    db_debug("pthread_k_spi_proc: init...\n");
/*
    nice(-2);    // 調整thread優先權
    while(1){
        get_current_usec(&lstTime);

        do_A2K_Sensor_CMD();
        do_A2K_Live_CMD();
        F_Pipe_Run();

        get_current_usec(&curTime);
        runTime = curTime - lstTime;
        
        if(runTime < 5000){
            usleep(5000-runTime);
        }
    }
*/
    return (void*)0;
}
#endif
void init_k_spi_proc(void)
{
    //AS2_CMD_Start();                        // rex+ 180306

    //pthread_mutex_init(&pthread_k_spi_mutex, NULL);
    //if(pthread_create(&pthread_k_spi_id, NULL, pthread_k_spi_proc, NULL) != 0)
    //{
    //    db_error("create thread_spi_proc fail !\n");
    //}
}
/*
 *  SendMainCmdPipe()                       // cycle 50ms
 *    F_Pipe_Run()
 *      F_Pipe_Add_Small()
 *      F_Pipe_Add_Big()
 *      F_Pipe_Add_Timelapse()
 *      F_Pipe_Reset_Sensor()
 *        Make_F_Pipe()
 *          Make_Main_Cmd()                 // Make_Main_Cmd( ((F_Pipe.Sensor.Idx-1)&0x3F) );
 *            Make_Sensor_Cmd()
 *            Make_ISP1_Cmd()
 *            Make_ISP2_Cmd()
 *            Make_ST_Cmd()
 *            Make_JPEG_Cmd()
 *            Make_H264_Cmd()
 *            Make_DMA_Cmd()
 *            Make_USB_Cmd()
 */

// Make Sensor Command
/*
 * value : -1 -> get last value
 *         other -> set value
 */
void AS2_FPS_Set(int value)
{
    static int Last_Value = -1;
    unsigned Data[2], Addr;
    
    if(value == -1){
        if(Last_Value == -1) value = FPGA_CNT_FS_TEST;
        else                 value = Last_Value;
    }
    Last_Value = value;
    
    //FPS SET
    Addr = 0xBBC;
    Data[0] = Addr;
    Data[1] = value;
    K_SPI_Write_IO_S2( 0x9  , &Data[0] , 8 );     // AS2_FPS_Set
}

typedef struct{
    unsigned IO_Addr    :32;
    unsigned CMD_SP     :24;
    unsigned State_Max  :6;
    unsigned REV        :1;
    unsigned Start      :1;
}CSP_struct;
void AS2_CMD_Start(void)
{
    CSP_struct CSP;
    unsigned *P;

    unsigned Data[2];
    Data[0] = 0xBAC;
    Data[1] = (F2_M_CMD_ADDR >> 5);
    K_SPI_Write_IO_S2( 0x9  , Data , 8);                  // AS2_CMD_Start

    CSP.IO_Addr = 0xBB0;
    CSP.CMD_SP = F0_M_CMD_ADDR >> 5;
    CSP.State_Max = M_CMD_PAGE_N - 1;                   // M_CMD_PAGE_N    64
    CSP.REV = 0;
    CSP.Start = 1;
    P = (unsigned *) &CSP;
    K_SPI_Write_IO_S2( 0x9  , P , sizeof(CSP_struct) );   // AS2_CMD_Start

    AS2_FPS_Set(-1);
}

/*
 * 不需要填M_CMD_PAGE_N, 用在FPGA休眠起來(省電模式)
 */
void AS2_CMD_Start2(void)
{
	unsigned Data[2];
	Data[0] = 0x70C;
	Data[1] = 0x00;
	K_SPI_Write_IO_S2(0x9, &Data[0], 8);
}

int AdjSensorSync(S_M_CMD_struct *S_Cmd_p, unsigned EP_Line, unsigned F2_cnt, 
                  int cmd_cnt, int flag)
{
    int i, clk_cnt, S;
    unsigned Addr, V_uSec, value, tmp;
    int value2[5], value2_abs[5], value3;
    int N_uSec, S_tmp;
    int clk_uSec;
    int s_idx;    //s_idx:類比開關順序
    int sum=0, sum2=0, avg=0, avg2=0, adj_mode=0, cnt=0;

    //db_debug("AdjSensorSync: EP_Line=%d F2_cnt=%d flag=%d\n", EP_Line, F2_cnt, flag);
    for(i = 0; i < 5; i++) {
        switch(i) {
        case 0: Addr = SYNC_F1_S2_IO_ADDR; break;
        case 1: Addr = SYNC_F1_S1_IO_ADDR; break;
        case 2: Addr = SYNC_F0_S0_IO_ADDR; break;
        case 3: Addr = SYNC_F1_S0_IO_ADDR; break;
        case 4: Addr = SYNC_F0_S1_IO_ADDR; break;
        }
        spi_read_io_porcess_S2(Addr, &value, 4);
        value &= 0xFFFFFFF;
        SetWaveDebug(2, (i << 16) | (value >> 10) );
        if(value != 0) {
			value2[i] = value - 0x140000;               // 13ms +- 300us
			value2_abs[i] = abs(value2[i]);
			sum  += value2[i];
			sum2 += value2_abs[i];
			cnt++;
        }
        else {
			value2[i] = 0;               // 13ms +- 300us
			value2_abs[i] = 0;
        }
        //db_debug("AdjSensorSync: i=%d value=%d value2=%d cnt=%d\n", i, value, value2[i], cnt);
    }
    if(cnt != 0) {
    	avg  = sum / cnt;
    	avg2 = sum2 / cnt;
    }
    else {
    	avg  = 0;
    	avg2 = 0;
    }

    adj_mode = 0;
    for(i = 0; i < 5; i++) {
        if(abs(value2_abs[i] - avg2) > 30000) { // 30000x10ns=300us
            adj_mode = 1;
            break;
        }
    }
    //db_debug("AdjSensorSync: flag=%d adj_mode=0x%x avg=%d avg2=%d\n", flag, adj_mode, avg, avg2);
    if(flag != 0) {
        for(i = 0; i < 5; i++) {
            switch(i) {
            case 0: s_idx = 3; break;
            case 1: s_idx = 1; break;
            case 2: s_idx = 2; break;
            case 3: s_idx = 0; break;
            case 4: s_idx = 4; break;
            }

            S_Cmd_p->S_Data[cmd_cnt].Addr = 0x0108;
            S_Cmd_p->S_Data[cmd_cnt].RW   = 0;
            S_Cmd_p->S_Data[cmd_cnt].S_ID = s_idx;
            S_Cmd_p->S_Data[cmd_cnt].Data = EP_Line;
            cmd_cnt++;
            if(cmd_cnt >= 126){ goto error; }

            S_Cmd_p->S_Data[cmd_cnt].Addr = 0x0116;
            S_Cmd_p->S_Data[cmd_cnt].RW   = 0;
            S_Cmd_p->S_Data[cmd_cnt].S_ID = s_idx;
            S_Cmd_p->S_Data[cmd_cnt].Data = EP_Line;
            cmd_cnt++;
            if(cmd_cnt >= 126){ goto error; }
        }
        S_Cmd_p->AS2_Sync_MAX = F2_cnt;
        SetWaveDebug(5, (0xB << 16) | (S_Cmd_p->AS2_Sync_MAX >> 10) );
    }
    else if(adj_mode == 1) {

        for(i = 0; i < 5; i++) {
            switch(i) {
            case 0: s_idx = 3; break;
            case 1: s_idx = 1; break;
            case 2: s_idx = 2; break;
            case 3: s_idx = 0; break;
            case 4: s_idx = 4; break;
            }

            if(value2_abs[i] > 0x20000) {           // +- 1.3ms
                if(value2[i] < 0)
                    value3 = value2[i] + F2_cnt;
                else
                    value3 = value2[i];

                V_uSec = value3 / 100;
                N_uSec = F2_cnt / 100;
                if(V_uSec < (N_uSec >> 1) )			// if(V_uSec < (N_uSec / 2) )
                    clk_uSec = -V_uSec;
                else
                    clk_uSec = (N_uSec - V_uSec);

                S_tmp = clk_uSec * EP_Line;
                S = S_tmp / N_uSec;
                //db_debug("AdjSensorSync() 01-1 i=%d V_uSec=%d S=%d S_tmp=%d N_uSec=%d\n", i, V_uSec, S, S_tmp, N_uSec);

                S_Cmd_p->S_Data[cmd_cnt].Addr = 0x0108;
                S_Cmd_p->S_Data[cmd_cnt].RW   = 0;
                S_Cmd_p->S_Data[cmd_cnt].S_ID = s_idx;
                S_Cmd_p->S_Data[cmd_cnt].Data = EP_Line + S;
                cmd_cnt++;
                if(cmd_cnt >= 126){ goto error; }

                S_Cmd_p->S_Data[cmd_cnt].Addr = 0x0116;
                S_Cmd_p->S_Data[cmd_cnt].RW   = 0;
                S_Cmd_p->S_Data[cmd_cnt].S_ID = s_idx;
                S_Cmd_p->S_Data[cmd_cnt].Data = EP_Line + S;
                //SetWaveDebug(4, (i << 16) | S_Cmd_p->S_Data[cmd_cnt].Data);
                cmd_cnt++;
                if(cmd_cnt >= 126){ goto error; }
            }
        }
    }
    else {
        S_Cmd_p->AS2_Sync_MAX = F2_cnt + avg;
        SetWaveDebug(5, (0xA << 16) | (S_Cmd_p->AS2_Sync_MAX >> 10) );
        //db_debug("AdjSensorSync() 01-2 F2_cnt=%d avg=%d\n", F2_cnt, avg);
    }
    goto exit;
error: 
    db_error("AdjSensorSync: err! cmd_cnt=%d\n", cmd_cnt);
exit:
    return cmd_cnt;
}

/*
 * 設定 fpga pipe 的目標 idx
 */
void Set_FPGA_Pipe_Idx(int idx)
{
	unsigned Data[2], Addr;
	Addr = 0xBC4;
	Data[0] = Addr;
	Data[1] = idx & 0x3F;
	K_SPI_Write_IO_S2( 0x9  , &Data[0] , 8 );     // Set_FPGA_Pipe_Idx
}

void set_AEG_System_Freq_NP(int np)
{
    AEG_System_Freq_NP = np;
}
int get_AEG_System_Freq_NP(void)
{
    return(AEG_System_Freq_NP);
}
static int AEG_EP_Change=0, AEG_EP_FRM_LENGTH, AEG_EP_INTEG_TIME, AEG_gain_H;
void set_AEG_EP_Var(int en, int frm, int time, int gain)
{
    AEG_EP_Change = en;
    AEG_EP_FRM_LENGTH = frm;
    AEG_EP_INTEG_TIME = time;
    AEG_gain_H = gain;
}
void get_AEG_EP_Var(int *frm, int *time, int *gain)
{
	*frm  = AEG_EP_FRM_LENGTH;
	*time = AEG_EP_INTEG_TIME;
	*gain = AEG_gain_H;
}
int get_Sensor_Input_Scale(void)
{
    int mode = A2K.B_RUL_Mode;
    int scale=1;
    switch(mode){
    case 0: scale = 1; break;   // 12k
    case 1: scale = 2; break;   // 8k
    case 2: scale = 2; break;   // 6k
    case 3: scale = 3; break;   // 4k
    case 4: scale = 2; break;   // 3k
    case 5: scale = 2; break;   // 2k
    }
    return scale;
}
CIS_CMD_struct FS_TABLE[CIS_TAB_N];
CIS_CMD_struct D2_TABLE[CIS_TAB_N];
CIS_CMD_struct D3_TABLE[CIS_TAB_N];

//                   x1280  x64
// nX=1  -> gain_adj=0      0   -> iso100  -> log(1)=0
// nX=2  -> gain_adj=1280   64  -> iso200  -> log(2)=0.301
// nX=3  -> gain_adj=2028   101 -> iso300  -> log(3)=0.477
// nX=4  -> gain_adj=2560   128 -> iso400  -> log(4)=0.602
// nX=5  -> gain_adj=2968   148 -> iso500  -> log(5)=0.698
// nX=6  -> gain_adj=3308   165 -> iso600  -> log(6)=0.778
// nX=7  -> gain_adj=3593   180 -> iso700  -> log(7)=0.845
// nX=8  -> gain_adj=3840   192 -> iso800  -> log(8)=0.903 / 0.301 * 64
// nX=9  -> gain_adj=4056   203 -> iso900  -> log(9)=0.954
// nX=10 -> gain_adj=4252   212 -> iso1000 -> log(10)=1
//int nx2gain[10] = {0,64,101,128,148,165,180,192,203,212};
//int nx2gain[10] = {0,1280,2028,2560,2968,3308,3593,3840,4056,4252};

int AGain_Debug_Offset = 0, AGain_Debug_Flag = 0;
void setAGainDebugOffset(int value)
{
	AGain_Debug_Offset = value;
	AGain_Debug_Flag = 1;
}

int getAGainDebugOffset(void)
{
	return AGain_Debug_Offset;
}

void make_HDR_EXP(int idx, int c_mode, int chHDR, int max_ep_ln, int *o_frame, int *o_integ, int *o_dgain)
{
    static int hdr_FRAME=0, hdr_INTEG=3666, hdr_DGAIN=0;
    int hdr_level = get_A2K_Sensor_HDR_Level();     // 3,4,6
    int freq = get_AEG_System_Freq_NP();
    int frame, integ, dgain;
    if(hdr_level <= 0){
        db_error("make_new_hdr_exp: level=%d\n", hdr_level);
        hdr_level = 2;
    }
    if(chHDR == 0){
        hdr_FRAME = AEG_EP_FRM_LENGTH;
        hdr_INTEG = AEG_EP_INTEG_TIME;
        hdr_DGAIN = AEG_gain_H;
    }
    //else{       // HDR第2、3張，使用同一組數值，不能被影響
        frame = hdr_FRAME;
        integ = hdr_INTEG;
        dgain = hdr_DGAIN;
    //}

    int ep_ln_now = (max_ep_ln * frame) + (max_ep_ln - integ);                    // 曝光掃描線數目
    if(hdr_level > 0){
        if(chHDR == 0) ep_ln_now = (ep_ln_now / hdr_level);    // 暗,
        if(chHDR == 1) ep_ln_now = (ep_ln_now);                // 中,
        if(chHDR == 2) ep_ln_now = (ep_ln_now * hdr_level);    // 亮,
        if(chHDR == 3) ep_ln_now = (ep_ln_now);
    }
    int ep_ln_33ms = get_ep_ln_default_33ms(freq);
    int ln_1s_x1024 = (ep_ln_33ms * (30-(freq*5))) << 10;     // NTSC:30, PAL:25
    int ln_8ms_x1024 = ln_1s_x1024 / (120-(freq*20));         // NTSC:120, PAL:100
    int ln_now_x1024 = ep_ln_now << 10;
    int nX;
    if((ln_now_x1024 < ln_1s_x1024) && (ln_now_x1024 > ln_8ms_x1024)){  // exp = 1/120, 要避免日光燈閃爍
        nX = (ln_now_x1024 + (ln_8ms_x1024 >> 1)) / ln_8ms_x1024;       // 換算成1/120的倍數
        ln_now_x1024 = nX * ln_8ms_x1024;
        ep_ln_now = ln_now_x1024 >> 10;
    }//*/
    
    if(c_mode == 3 || c_mode == 5 || c_mode == 7){     // 3:HDR, 7:N-WDR, 第3張快門時間拉長
        frame = (ep_ln_now / max_ep_ln);
        ep_ln_now = ep_ln_now % max_ep_ln;
    }

    if(ep_ln_now < 1)             ep_ln_now = 1;                // 曝光時間最短
    if(ep_ln_now > (max_ep_ln-1)) ep_ln_now = (max_ep_ln-1);    // 曝光時間最長
    integ = max_ep_ln - ep_ln_now;                              // 曝光掃描線參數要倒數運算，設給sensor

    db_debug("make_new_hdr_exp: idx=%d chHDR=%d gain_H=%d FRM=%d->%d INT=%d->%d(%d)\n",
          idx, chHDR, AEG_gain_H, AEG_EP_FRM_LENGTH, frame, AEG_EP_INTEG_TIME, integ, max_ep_ln-integ);    // max_ep_ln-integ=真實曝光掃瞄線，305.5=1/120

    *o_frame = frame;
    *o_integ = integ;
    *o_dgain = dgain;
}

/*
 * return: 0 -> wait times up
 *         1 -> execute;
 *         2 -> bulb_exp (M-Mode)
 */
int Make_Sensor_Cmd(int idx, S_M_CMD_struct *S_Cmd_p, FPGA_Engine_One_Struct *F_Pipe_p, 
                     F_Com_In_Struct *F_Com_p, int *bulb_exp)
{
    int Pic_No = F_Pipe_p->No;
    int freq = get_AEG_System_Freq_NP();
    int i, scale;
    int a_gain = 0, d_gain = 0;
    CIS_CMD_struct *SP;
    int gain_offset = FS_A_GAIN_OFFSET;
    int cmd_cnt = 0;
    unsigned long long F2_cnt = FPGA_CNT_FS_TEST, F2_cnt_base = FPGA_CNT_FS_TEST;
    int EP_Line, line_tmp;
    int F2_rate_x1000 = 1000;           // 1.0
    int start_reg = 0, reg_cnt = 0, reg;
    int doReset, chBinn, chAEG, chHDR=-1, chAEB=-1;
    static int c_usb_cnt = -1;
    int c_mode = A2K.Camera_Mode;
    int m_mode = A2K.B_RUL_Mode;
    int d_cnt = read_F_Com_In_Capture_D_Cnt();
    int integ, frame, dgain;

    memset(S_Cmd_p, 0, sizeof(S_M_CMD_struct));

    if(F_Pipe_p->Time_Buf <= 0 && F_Com_p->Sensor_Change_Binn != 11){
        //db_error("Make_Sensor_Cmd: err! Time_Buf=%d Binn=%d\n", F_Pipe_p->Time_Buf, F_Com_p->Sensor_Change_Binn);
        return 0;
    }
    
    doReset = F_Com_In.Reset_Sensor;
    if(doReset == 1) chBinn = 0;
    else             chBinn = F_Com_p->Sensor_Change_Binn;
    if(chBinn > 0 && chBinn <= 3 || doReset == 1) chAEG = 0;
    else                                          chAEG = AEG_EP_Change;
    int hdr_7idx=-1, ep_long_t=-1, small_f=0;
    get_F_Temp_Sensor_SubData(idx, &hdr_7idx, &ep_long_t, &small_f, &frame, &integ, &dgain);
    if(frame == -1 || integ == -1 || dgain == -1){
        integ = AEG_EP_INTEG_TIME;
        frame = AEG_EP_FRM_LENGTH;
        dgain = AEG_gain_H;
    }

    switch(hdr_7idx){        // default: chHDR = -1;
        case PIPE_SUBCODE_AEB_STEP_0: chAEB = 0; break;
        case PIPE_SUBCODE_AEB_STEP_1: chAEB = 1; break;
        case PIPE_SUBCODE_AEB_STEP_2: chAEB = 2; break;
        case PIPE_SUBCODE_AEB_STEP_3: chAEB = 3; break;
        case PIPE_SUBCODE_AEB_STEP_4: chAEB = 4; break;
        case PIPE_SUBCODE_AEB_STEP_5: chAEB = 5; break;
        case PIPE_SUBCODE_AEB_STEP_6: chAEB = 6; break;
        case PIPE_SUBCODE_AEB_STEP_E: chAEB = 7; break;
        case PIPE_SUBCODE_HDR_CNT_0: chHDR = 0; break;
        case PIPE_SUBCODE_HDR_CNT_1: chHDR = 1; break;
        case PIPE_SUBCODE_HDR_CNT_2: chHDR = 2; break;
        case PIPE_SUBCODE_HDR_CNT_E: chHDR = 3; break;
    }
    
    switch(c_mode) {
    case 0: // Capture
    case 3: // AEB (3p,5p,7p)
    case 4: // RAW (5p)
    case 5: // HDR (1p)
    case 8: // Sport (1p)
    case 9: // Sport + WDR
    case 13:// Removal (1p)
    case 14:// 3D-Model (1p)
        switch(m_mode) {
        case 0:
        case 3: SP = &FS_TABLE[0]; scale = 1; gain_offset = FS_A_GAIN_OFFSET; F2_cnt_base = FPGA_CNT_FS_100FPS; break;
        case 1:
        case 2:
        case 4:
        case 5: SP = &D2_TABLE[0]; scale = 2; gain_offset = D2_A_GAIN_OFFSET; F2_cnt_base = FPGA_CNT_D2_100FPS; break;
        }
        break;
    case 6: // Night
    case 7: // Night + HDR
    case 12:// M-Mode
        switch(m_mode) {
        case 0: // 12K
                SP = &FS_TABLE[0]; scale = 1; gain_offset = FS_A_GAIN_OFFSET; F2_cnt_base = FPGA_CNT_FS_50FPS; break;   // 設定FPGA Pipeline Times
        case 3: // 4K
                SP = &FS_TABLE[0]; scale = 1; gain_offset = FS_A_GAIN_OFFSET; F2_cnt_base = FPGA_CNT_FS_100FPS; break;  // sensor binning無法設定200ms
        case 1: // 8K
        case 2: // 6K
        case 4: // 3K
        case 5: // 2K
                SP = &D2_TABLE[0]; scale = 2; gain_offset = D2_A_GAIN_OFFSET; F2_cnt_base = FPGA_CNT_D2_100FPS; break;
        }
        break;
    case 1: //Rec
    case 10://Rec_WDR
        switch(m_mode) {
        case 3: SP = &D3_TABLE[0]; scale = 3; gain_offset = D3_A_GAIN_OFFSET; F2_cnt_base = FPGA_CNT_D3_100FPS; break;
        case 4: SP = &D2_TABLE[0]; scale = 2; gain_offset = D2_A_GAIN_OFFSET; F2_cnt_base = (freq == 0)?FPGA_CNT_D2_240FPS:(FPGA_CNT_D2_240FPS*6/5); break;
        case 5: SP = &D2_TABLE[0]; scale = 2; gain_offset = D2_A_GAIN_OFFSET; F2_cnt_base = (freq == 0)?FPGA_CNT_D2_300FPS:(FPGA_CNT_D2_300FPS*6/5); break;
        }
        break;
    case 2: //Time Lapse
    case 11://Time Lapse WDR
        switch(m_mode) {
        case 0: SP = &FS_TABLE[0]; scale = 1; gain_offset = FS_A_GAIN_OFFSET; F2_cnt_base = FPGA_CNT_FS_100FPS; break;
        case 1:
        case 2: SP = &D2_TABLE[0]; scale = 2; gain_offset = D2_A_GAIN_OFFSET; F2_cnt_base = FPGA_CNT_D2_100FPS; break;
        case 3: SP = &D3_TABLE[0]; scale = 3; gain_offset = D3_A_GAIN_OFFSET; F2_cnt_base = FPGA_CNT_D3_100FPS; break;
        }
        break;
    }

    int max_ep_ln = get_ep_ln_maximum(A2K.FPS_x10);

    // rex- 180521, F2_cnt永遠100ms為1單位
    F2_rate_x1000 = 1000;       //1.0
    F2_cnt = F2_cnt_base;

    line_tmp = ( (max_ep_ln * F2_rate_x1000 + 8191) >> 13);        //line_tmp = (max_ep_ln * F2_rate_x1000 + 8191) / 8192;        // 8192 = 223ms
    if(line_tmp < 1000) line_tmp = 1000;
    EP_Line = (max_ep_ln * F2_rate_x1000) / line_tmp;
    static int idx_cnt=0;

    if(chBinn > 0 && chBinn <= 3) {
        reg_cnt = 102;
        start_reg = (3 - chBinn) * reg_cnt;
        for(i = 0; i < reg_cnt; i++) {
            reg = start_reg + i;
            if(reg == 61 || reg == 62 || reg == 63 || reg == 64) continue;          //EP & Gain 保持原值
            S_Cmd_p->S_Data[cmd_cnt].Addr = ((SP[reg].Addr_H << 8) | SP[reg].Addr_L);
            S_Cmd_p->S_Data[cmd_cnt].RW   = 0;
            S_Cmd_p->S_Data[cmd_cnt].S_ID = 5;
            if(reg == 92 || reg == 106)             //Addr: 0x108 & 0x116, Sensor條數
                S_Cmd_p->S_Data[cmd_cnt].Data = max_ep_ln;
            else
                S_Cmd_p->S_Data[cmd_cnt].Data = SP[reg].Data;
            cmd_cnt++;
            if(cmd_cnt >= 126){ goto error; }
        }
        F_Com_p->Sensor_Change_Binn--;
    }
    else if(chBinn == 11 && adjust_State == 0/* && d_cnt == 0*/) {
        cmd_cnt = AdjSensorSync(S_Cmd_p, EP_Line, (int)F2_cnt, cmd_cnt, adjust_State);  // set (F2_cnt + avg), run adjust
        if(cmd_cnt == 0) adjust_State = 2;              //無調整Sensor條數, 只調整F2 CNT
        else             adjust_State = 1;              //有調整Sensor條數
    }
    else if(adjust_State == 1 || adjust_State == 2) {
        cmd_cnt = AdjSensorSync(S_Cmd_p, EP_Line, (int)F2_cnt, cmd_cnt, adjust_State);  // set F2_cnt, break
        F_Com_p->Sensor_Change_Binn = 0;
        adjust_State = 0;
    }

    if(chBinn == 3) c_usb_cnt = (CLOSE_USB_CNT-1);      //清掉切換mode後的 CLOSE_USB_CNT 道 usb cmd
    if(c_usb_cnt >= 0) {
        A2K.USB_Close_No[c_usb_cnt] = Pic_No;
        c_usb_cnt--;
    }

    if(chHDR >= 0){
        make_HDR_EXP(idx, c_mode, chHDR, max_ep_ln, &frame, &integ, &dgain);
    }
    if(chAEB >= 0){     // rex+ 190326
        if(dgain > 100*64) dgain = 100*64;
    }

    int bexp_1sec=0, bexp_gain=0;
    if(c_mode == 12 && ep_long_t > 0){
        get_AEG_B_Exp(&bexp_1sec, &bexp_gain);
        if(bexp_1sec < 2) bexp_1sec = 2;
        if(bexp_1sec > 819) bexp_1sec = 819;
        *bulb_exp = (bexp_1sec-1) * 1000000;        // B快門啟動, 
        frame = (bexp_1sec * 5) - 1;
        integ = 0;
        dgain = bexp_gain;
        if(chBinn > 0 && chBinn <= 3 || doReset == 1) chAEG = 0;
        else										  chAEG = 1;
    }
    else{
        *bulb_exp = 0;
    }

    static int AEG_Frame_lst=0;
    static int AEG_Integ_lst=0;
    static int AEG_DGain_lst;
    if(AEG_Frame_lst != frame || AEG_Integ_lst != integ || small_f == 0){
        AEG_Frame_lst = frame;
        AEG_Integ_lst = integ;
        db_debug("Make_Sensor: idx=%d small=%d frame=%d integ=%d dgain=%d max=%d chAEB=%d\n", idx, small_f,
                frame, integ, dgain>>6, max_ep_ln, chAEB);
    }
    if(adjust_State != 0 || small_f == 1){
        frame = 0;                              // rex+ 190312, 避免sensor曝光錯誤
    }
    if(chAEG > 0 || chHDR >= 0 || chAEB >= 0 || (chBinn > 0 && chBinn <= 3) || AGain_Debug_Flag == 1) {
    	AGain_Debug_Flag = 0;

        S_Cmd_p->S_Data[cmd_cnt].Addr = 0x0043;
        S_Cmd_p->S_Data[cmd_cnt].RW   = 0;
        S_Cmd_p->S_Data[cmd_cnt].S_ID = 5;
        S_Cmd_p->S_Data[cmd_cnt].Data = (F2_rate_x1000 == 1000)? (frame&0xFFF) : ((line_tmp/1000)-1);
        cmd_cnt++;
        if(cmd_cnt >= 126){ goto error; }
        
        S_Cmd_p->S_Data[cmd_cnt].Addr = 0x0044;    //Shutter
        S_Cmd_p->S_Data[cmd_cnt].RW   = 0;
        S_Cmd_p->S_Data[cmd_cnt].S_ID = 5;
        S_Cmd_p->S_Data[cmd_cnt].Data = (F2_rate_x1000 == 1000)? (integ&0x1FFF) : 0;
        cmd_cnt++;
        if(cmd_cnt >= 126){ goto error; }

        if(AEG_EP_Change == 1) AEG_EP_Change = 2;
        else                   AEG_EP_Change = 0;
        
        int gain_H; // gain_H單位不同，64=1倍
        if(ep_long_t == -1) gain_H = AEG_DGain_lst / 20;    // live or ep <= 1/15
        else                gain_H = dgain / 20;            // ep > 1/15
        AEG_DGain_lst = dgain;                   // rex+ 180418, 解畫面會閃一下, EP和GAIN不同步
        
        if(gain_H <= 107) { a_gain = gain_H; d_gain = 0; }
        else              { a_gain = 107;    d_gain = gain_H - 107; }
        if(d_gain > 255) d_gain = 255;

        S_Cmd_p->S_Data[cmd_cnt].Addr = 0x0041;    //AGain
        S_Cmd_p->S_Data[cmd_cnt].RW   = 0;
        S_Cmd_p->S_Data[cmd_cnt].S_ID = 5;
        S_Cmd_p->S_Data[cmd_cnt].Data = a_gain + 256 - gain_offset + AGain_Debug_Offset;            //107 = 363 - 256, a_gain(dB) = 0.09375 * (a_gain - 256), a_gain rang: 0~363
        if(S_Cmd_p->S_Data[cmd_cnt].Data < 0)   S_Cmd_p->S_Data[cmd_cnt].Data = 0;
        if(S_Cmd_p->S_Data[cmd_cnt].Data > 363) S_Cmd_p->S_Data[cmd_cnt].Data = 363;
        cmd_cnt++;
        if(cmd_cnt >= 126){ goto error; }

        S_Cmd_p->S_Data[cmd_cnt].Addr = 0x0042;    //DGain
        S_Cmd_p->S_Data[cmd_cnt].RW   = 0;
        S_Cmd_p->S_Data[cmd_cnt].S_ID = 5;
        S_Cmd_p->S_Data[cmd_cnt].Data = d_gain + 320;            // d_gain(dB) = 0.09375 * (d_gain - 320), d_gain rang: 0~575
        if(S_Cmd_p->S_Data[cmd_cnt].Data < 0)   S_Cmd_p->S_Data[cmd_cnt].Data = 0;
        if(S_Cmd_p->S_Data[cmd_cnt].Data > 575) S_Cmd_p->S_Data[cmd_cnt].Data = 575;
        cmd_cnt++;
        if(cmd_cnt >= 126){ goto error; }

//if(chAEB >= 0) {
//   	db_debug("Make_Sensor_Cmd() HDR: idx=%d chAEB=%d frame=%d integ=%d dgain=%d gain_H=%d a_gain=%d d_gain=%d ep_long_t=%d\n",
//   		idx, chAEB, frame, integ, dgain, gain_H, a_gain, d_gain, ep_long_t);
//}
    }

    if(cmd_cnt != 0 || doReset == 1 || S_Cmd_p->AS2_Sync_MAX != 0) {
        S_Cmd_p->S_HEADER.checksum = 0xD55C;
        S_Cmd_p->S_HEADER.rev = 0;
        S_Cmd_p->S_HEADER.reset = doReset;
        S_Cmd_p->S_HEADER.enable = 1;
        S_Cmd_p->S_HEADER.size = (doReset == 1)?0:cmd_cnt;
        if(S_Cmd_p->AS2_Sync_MAX == 0)
            S_Cmd_p->AS2_Sync_MAX = F2_cnt;

        if(chBinn == 1) {
        	F_JPEG_Header.F2_Cnt = F2_cnt_base;
        	F_JPEG_Header.Init_Flag = 0;
        }

        if(doReset == 1) {
            F_Com_In.Reset_Sensor = 0;
        }
    }
    return 1;
error: 
    db_error("Make_Sensor_Cmd: cmd_cnt=%d err!\n", cmd_cnt);
    return -1; 
}

extern int get_AEB_Frame_Cnt(void);
/*
 * F0_ISP1_CMD: 
 * F1_ISP1_CMD: 
 *     F0_ -> sensor x 2 
 *     F1_ -> sensor x 3
 * 
 * ISP1_STM1_T_P0_A_BUF_ADDR
 *    STM1_ -> big image stream
 *    STM2_ -> small image stream
 *    P0_ -> target addr buffer 1
 *    P1_ -> target addr buffer 2
 *    P2_ -> target addr buffer 3
 *    A_ -> sensor 1
 *    B_ -> sensor 2
 *    C_ -> sensor 3
 */
void set_ISP1_P0_P1_P2_Addr(int big_img, int fpga_binn, int o_page, int hdr_7idx, int hdr_fcnt, AS2_F0_MAIN_CMD_struct *Cmd_p)
{
    int isp1_t_addr_a=0, isp1_t_addr_b=0, isp1_t_addr_c=0;
    int isp1_b_addr_a=0, isp1_b_addr_b=0, isp1_b_addr_c=0;
    int isp1_f_offset=0x18;

    if(big_img == 1){               // 大圖
        switch(hdr_7idx){
            case PIPE_SUBCODE_AEB_STEP_0: o_page = 0; break;    // 暗 暗2 暗3
            case PIPE_SUBCODE_AEB_STEP_1: o_page = 1; break;    // 中 暗  暗2
            case PIPE_SUBCODE_AEB_STEP_2: o_page = 2; break;    // 亮 中  暗
            case PIPE_SUBCODE_AEB_STEP_3: o_page = 3; break;    //    亮  中
            case PIPE_SUBCODE_AEB_STEP_4: o_page = 4; break;    //    亮2 亮
            case PIPE_SUBCODE_AEB_STEP_5: o_page = 5; break;    //        亮2
            case PIPE_SUBCODE_AEB_STEP_6: o_page = 6; break;    //        亮3
            case PIPE_SUBCODE_AEB_STEP_E: o_page = 0; break;
            default: break;
        }
        switch(o_page){
        case 0: isp1_t_addr_a = ISP1_STM1_T_P0_A_BUF_ADDR;      // ISP1 P0 buffer
                isp1_t_addr_b = ISP1_STM1_T_P0_B_BUF_ADDR;
                isp1_t_addr_c = ISP1_STM1_T_P0_C_BUF_ADDR;
                break;
        case 1: isp1_t_addr_a = ISP1_STM1_T_P1_A_BUF_ADDR;      // ISP1 P1 buffer
                isp1_t_addr_b = ISP1_STM1_T_P1_B_BUF_ADDR;
                isp1_t_addr_c = ISP1_STM1_T_P1_C_BUF_ADDR;
                break;
        case 2: isp1_t_addr_a = ISP1_STM1_T_P2_A_BUF_ADDR;      // ISP1 P2 buffer
                isp1_t_addr_b = ISP1_STM1_T_P2_B_BUF_ADDR;
                isp1_t_addr_c = ISP1_STM1_T_P2_C_BUF_ADDR;
                break;
        case 3: if(hdr_fcnt == 7){
                    isp1_t_addr_a = ISP1_STM1_T_P3_A_BUF_ADDR;      // ISP2 P1 buffer, 記憶體不足共用ISP2 DDR
                    isp1_t_addr_b = ISP1_STM1_T_P3_B_BUF_ADDR;
                    isp1_t_addr_c = ISP1_STM1_T_P3_C_BUF_ADDR;
                }
                else{
                    isp1_t_addr_a = ISP1_STM1_T_P5_A_BUF_ADDR;      // ISP2 P0-2 buffer
                    isp1_t_addr_b = ISP1_STM1_T_P5_B_BUF_ADDR;
                    isp1_t_addr_c = ISP1_STM1_T_P5_C_BUF_ADDR;
                }
                break;
        case 4: isp1_t_addr_a = ISP1_STM1_T_P4_A_BUF_ADDR;      // ISP2 P0-1 buffer
                isp1_t_addr_b = ISP1_STM1_T_P4_B_BUF_ADDR;
                isp1_t_addr_c = ISP1_STM1_T_P4_C_BUF_ADDR;
                break;
        case 5: isp1_t_addr_a = ISP1_STM1_T_P5_A_BUF_ADDR;      // ISP2 P0-2 buffer
                isp1_t_addr_b = ISP1_STM1_T_P5_B_BUF_ADDR;
                isp1_t_addr_c = ISP1_STM1_T_P5_C_BUF_ADDR;
                break;
        case 6: isp1_t_addr_a = ISP1_STM1_T_P6_A_BUF_ADDR;      // NR3D
                isp1_t_addr_b = ISP1_STM1_T_P6_B_BUF_ADDR;
                isp1_t_addr_c = ISP1_STM1_T_P6_C_BUF_ADDR;
                break;
        }
        if(fpga_binn == 2){         // WDR運算
            switch(o_page){
            case 0: isp1_b_addr_a = FX_WDR_IMG_A_P0_ADDR;       // 亮度表
                    isp1_b_addr_b = FX_WDR_IMG_B_P0_ADDR;
                    isp1_b_addr_c = FX_WDR_IMG_C_P0_ADDR;
                    break;
            case 1: isp1_b_addr_a = FX_WDR_IMG_A_P1_ADDR;
                    isp1_b_addr_b = FX_WDR_IMG_B_P1_ADDR;
                    isp1_b_addr_c = FX_WDR_IMG_C_P1_ADDR;
                    break;
            case 2: isp1_b_addr_a = FX_WDR_IMG_A_P2_ADDR;
                    isp1_b_addr_b = FX_WDR_IMG_B_P2_ADDR;
                    isp1_b_addr_c = FX_WDR_IMG_C_P2_ADDR;
                    break;
            case 3: isp1_b_addr_a = FX_WDR_IMG_A_P3_ADDR;
                    isp1_b_addr_b = FX_WDR_IMG_B_P3_ADDR;
                    isp1_b_addr_c = FX_WDR_IMG_C_P3_ADDR;
                    break;
            case 4: isp1_b_addr_a = FX_WDR_IMG_A_P4_ADDR;
                    isp1_b_addr_b = FX_WDR_IMG_B_P4_ADDR;
                    isp1_b_addr_c = FX_WDR_IMG_C_P4_ADDR;
                    break;
            case 5: isp1_b_addr_a = FX_WDR_IMG_A_P5_ADDR;
                    isp1_b_addr_b = FX_WDR_IMG_B_P5_ADDR;
                    isp1_b_addr_c = FX_WDR_IMG_C_P5_ADDR;
                    break;
            case 6: isp1_b_addr_a = FX_WDR_IMG_A_P6_ADDR;
                    isp1_b_addr_b = FX_WDR_IMG_B_P6_ADDR;
                    isp1_b_addr_c = FX_WDR_IMG_C_P6_ADDR;
                    break;
            }
        }
        isp1_f_offset = 0x48;
    }
    else{                       // 小圖
        if(fpga_binn == 1){     // 透過FPGA縮圖
            if(o_page == 0){
                isp1_b_addr_a = ISP1_STM2_T_P0_A_BUF_ADDR;
                isp1_b_addr_b = ISP1_STM2_T_P0_B_BUF_ADDR;
                isp1_b_addr_c = ISP1_STM2_T_P0_C_BUF_ADDR;
            }
            else if(o_page == 1){
                isp1_b_addr_a = ISP1_STM2_T_P1_A_BUF_ADDR;
                isp1_b_addr_b = ISP1_STM2_T_P1_B_BUF_ADDR;
                isp1_b_addr_c = ISP1_STM2_T_P1_C_BUF_ADDR;
            }
        }
        else{                   // 透過Sensor縮圖
            if(o_page == 0){
                isp1_t_addr_a = ISP1_STM2_T_P0_A_BUF_ADDR;
                isp1_t_addr_b = ISP1_STM2_T_P0_B_BUF_ADDR;
                isp1_t_addr_c = ISP1_STM2_T_P0_C_BUF_ADDR;
            }
            else if(o_page == 1){
                isp1_t_addr_a = ISP1_STM2_T_P1_A_BUF_ADDR;
                isp1_t_addr_b = ISP1_STM2_T_P1_B_BUF_ADDR;
                isp1_t_addr_c = ISP1_STM2_T_P1_C_BUF_ADDR;
            }
        }
        isp1_f_offset = 0x18;
    }
    
    Cmd_p->F0_ISP1_CMD.S0_F_DDR_Addr.Data   = (isp1_t_addr_a >> 5) + 1;     // 校正影像位置偏移 + 1
    Cmd_p->F0_ISP1_CMD.S1_F_DDR_Addr.Data   = (isp1_t_addr_b >> 5) + 1;
    Cmd_p->F0_ISP1_CMD.S2_F_DDR_Addr.Data   = (isp1_t_addr_c >> 5) + 1;

    Cmd_p->F1_ISP1_CMD.S0_F_DDR_Addr.Data   = (isp1_t_addr_c >> 5) + 1;
    Cmd_p->F1_ISP1_CMD.S1_F_DDR_Addr.Data   = (isp1_t_addr_b >> 5) + 1;
    Cmd_p->F1_ISP1_CMD.S2_F_DDR_Addr.Data   = (isp1_t_addr_a >> 5) + 1;

    Cmd_p->F0_ISP1_CMD.S0_B_DDR_Addr.Data   = (isp1_b_addr_a >> 5) + 1;
    Cmd_p->F0_ISP1_CMD.S1_B_DDR_Addr.Data   = (isp1_b_addr_b >> 5) + 1;
    Cmd_p->F0_ISP1_CMD.S2_B_DDR_Addr.Data   = (isp1_b_addr_c >> 5) + 1;

    Cmd_p->F1_ISP1_CMD.S0_B_DDR_Addr.Data   = (isp1_b_addr_c >> 5) + 1;
    Cmd_p->F1_ISP1_CMD.S1_B_DDR_Addr.Data   = (isp1_b_addr_b >> 5) + 1;
    Cmd_p->F1_ISP1_CMD.S2_B_DDR_Addr.Data   = (isp1_b_addr_a >> 5) + 1;

    Cmd_p->F0_ISP1_CMD.F_Offset.Data        = isp1_f_offset;
    Cmd_p->F1_ISP1_CMD.F_Offset.Data        = isp1_f_offset;
}

void set_ISP1_Binn_Start(int M_Mode, int fpga_binn, int s_binn, int binn_mode, AS2_F0_MAIN_CMD_struct *Cmd_p, int Now_Mode,
		int wdr_live_en, int wdr_live_idx, int wdr_live_page, int hdr7_idx, int hdr_fcnt, int C_Mode, JPEG_HDR_AEB_Info_Struct info)
{
    int i;
    int v_start, h_start, h_size;
    int BH[5], BV[5], RH[5], RV[5];
    int BX[5], BY[5], RX[5], RY[5], B_BY[5], B_RY[5];
    int wb_a_ddr_f0 = ISP1_WB_P0_A_ADDR, wb_b_ddr_f0 = ISP1_WB_P0_B_ADDR, wb_c_ddr_f0 = ISP1_WB_P0_C_ADDR;
    int wb_a_ddr_f1 = ISP1_WB_P0_A_ADDR, wb_b_ddr_f1 = ISP1_WB_P0_B_ADDR, wb_c_ddr_f1 = ISP1_WB_P0_C_ADDR;
    int wd_a_ddr = ISP1_WD_P0_A_ADDR, wd_b_ddr = ISP1_WD_P0_B_ADDR, wd_c_ddr = ISP1_WD_P0_C_ADDR;

    if(wdr_live_idx == 0) {
		Cmd_p->F0_ISP1_CMD.S0_Binn_START.Data 	 = 0;               // F0
		Cmd_p->F0_ISP1_CMD.S1_Binn_START.Data 	 = 0;
		Cmd_p->F0_ISP1_CMD.S2_Binn_START.Data 	 = 0;
		Cmd_p->F0_ISP1_CMD.S0_ISP1_START.Address = 0x00000000;
		Cmd_p->F0_ISP1_CMD.S0_ISP1_START.Data 	 = 0x0;
		Cmd_p->F0_ISP1_CMD.S1_ISP1_START.Address = 0x00000000;
		Cmd_p->F0_ISP1_CMD.S1_ISP1_START.Data 	 = 0x0;
		Cmd_p->F0_ISP1_CMD.S2_ISP1_START.Address = 0x00000000;
		Cmd_p->F0_ISP1_CMD.S2_ISP1_START.Data 	 = 0x0;
		Cmd_p->F1_ISP1_CMD.S0_Binn_START.Data 	 = 0;               // F1
		Cmd_p->F1_ISP1_CMD.S1_Binn_START.Data 	 = 0;
		Cmd_p->F1_ISP1_CMD.S2_Binn_START.Data 	 = 0;
		Cmd_p->F1_ISP1_CMD.S0_ISP1_START.Address = 0x00000000;
		Cmd_p->F1_ISP1_CMD.S0_ISP1_START.Data 	 = 0x0;
		Cmd_p->F1_ISP1_CMD.S1_ISP1_START.Address = 0x00000000;
		Cmd_p->F1_ISP1_CMD.S1_ISP1_START.Data 	 = 0x0;
		Cmd_p->F1_ISP1_CMD.S2_ISP1_START.Address = 0x00000000;
		Cmd_p->F1_ISP1_CMD.S2_ISP1_START.Data 	 = 0x0;
    }
    else {
    if(fpga_binn == 2){         // sensor binn & fpga binn 同時開啟
        Cmd_p->F0_ISP1_CMD.S0_Binn_START.Data 	 = 1;               // F0
        Cmd_p->F0_ISP1_CMD.S1_Binn_START.Data 	 = 1;
        Cmd_p->F0_ISP1_CMD.S2_Binn_START.Data 	 = 0;
        Cmd_p->F0_ISP1_CMD.S0_ISP1_START.Address = 0xCCAA02B9;
        Cmd_p->F0_ISP1_CMD.S0_ISP1_START.Data 	 = 0x1;
        Cmd_p->F0_ISP1_CMD.S1_ISP1_START.Address = 0xCCAA02BA;
        Cmd_p->F0_ISP1_CMD.S1_ISP1_START.Data 	 = 0x1;
        Cmd_p->F0_ISP1_CMD.S2_ISP1_START.Address = 0xCCAA02BB;
        Cmd_p->F0_ISP1_CMD.S2_ISP1_START.Data 	 = 0x1;
        Cmd_p->F1_ISP1_CMD.S0_Binn_START.Data 	 = 1;               // F1
        Cmd_p->F1_ISP1_CMD.S1_Binn_START.Data 	 = 1;
        Cmd_p->F1_ISP1_CMD.S2_Binn_START.Data 	 = 1;
        Cmd_p->F1_ISP1_CMD.S0_ISP1_START.Address = 0xCCAA02B9;
        Cmd_p->F1_ISP1_CMD.S0_ISP1_START.Data 	 = 0x1;
        Cmd_p->F1_ISP1_CMD.S1_ISP1_START.Address = 0xCCAA02BA;
        Cmd_p->F1_ISP1_CMD.S1_ISP1_START.Data 	 = 0x1;
        Cmd_p->F1_ISP1_CMD.S2_ISP1_START.Address = 0xCCAA02BB;
        Cmd_p->F1_ISP1_CMD.S2_ISP1_START.Data 	 = 0x1;
    }
    else if(fpga_binn == 0){    // sensor binn
        Cmd_p->F0_ISP1_CMD.S0_Binn_START.Data 	 = 0;               // F0
        Cmd_p->F0_ISP1_CMD.S1_Binn_START.Data 	 = 0;
        Cmd_p->F0_ISP1_CMD.S2_Binn_START.Data 	 = 0;
        Cmd_p->F0_ISP1_CMD.S0_ISP1_START.Address = 0xCCAA02B9;
        Cmd_p->F0_ISP1_CMD.S0_ISP1_START.Data 	 = 0x1;
        Cmd_p->F0_ISP1_CMD.S1_ISP1_START.Address = 0xCCAA02BA;
        Cmd_p->F0_ISP1_CMD.S1_ISP1_START.Data 	 = 0x1;
        Cmd_p->F0_ISP1_CMD.S2_ISP1_START.Address = 0xCCAA02BB;
        Cmd_p->F0_ISP1_CMD.S2_ISP1_START.Data 	 = 0x1;
        Cmd_p->F1_ISP1_CMD.S0_Binn_START.Data 	 = 0;               // F1
        Cmd_p->F1_ISP1_CMD.S1_Binn_START.Data 	 = 0;
        Cmd_p->F1_ISP1_CMD.S2_Binn_START.Data 	 = 0;
        Cmd_p->F1_ISP1_CMD.S0_ISP1_START.Address = 0xCCAA02B9;
        Cmd_p->F1_ISP1_CMD.S0_ISP1_START.Data 	 = 0x1;
        Cmd_p->F1_ISP1_CMD.S1_ISP1_START.Address = 0xCCAA02BA;
        Cmd_p->F1_ISP1_CMD.S1_ISP1_START.Data 	 = 0x1;
        Cmd_p->F1_ISP1_CMD.S2_ISP1_START.Address = 0xCCAA02BB;
        Cmd_p->F1_ISP1_CMD.S2_ISP1_START.Data 	 = 0x1;
    }
    else{                       // fpga binn
        Cmd_p->F0_ISP1_CMD.S0_Binn_START.Data 	 = 1;               // F0
        Cmd_p->F0_ISP1_CMD.S1_Binn_START.Data 	 = 1;
        Cmd_p->F0_ISP1_CMD.S2_Binn_START.Data 	 = 0;
        Cmd_p->F0_ISP1_CMD.S0_ISP1_START.Address = 0x00000000;
        Cmd_p->F0_ISP1_CMD.S0_ISP1_START.Data 	 = 0x0;
        Cmd_p->F0_ISP1_CMD.S1_ISP1_START.Address = 0x00000000;
        Cmd_p->F0_ISP1_CMD.S1_ISP1_START.Data 	 = 0x0;
        Cmd_p->F0_ISP1_CMD.S2_ISP1_START.Address = 0x00000000;
        Cmd_p->F0_ISP1_CMD.S2_ISP1_START.Data 	 = 0x0;
        Cmd_p->F1_ISP1_CMD.S0_Binn_START.Data 	 = 1;               // F1
        Cmd_p->F1_ISP1_CMD.S1_Binn_START.Data 	 = 1;
        Cmd_p->F1_ISP1_CMD.S2_Binn_START.Data 	 = 1;
        Cmd_p->F1_ISP1_CMD.S0_ISP1_START.Address = 0x00000000;
        Cmd_p->F1_ISP1_CMD.S0_ISP1_START.Data 	 = 0x0;
        Cmd_p->F1_ISP1_CMD.S1_ISP1_START.Address = 0x00000000;
        Cmd_p->F1_ISP1_CMD.S1_ISP1_START.Data 	 = 0x0;
        Cmd_p->F1_ISP1_CMD.S2_ISP1_START.Address = 0x00000000;
        Cmd_p->F1_ISP1_CMD.S2_ISP1_START.Data 	 = 0x0;
    }
    }
    switch(s_binn){
        default:
        case 1: v_start = AS2_FS_V_START; h_start = AS2_FS_H_START; h_size = DS_H_PIXEL_SIZE_FS; break;
        case 2: v_start = AS2_D2_V_START; h_start = AS2_D2_H_START; h_size = DS_H_PIXEL_SIZE_D2; break;
        case 3: v_start = AS2_S3_V_START; h_start = AS2_S3_H_START; h_size = DS_H_PIXEL_SIZE_D3; break;
    }
    Cmd_p->F0_ISP1_CMD.S0_H_PIX_Size.Data   = h_size;
    Cmd_p->F0_ISP1_CMD.S1_H_PIX_Size.Data   = h_size;
    Cmd_p->F0_ISP1_CMD.S2_H_PIX_Size.Data   = h_size;
    Cmd_p->F0_ISP1_CMD.V_H_Start.Data       = ( (v_start << 8) | h_start);

    Cmd_p->F1_ISP1_CMD.S0_H_PIX_Size.Data   = h_size;
    Cmd_p->F1_ISP1_CMD.S1_H_PIX_Size.Data   = h_size;
    Cmd_p->F1_ISP1_CMD.S2_H_PIX_Size.Data   = h_size;
    Cmd_p->F1_ISP1_CMD.V_H_Start.Data       = ( (v_start << 8) | h_start);

    Cmd_p->F0_ISP1_CMD.B_Mode.Data          = binn_mode;
    Cmd_p->F1_ISP1_CMD.B_Mode.Data          = binn_mode;

//    db_debug("set_ISP1_Binn_Start() M_Mode=%d  Offset[0]=%d Offset[1]=%d Offset[2]=%d Offset[3]=%d Offset[4]=%d Offset[5]=%d\n",
//    		M_Mode, A2K.ISP1_Skip_XY_Offset[M_Mode][0], A2K.ISP1_Skip_XY_Offset[M_Mode][1], A2K.ISP1_Skip_XY_Offset[M_Mode][2],
//    		A2K.ISP1_Skip_XY_Offset[M_Mode][3], A2K.ISP1_Skip_XY_Offset[M_Mode][4], A2K.ISP1_Skip_XY_Offset[M_Mode][5]);

    for(i = 0; i < 5; i++) {
    	BH[i] = A2K.ISP1_Skip_H_Offset[i][0];
    	BV[i] = A2K.ISP1_Skip_V_Offset[i][0];
    	RH[i] = A2K.ISP1_Skip_H_Offset[i][1];
    	RV[i] = A2K.ISP1_Skip_V_Offset[i][1];
    }
   	for(i = 0; i < 5; i++) {	//sensor
		RX[i] = (A2K.ISP1_Skip_XY_Offset[Now_Mode][0]+RH[i])/s_binn;
		if(RX[i] < 0) RX[i] = 0;
		RY[i] = ( ( (A2K.ISP1_Skip_XY_Offset[Now_Mode][1]+RV[i])/s_binn << 1)+1);
		if(RY[i] < 0) RY[i] = 0;

		BX[i] = (A2K.ISP1_Skip_XY_Offset[Now_Mode][2]+BH[i])/s_binn;
		if(BX[i] < 0) BX[i] = 0;
		BY[i] = ( ( (A2K.ISP1_Skip_XY_Offset[Now_Mode][3]+BV[i])/s_binn << 1)+1);
		if(BY[i] < 0) BY[i] = 0;

		B_RY[i] = ( ( ( (A2K.ISP1_Skip_XY_Offset[Now_Mode][4]+RV[i])/s_binn/(binn_mode+1) ) << 1)+1);
		if(B_RY[i] < 0) B_RY[i] = 0;
		B_BY[i] = ( ( ( (A2K.ISP1_Skip_XY_Offset[Now_Mode][5]+BV[i])/s_binn/(binn_mode+1) ) << 1)+1);
		if(B_BY[i] < 0) B_BY[i] = 0;

		//db_debug("set_ISP1_Binn_Start() i=%d s_binn=%d binn_mode=%d RH=%d RV=%d BH=%d BV=%d  RX=%d RY=%d BX=%d BY=%d B_RY=%d B_BY=%d\n",
		//		i, s_binn, binn_mode, RH[i], RV[i], BH[i], BV[i],
		//		RX[i], RY[i], BX[i], BY[i], B_RY[i], B_BY[i]);
   	}

    Cmd_p->F0_ISP1_CMD.S0_FXY_Offset.Data	= (BX[2] << 24) | (BY[2] << 16) | (RX[2] << 8) | RY[2];		//Sensor 2
    Cmd_p->F0_ISP1_CMD.S1_FXY_Offset.Data	= (BX[4] << 24) | (BY[4] << 16) | (RX[4] << 8) | RY[4];		//Sensor 4
    Cmd_p->F0_ISP1_CMD.S2_FXY_Offset.Data	= (BX[4] << 24) | (BY[4] << 16) | (RX[4] << 8) | RY[4];
    Cmd_p->F0_ISP1_CMD.S01_BXY_Offset.Data	= (B_BY[4] << 24) | (B_RY[4] << 16) | (B_BY[2] << 8) | B_RY[2];
    Cmd_p->F0_ISP1_CMD.S2_BXY_Offset.Data	= (B_BY[4] << 24) | (B_RY[4] << 16) | (B_BY[2] << 8) | B_RY[2];

    Cmd_p->F1_ISP1_CMD.S0_FXY_Offset.Data	= (BX[0] << 24) | (BY[0] << 16) | (RX[0] << 8) | RY[0];		//Sensor 0
    Cmd_p->F1_ISP1_CMD.S1_FXY_Offset.Data	= (BX[1] << 24) | (BY[1] << 16) | (RX[1] << 8) | RY[1];		//Sensor 1
    Cmd_p->F1_ISP1_CMD.S2_FXY_Offset.Data	= (BX[3] << 24) | (BY[3] << 16) | (RX[3] << 8) | RY[3];		//Sensor 3
    Cmd_p->F1_ISP1_CMD.S01_BXY_Offset.Data	= (B_BY[1] << 24) | (B_RY[1] << 16) | (B_BY[0] << 8) | B_RY[0];
    Cmd_p->F1_ISP1_CMD.S2_BXY_Offset.Data	= (B_BY[3] << 24) | (B_BY[3] << 16) | (B_BY[3] << 8) | B_RY[3];

    if(wdr_live_en == 1) {
    	switch(wdr_live_page) {
    	case 0: wb_a_ddr_f0 = ISP1_WB_P0_A_ADDR; wb_b_ddr_f0 = ISP1_WB_P0_B_ADDR; wb_c_ddr_f0 = ISP1_WB_P0_C_ADDR;
    			wd_a_ddr = ISP1_WD_2_P0_A_ADDR; wd_b_ddr = ISP1_WD_2_P0_B_ADDR; wd_c_ddr = ISP1_WD_2_P0_C_ADDR;
    	    	break;
    	case 1: wb_a_ddr_f0 = ISP1_WB_P1_A_ADDR; wb_b_ddr_f0 = ISP1_WB_P1_B_ADDR; wb_c_ddr_f0 = ISP1_WB_P1_C_ADDR;
    			wd_a_ddr = ISP1_WD_2_P1_A_ADDR; wd_b_ddr = ISP1_WD_2_P1_B_ADDR; wd_c_ddr = ISP1_WD_2_P1_C_ADDR;
    	    	break;
    	}
    	wb_a_ddr_f1 = wb_a_ddr_f0; wb_b_ddr_f1 = wb_a_ddr_f0; wb_c_ddr_f1 = wb_a_ddr_f0;
    }
    else {
    	if(hdr7_idx >= 0) {
            switch(hdr7_idx) {
            case PIPE_SUBCODE_AEB_STEP_0:
            	wb_a_ddr_f0 = FX_WDR_MO_A_P1_ADDR; wb_b_ddr_f0 = FX_WDR_MO_B_P1_ADDR; wb_c_ddr_f0 = FX_WDR_MO_C_P1_ADDR;
              	wb_a_ddr_f1 = FX_WDR_MO_C_P1_ADDR; wb_b_ddr_f1 = FX_WDR_MO_B_P1_ADDR; wb_c_ddr_f1 = FX_WDR_MO_A_P1_ADDR;
              	break;
            case PIPE_SUBCODE_AEB_STEP_1:
            	wb_a_ddr_f0 = FX_WDR_MO_A_P2_ADDR; wb_b_ddr_f0 = FX_WDR_MO_B_P2_ADDR; wb_c_ddr_f0 = FX_WDR_MO_C_P2_ADDR;
               	wb_a_ddr_f1 = FX_WDR_MO_C_P2_ADDR; wb_b_ddr_f1 = FX_WDR_MO_B_P2_ADDR; wb_c_ddr_f1 = FX_WDR_MO_A_P2_ADDR;
               	break;
            case PIPE_SUBCODE_AEB_STEP_2:
            	wb_a_ddr_f0 = FX_WDR_MO_A_P3_ADDR; wb_b_ddr_f0 = FX_WDR_MO_B_P3_ADDR; wb_c_ddr_f0 = FX_WDR_MO_C_P3_ADDR;
               	wb_a_ddr_f1 = FX_WDR_MO_C_P3_ADDR; wb_b_ddr_f1 = FX_WDR_MO_B_P3_ADDR; wb_c_ddr_f1 = FX_WDR_MO_A_P3_ADDR;
               	break;
            case PIPE_SUBCODE_AEB_STEP_3:
            	wb_a_ddr_f0 = FX_WDR_MO_A_P4_ADDR; wb_b_ddr_f0 = FX_WDR_MO_B_P4_ADDR; wb_c_ddr_f0 = FX_WDR_MO_C_P4_ADDR;
               	wb_a_ddr_f1 = FX_WDR_MO_C_P4_ADDR; wb_b_ddr_f1 = FX_WDR_MO_B_P4_ADDR; wb_c_ddr_f1 = FX_WDR_MO_A_P4_ADDR;
               	break;
            case PIPE_SUBCODE_AEB_STEP_4:
            	wb_a_ddr_f0 = FX_WDR_MO_A_P5_ADDR; wb_b_ddr_f0 = FX_WDR_MO_B_P5_ADDR; wb_c_ddr_f0 = FX_WDR_MO_C_P5_ADDR;
               	wb_a_ddr_f1 = FX_WDR_MO_C_P5_ADDR; wb_b_ddr_f1 = FX_WDR_MO_B_P5_ADDR; wb_c_ddr_f1 = FX_WDR_MO_A_P5_ADDR;
               	break;
            case PIPE_SUBCODE_AEB_STEP_5:
            	wb_a_ddr_f0 = FX_WDR_MO_A_P6_ADDR; wb_b_ddr_f0 = FX_WDR_MO_B_P6_ADDR; wb_c_ddr_f0 = FX_WDR_MO_C_P6_ADDR;
               	wb_a_ddr_f1 = FX_WDR_MO_C_P6_ADDR; wb_b_ddr_f1 = FX_WDR_MO_B_P6_ADDR; wb_c_ddr_f1 = FX_WDR_MO_A_P6_ADDR;
               	break;
            case PIPE_SUBCODE_AEB_STEP_6:
            	wb_a_ddr_f0 = FX_WDR_MO_A_P0_ADDR; wb_b_ddr_f0 = FX_WDR_MO_B_P0_ADDR; wb_c_ddr_f0 = FX_WDR_MO_C_P0_ADDR;
               	wb_a_ddr_f1 = FX_WDR_MO_C_P0_ADDR; wb_b_ddr_f1 = FX_WDR_MO_B_P0_ADDR; wb_c_ddr_f1 = FX_WDR_MO_A_P0_ADDR;
               	break;
            case PIPE_SUBCODE_AEB_STEP_E:
            	wb_a_ddr_f0 = FX_WDR_MO_A_P0_ADDR; wb_b_ddr_f0 = FX_WDR_MO_B_P0_ADDR; wb_c_ddr_f0 = FX_WDR_MO_C_P0_ADDR;
               	wb_a_ddr_f1 = FX_WDR_MO_C_P0_ADDR; wb_b_ddr_f1 = FX_WDR_MO_B_P0_ADDR; wb_c_ddr_f1 = FX_WDR_MO_A_P0_ADDR;
               	break;
            default:
            	wb_a_ddr_f0 = FX_WDR_MO_A_P0_ADDR; wb_b_ddr_f0 = FX_WDR_MO_B_P0_ADDR; wb_c_ddr_f0 = FX_WDR_MO_C_P0_ADDR;
               	wb_a_ddr_f1 = FX_WDR_MO_C_P0_ADDR; wb_b_ddr_f1 = FX_WDR_MO_B_P0_ADDR; wb_c_ddr_f1 = FX_WDR_MO_A_P0_ADDR;
               	break;
            }
    	}
    	else {
    		wb_a_ddr_f0 = ISP1_WB_P0_A_ADDR; wb_b_ddr_f0 = ISP1_WB_P0_B_ADDR; wb_c_ddr_f0 = ISP1_WB_P0_C_ADDR;
    		wb_a_ddr_f1 = wb_a_ddr_f0; wb_b_ddr_f1 = wb_a_ddr_f0; wb_c_ddr_f1 = wb_a_ddr_f0;
    	}

    	wd_a_ddr = ISP1_WD_2_P0_A_ADDR; wd_b_ddr = ISP1_WD_2_P0_B_ADDR; wd_c_ddr = ISP1_WD_2_P0_C_ADDR;
    }

	Cmd_p->F0_ISP1_CMD.SA_WD_DDR_Addr.Data          = wd_a_ddr >> 5;
	Cmd_p->F0_ISP1_CMD.SB_WD_DDR_Addr.Data          = wd_b_ddr >> 5;
	Cmd_p->F0_ISP1_CMD.SC_WD_DDR_Addr.Data          = wd_c_ddr >> 5;
	Cmd_p->F0_ISP1_CMD.SA_WB_DDR_Addr.Data          = wb_a_ddr_f0 >> 5;
	Cmd_p->F0_ISP1_CMD.SB_WB_DDR_Addr.Data          = wb_b_ddr_f0 >> 5;
	Cmd_p->F0_ISP1_CMD.SC_WB_DDR_Addr.Data          = wb_c_ddr_f0 >> 5;
	Cmd_p->F1_ISP1_CMD.SA_WD_DDR_Addr.Data          = wd_a_ddr >> 5;
	Cmd_p->F1_ISP1_CMD.SB_WD_DDR_Addr.Data          = wd_b_ddr >> 5;
	Cmd_p->F1_ISP1_CMD.SC_WD_DDR_Addr.Data          = wd_c_ddr >> 5;
	Cmd_p->F1_ISP1_CMD.SA_WB_DDR_Addr.Data          = wb_a_ddr_f1 >> 5;
	Cmd_p->F1_ISP1_CMD.SB_WB_DDR_Addr.Data          = wb_b_ddr_f1 >> 5;
	Cmd_p->F1_ISP1_CMD.SC_WB_DDR_Addr.Data          = wb_c_ddr_f1 >> 5;

    Cmd_p->F0_ISP1_CMD.WDR_Start_En.Data = wdr_live_en;
    Cmd_p->F1_ISP1_CMD.WDR_Start_En.Data = wdr_live_en;

//    if(wdr_live_en == 0)
//    	db_debug("set_ISP1_Binn_Start() M_Mode=%d wdr_en=%d wdr_idx=%d hdr_7idx=%d  wb_a_ddr=0x%x wd_a_ddr=0x%x\n",
//    			M_Mode, wdr_live_en, wdr_live_idx, wdr_live_page, wb_a_ddr, wd_a_ddr);
//    else
//    	db_error("set_ISP1_Binn_Start() M_Mode=%d wdr_en=%d wdr_idx=%d hdr_7idx=%d  wb_a_ddr=0x%x wd_a_ddr=0x%x\n",
//    			M_Mode, wdr_live_en, wdr_live_idx, wdr_live_page, wb_a_ddr, wd_a_ddr);

    int idx, np_h = -1;
    int mo_en = 0, mo_mul = 2;
    if( (C_Mode == 5 || C_Mode == 7) && hdr7_idx >= 0) {		//HDR Mode, DeGhost
    	mo_mul = info.hdr_ev * 256;	//info.hdr_ev * 2 * 256 / 20;
        mo_en = 1;
    }
    else {
        mo_mul = 256;
        mo_en = 0;
    }
    if(mo_mul >= 2047) mo_mul = 2047;
    Cmd_p->F0_ISP1_CMD.Mo_Mul.Data = mo_mul;
    Cmd_p->F1_ISP1_CMD.Mo_Mul.Data = mo_mul;
    Cmd_p->F0_ISP1_CMD.Mo_En.Data  = mo_en;
    Cmd_p->F1_ISP1_CMD.Mo_En.Data  = mo_en;


    int noise_th = 0;
    noise_th = A2K.ISP1_Noise_Th;
    Cmd_p->F0_ISP1_CMD.Noise_TH.Data  = noise_th & 0x3FF;
    Cmd_p->F1_ISP1_CMD.Noise_TH.Data  = noise_th & 0x3FF;
}

void set_ISP1_LC_Addr(int s_binn, AS2_F0_MAIN_CMD_struct *Cmd_p, int en)
{
    int addr_a, addr_b, addr_c;
    switch(s_binn) {
    case 1:
        addr_a = FX_LC_FS_A_BUF_Addr;
        addr_b = FX_LC_FS_B_BUF_Addr;
        addr_c = FX_LC_FS_C_BUF_Addr;
        break;
    case 2:
        addr_a = FX_LC_D2_A_BUF_Addr;
        addr_b = FX_LC_D2_B_BUF_Addr;
        addr_c = FX_LC_D2_C_BUF_Addr;
        break;
    case 3:
        addr_a = FX_LC_D3_A_BUF_Addr;
        addr_b = FX_LC_D3_B_BUF_Addr;
        addr_c = FX_LC_D3_C_BUF_Addr;
        break;
    }
    Cmd_p->F0_ISP1_CMD.S0_LC_DDR_Addr.Data     = (en << 31) | (addr_a >> 5);
    Cmd_p->F0_ISP1_CMD.S1_LC_DDR_Addr.Data     = (en << 31) | (addr_b >> 5);
    Cmd_p->F0_ISP1_CMD.S2_LC_DDR_Addr.Data     = (en << 31) | (addr_c >> 5);

    Cmd_p->F1_ISP1_CMD.S0_LC_DDR_Addr.Data     = (en << 31) | (addr_c >> 5);
    Cmd_p->F1_ISP1_CMD.S1_LC_DDR_Addr.Data     = (en << 31) | (addr_b >> 5);
    Cmd_p->F1_ISP1_CMD.S2_LC_DDR_Addr.Data     = (en << 31) | (addr_a >> 5);
}

int AWB_TH_Max = 32, AWB_TH_Min = 0;
void SetAWBTHDebug(int idx, int value)
{
	if(idx == 0) AWB_TH_Max = value;
	else		 AWB_TH_Min = value;
}

void GetAWBTHDebug(int *value)
{
	*value 	   = AWB_TH_Max;
	*(value+1) = AWB_TH_Min;
}
int get_AWB_TH(int sel)
{
    int ret;
    if(sel == 1) ret = AWB_TH_Max;
    else         ret = AWB_TH_Min;
    return ret;
}

/*
 * type: 0:ISP1	1:JPEG
 */
void Set_HDR_AEB_Info(JPEG_HDR_AEB_Info_Struct *info, int type, int idx, int c_mode)
{
	int now_idx, now_line, now_gain, now_aeg;
	int next_idx, next_line, next_gain, next_aeg;
	int mid_idx, mid_line, mid_gain, mid_aeg;
	float now_g_rate, next_g_rate, mid_g_rate;
	float ep_rate, gain_rate, rate;
	int istart, half;
	int now_tmp, next_tmp, mid_tmp;
	int now_pow_tmp, next_pow_tmp, mid_pow_tmp;
	HDR7P_AEG_Parameter_Struct now_para, next_para, mid_para;

	if(c_mode == 13) {			//Removal
		info->hdr_manual 	 = get_Removal_HDRMode();
		info->hdr_np 		 = 5;
		info->strength		 = get_Removal_HDR_Strength();
	}
	else if(c_mode == 3) {		//AEB
		info->hdr_manual 	 = 0;
		info->hdr_np 		 = get_AEBFrameCnt();
		info->strength		 = 0;
	}
	else {
		info->hdr_manual 	 = get_HDRManual();
		info->hdr_np 		 = get_HDR7P();
		info->strength		 = get_HDR7P_Strength();
	}
	if(type == 0) {				//ISP1, Diffusion
		istart = (7 - info->hdr_np) >> 1;				//配合 do_AEB_sensor_cal() 的 istep
		half = info->hdr_np >> 1;
		now_idx  = (idx - PIPE_SUBCODE_AEB_STEP_0) + istart;
		next_idx = now_idx + 1;
		if(next_idx > (3+half) ) next_idx = (3+half);

		now_para  = Get_HDR7P_AEG_Idx(now_idx);
		now_aeg   = now_para.aeg_idx;
		now_line  = now_para.ep_line;
		if(now_line < 1) now_line = 1;
		now_gain  = now_para.gain;

		next_para = Get_HDR7P_AEG_Idx(next_idx);
		next_aeg  = next_para.aeg_idx;
		next_line = next_para.ep_line;
		if(next_line < 1) next_line = 1;
		next_gain = next_para.gain;

		ep_rate = (float)next_line / (float)now_line;

		//=(MOD(H7;64)/64 +1)* 2^(INT(H7/64))
		//now_tmp = now_gain / 1280;
		//now_pow_tmp = pow(2, now_tmp);
		now_g_rate = pow(2, (float)now_gain / 1280.0);		//( (now_gain % 1280) / 1280.0 + 1.0) * now_pow_tmp;

		//next_tmp = next_gain / 1280;
		//next_pow_tmp = pow(2, next_tmp);
		next_g_rate = pow(2, (float)next_gain / 1280.0);		//( (next_gain % 1280) / 1280.0 + 1.0) * next_pow_tmp;

		gain_rate = next_g_rate / now_g_rate;
		//gain_rate = pow(2, (float)(next_gain - now_gain) / 1280.0);

		rate = (ep_rate * gain_rate);
		if(rate == 0) info->hdr_ev = 1;			//1倍
		else		  info->hdr_ev = rate;
		//info->hdr_ev = pow(2, (float)(next_aeg - now_aeg) / 1280.0) * 10;


		// 計算 hdr_ev_mid
		mid_idx = half + istart;
		mid_para  = Get_HDR7P_AEG_Idx(mid_idx);
		mid_aeg   = mid_para.aeg_idx;
		mid_line  = mid_para.ep_line;
		if(mid_line < 1) mid_line = 1;
		mid_gain  = mid_para.gain;

		ep_rate = (float)now_line / (float)mid_line;
		mid_g_rate = pow(2, (float)mid_gain / 1280.0);
		gain_rate = now_g_rate / mid_g_rate;

		rate = (ep_rate * gain_rate);
		if(rate == 0) info->hdr_ev_mid = 1;			//1倍
		else		  info->hdr_ev_mid = rate;
if(idx != -1)
	db_debug("Set_HDR_AEB_Info() idx=%d istart=%d half=%d now_idx=%d mid_idx=%d hdr_ev=%f now_line=%d now_g_rate=%f mid_line=%d mid_g_rate=%f ep_rate=%f gain_rate=%f hdr_ev_mid=%f\n",
			idx, istart, half, now_idx, mid_idx, info->hdr_ev, now_line, now_g_rate, mid_line, mid_g_rate, ep_rate, gain_rate, info->hdr_ev_mid);
	}
	else {
		if(c_mode == 13)
			info->hdr_ev 		 = get_Removal_HDREv();
		else
			info->hdr_ev 		 = get_HDREv();
	}
	info->hdr_auto_ev[0] = get_HDR_Auto_Ev(0) * 20;		//EV+0
	info->hdr_auto_ev[1] = get_HDR_Auto_Ev(1) * 20;		//EV-5
	info->aeb_np      	 = get_AEBFrameCnt();
	info->aeb_ev		 = get_AEBEv();
}

/*
 * return: 0 -> wait times up
 *         1 -> execute;
 */
int Make_ISP1_Cmd(int idx, AS2_F0_MAIN_CMD_struct *Cmd_p, FPGA_Engine_One_Struct *F_Pipe_p, F_Com_In_Struct *F_Com_p)
{
    int C_Mode = F_Com_p->Record_Mode;
    int M_Mode = F_Pipe_p->M_Mode;
    int o_page = (F_Pipe_p->O_Page % 10);
    int DebugJPEGMode = A2K.DebugJPEGMode;
    int ISP2_Sensor = A2K.ISP2_Sensor;
    int big_img=1, fpga_binn=0, s_binn=1, binn_mode=DS_ISP1_BINN_3X3, hdr_7idx, hdr_fcnt;
    int Now_Mode;
    int wdr_live_en, wdr_live_idx, wdr_live_page;
    int lc_en;
    int d_cnt = read_F_Com_In_Capture_D_Cnt();
    JPEG_HDR_AEB_Info_Struct hdr_aeb_info;
    
    if(F_Pipe_p->Time_Buf <= 0) return 0;

    get_F_Temp_ISP1_SubData(idx, &hdr_7idx, &hdr_fcnt, &Now_Mode, &wdr_live_idx, &wdr_live_page, &lc_en);
    if(C_Mode == 3 || C_Mode == 5 || C_Mode == 7 || C_Mode == 13) Set_HDR_AEB_Info(&hdr_aeb_info, 0, hdr_7idx, C_Mode);
    if(wdr_live_page != -1) wdr_live_en = 1;
    else                    wdr_live_en = 0;

    if(DebugJPEGMode == 1 && (ISP2_Sensor == -4 || ISP2_Sensor == -6 || ISP2_Sensor == -9) ) {
        C_Mode = 0; M_Mode = 0;
        o_page = 0;
    }
    switch(C_Mode) {
    case 5: // HDR (1p)
    case 7: // Night + HDR
    case 13:// Removal
        switch(M_Mode) {
        case 0: big_img=1; fpga_binn=2; s_binn=1; binn_mode=DS_ISP1_BINN_8X8; break;     // 12K
        case 1: big_img=1; fpga_binn=2; s_binn=2; binn_mode=DS_ISP1_BINN_8X8; break;     // 8K
        case 2: big_img=1; fpga_binn=2; s_binn=2; binn_mode=DS_ISP1_BINN_8X8; break;     // 6K
        case 3: big_img=0; fpga_binn=1; s_binn=1; binn_mode=DS_ISP1_BINN_3X3; break;     // 4K, preview
        case 4: big_img=0; fpga_binn=1; s_binn=2; binn_mode=DS_ISP1_BINN_2X2; break;     // 3K, preview
        }
        break;
    case 0: // Cap
    case 3: // AEB (3p,5p,7p)
    case 4: // RAW (5p)
    case 6: // Night
    case 8: // Sport
    case 9: // Sport + WDR
    case 12:// M-Mode
    case 14:// 3D-Model
        switch(M_Mode) {
        case 0: big_img=1; fpga_binn=0; s_binn=1; binn_mode=DS_ISP1_BINN_3X3; break;     // 12K
        case 1: big_img=1; fpga_binn=0; s_binn=2; binn_mode=DS_ISP1_BINN_2X2; break;     // 8K
        case 2: big_img=1; fpga_binn=0; s_binn=2; binn_mode=DS_ISP1_BINN_2X2; break;     // 6K
        case 3: big_img=0; fpga_binn=1; s_binn=1; binn_mode=DS_ISP1_BINN_3X3; break;     // 4K/1K, preview
        case 4: big_img=0; fpga_binn=1; s_binn=2; binn_mode=DS_ISP1_BINN_2X2; break;     // 3K, preview
        }
        break;
    case 1:// Rec
    case 10:// Rec + WDR
        switch(M_Mode) {
        case 3: big_img=0; fpga_binn=0; s_binn=3; binn_mode=DS_ISP1_BINN_OFF; break;     // 4K
        case 4: big_img=0; fpga_binn=1; s_binn=2; binn_mode=DS_ISP1_BINN_2X2; break;     // 3K
        case 5: big_img=0; fpga_binn=1; s_binn=2; binn_mode=DS_ISP1_BINN_3X3; break;     // 2K
        }
        break;
    case 2:// Time Lapse
    case 11:// Time Lapse + WDR
        switch(M_Mode) {
        case 0: big_img=1; fpga_binn=0; s_binn=1; binn_mode=DS_ISP1_BINN_3X3; break;     // 12K
        case 1: big_img=1; fpga_binn=0; s_binn=2; binn_mode=DS_ISP1_BINN_2X2; break;     // 8K
        case 2: big_img=1; fpga_binn=0; s_binn=2; binn_mode=DS_ISP1_BINN_2X2; break;     // 6K
        case 3: big_img=0; fpga_binn=0; s_binn=3; binn_mode=DS_ISP1_BINN_OFF; break;     // 4K
        }
        break;
    }
    if(hdr_7idx >= 0 || big_img == 1){
        db_error("Make_ISP1_Cmd: idx=%d 7idx=%d fcnt=%d o_page=%d big=%d fpga=%d\n", idx, hdr_7idx, hdr_fcnt, o_page, big_img, fpga_binn);
    }
    
    set_ISP1_P0_P1_P2_Addr(big_img, fpga_binn, o_page, hdr_7idx, hdr_fcnt, Cmd_p);
    set_ISP1_Binn_Start(M_Mode, fpga_binn, s_binn, binn_mode, Cmd_p, Now_Mode, wdr_live_en, wdr_live_idx, wdr_live_page, hdr_7idx, hdr_fcnt, C_Mode, hdr_aeb_info);
    set_ISP1_LC_Addr(s_binn, Cmd_p, lc_en);
    
    int gamma_idx = get_FX_Gamma_Addr_Idx();        // gamma_idx使用1,2,3
    Gamma_Set_Function();
    if(Check_Is_HDR_Mode(C_Mode) != 1 && C_Mode != 3 && d_cnt != 0) {		//WDR, 拍照
    	Cap_Gamma_Set_Function(AEG_gain_H);		//拍照才做限制, 避免Live畫面閃爍
    	gamma_idx = 2;
    }
    Cmd_p->F0_ISP1_CMD.GAMMA_DDR_Addr.Data = (FX_GAMMA_ADDR + (2048 * gamma_idx)) >> 5;
    Cmd_p->F1_ISP1_CMD.GAMMA_DDR_Addr.Data = (FX_GAMMA_ADDR + (2048 * gamma_idx)) >> 5;

    int ost_I = A2K.ISP1_RGB_Offset_I;
    int ost_O = A2K.ISP1_RGB_Offset_O;
    Cmd_p->F0_ISP1_CMD.RGB_offset.Data      = ( (ost_O << 4) | ost_I);
    Cmd_p->F1_ISP1_CMD.RGB_offset.Data      = ( (ost_O << 4) | ost_I);
    
    if(A2K.ISP1_Timeout_F0[0] != 0 && A2K.ISP1_Timeout_F1[0] != 0){
        Cmd_p->F0_ISP1_CMD.S0_TimeOut_Set.Data  = ((1 << 31) | A2K.ISP1_Timeout_F0[0]);
        Cmd_p->F0_ISP1_CMD.S1_TimeOut_Set.Data  = ((1 << 31) | A2K.ISP1_Timeout_F0[1]);
        Cmd_p->F0_ISP1_CMD.S2_TimeOut_Set.Data  = ((1 << 31) | A2K.ISP1_Timeout_F0[2]);

        Cmd_p->F1_ISP1_CMD.S0_TimeOut_Set.Data  = ((1 << 31) | A2K.ISP1_Timeout_F1[0]);
        Cmd_p->F1_ISP1_CMD.S1_TimeOut_Set.Data  = ((1 << 31) | A2K.ISP1_Timeout_F1[1]);
        Cmd_p->F1_ISP1_CMD.S2_TimeOut_Set.Data  = ((1 << 31) | A2K.ISP1_Timeout_F1[2]);
    }
//    if(A2K.ISP1_Color_Matrix[0] != 0){                                  // Matrix
//        AS2_CMD_IO_struct *f0_m = &Cmd_p->F0_ISP1_CMD.M_00;
//        AS2_CMD_IO_struct *f1_m = &Cmd_p->F1_ISP1_CMD.M_00;
//        int i;
//        for(i = 0; i < 9; i++){
//            f0_m->Data = A2K.ISP1_Color_Matrix[i];
//            f1_m->Data = A2K.ISP1_Color_Matrix[i];
//            //db_debug("F0_ISP1_CMD.M_00: addr=%x data=%x\n", f0_m->Address, f0_m->Data);
//            f0_m++;
//            f1_m++;
//        }
//    }
    //{                                                                   // Gain
        int i;
        AS2_CMD_IO_struct *f0_g = &Cmd_p->F0_ISP1_CMD.R_GainI;
        AS2_CMD_IO_struct *f1_g = &Cmd_p->F1_ISP1_CMD.R_GainI;
        for(i = 0; i < 6; i++){
        	f0_g->Data = A2K.ISP1_RGB_Gain[i]<<16;
        	f1_g->Data = A2K.ISP1_RGB_Gain[i]<<16;
            f0_g++;
            f1_g++;
        }
    //}

    Cmd_p->F0_ISP1_CMD.AWB_TH.Data = (AWB_TH_Max << 8) | AWB_TH_Min;
    Cmd_p->F1_ISP1_CMD.AWB_TH.Data = (AWB_TH_Max << 8) | AWB_TH_Min;

    return 1;
}

void AS2_Diffusion_Address_Set(AS2_Diffusion_CMD_struct *SP)
{
	SP->Start_En_0.Address 		= 0xCCAAC00D;
	SP->CMD_DW0_0.Address 		= 0xCCAAC001;
	SP->SIZE_0.Address 		    = 0xCCAAC004;
	SP->TAB_ADDR_P_0.Address 	= 0xCCAAC008;
	SP->S0_DDR_P_0.Address 		= 0xCCAAC002;
	SP->S1_DDR_P_0.Address 		= 0xCCAAC009;
	SP->T_DDR_P_0.Address 		= 0xCCAAC003;
	SP->S0_DDR_Offset_0.Address = 0xCCAAC005;
	SP->S1_DDR_Offset_0.Address = 0xCCAAC006;
	SP->T_DDR_Offset_0.Address 	= 0xCCAAC007;

	SP->Start_En_1.Address 		= 0xCCAAC03D;
	SP->CMD_DW0_1.Address 		= 0xCCAAC031;
	SP->SIZE_1.Address 			= 0xCCAAC034;
	SP->TAB_ADDR_P_1.Address 	= 0xCCAAC038;
	SP->S0_DDR_P_1.Address 		= 0xCCAAC032;
	SP->S1_DDR_P_1.Address 		= 0xCCAAC039;
	SP->T_DDR_P_1.Address 		= 0xCCAAC033;
	SP->S0_DDR_Offset_1.Address = 0xCCAAC035;
	SP->S1_DDR_Offset_1.Address = 0xCCAAC036;
	SP->T_DDR_Offset_1.Address 	= 0xCCAAC037;

	SP->Start_En_2.Address 		= 0xCCAAC06D;
	SP->CMD_DW0_2.Address 		= 0xCCAAC061;
	SP->SIZE_2.Address 			= 0xCCAAC064;
	SP->TAB_ADDR_P_2.Address 	= 0xCCAAC068;
	SP->S0_DDR_P_2.Address 		= 0xCCAAC062;
	SP->S1_DDR_P_2.Address 		= 0xCCAAC069;
	SP->T_DDR_P_2.Address 		= 0xCCAAC063;
	SP->S0_DDR_Offset_2.Address = 0xCCAAC065;
	SP->S1_DDR_Offset_2.Address = 0xCCAAC066;
	SP->T_DDR_Offset_2.Address 	= 0xCCAAC067;

	SP->Start_En_3.Address 		= 0xCCAAC09D;
	SP->CMD_DW0_3.Address 		= 0xCCAAC091;
	SP->SIZE_3.Address 			= 0xCCAAC094;
	SP->TAB_ADDR_P_3.Address 	= 0xCCAAC098;
	SP->S0_DDR_P_3.Address 		= 0xCCAAC092;
	SP->S1_DDR_P_3.Address 		= 0xCCAAC099;
	SP->T_DDR_P_3.Address 		= 0xCCAAC093;
	SP->S0_DDR_Offset_3.Address = 0xCCAAC095;
	SP->S1_DDR_Offset_3.Address = 0xCCAAC096;
	SP->T_DDR_Offset_3.Address 	= 0xCCAAC097;

	SP->Start_En_4.Address 		= 0xCCAAC0CD;
	SP->CMD_DW0_4.Address 		= 0xCCAAC0C1;
	SP->SIZE_4.Address 			= 0xCCAAC0C4;
	SP->TAB_ADDR_P_4.Address 	= 0xCCAAC0C8;
	SP->S0_DDR_P_4.Address 		= 0xCCAAC0C2;
	SP->S1_DDR_P_4.Address 		= 0xCCAAC0C9;
	SP->T_DDR_P_4.Address 		= 0xCCAAC0C3;
	SP->S0_DDR_Offset_4.Address = 0xCCAAC0C5;
	SP->S1_DDR_Offset_4.Address = 0xCCAAC0C6;
	SP->T_DDR_Offset_4.Address 	= 0xCCAAC0C7;

	SP->Start_En_5.Address 		= 0xCCAAC0FD;
	SP->CMD_DW0_5.Address 		= 0xCCAAC0F1;
	SP->SIZE_5.Address 			= 0xCCAAC0F4;
	SP->TAB_ADDR_P_5.Address 	= 0xCCAAC0F8;
	SP->S0_DDR_P_5.Address 		= 0xCCAAC0F2;
	SP->S1_DDR_P_5.Address 		= 0xCCAAC0F9;
	SP->T_DDR_P_5.Address 		= 0xCCAAC0F3;
	SP->S0_DDR_Offset_5.Address = 0xCCAAC0F5;
	SP->S1_DDR_Offset_5.Address = 0xCCAAC0F6;
	SP->T_DDR_Offset_5.Address 	= 0xCCAAC0F7;

	//SP->Mo_Mul_D.Address 		= 0xCCAAC00F;

	SP->REV[0].Address    		= 0xCCAA03F3;
}
int Motion_Th = 20, Motion_Diff_Pix = 10, Overlay_Mul = 11, DeGhost_Th = 32, DeGhost_Mul = 0;
void SetDeGhostParameter(int idx, int value)
{
	switch(idx) {
	case 0: Motion_Th       = value; break;
	case 1: Motion_Diff_Pix = value; break;
	case 2: Overlay_Mul     = value; break;
	case 3: DeGhost_Th      = value; break;
	}
}

int GetDeGhostParameter(int idx)
{
	int value;
	switch(idx) {
	case 0: value = Motion_Th; 		 break;
	case 1: value = Motion_Diff_Pix; break;
	case 2: value = Overlay_Mul; 	 break;
	case 3: value = DeGhost_Th; 	 break;
	}
	return value;
}

unsigned Get_OV0_S1_ADDR(int t_idx, int mid, int en)
{
	unsigned addr, idx;

	if(en == 0)
		idx = t_idx;
	else {
		if(t_idx > mid) idx = t_idx - 1;
		else			idx = t_idx + 1;
	}
	switch(idx) {
	case 0: addr = FX_WDR_MO_DIFF_A_P0_ADDR; break;
	case 1: addr = FX_WDR_MO_DIFF_A_P1_ADDR; break;
	case 2: addr = FX_WDR_MO_DIFF_A_P2_ADDR; break;
	case 3: addr = FX_WDR_MO_DIFF_A_P3_ADDR; break;
	case 4: addr = FX_WDR_MO_DIFF_A_P4_ADDR; break;
	case 5: addr = FX_WDR_MO_DIFF_A_P5_ADDR; break;
	case 6: addr = FX_WDR_MO_DIFF_A_P6_ADDR; break;
	}
	return addr;
}
void AS2_DeGhost_Set(AS2_Diffusion_CMD_struct *SP, int s_idx, int t_idx, int m_idx, int M, int FX, int DeGhostEn, int mid, JPEG_HDR_AEB_Info_Struct info)
{
	Diff_CMD_struct	    DG_P[6];
//	unsigned *PP0;
//	unsigned *SPP;
	int i;
	int Diff_pix;
	int Diff_Divisor;
	int Mo_Mul, Mo_Mul_0, Mo_Mul_1, Mo_Mul_2, Mo_Mul_3;
	int Mo_TH;
	int Mo_Diff_Mode;
	unsigned Df0_TAB_DDR_P,Df2_TAB_DDR_P;
	unsigned Mo_S0_DDR_P, Mo_S1_DDR_P, Mo_T_DDR_P;
	unsigned Mo_S0_Offset, Mo_S1_Offset, Mo_T_Offset;

	unsigned Df0_S0_DDR_P, Df0_S1_DDR_P, Df0_T_DDR_P;
	unsigned Df0_S0_Offset, Df0_S1_Offset, Df0_T_Offset;

	unsigned Df1_S0_DDR_P, Df1_S1_DDR_P, Df1_T_DDR_P;
	unsigned Df1_S0_Offset, Df1_S1_Offset, Df1_T_Offset;

	unsigned Df2_S0_DDR_P, Df2_S1_DDR_P, Df2_T_DDR_P;
	unsigned Df2_S0_Offset, Df2_S1_Offset, Df2_T_Offset;

	unsigned Df3_S0_DDR_P, Df3_S1_DDR_P, Df3_T_DDR_P;
	unsigned Df3_S0_Offset, Df3_S1_Offset, Df3_T_Offset;

	unsigned OV0_En;
	unsigned OV0_S0_DDR_P, OV0_S1_DDR_P, OV0_T_DDR_P;
	unsigned OV0_S0_Offset, OV0_S1_Offset, OV0_T_Offset;

//	unsigned OV1_S0_DDR_P, OV1_S1_DDR_P, OV1_T_DDR_P;
//	unsigned OV1_S0_Offset, OV1_S1_Offset, OV1_T_Offset;
if(FX == 0) db_debug("AS2_DeGhost_Set() HDR: s_idx=%d t_idx=%d m_idx=%d\n", s_idx, t_idx, m_idx);
	/*
	 1. HDR's Diffision (1st)			:Df1
	 2. HDR's Diffision (2nd)			:Df2
     3. HDR's Diffision (3nd)			:Df3
	 4. Motion							:MO
	 5. Motion's Diffision				:Df0
	 6. Overlay(Now_Df0 + Lst_Df0)		:OV0
	 *
	 * 	IMG	 -> Df1 --------> Df2
	 *	 	 |  				  |
	 *	 |---|	 |----------> Df3
	 *	 	 |
	 *	MO_S -> MO  -> Df0 -> OV0
	 *
	 *	OV0:
	 *	t_id: 	0	1	2	3	4	5	6
	 *			|	|___|		|___|	|
	 *			|___|				|___|
	 *			|						|
	 */
	int Test_Enable[6] = {1,1,1,1,1,1};
	Df0_TAB_DDR_P = (FX_WDR_TABLE_ADDR + 0x300) >> 5;
	Df2_TAB_DDR_P = (FX_WDR_TABLE_ADDR + M * 0x100) >> 5;
	switch(s_idx) {
		case 0:
			Df1_S0_DDR_P = FX_WDR_IMG_A_P0_ADDR >> 5;
			Df1_S1_DDR_P = 0;
			break;
		case 1:
			Df1_S0_DDR_P = FX_WDR_IMG_A_P1_ADDR >> 5;
			Df1_S1_DDR_P = 0;
			break;
		case 2:
			Df1_S0_DDR_P = FX_WDR_IMG_A_P2_ADDR >> 5;
			Df1_S1_DDR_P = 0;
			break;
		case 3:
			Df1_S0_DDR_P = FX_WDR_IMG_A_P3_ADDR >> 5;
			Df1_S1_DDR_P = 0;
			break;
		case 4:
			Df1_S0_DDR_P = FX_WDR_IMG_A_P4_ADDR >> 5;
			Df1_S1_DDR_P = 0;
			break;
		case 5:
			Df1_S0_DDR_P = FX_WDR_IMG_A_P5_ADDR >> 5;
			Df1_S1_DDR_P = 0;
			break;
		case 6:
			Df1_S0_DDR_P = FX_WDR_IMG_A_P6_ADDR >> 5;
			Df1_S1_DDR_P = 0;
			break;
	}
	switch(m_idx) {
		case 0:
			Mo_S0_DDR_P = (FX_WDR_IMG_A_P0_ADDR >> 5) + 1;		//FX_WDR_MO_A_P0_ADDR >> 5;
			Mo_S1_DDR_P = FX_WDR_MO_A_P0_ADDR >> 5;
			break;
		case 1:
			Mo_S0_DDR_P = (FX_WDR_IMG_A_P1_ADDR >> 5) + 1;
			Mo_S1_DDR_P = FX_WDR_MO_A_P1_ADDR >> 5;
			break;
		case 2:
			Mo_S0_DDR_P = (FX_WDR_IMG_A_P2_ADDR >> 5) + 1;
			Mo_S1_DDR_P = FX_WDR_MO_A_P2_ADDR >> 5;
			break;
		case 3:
			Mo_S0_DDR_P = (FX_WDR_IMG_A_P3_ADDR >> 5) + 1;
			Mo_S1_DDR_P = FX_WDR_MO_A_P3_ADDR >> 5;
			break;
		case 4:
			Mo_S0_DDR_P = (FX_WDR_IMG_A_P4_ADDR >> 5) + 1;
			Mo_S1_DDR_P = FX_WDR_MO_A_P4_ADDR >> 5;
			break;
		case 5:
			Mo_S0_DDR_P = (FX_WDR_IMG_A_P5_ADDR >> 5) + 1;
			Mo_S1_DDR_P = FX_WDR_MO_A_P5_ADDR >> 5;
			break;
		case 6:
			Mo_S0_DDR_P = (FX_WDR_IMG_A_P6_ADDR >> 5) + 1;
			Mo_S1_DDR_P = FX_WDR_MO_A_P6_ADDR >> 5;
			break;
	}
	if(t_idx == mid)	//中間那張不做Motion
			Mo_S1_DDR_P = Mo_S0_DDR_P;

	if(t_idx >= (mid-1) && t_idx <= (mid+1) ) OV0_En = 0;
	else									  OV0_En = 1;
	switch(t_idx){
		case 0:
			Mo_T_DDR_P   = FX_WDR_MO_T_A_P0_ADDR >> 5;
			Df0_S0_DDR_P = FX_WDR_MO_T_A_P0_ADDR >> 5;
			Df0_S1_DDR_P = 0;
			Df0_T_DDR_P  = FX_WDR_MO_DIFF_A_P0_ADDR >> 5;

			Df1_T_DDR_P  = FX_WDR_DIF_A_P0_ADDR >> 5;
			Df2_S0_DDR_P = FX_WDR_DIF_A_P0_ADDR >> 5;
			Df2_S1_DDR_P = 0;
			Df2_T_DDR_P  = FX_WDR_DIF_2_A_P0_ADDR >> 5;

//			Df3_S0_DDR_P = FX_WDR_DIF_A_P0_ADDR >> 5;
//			Df3_S1_DDR_P = 0;
//			Df3_T_DDR_P  = FX_WDR_MO_DIFF_A_P0_ADDR >> 5;

			OV0_S1_DDR_P  = Get_OV0_S1_ADDR(t_idx, mid, OV0_En) >> 5;
			OV0_S0_DDR_P  = FX_WDR_MO_DIFF_A_P0_ADDR >> 5;
			OV0_T_DDR_P   = FX_WDR_MO_DIFF_A_P0_ADDR >> 5;
			break;
		case 1:
			Mo_T_DDR_P   = FX_WDR_MO_T_A_P1_ADDR >> 5;
			Df0_S0_DDR_P = FX_WDR_MO_T_A_P1_ADDR >> 5;
			Df0_S1_DDR_P = 0;
			Df0_T_DDR_P  = FX_WDR_MO_DIFF_A_P1_ADDR >> 5;

			Df1_T_DDR_P  = FX_WDR_DIF_A_P1_ADDR >> 5;
			Df2_S0_DDR_P = FX_WDR_DIF_A_P1_ADDR >> 5;
			Df2_S1_DDR_P = 0;
			Df2_T_DDR_P  = FX_WDR_DIF_2_A_P1_ADDR >> 5;

//			Df3_S0_DDR_P = FX_WDR_DIF_A_P1_ADDR >> 5;
//			Df3_S1_DDR_P = 0;
//			Df3_T_DDR_P  = FX_WDR_MO_DIFF_A_P1_ADDR >> 5;

			OV0_S1_DDR_P  = Get_OV0_S1_ADDR(t_idx, mid, OV0_En) >> 5;
			OV0_S0_DDR_P  = FX_WDR_MO_DIFF_A_P1_ADDR >> 5;
			OV0_T_DDR_P   = FX_WDR_MO_DIFF_A_P1_ADDR >> 5;
			break;
		case 2:
			Mo_T_DDR_P   = FX_WDR_MO_T_A_P2_ADDR >> 5;
			Df0_S0_DDR_P = FX_WDR_MO_T_A_P2_ADDR >> 5;
			Df0_S1_DDR_P = 0;
			Df0_T_DDR_P  = FX_WDR_MO_DIFF_A_P2_ADDR >> 5;

			Df1_T_DDR_P  = FX_WDR_DIF_A_P2_ADDR >> 5;
			Df2_S0_DDR_P = FX_WDR_DIF_A_P2_ADDR >> 5;
			Df2_S1_DDR_P = 0;
			Df2_T_DDR_P  = FX_WDR_DIF_2_A_P2_ADDR >> 5;

//			Df3_S0_DDR_P = FX_WDR_DIF_A_P2_ADDR >> 5;
//			Df3_S1_DDR_P = 0;
//			Df3_T_DDR_P  = FX_WDR_MO_DIFF_A_P2_ADDR >> 5;

			OV0_S1_DDR_P  = Get_OV0_S1_ADDR(t_idx, mid, OV0_En) >> 5;
			OV0_S0_DDR_P  = FX_WDR_MO_DIFF_A_P2_ADDR >> 5;
			OV0_T_DDR_P   = FX_WDR_MO_DIFF_A_P2_ADDR >> 5;
			break;
		case 3:
			Mo_T_DDR_P   = FX_WDR_MO_T_A_P3_ADDR >> 5;
			Df0_S0_DDR_P = FX_WDR_MO_T_A_P3_ADDR >> 5;
			Df0_S1_DDR_P = 0;
			Df0_T_DDR_P  = FX_WDR_MO_DIFF_A_P3_ADDR >> 5;

			Df1_T_DDR_P  = FX_WDR_DIF_A_P3_ADDR >> 5;
			Df2_S0_DDR_P = FX_WDR_DIF_A_P3_ADDR >> 5;
			Df2_S1_DDR_P = 0;
			Df2_T_DDR_P  = FX_WDR_DIF_2_A_P3_ADDR >> 5;

//			Df3_S0_DDR_P = FX_WDR_DIF_A_P3_ADDR >> 5;
//			Df3_S1_DDR_P = 0;
//			Df3_T_DDR_P  = FX_WDR_MO_DIFF_A_P3_ADDR >> 5;

			OV0_S1_DDR_P  = Get_OV0_S1_ADDR(t_idx, mid, OV0_En) >> 5;
			OV0_S0_DDR_P  = FX_WDR_MO_DIFF_A_P3_ADDR >> 5;
			OV0_T_DDR_P   = FX_WDR_MO_DIFF_A_P3_ADDR >> 5;
			break;
		case 4:
			Mo_T_DDR_P   = FX_WDR_MO_T_A_P4_ADDR >> 5;
			Df0_S0_DDR_P = FX_WDR_MO_T_A_P4_ADDR >> 5;
			Df0_S1_DDR_P = 0;
			Df0_T_DDR_P  = FX_WDR_MO_DIFF_A_P4_ADDR >> 5;

			Df1_T_DDR_P  = FX_WDR_DIF_A_P4_ADDR >> 5;
			Df2_S0_DDR_P = FX_WDR_DIF_A_P4_ADDR >> 5;
			Df2_S1_DDR_P = 0;
			Df2_T_DDR_P  = FX_WDR_DIF_2_A_P4_ADDR >> 5;

//			Df3_S0_DDR_P = FX_WDR_DIF_A_P4_ADDR >> 5;
//			Df3_S1_DDR_P = 0;
//			Df3_T_DDR_P  = FX_WDR_MO_DIFF_A_P4_ADDR >> 5;

			OV0_S1_DDR_P  = Get_OV0_S1_ADDR(t_idx, mid, OV0_En) >> 5;
			OV0_S0_DDR_P  = FX_WDR_MO_DIFF_A_P4_ADDR >> 5;
			OV0_T_DDR_P   = FX_WDR_MO_DIFF_A_P4_ADDR >> 5;
			break;
		case 5:
			Mo_T_DDR_P   = FX_WDR_MO_T_A_P5_ADDR >> 5;
			Df0_S0_DDR_P = FX_WDR_MO_T_A_P5_ADDR >> 5;
			Df0_S1_DDR_P = 0;
			Df0_T_DDR_P  = FX_WDR_MO_DIFF_A_P5_ADDR >> 5;

			Df1_T_DDR_P  = FX_WDR_DIF_A_P5_ADDR >> 5;
			Df2_S0_DDR_P = FX_WDR_DIF_A_P5_ADDR >> 5;
			Df2_S1_DDR_P = 0;
			Df2_T_DDR_P  = FX_WDR_DIF_2_A_P5_ADDR >> 5;

//			Df3_S0_DDR_P = FX_WDR_DIF_A_P5_ADDR >> 5;
//			Df3_S1_DDR_P = 0;
//			Df3_T_DDR_P  = FX_WDR_MO_DIFF_A_P5_ADDR >> 5;

			OV0_S1_DDR_P  = Get_OV0_S1_ADDR(t_idx, mid, OV0_En) >> 5;
			OV0_S0_DDR_P  = FX_WDR_MO_DIFF_A_P5_ADDR >> 5;
			OV0_T_DDR_P   = FX_WDR_MO_DIFF_A_P5_ADDR >> 5;
			break;
		case 6:
			Mo_T_DDR_P   = FX_WDR_MO_T_A_P6_ADDR >> 5;
			Df0_S0_DDR_P = FX_WDR_MO_T_A_P6_ADDR >> 5;
			Df0_S1_DDR_P = 0;
			Df0_T_DDR_P  = FX_WDR_MO_DIFF_A_P6_ADDR >> 5;

			Df1_T_DDR_P  = FX_WDR_DIF_A_P6_ADDR >> 5;
			Df2_S0_DDR_P = FX_WDR_DIF_A_P6_ADDR >> 5;
			Df2_S1_DDR_P = 0;
			Df2_T_DDR_P  = FX_WDR_DIF_2_A_P6_ADDR >> 5;

//			Df3_S0_DDR_P = FX_WDR_DIF_A_P6_ADDR >> 5;
//			Df3_S1_DDR_P = 0;
//			Df3_T_DDR_P  = FX_WDR_MO_DIFF_A_P6_ADDR >> 5;

			OV0_S1_DDR_P  = Get_OV0_S1_ADDR(t_idx, mid, OV0_En) >> 5;
			OV0_S0_DDR_P  = FX_WDR_MO_DIFF_A_P6_ADDR >> 5;
			OV0_T_DDR_P   = FX_WDR_MO_DIFF_A_P6_ADDR >> 5;
			break;
	}

	Mo_S0_Offset  = (768 >> 5); Mo_S1_Offset  = (768 >> 5); Mo_T_Offset  = (768 >> 5);
	Df0_S0_Offset = (768 >> 5); Df0_S1_Offset = 0; 	        Df0_T_Offset = (768 >> 5);
	Df1_S0_Offset = (768 >> 5); Df1_S1_Offset = 0; 	        Df1_T_Offset = (576 >> 5);
	Df2_S0_Offset = (576 >> 5); Df2_S1_Offset = 0; 	        Df2_T_Offset = (576 >> 5);
//	Df3_S0_Offset = (576 >> 5); Df3_S1_Offset = 0; 	        Df3_T_Offset = (768 >> 5);
	OV0_S0_Offset  = (768 >> 5); OV0_S1_Offset  = (768 >> 5); OV0_T_Offset  = (768 >> 5);

	memset(SP,0,512);
	memset(&DG_P[0],0,sizeof(DG_P));
//	SPP = (unsigned *) SP;
//	PP0 = (unsigned *) &DG_P[0];

	AS2_Diffusion_Address_Set(SP);

	//============ 0 =============
	Mo_Diff_Mode = 0;		//0 -> Diffision , 1 -> Motion , 3 -> Overlay
	Mo_TH	     = 255;
	Mo_Mul       = 255 * 4 / Mo_TH;
	Diff_pix     = 15;
	Diff_Divisor = (1024 * 16 / ((2 * Diff_pix + 1) * (2 * Diff_pix + 1)));

	DG_P[0].START_EN.SA_Start_En 	= Test_Enable[0];
	DG_P[0].START_EN.SB_Start_En 	= Test_Enable[0];
	DG_P[0].START_EN.SC_Start_En 	= FX & Test_Enable[0];
	DG_P[0].START_EN.Divisor 		= Diff_Divisor;
	DG_P[0].START_EN.I_TH_En 		= 0;
	DG_P[0].START_EN.I_TH 			= 0;
	DG_P[0].START_EN.rev 			= 0;

	DG_P[0].DW0.Offset_Bytes 		= 18;
	DG_P[0].DW0.diff_pix 			= Diff_pix;
	DG_P[0].DW0.bin_64_F 			= 0;
	DG_P[0].DW0.F_2nd 				= 0;
	DG_P[0].DW0.Mo_D_Mode 			= Mo_Diff_Mode;
	DG_P[0].DW0.Mo_TH 				= Mo_TH;
	DG_P[0].DW0.Mo_Mul_D 			= Mo_Mul;
	DG_P[0].DW0.rev 				= 0;

	DG_P[0].SIZE.Size_Y 			= 408;
	DG_P[0].SIZE.Size_X 			= 544;
	DG_P[0].SIZE.rev0 				= 0;
	DG_P[0].SIZE.rev1 				= 0;

	DG_P[0].S0_DDR_P                = Df1_S0_DDR_P;
	DG_P[0].S1_DDR_P                = Df1_S1_DDR_P;
	DG_P[0].T_DDR_P                 = Df1_T_DDR_P;
	DG_P[0].S0_DDR_Offset           = Df1_S0_Offset;
	DG_P[0].S1_DDR_Offset           = Df1_S1_Offset;
	DG_P[0].T_DDR_Offset            = Df1_T_Offset;
	DG_P[0].TAB_DDR_P               = Df0_TAB_DDR_P;

	SP->Start_En_0.Data = (DG_P[0].START_EN.I_TH << 24) | (DG_P[0].START_EN.Divisor << 4) | (DG_P[0].START_EN.I_TH_En << 3) | (DG_P[0].START_EN.SC_Start_En << 2) | (DG_P[0].START_EN.SB_Start_En << 1) | DG_P[0].START_EN.SA_Start_En;
	SP->CMD_DW0_0.Data = (DG_P[0].DW0.Mo_Mul_D << 24) | (DG_P[0].DW0.Mo_TH << 16) | (DG_P[0].DW0.Mo_D_Mode << 12) | (DG_P[0].DW0.F_2nd << 11) | (DG_P[0].DW0.bin_64_F << 10) | (DG_P[0].DW0.diff_pix << 5) | DG_P[0].DW0.Offset_Bytes;
	SP->SIZE_0.Data = (DG_P[0].SIZE.Size_X << 16) | DG_P[0].SIZE.Size_Y;
	SP->S0_DDR_P_0.Data      = DG_P[0].S0_DDR_P;
	SP->S1_DDR_P_0.Data      = DG_P[0].S1_DDR_P;
	SP->T_DDR_P_0.Data       = DG_P[0].T_DDR_P;
	SP->S0_DDR_Offset_0.Data = DG_P[0].S0_DDR_Offset;
	SP->S1_DDR_Offset_0.Data = DG_P[0].S1_DDR_Offset;
	SP->T_DDR_Offset_0.Data  = DG_P[0].T_DDR_Offset;
	SP->TAB_ADDR_P_0.Data    = DG_P[0].TAB_DDR_P;

	//============ 1 =============
	Mo_Diff_Mode = 0;		//0 -> Diffision , 1 -> Motion , 3 -> Overlay
	Mo_TH	     = 255;
	Mo_Mul       = 255 * 4 / Mo_TH;
	Diff_pix     = 15;
	Diff_Divisor = (1024 * 16 / ((2 * Diff_pix + 1) * (2 * Diff_pix + 1)));

	DG_P[1].START_EN.SA_Start_En 	= Test_Enable[1];
	DG_P[1].START_EN.SB_Start_En 	= Test_Enable[1];
	DG_P[1].START_EN.SC_Start_En 	= FX & Test_Enable[1];
	DG_P[1].START_EN.Divisor 		= Diff_Divisor;
	DG_P[1].START_EN.I_TH_En 		= 0;
	DG_P[1].START_EN.I_TH 			= 0;
	DG_P[1].START_EN.rev 			= 0;

	DG_P[1].DW0.Offset_Bytes 		= 0;
	DG_P[1].DW0.diff_pix 			= Diff_pix;
	DG_P[1].DW0.bin_64_F 			= 0;
	DG_P[1].DW0.F_2nd 				= 0;
	DG_P[1].DW0.Mo_D_Mode 			= Mo_Diff_Mode;
	DG_P[1].DW0.Mo_TH 				= Mo_TH;
	DG_P[1].DW0.Mo_Mul_D 			= Mo_Mul;
	DG_P[1].DW0.rev 				= 0;

	DG_P[1].SIZE.Size_Y 			= 408;
	DG_P[1].SIZE.Size_X 			= 544;
	DG_P[1].SIZE.rev0 				= 0;
	DG_P[1].SIZE.rev1 				= 0;

	DG_P[1].S0_DDR_P                = Df2_S0_DDR_P;
	DG_P[1].S1_DDR_P                = Df2_S1_DDR_P;
	DG_P[1].T_DDR_P                 = Df2_T_DDR_P;
	DG_P[1].S0_DDR_Offset           = Df2_S0_Offset;
	DG_P[1].S1_DDR_Offset           = Df2_S1_Offset;
	DG_P[1].T_DDR_Offset            = Df2_T_Offset;
	DG_P[1].TAB_DDR_P               = Df2_TAB_DDR_P;

	SP->Start_En_1.Data = (DG_P[1].START_EN.I_TH << 24) | (DG_P[1].START_EN.Divisor << 4) | (DG_P[1].START_EN.I_TH_En << 3) | (DG_P[1].START_EN.SC_Start_En << 2) | (DG_P[1].START_EN.SB_Start_En << 1) | DG_P[1].START_EN.SA_Start_En;
	SP->CMD_DW0_1.Data = (DG_P[1].DW0.Mo_Mul_D << 24) | (DG_P[1].DW0.Mo_TH << 16) | (DG_P[1].DW0.Mo_D_Mode << 12) | (DG_P[1].DW0.F_2nd << 11) | (DG_P[1].DW0.bin_64_F << 10) | (DG_P[1].DW0.diff_pix << 5) | DG_P[1].DW0.Offset_Bytes;
	SP->SIZE_1.Data = (DG_P[1].SIZE.Size_X << 16) | DG_P[1].SIZE.Size_Y;
	SP->S0_DDR_P_1.Data      = DG_P[1].S0_DDR_P;
	SP->S1_DDR_P_1.Data      = DG_P[1].S1_DDR_P;
	SP->T_DDR_P_1.Data       = DG_P[1].T_DDR_P;
	SP->S0_DDR_Offset_1.Data = DG_P[1].S0_DDR_Offset;
	SP->S1_DDR_Offset_1.Data = DG_P[1].S1_DDR_Offset;
	SP->T_DDR_Offset_1.Data  = DG_P[1].T_DDR_Offset;
	SP->TAB_ADDR_P_1.Data    = DG_P[1].TAB_DDR_P;

	//============ 2 =============
/*	Mo_Diff_Mode = 0;		//0 -> Diffision , 1 -> Motion , 3 -> Overlay
	Mo_TH	     = 255;
	Mo_Mul       = 255 * 4 / Mo_TH;
	Diff_pix     = 15;
	Diff_Divisor = (1024 * 16 / ((2 * Diff_pix + 1) * (2 * Diff_pix + 1)));

	DG_P[2].START_EN.SA_Start_En 	= Test_Enable[2];
	DG_P[2].START_EN.SB_Start_En 	= Test_Enable[2];
	DG_P[2].START_EN.SC_Start_En 	= FX & Test_Enable[2];
	DG_P[2].START_EN.Divisor 		= Diff_Divisor;
	DG_P[2].START_EN.I_TH_En 		= 0;
	DG_P[2].START_EN.I_TH 			= 0;
	DG_P[2].START_EN.rev 			= 0;

	DG_P[2].DW0.Offset_Bytes 		= 0;
	DG_P[2].DW0.diff_pix 			= Diff_pix;
	DG_P[2].DW0.bin_64_F 			= 0;
	DG_P[2].DW0.F_2nd 				= 0;
	DG_P[2].DW0.Mo_D_Mode 			= Mo_Diff_Mode;
	DG_P[2].DW0.Mo_TH 				= Mo_TH;
	DG_P[2].DW0.Mo_Mul_D 			= Mo_Mul;
	DG_P[2].DW0.rev 				= 0;

	DG_P[2].SIZE.Size_Y 			= 408;
	DG_P[2].SIZE.Size_X 			= 544;
	DG_P[2].SIZE.rev0 				= 0;
	DG_P[2].SIZE.rev1 				= 0;

	DG_P[2].S0_DDR_P                = Df3_S0_DDR_P;
	DG_P[2].S1_DDR_P                = Df3_S1_DDR_P;
	DG_P[2].T_DDR_P                 = Df3_T_DDR_P;
	DG_P[2].S0_DDR_Offset           = Df3_S0_Offset;
	DG_P[2].S1_DDR_Offset           = Df3_S1_Offset;
	DG_P[2].T_DDR_Offset            = Df3_T_Offset;
	DG_P[2].TAB_DDR_P               = Df0_TAB_DDR_P;

	SP->Start_En_2.Data = (DG_P[2].START_EN.I_TH << 24) | (DG_P[2].START_EN.Divisor << 4) | (DG_P[2].START_EN.I_TH_En << 3) | (DG_P[2].START_EN.SC_Start_En << 2) | (DG_P[2].START_EN.SB_Start_En << 1) | DG_P[2].START_EN.SA_Start_En;
	SP->CMD_DW0_2.Data = (DG_P[2].DW0.Mo_Mul_D << 24) | (DG_P[2].DW0.Mo_TH << 16) | (DG_P[2].DW0.Mo_D_Mode << 12) | (DG_P[2].DW0.F_2nd << 11) | (DG_P[2].DW0.bin_64_F << 10) | (DG_P[2].DW0.diff_pix << 5) | DG_P[2].DW0.Offset_Bytes;
	SP->SIZE_2.Data = (DG_P[2].SIZE.Size_X << 16) | DG_P[2].SIZE.Size_Y;
	SP->S0_DDR_P_2.Data      = DG_P[2].S0_DDR_P;
	SP->S1_DDR_P_2.Data      = DG_P[2].S1_DDR_P;
	SP->T_DDR_P_2.Data       = DG_P[2].T_DDR_P;
	SP->S0_DDR_Offset_2.Data = DG_P[2].S0_DDR_Offset;
	SP->S1_DDR_Offset_2.Data = DG_P[2].S1_DDR_Offset;
	SP->T_DDR_Offset_2.Data  = DG_P[2].T_DDR_Offset;
	SP->TAB_ADDR_P_2.Data    = DG_P[2].TAB_DDR_P;
*/
	//============ 3 ==============
	Mo_Diff_Mode = 1;		//0 -> Diffision , 1 -> Motion , 3 -> Overlay
	//Motion_Th:13, +-0.5:8, +-1.0:13, +-1.5:19, +-2.0:29, +-2.5:43
	Mo_TH	     = info.hdr_ev * 8 - 16 + Motion_Th;
	if(Mo_TH < 1) Mo_TH = 1;
db_debug("AS2_DeGhost_Set() hdr: m_idx=%d hdr_ev=%f Motion_Th=%d Mo_TH=%d\n", m_idx, info.hdr_ev, Motion_Th, Mo_TH);
	Mo_Mul       = 255 * 4 / Mo_TH;		//4:x1	max:255
	Diff_pix     = 15;
	Diff_Divisor = (1024 * 16 / ((2 * Diff_pix + 1) * (2 * Diff_pix + 1)));

	DG_P[3].START_EN.SA_Start_En 	= Test_Enable[3];
	DG_P[3].START_EN.SB_Start_En 	= Test_Enable[3];
	DG_P[3].START_EN.SC_Start_En 	= FX & Test_Enable[3];
	DG_P[3].START_EN.Divisor 		= Diff_Divisor;
	DG_P[3].START_EN.I_TH_En 		= 1;
	DG_P[3].START_EN.I_TH 			= 255;
	DG_P[3].START_EN.rev 			= 0;

	DG_P[3].DW0.Offset_Bytes 		= 0;
	DG_P[3].DW0.diff_pix 			= Diff_pix;
	DG_P[3].DW0.bin_64_F 			= 0;
	DG_P[3].DW0.F_2nd 				= 0;
	DG_P[3].DW0.Mo_D_Mode 			= Mo_Diff_Mode;
	DG_P[3].DW0.Mo_TH 				= Mo_TH;
	DG_P[3].DW0.Mo_Mul_D 			= Mo_Mul;
	DG_P[3].DW0.rev 				= 0;

	DG_P[3].SIZE.Size_Y 			= 408;
	DG_P[3].SIZE.Size_X 			= 544;
	DG_P[3].SIZE.rev0 				= 0;
	DG_P[3].SIZE.rev1 				= 0;

	DG_P[3].S0_DDR_P                = Mo_S0_DDR_P;
	DG_P[3].S1_DDR_P                = Mo_S1_DDR_P;
	DG_P[3].T_DDR_P                 = Mo_T_DDR_P;
	DG_P[3].S0_DDR_Offset           = Mo_S0_Offset;
	DG_P[3].S1_DDR_Offset           = Mo_S1_Offset;
	DG_P[3].T_DDR_Offset            = Mo_T_Offset;
	DG_P[3].TAB_DDR_P               = Df0_TAB_DDR_P;

	SP->Start_En_3.Data = (DG_P[3].START_EN.I_TH << 24) | (DG_P[3].START_EN.Divisor << 4) | (DG_P[3].START_EN.I_TH_En << 3) | (DG_P[3].START_EN.SC_Start_En << 2) | (DG_P[3].START_EN.SB_Start_En << 1) | DG_P[3].START_EN.SA_Start_En;
	SP->CMD_DW0_3.Data = (DG_P[3].DW0.Mo_Mul_D << 24) | (DG_P[3].DW0.Mo_TH << 16) | (DG_P[3].DW0.Mo_D_Mode << 12) | (DG_P[3].DW0.F_2nd << 11) | (DG_P[3].DW0.bin_64_F << 10) | (DG_P[3].DW0.diff_pix << 5) | DG_P[3].DW0.Offset_Bytes;
	SP->SIZE_3.Data = (DG_P[3].SIZE.Size_X << 16) | DG_P[3].SIZE.Size_Y;
	SP->S0_DDR_P_3.Data      = DG_P[3].S0_DDR_P;
	SP->S1_DDR_P_3.Data      = DG_P[3].S1_DDR_P;
	SP->T_DDR_P_3.Data       = DG_P[3].T_DDR_P;
	SP->S0_DDR_Offset_3.Data = DG_P[3].S0_DDR_Offset;
	SP->S1_DDR_Offset_3.Data = DG_P[3].S1_DDR_Offset;
	SP->T_DDR_Offset_3.Data  = DG_P[3].T_DDR_Offset;
	SP->TAB_ADDR_P_3.Data    = DG_P[3].TAB_DDR_P;

	//============ 4 =============
	Mo_Diff_Mode = 0;		//0 -> Diffision , 1 -> Motion , 3 -> Overlay
	Mo_TH	     = 255;
	Mo_Mul       = 255 * 4 / Mo_TH;
	Diff_pix     = Motion_Diff_Pix;
	Diff_Divisor = (1024 * 16 / ((2 * Diff_pix + 1) * (2 * Diff_pix + 1)));

	DG_P[4].START_EN.SA_Start_En 	= Test_Enable[4];
	DG_P[4].START_EN.SB_Start_En 	= Test_Enable[4];
	DG_P[4].START_EN.SC_Start_En 	= FX & Test_Enable[4];
	DG_P[4].START_EN.Divisor 		= Diff_Divisor;
	DG_P[4].START_EN.I_TH_En 		= 0;
	DG_P[4].START_EN.I_TH 			= 0;
	DG_P[4].START_EN.rev 			= 0;

	DG_P[4].DW0.Offset_Bytes 		= 2;
	DG_P[4].DW0.diff_pix 			= Diff_pix;
	DG_P[4].DW0.bin_64_F 			= 0;
	DG_P[4].DW0.F_2nd 				= 0;
	DG_P[4].DW0.Mo_D_Mode 			= Mo_Diff_Mode;
	DG_P[4].DW0.Mo_TH 				= Mo_TH;
	DG_P[4].DW0.Mo_Mul_D 			= Mo_Mul;
	DG_P[4].DW0.rev 				= 0;

	DG_P[4].SIZE.Size_Y 			= 408;
	DG_P[4].SIZE.Size_X 			= 544;
	DG_P[4].SIZE.rev0 				= 0;
	DG_P[4].SIZE.rev1 				= 0;

	DG_P[4].S0_DDR_P                = Df0_S0_DDR_P;
	DG_P[4].S1_DDR_P                = Df0_S1_DDR_P;
	DG_P[4].T_DDR_P                 = Df0_T_DDR_P;
	DG_P[4].S0_DDR_Offset           = Df0_S0_Offset;
	DG_P[4].S1_DDR_Offset           = Df0_S1_Offset;
	DG_P[4].T_DDR_Offset            = Df0_T_Offset;
	if(DeGhostEn == 1)
		DG_P[4].TAB_DDR_P               = Df0_TAB_DDR_P;
	else
		DG_P[4].TAB_DDR_P               = FX_WDR_TABLE_ADDR >> 5;

	SP->Start_En_4.Data = (DG_P[4].START_EN.I_TH << 24) | (DG_P[4].START_EN.Divisor << 4) | (DG_P[4].START_EN.I_TH_En << 3) | (DG_P[4].START_EN.SC_Start_En << 2) | (DG_P[4].START_EN.SB_Start_En << 1) | DG_P[4].START_EN.SA_Start_En;
	SP->CMD_DW0_4.Data = (DG_P[4].DW0.Mo_Mul_D << 24) | (DG_P[4].DW0.Mo_TH << 16) | (DG_P[4].DW0.Mo_D_Mode << 12) | (DG_P[4].DW0.F_2nd << 11) | (DG_P[4].DW0.bin_64_F << 10) | (DG_P[4].DW0.diff_pix << 5) | DG_P[4].DW0.Offset_Bytes;
	SP->SIZE_4.Data = (DG_P[4].SIZE.Size_X << 16) | DG_P[4].SIZE.Size_Y;
	SP->S0_DDR_P_4.Data      = DG_P[4].S0_DDR_P;
	SP->S1_DDR_P_4.Data      = DG_P[4].S1_DDR_P;
	SP->T_DDR_P_4.Data       = DG_P[4].T_DDR_P;
	SP->S0_DDR_Offset_4.Data = DG_P[4].S0_DDR_Offset;
	SP->S1_DDR_Offset_4.Data = DG_P[4].S1_DDR_Offset;
	SP->T_DDR_Offset_4.Data  = DG_P[4].T_DDR_Offset;
	SP->TAB_ADDR_P_4.Data    = DG_P[4].TAB_DDR_P;

	//============ 5 =============
	Mo_Diff_Mode = 3;		//0 -> Diffision , 1 -> Motion , 3 -> Overlay
	Mo_TH	     = 255;
	Mo_Mul       = 4;		//4:x1	max:255
	Diff_pix     = 15;
	Diff_Divisor = (64 * 1024 * 16 / ((2 * Diff_pix + 1) * (2 * Diff_pix + 1)));

	DG_P[5].START_EN.SA_Start_En 	= Test_Enable[5] & OV0_En;
	DG_P[5].START_EN.SB_Start_En 	= Test_Enable[5] & OV0_En;
	DG_P[5].START_EN.SC_Start_En 	= FX & Test_Enable[5] & OV0_En;
	DG_P[5].START_EN.Divisor 		= Diff_Divisor;
	DG_P[5].START_EN.I_TH_En 		= 0;
	DG_P[5].START_EN.I_TH 			= 0;
	DG_P[5].START_EN.rev 			= 0;

	DG_P[5].DW0.Offset_Bytes 		= 0;
	DG_P[5].DW0.diff_pix 			= Diff_pix;
	DG_P[5].DW0.bin_64_F 			= 0;
	DG_P[5].DW0.F_2nd 				= 0;
	DG_P[5].DW0.Mo_D_Mode 			= Mo_Diff_Mode;
	DG_P[5].DW0.Mo_TH 				= Mo_TH;
	DG_P[5].DW0.Mo_Mul_D 			= Mo_Mul;
	DG_P[5].DW0.rev 				= 0;

	DG_P[5].SIZE.Size_Y 			= 408;
	DG_P[5].SIZE.Size_X 			= 544;
	DG_P[5].SIZE.rev0 				= 0;
	DG_P[5].SIZE.rev1 				= 0;

	DG_P[5].S0_DDR_P                = OV0_S0_DDR_P;
	DG_P[5].S1_DDR_P                = OV0_S1_DDR_P;
	DG_P[5].T_DDR_P                 = OV0_T_DDR_P;
	DG_P[5].S0_DDR_Offset           = OV0_S0_Offset;
	DG_P[5].S1_DDR_Offset           = OV0_S1_Offset;
	DG_P[5].T_DDR_Offset            = OV0_T_Offset;
	DG_P[5].TAB_DDR_P               = Df0_TAB_DDR_P;
//db_debug("AS2_DeGhost_Set() hdr: t_idx=%d OV0_En=%d S0=0x%x S1=0x%x T=0x%x\n", t_idx, OV0_En, OV0_S0_DDR_P, OV0_S1_DDR_P, OV0_T_DDR_P);

	SP->Start_En_5.Data = (DG_P[5].START_EN.I_TH << 24) | (DG_P[5].START_EN.Divisor << 4) | (DG_P[5].START_EN.I_TH_En << 3) | (DG_P[5].START_EN.SC_Start_En << 2) | (DG_P[5].START_EN.SB_Start_En << 1) | DG_P[5].START_EN.SA_Start_En;
	SP->CMD_DW0_5.Data = (DG_P[5].DW0.Mo_Mul_D << 24) | (DG_P[5].DW0.Mo_TH << 16) | (DG_P[5].DW0.Mo_D_Mode << 12) | (DG_P[5].DW0.F_2nd << 11) | (DG_P[5].DW0.bin_64_F << 10) | (DG_P[5].DW0.diff_pix << 5) | DG_P[5].DW0.Offset_Bytes;
	SP->SIZE_5.Data = (DG_P[5].SIZE.Size_X << 16) | DG_P[5].SIZE.Size_Y;
	SP->S0_DDR_P_5.Data      = DG_P[5].S0_DDR_P;
	SP->S1_DDR_P_5.Data      = DG_P[5].S1_DDR_P;
	SP->T_DDR_P_5.Data       = DG_P[5].T_DDR_P;
	SP->S0_DDR_Offset_5.Data = DG_P[5].S0_DDR_Offset;
	SP->S1_DDR_Offset_5.Data = DG_P[5].S1_DDR_Offset;
	SP->T_DDR_Offset_5.Data  = DG_P[5].T_DDR_Offset;
	SP->TAB_ADDR_P_5.Data    = DG_P[5].TAB_DDR_P;

	//============ 5 =============
	/*Mo_Diff_Mode = 3;		//0 -> Diffision , 1 -> Motion , 3 -> Overlay
	Mo_TH	     = 255;
	Mo_Mul       = Overlay_Mul;		//4:x1	max:255
	Diff_pix     = 15;
	Diff_Divisor = (64 * 1024 * 16 / ((2 * Diff_pix + 1) * (2 * Diff_pix + 1)));

	DG_P[5].START_EN.SA_Start_En 	= Test_Enable[5];
	DG_P[5].START_EN.SB_Start_En 	= Test_Enable[5];
	DG_P[5].START_EN.SC_Start_En 	= FX & Test_Enable[5];
	DG_P[5].START_EN.Divisor 		= Diff_Divisor;
	DG_P[5].START_EN.I_TH_En 		= 0;
	DG_P[5].START_EN.I_TH 			= 0;
	DG_P[5].START_EN.rev 			= 0;

	DG_P[5].DW0.Offset_Bytes 		= 0;
	DG_P[5].DW0.diff_pix 			= Diff_pix;
	DG_P[5].DW0.bin_64_F 			= 0;
	DG_P[5].DW0.F_2nd 				= 0;
	DG_P[5].DW0.Mo_D_Mode 			= Mo_Diff_Mode;
	DG_P[5].DW0.Mo_TH 				= Mo_TH;
	DG_P[5].DW0.Mo_Mul_D 			= Mo_Mul;
	DG_P[5].DW0.rev 				= 0;

	DG_P[5].SIZE.Size_Y 			= 408;
	DG_P[5].SIZE.Size_X 			= 544;
	DG_P[5].SIZE.rev0 				= 0;
	DG_P[5].SIZE.rev1 				= 0;

	DG_P[5].S0_DDR_P                = OV1_S0_DDR_P;
	DG_P[5].S1_DDR_P                = OV1_S1_DDR_P;
	DG_P[5].T_DDR_P                 = OV1_T_DDR_P;
	DG_P[5].S0_DDR_Offset           = OV1_S0_Offset;
	DG_P[5].S1_DDR_Offset           = OV1_S1_Offset;
	DG_P[5].T_DDR_Offset            = OV1_T_Offset;
	DG_P[5].TAB_DDR_P               = Df0_TAB_DDR_P;

	SP->Start_En_5.Data = (DG_P[5].START_EN.I_TH << 24) | (DG_P[5].START_EN.Divisor << 4) | (DG_P[5].START_EN.I_TH_En << 3) | (DG_P[5].START_EN.SC_Start_En << 2) | (DG_P[5].START_EN.SB_Start_En << 1) | DG_P[5].START_EN.SA_Start_En;
	SP->CMD_DW0_5.Data = (DG_P[5].DW0.Mo_Mul_D << 24) | (DG_P[5].DW0.Mo_TH << 16) | (DG_P[5].DW0.Mo_D_Mode << 12) | (DG_P[5].DW0.F_2nd << 11) | (DG_P[5].DW0.bin_64_F << 10) | (DG_P[5].DW0.diff_pix << 5) | DG_P[5].DW0.Offset_Bytes;
	SP->SIZE_5.Data = (DG_P[5].SIZE.Size_X << 16) | DG_P[5].SIZE.Size_Y;
	SP->S0_DDR_P_5.Data      = DG_P[5].S0_DDR_P;
	SP->S1_DDR_P_5.Data      = DG_P[5].S1_DDR_P;
	SP->T_DDR_P_5.Data       = DG_P[5].T_DDR_P;
	SP->S0_DDR_Offset_5.Data = DG_P[5].S0_DDR_Offset;
	SP->S1_DDR_Offset_5.Data = DG_P[5].S1_DDR_Offset;
	SP->T_DDR_Offset_5.Data  = DG_P[5].T_DDR_Offset;
	SP->TAB_ADDR_P_5.Data    = DG_P[5].TAB_DDR_P;*/

	// Y: 0 --- 32--- 64--- 96---128---160---192---224---256
	//    | +3  | +2  | +1  | +0  | +0  | +1  | +2  | +3  |
	//Mo_Mul_0 = 255 * 4 / (Motion_Th);
	//Mo_Mul_1 = 255 * 4 / (Motion_Th + 1);
	//Mo_Mul_2 = 255 * 4 / (Motion_Th + 2);
	//Mo_Mul_3 = 255 * 4 / (Motion_Th + 3);
	//SP->Mo_Mul_D.Data = (Mo_Mul_3 << 24) | (Mo_Mul_2 << 16) | (Mo_Mul_1 << 8) | Mo_Mul_0;

	//Debug
	SP->REV[0].Data    = 0;		//Debug = 72;
}
int M_WDR_IMG_Addr[3];
/*
 *
 * hdr_7idx: 指定HDR來源DDR
 * wdr_idx : 指定WDR合成強度(曲線), 1->中/亮合成, 2->中/暗合成
 * wdr_mode: 0->ISP2 HDR, 多張合成
 *           1->ISP1 WDR, 單張
 */
void make_Diffusion_Cmd(int idx, int enable, int m_mode, int c_mode, int hdr_7idx, int wdr_idx, int wdr_mode, int deGhost_en,
                        AS2_F0_MAIN_CMD_struct *Cmd_p)
{
    int s_ddr_a, s_ddr_b, s_ddr_c, t_ddr_a, t_ddr_b, t_ddr_c, table_ddr;
    int Big_Mode = A2K.B_RUL_Mode;
    int bin_64x64=0, diff_offset=0;
    int bin_64x64_2=0;
    int f0_en_a=0, f0_en_b=0, f0_en_c=0, f1_en_a=0, f1_en_b=0, f1_en_c=0;
    int f0_en_a2=0, f0_en_b2=0, f0_en_c2=0, f1_en_a2=0, f1_en_b2=0, f1_en_c2=0;
    int s_ddr_a2, s_ddr_b2, s_ddr_c2, t_ddr_a2, t_ddr_b2, t_ddr_c2, table_ddr2, diff_offset2=0;
    
    if(enable == 1) {
        /*if(wdr_mode == 1) {        //ISP1.WDR, Sport_WDR
//                        Page
//                 ISP1.WB   0    1    0
//                 ISP1.WD   0    1    0
//                 Diff.S    1    0    1
//                 Diff.T    0    1    0
//
            if(hdr_7idx == 0) {
                s_ddr_a  = ISP1_WB_P1_A_ADDR;   s_ddr_b  = ISP1_WB_P1_B_ADDR;   s_ddr_c  = ISP1_WB_P1_C_ADDR;
                t_ddr_a  = ISP1_WD_P0_A_ADDR;   t_ddr_b  = ISP1_WD_P0_B_ADDR;   t_ddr_c  = ISP1_WD_P0_C_ADDR;
                s_ddr_a2 = t_ddr_a;             s_ddr_b2 = t_ddr_b;             s_ddr_c2 = t_ddr_c;
                t_ddr_a2 = ISP1_WD_2_P0_A_ADDR; t_ddr_b2 = ISP1_WD_2_P0_B_ADDR; t_ddr_c2 = ISP1_WD_2_P0_C_ADDR;
            }
            else {
                s_ddr_a  = ISP1_WB_P0_A_ADDR;   s_ddr_b  = ISP1_WB_P0_B_ADDR;   s_ddr_c  = ISP1_WB_P0_C_ADDR;
                t_ddr_a  = ISP1_WD_P1_A_ADDR;   t_ddr_b  = ISP1_WD_P1_B_ADDR;   t_ddr_c  = ISP1_WD_P1_C_ADDR;
                s_ddr_a2 = t_ddr_a;             s_ddr_b2 = t_ddr_b;             s_ddr_c2 = t_ddr_c;
                t_ddr_a2 = ISP1_WD_2_P1_A_ADDR; t_ddr_b2 = ISP1_WD_2_P1_B_ADDR; t_ddr_c2 = ISP1_WD_2_P1_C_ADDR;
            }
            table_ddr = FX_WDR_TABLE_ADDR + (3 * 256);
            bin_64x64 = 0;
            diff_offset = 2;
            f0_en_a = 1; f0_en_b = 1; f0_en_c = 0;
            f1_en_a = 1; f1_en_b = 1; f1_en_c = 1;

            table_ddr2 = FX_WDR_LIVE_TABLE_ADDR;
            bin_64x64_2 = 3;
            diff_offset2 = 1;
            f0_en_a2 = 1; f0_en_b2 = 1; f0_en_c2 = 0;
            f1_en_a2 = 1; f1_en_b2 = 1; f1_en_c2 = 1;
        }
        else if(wdr_mode == 2) {                                //ISP2.WDR
            db_debug("make_Diffusion_Cmd: enable=%d idx=%d page=%d\n", enable, wdr_idx, hdr_7idx); 
            if(wdr_idx < 0 || wdr_idx > 2){ wdr_idx = 0; }      // 不透明 
            hdr_7idx = (hdr_7idx + 1) % 3;                      // 暗-中-亮, 先取'中'運算
            
            switch(hdr_7idx){
            default:
            case 0: s_ddr_a = FX_WDR_IMG_A_P0_ADDR; s_ddr_b = FX_WDR_IMG_B_P0_ADDR; s_ddr_c = FX_WDR_IMG_C_P0_ADDR;
                    t_ddr_a = FX_WDR_DIF_A_P0_ADDR; t_ddr_b = FX_WDR_DIF_B_P0_ADDR; t_ddr_c = FX_WDR_DIF_C_P0_ADDR;
                    s_ddr_a2 = t_ddr_a;                s_ddr_b2 = t_ddr_b;                s_ddr_c2 = t_ddr_c;
                    t_ddr_a2 = FX_WDR_DIF_2_A_P0_ADDR; t_ddr_b2 = FX_WDR_DIF_2_B_P0_ADDR; t_ddr_c2 = FX_WDR_DIF_2_C_P0_ADDR;
                    break;
            case 1: s_ddr_a = FX_WDR_IMG_A_P1_ADDR; s_ddr_b = FX_WDR_IMG_B_P1_ADDR; s_ddr_c = FX_WDR_IMG_C_P1_ADDR;
                    t_ddr_a = FX_WDR_DIF_A_P1_ADDR; t_ddr_b = FX_WDR_DIF_B_P1_ADDR; t_ddr_c = FX_WDR_DIF_C_P1_ADDR;
                    s_ddr_a2 = t_ddr_a;                s_ddr_b2 = t_ddr_b;                s_ddr_c2 = t_ddr_c;
                    t_ddr_a2 = FX_WDR_DIF_2_A_P1_ADDR; t_ddr_b2 = FX_WDR_DIF_2_B_P1_ADDR; t_ddr_c2 = FX_WDR_DIF_2_C_P1_ADDR;
                    break;
            case 2: s_ddr_a = FX_WDR_IMG_A_P2_ADDR; s_ddr_b = FX_WDR_IMG_B_P2_ADDR; s_ddr_c = FX_WDR_IMG_C_P2_ADDR;
                    t_ddr_a = FX_WDR_DIF_A_P2_ADDR; t_ddr_b = FX_WDR_DIF_B_P2_ADDR; t_ddr_c = FX_WDR_DIF_C_P2_ADDR;
                    s_ddr_a2 = t_ddr_a;                s_ddr_b2 = t_ddr_b;                s_ddr_c2 = t_ddr_c;
                    t_ddr_a2 = FX_WDR_DIF_2_A_P2_ADDR; t_ddr_b2 = FX_WDR_DIF_2_B_P2_ADDR; t_ddr_c2 = FX_WDR_DIF_2_C_P2_ADDR;
                    break;
            }
            if(wdr_idx == 0){
                M_WDR_IMG_Addr[0] = s_ddr_a;
                M_WDR_IMG_Addr[1] = s_ddr_b;
                M_WDR_IMG_Addr[2] = s_ddr_c;
            }                
            else{
                s_ddr_a = M_WDR_IMG_Addr[0];
                s_ddr_b = M_WDR_IMG_Addr[1];
                s_ddr_c = M_WDR_IMG_Addr[2];
            }
            s_ddr_a += 0x10000; s_ddr_b += 0x10000; s_ddr_c += 0x10000;        // rex+ 180731, 0x10000(2條掃描線) fpga bug?
            //s_ddr_a2 += 0x10000; s_ddr_b2 += 0x10000; s_ddr_c2 += 0x10000;        // rex+ 180731, 0x10000(2條掃描線) fpga bug?
            table_ddr = FX_WDR_TABLE_ADDR + (3 * 256);
            bin_64x64 = 0;
            diff_offset = 18;
            f0_en_a = 1; f0_en_b = 1; f0_en_c = 0;
            f1_en_a = 1; f1_en_b = 1; f1_en_c = 1;

            table_ddr2 = FX_WDR_TABLE_ADDR + (wdr_idx * 256);
            bin_64x64_2 = 2;
            diff_offset2 = 0;
            f0_en_a2 = 1; f0_en_b2 = 1; f0_en_c2 = 0;
            f1_en_a2 = 1; f1_en_b2 = 1; f1_en_c2 = 1;
        }
        else*/ if(wdr_mode == 3 || wdr_mode == 5 || wdr_mode == 7){       // 3P、5P、7P
            // 3張HDR從1開始做，5張HDR從2開始做，7張HDR從3開始做
            int hdr_idx = ((hdr_7idx-PIPE_SUBCODE_AEB_STEP_0) + (wdr_mode>>1)) % wdr_mode;
            int hdr_mid = (wdr_mode>>1);
            int s_idx, t_idx, m_idx;;
            // wdr_mode=3, hdr_idx=1,2,0
            //         =5, hdr_idx=2,3,4,0,1
            //         =7, hdr_idx=3,4,5,6,0,1,2
            if(hdr_idx == hdr_mid){                     // 3->1 / 5->2 / 7->3 
                wdr_idx = 0;                            // 0->取透明為0(不透明)的轉換表
                s_idx = hdr_mid;
                t_idx = hdr_mid;
                m_idx = hdr_mid;
            }
            else if(hdr_idx > hdr_mid){                 // 3張->2 / 5張->3,4 / 7張->4,5,6
                wdr_idx = 1;                            // 1->中/亮合成
                s_idx = hdr_idx-1;                      // 3張->1 / 5張->2,3 / 7張->3,4,5
                t_idx = hdr_idx;                        // 3張->2 / 5張->3,4 / 7張->4,5,6
                m_idx = t_idx;
            }
            else if(hdr_idx < hdr_mid){                 // 3張->0 / 5張->0,1 / 7張->0,1,2
                wdr_idx = 2;                            // 2->中/暗合成
                s_idx = hdr_mid-hdr_idx;                // 3張->1 / 5張->2,1 / 7張->3,2,1
                t_idx = hdr_mid-(hdr_idx+1);            // 3張->0 / 5張->1,0 / 7張->2,1,0 (注意順序)
                m_idx = t_idx + 1;
            }
            JPEG_HDR_AEB_Info_Struct hdr_aeb_info;
            //if(c_mode == 5 || c_mode == 7 || c_mode == 13) Set_HDR_AEB_Info(&hdr_aeb_info, 0, hdr_7idx, c_mode);
            if(c_mode == 5 || c_mode == 7 || c_mode == 13) Set_HDR_AEB_Info(&hdr_aeb_info, 0, (m_idx-1+PIPE_SUBCODE_AEB_STEP_0), c_mode);
            db_debug("make_Diffusion_Cmd: hdr: idx=%d hdr_7idx=%d hdr_idx=%d s_idx=%d t_idx=%d m_idx=%d\n", idx, hdr_7idx, hdr_idx, s_idx, t_idx, m_idx);
            AS2_DeGhost_Set(&Cmd_p->F0_Diffusion_CMD, s_idx, t_idx, m_idx, wdr_idx, 0, deGhost_en, hdr_mid, hdr_aeb_info);
            AS2_DeGhost_Set(&Cmd_p->F1_Diffusion_CMD, s_idx, t_idx, m_idx, wdr_idx, 1, deGhost_en, hdr_mid, hdr_aeb_info);
            /*switch(s_idx){
            case 0: s_ddr_a  = FX_WDR_IMG_A_P0_ADDR;   s_ddr_b  = FX_WDR_IMG_B_P0_ADDR;   s_ddr_c  = FX_WDR_IMG_C_P0_ADDR; break;
            case 1: s_ddr_a  = FX_WDR_IMG_A_P1_ADDR;   s_ddr_b  = FX_WDR_IMG_B_P1_ADDR;   s_ddr_c  = FX_WDR_IMG_C_P1_ADDR; break;
            case 2: s_ddr_a  = FX_WDR_IMG_A_P2_ADDR;   s_ddr_b  = FX_WDR_IMG_B_P2_ADDR;   s_ddr_c  = FX_WDR_IMG_C_P2_ADDR; break;
            case 3: s_ddr_a  = FX_WDR_IMG_A_P3_ADDR;   s_ddr_b  = FX_WDR_IMG_B_P3_ADDR;   s_ddr_c  = FX_WDR_IMG_C_P3_ADDR; break;
            case 4: s_ddr_a  = FX_WDR_IMG_A_P4_ADDR;   s_ddr_b  = FX_WDR_IMG_B_P4_ADDR;   s_ddr_c  = FX_WDR_IMG_C_P4_ADDR; break;
            case 5: s_ddr_a  = FX_WDR_IMG_A_P5_ADDR;   s_ddr_b  = FX_WDR_IMG_B_P5_ADDR;   s_ddr_c  = FX_WDR_IMG_C_P5_ADDR; break;
            case 6: s_ddr_a  = FX_WDR_IMG_A_P6_ADDR;   s_ddr_b  = FX_WDR_IMG_B_P6_ADDR;   s_ddr_c  = FX_WDR_IMG_C_P6_ADDR; break;
            }
            switch(t_idx){                       // IMG_A(亮度表) 轉換 DIF_A(透明度表)
            case 0: t_ddr_a  = FX_WDR_DIF_A_P0_ADDR;   t_ddr_b  = FX_WDR_DIF_B_P0_ADDR;   t_ddr_c  = FX_WDR_DIF_C_P0_ADDR;
                    s_ddr_a2 = FX_WDR_DIF_A_P0_ADDR;   s_ddr_b2 = FX_WDR_DIF_B_P0_ADDR;   s_ddr_c2 = FX_WDR_DIF_C_P0_ADDR;
                    t_ddr_a2 = FX_WDR_DIF_2_A_P0_ADDR; t_ddr_b2 = FX_WDR_DIF_2_B_P0_ADDR; t_ddr_c2 = FX_WDR_DIF_2_C_P0_ADDR;
                    break;
            case 1: t_ddr_a  = FX_WDR_DIF_A_P1_ADDR;   t_ddr_b  = FX_WDR_DIF_B_P1_ADDR;   t_ddr_c  = FX_WDR_DIF_C_P1_ADDR;
                    s_ddr_a2 = FX_WDR_DIF_A_P1_ADDR;   s_ddr_b2 = FX_WDR_DIF_B_P1_ADDR;   s_ddr_c2 = FX_WDR_DIF_C_P1_ADDR;
                    t_ddr_a2 = FX_WDR_DIF_2_A_P1_ADDR; t_ddr_b2 = FX_WDR_DIF_2_B_P1_ADDR; t_ddr_c2 = FX_WDR_DIF_2_C_P1_ADDR;
                    break;
            case 2: t_ddr_a  = FX_WDR_DIF_A_P2_ADDR;   t_ddr_b  = FX_WDR_DIF_B_P2_ADDR;   t_ddr_c  = FX_WDR_DIF_C_P2_ADDR;
                    s_ddr_a2 = FX_WDR_DIF_A_P2_ADDR;   s_ddr_b2 = FX_WDR_DIF_B_P2_ADDR;   s_ddr_c2 = FX_WDR_DIF_C_P2_ADDR;
                    t_ddr_a2 = FX_WDR_DIF_2_A_P2_ADDR; t_ddr_b2 = FX_WDR_DIF_2_B_P2_ADDR; t_ddr_c2 = FX_WDR_DIF_2_C_P2_ADDR;
                    break;
            case 3: t_ddr_a  = FX_WDR_DIF_A_P3_ADDR;   t_ddr_b  = FX_WDR_DIF_B_P3_ADDR;   t_ddr_c  = FX_WDR_DIF_C_P3_ADDR;
                    s_ddr_a2 = FX_WDR_DIF_A_P3_ADDR;   s_ddr_b2 = FX_WDR_DIF_B_P3_ADDR;   s_ddr_c2 = FX_WDR_DIF_C_P3_ADDR;
                    t_ddr_a2 = FX_WDR_DIF_2_A_P3_ADDR; t_ddr_b2 = FX_WDR_DIF_2_B_P3_ADDR; t_ddr_c2 = FX_WDR_DIF_2_C_P3_ADDR;
                    break;
            case 4: t_ddr_a  = FX_WDR_DIF_A_P4_ADDR;   t_ddr_b  = FX_WDR_DIF_B_P4_ADDR;   t_ddr_c  = FX_WDR_DIF_C_P4_ADDR;
                    s_ddr_a2 = FX_WDR_DIF_A_P4_ADDR;   s_ddr_b2 = FX_WDR_DIF_B_P4_ADDR;   s_ddr_c2 = FX_WDR_DIF_C_P4_ADDR;
                    t_ddr_a2 = FX_WDR_DIF_2_A_P4_ADDR; t_ddr_b2 = FX_WDR_DIF_2_B_P4_ADDR; t_ddr_c2 = FX_WDR_DIF_2_C_P4_ADDR;
                    break;
            case 5: t_ddr_a  = FX_WDR_DIF_A_P5_ADDR;   t_ddr_b  = FX_WDR_DIF_B_P5_ADDR;   t_ddr_c  = FX_WDR_DIF_C_P5_ADDR;
                    s_ddr_a2 = FX_WDR_DIF_A_P5_ADDR;   s_ddr_b2 = FX_WDR_DIF_B_P5_ADDR;   s_ddr_c2 = FX_WDR_DIF_C_P5_ADDR;
                    t_ddr_a2 = FX_WDR_DIF_2_A_P5_ADDR; t_ddr_b2 = FX_WDR_DIF_2_B_P5_ADDR; t_ddr_c2 = FX_WDR_DIF_2_C_P5_ADDR;
                    break;
            case 6: t_ddr_a  = FX_WDR_DIF_A_P6_ADDR;   t_ddr_b  = FX_WDR_DIF_B_P6_ADDR;   t_ddr_c  = FX_WDR_DIF_C_P6_ADDR;
                    s_ddr_a2 = FX_WDR_DIF_A_P6_ADDR;   s_ddr_b2 = FX_WDR_DIF_B_P6_ADDR;   s_ddr_c2 = FX_WDR_DIF_C_P6_ADDR;
                    t_ddr_a2 = FX_WDR_DIF_2_A_P6_ADDR; t_ddr_b2 = FX_WDR_DIF_2_B_P6_ADDR; t_ddr_c2 = FX_WDR_DIF_2_C_P6_ADDR;
                    break;
            }
            s_ddr_a += 0x10000; s_ddr_b += 0x10000; s_ddr_c += 0x10000;        // rex+ 180731, 0x10000(2條掃描線) fpga bug?
            //s_ddr_a2 += 0x10000; s_ddr_b2 += 0x10000; s_ddr_c2 += 0x10000;        // rex+ 180731, 0x10000(2條掃描線) fpga bug?
            table_ddr = FX_WDR_TABLE_ADDR + (3 * 256);
            bin_64x64 = 0;
            diff_offset = 18;
            f0_en_a = 1; f0_en_b = 1; f0_en_c = 0;
            f1_en_a = 1; f1_en_b = 1; f1_en_c = 1;

            table_ddr2 = FX_WDR_TABLE_ADDR + (wdr_idx * 256);
            bin_64x64_2 = 2;
            diff_offset2 = 0;
            f0_en_a2 = 1; f0_en_b2 = 1; f0_en_c2 = 0;
            f1_en_a2 = 1; f1_en_b2 = 1; f1_en_c2 = 1;*/
        }
/*
        Cmd_p->F0_Diffusion_CMD.Start_En0.Data        = f0_en_a;
        Cmd_p->F0_Diffusion_CMD.S_DDR_P0.Data         = s_ddr_a >> 5;
        Cmd_p->F0_Diffusion_CMD.T_DDR_P0.Data         = t_ddr_a >> 5;
        Cmd_p->F0_Diffusion_CMD.Table_DDR_P0.Data     = table_ddr >> 5;
        Cmd_p->F0_Diffusion_CMD.Start_En1.Data        = f0_en_b;
        Cmd_p->F0_Diffusion_CMD.S_DDR_P1.Data         = s_ddr_b >> 5;
        Cmd_p->F0_Diffusion_CMD.T_DDR_P1.Data         = t_ddr_b >> 5;
        Cmd_p->F0_Diffusion_CMD.Table_DDR_P1.Data     = table_ddr >> 5;
        Cmd_p->F0_Diffusion_CMD.Start_En2.Data        = f0_en_c;
        Cmd_p->F0_Diffusion_CMD.S_DDR_P2.Data         = s_ddr_c >> 5;
        Cmd_p->F0_Diffusion_CMD.T_DDR_P2.Data         = t_ddr_c >> 5;
        Cmd_p->F0_Diffusion_CMD.Table_DDR_P2.Data     = table_ddr >> 5;
        Cmd_p->F0_Diffusion_CMD.Start_En3.Data        = f0_en_a2;
        Cmd_p->F0_Diffusion_CMD.S_DDR_P3.Data         = s_ddr_a2 >> 5;
        Cmd_p->F0_Diffusion_CMD.T_DDR_P3.Data         = t_ddr_a2 >> 5;
        Cmd_p->F0_Diffusion_CMD.Table_DDR_P3.Data     = table_ddr2 >> 5;
        Cmd_p->F0_Diffusion_CMD.Start_En4.Data        = f0_en_b2;
        Cmd_p->F0_Diffusion_CMD.S_DDR_P4.Data         = s_ddr_b2 >> 5;
        Cmd_p->F0_Diffusion_CMD.T_DDR_P4.Data         = t_ddr_b2 >> 5;
        Cmd_p->F0_Diffusion_CMD.Table_DDR_P4.Data     = table_ddr2 >> 5;
        Cmd_p->F0_Diffusion_CMD.Start_En5.Data        = f0_en_c2;
        Cmd_p->F0_Diffusion_CMD.S_DDR_P5.Data         = s_ddr_c2 >> 5;
        Cmd_p->F0_Diffusion_CMD.T_DDR_P5.Data         = t_ddr_c2 >> 5;
        Cmd_p->F0_Diffusion_CMD.Table_DDR_P5.Data     = table_ddr2 >> 5;
        Cmd_p->F1_Diffusion_CMD.Start_En0.Data        = f1_en_a;
        Cmd_p->F1_Diffusion_CMD.S_DDR_P0.Data         = s_ddr_a >> 5;
        Cmd_p->F1_Diffusion_CMD.T_DDR_P0.Data         = t_ddr_a >> 5;
        Cmd_p->F1_Diffusion_CMD.Table_DDR_P0.Data     = table_ddr >> 5;
        Cmd_p->F1_Diffusion_CMD.Start_En1.Data        = f1_en_b;
        Cmd_p->F1_Diffusion_CMD.S_DDR_P1.Data         = s_ddr_b >> 5;
        Cmd_p->F1_Diffusion_CMD.T_DDR_P1.Data         = t_ddr_b >> 5;
        Cmd_p->F1_Diffusion_CMD.Table_DDR_P1.Data     = table_ddr >> 5;
        Cmd_p->F1_Diffusion_CMD.Start_En2.Data        = f1_en_c;
        Cmd_p->F1_Diffusion_CMD.S_DDR_P2.Data         = s_ddr_c >> 5;
        Cmd_p->F1_Diffusion_CMD.T_DDR_P2.Data         = t_ddr_c >> 5;
        Cmd_p->F1_Diffusion_CMD.Table_DDR_P2.Data     = table_ddr >> 5;
        Cmd_p->F1_Diffusion_CMD.Start_En3.Data        = f1_en_a2;
        Cmd_p->F1_Diffusion_CMD.S_DDR_P3.Data         = s_ddr_a2 >> 5;
        Cmd_p->F1_Diffusion_CMD.T_DDR_P3.Data         = t_ddr_a2 >> 5;
        Cmd_p->F1_Diffusion_CMD.Table_DDR_P3.Data     = table_ddr2 >> 5;
        Cmd_p->F1_Diffusion_CMD.Start_En4.Data        = f1_en_b2;
        Cmd_p->F1_Diffusion_CMD.S_DDR_P4.Data         = s_ddr_b2 >> 5;
        Cmd_p->F1_Diffusion_CMD.T_DDR_P4.Data         = t_ddr_b2 >> 5;
        Cmd_p->F1_Diffusion_CMD.Table_DDR_P4.Data     = table_ddr2 >> 5;
        Cmd_p->F1_Diffusion_CMD.Start_En5.Data        = f1_en_c2;
        Cmd_p->F1_Diffusion_CMD.S_DDR_P5.Data         = s_ddr_c2 >> 5;
        Cmd_p->F1_Diffusion_CMD.T_DDR_P5.Data         = t_ddr_c2 >> 5;
        Cmd_p->F1_Diffusion_CMD.Table_DDR_P5.Data     = table_ddr2 >> 5;

        int wdr_pix = A2K.WDR_Pixel;
        int diff_Pix;    //getWDRTablePixel(m_mode, rec_mode);          // 擴散範圍
        int times;    // = (1024*16/((2*diff_Pix + 1)*(2*diff_Pix + 1)));
        if(wdr_mode == 1) diff_Pix = wdr_pix;								//ISP1.WDR
        else              diff_Pix = getWDRTablePixel(m_mode, c_mode);		//ISP2.WDR
        //times = (64*1024*16/((2*diff_Pix + 1)*(2*diff_Pix + 1)));
        times = (1024*16/((2*diff_Pix + 1)*(2*diff_Pix + 1)));				//FPGA: US362_7A50T_F8B.04.59_2E835_Jay

        Cmd_p->F0_Diffusion_CMD.Diff_pix0.Data        = diff_Pix;       // F0
        Cmd_p->F0_Diffusion_CMD.Divisor0.Data         = times;
        Cmd_p->F0_Diffusion_CMD.Diff_pix1.Data        = diff_Pix;
        Cmd_p->F0_Diffusion_CMD.Divisor1.Data         = times;
        Cmd_p->F0_Diffusion_CMD.Diff_pix2.Data        = diff_Pix;
        Cmd_p->F0_Diffusion_CMD.Divisor2.Data         = times;
        Cmd_p->F0_Diffusion_CMD.Diff_pix3.Data        = diff_Pix;
        Cmd_p->F0_Diffusion_CMD.Divisor3.Data         = times;
        Cmd_p->F0_Diffusion_CMD.Diff_pix4.Data        = diff_Pix;
        Cmd_p->F0_Diffusion_CMD.Divisor4.Data         = times;
        Cmd_p->F0_Diffusion_CMD.Diff_pix5.Data        = diff_Pix;
        Cmd_p->F0_Diffusion_CMD.Divisor5.Data         = times;
        Cmd_p->F1_Diffusion_CMD.Diff_pix0.Data        = diff_Pix;       // F1
        Cmd_p->F1_Diffusion_CMD.Divisor0.Data         = times;
        Cmd_p->F1_Diffusion_CMD.Diff_pix1.Data        = diff_Pix;
        Cmd_p->F1_Diffusion_CMD.Divisor1.Data         = times;
        Cmd_p->F1_Diffusion_CMD.Diff_pix2.Data        = diff_Pix;
        Cmd_p->F1_Diffusion_CMD.Divisor2.Data         = times;
        Cmd_p->F1_Diffusion_CMD.Diff_pix3.Data        = diff_Pix;
        Cmd_p->F1_Diffusion_CMD.Divisor3.Data         = times;
        Cmd_p->F1_Diffusion_CMD.Diff_pix4.Data        = diff_Pix;
        Cmd_p->F1_Diffusion_CMD.Divisor4.Data         = times;
        Cmd_p->F1_Diffusion_CMD.Diff_pix5.Data        = diff_Pix;
        Cmd_p->F1_Diffusion_CMD.Divisor5.Data         = times;

        int sx = 4352, sy = 3272;
        int sizeX = 544, sizeY = 409;
        int binn = A_L_I3_Header[Big_Mode].S_Binn;
        sx = 4352 / binn;
        sy = 3272 / binn;
        sizeX = sx >> 3;        //Ex: 4352 / 8 = 544
        Cmd_p->F0_Diffusion_CMD.Size_X0.Data          = sizeX;
        Cmd_p->F0_Diffusion_CMD.Size_X1.Data          = sizeX;
        Cmd_p->F0_Diffusion_CMD.Size_X2.Data          = sizeX;
        Cmd_p->F0_Diffusion_CMD.Size_X3.Data          = sizeX;
        Cmd_p->F0_Diffusion_CMD.Size_X4.Data          = sizeX;
        Cmd_p->F0_Diffusion_CMD.Size_X5.Data          = sizeX;
        Cmd_p->F1_Diffusion_CMD.Size_X0.Data          = sizeX;
        Cmd_p->F1_Diffusion_CMD.Size_X1.Data          = sizeX;
        Cmd_p->F1_Diffusion_CMD.Size_X2.Data          = sizeX;
        Cmd_p->F1_Diffusion_CMD.Size_X3.Data          = sizeX;
        Cmd_p->F1_Diffusion_CMD.Size_X4.Data          = sizeX;
        Cmd_p->F1_Diffusion_CMD.Size_X5.Data          = sizeX;

        sizeY = (sy >> 3) - 1;        //Ex: 3272 / 8 = 409
        Cmd_p->F0_Diffusion_CMD.Size_Y0.Data          = sizeY;
        Cmd_p->F0_Diffusion_CMD.Size_Y1.Data          = sizeY;
        Cmd_p->F0_Diffusion_CMD.Size_Y2.Data          = sizeY;
        Cmd_p->F0_Diffusion_CMD.Size_Y3.Data          = sizeY;
        Cmd_p->F0_Diffusion_CMD.Size_Y4.Data          = sizeY;
        Cmd_p->F0_Diffusion_CMD.Size_Y5.Data          = sizeY;
        Cmd_p->F1_Diffusion_CMD.Size_Y0.Data          = sizeY;
        Cmd_p->F1_Diffusion_CMD.Size_Y1.Data          = sizeY;
        Cmd_p->F1_Diffusion_CMD.Size_Y2.Data          = sizeY;
        Cmd_p->F1_Diffusion_CMD.Size_Y3.Data          = sizeY;
        Cmd_p->F1_Diffusion_CMD.Size_Y4.Data          = sizeY;
        Cmd_p->F1_Diffusion_CMD.Size_Y5.Data          = sizeY;

        Cmd_p->F0_Diffusion_CMD.Bin64x64_Flag_0.Data  = bin_64x64;
        Cmd_p->F0_Diffusion_CMD.Bin64x64_Flag_1.Data  = bin_64x64;
        Cmd_p->F0_Diffusion_CMD.Bin64x64_Flag_2.Data  = bin_64x64;
        Cmd_p->F0_Diffusion_CMD.Bin64x64_Flag_3.Data  = bin_64x64_2;
        Cmd_p->F0_Diffusion_CMD.Bin64x64_Flag_4.Data  = bin_64x64_2;
        Cmd_p->F0_Diffusion_CMD.Bin64x64_Flag_5.Data  = bin_64x64_2;
        Cmd_p->F1_Diffusion_CMD.Bin64x64_Flag_0.Data  = bin_64x64;
        Cmd_p->F1_Diffusion_CMD.Bin64x64_Flag_1.Data  = bin_64x64;
        Cmd_p->F1_Diffusion_CMD.Bin64x64_Flag_2.Data  = bin_64x64;
        Cmd_p->F1_Diffusion_CMD.Bin64x64_Flag_3.Data  = bin_64x64_2;
        Cmd_p->F1_Diffusion_CMD.Bin64x64_Flag_4.Data  = bin_64x64_2;
        Cmd_p->F1_Diffusion_CMD.Bin64x64_Flag_5.Data  = bin_64x64_2;

        Cmd_p->F0_Diffusion_CMD.Offset_Diff_P0.Data   = diff_offset;
        Cmd_p->F0_Diffusion_CMD.Offset_Diff_P1.Data   = diff_offset;
        Cmd_p->F0_Diffusion_CMD.Offset_Diff_P2.Data   = diff_offset;
        Cmd_p->F0_Diffusion_CMD.Offset_Diff_P3.Data   = diff_offset2;
        Cmd_p->F0_Diffusion_CMD.Offset_Diff_P4.Data   = diff_offset2;
        Cmd_p->F0_Diffusion_CMD.Offset_Diff_P5.Data   = diff_offset2;
        Cmd_p->F1_Diffusion_CMD.Offset_Diff_P0.Data   = diff_offset;
        Cmd_p->F1_Diffusion_CMD.Offset_Diff_P1.Data   = diff_offset;
        Cmd_p->F1_Diffusion_CMD.Offset_Diff_P2.Data   = diff_offset;
        Cmd_p->F1_Diffusion_CMD.Offset_Diff_P3.Data   = diff_offset2;
        Cmd_p->F1_Diffusion_CMD.Offset_Diff_P4.Data   = diff_offset2;
        Cmd_p->F1_Diffusion_CMD.Offset_Diff_P5.Data   = diff_offset2;

//        if(bin_64x64 == 0)
//            db_debug("make_Diffusion_Cmd() enable=%d sizeX=%d sizeY=%d mask=%d  s_ddr_a=0x%x t_ddr_a=0x%x  bin_64x64=%d wdr_mode=%d wdr_page=%d\n",
//                enable, sizeX, sizeY, mask, s_ddr_a, t_ddr_a, bin_64x64, wdr_mode, wdr_page);
//        else
//            db_error("make_Diffusion_Cmd() enable=%d sizeX=%d sizeY=%d mask=%d  s_ddr_a=0x%x t_ddr_a=0x%x  bin_64x64=%d wdr_mode=%d wdr_page=%d\n",
//                enable, sizeX, sizeY, mask, s_ddr_a, t_ddr_a, bin_64x64, wdr_mode, wdr_page);*/
    }
    else{
        /*Cmd_p->F0_Diffusion_CMD.Start_En0.Data        = 0x0;
        Cmd_p->F0_Diffusion_CMD.Start_En1.Data        = 0x0;
        Cmd_p->F0_Diffusion_CMD.Start_En2.Data        = 0x0;
        Cmd_p->F0_Diffusion_CMD.Start_En3.Data        = 0x0;
        Cmd_p->F0_Diffusion_CMD.Start_En4.Data        = 0x0;
        Cmd_p->F0_Diffusion_CMD.Start_En5.Data        = 0x0;
        Cmd_p->F1_Diffusion_CMD.Start_En0.Data        = 0x0;
        Cmd_p->F1_Diffusion_CMD.Start_En1.Data        = 0x0;
        Cmd_p->F1_Diffusion_CMD.Start_En2.Data        = 0x0;
        Cmd_p->F1_Diffusion_CMD.Start_En3.Data        = 0x0;
        Cmd_p->F1_Diffusion_CMD.Start_En4.Data        = 0x0;
        Cmd_p->F1_Diffusion_CMD.Start_En5.Data        = 0x0;

        Cmd_p->F0_Diffusion_CMD.Bin64x64_Flag_0.Data  = 0;
        Cmd_p->F0_Diffusion_CMD.Bin64x64_Flag_1.Data  = 0;
        Cmd_p->F0_Diffusion_CMD.Bin64x64_Flag_2.Data  = 0;
        Cmd_p->F0_Diffusion_CMD.Bin64x64_Flag_3.Data  = 0;
        Cmd_p->F0_Diffusion_CMD.Bin64x64_Flag_4.Data  = 0;
        Cmd_p->F0_Diffusion_CMD.Bin64x64_Flag_5.Data  = 0;
        Cmd_p->F1_Diffusion_CMD.Bin64x64_Flag_0.Data  = 0;
        Cmd_p->F1_Diffusion_CMD.Bin64x64_Flag_1.Data  = 0;
        Cmd_p->F1_Diffusion_CMD.Bin64x64_Flag_2.Data  = 0;
        Cmd_p->F1_Diffusion_CMD.Bin64x64_Flag_3.Data  = 0;
        Cmd_p->F1_Diffusion_CMD.Bin64x64_Flag_4.Data  = 0;
        Cmd_p->F1_Diffusion_CMD.Bin64x64_Flag_5.Data  = 0;*/
    }
}
void get_removal_binn_addr(int removal_st, int *addr_a, int *addr_b, int *addr_c)
{
    switch(removal_st){
    default:db_error("get_removal_binn_addr: err! st=%d\n", removal_st);
    case 0: *addr_a = ISP2_6X6_BIN_ADDR_A_P0;
            *addr_b = ISP2_6X6_BIN_ADDR_B_P0;
            *addr_c = ISP2_6X6_BIN_ADDR_C_P0;
            break;
    case 1: *addr_a = ISP2_6X6_BIN_ADDR_A_P1;
            *addr_b = ISP2_6X6_BIN_ADDR_B_P1;
            *addr_c = ISP2_6X6_BIN_ADDR_C_P1;
            break;
    case 2: *addr_a = ISP2_6X6_BIN_ADDR_A_P2;
            *addr_b = ISP2_6X6_BIN_ADDR_B_P2;
            *addr_c = ISP2_6X6_BIN_ADDR_C_P2;
            break;
    case 3: *addr_a = ISP2_6X6_BIN_ADDR_A_P3;
            *addr_b = ISP2_6X6_BIN_ADDR_B_P3;
            *addr_c = ISP2_6X6_BIN_ADDR_C_P3;
            break;
    }
}
void get_removal_now_addr(int i_page, int *now_a, int *now_b, int *now_c)
{
    switch(i_page){
        case 0: *now_a = ISP2_REMOVAL_P0_A_BUF_ADDR;                // ISP2 P1 buffer, (0x16A00000 + 128 * 2) = 379584768
                *now_b = ISP2_REMOVAL_P0_B_BUF_ADDR;
                *now_c = ISP2_REMOVAL_P0_C_BUF_ADDR;
                break;
        case 1: *now_a = ISP2_REMOVAL_P1_A_BUF_ADDR;                // ISP2 P1 buffer, (0x16A00000 + 128 * 2) + 3 * 4608 = 379598592
                *now_b = ISP2_REMOVAL_P1_B_BUF_ADDR;
                *now_c = ISP2_REMOVAL_P1_C_BUF_ADDR;
                break;
        case 2: *now_a = NR3D_STM1_A_BUF_ADDR;                      // (0x6A03600 + 64) = 111162944
                *now_b = NR3D_STM1_B_BUF_ADDR;                      // 111167624
                *now_c = NR3D_STM1_C_BUF_ADDR;
                break;
        case 3: *now_a = ISP2_STM1_S_P2_A_BUF_ADDR;                 // ISP1 P2 buffer, 0x6A00000 = 111149056
                *now_b = ISP2_STM1_S_P2_B_BUF_ADDR;
                *now_c = ISP2_STM1_S_P2_C_BUF_ADDR;
                break;
        default:*now_a = ISP2_STM1_S_P1_A_BUF_ADDR;                 // ISP1 P1 buffer
                *now_b = ISP2_STM1_S_P1_B_BUF_ADDR;
                *now_c = ISP2_STM1_S_P1_C_BUF_ADDR;
                break;
    }
}
void get_isp2_now_addr(int i_page, int hdr_fcnt, int *now_a, int *now_b, int *now_c)
{
    if(i_page < 0 || i_page > 6){
        db_error("get_isp2_now_addr: err! i_page=%d\n", i_page);
    }
    switch(i_page){
        default:
        case 0: *now_a = ISP2_STM1_S_P0_A_BUF_ADDR;             // ISP1 P0
                *now_b = ISP2_STM1_S_P0_B_BUF_ADDR;
                *now_c = ISP2_STM1_S_P0_C_BUF_ADDR;
                break;
        case 1: *now_a = ISP2_STM1_S_P1_A_BUF_ADDR;             // ISP1 P1
                *now_b = ISP2_STM1_S_P1_B_BUF_ADDR;
                *now_c = ISP2_STM1_S_P1_C_BUF_ADDR;
                break;
        case 2: *now_a = ISP2_STM1_S_P2_A_BUF_ADDR;             // ISP1 P2
                *now_b = ISP2_STM1_S_P2_B_BUF_ADDR;
                *now_c = ISP2_STM1_S_P2_C_BUF_ADDR;
                break;
        case 3: if(hdr_fcnt == 7){
                    *now_a = ISP1_STM1_T_P3_A_BUF_ADDR;             // ISP2 P1 buffer, AEB需要先處理
                    *now_b = ISP1_STM1_T_P3_B_BUF_ADDR;
                    *now_c = ISP1_STM1_T_P3_C_BUF_ADDR;
                }
                else{
                    *now_a = ISP1_STM1_T_P5_A_BUF_ADDR;             // ISP2 P0-2 buffer
                    *now_b = ISP1_STM1_T_P5_B_BUF_ADDR;
                    *now_c = ISP1_STM1_T_P5_C_BUF_ADDR;
                }
                break;
        case 4: *now_a = ISP1_STM1_T_P4_A_BUF_ADDR;             // ISP2 P0-1 buffer
                *now_b = ISP1_STM1_T_P4_B_BUF_ADDR;
                *now_c = ISP1_STM1_T_P4_C_BUF_ADDR;
                break;
        case 5: *now_a = ISP1_STM1_T_P5_A_BUF_ADDR;             // ISP2 P0-2 buffer
                *now_b = ISP1_STM1_T_P5_B_BUF_ADDR;
                *now_c = ISP1_STM1_T_P5_C_BUF_ADDR;
                break;
        case 6: *now_a = ISP1_STM1_T_P6_A_BUF_ADDR;             // NR3D
                *now_b = ISP1_STM1_T_P6_B_BUF_ADDR;
                *now_c = ISP1_STM1_T_P6_C_BUF_ADDR;
                break;
    }
}
void get_wdr_dif_addr(int i_page, int *addr_a, int *addr_b, int *addr_c)
{
    if(i_page < 0 || i_page > 6){
        db_error("get_wdr_dif_addr: err! i_page=%d\n", i_page);
    }
    switch(i_page){
        default:
        case 0: *addr_a = FX_WDR_DIF_2_A_P0_ADDR;             // 0,6 不使用
                *addr_b = FX_WDR_DIF_2_B_P0_ADDR;
                *addr_c = FX_WDR_DIF_2_C_P0_ADDR;
                break;
        case 1: *addr_a = FX_WDR_DIF_2_A_P1_ADDR;
                *addr_b = FX_WDR_DIF_2_B_P1_ADDR;
                *addr_c = FX_WDR_DIF_2_C_P1_ADDR;
                break;
        case 2: *addr_a = FX_WDR_DIF_2_A_P2_ADDR;
                *addr_b = FX_WDR_DIF_2_B_P2_ADDR;
                *addr_c = FX_WDR_DIF_2_C_P2_ADDR;
                break;
        case 3: *addr_a = FX_WDR_DIF_2_A_P3_ADDR;
                *addr_b = FX_WDR_DIF_2_B_P3_ADDR;
                *addr_c = FX_WDR_DIF_2_C_P3_ADDR;
                break;
        case 4: *addr_a = FX_WDR_DIF_2_A_P4_ADDR;
                *addr_b = FX_WDR_DIF_2_B_P4_ADDR;
                *addr_c = FX_WDR_DIF_2_C_P4_ADDR;
                break;
        case 5: *addr_a = FX_WDR_DIF_2_A_P5_ADDR;
                *addr_b = FX_WDR_DIF_2_B_P5_ADDR;
                *addr_c = FX_WDR_DIF_2_C_P5_ADDR;
                break;
        case 6: *addr_a = FX_WDR_DIF_2_A_P6_ADDR;             // 0,6 不使用
                *addr_b = FX_WDR_DIF_2_B_P6_ADDR;
                *addr_c = FX_WDR_DIF_2_C_P6_ADDR;
                break;
    }
db_debug("get_wdr_dif_addr() HDR: i_page=%d a=0x%x b=0x%x c=0x%x\n", i_page, *addr_a, *addr_b, *addr_c);
}
void get_isp2_nd_addr(int i_page, int *addr_a, int *addr_b, int *addr_c)
{
    if(i_page < 0 || i_page > 6){
        db_error("get_isp2_nd_addr: err! i_page=%d\n", i_page);
    }
    switch(i_page){
        default:
        case 0: *addr_a = FX_WDR_DIF_A_P0_ADDR;             // 0,6 不使用
                *addr_b = FX_WDR_DIF_B_P0_ADDR;
                *addr_c = FX_WDR_DIF_C_P0_ADDR;
                break;
        case 1: *addr_a = FX_WDR_DIF_A_P1_ADDR;
                *addr_b = FX_WDR_DIF_B_P1_ADDR;
                *addr_c = FX_WDR_DIF_C_P1_ADDR;
                break;
        case 2: *addr_a = FX_WDR_DIF_A_P2_ADDR;
                *addr_b = FX_WDR_DIF_B_P2_ADDR;
                *addr_c = FX_WDR_DIF_C_P2_ADDR;
                break;
        case 3: *addr_a = FX_WDR_DIF_A_P3_ADDR;
                *addr_b = FX_WDR_DIF_B_P3_ADDR;
                *addr_c = FX_WDR_DIF_C_P3_ADDR;
                break;
        case 4: *addr_a = FX_WDR_DIF_A_P4_ADDR;
                *addr_b = FX_WDR_DIF_B_P4_ADDR;
                *addr_c = FX_WDR_DIF_C_P4_ADDR;
                break;
        case 5: *addr_a = FX_WDR_DIF_A_P5_ADDR;
                *addr_b = FX_WDR_DIF_B_P5_ADDR;
                *addr_c = FX_WDR_DIF_C_P5_ADDR;
                break;
        case 6: *addr_a = FX_WDR_DIF_A_P6_ADDR;             // 0,6 不使用
                *addr_b = FX_WDR_DIF_B_P6_ADDR;
                *addr_c = FX_WDR_DIF_C_P6_ADDR;
                break;
    }
db_debug("get_isp2_nd_addr() HDR: i_page=%d a=0x%x b=0x%x c=0x%x\n", i_page, *addr_a, *addr_b, *addr_c);
}
void get_isp2_pd_addr(int i_page, int mid, int *addr_a, int *addr_b, int *addr_c)
{
	int idx;
	unsigned mid_w_addr_a=0, mid_w_addr_b=0, mid_w_addr_c=0;
    if(i_page < 0 || i_page > 6){
        db_error("get_isp2_nd_addr: err! i_page=%d\n", i_page);
    }
    if(i_page == mid)	  idx = i_page;
    else if(i_page > mid) idx = i_page - 1;
	else			      idx = i_page + 1;

	switch(idx){
		default:
        case 0: *addr_a = FX_WDR_DIF_A_P0_ADDR;             // 0,6 不使用
                *addr_b = FX_WDR_DIF_B_P0_ADDR;
                *addr_c = FX_WDR_DIF_C_P0_ADDR;
                break;
        case 1: *addr_a = FX_WDR_DIF_A_P1_ADDR;
                *addr_b = FX_WDR_DIF_B_P1_ADDR;
                *addr_c = FX_WDR_DIF_C_P1_ADDR;
                break;
        case 2: *addr_a = FX_WDR_DIF_A_P2_ADDR;
                *addr_b = FX_WDR_DIF_B_P2_ADDR;
                *addr_c = FX_WDR_DIF_C_P2_ADDR;
                break;
        case 3: *addr_a = FX_WDR_DIF_A_P3_ADDR;
                *addr_b = FX_WDR_DIF_B_P3_ADDR;
                *addr_c = FX_WDR_DIF_C_P3_ADDR;
                break;
        case 4: *addr_a = FX_WDR_DIF_A_P4_ADDR;
                *addr_b = FX_WDR_DIF_B_P4_ADDR;
                *addr_c = FX_WDR_DIF_C_P4_ADDR;
                break;
        case 5: *addr_a = FX_WDR_DIF_A_P5_ADDR;
                *addr_b = FX_WDR_DIF_B_P5_ADDR;
                *addr_c = FX_WDR_DIF_C_P5_ADDR;
                break;
        case 6: *addr_a = FX_WDR_DIF_A_P6_ADDR;             // 0,6 不使用
                *addr_b = FX_WDR_DIF_B_P6_ADDR;
                *addr_c = FX_WDR_DIF_C_P6_ADDR;
                break;
	}
db_debug("get_isp2_pd_addr() HDR: i_page=%d idx=%d a=0x%x b=0x%x c=0x%x\n", i_page, idx, *addr_a, *addr_b, *addr_c);
}
void get_isp2_md_addr(int i_page, int *addr_a, int *addr_b, int *addr_c)
{
    if(i_page < 0 || i_page > 6){
        db_error("get_isp2_nd_addr: err! i_page=%d\n", i_page);
    }
    switch(i_page){
        default:
        case 0: *addr_a = FX_WDR_MO_DIFF_A_P0_ADDR;             // 0,6 不使用
                *addr_b = FX_WDR_MO_DIFF_B_P0_ADDR;
                *addr_c = FX_WDR_MO_DIFF_C_P0_ADDR;
                break;
        case 1: *addr_a = FX_WDR_MO_DIFF_A_P1_ADDR;
                *addr_b = FX_WDR_MO_DIFF_B_P1_ADDR;
                *addr_c = FX_WDR_MO_DIFF_C_P1_ADDR;
                break;
        case 2: *addr_a = FX_WDR_MO_DIFF_A_P2_ADDR;
                *addr_b = FX_WDR_MO_DIFF_B_P2_ADDR;
                *addr_c = FX_WDR_MO_DIFF_C_P2_ADDR;
                break;
        case 3: *addr_a = FX_WDR_MO_DIFF_A_P3_ADDR;
                *addr_b = FX_WDR_MO_DIFF_B_P3_ADDR;
                *addr_c = FX_WDR_MO_DIFF_C_P3_ADDR;
                break;
        case 4: *addr_a = FX_WDR_MO_DIFF_A_P4_ADDR;
                *addr_b = FX_WDR_MO_DIFF_B_P4_ADDR;
                *addr_c = FX_WDR_MO_DIFF_C_P4_ADDR;
                break;
        case 5: *addr_a = FX_WDR_MO_DIFF_A_P5_ADDR;
                *addr_b = FX_WDR_MO_DIFF_B_P5_ADDR;
                *addr_c = FX_WDR_MO_DIFF_C_P5_ADDR;
                break;
        case 6: *addr_a = FX_WDR_MO_DIFF_A_P6_ADDR;             // 0,6 不使用
                *addr_b = FX_WDR_MO_DIFF_B_P6_ADDR;
                *addr_c = FX_WDR_MO_DIFF_C_P6_ADDR;
                break;
    }
db_debug("get_isp2_md_addr() HDR: i_page=%d a=0x%x b=0x%x c=0x%x\n", i_page, *addr_a, *addr_b, *addr_c);
}
int get_isp2_gd_offset(int i_page, int mid)
{
	int offset=0;
	int sub = abs(i_page - mid);

	if(i_page == mid)
		offset = 0;
	else {
		switch(sub) {
		default:
		case 1: if(i_page > mid) offset = A2K.ISP2_DG_Offset[0];
				else			 offset = A2K.ISP2_DG_Offset[1];
				break;
		case 2: if(i_page > mid) offset = A2K.ISP2_DG_Offset[2];
				else			 offset = A2K.ISP2_DG_Offset[3];
				break;
		case 3: if(i_page > mid) offset = A2K.ISP2_DG_Offset[4];
				else			 offset = A2K.ISP2_DG_Offset[5];
				break;
		}
	}
db_debug("get_isp2_gd_offset() i_page=%d mid=%d offset=%d\n", i_page, mid, offset);
	return offset;
}
int AS2_DG_MUL_Conv(int Mul_D, int TH_D)
{
	float a,b,c;
	int r;
	a = Mul_D;
	b = TH_D;
	c = (a / 4) * (255 / (255 - b));
	r = c * 32;
	if(r > 255) r = 255;
	return r;
}
void set_ISP2_P0_P1_P2_Addr(int big_img, int fpga_binn, int i_page, int o_page, int c_mode, 
                            int hdr_7idx, int hdr_fcnt, int removal_st, AS2_F0_MAIN_CMD_struct *Cmd_p)
{
    int now_addr_a, now_addr_b, now_addr_c;
    int full_addr_a, full_addr_b, full_addr_c;
    int pre_addr_a, pre_addr_b, pre_addr_c;
    int next_addr_a, next_addr_b, next_addr_c;
    int isp2_RB_offset_0, isp2_RB_offset_1, isp2_RB_offset_2;
    int isp2_w_addr_a=0, isp2_w_addr_b=0, isp2_w_addr_c=0;
    int nr3d_dis = 0, yuv_output = 0x10;
    int d_cnt = read_F_Com_In_Capture_D_Cnt();
    int removal_addr_a = ISP2_6X6_BIN_ADDR_A_P0;
    int removal_addr_b = ISP2_6X6_BIN_ADDR_B_P0;
    int removal_addr_c = ISP2_6X6_BIN_ADDR_C_P0;
    int isp2_nd_addr_a=0, isp2_nd_addr_b=0, isp2_nd_addr_c=0;
    int isp2_pd_addr_a=0, isp2_pd_addr_b=0, isp2_pd_addr_c=0;
    int isp2_md_addr_a=0, isp2_md_addr_b=0, isp2_md_addr_c=0;
    int hdr_mid = (hdr_fcnt>>1);
    int dg_offset = 0;

    if(fpga_binn == 2){ // WDR function
        Cmd_p->F0_ISP2_CMD.WDR_Addr_0.Address = 0xCCAAA048;     // WDR enable
        Cmd_p->F0_ISP2_CMD.WDR_Addr_1.Address = 0xCCAAA058;
        Cmd_p->F0_ISP2_CMD.WDR_Addr_2.Address = 0;
        Cmd_p->F1_ISP2_CMD.WDR_Addr_0.Address = 0xCCAAA048;
        Cmd_p->F1_ISP2_CMD.WDR_Addr_1.Address = 0xCCAAA058;
        Cmd_p->F1_ISP2_CMD.WDR_Addr_2.Address = 0xCCAAA068;
    }
    else{
        Cmd_p->F0_ISP2_CMD.WDR_Addr_0.Address = 0;              // WDR disable
        Cmd_p->F0_ISP2_CMD.WDR_Addr_1.Address = 0;
        Cmd_p->F0_ISP2_CMD.WDR_Addr_2.Address = 0;
        Cmd_p->F1_ISP2_CMD.WDR_Addr_0.Address = 0;
        Cmd_p->F1_ISP2_CMD.WDR_Addr_1.Address = 0;
        Cmd_p->F1_ISP2_CMD.WDR_Addr_2.Address = 0;
    }
    int addr;
    if(big_img == 1){

        if(c_mode == 13 && removal_st >= 4){
            i_page = (hdr_7idx-PIPE_SUBCODE_AEB_STEP_0);
            o_page = 0;
            if(i_page == 3){ yuv_output = 0x10; } 
            else           { yuv_output = 0; } 
            get_removal_now_addr(i_page, &now_addr_a, &now_addr_b, &now_addr_c);
            get_wdr_dif_addr(i_page, &isp2_w_addr_a, &isp2_w_addr_b, &isp2_w_addr_c);
            get_isp2_nd_addr(i_page, &isp2_nd_addr_a, &isp2_nd_addr_b, &isp2_nd_addr_c);
            get_isp2_pd_addr(i_page, hdr_mid, &isp2_pd_addr_a, &isp2_pd_addr_b, &isp2_pd_addr_c);
            get_isp2_md_addr(i_page, &isp2_md_addr_a, &isp2_md_addr_b, &isp2_md_addr_c);
        }
        else if((c_mode == 5 || c_mode == 7) || 
                (c_mode == 13 && removal_st >= 0 && removal_st <= 3))         // 5:D-HDR, 7:N-HDR
        {
            if(hdr_fcnt <= 0){ db_error("set_ISP2: err! fcnt=%d\n", hdr_fcnt); hdr_fcnt = 5; }
            // 3張HDR從1開始做，5張HDR從2開始做，7張HDR從3開始做
            int hdr_idx = ((hdr_7idx-PIPE_SUBCODE_AEB_STEP_0) + (hdr_fcnt>>1)) % hdr_fcnt;
            // hdr_fcnt=3: hdr_idx=1,2,0
            //          5: hdr_idx=2,3,4,0,1
            //          7: hdr_idx=3,4,5,6,0,1,2
            //switch(hdr_7idx){                                                      [Input] [Input]
            //    case PIPE_SUBCODE_AEB_STEP_0: o_page = 0; break;    // 暗 暗2 暗3  ISP1-0  ISP1-0
            //    case PIPE_SUBCODE_AEB_STEP_1: o_page = 1; break;    // 中 暗  暗2  ISP1-1  ISP1-1
            //    case PIPE_SUBCODE_AEB_STEP_2: o_page = 2; break;    // 亮 中  暗   ISP1-2  ISP1-2
            //    case PIPE_SUBCODE_AEB_STEP_3: o_page = 3; break;    //    亮  中   ISP2-0  ISP2-1
            //    case PIPE_SUBCODE_AEB_STEP_4: o_page = 4; break;    //    亮2 亮   ISP2-0  ISP2-0
            //    case PIPE_SUBCODE_AEB_STEP_5: o_page = 5; break;    //        亮2          ISP2-0
            //    case PIPE_SUBCODE_AEB_STEP_6: o_page = 6; break;    //        亮3          NR3D
            //    case PIPE_SUBCODE_AEB_STEP_E: o_page = 0; break;
            //}
            if(hdr_idx == hdr_mid){             // 3
                i_page = hdr_idx;
                o_page = 1;
                yuv_output = 0;
            }
            else if(hdr_idx > hdr_mid){         // 4,5,6
                i_page = hdr_idx;
                o_page = 1;
                yuv_output = 0;
            }
            else if(hdr_idx < hdr_mid){         // 2,1,0
                i_page = hdr_mid-hdr_idx-1;
                o_page = 1;
                if(i_page == 0){ yuv_output = 0x10; }        // 最後一張輸出ISP2 YUV影像
                else           { yuv_output = 0;    }
            }
            if(c_mode == 13 && i_page == 0){
                yuv_output = 0x40;                      // 輸出binn小圖(6x6)
                get_removal_binn_addr(removal_st, &removal_addr_a, &removal_addr_b, &removal_addr_c);   // binn小圖(6x6) output address
                db_debug("set_ISP2_P0_P1_P2_Addr: output=%x addr={%x,%x,%x}\n", yuv_output, removal_addr_a, removal_addr_b, removal_addr_c);
            }
            get_isp2_now_addr(i_page, hdr_fcnt, &now_addr_a, &now_addr_b, &now_addr_c);
            get_wdr_dif_addr(i_page, &isp2_w_addr_a, &isp2_w_addr_b, &isp2_w_addr_c);
            get_isp2_nd_addr(i_page, &isp2_nd_addr_a, &isp2_nd_addr_b, &isp2_nd_addr_c);
            get_isp2_pd_addr(i_page, hdr_mid, &isp2_pd_addr_a, &isp2_pd_addr_b, &isp2_pd_addr_c);
            get_isp2_md_addr(i_page, &isp2_md_addr_a, &isp2_md_addr_b, &isp2_md_addr_c);
        }
        else if(c_mode == 3){                   // 3:AEB
            if(hdr_fcnt <= 0){ db_error("set_ISP2: err! fcnt=%d\n", hdr_fcnt); hdr_fcnt = 5; }
            //i_page = fcnt - 1 - (hdr_7idx - PIPE_SUBCODE_AEB_STEP_0);             // 順序: 0,1,2,3,4,5,6, ISP2-1 DDR共用，需要先做
            i_page = ((hdr_7idx-PIPE_SUBCODE_AEB_STEP_0) + (hdr_fcnt>>1)) % hdr_fcnt;       // 順序: 3,4,5,6,0,1,2
            o_page = 1;                         // ISP2_P0被ISP1_P4、ISP1_P5使用
            nr3d_dis = 3;
            SetWaveDebug(9, 0);
            get_isp2_now_addr(i_page, hdr_fcnt, &now_addr_a, &now_addr_b, &now_addr_c);
            get_wdr_dif_addr(i_page, &isp2_w_addr_a, &isp2_w_addr_b, &isp2_w_addr_c);
            get_isp2_nd_addr(i_page, &isp2_nd_addr_a, &isp2_nd_addr_b, &isp2_nd_addr_c);
            get_isp2_pd_addr(i_page, hdr_mid, &isp2_pd_addr_a, &isp2_pd_addr_b, &isp2_pd_addr_c);
            get_isp2_md_addr(i_page, &isp2_md_addr_a, &isp2_md_addr_b, &isp2_md_addr_c);
        }
        else{                                   // -1
            if(fpga_binn == 2){                 // HDR
                i_page = (i_page + 1) % 3;      // 暗-中-亮, 先取'中'運算
            }
            get_isp2_now_addr(i_page, hdr_fcnt, &now_addr_a, &now_addr_b, &now_addr_c);
            get_wdr_dif_addr(i_page, &isp2_w_addr_a, &isp2_w_addr_b, &isp2_w_addr_c);
            get_isp2_nd_addr(i_page, &isp2_nd_addr_a, &isp2_nd_addr_b, &isp2_nd_addr_c);
            get_isp2_pd_addr(i_page, hdr_mid, &isp2_pd_addr_a, &isp2_pd_addr_b, &isp2_pd_addr_c);
            get_isp2_md_addr(i_page, &isp2_md_addr_a, &isp2_md_addr_b, &isp2_md_addr_c);
        }
        switch(o_page){
        case 0: full_addr_a = ISP2_STM1_T_P0_A_BUF_ADDR;
                full_addr_b = ISP2_STM1_T_P0_B_BUF_ADDR;
                full_addr_c = ISP2_STM1_T_P0_C_BUF_ADDR;
                break;
        case 1: full_addr_a = ISP2_STM1_T_P1_A_BUF_ADDR;
                full_addr_b = ISP2_STM1_T_P1_B_BUF_ADDR;
                full_addr_c = ISP2_STM1_T_P1_C_BUF_ADDR;
                break;
        }
        // pre_addr / next_addr
        if(c_mode == 13){
            if(removal_st < 4){
                get_removal_now_addr(removal_st, &pre_addr_a, &pre_addr_b, &pre_addr_c);        // ISP2_REMOVAL_P0_A_BUF_ADDR...
                get_removal_now_addr(removal_st, &next_addr_a, &next_addr_b, &next_addr_c);
            }
            else{
                get_isp2_now_addr(1, 5, &pre_addr_a, &pre_addr_b, &pre_addr_c);
                get_isp2_now_addr(1, 5, &next_addr_a, &next_addr_b, &next_addr_c);
            }
        }
        else if((c_mode == 5 || c_mode == 7) && hdr_fcnt == 7){           // NR3D記憶體被P6共用，需要改使用P3
            pre_addr_a = ISP1_STM1_T_P3_A_BUF_ADDR;                         // ISP2 P1 buffer
            pre_addr_b = ISP1_STM1_T_P3_B_BUF_ADDR;
            pre_addr_c = ISP1_STM1_T_P3_C_BUF_ADDR;
            next_addr_a = ISP1_STM1_T_P3_A_BUF_ADDR;
            next_addr_b = ISP1_STM1_T_P3_B_BUF_ADDR;
            next_addr_c = ISP1_STM1_T_P3_C_BUF_ADDR;
        }
        else{
            pre_addr_a = NR3D_STM1_A_BUF_ADDR;
            pre_addr_b = NR3D_STM1_B_BUF_ADDR;
            pre_addr_c = NR3D_STM1_C_BUF_ADDR;
            next_addr_a = NR3D_STM1_A_BUF_ADDR;
            next_addr_b = NR3D_STM1_B_BUF_ADDR;
            next_addr_c = NR3D_STM1_C_BUF_ADDR;
        }

        isp2_RB_offset_0 = 0x48;
        isp2_RB_offset_1 = 0x48;
        isp2_RB_offset_2 = 0x48;

        dg_offset = get_isp2_gd_offset(i_page, hdr_mid);
    }
    else{
        if(fpga_binn == 1){     // fpga binn
            if(i_page == 0){
                now_addr_a = ISP2_STM2_S_P0_A_BUF_ADDR;
                now_addr_b = ISP2_STM2_S_P0_B_BUF_ADDR;
                now_addr_c = ISP2_STM2_S_P0_C_BUF_ADDR;
            }
            else{
                now_addr_a = ISP2_STM2_S_P1_A_BUF_ADDR;
                now_addr_b = ISP2_STM2_S_P1_B_BUF_ADDR;
                now_addr_c = ISP2_STM2_S_P1_C_BUF_ADDR;
            }
        }
        else{                   // sensor binn
            if(i_page == 0) {
                now_addr_a = (ISP2_STM2_S_P0_A_BUF_ADDR);        //Sensor Binn 需要偏一條掃描線, FPGA Binn 不需要
                now_addr_b = (ISP2_STM2_S_P0_B_BUF_ADDR);
                now_addr_c = (ISP2_STM2_S_P0_C_BUF_ADDR);
            }
            else {
                now_addr_a = (ISP2_STM2_S_P1_A_BUF_ADDR);
                now_addr_b = (ISP2_STM2_S_P1_B_BUF_ADDR);
                now_addr_c = (ISP2_STM2_S_P1_C_BUF_ADDR);
            }
        }
        
        switch(o_page){
        case 0: full_addr_a = ISP2_STM2_T_P0_A_BUF_ADDR;
                full_addr_b = ISP2_STM2_T_P0_B_BUF_ADDR;
                full_addr_c = ISP2_STM2_T_P0_C_BUF_ADDR;
                break;
        case 1: full_addr_a = ISP2_STM2_T_P1_A_BUF_ADDR;
                full_addr_b = ISP2_STM2_T_P1_B_BUF_ADDR;
                full_addr_c = ISP2_STM2_T_P1_C_BUF_ADDR;
                break;
        }
        pre_addr_a = NR3D_STM2_A_BUF_ADDR;
        pre_addr_b = NR3D_STM2_B_BUF_ADDR;
        pre_addr_c = NR3D_STM2_C_BUF_ADDR;
        next_addr_a = NR3D_STM2_A_BUF_ADDR; //NR3D_STM1_A_BUF_ADDR;
        next_addr_b = NR3D_STM2_B_BUF_ADDR; //NR3D_STM1_B_BUF_ADDR;
        next_addr_c = NR3D_STM2_C_BUF_ADDR; //NR3D_STM1_C_BUF_ADDR;

        isp2_RB_offset_0 = 0x18;
        isp2_RB_offset_1 = 0x18;
        isp2_RB_offset_2 = 0x18;
        if(c_mode == 3 && d_cnt != 0){      // 拍照鍵按下
            yuv_output = 0;                     // 關閉YUV輸出，避免壓到ISP2共用記憶體
            SetWaveDebug(9, 1);
        }

        dg_offset = 0;
    }

    if(i_page == hdr_mid) {
    	isp2_nd_addr_a = isp2_pd_addr_a = isp2_md_addr_a = isp2_w_addr_a;
    	isp2_nd_addr_b = isp2_pd_addr_b = isp2_md_addr_b = isp2_w_addr_b;
    	isp2_nd_addr_c = isp2_pd_addr_c = isp2_md_addr_c = isp2_w_addr_c;
    }

    Cmd_p->F0_ISP2_CMD.Now_Addr_0.Data           = now_addr_a >> 5;
    Cmd_p->F0_ISP2_CMD.Pre_Addr_0.Data           = pre_addr_a >> 5;
    Cmd_p->F0_ISP2_CMD.Next_Addr_0.Data          = next_addr_a >> 5;
    Cmd_p->F0_ISP2_CMD.Sfull_Addr_0.Data         = full_addr_a >> 5;
    Cmd_p->F0_ISP2_CMD.Now_Addr_1.Data           = now_addr_b >> 5;
    Cmd_p->F0_ISP2_CMD.Pre_Addr_1.Data           = pre_addr_b >> 5;
    Cmd_p->F0_ISP2_CMD.Next_Addr_1.Data          = next_addr_b >> 5;
    Cmd_p->F0_ISP2_CMD.Sfull_Addr_1.Data         = full_addr_b >> 5;
    Cmd_p->F0_ISP2_CMD.Now_Addr_2.Data           = now_addr_c >> 5;
    Cmd_p->F0_ISP2_CMD.Pre_Addr_2.Data           = pre_addr_c >> 5;
    Cmd_p->F0_ISP2_CMD.Next_Addr_2.Data          = next_addr_c >> 5;
    Cmd_p->F0_ISP2_CMD.Sfull_Addr_2.Data         = full_addr_c >> 5;

    Cmd_p->F0_ISP2_CMD.RB_Offset_0.Data          = isp2_RB_offset_0;
    Cmd_p->F0_ISP2_CMD.RB_Offset_1.Data          = isp2_RB_offset_1;
    Cmd_p->F0_ISP2_CMD.RB_Offset_2.Data          = isp2_RB_offset_2;

    Cmd_p->F1_ISP2_CMD.Now_Addr_0.Data           = now_addr_a >> 5;
    Cmd_p->F1_ISP2_CMD.Pre_Addr_0.Data           = pre_addr_a >> 5;
    Cmd_p->F1_ISP2_CMD.Next_Addr_0.Data          = next_addr_a >> 5;
    Cmd_p->F1_ISP2_CMD.Sfull_Addr_0.Data         = full_addr_a >> 5;
    Cmd_p->F1_ISP2_CMD.Now_Addr_1.Data           = now_addr_b >> 5;
    Cmd_p->F1_ISP2_CMD.Pre_Addr_1.Data           = pre_addr_b >> 5;
    Cmd_p->F1_ISP2_CMD.Next_Addr_1.Data          = next_addr_b >> 5;
    Cmd_p->F1_ISP2_CMD.Sfull_Addr_1.Data         = full_addr_b >> 5;
    Cmd_p->F1_ISP2_CMD.Now_Addr_2.Data           = now_addr_c >> 5;
    Cmd_p->F1_ISP2_CMD.Pre_Addr_2.Data           = pre_addr_c >> 5;
    Cmd_p->F1_ISP2_CMD.Next_Addr_2.Data          = next_addr_c >> 5;
    Cmd_p->F1_ISP2_CMD.Sfull_Addr_2.Data         = full_addr_c >> 5;

    Cmd_p->F1_ISP2_CMD.RB_Offset_0.Data          = isp2_RB_offset_0;
    Cmd_p->F1_ISP2_CMD.RB_Offset_1.Data          = isp2_RB_offset_1;
    Cmd_p->F1_ISP2_CMD.RB_Offset_2.Data          = isp2_RB_offset_2;

    Cmd_p->F0_ISP2_CMD.WDR_Addr_0.Data           = isp2_w_addr_a >> 5;      // WDR source address
    Cmd_p->F0_ISP2_CMD.WDR_Addr_1.Data           = isp2_w_addr_b >> 5;
    Cmd_p->F0_ISP2_CMD.WDR_Addr_2.Data           = isp2_w_addr_c >> 5;
    Cmd_p->F1_ISP2_CMD.WDR_Addr_0.Data           = isp2_w_addr_a >> 5;
    Cmd_p->F1_ISP2_CMD.WDR_Addr_1.Data           = isp2_w_addr_b >> 5;
    Cmd_p->F1_ISP2_CMD.WDR_Addr_2.Data           = isp2_w_addr_c >> 5;
    
    Cmd_p->F0_ISP2_CMD.ND_Addr_0.Data            = isp2_nd_addr_a >> 5;
    Cmd_p->F0_ISP2_CMD.ND_Addr_1.Data            = isp2_nd_addr_b >> 5;
    Cmd_p->F0_ISP2_CMD.ND_Addr_2.Data            = isp2_nd_addr_c >> 5;
    Cmd_p->F1_ISP2_CMD.ND_Addr_0.Data            = isp2_nd_addr_a >> 5;
    Cmd_p->F1_ISP2_CMD.ND_Addr_1.Data            = isp2_nd_addr_b >> 5;
    Cmd_p->F1_ISP2_CMD.ND_Addr_2.Data            = isp2_nd_addr_c >> 5;

    Cmd_p->F0_ISP2_CMD.PD_Addr_0.Data            = isp2_pd_addr_a >> 5;
    Cmd_p->F0_ISP2_CMD.PD_Addr_1.Data            = isp2_pd_addr_b >> 5;
    Cmd_p->F0_ISP2_CMD.PD_Addr_2.Data            = isp2_pd_addr_c >> 5;
    Cmd_p->F1_ISP2_CMD.PD_Addr_0.Data            = isp2_pd_addr_a >> 5;
    Cmd_p->F1_ISP2_CMD.PD_Addr_1.Data            = isp2_pd_addr_b >> 5;
    Cmd_p->F1_ISP2_CMD.PD_Addr_2.Data            = isp2_pd_addr_c >> 5;

    Cmd_p->F0_ISP2_CMD.MD_Addr_0.Data            = isp2_md_addr_a >> 5;
    Cmd_p->F0_ISP2_CMD.MD_Addr_1.Data            = isp2_md_addr_b >> 5;
    Cmd_p->F0_ISP2_CMD.MD_Addr_2.Data            = isp2_md_addr_c >> 5;
    Cmd_p->F1_ISP2_CMD.MD_Addr_0.Data            = isp2_md_addr_a >> 5;
    Cmd_p->F1_ISP2_CMD.MD_Addr_1.Data            = isp2_md_addr_b >> 5;
    Cmd_p->F1_ISP2_CMD.MD_Addr_2.Data            = isp2_md_addr_c >> 5;

    static int ov_mul_lst=0, dg_th_lst=0;
    if(Overlay_Mul != ov_mul_lst || DeGhost_Th != dg_th_lst) {
    	DeGhost_Mul = AS2_DG_MUL_Conv(Overlay_Mul, DeGhost_Th);
    	ov_mul_lst = Overlay_Mul;
    	dg_th_lst  = DeGhost_Th;
    }
    Cmd_p->F0_ISP2_CMD.OV_Mul.Data               = DeGhost_Mul;
    Cmd_p->F1_ISP2_CMD.OV_Mul.Data               = DeGhost_Mul;

    Cmd_p->F0_ISP2_CMD.DG_Offset.Data            = dg_offset;
    Cmd_p->F1_ISP2_CMD.DG_Offset.Data            = dg_offset;

    Cmd_p->F0_ISP2_CMD.DG_TH.Data                = DeGhost_Th;
    Cmd_p->F1_ISP2_CMD.DG_TH.Data                = DeGhost_Th;
if(big_img == 1) {
	db_debug("set_ISP2_P0_P1_P2_Addr() HDR: i_page=%d wdr=0x%x nd=0x%x pd=0x%x md=0x%x OV_Mul=%d DG_TH=%d DG_Mul=%d\n",
			i_page, isp2_w_addr_a, isp2_nd_addr_a, isp2_pd_addr_a, isp2_md_addr_a, Overlay_Mul, DeGhost_Th, DeGhost_Mul);
}

    // 控制ISP2輸出YUV影像給ST(10:enable YUV, 0:disable YUV)
    Cmd_p->F0_ISP2_CMD.SH_Mode.Data = yuv_output;                   // 0x0;
    Cmd_p->F1_ISP2_CMD.SH_Mode.Data = yuv_output;                   // 0x0;
    // 控制ISP2停止NR3D運算(0:enable 3DNR, 3:disable 3DNR)
    Cmd_p->F0_ISP2_CMD.NR3D_Disable_0.Data = nr3d_dis;          // 0:normal(enable), 3:disable(AEB used DRR)
    Cmd_p->F0_ISP2_CMD.NR3D_Disable_1.Data = nr3d_dis;
    Cmd_p->F0_ISP2_CMD.NR3D_Disable_2.Data = nr3d_dis;
    Cmd_p->F1_ISP2_CMD.NR3D_Disable_0.Data = nr3d_dis;
    Cmd_p->F1_ISP2_CMD.NR3D_Disable_1.Data = nr3d_dis;
    Cmd_p->F1_ISP2_CMD.NR3D_Disable_2.Data = nr3d_dis;
    
    Cmd_p->F0_ISP2_CMD.Scaler_Addr_0.Data = removal_addr_a >> 5;          // 0:normal(enable), 3:disable(AEB used DRR)
    Cmd_p->F0_ISP2_CMD.Scaler_Addr_1.Data = removal_addr_b >> 5;
    Cmd_p->F0_ISP2_CMD.Scaler_Addr_2.Data = removal_addr_c >> 5;
    Cmd_p->F1_ISP2_CMD.Scaler_Addr_0.Data = removal_addr_a >> 5;
    Cmd_p->F1_ISP2_CMD.Scaler_Addr_1.Data = removal_addr_b >> 5;
    Cmd_p->F1_ISP2_CMD.Scaler_Addr_2.Data = removal_addr_c >> 5;
}
int set_ISP2_Colum_Size(int M_Mode, AS2_F0_MAIN_CMD_struct *Cmd_p)
{
	int isp2_colum, isp2_line;

    switch(M_Mode){
	case 0: isp2_colum = 24; isp2_line = 3392; break;	//24 = 4608 / 192
	case 1: isp2_colum = 12; isp2_line = 1728; break;
	case 2: isp2_colum = 12; isp2_line = 1728; break;
	case 3: isp2_colum = 8;  isp2_line = 1152; break;
	case 4: isp2_colum = 6;  isp2_line = 864;  break;
	case 5: isp2_colum = 6;  isp2_line = 864;  break;
    }
	Cmd_p->F0_ISP2_CMD.Col_Size_0.Data           = isp2_colum - 1;
	Cmd_p->F0_ISP2_CMD.Size_Y_0.Data             = isp2_line >> 1;
	Cmd_p->F0_ISP2_CMD.Col_Size_1.Data           = isp2_colum - 1;
	Cmd_p->F0_ISP2_CMD.Size_Y_1.Data             = isp2_line >> 1;
	Cmd_p->F0_ISP2_CMD.Col_Size_2.Data           = isp2_colum - 1;
	Cmd_p->F0_ISP2_CMD.Size_Y_2.Data             = isp2_line >> 1;
    Cmd_p->F0_ISP2_CMD.Start_En_0.Data           = 1;
    Cmd_p->F0_ISP2_CMD.Start_En_1.Data           = 1;
    Cmd_p->F0_ISP2_CMD.Start_En_2.Data           = 0;

	Cmd_p->F1_ISP2_CMD.Col_Size_0.Data           = isp2_colum - 1;
	Cmd_p->F1_ISP2_CMD.Size_Y_0.Data             = isp2_line >> 1;
	Cmd_p->F1_ISP2_CMD.Col_Size_1.Data           = isp2_colum - 1;
	Cmd_p->F1_ISP2_CMD.Size_Y_1.Data             = isp2_line >> 1;
	Cmd_p->F1_ISP2_CMD.Col_Size_2.Data           = isp2_colum - 1;
	Cmd_p->F1_ISP2_CMD.Size_Y_2.Data             = isp2_line >> 1;
    Cmd_p->F1_ISP2_CMD.Start_En_0.Data           = 1;
    Cmd_p->F1_ISP2_CMD.Start_En_1.Data           = 1;
    Cmd_p->F1_ISP2_CMD.Start_En_2.Data           = 1;
}
/*
 * cap_nr3d_idx: -1 -> Used 3DNR last table
 *                0 -> Used 3DNR table-1
 *                1 -> Used 3DNR table-2
 *                2 -> Used 3DNR table-3
 */
void set_ISP2_NR3D_Level_Rate(int nr3d_idx, AS2_F0_MAIN_CMD_struct *Cmd_p)
{
    int rate;
    
    Cmd_p->F0_ISP2_CMD.NR3D_Level.Data = A2K.ISP2_NR3D_Level;
    Cmd_p->F1_ISP2_CMD.NR3D_Level.Data = A2K.ISP2_NR3D_Level;
    
    switch(nr3d_idx){
    default:rate = A2K.ISP2_NR3D_Rate; break;   // Rec, TimeLapse, Normal used last table
    case 0: rate = 0x00000000; break;           // Capture 3DNR table1, NR3D_strength_tbl[0]
    case 1: rate = 0x00406070; break;           // Capture 3DNR table2, NR3D_strength_tbl[8]
    case 2: rate = 0x0056727C; break;           // Capture 3DNR table3, NR3D_strength_tbl[15]
    }
    //db_debug("set_ISP2_NR3D_Level_Rate: debug! nr3d=%d\n", nr3d_idx);
    
    Cmd_p->F0_ISP2_CMD.NR3D_Rate.Data = rate;
    Cmd_p->F1_ISP2_CMD.NR3D_Rate.Data = rate;
}

int Make_Debug_ISP2_CMD(AS2_F0_MAIN_CMD_struct *Cmd_p)
{
    int DebugJPEGMode = A2K.DebugJPEGMode;
    int DebugJPEGaddr = A2K.DebugJPEGaddr;
    int ISP2_Sensor   = A2K.ISP2_Sensor;
    int Focus_Sensor  = A2K.Focus_Sensor;
	int i, debug=0;
	int FPGA_ID, MT;

	if(DebugJPEGMode == 1 && ISP2_Sensor == -4) {
		Get_FId_MT(Focus_Sensor, &FPGA_ID, &MT);

		for(i = 0; i < 2; i++) {
			Cmd_p->F0_ISP2_CMD.Start_En_0.Data = 0;
			Cmd_p->F0_ISP2_CMD.Start_En_1.Data = 0;
			Cmd_p->F0_ISP2_CMD.Start_En_2.Data = 0;

			Cmd_p->F1_ISP2_CMD.Start_En_0.Data = 0;
			Cmd_p->F1_ISP2_CMD.Start_En_1.Data = 0;
			Cmd_p->F1_ISP2_CMD.Start_En_2.Data = 0;

			if(FPGA_ID == 0) {
				if(MT == 0)      Cmd_p->F0_ISP2_CMD.Start_En_0.Data = 1;
				else if(MT == 1) Cmd_p->F0_ISP2_CMD.Start_En_1.Data = 1;
				else if(MT == 2) Cmd_p->F0_ISP2_CMD.Start_En_2.Data = 1;
			}
			else if(FPGA_ID == 1) {
				if(MT == 0)      Cmd_p->F1_ISP2_CMD.Start_En_0.Data = 1;
				else if(MT == 1) Cmd_p->F1_ISP2_CMD.Start_En_1.Data = 1;
				else if(MT == 2) Cmd_p->F1_ISP2_CMD.Start_En_2.Data = 1;
			}
		}
		debug = 1;
	}
	else if(DebugJPEGMode == 1 && ISP2_Sensor == -6) {
		Get_FId_MT(Focus_Sensor, &FPGA_ID, &MT);

		for(i = 0; i < 2; i++) {
			Cmd_p->F0_ISP2_CMD.Start_En_0.Data = 0;
			Cmd_p->F0_ISP2_CMD.Start_En_1.Data = 0;
			Cmd_p->F0_ISP2_CMD.Start_En_2.Data = 0;

			Cmd_p->F1_ISP2_CMD.Start_En_0.Data = 0;
			Cmd_p->F1_ISP2_CMD.Start_En_1.Data = 0;
			Cmd_p->F1_ISP2_CMD.Start_En_2.Data = 0;

			if(FPGA_ID == 0) {
				if(MT == 0)      Cmd_p->F0_ISP2_CMD.Start_En_0.Data = 1;
				else if(MT == 1) Cmd_p->F0_ISP2_CMD.Start_En_1.Data = 1;
				else if(MT == 2) Cmd_p->F0_ISP2_CMD.Start_En_2.Data = 1;
			}
			else if(FPGA_ID == 1) {
				if(MT == 0)      Cmd_p->F1_ISP2_CMD.Start_En_0.Data = 1;
				else if(MT == 1) Cmd_p->F1_ISP2_CMD.Start_En_1.Data = 1;
				else if(MT == 2) Cmd_p->F1_ISP2_CMD.Start_En_2.Data = 1;
			}
		}
		debug = 1;
	}

	return debug;
}
int Get_DeGhost_En(int c_mode)
{
	int en;
    if(c_mode == 5 /*|| c_mode == 7*/) en = A2K.DeGhostEn;
    else							   en = 0;
    return en;
}
int Make_WDR_Cmd(int idx, AS2_F0_MAIN_CMD_struct *Cmd_p, FPGA_Engine_One_Struct *F_Pipe_p, F_Com_In_Struct *F_Com_p, int do_isp1_flag)
{
    int C_Mode = F_Com_p->Record_Mode;
    int M_Mode = F_Pipe_p->M_Mode;
    int i_page = (F_Pipe_p->I_Page % 10);
    int wdr_en = 0, hdr_7idx = -1, wdr_idx = -1, wdr_mode = -1;
    int deGhost_en = 0;

    get_F_Temp_WDR_SubData(idx, &hdr_7idx, &wdr_idx, &wdr_mode);
    if(wdr_mode == 1 && wdr_idx == 1 && do_isp1_flag == 1) {
    	make_Diffusion_Cmd(idx, 1, M_Mode, C_Mode, hdr_7idx, wdr_idx, wdr_mode, 0, Cmd_p);
    	wdr_en = 1;
    }
    else if(wdr_mode == 2 || wdr_mode == 3 || wdr_mode == 5 || wdr_mode == 7){
    	deGhost_en = Get_DeGhost_En(C_Mode);
        if(M_Mode == 0 || M_Mode == 1 || M_Mode == 2){
            send_WDR_Diffusion_Table(C_Mode);                         // us360_func.c
            make_Diffusion_Cmd(idx, 1, M_Mode, C_Mode, hdr_7idx, wdr_idx, wdr_mode, deGhost_en, Cmd_p);
            wdr_en = 1;
        }
    }
    cle_F_Temp_WDR_SubData(idx);
    
    if(wdr_en == 0) {
    	make_Diffusion_Cmd(idx, 0, -1, -1, -1, -1, -1, 0, Cmd_p);
    	return 0;
    }
    return 1;
}

int BD_TH_Debug = 8;
void SetBDTHDebug(int value)
{
	BD_TH_Debug = value;
}

int GetBDTHDebug(void)
{
	return BD_TH_Debug;
}

/*
 * return: 0 -> wait times up
 *         1 -> execute;
 */
int Make_ISP2_Cmd(int idx, AS2_F0_MAIN_CMD_struct *Cmd_p, FPGA_Engine_One_Struct *F_Pipe_p, F_Com_In_Struct *F_Com_p)
{
    int C_Mode = F_Com_p->Record_Mode;
    int M_Mode = F_Pipe_p->M_Mode;
    int i_page = (F_Pipe_p->I_Page % 10);
    int o_page = (F_Pipe_p->O_Page % 10);
    int DebugJPEGMode = A2K.DebugJPEGMode;
    int ISP2_Sensor = A2K.ISP2_Sensor;
    int debug;
    int YR=A2K.ISP2_YR, YG=A2K.ISP2_YG, YB=A2K.ISP2_YB;
    int UR=A2K.ISP2_UR, UG=A2K.ISP2_UG, UB=A2K.ISP2_UB;
    int VR=A2K.ISP2_VR, VG=A2K.ISP2_VG, VB=A2K.ISP2_VB;
    int DefectEn = A2K.ISP2_Defect_En;
    int big_img = 1, fpga_binn = 0, nr3d_idx = -1, hdr_7idx = -1, hdr_fcnt = -1;
    int removal_st;
    
    if(F_Pipe_p->Time_Buf <= 0) return 0;
    
    if(DebugJPEGMode == 1 && (ISP2_Sensor == -4 || ISP2_Sensor == -6 || ISP2_Sensor == -9) ) {
        C_Mode = 0; M_Mode = 0;
        i_page = 0; o_page = 0;
    }

    switch(C_Mode) {
    case 0: // Cap (1p)
    case 3: // AEB (3p,5p,7p)
    case 4: // RAW (5p)
    case 5: // HDR (1p)
    case 6: // Night
    case 7: // Night + HDR
    case 8: // Sport
    case 9: // Sport + WDR
    case 12:// M-Mode
    case 13:// Removal
    case 14:// 3D-Model
        switch(M_Mode) {
        case 0:
        case 1:
        case 2: get_F_Temp_ISP2_SubData(idx, &nr3d_idx, &hdr_7idx, &hdr_fcnt, &removal_st);
                if(C_Mode == 5 || C_Mode == 7 || C_Mode == 13){ big_img = 1; fpga_binn = 2; } 
                else                                          { big_img = 1; fpga_binn = 0; }

                db_debug("Make_ISP2_Cmd: idx=%d i_page=%d o_page=%d fpga_binn=%d nr3d=%d 7idx=%d fcnt=%d st=%d\n", idx, 
                        i_page, o_page, fpga_binn, nr3d_idx, hdr_7idx, hdr_fcnt, removal_st);
                break;
        case 3:
        case 4:
        case 5: big_img = 0; fpga_binn = 1; nr3d_idx = -1; break;
        }
        break;
    case 1: //Rec
    case 10: //Rec + WDR
        switch(M_Mode) {
        case 3: big_img = 0; fpga_binn = 0; nr3d_idx = -1; break;
        case 4:
        case 5: big_img = 0; fpga_binn = 1; nr3d_idx = -1; break;
        }
        break;

    case 2: //Time Lapse
    case 11: //Time Lapse + WDR
        switch(M_Mode) {
        case 0:
        case 1:
        case 2: big_img = 1; fpga_binn = 0; nr3d_idx = -1; break;
        case 3: big_img = 0; fpga_binn = 0; nr3d_idx = -1; break;
        }
        break;
    }
    set_ISP2_P0_P1_P2_Addr(big_img, fpga_binn, i_page, o_page, C_Mode, hdr_7idx, hdr_fcnt, removal_st, Cmd_p);
    set_ISP2_Colum_Size(M_Mode, Cmd_p);
    set_ISP2_NR3D_Level_Rate(nr3d_idx, Cmd_p);              // change NR3D

    Cmd_p->F0_ISP2_CMD.Set_Y_RG_mulV.Data        = (YG << 16) | YR;
    Cmd_p->F0_ISP2_CMD.Set_Y_B_mulV.Data         = YB;
    Cmd_p->F0_ISP2_CMD.Set_U_RG_mulV.Data        = (UG << 16) | UR;
    Cmd_p->F0_ISP2_CMD.Set_UV_BR_mulV.Data       = (VR << 16) | UB;
    Cmd_p->F0_ISP2_CMD.Set_V_GB_mulV.Data        = (VB << 16) | VG;

    Cmd_p->F1_ISP2_CMD.Set_Y_RG_mulV.Data        = (YG << 16) | YR;
    Cmd_p->F1_ISP2_CMD.Set_Y_B_mulV.Data         = YB;
    Cmd_p->F1_ISP2_CMD.Set_U_RG_mulV.Data        = (UG << 16) | UR;
    Cmd_p->F1_ISP2_CMD.Set_UV_BR_mulV.Data       = (VR << 16) | UB;
    Cmd_p->F1_ISP2_CMD.Set_V_GB_mulV.Data        = (VB << 16) | VG;

    if(C_Mode == 12) {		//M-Mode
    	Cmd_p->F0_ISP2_CMD.BD_TH.Data = (1 << 31) | BD_TH_Debug;
    	Cmd_p->F1_ISP2_CMD.BD_TH.Data = (1 << 31) | BD_TH_Debug;
    }
    else {
    	Cmd_p->F0_ISP2_CMD.BD_TH.Data = 0;
    	Cmd_p->F1_ISP2_CMD.BD_TH.Data = 0;
    }

    Cmd_p->F0_ISP2_CMD.Defect_Addr_0.Data = (DefectEn << 31) | (FX_DEFECT_TABLE_A_ADDR >> 5);
    Cmd_p->F0_ISP2_CMD.Defect_Addr_1.Data = (DefectEn << 31) | (FX_DEFECT_TABLE_B_ADDR >> 5);
    Cmd_p->F0_ISP2_CMD.Defect_Addr_2.Data = 0;
    Cmd_p->F1_ISP2_CMD.Defect_Addr_0.Data = (DefectEn << 31) | (FX_DEFECT_TABLE_A_ADDR >> 5);
    Cmd_p->F1_ISP2_CMD.Defect_Addr_1.Data = (DefectEn << 31) | (FX_DEFECT_TABLE_B_ADDR >> 5);
    Cmd_p->F1_ISP2_CMD.Defect_Addr_2.Data = (DefectEn << 31) | (FX_DEFECT_TABLE_C_ADDR >> 5);

    debug = Make_Debug_ISP2_CMD(Cmd_p);
    if(debug ==1) return 1;

    return 1;
}

//Debug
int ST_S_Cmd_Debug_Flag = 0;		// 0:ST_O 1:ST_I
void Set_ST_S_Cmd_Debug_Flag(int value)
{
	ST_S_Cmd_Debug_Flag = value;
}
int Get_ST_S_Cmd_Debug_Flag(void)
{
	return ST_S_Cmd_Debug_Flag;
}
typedef struct Line_YUV_Offset_Step_H
{
    int         step;
    int         t2_idx;
    int         t4_idx;
} Line_YUV_Offset_Step_S;
Line_YUV_Offset_Step_S Line_YUV_Ost_Step[8] = {{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}};
/*
 * step: 0->normal, 1->start, 2->st_yuvz finish
 *     : 3->readAWB, 4->smooth proc, 5->st_map finish
 */
void set_Line_YUV_Offset_Step(int i, int step)
{
    if(i < 0 || i > 7) return;
    Line_YUV_Ost_Step[i].step = step;
    
    int t2_idx = Line_YUV_Ost_Step[i].t2_idx;
    int t4_idx = Line_YUV_Ost_Step[i].t4_idx;
    SetWaveDebug(8, (i<<24) | (step<<16) | (t2_idx<<8) | (t4_idx));
}
void start_Line_YUV_Offset_Step(int st_state, int i, int idx)
{
    int p_mask = (FPGA_ENGINE_P_MAX-1);                   // FPGA_ENGINE_P_MAX=128
    if(i < 0 || i > 7) return;
    if(st_state == 1){
        Line_YUV_Ost_Step[i].t2_idx = (idx+1) & p_mask;      // 設定st_yuvz idx
        Line_YUV_Ost_Step[i].t4_idx = -1;
        set_Line_YUV_Offset_Step(i, 1);
    }
    else if(st_state == 2){
        Line_YUV_Ost_Step[i].t4_idx = (idx+3) & p_mask;      // 設定st_map idx
    }
    
}
/*
 * check Stitch command finished && goto state 2
 */
void run_Line_YUV_Offset_Step_S1S4(int idx)
{
    int i;
    for(i = 1; i < 8; i++){
        if(Line_YUV_Ost_Step[i].step == 1 &&
           Line_YUV_Ost_Step[i].t2_idx == idx)
        {
            set_Line_YUV_Offset_Step(i, 2);
        }
        else if(Line_YUV_Ost_Step[i].step == 4 &&
                Line_YUV_Ost_Step[i].t4_idx == idx)
        {
            set_Line_YUV_Offset_Step(i, 5);
        }    
    }
}
/*
 * 0: pass
 * 1: execute
 */
int chk_Line_YUV_Offset_Step_S3(void)
{
    int i;
    for(i = 1; i < 8; i++){
        if(Line_YUV_Ost_Step[i].step == 3)
            return i;
    }
    return 0;
}
int chk_Line_YUV_Offset_Step_S4(void)
{
    int i;
    for(i = 1; i < 8; i++){
        if(Line_YUV_Ost_Step[i].step == 4)
            return i;
    }
    return 0;
}
void run_Line_YUV_Offset_Step_S3(void)
{
    int i;
    for(i = 1; i < 8; i++){                         // 0:Normal, 1~3:HDR
        if(Line_YUV_Ost_Step[i].step == 3){
            set_Line_YUV_Offset_Step(i, 4);
            break;
        }
    }
}
int run_Line_YUV_Offset_Step_S2(int *yuv_ost)
{
    int i;
    for(i = 1; i < 8; i++){                         // 0:Normal, 1~3:HDR
        if(Line_YUV_Ost_Step[i].step == 3 ||        // waiting st_map finish
           Line_YUV_Ost_Step[i].step == 4)
        {
            *yuv_ost = i * YUV_LINE_YUV_OFFSET;
            return i;
        }
        if(Line_YUV_Ost_Step[i].step == 2){
            set_Line_YUV_Offset_Step(i, 3);
            *yuv_ost = i * YUV_LINE_YUV_OFFSET;
            return i;
        }
    }
    *yuv_ost = 0;
    return 0;
}

//190806 暫時解因HDR取到垃圾資料擴散, 導致縫合看到邊界問題
int Add_Mask_X = 0, Add_Mask_Y = -10;
int Add_Pre_Mask_X = 0, Add_Pre_Mask_Y = 10;
void SetMaskAdd(int idx, int value)
{
	switch(idx) {
	case 0: Add_Mask_X = value;     break;
	case 1: Add_Mask_Y = value;     break;
	case 2: Add_Pre_Mask_X = value; break;
	case 3: Add_Pre_Mask_Y = value; break;
	}
}
int GetMaskAdd(int idx)
{
	int value;
	switch(idx) {
	case 0: value = Add_Mask_X;     break;
	case 1: value = Add_Mask_Y;     break;
	case 2: value = Add_Pre_Mask_X; break;
	case 3: value = Add_Pre_Mask_Y; break;
	}
	return value;
}

void Get_ST_Mask_XY(int M_Mode, int *X, int *Y)
{
	int binn = A_L_I3_Header[M_Mode].Binn;
    switch(M_Mode) {
    case 0:	//12K
    		*X = 4356 / binn;	//4354 / binn;
    		*Y = 3274 / binn;	//3276 / binn;
    	    break;
    case 1:	//8K
    		*X = 4330 / binn;
    		*Y = 3272 / binn;
			break;
    case 2:	//6K
    		*X = 4330 / binn;
    		*Y = 3272 / binn;
        	break;
    case 3:	//4K
    		*X = 4347 / binn;	//mask_x = (4390 / binn) - 14;
    		*Y = 3252 / binn;	//mask_y = (3284 / binn) - 7;
			break;
    case 4:	//3K
    		*X = 4340 / binn;
    		*Y = 3260 / binn;
			break;
    case 5:	//2K
    		*X = 4314 / binn;
    		*Y = 3234 / binn;
    		break;
    }
}
void set_ST_Cmd_Addr(int big_img, int C_Mode, int M_Mode, int ST_LM, int i_page, int o_page,
                    int yuvz_en, int ddr_5s, int st_step, int yuv_ddr,
                    AS2_F0_MAIN_CMD_struct *Cmd_p, int idx)
{
    int st_s_addr, st_s_addr_base;
    int st_s_addr_lm, st_s_addr_base_lm, st_lm_en = 0;
    int st_t_offset;
    int doTrans = get_A2K_doTrans(M_Mode);
    int i, ST_YUV_ADDR, LINE_YUV_ADDR;
    AS2_Stitch_CMD_struct *FX_Stitch_CMD;
    int img_sum=0, img_cmd_addr, f_id, s_id, MT;
    int binn = A_L_I3_Header[M_Mode].Binn;
    int p_st_idx = A2K.ST_3DModel_Idx;
    if(binn < 1) binn = 1;

    if(big_img == 1){
    	st_s_addr_base = (i_page == 0)? ST_STM1_P0_S_ADDR: ST_STM1_P1_S_ADDR;       // F0/F1, (ISP2_STM1_T_P0_BUF_ADDR - 128 * 2 - 128 *0x8000)
        st_t_offset = o_page * (ST_STM1_P1_T_ADDR - ST_STM1_P0_T_ADDR);             // F2

        if(ST_LM == -1 || st_step == 1 || st_step == 2) {
            st_lm_en = 1;
            st_s_addr_base_lm = (i_page == 0)? ST_STM1_P0_S_ADDR: ST_STM1_P1_S_ADDR;
            ST_LM = M_Mode;                                 // 使用 M_Mode 縫合資料
        }
        else {                                              // ST_LM=3,4 固定使用小圖縫合
            st_lm_en = 0;
            st_s_addr_base_lm = (i_page == 0)? ST_STM2_P0_S_ADDR: ST_STM2_P1_S_ADDR;
        }
    }
    else{
    	st_s_addr_base   = (i_page == 0)? ST_STM2_P0_S_ADDR: ST_STM2_P1_S_ADDR;
        switch(o_page) {
        case 0: st_t_offset = 0; break;
        case 1: st_t_offset = ST_STM2_P1_T_ADDR - ST_STM2_P0_T_ADDR; break;
        case 2: st_t_offset = ST_STM2_P2_T_ADDR - ST_STM2_P0_T_ADDR; break;
        }

        st_lm_en = 1;
        st_s_addr_base_lm = (i_page == 0)? ST_STM2_P0_S_ADDR: ST_STM2_P1_S_ADDR;
        ST_LM = M_Mode;                                     // Stitch Line Mode (ST_LM) = M_Mode
    }
    if(ddr_5s >= PIPE_SUBCODE_REMOVAL_CNT_0 && ddr_5s <= PIPE_SUBCODE_REMOVAL_CNT_3){   // Removal
        st_s_addr_base    = ISP2_6X6_BIN_ADDR_A_P0 - 128 * 2 - 128 *0x8000;             // F1 DDR
        st_s_addr_base_lm = ST_STM1_P0_S_ADDR;
        st_t_offset = 0;
    }
    //*r_st_s_addr = st_s_addr;
    //*r_st_t_offset = st_t_offset;
    for(i = 0; i < 2; i++) {			//F_Id
        //Cmd_p->F0_Stitch_CMD.Start_En2.Data
        if(i == 0){ FX_Stitch_CMD = &Cmd_p->F0_Stitch_CMD; ST_YUV_ADDR = F0_ST_YUV_ADDR; }
        else      { FX_Stitch_CMD = &Cmd_p->F1_Stitch_CMD; ST_YUV_ADDR = F1_ST_YUV_ADDR; }

        if(ddr_5s >= PIPE_SUBCODE_REMOVAL_CNT_0 && ddr_5s <= PIPE_SUBCODE_REMOVAL_CNT_3){   // Removal
            s_id         = (ddr_5s - PIPE_SUBCODE_REMOVAL_CNT_0);                           // 0,1,2,3 offset 768*3*2
            st_s_addr    = st_s_addr_base;  // + (s_id * (768*3) * 2);                      // ISP2_6X6_BIN_ADDR_A_P0
            st_s_addr_lm = st_s_addr_base_lm;
            st_t_offset  = 0;
            img_sum      = ST_S_Header[i].Sum[0][i];
            img_cmd_addr = FX_ST_S_CMD_ADDR + ST_S_Header[i].Start_Idx[0] * 32;
            db_debug("img_sum=%d st_s_addr=%x lm=%x offset=%x cmd_addr=%x\n", img_sum, st_s_addr, st_s_addr_lm, st_t_offset, img_cmd_addr);
        }
        else if(ddr_5s >= PIPE_SUBCODE_5S_CNT_0 && ddr_5s <= PIPE_SUBCODE_5S_CNT_4) {       // RAW only
        	s_id = (ddr_5s - PIPE_SUBCODE_5S_CNT_0);
        	Get_FId_MT(s_id, &f_id, &MT);
        	st_s_addr = st_s_addr_base + (MT * 4608 * 2);
        	st_s_addr_lm = st_s_addr_base_lm;
        	if(f_id == i) img_sum = ST_S_Header[2+binn-1].Sum[0][i];
        	else          img_sum = 0;
        	img_cmd_addr = FX_ST_S_CMD_ADDR + ST_S_Header[2+binn-1].Start_Idx[0] * 32;
        }
        else {                                                                              // CAP, HDR, Small...
        	st_s_addr = st_s_addr_base;
        	st_s_addr_lm = st_s_addr_base_lm;

        	if(C_Mode == 14 && big_img == 0) {		//3D-Model
				/*if(doTrans == 1) img_sum = ST_3DModel_Header[p_st_idx].Sum[ST_H_Img][i] + ST_3DModel_Header[idx].Sum[ST_H_Trans][i];
				else*/             img_sum = ST_3DModel_Header[p_st_idx].Sum[ST_H_Img][i];

				//if(ST_S_Cmd_Debug_Flag == 0)
				//	img_cmd_addr = PLANT_ST_O_2_CMD_ADDR + ST_3DModel_Header[p_st_idx].Start_Idx[ST_H_Img] * 32;
				//else
					img_cmd_addr = FX_PLANT_ST_CMD_ADDR + p_st_idx * PLANT_ST_CMD_PAGE_OFFSET + ST_3DModel_Header[p_st_idx].Start_Idx[ST_H_Img] * 32;
        	}
        	else {
				if(doTrans == 1) img_sum = ST_Header[M_Mode].Sum[ST_H_Img][i] + ST_Header[M_Mode].Sum[ST_H_Trans][i];
				else             img_sum = ST_Header[M_Mode].Sum[ST_H_Img][i];

				if(ST_S_Cmd_Debug_Flag == 0)
					img_cmd_addr = SPI_IO_MP_ST_O_2_P0_DATA + ST_Header[M_Mode].Start_Idx[ST_H_Img] * 32;
				else
					img_cmd_addr = FX_ST_CMD_ADDR + ST_Header[M_Mode].Start_Idx[ST_H_Img] * 32;
        	}
        }
    
        int enable;
        //MAP
        enable = 0;
        if(st_step == -1 || st_step == 2){
          if(img_sum != 0) {
            FX_Stitch_CMD->S_DDR_P3.Data    = st_s_addr >> 5;
            FX_Stitch_CMD->Comm_P3.Data     = img_cmd_addr >> 5;
            FX_Stitch_CMD->Block_Size3.Data = img_sum;
            FX_Stitch_CMD->T_Offset_3.Data  = st_t_offset >> 5;
            FX_Stitch_CMD->YUVC_P3.Data     = ST_YUV_ADDR >> 5;     // output: YUV計算亮度，目前沒用到
            FX_Stitch_CMD->CP_En3.Data      = 0;
            FX_Stitch_CMD->Start_En3.Data   = 1;
            FX_Stitch_CMD->Alpha_DDR_P3.Data   = (FX_ST_TRAN_CMD_ADDR + ST_Header[M_Mode].Start_Idx[ST_H_Img] * 4) >> 5;		//起始位址需要對齊8, 8 * 4byte = 32
            enable = 1;
          }
        }
        if(enable == 0)
          FX_Stitch_CMD->Start_En3.Data   = 0;
    
        //YUV, 色縫合
        LINE_YUV_ADDR = (i == 0)? F0_YUV_LINE_YUV_ADDR: F1_YUV_LINE_YUV_ADDR;
        if(yuv_ddr > 0 && yuv_ddr < 8)                              // 1-8, 順序: 1,2,3,4,5,6,7
            LINE_YUV_ADDR += (yuv_ddr * YUV_LINE_YUV_OFFSET);
        
        enable = 0;
        if(st_step == -1 || st_step == 1){
          int yuvz0 = ((yuvz_en & PIPE_SUBCODE_ST_YUVZ_0) == PIPE_SUBCODE_ST_YUVZ_0)? 1: 0;
          if(ST_Header[ST_LM].Sum[ST_H_YUV][i] != 0 && st_lm_en == 1 && yuvz0 == 1) {
            FX_Stitch_CMD->S_DDR_P2.Data    = st_s_addr_lm >> 5;
            FX_Stitch_CMD->Comm_P2.Data     = (SPI_IO_MP_ST_O_2_P0_DATA + ST_Header[ST_LM].Start_Idx[ST_H_YUV] * 32) >> 5;
            FX_Stitch_CMD->Block_Size2.Data = ST_Header[ST_LM].Sum[ST_H_YUV][i];
            FX_Stitch_CMD->T_Offset_2.Data  = 0;
            FX_Stitch_CMD->YUVC_P2.Data     = LINE_YUV_ADDR >> 5;           // output: 給check_SLT_40point()使用
            FX_Stitch_CMD->CP_En2.Data      = 0;
            FX_Stitch_CMD->Start_En2.Data   = 1;
            FX_Stitch_CMD->Alpha_DDR_P2.Data   = 0;
            enable = 1;
          }
        }
        if(enable == 0)
          FX_Stitch_CMD->Start_En2.Data   = 0;
    
        //Z_V, 空間縫合
        enable = 0;
        if(st_step == -1 || st_step == 1){
          int yuvz1 = ((yuvz_en & PIPE_SUBCODE_ST_YUVZ_1) == PIPE_SUBCODE_ST_YUVZ_1)? 1: 0;
          if(ST_Header[ST_LM].Sum[ST_H_ZV][i] != 0 && st_lm_en == 1 && yuvz1 == 1) {
            FX_Stitch_CMD->S_DDR_P0.Data    = st_s_addr_lm >> 5;
            FX_Stitch_CMD->Comm_P0.Data     = (SPI_IO_MP_ST_O_2_P0_DATA + ST_Header[ST_LM].Start_Idx[ST_H_ZV] * 32) >> 5;
            FX_Stitch_CMD->Block_Size0.Data = ST_Header[ST_LM].Sum[ST_H_ZV][i];
            FX_Stitch_CMD->T_Offset_0.Data  = 0;
            FX_Stitch_CMD->YUVC_P0.Data     = F1_Z_V_LINE_YUV_ADDR >> 5;
            FX_Stitch_CMD->CP_En0.Data      = 0;
            FX_Stitch_CMD->Start_En0.Data   = 1;
            FX_Stitch_CMD->Alpha_DDR_P0.Data   = 0;
            enable = 1;
          }
        }
        if(enable == 0)
          FX_Stitch_CMD->Start_En0.Data   = 0;
    
        //Z_H, 空間縫合
        enable = 0;
        if(st_step == -1 || st_step == 1){
          int yuvz2 = ((yuvz_en & PIPE_SUBCODE_ST_YUVZ_2) == PIPE_SUBCODE_ST_YUVZ_2)? 1: 0;
          if(ST_Header[ST_LM].Sum[ST_H_ZH][i] != 0 && st_lm_en == 1 && yuvz2 == 1) {
            FX_Stitch_CMD->S_DDR_P1.Data    = st_s_addr_lm >> 5;
            FX_Stitch_CMD->Comm_P1.Data     = (SPI_IO_MP_ST_O_2_P0_DATA + ST_Header[ST_LM].Start_Idx[ST_H_ZH] * 32) >> 5;
            FX_Stitch_CMD->Block_Size1.Data = ST_Header[ST_LM].Sum[ST_H_ZH][i];
            FX_Stitch_CMD->T_Offset_1.Data  = 0;
            FX_Stitch_CMD->YUVC_P1.Data     = F1_Z_H_LINE_YUV_ADDR >> 5;
            FX_Stitch_CMD->CP_En1.Data      = 1;
            FX_Stitch_CMD->Start_En1.Data   = 1;
            FX_Stitch_CMD->Alpha_DDR_P1.Data   = 0;
            enable = 1;
          }
        }
        if(enable == 0)
          FX_Stitch_CMD->Start_En1.Data   = 0;
            
        //MASK
        int X_Pre_Mask, Y_Pre_Mask;
        int X_MASK, Y_MASK, X_MASK_Offset;
        int mask_x = 4354, mask_y = 3276;
        Get_ST_Mask_XY(M_Mode, &mask_x, &mask_y);
        switch(M_Mode) {
        case 0:	//12K
        	    X_Pre_Mask = 162;		//148;
        	    Y_Pre_Mask = 132;
        	    break;
        case 1:	//8K
    			X_Pre_Mask = 162;
    			Y_Pre_Mask = 132;
    			break;
        case 2:	//6K
                X_Pre_Mask = 162;
                Y_Pre_Mask = 132;
            	break;
        case 3:	//4K
    			X_Pre_Mask = 140;		//148;
    			Y_Pre_Mask = 132;
    			break;
        case 4:	//3K
    			X_Pre_Mask = 140;
    			Y_Pre_Mask = 132;
    			break;
        case 5:	//2K
                X_Pre_Mask = 140;
                Y_Pre_Mask = 132;
        		break;
        }

        if(big_img == 1) {
    		X_MASK = ( (mask_x + Add_Mask_X) >> 1);
    		Y_MASK = ( (mask_y + Add_Mask_Y) >> 1);

    		X_Pre_Mask += Add_Pre_Mask_X;
    		Y_Pre_Mask += Add_Pre_Mask_Y;

        	X_MASK_Offset = 4608;
        }
        else {
    		X_MASK = (mask_x >> 1);
    		Y_MASK = (mask_y >> 1);

        	X_MASK_Offset = 1536;
        }

        FX_Stitch_CMD->SML_XY_Offset1.Data    = ( (Y_Pre_Mask >> 1) << 16) | ( (X_Pre_Mask + 0 * X_MASK_Offset) >> 1);
        FX_Stitch_CMD->SML_XY_Offset2.Data    = ( (Y_Pre_Mask >> 1) << 16) | ( (X_Pre_Mask + 1 * X_MASK_Offset) >> 1);
        FX_Stitch_CMD->SML_XY_Offset3.Data    = ( (Y_Pre_Mask >> 1) << 16) | ( (X_Pre_Mask + 2 * X_MASK_Offset) >> 1);
    
        FX_Stitch_CMD->S_MASK_1_P3.Data    = (Y_MASK << 16) | X_MASK;
        FX_Stitch_CMD->S_MASK_2_P3.Data    = (Y_MASK << 16) | X_MASK;
        FX_Stitch_CMD->S_MASK_3_P3.Data    = (Y_MASK << 16) | X_MASK;
        FX_Stitch_CMD->S_MASK_1_P2.Data    = (Y_MASK << 16) | X_MASK;
        FX_Stitch_CMD->S_MASK_2_P2.Data    = (Y_MASK << 16) | X_MASK;
        FX_Stitch_CMD->S_MASK_3_P2.Data    = (Y_MASK << 16) | X_MASK;
        FX_Stitch_CMD->S_MASK_1_P0.Data    = (Y_MASK << 16) | X_MASK;
        FX_Stitch_CMD->S_MASK_2_P0.Data    = (Y_MASK << 16) | X_MASK;
        FX_Stitch_CMD->S_MASK_3_P0.Data    = (Y_MASK << 16) | X_MASK;
        FX_Stitch_CMD->S_MASK_1_P1.Data    = (Y_MASK << 16) | X_MASK;
        FX_Stitch_CMD->S_MASK_2_P1.Data    = (Y_MASK << 16) | X_MASK;
        FX_Stitch_CMD->S_MASK_3_P1.Data    = (Y_MASK << 16) | X_MASK;
    }
    
    if(C_Mode == 14 && big_img == 0) {		//3D Model
    	A2K.ST_3DModel_Idx = ((A2K.ST_3DModel_Idx+1)&0x7);
    }
}
extern int *get_ST_Sum_Test_p(void);        // k_test.c
extern int do_FX_ST_Test(int f_id);         // k_test.c
extern int changeST(int f_id, int MT);      // k_test.c
int Make_Debug_Stitch_CMD(int o_page, AS2_F0_MAIN_CMD_struct *Cmd_p)
{
    int DebugJPEGMode = A2K.DebugJPEGMode;
    int DebugJPEGaddr = A2K.DebugJPEGaddr;
    int ISP2_Sensor   = A2K.ISP2_Sensor;
    int Focus_Tool    = A2K.Focus_Tool;
    int Focus_Sensor  = A2K.Focus_Sensor;
    int *ST_Sum_Test  = get_ST_Sum_Test_p();        // k_test.c
	int f_id, MT, debug=0;
	int X_Pre_Mask, Y_Pre_Mask;
	int X_MASK, Y_MASK, X_MASK_Offset;
    int st_s_addr;
    int st_t_offset;
    
	if(DebugJPEGMode == 1 && ISP2_Sensor >= 0 && ISP2_Sensor <= 4) {
		Get_FId_MT(ISP2_Sensor, &f_id, &MT);
		changeST(f_id, MT);

		if(f_id == 0) {
			st_s_addr = ST_STM2_P0_S_ADDR;
			st_t_offset = 0 * 0x200;
			Cmd_p->F0_Stitch_CMD.S_DDR_P3.Data    = st_s_addr >> 5;
			Cmd_p->F0_Stitch_CMD.Comm_P3.Data     = FX_TEST_ST_CMD_ADDR >> 5;
			Cmd_p->F0_Stitch_CMD.Block_Size3.Data = ST_Sum_Test[f_id];
			Cmd_p->F0_Stitch_CMD.T_Offset_3.Data  = st_t_offset;
			Cmd_p->F0_Stitch_CMD.Start_En3.Data   = 1;
			Cmd_p->F1_Stitch_CMD.Start_En3.Data   = 0;
		}
		else {
			st_s_addr = ST_STM2_P0_S_ADDR;
			st_t_offset = 0 * 0x200;
			Cmd_p->F1_Stitch_CMD.S_DDR_P3.Data    = st_s_addr >> 5;
			Cmd_p->F1_Stitch_CMD.Comm_P3.Data     = FX_TEST_ST_CMD_ADDR >> 5;
			Cmd_p->F1_Stitch_CMD.Block_Size3.Data = ST_Sum_Test[f_id];
			Cmd_p->F1_Stitch_CMD.T_Offset_3.Data  = st_t_offset;
			Cmd_p->F1_Stitch_CMD.Start_En3.Data   = 1;
			Cmd_p->F0_Stitch_CMD.Start_En3.Data   = 0;
		}
		debug = 1;
	}
	else if(DebugJPEGMode == 1 && ISP2_Sensor == -2) {

		do_ST_Test_S2();

		st_s_addr = ST_STM2_P0_S_ADDR;
		st_t_offset = 0 * 0x200;
		if(ST_Sum_Test[0] > 0) {
			Cmd_p->F0_Stitch_CMD.S_DDR_P3.Data    = st_s_addr >> 5;
			Cmd_p->F0_Stitch_CMD.Comm_P3.Data     = FX_TEST_ST_CMD_ADDR >> 5;
			Cmd_p->F0_Stitch_CMD.Block_Size3.Data = ST_Sum_Test[0];
			Cmd_p->F0_Stitch_CMD.T_Offset_3.Data  = st_t_offset;
			Cmd_p->F0_Stitch_CMD.Start_En3.Data   = 1;
		}
		else
			Cmd_p->F0_Stitch_CMD.Start_En3.Data = 0;

		if(ST_Sum_Test[1] > 0) {
			Cmd_p->F1_Stitch_CMD.S_DDR_P3.Data    = st_s_addr >> 5;
			Cmd_p->F1_Stitch_CMD.Comm_P3.Data     = FX_TEST_ST_CMD_ADDR >> 5;
			Cmd_p->F1_Stitch_CMD.Block_Size3.Data = ST_Sum_Test[1];
			Cmd_p->F1_Stitch_CMD.T_Offset_3.Data  = st_t_offset;
			Cmd_p->F1_Stitch_CMD.Start_En3.Data   = 1;
		}
		else
			Cmd_p->F1_Stitch_CMD.Start_En3.Data = 0;
		debug = 1;
	}
	else if(DebugJPEGMode == 1 && ISP2_Sensor == -4) {
		do_Focus_ST_Test(Focus_Sensor);
		st_s_addr = ST_STM1_P0_S_ADDR;
		st_t_offset = 0 * 0x200;
		if(ST_Sum_Test[0] > 0) {
			Cmd_p->F0_Stitch_CMD.S_DDR_P3.Data    = st_s_addr >> 5;
			Cmd_p->F0_Stitch_CMD.Comm_P3.Data     = FX_TEST_ST_CMD_ADDR >> 5;
			Cmd_p->F0_Stitch_CMD.Block_Size3.Data = ST_Sum_Test[0];
			Cmd_p->F0_Stitch_CMD.T_Offset_3.Data  = st_t_offset;
			Cmd_p->F0_Stitch_CMD.Start_En3.Data   = 1;
		}
		else
			Cmd_p->F0_Stitch_CMD.Start_En3.Data = 0;

		if(ST_Sum_Test[1] > 0) {
			Cmd_p->F1_Stitch_CMD.S_DDR_P3.Data    = st_s_addr >> 5;
			Cmd_p->F1_Stitch_CMD.Comm_P3.Data     = FX_TEST_ST_CMD_ADDR >> 5;
			Cmd_p->F1_Stitch_CMD.Block_Size3.Data = ST_Sum_Test[1];
			Cmd_p->F1_Stitch_CMD.T_Offset_3.Data  = st_t_offset;
			Cmd_p->F1_Stitch_CMD.Start_En3.Data   = 1;
		}
		else
			Cmd_p->F1_Stitch_CMD.Start_En3.Data = 0;
		debug = 1;
	}
	else if(DebugJPEGMode == 1 && ISP2_Sensor == -6) {
		do_Focus_ST_Test2(Focus_Tool, Focus_Sensor);
		st_s_addr = ST_STM1_P0_S_ADDR;
		st_t_offset = 0 * 0x200;
		if(ST_Sum_Test[0] > 0) {
			Cmd_p->F0_Stitch_CMD.S_DDR_P3.Data    = st_s_addr >> 5;
			Cmd_p->F0_Stitch_CMD.Comm_P3.Data     = FX_TEST_ST_CMD_ADDR >> 5;
			Cmd_p->F0_Stitch_CMD.Block_Size3.Data = ST_Sum_Test[0];
			Cmd_p->F0_Stitch_CMD.T_Offset_3.Data  = st_t_offset;
			Cmd_p->F0_Stitch_CMD.Start_En3.Data   = 1;
		}
		else
			Cmd_p->F0_Stitch_CMD.Start_En3.Data = 0;

		if(ST_Sum_Test[1] > 0) {
			Cmd_p->F1_Stitch_CMD.S_DDR_P3.Data    = st_s_addr >> 5;
			Cmd_p->F1_Stitch_CMD.Comm_P3.Data     = FX_TEST_ST_CMD_ADDR >> 5;
			Cmd_p->F1_Stitch_CMD.Block_Size3.Data = ST_Sum_Test[1];
			Cmd_p->F1_Stitch_CMD.T_Offset_3.Data  = st_t_offset;
			Cmd_p->F1_Stitch_CMD.Start_En3.Data   = 1;
		}
		else
			Cmd_p->F1_Stitch_CMD.Start_En3.Data = 0;
		debug = 1;
	}
	else if(DebugJPEGMode == 1 && ISP2_Sensor == -8) {
		db_debug("Make_Main_Cmd: ST ISP2_Sensor == -8\n");
		f_id = 1;
		do_FX_ST_Test(f_id);
        switch(o_page) {
        case 0: st_t_offset = 0; break;
        case 1: st_t_offset = ST_STM2_P1_T_ADDR - ST_STM2_P0_T_ADDR; break;
        case 2: st_t_offset = ST_STM2_P2_T_ADDR - ST_STM2_P0_T_ADDR; break;
        }
		if(f_id == 0) {
			st_s_addr = DebugJPEGaddr;
			//st_t_offset = o_page * 0x200;
			Cmd_p->F0_Stitch_CMD.S_DDR_P3.Data    = st_s_addr >> 5;
			Cmd_p->F0_Stitch_CMD.Comm_P3.Data     = FX_TEST_ST_CMD_ADDR >> 5;
			Cmd_p->F0_Stitch_CMD.Block_Size3.Data = ST_Sum_Test[f_id];
			Cmd_p->F0_Stitch_CMD.T_Offset_3.Data  = st_t_offset >> 5;
			Cmd_p->F0_Stitch_CMD.Start_En3.Data   = 1;
			Cmd_p->F1_Stitch_CMD.Start_En3.Data   = 0;
		}
		else {
			st_s_addr = DebugJPEGaddr;
			//st_t_offset = o_page * 0x200;
			Cmd_p->F1_Stitch_CMD.S_DDR_P3.Data    = st_s_addr >> 5;
			Cmd_p->F1_Stitch_CMD.Comm_P3.Data     = FX_TEST_ST_CMD_ADDR >> 5;
			Cmd_p->F1_Stitch_CMD.Block_Size3.Data = ST_Sum_Test[f_id];
			Cmd_p->F1_Stitch_CMD.T_Offset_3.Data  = st_t_offset >> 5;
			Cmd_p->F1_Stitch_CMD.Start_En3.Data   = 1;
			Cmd_p->F0_Stitch_CMD.Start_En3.Data   = 0;
		}
		debug = 1;
	}
	else if(DebugJPEGMode == 1 && ISP2_Sensor == -9) {
		X_MASK = 4364;
		Y_MASK = (3282 >> 1);
		X_MASK_Offset = 4608;

		//MAP
		if(ST_Header[2].Sum[ST_H_Img][0] != 0) {
			Cmd_p->F0_Stitch_CMD.S_DDR_P3.Data    = ST_STM1_P0_S_ADDR >> 5;
			Cmd_p->F0_Stitch_CMD.Comm_P3.Data     = (SPI_IO_MP_ST_O_2_P0_DATA + ST_Header[2].Start_Idx[ST_H_Img] * 32/* + idx * 0x80000*/) >> 5;
			Cmd_p->F0_Stitch_CMD.Block_Size3.Data = ST_Header[2].Sum[ST_H_Img][0];
			Cmd_p->F0_Stitch_CMD.T_Offset_3.Data  = 0;
			Cmd_p->F0_Stitch_CMD.YUVC_P3.Data     = F0_ST_YUV_ADDR >> 5;
			Cmd_p->F0_Stitch_CMD.CP_En3.Data      = 0;
			Cmd_p->F0_Stitch_CMD.Start_En3.Data   = 1;
		}
		else
			Cmd_p->F0_Stitch_CMD.Start_En3.Data   = 0;

		if(ST_Header[2].Sum[ST_H_Img][1] != 0) {
			Cmd_p->F1_Stitch_CMD.S_DDR_P3.Data    = ST_STM1_P0_S_ADDR >> 5;
			Cmd_p->F1_Stitch_CMD.Comm_P3.Data     = (SPI_IO_MP_ST_O_2_P0_DATA + ST_Header[2].Start_Idx[ST_H_Img] * 32/* + idx * 0x80000*/) >> 5;
			Cmd_p->F1_Stitch_CMD.Block_Size3.Data = ST_Header[2].Sum[ST_H_Img][1];
			Cmd_p->F1_Stitch_CMD.T_Offset_3.Data  = 0;
			Cmd_p->F1_Stitch_CMD.YUVC_P3.Data     = F1_ST_YUV_ADDR >> 5;
			Cmd_p->F1_Stitch_CMD.CP_En3.Data      = 0;
			Cmd_p->F1_Stitch_CMD.Start_En3.Data   = 1;
		}
		else
			Cmd_p->F1_Stitch_CMD.Start_En3.Data   = 0;

		Cmd_p->F0_Stitch_CMD.S_MASK_1_P3.Data	= (Y_MASK << 16) | ( (X_MASK + 0 * X_MASK_Offset) >> 1);
		Cmd_p->F0_Stitch_CMD.S_MASK_2_P3.Data	= (Y_MASK << 16) | ( (X_MASK + 1 * X_MASK_Offset) >> 1);
		Cmd_p->F0_Stitch_CMD.S_MASK_3_P3.Data	= (Y_MASK << 16) | ( (X_MASK + 2 * X_MASK_Offset) >> 1);

		Cmd_p->F1_Stitch_CMD.S_MASK_1_P3.Data	= (Y_MASK << 16) | ( (X_MASK + 0 * X_MASK_Offset) >> 1);
		Cmd_p->F1_Stitch_CMD.S_MASK_2_P3.Data	= (Y_MASK << 16) | ( (X_MASK + 1 * X_MASK_Offset) >> 1);
		Cmd_p->F1_Stitch_CMD.S_MASK_3_P3.Data	= (Y_MASK << 16) | ( (X_MASK + 2 * X_MASK_Offset) >> 1);
		debug = 1;
	}
    return debug;
}
int Make_ST_Cmd(int idx, AS2_F0_MAIN_CMD_struct *Cmd_p, FPGA_Engine_One_Struct *F_Pipe_p, F_Com_In_Struct *F_Com_p)
{
    int C_Mode = F_Com_p->Record_Mode;
    int M_Mode = F_Pipe_p->M_Mode;
    int i_page = (F_Pipe_p->I_Page % 10);
    int o_page = (F_Pipe_p->O_Page % 10);
    int debug;
    int yuvz_en = -1, ddr_5s = -1, st_step = -1, yuv_ddr = -1, st_i_page = -1;

    if(F_Pipe_p->Time_Buf <= 0) return 0;
    get_F_Temp_Stitch_SubData(idx, &yuvz_en, &ddr_5s, &st_step, &yuv_ddr, &st_i_page);
    
    switch(C_Mode) {
    case 0: // Cap (1p)
    case 3: // AEB (3p,5p,7p)
    case 4: // RAW (5p)
    case 5: // HDR (1p)
    case 6: // Night
    case 7: // Night + HDR
    case 8: // Sport
    case 9: // Sport + WDR
    case 12:// M-Mode
    case 13:// Removal
    case 14:// 3D-Model
        if(st_i_page >= 0) i_page = st_i_page;       // ISP2_P0被ISP1_P4、ISP1_P5使用
        switch(M_Mode) {
            case 0: //STLM = 3;   // 下cmd，使用小圖縫合
            	set_ST_Cmd_Addr(1, C_Mode, M_Mode, 3, i_page, o_page, yuvz_en, ddr_5s, st_step, yuv_ddr, Cmd_p, idx);            // big_img=1
                db_debug("Make_ST_Cmd: idx=%d M_Mode=%d st=%d yuv=%d i=%d o=%d\n", idx, M_Mode, st_step, yuv_ddr, i_page, o_page);
                if(yuv_ddr > 0 && yuv_ddr < 8) start_Line_YUV_Offset_Step(st_step, yuv_ddr, idx);
                break; 
            case 1:
            case 2: //STLM = 4;   // 下cmd，使用小圖縫合
            	set_ST_Cmd_Addr(1, C_Mode, M_Mode, 4, i_page, o_page, yuvz_en, ddr_5s, st_step, yuv_ddr, Cmd_p, idx);            // big_img=1
                db_debug("Make_ST_Cmd: idx=%d M_Mode=%d st=%d yuv=%d i=%d o=%d\n", idx, M_Mode, st_step, yuv_ddr, i_page, o_page);
                if(yuv_ddr > 0 && yuv_ddr < 8) start_Line_YUV_Offset_Step(st_step, yuv_ddr, idx);
                break;
            case 3:
            case 4:
            case 5: set_ST_Cmd_Addr(0, C_Mode, M_Mode, -1, i_page, o_page, yuvz_en, ddr_5s, st_step, yuv_ddr, Cmd_p, idx); break;     // big_img=0
            default: return 0; break;
        }
        break;

    case 1:                //Rec
    case 10:                //Rec + WDR
        switch(M_Mode) {
            case 3:
            case 4:
            case 5: set_ST_Cmd_Addr(0, C_Mode, M_Mode, -1, i_page, o_page, yuvz_en, ddr_5s, st_step, yuv_ddr, Cmd_p, idx); break;
            default: return 0; break;
        }
        break;

    case 2:                //Time Lapse
    case 11:                //Time Lapse + WDR
        switch(M_Mode) {
            case 0:
            case 1:
            case 2: set_ST_Cmd_Addr(1, C_Mode, M_Mode, -1, i_page, o_page, yuvz_en, ddr_5s, st_step, yuv_ddr, Cmd_p, idx); break;
            case 3: set_ST_Cmd_Addr(0, C_Mode, M_Mode, -1, i_page, o_page, yuvz_en, ddr_5s, st_step, yuv_ddr, Cmd_p, idx); break;
            default: return 0; break;
        }
        break;
    }

    debug = Make_Debug_Stitch_CMD(o_page, Cmd_p);
    if(debug ==1) return 1;

    return 1;
}

int Make_Smooth_Cmd(int idx, AS2_F2_MAIN_CMD_struct *Cmd_p, FPGA_Engine_One_Struct *F_Pipe_p, F_Com_In_Struct *F_Com_p)
{
    int C_Mode = F_Com_p->Record_Mode;
    int M_Mode = F_Pipe_p->M_Mode;
    int i_page = (F_Pipe_p->I_Page % 10);
    int o_page = (F_Pipe_p->O_Page % 10);
    int smooth_en = 0;
    int sx, sy;
    unsigned s_addr, t_addr;

    if(F_Pipe_p->Time_Buf <= 0) return 0;
    get_F_Temp_Smooth_SubData(idx, &smooth_en);

    if(smooth_en == 1) {
    	switch(M_Mode) {
    	case 0: sx = S2_RES_12K_WIDTH; sy = S2_RES_12K_HEIGHT; break;
    	case 1: sx = S2_RES_8K_WIDTH;  sy = S2_RES_8K_HEIGHT; break;
    	case 2: sx = S2_RES_6K_WIDTH;  sy = S2_RES_6K_HEIGHT; break;
    	case 3: sx = S2_RES_4K_WIDTH;  sy = S2_RES_4K_HEIGHT; break;
    	case 4: sx = S2_RES_3K_WIDTH;  sy = S2_RES_3K_HEIGHT; break;
    	case 5: sx = S2_RES_2K_WIDTH;  sy = S2_RES_2K_HEIGHT; break;
    	}

    	if(i_page == 0) s_addr = ST_STM1_P0_T_ADDR;
    	else			s_addr = ST_STM1_P1_T_ADDR;

    	if(o_page == 0) t_addr = ST_STM1_P0_T_ADDR;
    	else			t_addr = ST_STM1_P1_T_ADDR;
    }
    else {
    	sx = 0; sy = 0;
    	s_addr = 0;
    	t_addr = 0;
    }

	Cmd_p->SMOOTH_CMD.Size.Data			= ( (sy << 16) | (sx / 192) );
	Cmd_p->SMOOTH_CMD.S_DDR_P.Data   	= (s_addr >> 5);
	Cmd_p->SMOOTH_CMD.T_DDR_P.Data   	= (t_addr >> 5);
	Cmd_p->SMOOTH_CMD.Start_En.Data   	= smooth_en & 0x1;

    cle_F_Temp_Smooth_SubData(idx);

    return 1;
}

int Get_JPEG_Quality_Sel(int Quality)
{
	int sel = 0;
	if(Quality == F2_J_HIGH_QUALITY)        sel = 0;
	else if(Quality == F2_J_MIDDLE_QUALITY) sel = 1;
	else if(Quality == F2_J_LOW_QUALITY)    sel = 2;
	else                                    sel = 3;
	return sel;
}
/*
 * big_img = 1 -> big image (6K~12K) enable
 * s_jpeg = 1 -> fpga compress small jpeg enable
 */
void set_JPEG_Cmd_ADDR(int idx, int big_img, int s_jpeg, int C_Mode, int M_Mode, int i_page, int o_page,
                       int ddr_5s, int hdr_3s, AS2_F2_MAIN_CMD_struct *Cmd_p, int JPEG_Quality, int id,
                       int h_page, int exp_n, int exp_m, int iso, int deGhost_en, int smooth_en, int jpeg_3d_mode,
                       JPEG_HDR_AEB_Info_Struct info, JPEG_UI_Info_Struct ui_info)
{
    int jpeg_s_addr0, jpeg_t_addr0;
    int jpeg_thm_offsetX, jpeg_thm_offsetY;
    int binn = A_L_I3_Header[M_Mode].Binn;
    //int cap_cnt = A2K.cap_file_cnt;
    int jpeg_page = 0;
    int offset = 0;
    AS2_JPEG_CMD_struct *JPEG_p;

    if(id == 0) JPEG_p = &Cmd_p->JPEG0_CMD;
    else        JPEG_p = &Cmd_p->JPEG1_CMD;

    if(big_img == 1){
    	if(ddr_5s >= PIPE_SUBCODE_REMOVAL_CNT_0 && ddr_5s <= PIPE_SUBCODE_REMOVAL_CNT_3){
            if(i_page == 0) jpeg_s_addr0 = JPEG_STM1_P0_S_BUF_ADDR;
            else            jpeg_s_addr0 = JPEG_STM1_P1_S_BUF_ADDR;
        }
        else if(ddr_5s >= PIPE_SUBCODE_5S_CNT_0 && ddr_5s <= PIPE_SUBCODE_5S_CNT_4) {
            if(i_page == 0) jpeg_s_addr0 = JPEG_STM1_P0_S_BUF_ADDR + 64;	//去除前面黑邊
            else            jpeg_s_addr0 = JPEG_STM1_P1_S_BUF_ADDR + 64;
    	}
    	else {
            if(i_page == 0) jpeg_s_addr0 = JPEG_STM1_P0_S_BUF_ADDR;
            else            jpeg_s_addr0 = JPEG_STM1_P1_S_BUF_ADDR;
        }

    	if(id == 0) {
            if(o_page == 0) { jpeg_t_addr0 = JPEG_STM1_P0_T_BUF_ADDR; jpeg_page = 2; }
            else            { jpeg_t_addr0 = JPEG_STM1_P0_T_BUF_ADDR; jpeg_page = 2; }
    	}
    	else {
            if(o_page == 0) { jpeg_t_addr0 = JPEG_STM1_P1_T_BUF_ADDR; jpeg_page = 2; }
            else            { jpeg_t_addr0 = JPEG_STM1_P1_T_BUF_ADDR; jpeg_page = 2; }
    	}

    	//db_debug("set_JPEG_Cmd_ADDR() 00 id=%d ddr_5s=%d s_jpeg=%d jpeg_s_addr0=0x%x jpeg_t_addr0=0x%x jpeg_page=%d\n", id, ddr_5s, s_jpeg, jpeg_s_addr0, jpeg_t_addr0, jpeg_page);
    }
    else{   // big_img == 0 
    	if(C_Mode == 14) {
    		if(jpeg_3d_mode == 0)
    			offset = 0;
    		else
    			offset = ((S2_RES_3D1K_HEIGHT+64) << 15);
    	}
    	else
    		offset = 0;

    	switch(i_page) {
    	case 0: jpeg_s_addr0 = JPEG_STM2_P0_S_BUF_ADDR + offset; break;
    	case 1: jpeg_s_addr0 = JPEG_STM2_P1_S_BUF_ADDR + offset; break;
    	case 2: jpeg_s_addr0 = JPEG_STM2_P2_S_BUF_ADDR + offset; break;
    	}

    	if(id == 0) {
			if(o_page == 0) { jpeg_t_addr0 = JPEG_STM2_P0_T_BUF_ADDR; jpeg_page = 0; }
			else            { jpeg_t_addr0 = JPEG_STM2_P1_T_BUF_ADDR; jpeg_page = 1; }
    	}
    	else {
			if(o_page == 0) { jpeg_t_addr0 = JPEG_STM2_P2_T_BUF_ADDR; jpeg_page = 0; }
			else            { jpeg_t_addr0 = JPEG_STM2_P3_T_BUF_ADDR; jpeg_page = 1; }
    	}
    }

    int jpeg_x_size0, jpeg_y_size0, jpeg_h_addr0;
    if(ddr_5s >= PIPE_SUBCODE_REMOVAL_CNT_0 && ddr_5s <= PIPE_SUBCODE_REMOVAL_CNT_3){
        jpeg_x_size0 = 6912;        //768*3*4;             //S2_RES_12K_WIDTH;     //11520
        jpeg_y_size0 = 864;         //576*2;               //S2_RES_12K_HEIGHT;    // 5760
        jpeg_h_addr0 = JPEG_HEADER_S_ADDR + h_page * 0x800;
    }
    else if(ddr_5s >= PIPE_SUBCODE_5S_CNT_0 && ddr_5s <= PIPE_SUBCODE_5S_CNT_4) {		//set_JPEG_Cmd_ADDR: 5 Sensor
    	switch(binn) {
    	case 1: jpeg_x_size0 = S2_RES_S_FS_WIDTH - 32; jpeg_y_size0 = S2_RES_S_FS_HEIGHT; break;
    	case 2: jpeg_x_size0 = S2_RES_S_D2_WIDTH - 32; jpeg_y_size0 = S2_RES_S_D2_HEIGHT; break;
    	case 3: jpeg_x_size0 = S2_RES_S_D3_WIDTH - 32; jpeg_y_size0 = S2_RES_S_D3_HEIGHT; break;
    	}
		jpeg_h_addr0 = JPEG_HEADER_S_ADDR + h_page * 0x800;
    }
    else{
        switch(M_Mode){
		case 0: jpeg_x_size0 = S2_RES_12K_WIDTH; jpeg_y_size0 = S2_RES_12K_HEIGHT; jpeg_h_addr0 = JPEG_HEADER_12K_ADDR; break;
		case 1: jpeg_x_size0 = S2_RES_8K_WIDTH;  jpeg_y_size0 = S2_RES_8K_HEIGHT;  jpeg_h_addr0 = JPEG_HEADER_8K_ADDR;  break;
		case 2: jpeg_x_size0 = S2_RES_6K_WIDTH;  jpeg_y_size0 = S2_RES_6K_HEIGHT;  jpeg_h_addr0 = JPEG_HEADER_6K_ADDR;  break;
		case 3:
			if(C_Mode == 14) {
				if(jpeg_3d_mode == 0) { jpeg_x_size0 = S2_RES_3D1K_WIDTH;  jpeg_y_size0 = S2_RES_3D1K_HEIGHT; }
				else				  { jpeg_x_size0 = S2_RES_3D4K_WIDTH;  jpeg_y_size0 = S2_RES_3D4K_HEIGHT; }
				jpeg_h_addr0 = JPEG_HEADER_3D_ADDR;
			}
			else {
				jpeg_x_size0 = S2_RES_4K_WIDTH;  jpeg_y_size0 = S2_RES_4K_HEIGHT;
				jpeg_h_addr0 = JPEG_HEADER_4K_ADDR;
			}
			break;
		case 4: jpeg_x_size0 = S2_RES_3K_WIDTH;  jpeg_y_size0 = S2_RES_3K_HEIGHT;  jpeg_h_addr0 = JPEG_HEADER_3K_ADDR;  break;
		case 5: jpeg_x_size0 = S2_RES_2K_WIDTH;  jpeg_y_size0 = S2_RES_2K_HEIGHT;  jpeg_h_addr0 = JPEG_HEADER_2K_ADDR;  break;
        }
        jpeg_h_addr0 += h_page * 0x800;
//        if(hdr_3s > 0 && hdr_3s < 4){   // 1,2,3
//            jpeg_h_addr0 += (0x800*hdr_3s);
//        }
    }

	if(s_jpeg == 1){					//THM
		if(big_img == 1) {		//CAP
			jpeg_thm_offsetX = ( (3840 >> 1) - (S2_RES_THM_WIDTH >> 1) );
			jpeg_thm_offsetY = ( (1920 >> 1) - (S2_RES_THM_HEIGHT >> 1) );
		}
		else {					//REC
			jpeg_thm_offsetX = ( (jpeg_x_size0 >> 1) - (S2_RES_THM_WIDTH >> 1) );
			jpeg_thm_offsetY = ( (jpeg_y_size0 >> 1) - (S2_RES_THM_HEIGHT >> 1) );
		}
        if(i_page == 0) jpeg_s_addr0 = JPEG_STM2_P0_S_BUF_ADDR + jpeg_thm_offsetY * 0x8000 + jpeg_thm_offsetX * 2;
        else            jpeg_s_addr0 = JPEG_STM2_P1_S_BUF_ADDR + jpeg_thm_offsetY * 0x8000 + jpeg_thm_offsetX * 2;

		jpeg_x_size0 = S2_RES_THM_WIDTH;        // S2_RES_THM_WIDTH     1024
		jpeg_y_size0 = S2_RES_THM_HEIGHT;       // S2_RES_THM_HEIGHT    1024
		jpeg_h_addr0 = JPEG_HEADER_THM_ADDR + h_page * 0x800;
    }


    int doQuality = 0;
    if(JPEG_Quality != A2K.JPEG_Quality_lst) {
        A2K.JPEG_Quality_lst = JPEG_Quality;
     	doQuality = 1;
    }
    Set_JPEG_Header(doQuality, JPEG_Quality, jpeg_y_size0, jpeg_x_size0, jpeg_h_addr0, hdr_3s, F_JPEG_Header.Time[h_page].ISP1_Time, C_Mode,
    		exp_n, exp_m, iso, deGhost_en, smooth_en, info, ui_info);

    JPEG_p->R_DDR_ADDR_0.Data      = (jpeg_s_addr0 >> 5);
    JPEG_p->W_DDR_ADDR_0.Data      = (jpeg_t_addr0 >> 5);
    JPEG_p->Y_Size_0.Data          = jpeg_y_size0;
    JPEG_p->X_Size_0.Data          = jpeg_x_size0;
    JPEG_p->H_B_Addr_0.Data        = (jpeg_h_addr0 >> 5);
    JPEG_p->Hder_Size_0.Data       = sizeof(J_Hder_Struct);
    JPEG_p->H_B_Size_0.Data        = sizeof(J_Hder_Struct) << 3;
    JPEG_p->Page_sel_0.Data        = jpeg_page & 0x3;
    JPEG_p->Q_table_sel_0.Data     = Get_JPEG_Quality_Sel(JPEG_Quality);
    if(id == 0) JPEG_p->Start_En_0.Address     = 0xCCAA0186;
    else        JPEG_p->Start_En_0.Address     = 0xCCAA01A6;
    JPEG_p->Start_En_0.Data        = 1;                    // 1->enable
}
/*
 * return: 1 -> execute
 */
int Make_Debug_JPEG_Cmd(int o_page, AS2_F2_MAIN_CMD_struct *Cmd_p, int JPEG_Quality, int id, int h_page,
		int exp_n, int exp_m, int iso, int deGhost_en, int smooth_en, JPEG_HDR_AEB_Info_Struct info, JPEG_UI_Info_Struct ui_info)
{
    int DebugJPEGMode = A2K.DebugJPEGMode;
    int DebugJPEGaddr = A2K.DebugJPEGaddr;
    int ISP2_Sensor   = A2K.ISP2_Sensor;
    int jpeg_h_addr;
    AS2_JPEG_CMD_struct *JPEG_p;

    if(id == 0) JPEG_p = &Cmd_p->JPEG0_CMD;
    else        JPEG_p = &Cmd_p->JPEG1_CMD;

    if(DebugJPEGMode == 1) {
        if(ISP2_Sensor == -99) {
        	JPEG_p->R_DDR_ADDR_0.Data = DebugJPEGaddr;
        	JPEG_p->X_Size_0.Data = 1920;
        	JPEG_p->Y_Size_0.Data = 1080;
        }
        else if(ISP2_Sensor >= 0 && ISP2_Sensor <= 4) {
        	JPEG_p->R_DDR_ADDR_0.Data = (JPEG_STM2_P0_S_BUF_ADDR >> 5);
            if(o_page == 0)
            	JPEG_p->W_DDR_ADDR_0.Data = (JPEG_STM2_P0_T_BUF_ADDR >> 5);
            else
            	JPEG_p->W_DDR_ADDR_0.Data = (JPEG_STM2_P1_T_BUF_ADDR >> 5);
            JPEG_p->X_Size_0.Data     = 1536;
            JPEG_p->Y_Size_0.Data     = 1152;
        }
        else if(ISP2_Sensor == -2) {
        	JPEG_p->R_DDR_ADDR_0.Data = (JPEG_STM2_P0_S_BUF_ADDR >> 5);
            if(o_page == 0)
            	JPEG_p->W_DDR_ADDR_0.Data = (JPEG_STM2_P0_T_BUF_ADDR >> 5);
            else
            	JPEG_p->W_DDR_ADDR_0.Data = (JPEG_STM2_P1_T_BUF_ADDR >> 5);
            JPEG_p->X_Size_0.Data     = 1920;
            JPEG_p->Y_Size_0.Data     = 1024;
        }
        else if(ISP2_Sensor == -4) {
        	JPEG_p->R_DDR_ADDR_0.Data = (JPEG_STM1_P0_S_BUF_ADDR >> 5);
            if(o_page == 0)
            	JPEG_p->W_DDR_ADDR_0.Data = (JPEG_STM2_P0_T_BUF_ADDR >> 5);
            else
            	JPEG_p->W_DDR_ADDR_0.Data = (JPEG_STM2_P1_T_BUF_ADDR >> 5);
            JPEG_p->X_Size_0.Data     = 1920;
            JPEG_p->Y_Size_0.Data     = 960;
        }
        else if(ISP2_Sensor == -6) {
        	JPEG_p->R_DDR_ADDR_0.Data = (JPEG_STM1_P0_S_BUF_ADDR >> 5);
            if(o_page == 0)
            	JPEG_p->W_DDR_ADDR_0.Data = (JPEG_STM2_P0_T_BUF_ADDR >> 5);
            else
            	JPEG_p->W_DDR_ADDR_0.Data = (JPEG_STM2_P1_T_BUF_ADDR >> 5);
            JPEG_p->X_Size_0.Data     = 1920;
            JPEG_p->Y_Size_0.Data     = 1024;

            JPEG_Quality = F2_J_HIGH_QUALITY;
        }
        else if(ISP2_Sensor == -8) {
        	JPEG_p->X_Size_0.Data = 1536;
        	JPEG_p->Y_Size_0.Data = 1152;
        }
        else if(ISP2_Sensor == -9) {
        	JPEG_p->R_DDR_ADDR_0.Data = (DebugJPEGaddr >> 5);
            if(o_page == 0)
            	JPEG_p->W_DDR_ADDR_0.Data = (JPEG_STM1_P0_T_BUF_ADDR >> 5);
            else
            	JPEG_p->W_DDR_ADDR_0.Data = (JPEG_STM1_P1_T_BUF_ADDR >> 5);
            JPEG_p->X_Size_0.Data = 1920;
            JPEG_p->Y_Size_0.Data = 1080;
        }

        int doQuality = 0;
        if(JPEG_Quality != A2K.JPEG_Quality_lst) {
            A2K.JPEG_Quality_lst = JPEG_Quality;
            doQuality = 1;
        }
        jpeg_h_addr = JPEG_HEADER_TEST_ADDR + h_page * 0x800;
        JPEG_p->H_B_Addr_0.Data = (jpeg_h_addr >> 5);
        JPEG_p->Q_table_sel_0.Data = Get_JPEG_Quality_Sel(JPEG_Quality);
        Set_JPEG_Header(doQuality, JPEG_Quality, JPEG_p->Y_Size_0.Data, JPEG_p->X_Size_0.Data, jpeg_h_addr, -1, 0, 0,
        		exp_n, exp_m, iso, deGhost_en, smooth_en, info, ui_info);

        return 1;
    }
    return 0;
}
int Make_JPEG_Cmd(int idx, AS2_F2_MAIN_CMD_struct *Cmd_p, FPGA_Engine_One_Struct *F_Pipe_p, F_Com_In_Struct *F_Com_p, int jpeg_eng)
{
    int C_Mode = F_Com_p->Record_Mode;
    int M_Mode = F_Pipe_p->M_Mode;
    int i_page = (F_Pipe_p->I_Page % 10);
    int o_page = (F_Pipe_p->O_Page % 10);
    int s_jpeg, debug;
    int gain = AEG_gain_H, Quality = F2_J_MIDDLE_QUALITY;
    int cap_thm=-1, ddr_5s=-1, hdr_3s=-1, h_page;
    int Quality_Mode = A2K.JPEG_Quality_Mode;
    int Live_Quality = A2K.JPEG_Live_Quality_Mode;
    int exp_n, exp_m, iso;
    int deGhost_en;
    int cap_smooth_en = F_Com_p->Smooth_En;
    int jpeg_3d_mode = A2K.JPEG_3D_Res_Mode;
    JPEG_HDR_AEB_Info_Struct hdr_aeb_info;
    JPEG_UI_Info_Struct ui_info;

    if(F_Pipe_p->Time_Buf <= 0) return 0; 
    get_F_Temp_Jpeg_SubData(idx, jpeg_eng, &cap_thm, &ddr_5s, &hdr_3s, &h_page, &exp_n, &exp_m, &iso);
    Set_HDR_AEB_Info(&hdr_aeb_info, 1, 0, C_Mode);

    ui_info.wb_mode    = A2K.JPEG_WB_Mode;
    ui_info.color      = A2K.JPEG_Color;
    ui_info.tint       = A2K.JPEG_Tint;
    ui_info.saturation = A2K.JPEG_Saturation_C;
    ui_info.contrast   = A2K.JPEG_Contrast;
    ui_info.tone       = A2K.JPEG_Tone;
    ui_info.sharpness  = A2K.JPEG_Sharpness;
    ui_info.smooth     = A2K.JPEG_Smooth_Auto;

    deGhost_en = Get_DeGhost_En(C_Mode);

    switch(C_Mode) {
    case 0: // Cap (1p)
    case 3: // AEB (3p,5p,7p)
    case 4: // RAW (5p)
    case 5: // HDR (1p)
    case 6: // Night
    case 7: // Night + HDR
    case 8: // Sport
    case 9: // Sport + WDR
    case 12:// M-Mode
    case 13:// Removal
    case 14:// 3D-Model
        switch(M_Mode) {
        case 0:
        case 1:
        case 2: 
            if(cap_thm == PIPE_SUBCODE_CAP_THM) s_jpeg = 1;         // make jpeg small size
            else                                s_jpeg = 0;
            if(C_Mode == 13){
                if(ddr_5s == PIPE_SUBCODE_REMOVAL_CNT_0){
                    Quality = 95;
                    i_page = 1;
                }
                // rex+ 190527, debug removal
                if(i_page == 0) debug_Removal_D_Table(JPEG_STM1_P0_S_BUF_ADDR+2*576*0x8000);
                else            debug_Removal_D_Table(JPEG_STM1_P1_S_BUF_ADDR+2*576*0x8000);
            }
            else if(gain >= 100*64) Quality = F2_J_LOW_QUALITY;		// >= ISO-3200
            else if(gain >= 40*64)  Quality = F2_J_MIDDLE_QUALITY;	// >= ISO-400
            else {
            	if(Check_Is_HDR_Mode(C_Mode) == 1) Quality = F2_J_HIGH_QUALITY;			//HDR
            	else							   Quality = F2_J_MIDDLE_QUALITY_2;		//WDR
            }

            set_JPEG_Cmd_ADDR(idx, 1, s_jpeg, C_Mode, M_Mode, i_page, o_page, ddr_5s, hdr_3s, Cmd_p, Quality, jpeg_eng, h_page,
            		exp_n, exp_m, iso, deGhost_en, cap_smooth_en, jpeg_3d_mode, hdr_aeb_info, ui_info);        // big_img + small_jpg

            db_debug("Make_JPEG_Cmd: idx=%d M_Mode=%d jpeg_eng=%d i_page=%d o_page=%d gain=%d cap_thm=%d ddr_5s=%d hdr_3s=%d Quality=%d\n",
            		idx, M_Mode, jpeg_eng, i_page, o_page, gain, cap_thm, ddr_5s, hdr_3s, Quality);
            break;
        case 3:
        case 4:
        case 5:
        	if(Live_Quality == 1) Quality = F2_J_LOW_QUALITY;
            set_JPEG_Cmd_ADDR(idx, 0, 0, C_Mode, M_Mode, i_page, o_page, ddr_5s, hdr_3s, Cmd_p, Quality, jpeg_eng, h_page,
            		exp_n, exp_m, iso, deGhost_en, cap_smooth_en, jpeg_3d_mode, hdr_aeb_info, ui_info);
            break;
        }
        break;
    case 1:				//Rec
    case 10:			//Rec + WDR
        switch(M_Mode) {
        case 3:
        case 4:
        case 5:
            if(cap_thm == PIPE_SUBCODE_CAP_THM) s_jpeg = 1;         // make jpeg small size
            else                                s_jpeg = 0;
            if(gain >= 100*64) Quality = F2_J_LOW_QUALITY;		// >= ISO-3200
            else     		   Quality = F2_J_MIDDLE_QUALITY;
            set_JPEG_Cmd_ADDR(idx, 0, s_jpeg, C_Mode, M_Mode, i_page, o_page, ddr_5s, hdr_3s, Cmd_p, Quality, jpeg_eng, h_page,
            		exp_n, exp_m, iso, deGhost_en, cap_smooth_en, jpeg_3d_mode, hdr_aeb_info, ui_info);
            break;
        }
        break;
    case 2:				//Time Lapse
    case 11:			//Time Lapse + WDR
        switch(M_Mode) {
        case 0:
        case 1:
        case 2:
                if     (gain >= 100*64) Quality = F2_J_LOW_QUALITY;		// >= ISO-3200
                else if(gain >= 40*64)  Quality = F2_J_MIDDLE_QUALITY;	// >= ISO-400
                else {
                    if(A2K.Time_Lapse >= 2) Quality = F2_J_HIGH_QUALITY;
                    else                    Quality = F2_J_MIDDLE_QUALITY;
                }
                set_JPEG_Cmd_ADDR(idx, 1, 0, C_Mode, M_Mode, i_page, o_page, ddr_5s, hdr_3s, Cmd_p, Quality, jpeg_eng, h_page,
                		exp_n, exp_m, iso, deGhost_en, cap_smooth_en, jpeg_3d_mode, hdr_aeb_info, ui_info);
                break;
        case 3:
        case 4:
            	set_JPEG_Cmd_ADDR(idx, 0, 0, C_Mode, M_Mode, i_page, o_page, ddr_5s, hdr_3s, Cmd_p, Quality, jpeg_eng, h_page,
            			exp_n, exp_m, iso, deGhost_en, cap_smooth_en, jpeg_3d_mode, hdr_aeb_info, ui_info);
                break;
        }
        break;
    }

    debug = Make_Debug_JPEG_Cmd(o_page, Cmd_p, Quality, jpeg_eng, h_page, exp_n, exp_m, iso, deGhost_en, cap_smooth_en, hdr_aeb_info, ui_info);
    if(debug == 1) return 1;

    return 1;
}

//decode
unsigned MQ_Table[6][3] = {
  {13107, 8066, 5243},
  {11916, 7490, 4660},
  {10082, 6554, 4194},
  { 9362, 5825, 3647},
  { 8192, 5243, 3355},
  { 7282, 4559, 2893}
};

//encode
unsigned DeMQ_Table[6][3] = {
  {10, 13, 16},
  {11, 14, 18},
  {13, 16, 20},
  {14, 18, 23},
  {16, 20, 25},
  {18, 23, 29}
};

void set_H264_Cmd_ADDR(int idx, int big_img, int M_Mode, int C_Mode, int i_page, int o_page,
                       AS2_F2_MAIN_CMD_struct *Cmd_p, int IP_M, int Frame_Stamp, int F_Cnt)
{
	int i;
    int size_x, size_y;
    int Quality = 29, Quality_M = Quality % 6;		//20 24 28
    int S_Addr, T_Addr, M_Addr;
    int Sel;
    unsigned Time_Stamp = 0;
    unsigned char Head_Buf[128], *head_p;
    int size_bit=0;

    switch(M_Mode){
	case 0: size_x = S2_RES_12K_WIDTH; size_y = S2_RES_12K_HEIGHT; break;
	case 1: size_x = S2_RES_8K_WIDTH;  size_y = S2_RES_8K_HEIGHT;  break;
	case 2: size_x = S2_RES_6K_WIDTH;  size_y = S2_RES_6K_HEIGHT;  break;
	case 3: size_x = S2_RES_4K_WIDTH;  size_y = S2_RES_4K_HEIGHT;  break;
	case 4: size_x = S2_RES_3K_WIDTH;  size_y = S2_RES_3K_HEIGHT;  break;
	case 5: size_x = S2_RES_2K_WIDTH;  size_y = S2_RES_2K_HEIGHT;  break;
    }

    if(big_img == 1) {
        if(i_page == 0) S_Addr = H264_STM1_P0_S_BUF_ADDR;
        else            S_Addr = H264_STM1_P1_S_BUF_ADDR;

		if(o_page == 0) {
			T_Addr = H264_STM1_P0_T_BUF_ADDR;
			M_Addr = H264_STM1_P0_M_BUF_ADDR;
		}
		else {
			T_Addr = H264_STM1_P1_T_BUF_ADDR;
			M_Addr = H264_STM1_P0_M_BUF_ADDR;
		}
    }
    else{   // big_img == 0
        if(i_page == 0) S_Addr = H264_STM2_P0_S_BUF_ADDR;
        else            S_Addr = H264_STM2_P1_S_BUF_ADDR;

		if(o_page == 0) {
			T_Addr = H264_STM2_P0_T_BUF_ADDR;
			M_Addr = H264_STM1_P0_M_BUF_ADDR;
		}
		else {
			T_Addr = H264_STM2_P1_T_BUF_ADDR;
			M_Addr = H264_STM1_P0_M_BUF_ADDR;
		}
    }
    Sel = o_page;

	Cmd_p->H264_CMD.S_Addr.Data        			= (S_Addr >> 5);
	Cmd_p->H264_CMD.SM_Addr.Data       			= (M_Addr >> 5);
	Cmd_p->H264_CMD.T_Addr.Data        			= (T_Addr >> 5);
	Cmd_p->H264_CMD.TM_Addr.Data       			= (M_Addr >> 5);
	Cmd_p->H264_CMD.size_v.Data        			= (size_y >> 4);					//size_y*2/32
	Cmd_p->H264_CMD.size_h.Data        			= (size_x >> 4);					//size_x*2/32

	Cmd_p->H264_CMD.QSL.Data           			= Quality / 6;
	Cmd_p->H264_CMD.QML0.Data          			= MQ_Table[Quality_M][0];
	Cmd_p->H264_CMD.QML1.Data          			= MQ_Table[Quality_M][1];
	Cmd_p->H264_CMD.QML2.Data         			= MQ_Table[Quality_M][2];
	Cmd_p->H264_CMD.DeQML0.Data        			= DeMQ_Table[Quality_M][0];
	Cmd_p->H264_CMD.DeQML1.Data       			= DeMQ_Table[Quality_M][1];
	Cmd_p->H264_CMD.DeQML2.Data        			= DeMQ_Table[Quality_M][2];

	Cmd_p->H264_CMD.Size_Y.Data        			= (size_y >> 4);					//size_y*2/32
	Cmd_p->H264_CMD.Size_X.Data        			= (size_x >> 4);					//size_x*2/32
	Cmd_p->H264_CMD.I_P_Mode.Data      			= IP_M & 0xFF;							//0:I_Frame 1:P_Frame
	Cmd_p->H264_CMD.USB_Buffer_Sel.Data			= Sel;
	Cmd_p->H264_CMD.H264_en.Data       			= 0x0001;


	//make header
	memset(&Head_Buf, 0, sizeof(Head_Buf) );
	size_bit = MakeH264Header(&Head_Buf[0], IP_M, F_Cnt, Quality, size_x, size_y,
			Time_Stamp, Frame_Stamp, M_Mode, C_Mode);
	for(i = 0; i < (size_bit/8); i++) {
		Cmd_p->H264_CMD.Add_Byte[i].Address = 0xCCAA8018;
		Cmd_p->H264_CMD.Add_Byte[i].Data 	= (unsigned int)Head_Buf[i];
	}
	if( (size_bit%8) == 0) {
		Cmd_p->H264_CMD.Add_Last_Byte.Address = 0x0000;
		Cmd_p->H264_CMD.Add_Last_Byte.Data 	  = 0;
	}
	else {
		Cmd_p->H264_CMD.Add_Last_Byte.Address = 0xCCAA8019;
		Cmd_p->H264_CMD.Add_Last_Byte.Data    = (unsigned int)( (size_bit%8)<<8) + Head_Buf[i];
	}
}
int Make_H264_Cmd(int idx, AS2_F2_MAIN_CMD_struct *Cmd_p, FPGA_Engine_One_Struct *F_Pipe_p, F_Com_In_Struct *F_Com_p)
{
    int C_Mode = F_Com_p->Record_Mode;
    int M_Mode = F_Pipe_p->M_Mode;
    int i_page = (F_Pipe_p->I_Page % 10);
    int o_page = (F_Pipe_p->O_Page % 10);
    int do_h264 = 0, ip_m = 0, fs = 0, fc = 0;

    if(F_Pipe_p->Time_Buf <= 0) return 0;

    get_F_Temp_H264_SubData(idx, &do_h264, &ip_m, &fs, &fc);
    if(do_h264 == 0) return 0;

    switch(C_Mode) {
	default:// 0:Cap  1:Rec  3:AEB  4:RAW  5:HDR  6:Night  7:N-HDR
        	// 8:Sport  9:Sport+WDR  10:Rec+WDR  12:M-Mode  13:Removal
        	return 0;
        	break;
	case 2:				//Timelapse
	case 11:				//Timelapse + WDR
		switch(M_Mode) {
		case 0:
		case 1:
		case 2:
			set_H264_Cmd_ADDR(idx, 1, M_Mode, C_Mode, i_page, o_page, Cmd_p, ip_m, fs, fc);
			break;
		case 3:
		case 4:
			set_H264_Cmd_ADDR(idx, 0, M_Mode, C_Mode, i_page, o_page, Cmd_p, ip_m, fs, fc);
			break;
		}
		break;
	}

    //debug = Make_Debug_JPEG_Cmd(o_page, Cmd_p, Quality);
    //if(debug == 1) return 1;

    return 1;
}

/*
 * tar_lx: max=3840
 * M:1~15
 * N:1~16
 * Anti: 0:正 1:反
 */
void Make_DMA_Scale(Com_Video_struct *P1,unsigned src_lx, unsigned src_ly, unsigned tar_lx, unsigned tar_ly, int M, int N, int Anti)
{
    int max_scale_x;
    max_scale_x = (M << 12) / N;
    P1->Com_Size    = (tar_ly << 8) | (tar_lx >> 4);    // (240 << 7) | (640 >> 4)
    P1->Com_Size_H  = (tar_ly >> 10);                   // tar_y最高位
    P1->Com_I_M     = M;                        // (VIN_WIDTH + 63) >> 6
    P1->Com_O_N     = N;                        // (640 + 63) >> 6    // 應該是跟 tar_lx * P1->Com_I_M/src_lx 的結果一樣
    //--- Video -> Video mode
    P1->Com_Data_Video  = 1;                    // 1
    P1->Com_S_IM_DDR    = 1;
    P1->Com_XI_Size_H   = src_lx >> 12;         // 1
    //---
    P1->Com_XI_Size = src_lx >> 4;              // VIN_WIDTH >> 4
    P1->Com_Check   = 0x15;                     // 0x15
    P1->Com_X_Step  = max_scale_x;              // (P1.Com_I_M << 12)/P1.Com_O_N
    P1->Com_Y_Step  = (src_ly << 8) / tar_ly;   // (480 << 12) / 240
    P1->Anti_Flag   = Anti & 0x1;
};
void Make_Bottom_DMA_Cmd(Com_Video_struct *TP, int M_Mode, int i_page, int o_page, int btm_m, int btm_s, int cp_mode, int btm_t_m, int btm_idx)
{
	int i, idx, cnt;
    int dma_s_addr, dma_t_addr;
    int dma_s_addr_base, dma_t_addr_base;
    int dma_s_x, dma_s_y;
    int dma_t_x, dma_t_y, dma_t_y2;
    int dma_m, dma_n;
    int dma_cnt, x_tmp, y_tmp, addr_y_offset;
    int dma_anti;
    int dma_text_y = 140;	//外圈文字佔多少高度(pix), MAX: BOTTOM_T_HEIGHT
    int btm_width, btm_height;
    int btm_addr, btm_addr_2;
    int t_offset, s_offset;

    if(btm_m <= 0 && btm_t_m <= 0) return;

    switch(M_Mode) {
    case 0: x_tmp = S2_RES_12K_WIDTH; y_tmp = S2_RES_12K_HEIGHT;
    		btm_width  = BOTTOM_T_WIDTH; btm_height = BOTTOM_T_HEIGHT;
    		dma_text_y = 140;											//3840*512, text_height=140
			break;
    case 1: x_tmp = S2_RES_8K_WIDTH;  y_tmp = S2_RES_8K_HEIGHT;
    		btm_width  = BOTTOM_T_WIDTH; btm_height = BOTTOM_T_HEIGHT;
    		dma_text_y = 140;
    		break;
    case 2: x_tmp = S2_RES_6K_WIDTH;  y_tmp = S2_RES_6K_HEIGHT;
    		btm_width  = BOTTOM_T_WIDTH; btm_height = BOTTOM_T_HEIGHT;
    		dma_text_y = 140;
    		break;
    case 3: x_tmp = S2_RES_4K_WIDTH;  y_tmp = S2_RES_4K_HEIGHT;
    		btm_width  = BOTTOM_T_WIDTH; btm_height = BOTTOM_T_HEIGHT;
    		dma_text_y = 140;
       		break;
    case 4: x_tmp = S2_RES_3K_WIDTH;  y_tmp = S2_RES_3K_HEIGHT;
    		btm_width  = BOTTOM_T_WIDTH_2; btm_height = BOTTOM_T_HEIGHT_2;
    		dma_text_y = 35;											//1024*128, text_height=28
       		break;
    case 5: x_tmp = S2_RES_2K_WIDTH;  y_tmp = S2_RES_2K_HEIGHT;
    		btm_width  = BOTTOM_T_WIDTH_2; btm_height = BOTTOM_T_HEIGHT_2;
    		dma_text_y = 35;
       	    break;
    }
    dma_cnt = (x_tmp + 3839) / 3840;			//超過3840需要分段DMA
    if(btm_idx == 4)	  cnt = 2;				//底圖和文字一起DMA
    else if(btm_idx == 5) cnt = 4;				//8K 6K分段後(dma_cnt=2), 底圖和文字一起DMA
    else				  cnt = dma_cnt;		//寬度超過3840需分段DMA

	if(M_Mode <= 2) {
		dma_s_addr_base = (i_page == 0)? ST_STM1_P0_T_ADDR : ST_STM1_P1_T_ADDR;
		dma_t_addr_base = (o_page == 0)? ST_STM1_P0_T_ADDR : ST_STM1_P1_T_ADDR;
	}
	else {
		switch(i_page) {
		case 0: dma_s_addr_base = ST_STM2_P0_T_ADDR; break;
		case 1: dma_s_addr_base = ST_STM2_P1_T_ADDR; break;
		case 2: dma_s_addr_base = ST_STM2_P2_T_ADDR; break;
		}
		switch(o_page) {
		case 0: dma_t_addr_base = ST_STM2_P0_T_ADDR; break;
		case 1: dma_t_addr_base = ST_STM2_P1_T_ADDR; break;
		case 2: dma_t_addr_base = ST_STM2_P2_T_ADDR; break;
		}
	}

	if(M_Mode <= 3) {			// <= 4K, 使用大張底圖(3840*512)
		btm_addr   = DMA_BOTTOM_IMG_BUF_ADDR;
		btm_addr_2 = DMA_BOTTOM_IMG_BUF_2_ADDR;
	}
	else {
		btm_addr   = DMA_BOTTOM_IMG_BUF_3_ADDR;
		btm_addr_2 = DMA_BOTTOM_IMG_BUF_4_ADDR;
	}

    for(i = 0; i < cnt; i++) {
		if(btm_idx == 4) 	  idx = i;			//Live 合併 DMA
		else if(btm_idx == 5) idx = (i >> 1);	//H264 8K 6K合併DMA
		else				  idx = btm_idx;
		switch(M_Mode) {
		case 0:
    		switch(btm_m) {
    		case 1:
				case 3: if(idx == 0) { dma_m = 12; dma_n = 12; }
						else         { dma_m =  4; dma_n = 12; }		//BOTTOM_T_WIDTH / S2_RES_12K_WIDTH
    			break;
    		case 0:
    		case 2:
				case 4: dma_m = 4; dma_n = 12; break;					//BOTTOM_T_WIDTH / S2_RES_12K_WIDTH
    		}
			break;
		case 1:
    		switch(btm_m) {
    		case 1:
				case 3: if(idx == 0) { dma_m = 12; dma_n = 12; }
    			else             { dma_m =  5; dma_n = 10; }
    			break;
    		case 0:
    		case 2:
    		case 4: dma_m = 5; dma_n = 10; break;
    		}
    		break;
		case 2:
    		switch(btm_m) {
    		case 1:
				case 3: if(idx == 0) { dma_m = 12; dma_n = 12; }
    			else             { dma_m =  5; dma_n =  8; }
    			break;
    		case 0:
    		case 2:
    		case 4: dma_m = 5; dma_n = 8; break;
    		}
    		break;
		case 3:
			switch(btm_m) {
			case 1:
			case 3: if(idx == 0) { dma_m = 12; dma_n = 12; }
					else         { dma_m =  8; dma_n =  8; }			//BOTTOM_T_WIDTH / S2_RES_4K_WIDTH
					break;
			case 0:
			case 2:
			case 4: dma_m = 8; dma_n = 8; break;
			}
			break;
		case 4:
			switch(btm_m) {
			case 1:
			case 3: if(idx == 0) { dma_m = 12; dma_n = 12; }
					else         { dma_m =  4; dma_n = 12; }			//BOTTOM_T_WIDTH_2 / S2_RES_3K_WIDTH
					break;
			case 0:
			case 2:
			case 4: dma_m = 4; dma_n = 12; break;
			}
			break;
		case 5:
			switch(btm_m) {
			case 1:
			case 3: if(idx == 0) { dma_m = 12; dma_n = 12; }
					else         { dma_m =  4; dma_n =  8; }			//BOTTOM_T_WIDTH_2 / S2_RES_2K_WIDTH
					break;
			case 0:
			case 2:
			case 4: dma_m = 4; dma_n = 8; break;
			}
			break;
		}

		dma_t_y = y_tmp * 24 * btm_s / 180 / 100;		//依據 btm_s 計算底圖高度
		switch(btm_m) {
		case 0:
			//疊外圈文字
				dma_s_x = btm_width / dma_cnt;
			dma_s_y = dma_text_y;
				dma_t_y2 = dma_t_y * dma_text_y / btm_height;
			dma_t_x = x_tmp / dma_cnt;
			if(cp_mode == 0) addr_y_offset = (y_tmp - dma_t_y) * 0x8000;
			else             addr_y_offset = dma_t_y * 0x8000;
			dma_anti = cp_mode;
			break;
		case 1:		//延展
			if(idx == 0) {
				dma_s_x = x_tmp / dma_cnt;
				dma_s_y = 1;
				dma_t_y2 = dma_t_y;
			}
			else {			//疊外圈文字
					dma_s_x = btm_width / dma_cnt;
				dma_s_y = dma_text_y;
					dma_t_y2 = dma_t_y * dma_text_y / btm_height;
			}
			dma_t_x = x_tmp / dma_cnt;
			if(cp_mode == 0) addr_y_offset = (y_tmp - dma_t_y) * 0x8000;
			else             addr_y_offset = dma_t_y * 0x8000;
			dma_anti = cp_mode;
			break;
		case 2:		//底圖(default)
			dma_s_x = btm_width / dma_cnt;
			if(idx == 0)  {
				dma_s_y = btm_height;
				dma_t_y2 = dma_t_y;
			}
			else {			//疊外圈文字
				dma_s_y = dma_text_y;
				dma_t_y2 = dma_t_y * dma_text_y / btm_height;
			}
			dma_t_x = x_tmp / dma_cnt;
			if(cp_mode == 0) addr_y_offset = (y_tmp - dma_t_y) * 0x8000;
			else             addr_y_offset = dma_t_y * 0x8000;
			dma_anti = cp_mode;
			break;
		case 4:		//底圖(user)
			dma_s_x = btm_width / dma_cnt;
			if(btm_t_m == 1) {		//有文字模式, 整張DMA
				dma_s_y = btm_height;
				dma_t_y2 = dma_t_y;
			}
			else {					//無文字模式, 只DMA中心圖案部分
				dma_s_y = btm_height - dma_text_y;
				dma_t_y2 = dma_t_y;
			}
			dma_t_x = x_tmp / dma_cnt;
			if(cp_mode == 0) addr_y_offset = (y_tmp - dma_t_y) * 0x8000;
			else             addr_y_offset = dma_t_y * 0x8000;
			dma_anti = cp_mode;
			break;
		case 3:		//鏡像
			if(idx == 0) {
				dma_s_x = x_tmp / dma_cnt;
				dma_s_y = y_tmp - dma_t_y;
				dma_t_y2 = dma_t_y;
				if(cp_mode == 0) addr_y_offset = y_tmp * 0x8000;
				else             addr_y_offset = dma_t_y * 0x8000;
				dma_anti = 1;
			}
			else {			//疊外圈文字
				dma_s_x = btm_width / dma_cnt;
				dma_s_y = dma_text_y;
				dma_t_y2 = dma_t_y * dma_text_y / btm_height;
				if(cp_mode == 0) addr_y_offset = (y_tmp - dma_t_y) * 0x8000;
				else             addr_y_offset = dma_t_y * 0x8000;
				dma_anti = cp_mode;
			}
			dma_t_x = x_tmp / dma_cnt;
			break;
		}

		if(btm_idx == 4) {
			s_offset = 0;
			t_offset = 0;
		}
		else if(btm_idx == 5) {
			s_offset = (i%2) * dma_s_x * 2;
			t_offset = (i%2) * dma_t_x * 2;
		}
		else {
			s_offset = i * dma_s_x * 2;
			t_offset = i * dma_t_x * 2;
		}
		switch(btm_m) {
		case 0: dma_s_addr = btm_addr + s_offset; break;
		case 1:
			if(idx == 0) {
				if(cp_mode == 0) dma_s_addr = dma_s_addr_base + s_offset + addr_y_offset;
				else			 dma_s_addr = dma_s_addr_base + s_offset + dma_t_y * 0x8000;
			}
			else
				dma_s_addr = btm_addr + s_offset;
			break;
		case 2:
			if(idx == 0) dma_s_addr = btm_addr + s_offset;
			else		 dma_s_addr = btm_addr_2 + s_offset;
			break;
		case 4:
			if(btm_t_m == 1) dma_s_addr = btm_addr + s_offset;
			else			 dma_s_addr = btm_addr + s_offset + dma_text_y * 0x8000;
			break;
		case 3:
			if(idx == 0) {
				if(cp_mode == 0) dma_s_addr = dma_s_addr_base + s_offset;
				else			 dma_s_addr = dma_s_addr_base + s_offset + dma_t_y * 0x8000;
			}
			else
				dma_s_addr = btm_addr + s_offset;
			break;
		}
		dma_t_addr = dma_t_addr_base + t_offset + addr_y_offset;

		Make_DMA_Scale(&TP[i], dma_s_x, dma_s_y, dma_t_x, dma_t_y2, dma_m, dma_n, dma_anti & 0x1);
		TP[i].Com_S_DDR_P = (dma_s_addr >> 5);
		TP[i].Com_T_DDR_P = (dma_t_addr >> 5);
	}

//	db_debug("Make_Bottom_DMA_Cmd() M_Mode=%d i_page=%d o_page=%d btm_m=%d btm_s=%d cp_mode=%d btm_t_m=%d btm_idx=%d  s_x=%d s_y=%d t_x=%d t_y=%d t_y2=%d y_offset=0x%x s_addr=0x%x t_addr=0x%x dma_cnt=%d\n",
//			M_Mode, i_page, o_page, btm_m, btm_s, cp_mode, btm_t_m, btm_idx,
//			dma_s_x, dma_s_y, dma_t_x, dma_t_y, dma_t_y2, addr_y_offset, dma_s_addr, dma_t_addr, dma_cnt);
}
void Make_Small_DMA_Cmd(Com_Video_struct *TP, int M_Mode, int i_page, int o_page)
{
    int dma_s_addr, dma_t_addr;
    int dma_s_x, dma_s_y;
    int dma_t_x, dma_t_y;
    int dma_m, dma_n;

    if(i_page == 0) dma_s_addr = ST_STM1_P0_T_ADDR;
    else			dma_s_addr = ST_STM1_P1_T_ADDR;

    switch(o_page) {
    case 0: dma_t_addr = ST_STM2_P0_T_ADDR; break;
    case 1: dma_t_addr = ST_STM2_P1_T_ADDR; break;
    case 2: dma_t_addr = ST_STM2_P2_T_ADDR; break;
    }

    if(M_Mode == 0)      {dma_s_x = S2_RES_12K_WIDTH; dma_s_y = S2_RES_12K_HEIGHT; dma_t_x = S2_RES_4K_WIDTH; dma_t_y = S2_RES_4K_HEIGHT; dma_m = 12; dma_n = 4; }
    else if(M_Mode == 1) {dma_s_x = S2_RES_8K_WIDTH;  dma_s_y = S2_RES_8K_HEIGHT;  dma_t_x = S2_RES_3K_WIDTH; dma_t_y = S2_RES_3K_HEIGHT; dma_m = 10; dma_n = 4; }
    else if(M_Mode == 2) {dma_s_x = S2_RES_6K_WIDTH;  dma_s_y = S2_RES_6K_HEIGHT;  dma_t_x = S2_RES_3K_WIDTH; dma_t_y = S2_RES_3K_HEIGHT; dma_m = 12; dma_n = 6; }

    Make_DMA_Scale(&TP[0], dma_s_x, dma_s_y, dma_t_x, dma_t_y, dma_m, dma_n, 0);
    TP[0].Com_S_DDR_P = (dma_s_addr >> 5);
    TP[0].Com_T_DDR_P = (dma_t_addr >> 5);
}
void Make_12K_to_6K_8K_DMA_Cmd(Com_Video_struct *TP, int M_Mode, int i_page, int o_page)
{
	int i;
    int dma_s_addr, dma_t_addr;
    int dma_s_x, dma_s_y;
    int dma_t_x, dma_t_y;
    int dma_m, dma_n;
    int dma_cnt, x_tmp, y_tmp;
    int x_offset, y_offset;

    switch(M_Mode) {
    case 1: x_tmp = S2_RES_8K_WIDTH;  y_tmp = S2_RES_8K_HEIGHT;
    		dma_m = 12; dma_n = 8;
    		break;

    case 2: x_tmp = S2_RES_6K_WIDTH;  y_tmp = S2_RES_6K_HEIGHT;
    		dma_m = 15; dma_n = 8;
    		break;
    default:return;
    		break;
    }
    dma_cnt = 4;	//(x_tmp + 3839) / 3840;
	dma_s_x = S2_RES_12K_WIDTH / 2;
	dma_s_y = S2_RES_12K_HEIGHT / 2;
	dma_t_x = x_tmp / 2;
	dma_t_y = y_tmp / 2;

	for(i = 0; i < dma_cnt; i++) {		//因DMA CMD tar_lx bit數不夠, 所以分段執行
		x_offset = i & 0x1;
		y_offset = i >> 1;
		if(i_page == 0) dma_s_addr = ST_STM1_P0_T_ADDR + x_offset * dma_s_x * 2 + y_offset * dma_s_y * 0x8000;
		else			dma_s_addr = ST_STM1_P1_T_ADDR + x_offset * dma_s_x * 2 + y_offset * dma_s_y * 0x8000;
		if(o_page == 0) dma_t_addr = ST_STM1_P0_T_ADDR + x_offset * dma_t_x * 2 + y_offset * dma_t_y * 0x8000;
		else			dma_t_addr = ST_STM1_P1_T_ADDR + x_offset * dma_t_x * 2 + y_offset * dma_t_y * 0x8000;

		Make_DMA_Scale(&TP[i], dma_s_x, dma_s_y, dma_t_x, dma_t_y, dma_m, dma_n, 0);
		TP[i].Com_S_DDR_P = (dma_s_addr >> 5);
		TP[i].Com_T_DDR_P = (dma_t_addr >> 5);
	}
}

void Make_9216_to_6912_DMA_Cmd(Com_Video_struct *TP, int i_page, int o_page)
{
	int i;
    int dma_s_addr, dma_t_addr;
    int dma_s_x, dma_s_y;
    int dma_t_x, dma_t_y;
    int dma_m, dma_n;
    int dma_cnt, x_tmp, y_tmp;
    int x_offset, y_offset;

    // 11520/6=1920  11520/8=1440
    x_tmp = 6912;                   // (768*3*4) * 6 / 8 = 6912
    y_tmp = 864;                    // (576*2) * 6 / 8 = 864
    dma_m = 8;                      //16; 
    dma_n = 6;                      //12;         //  1920 / 1440
    

    dma_cnt = 2;	                //(x_tmp + 3839) / 3840;
	dma_s_x = (768*3*4) / 2;
	dma_s_y = (576*2);
	dma_t_x = x_tmp / 2;
	dma_t_y = y_tmp;

	for(i = 0; i < dma_cnt; i++) {		//因DMA CMD tar_lx bit數不夠, 所以分段執行
		x_offset = i & 0x1;
		y_offset = 0;
		if(i_page == 0) dma_s_addr = ST_STM1_P0_T_ADDR + x_offset * dma_s_x * 2 + y_offset * dma_s_y * 0x8000;
		else			dma_s_addr = ST_STM1_P1_T_ADDR + x_offset * dma_s_x * 2 + y_offset * dma_s_y * 0x8000;
		if(o_page == 0) dma_t_addr = ST_STM1_P0_T_ADDR + x_offset * dma_t_x * 2 + y_offset * dma_t_y * 0x8000;
		else			dma_t_addr = ST_STM1_P1_T_ADDR + x_offset * dma_t_x * 2 + y_offset * dma_t_y * 0x8000;

		Make_DMA_Scale(&TP[i], dma_s_x, dma_s_y, dma_t_x, dma_t_y, dma_m, dma_n, 0);
		TP[i].Com_S_DDR_P = (dma_s_addr >> 5);
		TP[i].Com_T_DDR_P = (dma_t_addr >> 5);
	}
}
int Make_DMA_Cmd(int idx, AS2_F2_MAIN_CMD_struct *Cmd_p, FPGA_Engine_One_Struct *F_Pipe_p, F_Com_In_Struct *F_Com_p)
{
    int C_Mode = F_Com_p->Record_Mode;
    int M_Mode = F_Pipe_p->M_Mode;
    int i_page = (F_Pipe_p->I_Page % 10);
    int o_page = (F_Pipe_p->O_Page % 10);

    unsigned *dma_ptr;
    Com_Video_struct TP[4];
    int btm_m = 0, btm_s = 100, btm_t_m = 0, btm_idx = 0;
    int cp_mode = 0;

    if(F_Pipe_p->Time_Buf <= 0) return 0;
    get_F_Temp_DMA_SubData(idx, &btm_m, &btm_s, &cp_mode, &btm_t_m, &btm_idx);

    memset(&TP, 0, sizeof(TP) );

    switch(C_Mode) {
    case 0: // Cap (1p)
    case 5: // HDR (1p)
    case 6: // Night
    case 7: // Night + HDR
    case 8: // Sport
    case 9: // Sport + WDR
    case 12:// M-Mode
    case 13:// Removal
    case 14:// 3D-Model
        switch(M_Mode) {
        case 0:
        case 1:
        case 2:
            if(btm_idx == 3)
                Make_9216_to_6912_DMA_Cmd(&TP, 0, 1);     // i_page=0, o_page=1
        	else if(btm_idx == 2)
        		Make_12K_to_6K_8K_DMA_Cmd(&TP, M_Mode, i_page, o_page);
        	else if(btm_idx == 0 || btm_idx == 1 || btm_idx == 4)
        		Make_Bottom_DMA_Cmd(&TP, M_Mode, i_page, o_page, btm_m, btm_s, cp_mode, btm_t_m, btm_idx);
            db_debug("Make_DMA_Cmd: 00 idx=%d M_Mode=%d i_page=%d o_page=%d btm_idx=%d\n", idx, M_Mode, i_page, o_page, btm_idx);
            break;
        case 3:
        case 4:
        case 5:
        	if(btm_idx == 0 || btm_idx == 1 || btm_idx == 4)
        		Make_Bottom_DMA_Cmd(&TP, M_Mode, i_page, o_page, btm_m, btm_s, cp_mode, btm_t_m, btm_idx);
        	break;
        }
        break;
    case 3: // AEB (3p,5p,7p)
    	if(btm_idx == 2)
    		Make_12K_to_6K_8K_DMA_Cmd(&TP, M_Mode, i_page, o_page);
    	db_debug("Make_DMA_Cmd: 01 idx=%d M_Mode=%d i_page=%d o_page=%d btm_idx=%d\n", idx, M_Mode, i_page, o_page, btm_idx);
    	break;
    case 4: // RAW (5p)
    	break;
    case 1: // Rec
    case 10:// Rec + WDR
    	if(btm_idx == 0 || btm_idx == 1 || btm_idx == 4)
    		Make_Bottom_DMA_Cmd(&TP, M_Mode, i_page, o_page, btm_m, btm_s, cp_mode, btm_t_m, btm_idx);
        break;
    case 2: // Time Lapse
    case 11:// Time Lapse + WDR
        switch(M_Mode) {
        case 0:
        case 1:
        case 2:
        case 3:
        	if(btm_idx == 0 || btm_idx == 1 || btm_idx == 4 || btm_idx == 5)
        		Make_Bottom_DMA_Cmd(&TP, M_Mode, i_page, o_page, btm_m, btm_s, cp_mode, btm_t_m, btm_idx);
        	else if(btm_idx == 6)
        		Make_Small_DMA_Cmd(&TP, M_Mode, i_page, o_page);
                break;
        }
        break;
    }

    dma_ptr = (unsigned*)&TP[0];
    Cmd_p->DMA_CMD.DMA_CMD_DW0_0.Data   = dma_ptr[0];
    Cmd_p->DMA_CMD.DMA_CMD_DW1_0.Data   = dma_ptr[1];
    Cmd_p->DMA_CMD.DMA_CMD_DW2_0.Data   = dma_ptr[2];
    Cmd_p->DMA_CMD.DMA_CMD_DW3_0.Data   = dma_ptr[3];

    dma_ptr = (unsigned*)&TP[1];
    Cmd_p->DMA_CMD.DMA_CMD_DW0_1.Data   = dma_ptr[0];
    Cmd_p->DMA_CMD.DMA_CMD_DW1_1.Data   = dma_ptr[1];
    Cmd_p->DMA_CMD.DMA_CMD_DW2_1.Data   = dma_ptr[2];
    Cmd_p->DMA_CMD.DMA_CMD_DW3_1.Data   = dma_ptr[3];

    dma_ptr = (unsigned*)&TP[2];
    Cmd_p->DMA_CMD.DMA_CMD_DW0_2.Data   = dma_ptr[0];
    Cmd_p->DMA_CMD.DMA_CMD_DW1_2.Data   = dma_ptr[1];
    Cmd_p->DMA_CMD.DMA_CMD_DW2_2.Data   = dma_ptr[2];
    Cmd_p->DMA_CMD.DMA_CMD_DW3_2.Data   = dma_ptr[3];

    dma_ptr = (unsigned*)&TP[3];
    Cmd_p->DMA_CMD.DMA_CMD_DW0_3.Data   = dma_ptr[0];
    Cmd_p->DMA_CMD.DMA_CMD_DW1_3.Data   = dma_ptr[1];
    Cmd_p->DMA_CMD.DMA_CMD_DW2_3.Data   = dma_ptr[2];
    Cmd_p->DMA_CMD.DMA_CMD_DW3_3.Data   = dma_ptr[3];
    return 1;
}
int Make_USB_Cmd(int idx, AS2_F2_MAIN_CMD_struct *Cmd_p, FPGA_Engine_One_Struct *F_Pipe_p, F_Com_In_Struct *F_Com_p)
{
	int i;
    int C_Mode = F_Com_p->Record_Mode;
    int M_Mode = F_Pipe_p->M_Mode;
    int i_page = (F_Pipe_p->I_Page % 10);
    int o_page = (F_Pipe_p->O_Page % 10);
    int Pic_No = F_Pipe_p->No;
    int jpeg_sel;
    int usb_s_addr0;
    int usb_en0;
    int enc_type, usb_en;

    if(F_Pipe_p->Time_Buf <= 0) return 0;

    get_F_Temp_USB_SubData(idx, &enc_type, &usb_en);
    if(usb_en == 0) return 0;

    //比對是否為切mode後需要清空的cmd
    for(i = 0; i < CLOSE_USB_CNT; i++) {
    	if(Pic_No == A2K.USB_Close_No[i]) {
    		A2K.USB_Close_No[i] = -1;
    		return 0;
    	}
    }

    /*
     * jpeg_sel:
     * 0 1: JPEG0 Small		2: JPEG0 Big	3: JPEG0
     * 4 5: JPEG1 Small		6: JPEG1 Big	7: JPEG1
     * 8 9: H264
     */
    switch(C_Mode) {
	case 0: // Cap (1p)
	case 3: // AEB (3p,5p,7p)
	case 4: // RAW (5p)
	case 5: // HDR (1p)
	case 6: // Night
	case 7: // Night + HDR
	case 8:	// Sport
	case 9:	// Sport + WDR
	case 12:// M-Mode
	case 13:// Removal
	case 14:// 3D-Model
		switch(M_Mode) {
		case 0:
		case 1:
		case 2:
			switch(i_page) {
			case 0: usb_s_addr0 = USB_STM1_P0_S_BUF_ADDR; jpeg_sel = 2; break;		//JPEG0
			case 1: usb_s_addr0 = USB_STM1_P0_S_BUF_ADDR; jpeg_sel = 2; break;		//JPEG0
			case 2: usb_s_addr0 = USB_STM1_P1_S_BUF_ADDR; jpeg_sel = 6; break;		//JPEG1
			case 3: usb_s_addr0 = USB_STM1_P1_S_BUF_ADDR; jpeg_sel = 6; break;		//JPEG1
			}
			usb_en0 = 1;
			db_debug("Make_USB_Cmd: idx=%d i_page=%d o_page=%d usb_s_addr0=0x%x jpeg_sel=%d\n", idx, i_page, o_page, usb_s_addr0, jpeg_sel);
			break;
		case 3:
		case 4:
		case 5:
			//Pipe:		T1		T2		T3		T4
			//JPEG0:	JPEG0_0			JPEG0_1
			//JPEG1:			JPEG1_0			JPEG1_1
			//USB:		JPEG0_1	JPEG1_1	JPEG0_0	JPEG1_0
			switch(i_page) {
			case 0: usb_s_addr0 = USB_STM2_P3_S_BUF_ADDR; jpeg_sel = 5; break;
			case 1: usb_s_addr0 = USB_STM2_P2_S_BUF_ADDR; jpeg_sel = 4; break;
			case 2: usb_s_addr0 = USB_STM2_P0_S_BUF_ADDR; jpeg_sel = 0; break;
			case 3: usb_s_addr0 = USB_STM2_P1_S_BUF_ADDR; jpeg_sel = 1; break;
			}
			usb_en0 = 1;
			break;
		}
		break;

	case 1:				//Rec
	case 10:				//Rec + WDR
		switch(M_Mode) {
		case 3:
		case 4:
		case 5:
			switch(i_page) {
			case 0: usb_s_addr0 = USB_STM2_P3_S_BUF_ADDR; jpeg_sel = 5; break;
			case 1: usb_s_addr0 = USB_STM2_P2_S_BUF_ADDR; jpeg_sel = 4; break;
			case 2: usb_s_addr0 = USB_STM2_P0_S_BUF_ADDR; jpeg_sel = 0; break;
			case 3: usb_s_addr0 = USB_STM2_P1_S_BUF_ADDR; jpeg_sel = 1; break;
			}
			usb_en0 = 1;
			break;
		}
		break;

	case 2:				//Time Lapse
	case 11:				//Time Lapse + WDR
		switch(M_Mode) {
		case 0:
		case 1:
		case 2:
			if(enc_type == 0 || enc_type == 2) {
				switch(i_page) {
				case 0: usb_s_addr0 = USB_STM1_P0_S_BUF_ADDR; jpeg_sel = 2; break;
				case 1: usb_s_addr0 = USB_STM1_P0_S_BUF_ADDR; jpeg_sel = 2; break;
				case 2: usb_s_addr0 = USB_STM1_P1_S_BUF_ADDR; jpeg_sel = 6; break;
				case 3: usb_s_addr0 = USB_STM1_P1_S_BUF_ADDR; jpeg_sel = 6; break;
				}
			}
			else {
				switch(i_page) {
				case 0: usb_s_addr0 = USB_STM1_H264_P0_S_BUF_ADDR; jpeg_sel = 8; break;
				case 1: usb_s_addr0 = USB_STM1_H264_P1_S_BUF_ADDR; jpeg_sel = 9; break;
				case 2: usb_s_addr0 = USB_STM1_H264_P0_S_BUF_ADDR; jpeg_sel = 8; break;
				case 3: usb_s_addr0 = USB_STM1_H264_P1_S_BUF_ADDR; jpeg_sel = 9; break;
				}
			}
			usb_en0 = 1;
			break;
		case 3:
		case 4:
			if(enc_type == 0 || enc_type == 2) {
				//縮時一個JPEG引擎壓大圖, 另一個壓小圖
				switch(i_page) {
				case 0: usb_s_addr0 = USB_STM2_P0_S_BUF_ADDR; jpeg_sel = 0; break;
				case 1: usb_s_addr0 = USB_STM2_P1_S_BUF_ADDR; jpeg_sel = 1; break;
				case 2: usb_s_addr0 = USB_STM2_P2_S_BUF_ADDR; jpeg_sel = 4; break;
				case 3: usb_s_addr0 = USB_STM2_P3_S_BUF_ADDR; jpeg_sel = 5; break;
				}
			}
			else {
				switch(i_page) {
				case 0: usb_s_addr0 = USB_STM2_H264_P0_S_BUF_ADDR; jpeg_sel = 8; break;
				case 1: usb_s_addr0 = USB_STM2_H264_P1_S_BUF_ADDR; jpeg_sel = 9; break;
				case 2: usb_s_addr0 = USB_STM2_H264_P0_S_BUF_ADDR; jpeg_sel = 8; break;
				case 3: usb_s_addr0 = USB_STM2_H264_P1_S_BUF_ADDR; jpeg_sel = 9; break;
				}
			}
			usb_en0 = 1;
			break;
		}
		break;
	}

    Cmd_p->USB_CMD.S_DDR_ADDR_0.Data         = (usb_s_addr0 >> 5);
    Cmd_p->USB_CMD.JPEG_SEL_0.Data			 = jpeg_sel;
    if(usb_en0 == 1) {
        Cmd_p->USB_CMD.Start_En_0.Address = 0xCCAA0303;
        Cmd_p->USB_CMD.Start_En_0.Data    = usb_en0;
    }
    else {
        Cmd_p->USB_CMD.Start_En_0.Address = 0x00000000;
        Cmd_p->USB_CMD.Start_En_0.Data    = 0;
    }
    return 1;
}
char zero_cmd[2048];
void Make_Main_Cmd(int idx, int *bulb_st)
{
    int do_isp1_flag = 0;
    static unsigned long long curTime, lstTime;
    AS2_F0_MAIN_CMD_struct *FX_M_Cmd_Tmp;
    AS2_F2_MAIN_CMD_struct *F2_M_Cmd_Tmp;

    S_M_CMD_struct *S_Cmd_Tmp;
    int execute=0;

    if(idx < 0 || idx >= FPGA_ENGINE_P_MAX){
        db_error("Make_Main_Cmd: err! idx=%d\n", idx);
        return;
    }

    FX_M_Cmd_Tmp = &FX_MAIN_CMD_Q[idx];
    F2_M_Cmd_Tmp = &F2_MAIN_CMD_Q[idx];


    // 需按照step順序執行
    // step.1  make sensor command
        S_Cmd_Tmp = &F2_MAIN_CMD_Q[idx].SENSOR_CMD;
        execute = Make_Sensor_Cmd(idx, S_Cmd_Tmp, &F_Pipe.Sensor.P[idx], &F_Com_In, bulb_st);
        cle_F_Temp_Sensor_SubData(idx);
        if(execute == 0){
            memset(S_Cmd_Tmp, 0, sizeof(S_M_CMD_struct));
        }
        else{
            memset(&F_Pipe.Sensor.P[idx], 0, sizeof(F_Pipe.Sensor.P[idx]));
        }
    // step.2  make isp1 command
        FX_M_Cmd_Tmp->F0_ISP1_CMD = FX_MAIN_CMD_P[0].F0_ISP1_CMD;
        FX_M_Cmd_Tmp->F1_ISP1_CMD = FX_MAIN_CMD_P[0].F1_ISP1_CMD;

        execute = Make_ISP1_Cmd(idx, FX_M_Cmd_Tmp,  &F_Pipe.ISP1.P[idx], &F_Com_In);
        if(execute == 0){
            memset(&FX_M_Cmd_Tmp->F0_ISP1_CMD, 0, sizeof(FX_M_Cmd_Tmp->F0_ISP1_CMD) );
            memset(&FX_M_Cmd_Tmp->F1_ISP1_CMD, 0, sizeof(FX_M_Cmd_Tmp->F1_ISP1_CMD) );
            do_isp1_flag = 0;
        }
        else{
            memset(&F_Pipe.ISP1.P[idx], 0, sizeof(F_Pipe.ISP1.P[idx]));
            do_isp1_flag = 1;
        }
    // step.2-1 make wdr command
        FX_M_Cmd_Tmp->F0_Diffusion_CMD = FX_MAIN_CMD_P[0].F0_Diffusion_CMD;
        FX_M_Cmd_Tmp->F1_Diffusion_CMD = FX_MAIN_CMD_P[0].F1_Diffusion_CMD;
        execute = Make_WDR_Cmd(idx, FX_M_Cmd_Tmp, &F_Pipe.ISP2.P[idx], &F_Com_In, do_isp1_flag);         // used ISP2 page index
        if(execute == 0){
            memset(&FX_M_Cmd_Tmp->F0_Diffusion_CMD, 0, sizeof(FX_M_Cmd_Tmp->F0_Diffusion_CMD) );
            memset(&FX_M_Cmd_Tmp->F1_Diffusion_CMD, 0, sizeof(FX_M_Cmd_Tmp->F1_Diffusion_CMD) );
        }
        //else{
        //    memset(&F_Pipe.ISP2.P[idx], 0, sizeof(F_Pipe.ISP2.P[idx]));
        //}
    // step.3 make isp2 command
        FX_M_Cmd_Tmp->F0_ISP2_CMD = FX_MAIN_CMD_P[0].F0_ISP2_CMD;
        FX_M_Cmd_Tmp->F1_ISP2_CMD = FX_MAIN_CMD_P[0].F1_ISP2_CMD;
        execute = Make_ISP2_Cmd(idx, FX_M_Cmd_Tmp, &F_Pipe.ISP2.P[idx], &F_Com_In);
        if(execute == 0){
            memset(&FX_M_Cmd_Tmp->F0_ISP2_CMD, 0, sizeof(FX_M_Cmd_Tmp->F0_ISP2_CMD) );
            memset(&FX_M_Cmd_Tmp->F1_ISP2_CMD, 0, sizeof(FX_M_Cmd_Tmp->F1_ISP2_CMD) );
        }
        else{
            memset(&F_Pipe.ISP2.P[idx], 0, sizeof(F_Pipe.ISP2.P[idx]));
        }
    // step.4 make stitch command
        FX_M_Cmd_Tmp->F0_Stitch_CMD = FX_MAIN_CMD_P[0].F0_Stitch_CMD;
        FX_M_Cmd_Tmp->F1_Stitch_CMD = FX_MAIN_CMD_P[0].F1_Stitch_CMD;
        execute = Make_ST_Cmd(idx, FX_M_Cmd_Tmp, &F_Pipe.Stitch.P[idx], &F_Com_In);
        if(execute == 0) {
            memset(&FX_M_Cmd_Tmp->F0_Stitch_CMD, 0, sizeof(FX_M_Cmd_Tmp->F0_Stitch_CMD) );
            memset(&FX_M_Cmd_Tmp->F1_Stitch_CMD, 0, sizeof(FX_M_Cmd_Tmp->F1_Stitch_CMD) );
        }
        else{
            memset(&F_Pipe.Stitch.P[idx], 0, sizeof(F_Pipe.Stitch.P[idx]));
        }
    // step.4-1 make smooth command
        F2_M_Cmd_Tmp->SMOOTH_CMD = F2_MAIN_CMD_P[0].SMOOTH_CMD;
        execute = Make_Smooth_Cmd(idx, F2_M_Cmd_Tmp, &F_Pipe.Smooth.P[idx], &F_Com_In);
        if(execute == 0){
            memset(&F2_M_Cmd_Tmp->SMOOTH_CMD, 0, sizeof(F2_M_Cmd_Tmp->SMOOTH_CMD) );
        }
        else{
            memset(&F_Pipe.Smooth.P[idx], 0, sizeof(F_Pipe.Smooth.P[idx]));
        }
    // step.5 make jpeg 0 command
        F2_M_Cmd_Tmp->JPEG0_CMD = F2_MAIN_CMD_P[0].JPEG0_CMD;
        execute = Make_JPEG_Cmd(idx, F2_M_Cmd_Tmp,  &F_Pipe.Jpeg[0].P[idx], &F_Com_In, 0);
        if(execute == 0){
            memset(&F2_M_Cmd_Tmp->JPEG0_CMD, 0, sizeof(F2_M_Cmd_Tmp->JPEG0_CMD) );
        }
        else{
            memset(&F_Pipe.Jpeg[0].P[idx], 0, sizeof(F_Pipe.Jpeg[0].P[idx]));
        }
    // step.5 make jpeg 1 command
        F2_M_Cmd_Tmp->JPEG1_CMD = F2_MAIN_CMD_P[0].JPEG1_CMD;
        execute = Make_JPEG_Cmd(idx, F2_M_Cmd_Tmp,  &F_Pipe.Jpeg[1].P[idx], &F_Com_In, 1);
        if(execute == 0){
            memset(&F2_M_Cmd_Tmp->JPEG1_CMD, 0, sizeof(F2_M_Cmd_Tmp->JPEG1_CMD) );
        }
        else{
            memset(&F_Pipe.Jpeg[1].P[idx], 0, sizeof(F_Pipe.Jpeg[1].P[idx]));
        }
    // step.8 make h264 command
        F2_M_Cmd_Tmp->H264_CMD = F2_MAIN_CMD_P[0].H264_CMD;
        execute = Make_H264_Cmd(idx, F2_M_Cmd_Tmp,  &F_Pipe.H264.P[idx], &F_Com_In);
        if(execute == 0){
            memset(&F2_M_Cmd_Tmp->H264_CMD, 0, sizeof(F2_M_Cmd_Tmp->H264_CMD) );
        }
        else{
            memset(&F_Pipe.H264.P[idx], 0, sizeof(F_Pipe.H264.P[idx]));
        }
    // step.6 make dma command
        F2_M_Cmd_Tmp->DMA_CMD = F2_MAIN_CMD_P[0].DMA_CMD;
        execute = Make_DMA_Cmd(idx, F2_M_Cmd_Tmp, &F_Pipe.DMA.P[idx], &F_Com_In);
        if(execute == 0){
            memset(&F2_M_Cmd_Tmp->DMA_CMD, 0, sizeof(F2_M_Cmd_Tmp->DMA_CMD) );
        }
        else{
            memset(&F_Pipe.DMA.P[idx], 0, sizeof(F_Pipe.DMA.P[idx]));
        }
    // step.7 make usb command
        F2_M_Cmd_Tmp->USB_CMD = F2_MAIN_CMD_P[0].USB_CMD;
        execute = Make_USB_Cmd(idx, F2_M_Cmd_Tmp, &F_Pipe.USB.P[idx], &F_Com_In);
        if(execute == 0){
            memset(&F2_M_Cmd_Tmp->USB_CMD, 0, sizeof(F2_M_Cmd_Tmp->USB_CMD) );
        }
        else{
            memset(&F_Pipe.USB.P[idx], 0, sizeof(F_Pipe.USB.P[idx]));
        }

	SetWaveDebug(1, idx);
	//Send Main Cmd
	int i, addr, size;
	unsigned char *data;
	addr = F0_M_CMD_ADDR;
	size = sizeof(AS2_F0_MAIN_CMD_struct);
	for(i = 0; i < size; i+=2048) {
		data = (unsigned char *) FX_M_Cmd_Tmp;
		k_ua360_spi_ddr_write(addr + (idx&0x3f)*0x1000 + i,  (data+i), 2048);      // (idx&0x3f) -> HW只保留64組
	}

	addr = F2_M_CMD_ADDR;
	size = sizeof(AS2_F2_MAIN_CMD_struct);
	for(i = 0; i < size; i+=2048) {
		data = (unsigned char *) F2_M_Cmd_Tmp;
		k_ua360_spi_ddr_write(addr + (idx&0x3f)*0x1000 + i,  (data+i), 2048);
	}

	if(*bulb_st > 0){           // 避免sensor誤動作，清掉下一道cmd
	    memset(zero_cmd, 0, sizeof(zero_cmd));
	    addr = F0_M_CMD_ADDR;
	    size = sizeof(AS2_F0_MAIN_CMD_struct);
	    for(i = 0; i < size; i+=2048) {
	    	data = (unsigned char *) zero_cmd;
	    	k_ua360_spi_ddr_write(addr + ((idx+1)&0x3f)*0x1000 + i,  (data+i), 2048);
	    }
	    addr = F2_M_CMD_ADDR;
	    size = sizeof(AS2_F2_MAIN_CMD_struct);
	    for(i = 0; i < size; i+=2048) {
	    	data = (unsigned char *) zero_cmd;
	    	k_ua360_spi_ddr_write(addr + ((idx+1)&0x3f)*0x1000 + i,  (data+i), 2048);
	    }
	}

	Set_FPGA_Pipe_Idx(idx & 0x3F);
}

void MainCmdInit(void)
{
	int i, addr, size;
	unsigned char *data;

    memset(&FX_MAIN_CMD_Q[0], 0, sizeof(FX_MAIN_CMD_Q) );
    memset(&F2_MAIN_CMD_Q[0], 0, sizeof(F2_MAIN_CMD_Q) );

	addr = F0_M_CMD_ADDR;
	size = 0x80000;		//sizeof(FX_MAIN_CMD_Q);
	data = (unsigned char *) &FX_MAIN_CMD_Q[0];
	for(i = 0; i < size; i+=2048)
		k_ua360_spi_ddr_write(addr+i, (data+i), 2048);

	addr = F2_M_CMD_ADDR;
	size = 0x80000;		//sizeof(F2_MAIN_CMD_Q);
	data = (unsigned char *) &F2_MAIN_CMD_Q[0];
	for(i = 0; i < size; i+=2048)
		k_ua360_spi_ddr_write(addr+i, (data+i), 2048);

	db_debug("MainCmdInit() 00 size=%d\n", size);
}

//---------------------------------------------------------------------------  MakeH264Header
// 製作檔頭(Pic)

unsigned Qp_Table[52] = {
	0x0B35,
	0x0B33,0x0B31,0x0B2F,0x0B2D,0x0B2B,
	0x0B29,0x0B27,0x0B25,0x0B23,0x0B21,
	0x091F,0x091D,0x091B,0x0919,0x0917,
	0x0915,0x0913,0x0911,0x070F,0x070D,
	0x070B,0x0709,0x0507,0x0505,0x0303,
	0x0101,
	0x0302,0x0504,0x0506,0x0708,0x070A,
	0x070C,0x070E,0x0910,0x0912,0x0914,
	0x0916,0x0918,0x091A,0x091C,0x091E,
	0x0B20,0x0B22,0x0B24,0x0B26,0x0B28,
	0x0B2A,0x0B2C,0x0B2E,0x0B30,0x0B32,
};
char ALPHA_TABLE[52]  = {0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,4,4,5,6,  7,8,9,10,12,13,15,17,  20,22,25,28,32,36,40,45,  50,56,63,71,80,90,101,113,  127,144,162,182,203,226,255,255} ;
char  BETA_TABLE[52]  = {0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,2,2,2,3,  3,3,3, 4, 4, 4, 6, 6,   7, 7, 8, 8, 9, 9,10,10,  11,11,12,12,13,13, 14, 14,   15, 15, 16, 16, 17, 17, 18, 18} ;
char CLIP_TAB[52][5]  = {
  { 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},{ 0, 0, 0, 0, 0},
  { 0, 0, 0, 0, 0},{ 0, 0, 0, 1, 1},{ 0, 0, 0, 1, 1},{ 0, 0, 0, 1, 1},{ 0, 0, 0, 1, 1},{ 0, 0, 1, 1, 1},{ 0, 0, 1, 1, 1},{ 0, 1, 1, 1, 1},
  { 0, 1, 1, 1, 1},{ 0, 1, 1, 1, 1},{ 0, 1, 1, 1, 1},{ 0, 1, 1, 2, 2},{ 0, 1, 1, 2, 2},{ 0, 1, 1, 2, 2},{ 0, 1, 1, 2, 2},{ 0, 1, 2, 3, 3},
  { 0, 1, 2, 3, 3},{ 0, 2, 2, 3, 3},{ 0, 2, 2, 4, 4},{ 0, 2, 3, 4, 4},{ 0, 2, 3, 4, 4},{ 0, 3, 3, 5, 5},{ 0, 3, 4, 6, 6},{ 0, 3, 4, 6, 6},
  { 0, 4, 5, 7, 7},{ 0, 4, 5, 8, 8},{ 0, 4, 6, 9, 9},{ 0, 5, 7,10,10},{ 0, 6, 8,11,11},{ 0, 6, 8,13,13},{ 0, 7,10,14,14},{ 0, 8,11,16,16},
  { 0, 9,12,18,18},{ 0,10,13,20,20},{ 0,11,15,23,23},{ 0,13,17,25,25}
} ;
int MakeH264Header(unsigned char *buf, int IP_M, int FrameNum, int Qp,
			unsigned Width, unsigned Height, unsigned Time_Stamp, unsigned Frame_Stamp,
			int CameraMode, int Resolution)
{
	int i, val, len, Bit_Size, ByP,BtP;
	char byte_buf;
	unsigned mask;
	char D0[4] = {0x00,0x00,0x00,0x01};
	char D1[4] = {0x65,0x88,0x84,0x00};
	H_File_Head_rec *FHead_rec;

	memcpy(&buf[30], &D0[0], sizeof(D0) );
	if(IP_M == 0) {
		memcpy(&buf[34], &D1[0], sizeof(D1) );
//	  	*(buf+36)  = 0x65;
//	  	*(buf+37)  = 0x88;
//	  	*(buf+38)  = (FrameNum << 3) | 0x84;
//	  	*(buf+39)  = 0x00;
		ByP = 37;
		BtP = 4;
		Bit_Size = ByP * 8 + (8 - BtP);
	} else {
		*(buf+34)  = 0x41;
		*(buf+35)  = 0x9A;
		*(buf+35) |= (FrameNum >> 3) &0x1;
		*(buf+36)  = (FrameNum << 5) | 0x01;
		*(buf+37)  = 0x80;
		ByP = 37;
		BtP = 5;
		Bit_Size = ByP * 8 + (8 - BtP);
	}

	val = Qp_Table[Qp]&0xFF;
	len = (Qp_Table[Qp]&0xFF00)>>8;
	mask = 1 << (len-1);
	byte_buf = *(buf+ByP);
	byte_buf >>= BtP;
	for(i = 0; i < len; i++) {
		byte_buf <<= 1;
		if (val & mask) byte_buf |= 1;
		BtP--;
		mask >>= 1;
		if (BtP == 0){
			BtP	        = 8;
			*(buf+ByP) = byte_buf;
			ByP++;
			byte_buf    = 0;
		}
	}
//  byte_buf <<= BtP;
	*(buf+ByP) = byte_buf;

	Bit_Size += len;

	FHead_rec = (H_File_Head_rec *)buf;
	FHead_rec->Start_Code 	= Head_REC_Start_Code;
	FHead_rec->CameraMode 	= CameraMode;
	FHead_rec->Resolution 	= Resolution;
	FHead_rec->Time_Stamp 	= Time_Stamp;
	FHead_rec->Frame_Stamp 	= Frame_Stamp;
	FHead_rec->Size_V		= Height;
	FHead_rec->Size_H 		= Width;
	FHead_rec->QP          	= Qp;
	FHead_rec->F_Cnt 		= FrameNum;
	FHead_rec->Alpha 		= ALPHA_TABLE[Qp];
	FHead_rec->IP_M			= IP_M;
	FHead_rec->Beta	   		= BETA_TABLE[Qp];
	FHead_rec->clip        	= CLIP_TAB[Qp][3];
    FHead_rec->rev_bit		= 0;

    return Bit_Size;
}
