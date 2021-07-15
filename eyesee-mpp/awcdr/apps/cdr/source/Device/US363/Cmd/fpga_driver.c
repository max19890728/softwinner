/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/
 
#include "Device/US363/Cmd/fpga_driver.h"

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>

#include "Device/us363_camera.h"
#include "Device/US363/us363_para.h"
#include "Device/US363/Cmd/us363_spi.h"
#include "Device/US363/Cmd/spi_cmd.h"
#include "Device/US363/Cmd/us360_func.h"
#include "Device/US363/Cmd/us360_define.h"
#include "Device/US363/Cmd/AletaS2_CMD_Struct.h"
#include "Device/US363/Cmd/Smooth.h"
#include "Device/US363/Cmd/defect.h"
#include "Device/US363/Kernel/k_spi_cmd.h"
#include "Device/US363/Kernel/FPGA_Pipe.h"
#include "Device/US363/Test/test.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::FPGADriver"

/*
 * ddr_avg_rgb[]
 *
 *   135(y) x 1024(byte) = 138240(byte)
 *
 *     256   256   256   256       -> 256(byte) = 240(pixel)+16(null) = 1920/8
 *   +-----+-----+-----+-----+
 *   |  R  |  G  |  B  |     | 135 -> 135(line) = 1080/8
 *   +-----+-----+-----+-----+
 * */

int ISP_command_ready = 0;
int Block2_2DNR_ready = -1;
int Block2_3DNR_ready = 0;

ISP_All_Set_Reg_Struct  ISP_All_Set_Reg;
ISP_Command_Struct 	    ISP_All_Command_D;					// 取代 isp_cmd_tmp[0]
ISP_Stati_Struct 		ISP_All_State_D;					// 取代 isp_state_tmp[0]

int YUV2RGB(int y, int u, int v, int *r, int *g, int *b)
{
	*r = (float)(y + ((292 * v) >> 8) - 146);
	*g = (float)(y - (((149 * v)+(101 * u)) >> 8) + 125);
	*b = (float)(y + ((516 * u) >> 8) - 258);

    if (*r > 255)  *r= 255; if (*r < 0)  *r= 0;
    if (*g > 255)  *g= 255; if (*g < 0)  *g= 0;
    if (*b > 255)  *b= 255; if (*b < 0)  *b= 0;

    return 0;
}

int YUV2RGB_2(int y, int u, int v, int *r, int *g, int *b)
{
	*r = (float)(y + ((292 * v) >> 8) - (146 << 8));
	*g = (float)(y - (((149 * v)+(101 * u)) >> 8) + (125 << 8));
	*b = (float)(y + ((516 * u) >> 8) - (258 << 8));

    if (*r > 0xFFFF)  *r= 0xFFFF; if (*r < 0)  *r= 0;
    if (*g > 0xFFFF)  *g= 0xFFFF; if (*g < 0)  *g= 0;
    if (*b > 0xFFFF)  *b= 0xFFFF; if (*b < 0)  *b= 0;

    return 0;
}

//sensor 亮度取線轉換
static int Bri_Table_1024[1024];
static int Bri_Table_256[256]={0};
void Brightness1024to256Init(void)
{
	int i, j, idx;
	int table1[5] = {0, 256, 512, 768, 1024}; // 256-256-256-256
	int table2[5] = {0, 128, 192, 224,  256}; // 128-64-32-32

	for(i = 0; i < 1024; i++) {
		idx = i / 256;
		Bri_Table_1024[i] = table2[idx] + (table2[idx+1] - table2[idx]) * (i % 256) / 256;
		//[1024]                                [256]
		//   0 :   0 + (128-  0) * (  0) / 256 = 0
		//   1 :   0 + (128-  0) * (  1) / 256 = 0
		//   2 :   0 + (128-  0) * (  2) / 256 = 1
		// 255 :   0 + (128-  0) * (255) / 256 = 127
		// 256 : 128 + (192-128) * (  0) / 256 = 128

		if(Bri_Table_1024[i] > 255) Bri_Table_1024[i] = 255;

		j = Bri_Table_1024[i];
		if(Bri_Table_256[j] == 0){
			Bri_Table_256[j] = i;
		}
	}
}

int Brightness256to1024(int value)
{
    static init=0;
    int i;
    if(init == 0){
        Brightness1024to256Init();
        init = 1;
    }
    return Bri_Table_256[value];
}

int Color_ST_Y_Th_Min = 50;
void SetColorSTYThMin(int value)
{
	Color_ST_Y_Th_Min = value;
}

int GetColorSTYThMin(void)
{
	return Color_ST_Y_Th_Min;
}

float Adj_WB_R=0.0, Adj_WB_G=0.0, Adj_WB_B=0.0;
float WB_Mode_Table2[6][3] = {
		// R    G    B
		{  1.0,   1.0, 1.0},	// Auto
		{  1.543, 1.0, 2.195},	// Filament Lamp (2500K)
		{  1.647, 1.0, 1.721},	// Daylight Lamp (4500K)
		{  1.715, 1.0, 1.422},	// Sun (6000K)
		{  1.876, 1.0, 1.28},	// Cloudy (7000K)
		{  1.0,   1.0, 1.0}	    // rgb
};


void check_SLT_40point(int *data_p, int M_Mode, struct US360_Stitching_Line_Table *slt_p)			// 40點色縫合
{
	int SL_id = 0;
	int SL_cnt = 0, SL_Top_cnt = 0, SL_Btm_cnt = 0;
	int top_s, top_s_idx, btm_s, btm_s_idx;
	struct US360_Stitching_FeedBack *buf_p = (struct US360_Stitching_FeedBack *)data_p;

	SL_id = slt_p->CB_Idx;

    top_s     = A_L_I3[M_Mode][SL_id].p[0].Sensor_Idx;
    top_s_idx = A_L_I3[M_Mode][SL_id].p[0].A_L_S_Idx;
    btm_s	  = A_L_I3[M_Mode][SL_id].p[1].Sensor_Idx;
    btm_s_idx = A_L_I3[M_Mode][SL_id].p[1].A_L_S_Idx;

    if(buf_p->Sum >= 256) {
    	if(slt_p->X_Posi == 0) {				//Top
    		A_L_I3[M_Mode][SL_id].YC_Temp[0].Y = (buf_p->Y << 13) / buf_p->Sum;
    		A_L_I3[M_Mode][SL_id].YC_Temp[0].U = (buf_p->U << 13) / buf_p->Sum;
    		A_L_I3[M_Mode][SL_id].YC_Temp[0].V = (buf_p->V << 13) / buf_p->Sum;

    		Smooth_YUV_Data[M_Mode][0][SL_id][0] = A_L_I3[M_Mode][SL_id].YC_Temp[0].Y;
    		Smooth_YUV_Data[M_Mode][1][SL_id][0] = A_L_I3[M_Mode][SL_id].YC_Temp[0].U;
    		Smooth_YUV_Data[M_Mode][2][SL_id][0] = A_L_I3[M_Mode][SL_id].YC_Temp[0].V;

       		SL_Top_cnt++;
       	}
       	else { 										//Bottom
    		A_L_I3[M_Mode][SL_id].YC_Temp[1].Y = (buf_p->Y << 13) / buf_p->Sum;
    		A_L_I3[M_Mode][SL_id].YC_Temp[1].U = (buf_p->U << 13) / buf_p->Sum;
    		A_L_I3[M_Mode][SL_id].YC_Temp[1].V = (buf_p->V << 13) / buf_p->Sum;

    		Smooth_YUV_Data[M_Mode][0][SL_id][1] = A_L_I3[M_Mode][SL_id].YC_Temp[1].Y;
    		Smooth_YUV_Data[M_Mode][1][SL_id][1] = A_L_I3[M_Mode][SL_id].YC_Temp[1].U;
    		Smooth_YUV_Data[M_Mode][2][SL_id][1] = A_L_I3[M_Mode][SL_id].YC_Temp[1].V;

       		SL_Btm_cnt++;
       	}
    }
    else {
    	//db_debug("check_SLT_40point: SL_id=%d sum=%d\n", SL_id, buf_p->Sum);    // 切換時, sum=0
       	A_L_I3[M_Mode][SL_id].YC_Sub.Y = 0;
       	A_L_I3[M_Mode][SL_id].YC_Sub.U = 0;
       	A_L_I3[M_Mode][SL_id].YC_Sub.V = 0;
    }
    SL_cnt++;
}

int Sensor_RGB_Table[2][5][3] = {
  {{S0_R_IO_ADDR, S0_G_IO_ADDR, S0_B_IO_ADDR}, 
   {S1_R_IO_ADDR, S1_G_IO_ADDR, S1_B_IO_ADDR}, 
   {S2_R_IO_ADDR, S2_G_IO_ADDR, S2_B_IO_ADDR}, 
   {S3_R_IO_ADDR, S3_G_IO_ADDR, S3_B_IO_ADDR}, 
   {S4_R_IO_ADDR, S4_G_IO_ADDR, S4_B_IO_ADDR}}, 
  {{S0_R2_IO_ADDR, S0_G2_IO_ADDR, S0_B2_IO_ADDR}, 
   {S1_R2_IO_ADDR, S1_G2_IO_ADDR, S1_B2_IO_ADDR}, 
   {S2_R2_IO_ADDR, S2_G2_IO_ADDR, S2_B2_IO_ADDR}, 
   {S3_R2_IO_ADDR, S3_G2_IO_ADDR, S3_B2_IO_ADDR}, 
   {S4_R2_IO_ADDR, S4_G2_IO_ADDR, S4_B2_IO_ADDR}}
};
int Read_Sensor_RGB(int mode, int sensor, int rgb)
{
    int addr, err=0;
    unsigned value=0;
    if(mode < 0 || mode > 1) err = 1;
    if(sensor < 0 || sensor > 4) err = 1;
    if(rgb < 0 || rgb > 2) err = 1;
    if(err == 1){
        db_error("Read_Sensor_RGB: err! mode=%d sensor=%d rgb=%d\n", mode, sensor, rgb);
        return 0;
    }

    addr = Sensor_RGB_Table[mode][sensor][rgb];
    spi_read_io_porcess_S2(addr, &value, 4);

    return value;
}
int get_video_image_mblock(void)
{
    int mblock;
    int M_Mode, S_Mode;
    Get_M_Mode(&M_Mode, &S_Mode);
    mblock = ST_Header[M_Mode].Sum[ST_H_YUV][1];
    return mblock;
}
int Delta_R = 0, Delta_G = 0, Delta_B = 0;
int Adjust_R_Gain = ADJUST_R_GAIN_DEFAULT;  // =0
int Adjust_G_Gain = ADJUST_G_GAIN_DEFAULT;  // =0
int Adjust_B_Gain = ADJUST_B_GAIN_DEFAULT;  // =0, 數值範圍 0~255
int InitImageState = 0;
int Awb_Quick_Times = 0;
unsigned char AWB_buf[2][0x28000];          // 160kb

void Set_Init_Image_State(int state) {
	InitImageState = state;
}

int Get_Init_Image_State() {
	return InitImageState;
}

extern int get_Line_YUV_Addr_Offset(void);
/*
 * rex+s 160316
 *   取得目前影像縫合後的YUV參數
 *   st_mode = M_Mode or S_Mode (0->12K, 1->8K, 2->6K, 3->4K, 4->3K, 5->2K)
 *   run_big_smooth
 */
void readAWBbuf(int st_mode)
{
    int i, j, k, f_id, base_addr;
    int mblock = get_video_image_mblock();
    int mbsize = (mblock << 3) + 2048;
    if(mbsize > 0x4000){
        db_error("readAWBbuf: overflow! mbsize=%d\n", mbsize);
        return;
    }

    int yuv_ost=0; char *buf;
    int s2en = run_Line_YUV_Offset_Step_S2(&yuv_ost);      // readAWBbuf

    for(f_id = 0; f_id < 2; f_id++) {
        base_addr = (f_id == 0)? F0_YUV_LINE_YUV_ADDR: F1_YUV_LINE_YUV_ADDR;
        base_addr += yuv_ost;
        buf = &AWB_buf[f_id][yuv_ost];
        for(i = 0; i < mbsize; i += 2048, buf += 2048){
            ua360_spi_ddr_read(base_addr + i, buf, 2048, 2, 0);
        }
    }
    
    int id, id_ng_cnt=0, slt_cnt[2]={0};
    int st_sum, st_sum_max;
    unsigned st_start_idx;
    struct US360_Stitching_FeedBack *buf_p;
    struct US360_Stitching_Table *tbl_p;
    for(f_id = 0; f_id < 2; f_id++) {
        buf_p = (struct US360_Stitching_FeedBack *)&AWB_buf[f_id][yuv_ost];
        id = buf_p->ID;
        st_sum = ST_Header[st_mode].Sum[ST_H_YUV][f_id];
        if(st_sum <= 0) {
        	db_error("get_avg_rgby: err-1, st_mode=%d f_id=%d sum=%d\n", st_mode, f_id, st_sum);
        	continue;
        }
        //id = id & 7;
        st_sum_max = (st_sum < mblock)? st_sum: mblock;

        st_start_idx = ST_Header[st_mode].Start_Idx[ST_H_YUV];
        if(st_start_idx >= Stitch_Block_Max){
            db_error("get_avg_rgby: err-2, st_start_idx=%d\n", st_start_idx);
            continue;
        }
		
		tbl_p = (struct US360_Stitching_Table *)&ST_I[f_id][st_start_idx];
		for(j = 0; j < st_sum_max; j++, buf_p++, tbl_p++) {
			if(buf_p->ID != tbl_p->CB_Block_ID){
				id_ng_cnt++;
				break;
			}
//			else if(tbl_p->CB_Mask == 1){
//				mask_cnt++;
//			}
			else if(SLT[f_id][st_start_idx+j].CB_Mode == 1) {	//色縫合
				check_SLT_40point((int *)buf_p, st_mode, &SLT[f_id][st_start_idx+j]);
				slt_cnt[f_id]++;
			}
		}
        //int s3en = chk_Line_YUV_Offset_Step_S3();
        //if(s3en != 0 && s2en > 0 && s2en < 4){
        //    db_debug("readAWBbuf: st_mode=%d yuv_ost=0x%x slt_cnt[%d]=%d\n", st_mode, yuv_ost, f_id, slt_cnt[f_id]);
        //}
    }
}
int Y_Off_Value = 50, Lst_Y_Off_V = 0;
int Y_Table[256], Y_Off_50[256];
unsigned short Sensor_Y[5][256];            // 5顆sensor, 256筆short資料
int AWB_CrCb_G = 512;
int AWB_DIV=12, AWB_Stable=0, LOGE_Enable=0;
//tmp extern int rec_state;

unsigned All_SeY_Pixel[256];                // rex+ 191218, 5顆sensor加總目前亮度值
unsigned All_SeY_V[256];                    // 計算預測亮度值
/*
 * *SeY = unsigned short Sensor_Y[5][256]
 */
int Gamma_Table_Init = 1;
void read_Sensor_Y(unsigned short *SeY, unsigned long long *TsM, unsigned long long *TsP)
{
    int i, j;
    unsigned base_addr, pixel;
    unsigned long long TsumM=0, TsumP=0;

    // 1.Y亮度運算
    if(Gamma_Table_Init == 1){
        Gamma_Table_Init = 0;
        // gamma table
        // 0->0, 64->134, 128->186, 192->221, 255->255
        for(j = 0; j < 256; j++){
            if     (j ==   0){ Y_Table[j] =      0; }                   // gamma_table "多" *64, 後面要除回來
            else if(j  <  64){ Y_Table[j] =      0 + ((j-0)*134); }     // /64
            else if(j ==  64){ Y_Table[j] = 134*64; }
            else if(j  < 128){ Y_Table[j] = 134*64 + ((j-64)*52); }     // /64
            else if(j == 128){ Y_Table[j] = 186*64; }
            else if(j  < 192){ Y_Table[j] = 186*64 + ((j-128)*35); }    // /64
            else if(j == 192){ Y_Table[j] = 221*64; }
            else if(j  > 192){ Y_Table[j] = 221*64 + ((j-192)*35); }    // /64, 221+(194-192)*35/64=222 , 221+(255-192)35/64=255
        }
    }
    // 製作高(低)亮度減權參數
    if(Lst_Y_Off_V != Y_Off_Value){
        Lst_Y_Off_V = Y_Off_Value;
        //int mul = (128 * Y_Off_Value) / 100;
        for(j = 0; j < 256; j++){
            if(j  < 64) Y_Off_50[j] = 64 + j;                   // 190103, miller 高低亮度減權改成50%, 亮度64=100%
            if(j == 64) Y_Off_50[j] = 128;
            if(j  > 64) Y_Off_50[j] = 128 - ((j-63)/3);
        }
    }
    memset(&All_SeY_Pixel[0], 0, sizeof(All_SeY_Pixel));
    for(i = 0; i < 5; i++){
        switch(i){
        case 0: base_addr = 0xA0400; break;         // SENSOR 0, G(x9)(/8)=G(x1.125)=Y(近似)
        case 1: base_addr = 0xA0200; break;         // SENSOR 1
        case 2: base_addr = 0x80000; break;         // SENSOR 2
        case 3: base_addr = 0xA0000; break;         // SENSOR 3
        case 4: base_addr = 0x80200; break;         // SENSOR 4
        }
        spi_read_io_porcess_S2(base_addr, (int *)SeY, 512);

        // max: 0xB810(pixel/128/2)*256(value)*5(sensor)=0x3985000
        // 1*1 + 2*2 + 3*3 = 1+4+9 -> 14/6 = 2.8
        if(i == 1 || i == 2 || i == 4){     // 取中央計算亮度, sensor0和sensor3不計算
            for(j = 0; j < 256; j++){
                pixel = (SeY[j] * Y_Off_50[j]) >> 7;  // pixel減權, 0->50%, 128->100%, 256->50%
                if(pixel > 0xB810){
                    db_debug("i=%d j=%d pixel=%x Sensor_Y=%x off_50=%x\n", i, j, pixel, SeY[j], Y_Off_50[j]);
                    break;
                }
                TsumM += (pixel * Y_Table[j]);
                TsumP += (pixel);
                All_SeY_Pixel[j] += SeY[j];        // rex+ 191218
            }
        }
        SeY += 256;     // +512byte
    }
    *TsM = TsumM;
    *TsP = TsumP;
}

int get_avg_rgby(void)
{
    unsigned i, j, k, w, h;
    ISP_Stati_Struct *ISP_All_State = &ISP_All_State_D;


    /*
     * sensor binning
     * 1x1 3968*3040 = 12062720(0xB81000)/128=94240/2=47120(0xB810) [總和Pixel]
     * 2x2 1920*1504 =  2887680(0x2C1000)/128=22560/2=11280(0x2C10)
     * 3x3 1152* 992 =  1142784(0x117000)/128= 8928/2= 4464(0x1170)
     */
    unsigned TsumY=0;
    unsigned long long TsumM=0, TsumP=0;

    read_Sensor_Y(&Sensor_Y[0][0], &TsumM, &TsumP);

    //if(Get_HDR7P_Auto_Read_En() == 1)
    //	Copy_to_HDR7P_Sensor_Y(&Sensor_Y[0][0]);

//tmp    if(rec_state < 0 && getWhiteBalanceMode() == 0)
    	Do_Next_Color_T();

    if(TsumP > 0)
        TsumY = (TsumM>>6) / TsumP;                // /64

    if(TsumY > 255) TsumY = 255;
    else if(TsumY < 1) TsumY = 1;
    
    
    // 2.RGB運算
    int cnt = 0;
    int RGB[2][5][3] = {0};
    long long RGB_T[2][3] = {0}, RGB_Avg[2][3] = {0};
    int scale = Get_Input_Scale();
    w = (SENSOR_BINN_FS_W_PIXEL / scale);         //W=4432 / 1,2,3
    h = (SENSOR_BINN_FS_H_PIXEL / scale);         //H=3354 / 1,2,3

    // 彩度(RGB)原始數值
    // (4432*3354)=0xE2D220*(0x100)/2=0x71691000        // 數值=0~0xff, RGGB=2點取1個數值
    for(j = 0; j < 3; j++) {            //RGB
        RGB_T[0][j] = 0;
        for(i = 1; i < 5; i++) {        //Sensor
            // 彩度(RGB)依亮度減權後數值
            RGB[0][i][j] = Read_Sensor_RGB(0, i, j);    // 0:無減權 1:高彩度減權
            RGB_T[0][j] += RGB[0][i][j];
        }
        RGB_Avg[0][j] = RGB_T[0][j] * 2 / 4 / w;        // 4顆SENSOR取平均, 除以SENSOR BINNING解析度, 乘2(RGGB...)
                                                        // 先除'w', 後面運算再除'h'
        cnt = 0;
        for(i = 1; i < 5; i++) {                        //Sensor
            if(i == 1 || i == 3){                       // 取中央計算亮度, sensor0和sensor4不計算
                RGB[1][i][j] = Read_Sensor_RGB(1, i, j);
                RGB_T[1][j] += RGB[1][i][j];
                cnt ++;
            }
        }
        RGB_Avg[1][j] = RGB_T[1][j] * 1024 / cnt / w;
    }
    // TH = AWB_WEI_Max = 16, 數值範圍=0xFF(Data)*16(AWB_WEI_Max)=0~4080
    //                     W * H * TH
    //Sum = Pixel_Data * -------------- ->  0xFF(Data) * W * H * 0x20(TH) / 256 / 4(RGGB) 
    //                        256
    
    unsigned r01, g01, b01, r02, g02, b02;
    //rgb2y = (RGB_Avg[0] * 299 +RGB_Avg[1] * 587 + RGB_Avg[2] * 114) / 1000 / h;
    //if(rgb2y > 255) rgb2y = 255;
    //if(rgb2y < 1)   rgb2y = 1;
    //R = Cr + G(2048)
    //B = Cb + G(2048)
    
    //R = Y + 1.402Cr
    //G = Y - 0.344Cb - 0.714Cr
    //B = Y + 1.772Cb
    
    
    r01 = RGB_Avg[0][0] / h;
    g01 = RGB_Avg[0][1] / h;
    b01 = RGB_Avg[0][2] / h;
    //r02 = (RGB_Avg[1][0] / h) + AWB_CrCb_G;
    //g02 = (RGB_Avg[1][1] / h) + AWB_CrCb_G;
    //b02 = (RGB_Avg[1][2] / h) + AWB_CrCb_G;

    if(RGB_Avg[1][0] < 0 && RGB_Avg[1][2] < 0){                     // R<0, B<0, 避免[紫]<->[綠]震盪
        r02 = ((RGB_Avg[1][0] / h) >> 2) + AWB_CrCb_G;
        g02 = (RGB_Avg[1][1] / h) + AWB_CrCb_G;
        b02 = ((RGB_Avg[1][2] / h) >> 2) + AWB_CrCb_G;
    }
    else if(RGB_Avg[1][0] < 0 && RGB_Avg[1][1] < RGB_Avg[1][2]){    // R<0, G<B, 避免藍天過度反應
        r02 = ((RGB_Avg[1][0] / h) >> 2) + AWB_CrCb_G;
        g02 = (RGB_Avg[1][1] / h) + AWB_CrCb_G;
        b02 = ((RGB_Avg[1][2] / h) >> 2) + AWB_CrCb_G;
    }
    else{
        r02 = (RGB_Avg[1][0] / h) + AWB_CrCb_G;
        g02 = (RGB_Avg[1][1] / h) + AWB_CrCb_G;
        b02 = (RGB_Avg[1][2] / h) + AWB_CrCb_G;
    }

    /*//測試機制
    r = RGB_T[1][0]/(3968*3040);
    g = RGB_T[1][1]/(3968*3040);
    b = RGB_T[1][2]/(3968*3040);
    //*/
    unsigned long long r, g, b, rgb_sum;
    static int lst_real_Y=0;

    r = r02;                // 解晰度提高
    g = g02;
    b = b02;

    if(b01 < g01 && r01 < g01 && b02 > g02 && b02 > r02){     // 畫面固有偏藍
        //db_debug("  == blue ==\n");
        r = g = b = g02;
    }
    if(r01 < g01 && r01 < b01 && r02 > g02 && r02 > b02){     // 畫面固有偏紅
        //db_debug("  == red ==\n");
        r = g = b = g02;
    }
    if(g01 < r01 && g01 < b01 && g02 > r02 && g02 > b02){     // 畫面固有偏綠
        //db_debug("  == green ==\n");
        r = g = b = g02;
    }
    if((abs(r01-g01)+abs(b01-g01)+abs(r01-b01) < 6) ||
       (abs(r02-g02)+abs(b02-g02)+abs(r02-b02) < 6))
    {
        AWB_Stable = 1;
    }
    else AWB_Stable = 0;

    if(LOGE_Enable == 1){
        //CIE-XYZ系統
        float m = (0.66697*r01 + 1.13240*g01 + 1.20063*b01);
        float x = (0.49000*r01 + 0.31000*g01 + 0.20000*b01) / m;
        float y = (0.17697*r01 + 0.81240*g01 + 0.01063*b01) / m;
        //float z = (0.01000*g01 + 0.99000*b01) / m;
    
        int sR=0, sG=0, sB=0;
        int Cr01, Cb01, Cr02, Cb02;
        float sq01, sq02;
        Cr01 = r01 - g01;
        Cb01 = b01 - g01;
        Cr02 = r02 - g02;
        Cb02 = b02 - g02;
        sq01 = sqrt((Cr01*Cr01)+(Cb01*Cb01));
        sq02 = sqrt((Cr02*Cr02)+(Cb02*Cb02));
        
        sR = r02*g01/g02;        // 高彩度減權後Cr,Cb, 固有色偏微調
        sG = g01;
        sB = b02*g01/g02;
        
        db_debug("rgb01={%d,%d,%d} rgb02={%d,%d,%d} rgb={%d,%d,%d} div=%d\n", r01, g01, b01, r02, g02, b02, sR , sG, sB, AWB_DIV);
        db_debug("sqrt={%4.3f,%4.3f} gain0={%4.3f,%4.3f,%4.3f} xy={%4.3f,%4.3f}\n", sq01, sq02,
            (float)ISP_All_Set_Reg.ISP_Block1.Gain_R/0x4000, (float)ISP_All_Set_Reg.ISP_Block1.Gain_G/0x4000, (float)ISP_All_Set_Reg.ISP_Block1.Gain_B/0x4000,
            x, y);
    }
//*/
    
    int max = (AWB_CrCb_G*2)-1;
    if(r < 1) r = 1; if(r > max) r = max;
    if(g < 1) g = 1; if(g > max) g = max;
    if(b < 1) b = 1; if(b > max) b = max;
    rgb_sum = r + g + b;

    if(rgb_sum > 0) {
        Delta_R = (r * 0x30000 / rgb_sum) - 0x10000;        // 0x30000
        Delta_G = (g * 0x30000 / rgb_sum) - 0x10000;
        Delta_B = (b * 0x30000 / rgb_sum) - 0x10000;

        ISP_All_State->AVG_YRGB.real_Y = TsumY; //TsumY; //rgb2y;
        ISP_All_State->AVG_YRGB.AVG_R = r;
        ISP_All_State->AVG_YRGB.AVG_G = g;
        ISP_All_State->AVG_YRGB.AVG_B = b;
        ISP_All_State->AVG_YRGB.AVG_Y = TsumY;  //TsumY;  //rgb2y;
    }
    else {
        db_error("get_avg_rgby: err! rgb_sum=%d\n", rgb_sum);
    }
    
    /*static int r02_lst, g02_lst, b02_lst;
    if(r02_lst != r02 || b02_lst != b02){
        r02_lst = r02;
        b02_lst = b02;
        db_debug("awb: 01={%d,%d,%d} 02={%d,%d,%d},{%lld,%lld,%lld} delta={%d,%d,%d} \n", r01, g01, b01, r02, g02, b02,
            RGB_Avg[1][0]/h, RGB_Avg[1][1]/h, RGB_Avg[1][2]/h, Delta_R, Delta_G, Delta_B);
        db_debug("gain0={%04x,%04x,%04x} gain1={%04x,%04x,%04x} awb={%04x,%04x,%04x}\n", 
            ISP_All_Set_Reg.ISP_Block1.Gain_R, ISP_All_Set_Reg.ISP_Block1.Gain_G, ISP_All_Set_Reg.ISP_Block1.Gain_B,    // gain0
            ISP_All_Set_Reg.ISP_Block1.MR    , ISP_All_Set_Reg.ISP_Block1.MG    , ISP_All_Set_Reg.ISP_Block1.MB,        // gain1
            ISP_All_State_D.AWB_Gain.gain_R  , ISP_All_State_D.AWB_Gain.gain_G  , ISP_All_State_D.AWB_Gain.gain_B);     // temp
    }//*/

    return 0;
}
// msec
int get_ep_time(int idx, int freq)
{
	int ep_t=30;
    int ep_val = idx / 1280;     //20*64;
    if(ep_val >= 0) {
    	if(freq == 0) ep_t = 120000 / pow(2, ep_val);           // 1/120, 1/60, 1/30...
    	else          ep_t = 100000 / pow(2, ep_val);
    }
    else {
    	if(freq == 0) ep_t = 120000 * pow(2, abs(ep_val) );     // 1/240, 1/480, 1/960
    	else          ep_t = 100000 * pow(2, abs(ep_val) );
    }
    return ep_t;
}
/*
 * *sRGB = &ISP_All_State_D.AWB_Gain.gain_R 
 * *tRGB = &ISP_All_Set_Reg.ISP_Block1.MR
 */
void set_ISP_Block1_MRGB(unsigned *sRGB, unsigned *tRGB)
{
    int show = 0, sub;
    /*if(*sRGB == *tRGB) return;        // 全部改設定gain0
    if(*sRGB > *tRGB){
        // *sRGB > *tRGB
        if(*sRGB > (*tRGB)+0x400) { *tRGB = *sRGB; }        // *tRGB = *sRGB
        else                      { *tRGB = *tRGB + 1; }

        if(*tRGB >= 0xfc00) *tRGB = 0xfc00;
    }
    else{
        // *sRGB < *tRGB
        if(*sRGB < (*tRGB)-0x400) { *tRGB = *sRGB; }        // *tRGB = *sRGB
        else                      { *tRGB = *tRGB - 1; }

        if(*tRGB <= 0x4000) *tRGB = 0x4000;     // 最少要1倍(0x4000)
    }//*/
}

/*
		{  1.543, 1.0, 2.195},	// Filament Lamp (2500K)
		{  1.647, 1.0, 1.721},	// Daylight Lamp (4500K)
		{  1.715, 1.0, 1.422},	// Sun (6000K)
		{  1.876, 1.0, 1.280},	// Cloudy (7000K)

  2700K gain0={1.125,1.008,2.223}
  5000K gain0={1.435,1.000,2.000}
  6500K gain0={1.685,1.005,1.412}
*/
int RGB_Gain_Range[3][2] = 
{
   {0x8400, 0x4000},        // max: 2.0625(0x8400), min: 1.0625(0x4400)
   {0x4800, 0x4000},
   {0x9800, 0x4000}         // max: 2.375(0x9800), min: 1.500(0x6000)
};

void ISP_AWB(void)
{
    static unsigned long long curTime, lstTime=0, runTime;
    int show_info = 0;
    int d_cnt = read_F_Com_In_Capture_D_Cnt();
//tmp    get_current_usec(&curTime);
    if(curTime < lstTime) lstTime = curTime;    // rex+ 151229, 防止例外錯誤
    else if((curTime - lstTime) >= 500000){
    	show_info = 1;
        lstTime = curTime;
    }
    
    ISP_Stati_Struct *ISP_All_State = &ISP_All_State_D;                 // &isp_state_tmp[0];
    int maxRGB;
    int tR=0, tG=0, tB=0;

    if(d_cnt != 0) return;
    if(WB_SPEED == 0) return;                                           // WB_SPEED=30
    if(ISP_All_State->AVG_YRGB.real_Y < 24 || 
       ISP_All_State->AVG_YRGB.real_Y > 240) return;                    // 低亮度不做調整
    
    //Delta_R => +(-)65535, 0x10000
    maxRGB = Delta_G;							                        // miller+s 140604
    if(Delta_R > maxRGB) maxRGB = Delta_R;
    if(Delta_B > maxRGB) maxRGB = Delta_B;

    //int div = 150;
    //if     (maxRGB > 10000) div = 30;        // RGB差越大, GAIN數值就越大, 加速AWB
    //else if(maxRGB > 1000) div = 60;
    //else if(maxRGB > 100) div = 90;
    //int div = 12;
    
    AWB_DIV = 12;

    if(AWB_Stable == 1){
        AWB_DIV = 36;
    }
    if(ISP_All_Set_Reg.ISP_Block1.Gain_R > (RGB_Gain_Range[0][0]-0x100) && (Delta_R - maxRGB) < 0){ // 避免閃爍
        AWB_DIV = 60;
        Delta_R = maxRGB;
    }
    if(ISP_All_Set_Reg.ISP_Block1.Gain_R < (RGB_Gain_Range[0][1]+0x100) && (Delta_R - maxRGB) > 0){
        AWB_DIV = 60;
        Delta_R = maxRGB;
    }
    if(ISP_All_Set_Reg.ISP_Block1.Gain_B > (RGB_Gain_Range[2][0]-0x100) && (Delta_B - maxRGB) < 0){
        AWB_DIV = 60;
        Delta_B = maxRGB;
    }
    if(ISP_All_Set_Reg.ISP_Block1.Gain_B < (RGB_Gain_Range[2][1]+0x100) && (Delta_B - maxRGB) > 0){
        AWB_DIV = 60;
        Delta_B = maxRGB;
    }

//tmp    if(rec_state >= 0)                                      // 錄影中
        AWB_DIV = 120;                                      // 調整再減半

    if(Awb_Quick_Times != 0){
    	Awb_Quick_Times--;
    	AWB_DIV = 5;
    }
    
    tR = ISP_All_State->AWB_Gain.gain_R - 0x4000;
    tG = ISP_All_State->AWB_Gain.gain_G - 0x4000;
    tB = ISP_All_State->AWB_Gain.gain_B - 0x4000;
    
    tR = tR - ( (Delta_R - maxRGB) / AWB_DIV);
    tG = tG - ( (Delta_G - maxRGB) / AWB_DIV);
    tB = tB - ( (Delta_B - maxRGB) / AWB_DIV);
/*
    if(tR <= -(0x4000) || tG <= -(0x4000) || tB <= -(0x4000) ) {
    	db_error("ISP_AWB: err! gain_R=%d gain_G=%d gain_B=%d\n", tR, tG, tB);
    	tR = tG = tB = 0;
    }
    if(tR >= 0x800 && tG >= 0x800 && tB >= 0x800){
        tR -= (tR>>3);
        tG -= (tG>>3);
        tB -= (tB>>3);
    }
*/
    ISP_All_State->AWB_Gain.gain_R  = (0x4000 + tR);
    ISP_All_State->AWB_Gain.gain_G  = (0x4000 + tG);
    ISP_All_State->AWB_Gain.gain_B  = (0x4000 + tB);

    if(ISP_All_State->AWB_Gain.gain_R > 0xFFFF) ISP_All_State->AWB_Gain.gain_R = 0xFFFF;
    if(ISP_All_State->AWB_Gain.gain_G > 0xFFFF) ISP_All_State->AWB_Gain.gain_G = 0xFFFF;
    if(ISP_All_State->AWB_Gain.gain_B > 0xFFFF) ISP_All_State->AWB_Gain.gain_B = 0xFFFF;

    /*if(show_info == 1){
        int MR = ISP_All_Set_Reg.ISP_Block1.MR;
        int MG = ISP_All_Set_Reg.ISP_Block1.MG;
        int MB = ISP_All_Set_Reg.ISP_Block1.MB;
        int GR = ISP_All_Set_Reg.ISP_Block1.Gain_R;
        int GG = ISP_All_Set_Reg.ISP_Block1.Gain_G;
        int GB = ISP_All_Set_Reg.ISP_Block1.Gain_B;
        int vY = ISP_All_State_D.AVG_YRGB.AVG_Y;
        int vR = ISP_All_State_D.AVG_YRGB.AVG_R;
        int vG = ISP_All_State_D.AVG_YRGB.AVG_G;
        int vB = ISP_All_State_D.AVG_YRGB.AVG_B;
        int wR = ISP_All_State_D.AWB_Gain.gain_R;
        int wG = ISP_All_State_D.AWB_Gain.gain_G;
        int wB = ISP_All_State_D.AWB_Gain.gain_B;
        db_debug("get_avg_rgby: Y=%d RGB={%x,%x,%x}{%x,%x,%x} \n", vY, vR, vG, vB, tR, tG, tB);
        db_debug("              Delta={%d,%d,%d} AWB={%x,%x,%x}\n", Delta_R, Delta_G, Delta_B, wR, wG, wB);
        db_debug("              M={%x,%x,%x} G={%x,%x,%x}\n", MR, MG, MB, GR, GG, GB);
    }//*/
    //if(maxRGB != Delta_R) set_ISP_Block1_MRGB(&ISP_All_State->AWB_Gain.gain_R, &ISP_All_Set_Reg.ISP_Block1.MR);
    //if(maxRGB != Delta_G) set_ISP_Block1_MRGB(&ISP_All_State->AWB_Gain.gain_G, &ISP_All_Set_Reg.ISP_Block1.MG);
    //if(maxRGB != Delta_B) set_ISP_Block1_MRGB(&ISP_All_State->AWB_Gain.gain_B, &ISP_All_Set_Reg.ISP_Block1.MB);

}
int AEG_B_Exp_1Sec=4, AEG_B_Exp_Gain=0;
void get_AEG_B_Exp(int *sec, int *gain)
{
    *sec = AEG_B_Exp_1Sec;
    *gain = AEG_B_Exp_Gain;             // 64*20=1280=1倍
}

int AEG_EP_manual=0, AEG_EP_idx=0;
int AEG_EP_FPS=300;
int AEG_Gain_manual=0, AEG_Gain_idx=(50*64);
int ISP_AEG_EP_IMX222_Freq = 0;	// 0:60Hz 1:50Hz

void get_AEG_UI_Setting(int *exp_mul, int *exp_idx, int *iso_mul, int *iso_idx, int *bulb_sec, int *bulb_iso)
{
    *exp_mul = AEG_EP_manual;
    *exp_idx = AEG_EP_idx;
    *iso_mul = AEG_Gain_manual;
    *iso_idx = AEG_Gain_idx;
    *bulb_sec = AEG_B_Exp_1Sec;
    *bulb_iso = AEG_B_Exp_Gain;
}
// 60Hz	    50Hz	AEG_EP_idx	曝光時間
// 1/32000  1/25600    -8 = -160
// 1/16000  1/12800    -7 = -140
// 1/8000   1/6400     -6 = -120
// 1/4000   1/3200     -5 = -100
// 1/2000   1/1600     -4 = -80
// 1/1000   1/800      -3 = -60
// 1/480    1/400      -2 = -40
// 1/240    1/200      -1 = -20
// 1/120    1/100       0 = 0
// 1/60	    1/50        1 = 20
// 1/30	    1/25        2 = 40
// 1/15	    1/12.5      3 = 60
// 1/8      1/6.25      4 = 80
// 1/4	    1/3	    	5 = 100
// 1/2	    1/2	    	6 = 120
// 1	    1	    	7 = 140
// 2	    2	    	8 = 160
// 4	    4	    	9 = 180

int Adj_Y_target = 20;
void SetAdjYtarget(int y) {
	Adj_Y_target = y;
}
int GetAdjYtarget(void) {
	return Adj_Y_target;
}

int     AEG_Y_target = AEG_Y_TARGET_DEFAULT;        // default: 0x64
float   AEG_Y_ev = AEG_Y_TARGET_EV_DEFAULT;         // default: 1.0


int AEG_gain_H = (60*64);       // 64*20=1280=1倍
int AEG_EP_FRM_LENGTH = 0;	// EP_FRM_LENGTH_60Hz_D3_DEFAULT;
int AEG_EP_INTEG_TIME = 0;


extern struct Test_Tool_Cmd_Struct_H TestToolCmd;
extern int testtool_set_AEG, testtool_set_gain;
int AEG_idx = (60*64), AEG_idx_Lst = (60*64);	    //EP:0 Gain:10, AEG_idx調整亮度使用
int PipeCmd_AEG = (60*64);
int PipeCmd_AEG_Max, PipeCmd_AEG_Min, PipeCmd_AEG_GainMax;
void set_PipeCmd_AEG_idx(int aeg_idx, int aeg_max, int aeg_min, int gain_max)
{
    PipeCmd_AEG         = AEG_idx;
    PipeCmd_AEG_Max     = aeg_max;
    PipeCmd_AEG_Min     = aeg_min;
    PipeCmd_AEG_GainMax = gain_max;
}
void get_PipeCmd_AEG_idx(int *aeg, int *aeg_max, int *aeg_min, int *gain_max)
{
	*aeg      = PipeCmd_AEG;
	*aeg_max  = PipeCmd_AEG_Max;
	*aeg_min  = PipeCmd_AEG_Min;
	*gain_max = PipeCmd_AEG_GainMax;
}
int run_ep_alignment(int freq, int ep_in)
{
    int ep_out = ep_in;
    if(freq == 0) {     // NTSC
		if(ep_in >= 7680000)      ep_out = 8000000;
		else if(ep_in >= 3840000) ep_out = 4000000;
		else if(ep_in >= 1920000) ep_out = 2000000;
		else if(ep_in >= 960000)  ep_out = 1000000;
		else if(ep_in >= 480000)  ep_out = 500000;
		else if(ep_in >= 240000)  ep_out = 250000;
		else if(ep_in >= 120000)  ep_out = 120000;
		else if(ep_in >= 60000)   ep_out = 60000;
		else if(ep_in >= 30000)   ep_out = 30000;
		else if(ep_in >= 15000)   ep_out = 15000;
		else if(ep_in >= 7500)    ep_out = 8000;
		else if(ep_in >= 3750)    ep_out = 4000;
		else if(ep_in >= 1875)    ep_out = 2000;
		else if(ep_in >= 937)     ep_out = 1000;
		else if(ep_in >= 468)     ep_out = 500;
		else if(ep_in >= 234)     ep_out = 250;
	}
	else {              // PAL
		if(ep_in >= 6400000)      ep_out = 6400000;
		else if(ep_in >= 3200000) ep_out = 3200000;
		else if(ep_in >= 1600000) ep_out = 1600000;
		else if(ep_in >= 800000)  ep_out = 800000;
		else if(ep_in >= 400000)  ep_out = 400000;
		else if(ep_in >= 200000)  ep_out = 200000;
		else if(ep_in >= 100000)  ep_out = 100000;
		else if(ep_in >= 50000)   ep_out = 50000;
		else if(ep_in >= 25000)   ep_out = 25000;
		else if(ep_in >= 12500)   ep_out = 12000;
		else if(ep_in >= 6250)    ep_out = 6000;
		else if(ep_in >= 3125)    ep_out = 3000;
		else if(ep_in >= 1562)    ep_out = 2000;
		else if(ep_in >= 781)     ep_out = 1000;
		else if(ep_in >= 390)     ep_out = 500;
		else if(ep_in >= 195)     ep_out = 250;
	}
    return ep_out;
}

int Focus_EP_Idx = 20*64, Focus_Gain_Idx = 0;
void setFocusEPIdx(int value)
{
	Focus_EP_Idx = value;
}

int getFocusEPIdx(void)
{
	return Focus_EP_Idx;
}

void setFocusGainIdx(int value)
{
	Focus_Gain_Idx = value;
}

int getFocusGainIdx(void)
{
	return Focus_Gain_Idx;
}

void cal_exp_iso(int freq, int exp_idx, int gain_idx, int c_mode, int b_exp, int b_iso, 
                 int *exp_n, int *exp_m, int *iso)
{
    if(c_mode == 12 && b_exp > 0){
        *iso = pow(2, (((float)(b_iso))/1280.0)) * 100;
        *exp_n = b_exp;
        *exp_m = 1;
    }
    else{
        if(gain_idx < 0) gain_idx = 0;
        *iso = (pow(2, (((float)(gain_idx))/1280.0)) * 100);
        if(exp_idx >= 140*64){      // >= 1s ... 4s
            *exp_n = pow(2, (((float)(exp_idx-(140*64)))/1280.0));
            *exp_m = 1;
        }
        else if(exp_idx >= 80*64){  // >= 1/8, 1/8 ... 1/2
            *exp_n = 1;
            *exp_m = (128.0 / pow(2, (((float)(exp_idx))/1280.0)));
        }
        else if(exp_idx >= 0){      // >= 1/120, /n, 1/120 ... 1/15
            *exp_n = 1;
            *exp_m = (120.0 / pow(2, (((float)(exp_idx))/1280.0)));
        }
        else{                       // < 1/120, *n
            *exp_n = 1;            
            *exp_m = (120.0 * pow(2, (((float)(abs(exp_idx)))/1280.0)));
        }
        if(freq == 1 && *exp_n == 1){  //PAL顯示修正
            if(*exp_m > 3 && *exp_m < 12000){      // 1/4 ~ 1/8000
                int m = *exp_m;
                *exp_m = (m * 5) /6;
            }
        }
    }
}

extern int Save_Jpeg_Now_Cnt, Save_Jpeg_End_Cnt;
static int ep_init_change=0;
int gain_tmp_idx=0, exp_tmp_idx=0;
int Cap_Gain_Idx=0;

int SeY_255_Default=512;                // rex+ 191218, 亮度統計256代表512

/*
 * 外部: All_SeY_Pixel[]
 *       All_SeY_V[]
 *       SeY_255_Default
 */
int predict_AEG_calc(int aeg_idx_now, int aeg_idx_max, int aeg_idx_min, int aeg_y_target)
{
    int i, j, k;
    // rex+ 191016, 新AEG演算法
    unsigned long long TsumM=0, TsumP=0, lltmp;
    unsigned TsumY=0;
    int aeg_idx2;
    unsigned totP1=0, totP2=0;
            
    // All_SeY_V[] = 運算中途數值
    // All_SeY_Pixel[] = 原始數值
    for(j = 0; j < 256; j++){               // 計算總數
       totP1 += All_SeY_Pixel[j];
    }

    int mul_idx2=0;
    int mul_x=0;
    int y_tmp[384]={0,}, y_min=999;
    int src_i, lst_i, lst_j, sum_p;
    int aeg_cnt=0, aeg_val=0, sum_y2i=0;
    //for(i = 0; i < 384; i++){
    i = 192;
    int r=0, jump=96, step=0, ci=0;
    do{
        // step.1 將各亮度統計pixel寫入All_SeY_V[]中
        aeg_idx2 = aeg_idx_now-3840+i*20;                           // (3*20*64)=3840=x8
        mul_idx2 = pow(2, (float)(i-192)/64.0)*1000;            /// 125~8000

        // 調整計算256個亮度統計數值
        memset(&All_SeY_V[0], 0, sizeof(All_SeY_V));
        lst_i=-1; lst_j=-1;
        for(j = 0; j < 256; j++){
            if(mul_idx2 == 0)     src_i = j;
            else                  src_i = j*1000/mul_idx2;
            // ex.
            //   亮pix往暗pix移動, 模擬+AE的情況
            //   tar[0,1,2,3...] = src[0,2,4,6...]
            //   tar[0] = src[0:1]
            //   暗pix往亮pix移動, 模擬-AE的情況
            //   tar[0,2,4,6...] = src[0,1,2,3...]
            //   tar[255] = src[128:255]

            if(src_i < 256 && src_i != lst_i){
                sum_p = 0;
                for(k = lst_i+1; k < src_i; k++){
                    sum_p += All_SeY_Pixel[k];
                    totP2 += All_SeY_Pixel[k];
                }
                All_SeY_V[j] = All_SeY_Pixel[src_i] + sum_p;
                totP2 += All_SeY_Pixel[src_i];
                lst_i = src_i;
                lst_j = j;
            }
            if(j == 255){
                if((mul_idx2 >= 1000) && totP1 > totP2){
                    All_SeY_V[255] += (totP1 - totP2);
                }
                else if((mul_idx2 < 1000) && (lst_j+1) < 255 && (totP1 > totP2)){
                    All_SeY_V[lst_j+1] += (totP1 - totP2);
                }
            }
        }
        totP2 = 0;  // debug break point
        
        // step.2 開始計算亮度值
        TsumM = 0; TsumP = 0;
        for(j = 0; j < 248; j++){           // 0~240
            TsumM += (All_SeY_V[j] * Y_Table[j]);
            TsumP += (All_SeY_V[j]);
        }
        for(j = 248; j < 256; j++){         // 240以後, 計算需加權3倍
            // lltmp避免int overflow
            lltmp = (unsigned long long)All_SeY_V[j] * (unsigned long long)Y_Table[j];
            lltmp = lltmp * (unsigned long long)((SeY_255_Default>>3)*(j-248+1)+256) / 256;
            TsumM += lltmp;
            lltmp = (unsigned long long)All_SeY_V[j];
            lltmp = lltmp * (unsigned long long)((SeY_255_Default>>3)*(j-248+1)+256) / 256;
            TsumP += lltmp;
        }
        if(TsumP > 0){
            TsumY = (TsumM>>6) / TsumP;                // /64
            if(TsumY > 255) TsumY = 255;
            else if(TsumY < 1) TsumY = 1;
        }
        else{
            //db_error("TsumP Err! TsumP=%d\n", TsumP);
            break;
        }
        y_tmp[i] = TsumY;

        if(TsumY == aeg_y_target){           //ISP_All_Command->AEG_Y_target){
            //aeg_idx_now = aeg_idx2;
            aeg_val += aeg_idx2;
            aeg_cnt ++;
            sum_y2i += i;
        }
        else{
            if(abs(TsumY - aeg_y_target) < y_min){     // 找最接近AEG_Y_target的數值
                y_min = abs(TsumY - aeg_y_target);
                //aeg_idx_now = aeg_idx2;
                aeg_val = aeg_idx2;
                aeg_cnt = 1;
                sum_y2i = i;
            }
            else if(abs(TsumY - aeg_y_target) == y_min){
                aeg_val += aeg_idx2;
                aeg_cnt ++;
                sum_y2i += i;
            }
        }
        if(step == 0){              // 跳躍尋找接近的數值
            if(jump == 1){
                if(aeg_cnt > 0) ci = sum_y2i / aeg_cnt;
                else            ci = i;
                if(TsumY < aeg_y_target){
                    i = ci;
                    step = 3;
                    //db_debug("step=%d i=%d r=%d\n", step, i, r);
                }
                else if(TsumY > aeg_y_target){
                    i = ci;
                    step = 4;
                    //db_debug("step=%d i=%d r=%d\n", step, i, r);
                }
            }
            if(TsumY == aeg_y_target){
                step = 1;           // 進入逐格+1搜尋
                //db_debug("step=%d i=%d r=%d\n", step, i, r);
                ci = i;
                i ++;
            }
            else if(TsumY > aeg_y_target){
                i -= jump;
            }
            else if(TsumY < aeg_y_target){
                i += jump;
            }
        }
        else{
            if(step == 1 || step == 3){
                if(TsumY == aeg_y_target){
                    //db_debug("step=%d i=%d r=%d\n", step, i, r);
                    i ++;
                }
                else{ 
                    if(step == 1){
                        step = 2;      // 進入逐格-1搜尋
                        i = ci-1;
                    }
                    else if(step == 3){
                        if(TsumY < aeg_y_target){
                            //db_debug("step=%d i=%d r=%d\n", step, i, r);
                            i ++;
                        }
                        else break;
                    }
                }
            }
            else if(step == 2 || step == 4){
                if(TsumY == aeg_y_target){
                    //db_debug("step=%d i=%d r=%d\n", step, i, r);
                    i --;
                }
                else{
                    if(step == 2){
                        break;
                    }
                    else if(step == 4){
                        if(TsumY > aeg_y_target){
                            //db_debug("step=%d i=%d r=%d\n", step, i, r);
                            i --;
                        }
                        else break;
                    }
                }
            }
        }
        if(i >= 384 || i < 0) break;

        jump = (jump+1) >> 1;
        r++;
    } while(r < 192);
    // for(i = 0; i < 384; i++){
    
    if(aeg_cnt > 0){
        int y2i = sum_y2i / aeg_cnt;
        db_debug("r=%d step=%d AEG=%d->%d [192]=%d y_tmp[..%d..]={%d,%d,%d,[%d],%d,%d,%d}\n", r, step, 
            aeg_idx_now, aeg_val / aeg_cnt, 
            y_tmp[192],
            y2i, 
            y_tmp[(y2i-3)&0x1ff], y_tmp[(y2i-2)&0x1ff], y_tmp[(y2i-1)&0x1ff], 
            y_tmp[y2i], 
            y_tmp[(y2i+1)&0x1ff], y_tmp[(y2i+2)&0x1ff], y_tmp[(y2i+3)&0x1ff]);
        aeg_idx_now = aeg_val / aeg_cnt;
    }

    //aeg_idx_now = aeg_idx2/* - (160*64)*/;
    if(aeg_idx_now > aeg_idx_max){
        aeg_idx_now = aeg_idx_max;
    }
    else if(aeg_idx_now < aeg_idx_min){
        aeg_idx_now = aeg_idx_min;
    }

    return aeg_idx_now;
}
/*
 * gain_tmp_idx = 0*20*64 iso100    exp_tmp_idx = 9*20*64 4s
 *                1*20*64 iso200                  8*20*64 2s
 *                2*20*64 iso400                  7*20*64 1s
 *                3*20*64 iso800                  6*20*64 1/2
 *                4*20*64 iso1600                 5*20*64 1/4
 *                5*20*64 iso3200                 4*20*64 1/8
 *                6*20*64 iso6400                 3*20*64 1/15
 *                                                2*20*64 1/30
 *                                                1*20*64 1/60
 *                                                0*20*64 1/120  (>=0)
 *                                                ...            (n<0)
 */

int ISP_AEG_IMX222( int c_mode, int m_mode)
{
    static unsigned long long lstTime, curTime, runTime;
    int aeg_idx_max, aeg_idx_min;

    static int AEG_target_M, AEG_target_H, AEG_target_L, AEG_target_C_H, AEG_target_C_L;

//tmp    int picture = get_C_Mode_Picture(c_mode);
	int picture = 1;
    int i, n, quick=0, temp, temp1=0, temp2=0, chang_en=0;
    int exp_max=(120*64), exp_min=(-160*64), gain_max=(120*64), gain_min=0;
    int d_cnt = read_F_Com_In_Capture_D_Cnt();
    int fps = getFPS();

    ISP_Command_Struct *ISP_All_Command = &ISP_All_Command_D;       // &isp_cmd_tmp[0];
    ISP_Stati_Struct *ISP_All_State = &ISP_All_State_D;             // &isp_state_tmp[0];

    
    if(ISP_All_State->AVG_YRGB.real_Y < 0 || ISP_All_State->AVG_YRGB.real_Y > 255){
        db_error("ISP_AEG_IMX222: return! c_mode=%d real_Y=%d\n", c_mode, ISP_All_State->AVG_YRGB.real_Y);
        return 1;
    }
    if(Check_Is_HDR_Mode(c_mode) == 1) ISP_All_Command->AEG_Y_target = AEG_Y_target * AEG_Y_ev;						//HDR
    else							   ISP_All_Command->AEG_Y_target = (AEG_Y_target + Adj_Y_target) * AEG_Y_ev;	//WDR
    if(ISP_All_Command->AEG_Y_target > 255)    ISP_All_Command->AEG_Y_target = 255;
    else if(ISP_All_Command->AEG_Y_target < 0) ISP_All_Command->AEG_Y_target = 0;
    
    int iso, exp_n, exp_m;
    int freq = ISP_AEG_EP_IMX222_Freq;                              // 0:60Hz 1:50Hz
    int sflash = (((60-(10*freq))*2)*10)/fps;                       // ((60-(10*freq))*2)=120(or 100), freq: 0->60Hz, 1->50Hz
                                                                    // sflash: 4 = 30fps, 120/30=4
    int qmax=12;                                                    //         12 = 10fps, 120/10=12
                                                                    //         120 = 1fps, 120/1=120
//tmp    if(c_mode == 1 || c_mode == 10)                 // 錄影
//tmp        qmax = (rec_state < 0)? 12: 6;              // 錄影中調整減半
//tmp    else if(c_mode == 2 || c_mode == 11)            // 縮時
//tmp        qmax = (rec_state < 0)? 12: 3;              // 縮時調整再減半

    static int Last_gain_min=0, Last_long_ep=0, Last_gain_idx=0;
    int exp_long = exp_tmp_idx;
    int showDebug = 0;

//tmp    get_current_usec(&curTime);
    if(curTime < lstTime || lstTime == 0) lstTime = curTime;        // rex+ 151229, 防止例外錯誤
    else if((curTime - lstTime) >= 5000000){
        lstTime = curTime;
        showDebug = 1;
    }
    static int showOverQmax = 0;
    if(ISP_All_Command->Fix_AEG_table == 0)
    {
        if(AEG_target_M != ISP_All_Command->AEG_Y_target){
            AEG_target_M = ISP_All_Command->AEG_Y_target;               // 0x50, 0x78, 0xA0...
            AEG_target_H = AEG_target_M + 10;                           // +8 (10%)
            AEG_target_L = AEG_target_M - 10;                           // -8 (10%)
            AEG_target_C_H = AEG_target_M + 5;
            AEG_target_C_L = AEG_target_M - 5;
        }
        if((ISP_All_State->AVG_YRGB.real_Y <= 5) ||                         // 開關燈, 快速調整
           (ISP_All_State->AVG_YRGB.real_Y >= 215)){
            quick = qmax*4;
        }
        else if(ISP_All_State->AVG_YRGB.real_Y > AEG_target_H){             // 快速調整機制，過亮
            //temp1 = (255-ISP_All_State->AVG_YRGB.real_Y);                 // (255-250)^2=25
            //temp2 = (255-AEG_target_M);                                   // (255-110)^2=21025
            //if(temp1 > 0) quick = (temp2 * temp2) / (temp1 * temp1);
            quick = (ISP_All_State->AVG_YRGB.real_Y - AEG_target_H) >> 2;   // 調整加速
            if(quick > qmax) quick = qmax;            //*/
        }
        else if(ISP_All_State->AVG_YRGB.real_Y < AEG_target_L){             // 快速調整機制，過暗
            //temp1 = (ISP_All_State->AVG_YRGB.real_Y);                     // (5)^2=25
            //temp2 = (AEG_target_M);                                       // (110)^2=12100
            //if(temp1 > 0) quick = (temp2 * temp2) / (temp1 * temp1);
            quick = (AEG_target_L - ISP_All_State->AVG_YRGB.real_Y) >> 2;   // 調整加速
            if(quick > qmax) quick = qmax;            //*/
        }
        //if(quick == 0){ if(AEG_adj_count >  0) AEG_adj_count--; }
        //else          { if(AEG_adj_count < 19) AEG_adj_count++; }
        
        if(quick > qmax){
            if(showOverQmax == 0){
                showOverQmax = 1;
                db_debug("real_Y=%d AEG=%d M=%d H=%d L=%d cH=%d cL=%d quick=%d\n", 
                        ISP_All_State->AVG_YRGB.real_Y, AEG_idx>>6, 
                        AEG_target_M, AEG_target_H, AEG_target_L, AEG_target_C_H, AEG_target_C_L, quick);
            }
        }
        else showOverQmax = 0;

        // Digital Gain 上限
        if(AEG_Gain_manual == 1)             gain_max = (120*64);               // 140=12800
        else if(c_mode == 6 || c_mode == 12) gain_max = (120*64);               // 120=6400
        else if(c_mode == 5 || c_mode == 7)  gain_max = (80*64);                // 80=1600, HDR
        else if(c_mode == 2 || c_mode == 11) gain_max = (80*64);                // 80=1600, Timelapse
        else                                 gain_max = (100*64);               // 100=3200, c_mode= 0 3 4 / 1 2
        gain_min = 0;

        // Exposure 上限
        if(c_mode == 1 || c_mode == 10){                                // 1:Rec
            if(AEG_EP_manual == 1) exp_max = AEG_EP_idx;
            else                   exp_max = (60*64);                   // 60=1/15
            if(m_mode == 4 || m_mode == 5)
                if(exp_max > (40*64)) exp_max = (40*64);                // 40=1/30, 4:3K 5:2K
            else 
                if(exp_max > (60*64)) exp_max = (60*64);                // 60=1/15, 3:4K
        }
        else if(c_mode == 3)                 exp_max = (120*64);
        else if(c_mode == 5)                 exp_max = (60*64);         // 60=1/15s, WDR
        else if(c_mode == 6 || c_mode == 12) exp_max = (180*64);        // 180=4s 160=2s 140=1s 120=1/2s 100=1/4s 80=1/8s
        else if(c_mode == 7)                 exp_max = (120*64);        // 120=1/2s, 80=1/8s, Night+WDR
        else if(c_mode == 8 || c_mode == 9)  exp_max = (20*64);	        // 20=1/60s, Sport / Sport_WDR
        else if(AEG_EP_manual == 1)          exp_max = (180*64);        // 180=4s
        else                                 exp_max = (60*64);         // 60=1/15s
        exp_min = EP_Line_2_Gain_Table[freq].min_idx;   // 180:4s ~ -160:1/32000

        // exp_max、exp_min 數值定義
        // 180=4s  160=2s  140=1s  120=(1/2)s  100=(1/4)s  80=(1/8)s  60=(1/15)s  40=(1/30)s  20=(1/60)s
        // 0=(1/120)s
        // -20=1/250  -40=1/500  -60=1/1000  -80=1/2000  -100=1/4000  -120=1/8000  -140=1/16000  -160=1/32000
        aeg_idx_max = exp_max + gain_max;
        aeg_idx_min = exp_min;
        AEG_idx_Lst = AEG_idx;                                  // 避免切換手動自動時會瞬間過亮或過暗

        if(d_cnt == 0 && (AEG_EP_manual == 0 || AEG_Gain_manual == 0)){     // '00' '01' '10'執行, '11'不執行
          if(picture == 1){
            AEG_idx = predict_AEG_calc(AEG_idx, aeg_idx_max, aeg_idx_min, ISP_All_Command->AEG_Y_target);
          }
          else{
again0:
            if(ISP_All_State->AVG_YRGB.real_Y > AEG_target_C_H){            // 過亮
                AEG_idx -= AEG_Step;
            }
            else if(ISP_All_State->AVG_YRGB.real_Y < AEG_target_C_L){       // 過暗
                AEG_idx += AEG_Step;
            }
            if     (AEG_idx > aeg_idx_max){ AEG_idx = aeg_idx_max; quick = 0; }
            else if(AEG_idx < aeg_idx_min){ AEG_idx = aeg_idx_min; quick = 0; }
            else if(quick > 0){
                quick --;
                goto again0;
            }
          }
        }

        get_real_exp_gain_idx(c_mode, AEG_idx, exp_max, &exp_tmp_idx, &gain_tmp_idx, -1);       // 取得條件限制後的曝光值
        //if(showDebug == 1){
        //    db_debug("ISP_AEG_IMX222: c_mode=%d manual=%d:%d/%d:%d aeg=%d max=%d tmp=%d:%d d_cnt=%d\n", 
        //         c_mode, AEG_EP_manual, AEG_Gain_manual, AEG_EP_idx>>6, AEG_Gain_idx>>6, 
        //         AEG_idx>>6, exp_max>>6, exp_tmp_idx>>6, gain_tmp_idx>>6, d_cnt);
        //}
        exp_long = exp_tmp_idx;							// 紀錄曝光時間和ISO

        int cap_gain_max = gain_max;
        if(AEG_Gain_manual == 0){
            if     (exp_tmp_idx >= 180*64) cap_gain_max = 0;        // 4s, iso100
            else if(exp_tmp_idx >= 160*64) cap_gain_max = 20*64;    // 2s, iso200
            else if(exp_tmp_idx >= 140*64) cap_gain_max = 40*64;    // 1s, iso400
            else if(exp_tmp_idx >= 120*64) cap_gain_max = 60*64;    // 1/2, iso800
            else if(exp_tmp_idx >= 100*64) cap_gain_max = 80*64;    // 1/4, iso1600
            else if(exp_tmp_idx >=  80*64) cap_gain_max = 100*64;   // 1/8, iso3200
            if(exp_tmp_idx >=  80*64){
                if(c_mode == 6) cap_gain_max += (20*64);            // 夜間模式iso提高一倍
            }
        }
        Cap_Gain_Idx = gain_tmp_idx;
        if(Cap_Gain_Idx > cap_gain_max) Cap_Gain_Idx = cap_gain_max;
        cal_exp_iso(freq, exp_tmp_idx, Cap_Gain_Idx, c_mode, AEG_B_Exp_1Sec, AEG_B_Exp_Gain, &exp_n, &exp_m, &iso);
        setLiveShowValue(exp_n, exp_m, iso);

        //int live_max = Flash_2_Gain_Table[sflash-1];
        int live_max=0, gain_tmp=0;
        if(d_cnt == 0){                         // live, 沒有按下拍照, 上限1/15 or 1/8, 手動/長時間曝光
            if(c_mode == 6 || c_mode == 7 || c_mode == 12) live_max = 80*64;     // 1/8				//Night / NightHDR / M-Mode
            else if(c_mode == 8 || c_mode == 9)            live_max = 20*64;     // 1/60			//Sport / SportWDR
            else                                           live_max = 60*64;     // 1/15

            if(exp_tmp_idx > live_max){         // 調整live畫面的亮度
                if(AEG_EP_manual == 1 && gain_tmp_idx < 0){ gain_tmp_idx = 0; AEG_idx = AEG_idx_Lst; }
                gain_tmp = (exp_tmp_idx - live_max) + gain_tmp_idx;
                exp_tmp_idx = live_max;
                gain_tmp_idx = gain_tmp;
            }
            if((AEG_EP_manual == 1 && AEG_Gain_manual == 1) || (c_mode == 12)){         // 手動模式
                AEG_idx = AEG_idx_Lst;
            }
            if(showDebug == 1){
                db_debug("ISP_AEG_IMX222: aeg=%d max=%d:%d tmp=%d:%d long=%d c_mode=%d m_mode=%d \n", 
                     AEG_idx>>6, exp_max>>6, live_max>>6, exp_tmp_idx>>6, gain_tmp_idx>>6, exp_long>>6, c_mode, m_mode);
            }
        }
        else{
            if(gain_max > cap_gain_max) gain_max = cap_gain_max;
        }
    }
    else {  // if(ISP_All_Command->Fix_AEG_table == 0)
        gain_tmp_idx = ISP_All_Command->AEG_table_idx;      // GAIN可調
        exp_tmp_idx = (120*64);       // 7680;              // 測試環境定義為 1/60(16.6ms)
    }
    if(gain_tmp_idx > gain_max) gain_tmp_idx = gain_max;
    if(gain_tmp_idx < gain_min) gain_tmp_idx = gain_min;
    if(exp_tmp_idx > exp_max) exp_tmp_idx = exp_max;
    if(exp_tmp_idx < exp_min) exp_tmp_idx = exp_min;        // AEG_EP_idx
    
    if(InitImageState == 1) InitImageState = 2;                             // 可以開始錄影

    static unsigned long long aeg_time=0, aeg_time_lst=0;
//tmp    get_current_usec(&aeg_time);
    static int m_mode_lst=-1, c_mode_lst=-1, m_fps_lst=-1;
    static int lst_aeg_exp=0, lst_exp_tmp=0, lst_gain_tmp=0, lst_freq=0;
    
    chang_en = 0;    // 控制暫時不動作
    if(AEG_EP_idx != lst_aeg_exp) {                     // 手動exp
        lst_aeg_exp = AEG_EP_idx;
        chang_en = 1;
    }
    if(lst_exp_tmp != exp_tmp_idx){                     // 自動exp
        lst_exp_tmp = exp_tmp_idx;
        chang_en = 1;
    }
    if(lst_gain_tmp != gain_tmp_idx){                   // 自動gain
        lst_gain_tmp = gain_tmp_idx;
        chang_en = 1;
    }
    if(lst_freq != freq) {                              // 60Hz <-> 50Hz
        lst_freq = freq;
        chang_en = 1;
    }
    if( AEG_EP_manual == 1 && AEG_Gain_manual == 1 && ep_init_change < 10 &&  (aeg_time - aeg_time_lst) > 1000000) {
        //解ep & gain都固定, 開機過曝問題, 開機每秒下一次AE, 10次
        chang_en = 1;
        ep_init_change++;
        db_debug("03 chang_en == 1\n");
    }
    else if(m_mode_lst != m_mode || c_mode_lst != c_mode || m_fps_lst != fps){
        chang_en = 1;
        m_mode_lst = m_mode;
        c_mode_lst = c_mode;
        m_fps_lst = fps;
        db_debug("04 chang_en == 1\n");
    }
    else if((aeg_time - aeg_time_lst) > 1000000){       // rex+ 190114, 每秒下一次AE
        chang_en == 1;
    }

    if(chang_en == 1) {
        aeg_time_lst = aeg_time;
    }

    int tool_cmd = get_TestToolCmd();
    if(tool_cmd > 0) {
        chang_en = 1;
        if(testtool_set_AEG != -1000 && testtool_set_gain != -1000){
            exp_tmp_idx = testtool_set_AEG;
            gain_tmp_idx = testtool_set_gain;
        }
        else if((tool_cmd&0xff) == 5 || (tool_cmd&0xff) == 6){        // 縫合 and 色縫合
            exp_tmp_idx = (60*64);
            gain_tmp_idx = (80*64);        //S2
        }
        else if( (tool_cmd&0xff) == 2) {		// Focus: main_cmd == 2
            exp_tmp_idx = Focus_EP_Idx;
            gain_tmp_idx = Focus_Gain_Idx;
        }
        else{
            exp_tmp_idx = (40*64);
            gain_tmp_idx = (32*64);
        }
        db_debug("05 chang_en == 1  tool_cmd=%d exp=%d gain=%d\n", tool_cmd, exp_tmp_idx, gain_tmp_idx);
    }

    // rex+ 181212, 計算sensor cmd需要的參數
    calculate_real_sensor_exp(exp_tmp_idx, &AEG_EP_FRM_LENGTH, &AEG_EP_INTEG_TIME, &AEG_EP_FPS);

    AEG_gain_H = gain_tmp_idx;
    int ep_ln_33ms = get_ep_ln_default_33ms(freq);
    int fps_us = 10000000 / fps;
    //int shuter_sp = (AEG_EP_FRM_LENGTH + 1) * fps_us - AEG_EP_INTEG_TIME * fps_us / (ep_ln_33ms * fps_us / 33333);
    int exp_us = cal_sensor_exp_us(freq, fps, AEG_EP_FRM_LENGTH, AEG_EP_INTEG_TIME);
    int send_cmd = 0;
    static int lst_d_cnt = 0;
    if(d_cnt != lst_d_cnt){
        if(d_cnt != 0 && chang_en == 1){                     // 拍照時，要送最後一道cmd
        	db_debug("ISP_AEG_IMX222: en=%d frm=%d int=%d gain=%d sp=%d\n", chang_en, AEG_EP_FRM_LENGTH, AEG_EP_INTEG_TIME, AEG_gain_H>>6, exp_us);
            send_cmd = 1;
        }
        lst_d_cnt = d_cnt;
    }
    //if(showDebug == 1)
    //    db_debug("ISP_AEG_IMX222: en=%d frm=%d int=%d gain=%d sp=%d\n", chang_en, AEG_EP_FRM_LENGTH, AEG_EP_INTEG_TIME, AEG_gain_H>>6, exp_us);
//    int hdr_manual = get_HDRManual();
//    int removal_hdr_mode = get_Removal_HDRMode();
//    int hdr_step = Get_HDR7P_Auto_Step();
//    if(Check_AEG_En(c_mode, hdr_manual, removal_hdr_mode, hdr_step) == 1) {
		if((d_cnt == 0 && chang_en == 1) || send_cmd == 1){     // 解長時間曝光Auto ISO會一直改變，造成header資訊錯誤
			set_A2K_Shuter_Speed(exp_us, exp_long);
			set_PipeCmd_AEG_idx(AEG_idx, aeg_idx_max, aeg_idx_min, gain_max);           // make pipeline command
			set_AEG_EP_Var(chang_en, AEG_EP_FRM_LENGTH, AEG_EP_INTEG_TIME, AEG_gain_H); // make sensor command

			cal_exp_iso(freq, exp_tmp_idx, gain_tmp_idx, c_mode, AEG_B_Exp_1Sec, AEG_B_Exp_Gain, &exp_n, &exp_m, &iso);
			set_A2K_JPEG_EP_v(exp_n, exp_m, iso);
		}
//    }
    return chang_en;
}//*/



//int ep_mode = 0, ep_mode_lst = 0;		// 0: 1/30 以下   1:1/15  2:1/8  3:1/4  4:1/2  5:1/1
int Integ_Lock = 0;
void AEG_IMX222_write(void)
{
	int TimeOut_CLK;
	int TimeOut_Time;
	int ISP1_TIME_OUT;
	int Input_Scale = Get_Input_Scale();

	//if(AEG_EP_FRM_LENGTH != 0) {
		if(Input_Scale == 1)      ISP1_TIME_OUT = FPGA_FS_TIME_OUT;
		else if(Input_Scale == 2) ISP1_TIME_OUT = FPGA_D2_TIME_OUT;
		else if(Input_Scale == 3) ISP1_TIME_OUT = FPGA_D3_TIME_OUT;

		TimeOut_CLK   = 100; // MHz
		TimeOut_Time  = ISP1_TIME_OUT * (AEG_EP_FRM_LENGTH+1); //us

		int t1 = 0x4000000 - TimeOut_CLK * TimeOut_Time;
		int t2 = 0x4000000 - TimeOut_CLK * 5000;
		set_A2K_ISP1_Timeout(t1, t1, t2, t1, t1, t1);
	//}
}

int S2_A_Gain=224, S2_D_Gain=320;
void setS2ISO(int idx, int value)
{
	int again = 0, dgain = 0;

	switch(idx) {
	case 0: S2_A_Gain   = value; 	  break;
	case 1: S2_D_Gain   = value;       break;
	}

	unsigned Data[2];
	CIS_CMD_struct s_cmd_tmp;
	unsigned *MSPI_DATA;
	
	//AGain
	again = S2_A_Gain + 107;
	s_cmd_tmp.Addr_H = 0x00;
	s_cmd_tmp.Addr_L = 0x09;
	s_cmd_tmp.Data = ((again >> 8)&0xFF);			//107 = 363 - 256, a_gain(dB) = 0.09375 * (a_gain - 256), a_gain rang: 0~363
	MSPI_DATA = (unsigned *) &s_cmd_tmp;
	Data[0] = 0xF08;
	Data[1] = *MSPI_DATA;
	SPI_Write_IO_S2(0x9, &Data[0], 8);          // setS2ISO
	
	s_cmd_tmp.Addr_H = 0x00;
	s_cmd_tmp.Addr_L = 0x0A;
	s_cmd_tmp.Data = (again&0xFF);			//107 = 363 - 256, a_gain(dB) = 0.09375 * (a_gain - 256), a_gain rang: 0~363
	MSPI_DATA = (unsigned *) &s_cmd_tmp;
	Data[0] = 0xF08;
	Data[1] = *MSPI_DATA;
	SPI_Write_IO_S2(0x9, &Data[0], 8);          // setS2ISO

	//DGain
	dgain = S2_D_Gain + 320;
	s_cmd_tmp.Addr_H = 0x00;
	s_cmd_tmp.Addr_L = 0x11;
	s_cmd_tmp.Data = (dgain&0xFF);			// d_gain(dB) = 0.09375 * (d_gain - 320), d_gain rang: 0~575
	MSPI_DATA = (unsigned *) &s_cmd_tmp;
	Data[0] = 0xF08;
	Data[1] = *MSPI_DATA;
	SPI_Write_IO_S2(0x9, &Data[0], 8);          // setS2ISO
}

void getS2ISO(int *val)
{
	*val     = S2_A_Gain;
	*(val+1) = S2_D_Gain;
}

int NR3D_strength_debug = -1;
void SetNR3DStrength(int value)
{
	NR3D_strength_debug = value;
}

int GetNR3DStrength(void)
{
	return NR3D_strength_debug;
}

int NR3D_leve_debug = -1;
void SetNR3DLeve(int value)
{
	if(value >= -1 && value <= 7)
		NR3D_leve_debug = value;
}

int GetNR3DLeve(void)
{
	return NR3D_leve_debug;
}

int NR3D_sill_table[8] =
{				// miller+s 140827	miller+ 140825, 測試數值	// 20.03.02	// 20.03.01
   6,			// 0	// { 6, 10}		{ 4, 10} gain=0,   nr3d=4	// {16,  3}	// {16,  3}
  12,			// 1	// { 8,  3}		{ 6,  3} gain=20,               // {24,  3}	// {22,  3}
  16,			// 2	// {15,  3}		{12,  3} gain=40,  nr3d=12      // {32,  3}	// {28,  2}
  22,			// 3	// {22,  2}		{18,  2} gain=60,  nr3d=18      // {40,  3}	// {36,  2}
  32,			// 4	// {32,  2}		{28,  2} gain=80,  nr3d=28      // {56,  3}	// {48,  1}
  48,			// 5	// {44,  2}		{40,  2} gain=100, nr3d=40
  70,			// 6	// {56,  2}		{50,  2} gain=120, nr3d=50
  105			// 7
};
int NR3D_strength_tbl[16][3] =
{
  {0x00, 0x00, 0x00},		// 0	 0, 0, 0		// 3DNR最弱
  {0x08, 0x0C, 0x0E},		// 1
  {0x10, 0x18, 0x1C},		// 2
  {0x18, 0x24, 0x2A},		// 3
  {0x20, 0x30, 0x38},		// 4
  {0x28, 0x3c, 0x46},		// 5
  {0x30, 0x48, 0x54},		// 6
  {0x38, 0x54, 0x62},		// 7
  {0x40, 0x60, 0x70},		// 8	1/2, 1/4, 1/8
  {0x43, 0x62, 0x71},		// 9
  {0x46, 0x64, 0x72},		// 10
  {0x49, 0x66, 0x74},		// 11
  {0x4C, 0x69, 0x76},		// 12
  {0x4F, 0x6C, 0x78},		// 13
  {0x52, 0x6F, 0x7A},		// 14
  {0x56, 0x72, 0x7C}		// 15	1/3, 1/9, 1/27	// 3DNR最強
};
void Set_ISP_Block2_3DNR(int init_flag)
{
	int i;
    unsigned level, rate, sill_idx, sill_rem, cmd[2];
    ISP_Command_Struct *ISP_All_Command = &ISP_All_Command_D;
    ISP_Stati_Struct *ISP_All_State = &ISP_All_State_D;
    static unsigned NR3D_Gain=9999, NR3D_Rate=0, NR3D_Level=0, NR3D_Strength = 9999;

if(NR3D_strength_debug == -1)
	ISP_All_Command->NR3D_Strength = 8;
else
	ISP_All_Command->NR3D_Strength = NR3D_strength_debug;

    if(NR3D_leve_debug != -1) {
    	if(NR3D_leve_debug == 7)
    		AEG_gain_H = NR3D_leve_debug * (20*64) - 1;
    	else
    		AEG_gain_H = NR3D_leve_debug * (20*64);
    }

    if((ISP_All_Command->NR3D_Strength >= 0) && (ISP_All_Command->NR3D_Strength <= 15)){
        ISP_All_Command->Mix_Rate = 0x80;		//0x00 暫時關閉3DNR
        if((NR3D_Gain*20) != AEG_gain_H || init_flag == 1 || NR3D_Strength != ISP_All_Command->NR3D_Strength){
            NR3D_Gain = AEG_gain_H / 20;        // 1280=1倍 轉 64=1倍
            NR3D_Strength = ISP_All_Command->NR3D_Strength;
            //db_debug("Set_ISP_Block2_3DNR() NR3D_Gain=%d\n", NR3D_Gain);
            sill_idx = NR3D_Gain / 64;        // 64=1倍
            sill_rem = NR3D_Gain % 64;
            if(sill_idx > 6) sill_idx = 6;
            ISP_All_Command->Level2 = ((NR3D_sill_table[sill_idx] * (64 - sill_rem)) + (NR3D_sill_table[sill_idx+1] * sill_rem)) / 64;
            ISP_All_Command->Level1 = (ISP_All_Command->Level2 * 3) >> 2;	// n * 3 / 4 = 0.75
            ISP_All_Command->Level0 = (ISP_All_Command->Level2 >> 1);		// n / 2 = 0.5

            ISP_All_Command->Rate3 = 0x00;
            ISP_All_Command->Rate2 = NR3D_strength_tbl[ISP_All_Command->NR3D_Strength][0];		// 0..1/2..1/4
            ISP_All_Command->Rate1 = NR3D_strength_tbl[ISP_All_Command->NR3D_Strength][1];		// 0..1/4..1/16
            ISP_All_Command->Rate0 = NR3D_strength_tbl[ISP_All_Command->NR3D_Strength][2];		// 0..1/8..1/64
        }
    }

    level = (ISP_All_Command->Mix_Rate << 24) | ( (ISP_All_Command->Level2+1) << 16) | ( (ISP_All_Command->Level1+1) << 8) | (ISP_All_Command->Level0+1);
    rate =  (ISP_All_Command->Rate3 << 24) | (ISP_All_Command->Rate2 << 16) | (ISP_All_Command->Rate1 << 8) | (ISP_All_Command->Rate0);
    //db_debug("Set_ISP_Block2_3DNR() level=0x%x rate=0x%x NR3D_Gain=%d\n", level, rate, NR3D_Gain);
    if(NR3D_Level != level || NR3D_Rate != rate || init_flag == 1){
    	NR3D_Level = level;
        NR3D_Rate = rate;
        set_A2K_ISP2_NR3D(level, rate);
    }
}

int Get_3DNR_Rate(int strength)
{
	int Rate;
	int rate3, rate2, rate1, rate0;
    rate3 = 0x00;
    rate2 = NR3D_strength_tbl[strength][0];		// 0..1/2..1/4
    rate1 = NR3D_strength_tbl[strength][1];		// 0..1/4..1/16
    rate0 = NR3D_strength_tbl[strength][2];		// 0..1/8..1/64
    Rate =  (rate3 << 24) | (rate2 << 16) | (rate1 << 8) | (rate0);
    return Rate;
}
int AE_adj_tout=0;
int do_Focus_2DNR = 0;
void ISP_Command_Porcess(int c_mode)
{
    static unsigned long long curTime, lstTime, lstTime2;			// 反應時間為200ms
    static int ISP_contrast=-1, ISP_bright=-1;
    int aeg_change=0, delay=100000;
    int mode, res;
    ISP_Command_Struct *ISP_All_Command = &ISP_All_Command_D;
    int M_Mode, S_Mode;
    int run_big_smooth = get_run_big_smooth();
    int d_cnt = read_F_Com_In_Capture_D_Cnt();
//tmp    int picture = get_C_Mode_Picture(c_mode);
	int picture = 1;

    Get_M_Mode(&M_Mode, &S_Mode);

    if(GetDefectStep() == 0){
        int st_mode = M_Mode;                           // 錄影拍照都用大圖縫合
        if(run_big_smooth == 0){
            if(picture == 1)
                st_mode = S_Mode;
        }
        else{
            if(c_mode == 4){
                st_mode = S_Mode;                       // 4:RAW, 用小圖縫合
            }
            else if(picture == 1){
                if(read_F_Com_In_Capture_D_Cnt() == 0)
                    st_mode = S_Mode;                   // LIVE用小圖縫合
            }
        }
        readAWBbuf(st_mode);                                // read 160kb
        get_avg_rgby();
    }

    int tool_cmd = get_TestToolCmd();
    switch(c_mode){
    case 2:
    case 11: delay = 1000000; break;        // 縮時
    case 6:                                 // Night
    case 7:                                 // Night + WDR
    case 12: delay = 200000; break;         // M-Mode
    default: delay = 100000; break;
    }

    // AE
    int is_hdr_auto = 0;
    int hdr_step = Get_HDR7P_Auto_Step();
    static int aeb_cmd_delay = 0;
//tmp    get_current_usec(&curTime);
    if(curTime < lstTime) lstTime = curTime;
    else if( (curTime - lstTime) >= delay) {
            lstTime = curTime;
            int next = get_Pipe_Next_Cmd_Delay();
    	    if(tool_cmd == 0 && getWhiteBalanceMode() == 0 && DebugJPEGMode == 0 && d_cnt == 0 && next == 0)
    	        ISP_AWB();

            if(c_mode == 3 || c_mode == 5 || c_mode == 7){  // 避免AEB/HDR模式造成LIVE變黑閃一下
                if(d_cnt != 0 || next != 0){
                    aeb_cmd_delay = 10;
                }
            }

    	    if(aeb_cmd_delay > 0)
    	    	aeb_cmd_delay --;
            else {
            	is_hdr_auto = Check_Is_HDR_Auto();
            	if(is_hdr_auto == 0 || (is_hdr_auto == 1 && hdr_step == 0) )
            		aeg_change = ISP_AEG_IMX222(c_mode, M_Mode);
            }

    	    if(aeg_change == 1){
                if(picture == 1){
                    if(c_mode == 6 || c_mode == 7 || c_mode == 12) aeb_cmd_delay = 5;      // 夜間模式
                    else                                           aeb_cmd_delay = 4;
                }
    	        AEG_IMX222_write();
    	        if(Block2_2DNR_ready == -1) Block2_2DNR_ready = 1;		//依據Gain值調整銳利度
    	        run_Gamma_Set_Func = 1;
    	    }
            else{
                if(Gamma_Line_Ready != -1)
                    AS2_Make_Gamma_Line(c_mode);
            }
            //Do_Color_Matrix();                                  //CMOS_ISP_Block1_SET();
            Do_RGB_Gain();

    		if(Block2_2DNR_ready != -1){
    			if(do_Focus_2DNR == 1)
    				Set_ISP_Block2_2DNR(Cap_Gain_Idx, (Block2_2DNR_ready & 0x1), c_mode, M_Mode, 10);
    			else
    				Set_ISP_Block2_2DNR(Cap_Gain_Idx, (Block2_2DNR_ready & 0x1), c_mode, M_Mode, Adj_NR2D_Strength);
                Block2_2DNR_ready = -1;
            }
    }
    Set_ISP_Block2_3DNR(Block2_3DNR_ready);
    Block2_3DNR_ready = 0;
}

/*
 * rex+ 140721, 夜間黃色太濃由0.1->0.05
 */
//#define COLOR_MATRIX_RATE_DEFAULT 250
//int color_matrix_rate = COLOR_MATRIX_RATE_DEFAULT;
//int color_matrix_manual=0, color_matrix_idx=0;
//int color_matrix_tbl[7][9]={
//    {0x2000, 0x8000, 0x0000, 0x8000, 0x2000, 0x0000, 0x0000, 0x8000, 0x2000},		// 0
//    {0x20A7, 0x80F1, 0x0049, 0x8093, 0x2062, 0x0031, 0x0031, 0x81F3, 0x21C2},		// 0.05
//    {0x214F, 0x81E3, 0x0093, 0x8126, 0x20C4, 0x0062, 0x0062, 0x83E7, 0x2385},		// 0.1
//    {0x229F, 0x83C6, 0x0126, 0x824D, 0x2189, 0x00C4, 0x00C4, 0x87CE, 0x270A},		// 0.2
//    {0x253F, 0x878D, 0x024D, 0x849B, 0x2312, 0x0189, 0x0189, 0x8F9D, 0x2E14},		// 0.4
//    {0x268F, 0x8970, 0x02E1, 0x85C2, 0x23D7, 0x01EB, 0x01EB, 0x9385, 0x3199},		// 0.5
//    {0x2d1e, 0x92e0, 0x05c2, 0x8b84, 0x27ae, 0x03d6, 0x03d6, 0xa70a, 0x4332}		// 1.0
//};
//     {0x2000, 0x0000, 0x0000, 0x0000, 0x2000, 0x0000, 0x0000, 0x0000, 0x2000},		// 0

/*
 * |R'|   | M11 M12 M13 |   | R |
 * |G'|	= | M21 M22 M23 | X | G |
 * |B'|	  | M31 M32 M33 |   | B |
 *
 *  ISP_All_Set_Reg.ISP_Block1.M11 	= 0x2000;
 *  ISP_All_Set_Reg.ISP_Block1.M12 	= 0;
 *  ISP_All_Set_Reg.ISP_Block1.M13 	= 0;
 *  ISP_All_Set_Reg.ISP_Block1.M21 	= 0;
 *  ISP_All_Set_Reg.ISP_Block1.M22 	= 0x2000;
 *  ISP_All_Set_Reg.ISP_Block1.M23 	= 0;
 *  ISP_All_Set_Reg.ISP_Block1.M31 	= 0
 *  ISP_All_Set_Reg.ISP_Block1.M32 	= 0;
 *  ISP_All_Set_Reg.ISP_Block1.M33 	= 0x2000;
 *
 * rex+ 150626
 *   rturn: 0 -> 沒有更動
 *          1 -> 有更動
 */
//int Do_Color_Matrix(void)
//{
//    int gain_idx, i, j, lst_idx;
//    ISP_Command_Struct *ISP_All_Command = &ISP_All_Command_D;       // &isp_cmd_tmp[0];
//    ISP_Stati_Struct *ISP_All_State = &ISP_All_State_D;             // &isp_state_tmp[0];
//
//    gain_idx = AEG_gain_H;
//    lst_idx = color_matrix_idx;
//
//    if(color_matrix_manual >= 0 && color_matrix_manual <= 6){			// 手動模式
//        color_matrix_idx = color_matrix_manual;
//    }
//    else{									// 自動模式
//        if(ISP_All_Command->BW_EN == 1){	// 夜間模式
//            color_matrix_idx = 0;
//        }
//        else {					// 日間模式
//            if     (gain_idx <  40*64){ color_matrix_idx = 5; }         // ISO400
//            else if(gain_idx <  60*64){ color_matrix_idx = 4; }         // ISO800
//            else if(gain_idx <  80*64){ color_matrix_idx = 3; }         // ISO1600
//            else if(gain_idx < 100*64){ color_matrix_idx = 2; }         // ISO3200
//            else                      { color_matrix_idx = 1; }         // ISO6400
//        }
//    }
//
//    /*
//     * FPGA CMD, ISP1  M_00, M01, M02, M10, M11, M12, M20, M21, M22
//     */
//    int color_matrix_tmp[9], rate = color_matrix_rate;
//	for(i = 0; i < 9; i++){
//		color_matrix_tmp[i] = ( ( (color_matrix_tbl[6][i] - color_matrix_tbl[0][i]) & 0x7FFF) * rate / 1000) + (color_matrix_tbl[0][i] & 0xF000);
//    }
//    set_A2K_ISP1_Color_Matrix(color_matrix_tmp);
//
//    /*for(i=0;i<9;i++){
//    	color_matrix_idx = color_matrix_idx & 6;
//    	//set_isp1_2m_data(i, color_matrix_tbl[color_matrix_idx][i]);
//    	set_512_2m_data(i, color_matrix_tbl[color_matrix_idx][i]);
//    }*/
//    if(lst_idx == color_matrix_idx) return 0;
//    else                            return 1;
//}

/*
 * 190308 max+,	0x1000 = 1倍, 負數:15bit = 1, 填ISP2 CMD URGB VRGB
 * YUV = [YUV_Matrix] * [RGB_Matrix] * [RGB]
 */
float YUV_Matrix[3][3] = {
		{ 0.299,  0.586,  0.114},
		{-0.169, -0.331,  0.5  },
		{ 0.5,	 -0.419, -0.081}
};
float RGB_Matrix[3][3] = {
		{ 1.2,	-0.1,	-0.1  },
		{-0.15,	 1.3,	-0.15 },
		{-0.15,	-0.15,	 1.3  }
};
int RGB_x_YUV_Matrix[3][3] = {
		{0x1000,	0x1000,	0x1000},
		{0x1000,	0x1000,	0x1000},
		{0x1000,	0x1000,	0x1000}
};
void Color_Matrix_x_YUV_Matrix(void)
{
	int i, j;
	float tmp, tmp1, tmp2, tmp3;

	for(i = 0; i < 3; i++) {
		for(j = 0; j < 3; j++) {
			tmp1 = YUV_Matrix[i][0] * RGB_Matrix[0][j];
			tmp2 = YUV_Matrix[i][1] * RGB_Matrix[1][j];
			tmp3 = YUV_Matrix[i][2] * RGB_Matrix[2][j];
			tmp = tmp1 + tmp2 + tmp3;
			if(tmp >= 0)
				RGB_x_YUV_Matrix[i][j] = tmp * 0x1000;
			else
				RGB_x_YUV_Matrix[i][j] = 0x8000 | abs(tmp * 0x1000);
			set_A2K_ISP2_UVtoRGB( (i * 3 + j), RGB_x_YUV_Matrix[i][j]);

			db_debug("Color_Matrix_x_YUV_Matrix() i=%d j=%d tmp=%f M=0x%x RGB=%f\n", i, j, tmp, RGB_x_YUV_Matrix[i][j], RGB_Matrix[i][j]);
		}
	}
}
void SetRGBMatrix(int idx, int value)
{
	RGB_Matrix[idx/3][idx%3] = (float)value / 1000.0;
	Color_Matrix_x_YUV_Matrix();
}

int GetRGBMatrix(int idx)
{
	int value;
	value = (int)(RGB_Matrix[idx/3][idx%3] * 1000);
	return value;
}

void SetRGB2YUVMatrix(int idx, int value)
{
	YUV_Matrix[idx/3][idx%3] = (float)value / 1000.0;
	Color_Matrix_x_YUV_Matrix();
}

int GetRGB2YUVMatrix(int idx)
{
	int value;
	value = (int)(YUV_Matrix[idx/3][idx%3] * 1000);
	return value;
}

int S2_RGB_Debug_En = 0;
int S2_R_Gain = 0x7000, S2_G_Gain = 0x4000, S2_B_Gain = 0x7000;
void setS2RGB(int idx, int value)
{
	switch(idx) {
	case 0: S2_R_Gain = value; S2_RGB_Debug_En = 1; break;
	case 1: S2_G_Gain = value; S2_RGB_Debug_En = 1; break;
	case 2: S2_B_Gain = value; S2_RGB_Debug_En = 1; break;
	case 3: S2_RGB_Debug_En = value; break;
	}
	if(S2_RGB_Debug_En != 0)
		set_A2K_ISP1_RGB_Gain(0x4000, 0x4000, 0x4000, S2_R_Gain, S2_G_Gain, S2_B_Gain);
}

void getS2RGB(int *val)
{
	*val      	  = S2_R_Gain;
	*(val+1)      = S2_G_Gain;
	*(val+2)      = S2_B_Gain;
	*(val+3)      = S2_RGB_Debug_En;
}

/*
 * setting...
 *   *mRGB = &ISP_All_Set_Reg.ISP_Block1.MR
 *   *gRGB = &ISP_All_Set_Reg.ISP_Block1.Gain_R
 *   *wRGB = &ISP_All_State_D.AWB_Gain.gain_R
 */
void set_ISP_Block1_Limit(int sRGB, unsigned *mRGB, unsigned *gRGB, unsigned *wRGB, int *change)
{
    int i, sub, max=0x8400, min=0x4000;

    max = RGB_Gain_Range[sRGB][0];
    min = RGB_Gain_Range[sRGB][1];
    //*gRGB = *gRGB + (*mRGB - 0x4000);
    //if(*gRGB > max) *gRGB = max;
    //if(*gRGB < min) *gRGB = min;

    for(i = 0; i < 20; i++){
        if(*wRGB >= 0x4100 && *gRGB <= (max-0x100))           //0xF800           // > 1.5625%
        {
            *wRGB = *wRGB - 0x100;
            *gRGB = *gRGB + 0x100;
            *change = 1;
        }
        else if(*wRGB <= 0xF800 && *gRGB >= (min+0x100))      //0x4100
        {
            *wRGB = *wRGB + 0x100;
            *gRGB = *gRGB - 0x100;
            *change = 1;
        }
        else break;
    }//*/
    if(*change == 1){
        *gRGB = (*gRGB) & 0xffffff00;
        *gRGB = (*gRGB) | ((*wRGB)&0xff);
    }



    /*for(i = 0; i < 20; i++){
        if(*mRGB >= 0x4100 && *gRGB <= 0xF800)           // > 1.5625%
        {
            *wRGB = *wRGB - 0x100;
            *mRGB = *mRGB - 0x100;
            *gRGB = *gRGB + 0x100;
            *change = 1;
        }
        else if(*mRGB <= 0xF800 && *gRGB >= 0x4100)
        {
            *wRGB = *wRGB + 0x100;
            *mRGB = *mRGB + 0x100;
            *gRGB = *gRGB - 0x100;
            *change = 1;
        }
        else break;
    }//*/
}
unsigned int GinaI_Debug_En = 0;
void SetRGBGainI(int idx, int gain)
{
	if(gain == -1) {
		GinaI_Debug_En = 0;
	}
	else {
		switch(idx) {
		case 0: ISP_All_Set_Reg.ISP_Block1.MR = gain; break;
		case 1: ISP_All_Set_Reg.ISP_Block1.MG = gain; break;
		case 2: ISP_All_Set_Reg.ISP_Block1.MB = gain; break;
		}
		GinaI_Debug_En = 1;
	}
}

int GetRGBGainI(int idx)
{
	int gain;
	switch(idx) {
	case 0: gain = ISP_All_Set_Reg.ISP_Block1.MR; break;
	case 1: gain = ISP_All_Set_Reg.ISP_Block1.MG; break;
	case 2: gain = ISP_All_Set_Reg.ISP_Block1.MB; break;
	}
	return gain;
}

int Do_RGB_Gain(void)
{
    int i, change=0;
    int MR, MG, MB;
    int GR, GG, GB;
    int d_cnt = read_F_Com_In_Capture_D_Cnt();
    int min;
    int wb_mode;
    float rate;
    if(S2_RGB_Debug_En != 0 || d_cnt != 0) return 0;

    if(GetDefectStep() == 0) {
        wb_mode = getWhiteBalanceMode();
        if(wb_mode == 0) {
            /*//if(ISP_All_Command->AWB_en_do != 0){                            // AWB才進入計算
                set_ISP_Block1_Limit(0, &ISP_All_Set_Reg.ISP_Block1.MR, &ISP_All_Set_Reg.ISP_Block1.Gain_R, 
                                     &ISP_All_State_D.AWB_Gain.gain_R, &change);
                set_ISP_Block1_Limit(1, &ISP_All_Set_Reg.ISP_Block1.MG, &ISP_All_Set_Reg.ISP_Block1.Gain_G, 
                                     &ISP_All_State_D.AWB_Gain.gain_G, &change);
                set_ISP_Block1_Limit(2, &ISP_All_Set_Reg.ISP_Block1.MB, &ISP_All_Set_Reg.ISP_Block1.Gain_B, 
                                     &ISP_All_State_D.AWB_Gain.gain_B, &change);

                if(ISP_All_Set_Reg.ISP_Block1.Gain_G > (RGB_Gain_Range[1][1]+0x100))
                {
                    float tR, tG, tB;
                    int tMin = ISP_All_Set_Reg.ISP_Block1.Gain_G;
                    tR = (float)ISP_All_Set_Reg.ISP_Block1.Gain_R / (float)tMin;
                    tG = 1.0;
                    tB = (float)ISP_All_Set_Reg.ISP_Block1.Gain_B / (float)tMin;
                    if(tMin > 0x4000) tMin = 0x4000;

                    ISP_All_Set_Reg.ISP_Block1.Gain_R = tR * tMin;
                    ISP_All_Set_Reg.ISP_Block1.Gain_G = tG * tMin;
                    ISP_All_Set_Reg.ISP_Block1.Gain_B = tB * tMin;
                }
                if(ISP_All_Set_Reg.ISP_Block1.Gain_R < RGB_Gain_Range[0][1])
                    ISP_All_Set_Reg.ISP_Block1.Gain_R = RGB_Gain_Range[0][1];
                if(ISP_All_Set_Reg.ISP_Block1.Gain_G < RGB_Gain_Range[1][1])
                    ISP_All_Set_Reg.ISP_Block1.Gain_G = RGB_Gain_Range[1][1];
                if(ISP_All_Set_Reg.ISP_Block1.Gain_B < RGB_Gain_Range[2][1])
                    ISP_All_Set_Reg.ISP_Block1.Gain_B = RGB_Gain_Range[2][1];
                if(ISP_All_Set_Reg.ISP_Block1.Gain_R > RGB_Gain_Range[0][0])
                    ISP_All_Set_Reg.ISP_Block1.Gain_R = RGB_Gain_Range[0][0];
                if(ISP_All_Set_Reg.ISP_Block1.Gain_G > RGB_Gain_Range[1][0])
                    ISP_All_Set_Reg.ISP_Block1.Gain_G = RGB_Gain_Range[1][0];
                if(ISP_All_Set_Reg.ISP_Block1.Gain_B > RGB_Gain_Range[2][0])
                    ISP_All_Set_Reg.ISP_Block1.Gain_B = RGB_Gain_Range[2][0];

                S2_R_Gain = ISP_All_Set_Reg.ISP_Block1.Gain_R * 1000 / 0x4000;  // 暫時使用，看即時RGB倍率
                S2_G_Gain = ISP_All_Set_Reg.ISP_Block1.Gain_G * 1000 / 0x4000;
                S2_B_Gain = ISP_All_Set_Reg.ISP_Block1.Gain_B * 1000 / 0x4000;

                if(GinaI_Debug_En == 1) {		//Debug: Adj GainI, Gain0 固定
                    ISP_All_Set_Reg.ISP_Block1.Gain_R = 0x4000;
                    ISP_All_Set_Reg.ISP_Block1.Gain_G = 0x4000;
                    ISP_All_Set_Reg.ISP_Block1.Gain_B = 0x4000;
                }*/

            ISP_All_Set_Reg.ISP_Block1.Gain_R    = 0x4000 * WB_Mode_Table2[wb_mode][0];
            ISP_All_Set_Reg.ISP_Block1.Gain_G    = 0x4000 * WB_Mode_Table2[wb_mode][1];
            ISP_All_Set_Reg.ISP_Block1.Gain_B    = 0x4000 * WB_Mode_Table2[wb_mode][2];
        }
        else {
            ISP_All_Set_Reg.ISP_Block1.Gain_R    = 0x4000 * (WB_Mode_Table2[wb_mode][0] + Adj_WB_R);
            ISP_All_Set_Reg.ISP_Block1.Gain_G    = 0x4000 * (WB_Mode_Table2[wb_mode][1] + Adj_WB_G);
            ISP_All_Set_Reg.ISP_Block1.Gain_B    = 0x4000 * (WB_Mode_Table2[wb_mode][2] + Adj_WB_B);
//            ISP_All_Set_Reg.ISP_Block1.MR        = 0x4000;
//            ISP_All_Set_Reg.ISP_Block1.MG        = 0x4000;
//            ISP_All_Set_Reg.ISP_Block1.MB        = 0x4000;
        }

    	if(ISP_All_Set_Reg.ISP_Block1.Gain_R < 0x4000 ||
    	   ISP_All_Set_Reg.ISP_Block1.Gain_G < 0x4000 ||
    	   ISP_All_Set_Reg.ISP_Block1.Gain_B < 0x4000)
    	{
    		min = ISP_All_Set_Reg.ISP_Block1.Gain_R;
    		rate = (float)0x4000 / (float)ISP_All_Set_Reg.ISP_Block1.Gain_R;
    		if(ISP_All_Set_Reg.ISP_Block1.Gain_G < min) {
    			min = ISP_All_Set_Reg.ISP_Block1.Gain_G;
    			rate = (float)0x4000 / (float)ISP_All_Set_Reg.ISP_Block1.Gain_G;
    		}
    		if(ISP_All_Set_Reg.ISP_Block1.Gain_B < min) {
    			min = ISP_All_Set_Reg.ISP_Block1.Gain_B;
    			rate = (float)0x4000 / (float)ISP_All_Set_Reg.ISP_Block1.Gain_B;
    		}
			ISP_All_Set_Reg.ISP_Block1.Gain_R *= rate;
			ISP_All_Set_Reg.ISP_Block1.Gain_G *= rate;
			ISP_All_Set_Reg.ISP_Block1.Gain_B *= rate;
    	}

        int tool_cmd = get_TestToolCmd();
        /*
         * FPGA CMD, ISP1  R_GainI, G_GainI, B_GainI, R_Gain0, G_Gain0, B_Gain0
         */
        if(tool_cmd > 0) {
//            GR = 0x7000;
//            GG = 0x4800;
//            GB = 0x8000;
//            set_A2K_ISP1_RGB_Gain(0x4000, 0x4000, 0x4000, GR, GG, GB);
            MR = ISP_All_Set_Reg.ISP_Block1.MR;         // gain I
            MG = ISP_All_Set_Reg.ISP_Block1.MG;
            MB = ISP_All_Set_Reg.ISP_Block1.MB;
            GR = 0x4000;
            GG = 0x4000;
            GB = 0x4000;
            set_A2K_ISP1_RGB_Gain(MR, MG, MB, GR, GG, GB);
        }
        else {
            MR = ISP_All_Set_Reg.ISP_Block1.MR;         // gain I
            MG = ISP_All_Set_Reg.ISP_Block1.MG;
            MB = ISP_All_Set_Reg.ISP_Block1.MB;
            GR = ISP_All_Set_Reg.ISP_Block1.Gain_R;     // gain 0
            GG = ISP_All_Set_Reg.ISP_Block1.Gain_G;
            GB = ISP_All_Set_Reg.ISP_Block1.Gain_B;
            set_A2K_ISP1_RGB_Gain(MR, MG, MB, GR, GG, GB);
        }
    }
    else {		//做壞點
//        GR = 0x6000;
//        GG = 0x4000;
//        GB = 0x6000;
//        set_A2K_ISP1_RGB_Gain(0x4000, 0x4000, 0x4000, GR, GG, GB);
        MR = ISP_All_Set_Reg.ISP_Block1.MR;         // gain 1
        MG = ISP_All_Set_Reg.ISP_Block1.MG;
        MB = ISP_All_Set_Reg.ISP_Block1.MB;
        GR = 0x4000;
        GG = 0x4000;
        GB = 0x4000;
        set_A2K_ISP1_RGB_Gain(MR, MG, MB, GR, GG, GB);
    }

    return change;
}

char ISP_2NDR_Value[512];
int ISP_DDR_command_init_finish = 0;
int Adj_NR2D_Strength = ADJ_NR2D_STRENGTH_DEFAULT;

/*
 * gain: ISO
 * flag: 0:強制設定一次2DNR 1:AE有調整, NR2D_Strength有不同才設定2DNR
 * m_mode:  0->12K, 1->8K, 2->6K, 3->4K, 4->3K, 5->2K
 */
void Set_ISP_Block2_2DNR(int gain, int ready, int c_mode, int m_mode, int strength)
{
    int i, j, run=0, temp_Table, PI0, PV0, PI1, PV1, Delta_PV, Delta_PI, Data[2], Addr;
    static int C_PI0[4], C_PV0[4], C_PI1[4], C_PV1[4], Y_PI0[4], Y_PV0[4], Y_PI1[4], Y_PV1[4];
    ISP_Command_Struct *ISP_All_Command = &ISP_All_Command_D;       // &isp_cmd_tmp[0];
    ISP_Stati_Struct *ISP_All_State = &ISP_All_State_D;             // &isp_state_tmp[0];

    unsigned s_addr, t_addr, size, mode, id;
	AS2_SPI_Cmd_struct TP[2];
	AS2_CMD_IO_struct IOP[7];
	unsigned *uTP;
	int Strength_Max = 46;      // 16~46
	static int Strength_lst = -1;
	int NR2D_Strength[2] = {18, 38};	//[0]:H  [1]:V

    if(ISP_DDR_command_init_finish == 0){						    // 開機initial
    	for(i = 0; i < 4; i++){
    	    C_PI0[i] = 0; C_PV0[i] = 0; C_PI1[i] = 0; C_PV1[i] = 0;
    	    Y_PI0[i] = 0; Y_PV0[i] = 0; Y_PI1[i] = 0; Y_PV1[i] = 0;
    	}
    	ISP_DDR_command_init_finish = 1;
    	//return;
    }

    //
    //            _______ PV1 (銳利+)	+127
    //           /
    //      ____/         PV0 (模糊-)	-52
    //          | |
    //         PI0|
    //           PI1 (數值差異)
    //         0 ~ +63
    //

    /*
    gain
      max = 7*20*64 = 8960
      min = 0
      6400 = 5*20*64
    */
    //依ISO調整銳利度上限
    //if(gain > 6400) gain = 6400;                            // 5*20*64=320=ISO3200
    int gain_tmp = (gain > 6400)? 6400: gain;
    Strength_Max = (6400 - gain_tmp) * 15 / 1280 + 31;          // ISO1600 -> 46, ISO3200 -> 31
    
    if(c_mode == 1 || c_mode == 10) {		//Rec / RecWDR
        // ISO6400:max-=10
        if(gain >= 120*64){		// >= ISO6400
            Strength_Max -= ((gain - (20*64)) / 640);
        }
    }
    else if(/*c_mode == 8 ||*/ c_mode == 9){                    // Sport HDR, 解高ISO會容量大掉張
        // ISO400:max-=4, ISO800:max-=8, ISO1600:max-=12, ISO3200:max-=16...
        if(gain >= 40*64){		// >= ISO400
            Strength_Max -= ((gain - (20*64)) / 320);
        }
    }
    if(Strength_Max < 0) Strength_Max = 0;
    if(Strength_Max > 46) Strength_Max = 46;
    
    int s_tmp, s_pow, offset;
    if(c_mode == 1 || c_mode == 10) {		//錄影
    	s_tmp = 0;
    }
    else {
    	if(c_mode == 2 || c_mode == 11) offset = 3;	//縮時
    	else							offset = 1;	//拍照
		s_pow = (gain - 1280 * offset) * 6 / 1280;		// = ( (float)gain / 1280.0 - 1.0 * step) * 6;
		s_tmp = s_pow;
		if(s_tmp < 0) s_tmp = 0;
    }

    int sharpness;  // 銳利度
    sharpness = strength - s_tmp;
    if(m_mode == 0){ sharpness -= 6; }      // rex+ 180807, 降低 12K 銳利度
    else           { sharpness -= 12; }     // 降低 8K、6K、4K、3K、2K 銳利度
    if(sharpness < 0) sharpness = 0;
    if(sharpness > Strength_Max) sharpness = Strength_Max;
    ISP_All_Command->NR2D_Strength = sharpness;

    NR2D_Strength[0] = sharpness - 14;		//18,	18 - ADJ_NR2D_STRENGTH_DEFAULT(32)
    if(NR2D_Strength[0] < 0) NR2D_Strength[0] = 0;
    if(NR2D_Strength[0] > Strength_Max) NR2D_Strength[0] = Strength_Max;

    NR2D_Strength[1] = sharpness + 6;		//38,	38 - ADJ_NR2D_STRENGTH_DEFAULT(32)
    if(NR2D_Strength[1] < 0) NR2D_Strength[1] = 0;
    if(NR2D_Strength[1] > Strength_Max) NR2D_Strength[1] = Strength_Max;

    if(ready == 1){ // aeg_change = 1
        if(Strength_lst == ISP_All_Command->NR2D_Strength) return;
    }
    Strength_lst = ISP_All_Command->NR2D_Strength;

    if((ISP_All_Command->NR2D_Strength >= 0) && (ISP_All_Command->NR2D_Strength <= 46)){
    	for(j = 0; j < 2; j++){		// H / V
    	    PV0 = -20;										// 0~-120

    	    ISP_All_Command->NR2D_Table[j].C_PI0 	= 30;
            ISP_All_Command->NR2D_Table[j].C_PV0	= -52; //(PV0 - ((PV0/3)*(3-j))) - 20;					// +8~-52 -> +8~-112
            ISP_All_Command->NR2D_Table[j].C_PV1 	= ISP_All_Command->NR2D_Table[j].C_PV0 + (NR2D_Strength[j]*2);
            ISP_All_Command->NR2D_Table[j].C_PI1 	= 45;			// 0~60

            ISP_All_Command->NR2D_Table[j].Y_PI0 	= 2;	//15;
            ISP_All_Command->NR2D_Table[j].Y_PV0 	= -4 + (PV0 - ((PV0/3)*(3-j)));
            ISP_All_Command->NR2D_Table[j].Y_PV1 	= ISP_All_Command->NR2D_Table[j].Y_PV0 + (NR2D_Strength[j]*4);	// NR2D_Strength = 0~15
            ISP_All_Command->NR2D_Table[j].Y_PI1 	= 14;	//25;			// DN2D_user_score = 0~15

            if(ISP_All_Command->NR2D_Table[j].C_PI1 > 63) ISP_All_Command->NR2D_Table[j].C_PI1 = 63;
            if(ISP_All_Command->NR2D_Table[j].Y_PI1 > 63) ISP_All_Command->NR2D_Table[j].Y_PI1 = 63;
            if((C_PI0[j] != ISP_All_Command->NR2D_Table[j].C_PI0) || (C_PV0[j] != ISP_All_Command->NR2D_Table[j].C_PV0) ||
               (C_PI1[j] != ISP_All_Command->NR2D_Table[j].C_PI1) || (C_PV1[j] != ISP_All_Command->NR2D_Table[j].C_PV1) ||
               (Y_PI0[j] != ISP_All_Command->NR2D_Table[j].Y_PI0) || (Y_PV0[j] != ISP_All_Command->NR2D_Table[j].Y_PV0) ||
               (Y_PI1[j] != ISP_All_Command->NR2D_Table[j].Y_PI1) || (Y_PV1[j] != ISP_All_Command->NR2D_Table[j].Y_PV1) )
            {
                run = 1;
            }
    	}
        if(run == 0) return;
    }

    ISP_All_Command->NR2D_Table_Start = (ISP_All_Command->NR2D_Table_Start & 0xffff) + 1;
    ISP_All_Command->NR2D_Table_Start |= 0x2D2D0000;

    db_debug("Set_ISP_Block2_2DNR: gain=%d Adj_NR2D_Strength=%d Strength_Max=%d NR2D_Strength=%d strength=%d H=%d V=%d\n",
    	gain>>6, Adj_NR2D_Strength, Strength_Max, ISP_All_Command->NR2D_Strength, strength, NR2D_Strength[0], NR2D_Strength[1]);

    // ========= NR2D C Table ================
    int buf[512];
    memset(&buf[0], 0, sizeof(buf));
    for(j = 0; j < 2; j++){
        PI0 = ISP_All_Command->NR2D_Table[j].C_PI0;
        PV0 = ISP_All_Command->NR2D_Table[j].C_PV0;
        PI1 = ISP_All_Command->NR2D_Table[j].C_PI1;
        PV1 = ISP_All_Command->NR2D_Table[j].C_PV1;
        Delta_PV = PV1 - PV0;
        Delta_PI = PI1 - PI0;

        for (i = 0; i < 64; i++) {
        	if (i < PI0) temp_Table = PV0;
        	else {
        		if (i > PI1) temp_Table = PV1;
        		else {
        			if (Delta_PI > 0) temp_Table = PV0 +  (Delta_PV * (i - PI0) / Delta_PI);
        			else temp_Table = PV0;
        		}
        	}

        	if(temp_Table > 127) temp_Table = 127;
        	if(temp_Table < -52) temp_Table = -52;
        	if(temp_Table < 0) temp_Table = 0x80 + (temp_Table * -1);

        	//*(volatile int *)(FPGA_REG_SET_MOCOM + ((i+0x40 + j*0x80) << 2)) = temp_Table;
    		//Data[0] = (FPGA_REG_SET_MOCOM + ((i+0x40 + j*0x80) << 2));
    		//Data[1] = temp_Table;
    		//SPI_Write_IO(0x9, &Data[0], 8);

    		buf[j*128+i*2] = (FPGA_REG_SET_MOCOM + ((i+0x40 + j*0x80) << 2));
    		buf[j*128+i*2+1] = temp_Table;

        	if(ISP_All_Command->NR2D_Strength == 0xff) ISP_2NDR_Value[i+0x40 + j*0x80] = temp_Table;
        }
    }

    Addr = F2_NR2D_C_ADDR;
	ua360_spi_ddr_write(Addr, &buf[0], sizeof(buf) );

	/*
	 *   Command is Translated to FPGA0 and FPGA1 by SPI
	 */
	for(i = 0; i < 2; i++) {
		s_addr = F2_NR2D_C_ADDR;
		t_addr = FPGA_REG_SET_MOCOM;
		size = sizeof(buf);
		mode = 0;
		if(i == 0) id = AS2_MSPI_F0_ID;
		else       id = AS2_MSPI_F1_ID;
		AS2_Write_F2_to_FX_CMD(s_addr, t_addr, size, mode, id, 1);
	}

    // ========= NR2D Y Table ================

    memset(&buf[0], 0, sizeof(buf));
    for(j = 0; j < 2; j++){
    	PI0 = ISP_All_Command->NR2D_Table[j].Y_PI0;
    	PV0 = ISP_All_Command->NR2D_Table[j].Y_PV0;
      	PI1 = ISP_All_Command->NR2D_Table[j].Y_PI1;
      	PV1 = ISP_All_Command->NR2D_Table[j].Y_PV1;
      	Delta_PV = PV1 - PV0;
      	Delta_PI = PI1 - PI0;

      	for (i = 0; i < 64; i++) {
      		if (i < PI0) temp_Table = PV0;
      		else {
      			if (i > PI1) temp_Table = PV1;
      			else {
      				if (Delta_PI > 0) temp_Table = PV0 +  (Delta_PV * (i - PI0) / Delta_PI);
      				else temp_Table = PV0;
      			}
      		}
      		if(temp_Table > 127) temp_Table = 127;
      		if(temp_Table < -52) temp_Table = -52;
      		if(temp_Table < 0) temp_Table = 0x80 + (temp_Table * -1);
      		//*(volatile int *)(FPGA_REG_SET_MOCOM + ((i + j*0x80) << 2)) = temp_Table;

    		//Data[0] = (FPGA_REG_SET_MOCOM + ((i + j*0x80) << 2));
    		//Data[1] = temp_Table;
    		//SPI_Write_IO(0x9, &Data[0], 8);

    		buf[j*128+i*2] = (FPGA_REG_SET_MOCOM + ((i + j*0x80) << 2));
    		buf[j*128+i*2+1] = temp_Table;

      		if(ISP_All_Command->NR2D_Strength == 0xff) ISP_2NDR_Value[i + j*0x80] = temp_Table;
        }
    }

    Addr = F2_NR2D_Y_ADDR;
	ua360_spi_ddr_write(Addr, &buf[0], sizeof(buf) );

	/*
	 *   Command is Translated to FPGA0 and FPGA1 by SPI
	 */
	for(i = 0; i < 2; i++) {
		s_addr = F2_NR2D_Y_ADDR;
		t_addr = FPGA_REG_SET_MOCOM;
		size = sizeof(buf);
		mode = 0;
		if(i == 0) id = AS2_MSPI_F0_ID;
		else       id = AS2_MSPI_F1_ID;
		AS2_Write_F2_to_FX_CMD(s_addr, t_addr, size, mode, id, 1);
	}

}

void ISP_DDR_command_init(void)
{
    int j;
	ISP_Command_Struct ISP_All_Command;

    db_debug("ISP_DDR_command_init: ...\n");
    //ISP_All_Set_Reg_Struct    ISP_All_Set_Reg;
    //ISP_Command_Struct 	    ISP_All_Command_D;		// 取代 isp_cmd_tmp[0]
    //ISP_Stati_Struct 		    ISP_All_State_D;
    memset(&ISP_All_Set_Reg, 0, sizeof(ISP_All_Set_Reg_Struct));
    memset(&ISP_All_State_D, 0, sizeof(ISP_Stati_Struct));
    memset(&ISP_All_Command, 0, sizeof(ISP_Command_Struct));

    //-- get_avg_rgb_en: 統計畫面的平均亮度、平均RGB
    //-- Default: 1
    ISP_All_Command.get_avg_rgb_en	= 1;

    //-- AWB_en_do: 開啟白平衡，0: 關閉白平衡    1: 開啟白平衡(sensor or FPGA ISP)
    ISP_All_Command.AWB_en_do		= 1;			// FV.CH2.Awb;

    //-- 手動白平衡, 關閉FPGA ISP白平衡時有效
    //-- MWB_Rgain, MWB_Ggain, MWB_Bgain Default: 109, 加入GUI使用者選項, 三條拉條, 輸入皆為0~255
    ISP_All_Command.MWB_Rgain		= 0x4000;		// 3162;			// 取辦公室場景(未校正) R:3162, G:4831, B:2561
    ISP_All_Command.MWB_Ggain		= 0x4000;		// 4831;
    ISP_All_Command.MWB_Bgain		= 0x4000;		// 2561;


    //-- 亮度、對比、飽和 直接由GUI控制
    ISP_All_Command.Brightness		= 0x70;			// FV.CH[0].Bright;
    ISP_All_Command.Contrast		= 0x88;			// FV.CH[0].Cont;	// 0.5~1.5
    ISP_All_Command.Saturation		= 0xE0;			// FV.CH[0].Satn;	// 0.5~1.5

    /*
    //-- AEG_en: 自動曝光, gain調整, Default: 1
    //-- AEG_Y_target: 目標畫面亮度, Default: 0x40
    */
    ISP_All_Command.AEG_en			= 1;			// AEG enable
    ISP_All_Command.AEG_Y_target	= 0x50;			// AEG Y target

    //-- Fix_AEG_table: 測試用, 若Fix_AEG_table = 1, 則會將AEG_table固定在AEG_table_idx
    //-- Default: 0
    ISP_All_Command.Fix_AEG_table	= 0;
    ISP_All_Command.AEG_table_idx	= 0x40;

    //-- DWDR_en:
    ISP_All_Command.DWDR_en			= 1;			// FV.CH2.Dwdr;


    ISP_All_Set_Reg.ISP_Block1.Gain_R	= 0x4000;	//0x6000;	//0x9c00;	// 0x4000;	0x6400;
    ISP_All_Set_Reg.ISP_Block1.Gain_G	= 0x4000;	//0x4000;	//0x5400;	// 0x4000;	0x4000;
    ISP_All_Set_Reg.ISP_Block1.Gain_B	= 0x4000;	//0x6000;	//0xc000;	// 0x4000;	0x7C00;

    ISP_All_Set_Reg.ISP_Block1.MR	    = 0x6F51;	// 0x4000 * 1.73935
    ISP_All_Set_Reg.ISP_Block1.MG	    = 0x4268;	// 0x4000 * 1.0
    ISP_All_Set_Reg.ISP_Block1.MB	    = 0x7530;	// 0x4000 * 1.83105;

    ISP_All_State_D.AWB_Gain.gain_R     = 0x4000;
    ISP_All_State_D.AWB_Gain.gain_G     = 0x4000;
    ISP_All_State_D.AWB_Gain.gain_B     = 0x4000;

    ISP_All_Command.auto_motion_threshold_en 	= 1;
    ISP_All_Command.motion_sill			= 6;			// for RGB motion till

    ISP_All_Command.motion_debug		= 0;
    ISP_All_Command.motion_debug2		= 0;
    ISP_All_Command.Black_Level			= 0;

    ISP_All_Command.DN2D_en				= 0;			// FV.CH2.Dn2d;	// rex- 140718, 功能還沒好先關起來
    ISP_All_Command.DN2D_edge_offset	= 8;
    ISP_All_Command.DN2D_motion_score	= 4;

    ISP_All_Command.DN2D_user_score		= 0xF;
    ISP_All_Command.BW_EN				= 0;
    ISP_All_Command.DWDR_YUV			= 1;

    ISP_All_Command.V_start				= 0x20;
    ISP_All_Command.H_start				= 0x94;			//0x94;      Tom +150205

    ISP_All_Command.FLIP_ts				= 0;
    ISP_All_Command.H_FLIP				= 0;
    ISP_All_Command.V_FLIP				= 0;

    ISP_All_Command.MASK				= 0x0;
    ISP_All_Command.Position			= 0x20;
    ISP_All_Command.MoCom				= 0x10;

    // 3DNR
    ISP_All_Command.Mix_Rate       		= 0x80;
    ISP_All_Command.Level2         		= 0x30;
    ISP_All_Command.Level1         		= 0x20;
    ISP_All_Command.Level0         		= 0x18;
    ISP_All_Command.Rate3          		= 0x0;
    ISP_All_Command.Rate2          		= 0x40;
    ISP_All_Command.Rate1          		= 0x60;
    ISP_All_Command.Rate0          		= 0x70;

    ISP_All_Command.SH_Mode				= 0x10;		    //Tom +140203 (display only 1920 Mode)
    ISP_All_Command.raw_h_s        		= 0x00;			//畫面擷取

    ISP_All_Command.NR3D_Strength  		= 8;			// FV.CH2.Dn3d_Lv;		// 強度 (0~15)
    ISP_All_Command.NR2D_Strength		= 8;			// FV.CH2.Dn2d_Lv;		// 強度 (0~15)

    // 2DNR
    for(j = 0; j < 4; j++){
        ISP_All_Command.NR2D_Table[j].C_PI0 	= 0;			// rex+ 140821
        ISP_All_Command.NR2D_Table[j].C_PV0 	= 0;
        ISP_All_Command.NR2D_Table[j].C_PI1 	= 0;
        ISP_All_Command.NR2D_Table[j].C_PV1 	= 0;
        ISP_All_Command.NR2D_Table[j].Y_PI0 	= 0;
        ISP_All_Command.NR2D_Table[j].Y_PV0 	= 0;
        ISP_All_Command.NR2D_Table[j].Y_PI1 	= 0;
        ISP_All_Command.NR2D_Table[j].Y_PV1 	= 0;
    }

	//memcpy(isp_tmp, &ISP_All_Command, sizeof(ISP_Command_Struct));
	//ua360_spi_ddr_write(DDR_ISP_ALL_COMMAND_ADDR, &isp_tmp[0], sizeof(ISP_Command_Struct));
    memcpy(&ISP_All_Command_D, &ISP_All_Command, sizeof(ISP_Command_Struct));       				// rex+ 150625
    //ua360_spi_ddr_write(DDR_ISP_ALL_COMMAND_ADDR, &ISP_All_Command_D, sizeof(ISP_Command_Struct));// rex- 150703
}

int ADT_ISO = 270;
//void SetAdtIso(int iso)
//{
//	ADT_ISO = iso;
//	Senser_AD_Trans(ADT_ISO);
//}
//int GetAdtIso(void)
//{
//	return ADT_ISO;
//}

/*
 * 做AWB AE調整
 */
void VinInterrupt(int c_mode)
{
	if(ISP_command_ready == 0){
		ISP_command_ready = 1;
	    ISP_DDR_command_init();
        Block2_2DNR_ready = 0;
        Block2_3DNR_ready = 1;
        ep_init_change = 0;
        ISP_DDR_command_init_finish = 0;
        Gamma_Line_Ready = 0;
        Color_Matrix_x_YUV_Matrix();
        Init_T_Gain();
        Senser_AD_Trans(ADT_ISO);
	}
	ISP_Command_Porcess(c_mode);
}

/*
 * 	取得當下 RGB 的值, array[6]
 */
/*void getAWBRGB2(int *array)
{
	*array     = ISP_All_State_D.AVG_YRGB.AVG_R;
	*(array+1) = ISP_All_State_D.AVG_YRGB.AVG_G;
	*(array+2) = ISP_All_State_D.AVG_YRGB.AVG_B;
	*(array+3) = ISP_All_State_D.AWB_Gain.gain_R;    //Temp_gain_R;
	*(array+4) = ISP_All_State_D.AWB_Gain.gain_G;    //Temp_gain_G;
	*(array+5) = ISP_All_State_D.AWB_Gain.gain_B;    //Temp_gain_B;
}*/

Color_Temperature_Struct  T_Data[16] =		// < 3200K: G+5.75%, B+5.75%
{
   {2200,100.00,191.63,399.58},  //2200, GB+23%
   {2500,100.00,174.17,356.94},  //2500, GB+21.5%
   {2700,100.00,164.11,282.65},  //2700, GB+20%
   {2900,100.00,152.49,232.79},  //2900, GB+16.25%
   {3200,100.00,138.77,186.89},  //3200, GB+11.75%

   {3500,100.00,124.35,153.53},  //3500, GB+5%
   {3800,100.00,115.87,134.19},  //3800, GB+2%

   {4200,100.00,108.23,117.66},  //4200
   {4600,100.00,103.77,107.63},  //4600
   {5000,100.00,100.00,100.00},  //5000
   {5500,107.92,103.61,100.00},  //5500
   {6000,115.01,106.53,100.00},  //6000
   {6600,126.31,112.36,100.00},  //6600
   {7200,142.71,120.42,100.00},  //7200
   {7900,160.39,130.82,100.00},  //7900
   {8600,187.23,148.27,100.00}   //8600
};
/*Color_Temperature_Struct  T_Data[16] =		// < 3200K: G+5.75%, B+5.75%
{
   {2200,100.00,182.29,399.58},  //2200
   {2500,100.00,165.57,339.32},  //2500
   {2700,100.00,155.91,268.52},  //2700
   {2900,100.00,144.61,220.78},  //2900
   {3200,100.00,131.32,176.86},  //3200

   {3500,100.00,118.43,146.22},  //3500
   {3800,100.00,113.60,131.56},  //3800

   {4200,100.00,108.23,117.66},  //4200
   {4600,100.00,103.77,107.63},  //4600
   {5000,100.00,100.00,100.00},  //5000
   {5500,107.92,103.61,100.00},  //5500
   {6000,115.01,106.53,100.00},  //6000
   {6600,126.31,112.36,100.00},  //6600
   {7200,142.71,120.42,100.00},  //7200
   {7900,160.39,130.82,100.00},  //7900
   {8600,187.23,148.27,100.00}   //8600
};*/
/*Color_Temperature_Struct  T_Data[16] =
{
   {2200,100.00,155.80,399.58},  //2200
   {2500,100.00,143.35,293.78},  //2500
   {2700,100.00,136.76,235.54},  //2700
   {2900,100.00,131.17,200.25},  //2900
   {3200,100.00,124.18,167.24},  //3200

   {3500,100.00,118.43,146.22},  //3500
   {3800,100.00,113.60,131.56},  //3800

   {4200,100.00,108.23,117.66},  //4200
   {4600,100.00,103.77,107.63},  //4600
   {5000,100.00,100.00,100.00},  //5000
   {5500,107.92,103.61,100.00},  //5500
   {6000,115.01,106.53,100.00},  //6000
   {6600,126.31,112.36,100.00},  //6600
   {7200,142.71,120.42,100.00},  //7200
   {7900,160.39,130.82,100.00},  //7900
   {8600,187.23,148.27,100.00}   //8600
};*/

#define     Color_5000K   (50 - 22)
typedef struct Color_Temperature_Parameter_Struct_h {
    Color_Temperature_Struct    T_Gain[65];			    // {int T, float R, float G, float B}
    RGB_Gain_struct             T_Gain_Table[64];		    // 4 bytes
    int                         P0;
    int                         P1;
    int                         V0;
    int                         V1;
    int                         Rate;
}Color_Temperature_Parameter_Struct;				

typedef struct Color_Temperature_Control_Struct_h {
    int                                 Step;						//0:標準table	1:依據Step0結果, 固定RB, 變動G +-7%
    int                                 Step1_Change_Flag;			//如果Step0改變, 則Step1強制改變一次, 不需判斷超過1%
    int                                 T_Count[65];			
    unsigned long long                  T1;
    unsigned long long                  T2;
    Color_Temperature_Parameter_Struct	Parameter[2];				
}Color_Temperature_Control_Struct;
Color_Temperature_Control_Struct color_temp_ctrl;

void Temperature2RGB_Gain(int Temperature, Color_Temperature_Struct *color)
{
  int i;
  int T_L, T_H;
  Color_Temperature_Struct *P0, *P1;
  //Color_Temperature_Struct Temperature_Calc;
  int L_Rate, H_Rate;

  color->T = Temperature;
  for (i = 0; i < 16; i++)
	  if (T_Data[i].T > Temperature) break;

  if ((i > 0) && (i < 16)) {
	  P0 = &T_Data[i-1];
	  P1 = &T_Data[i];
	  T_L = Temperature - P0->T;
	  T_H = P1->T - Temperature;
	  L_Rate = T_H * 1000 / (T_H + T_L);
	  H_Rate = T_L * 1000 / (T_H + T_L);

	  color->R = (P0->R * L_Rate + P1->R * H_Rate) / 1000;
	  color->G = (P0->G * L_Rate + P1->G * H_Rate) / 1000;
	  color->B = (P0->B * L_Rate + P1->B * H_Rate) / 1000;
  }
  else {
	  color->R = 100;
	  color->G = 100;
	  color->B = 100;
  }

}
void Init_T_Gain(void)
{
  int i, f_id;
  Color_Temperature_Parameter_Struct	*Para;

  memset(&color_temp_ctrl, 0, sizeof(color_temp_ctrl) );
  Para = &color_temp_ctrl.Parameter[0];
  for (i = 0; i <= 63; i++)
	  Temperature2RGB_Gain(2200 + i * 100, &Para->T_Gain[i]);
  Para->T_Gain[64] = Para->T_Gain[Color_5000K];
  WB_Mode_Table2[0][0] = Para->T_Gain[64].R / 100.0;
  WB_Mode_Table2[0][1] = Para->T_Gain[64].G / 100.0;
  WB_Mode_Table2[0][2] = Para->T_Gain[64].B / 100.0;
  Para->Rate = 101;


  Para = &color_temp_ctrl.Parameter[1];
  for (i = 0; i <= 63; i++)
	  Temperature2RGB_Gain(5000, &Para->T_Gain[i]);
  Para->Rate = 101;


  //Write Table
  Write_Color_Temperature_Table(0);
}

int Color_Temp_Th = 11;		//8;
void SetColorTempTh(int value) {
	Color_Temp_Th = value;
}

int GetColorTempTh(void) {
	return Color_Temp_Th;
}

void Write_Color_Temperature_Table(int step)
{
	int i, f_id;
	int Addr, s_addr, t_addr, size, id, mode;
	Color_Temperature_Parameter_Struct	*Para;
	unsigned Data[128];
	unsigned *ptr;

	//Write Gain Table
	Para = &color_temp_ctrl.Parameter[step];
	memset(&Data[0], 0, sizeof(Data) );
	for(i = 0; i <= 63; i++) {
		Para->T_Gain_Table[i].Gain_R = Para->T_Gain[i].R * 0x100 / 100;					//0x100 = 1倍
		Para->T_Gain_Table[i].Gain_G = Para->T_Gain[i].G * 0x100 / 100;
		Para->T_Gain_Table[i].Gain_B = Para->T_Gain[i].B * 0x100 / 100;

		ptr = &Para->T_Gain_Table[i];
		Data[i*2] = 0xCCAEE000 + i + step * 0x40;
		Data[i*2+1] = *ptr;
	}

	Addr = COLOR_TEMP_TABLE_ADDR + step * 0x200;
	ua360_spi_ddr_write(Addr, &Data[0], sizeof(Data) );
	for(f_id = 0; f_id < 2; f_id++) {
		s_addr = Addr;
		t_addr = 0;
		size = sizeof(Data);
		mode = 0;
		if(f_id == 0) id = AS2_MSPI_F0_ID;
		else		  id = AS2_MSPI_F1_ID;
		AS2_Write_F2_to_FX_CMD(s_addr, t_addr, size, mode, id, 1);
	}

	//Write Th / Choose Gain Table
	memset(&Data[0], 0, sizeof(Data) );
	Data[0] = 0xCCA002C0;		//Th
	Data[1] = Color_Temp_Th;

	Data[2] = 0xCCA002C1;		//選用第幾組 Gain, 3bit
	Data[3] = step & 0x7;

	Data[4] = 0xCCA002C2;		//輸出第幾組 Gain 之統計值, 3bit
	Data[5] = step & 0x7;

	Addr = COLOR_TEMP_TH_ADDR;
	ua360_spi_ddr_write(Addr, &Data[0], sizeof(Data) );
	for(f_id = 0; f_id < 2; f_id++) {
		s_addr = Addr;
		t_addr = 0;
		size = sizeof(Data);
		mode = 0;
		if(f_id == 0) id = AS2_MSPI_F0_ID;
		else		  id = AS2_MSPI_F1_ID;
		AS2_Write_F2_to_FX_CMD(s_addr, t_addr, size, mode, id, 1);
	}
}

void Send_Color_Temperature_Table(void)
{
	int *step    = &color_temp_ctrl.Step;

//tmp	get_current_usec(&color_temp_ctrl.T2);
	if(color_temp_ctrl.T1 > color_temp_ctrl.T2) color_temp_ctrl.T1 = color_temp_ctrl.T2;
	if( (color_temp_ctrl.T2 - color_temp_ctrl.T1) > 500000) {		// > 0.5s
		if(*step == 0) {
			Cal_Delta_G_Table();
			Write_Color_Temperature_Table(1);
		}
		else if(*step == 1) {
			Write_Color_Temperature_Table(0);
		}
		*step = (*step + 1) & 0x1;
//tmp		get_current_usec(&color_temp_ctrl.T1);
	}
}

float Color_Temp_G_Rate = 0.07;		//0.04;
void SetColorTempGRate(int value) {
	Color_Temp_G_Rate = (float)value / 1000.0;
}

int GetColorTempGRate(void) {
	return (Color_Temp_G_Rate * 1000);
}

int Read_Color_Temperature(Color_Temperature_Control_Struct *Ctrl)
{
	int i, addr, ret=0, step=0;
	unsigned count[2][128];		//[2]: F0 / F1

	for(i = 0; i < 2; i++) {
		if(i == 0) addr = 0x80600;		//F0	0~63:S2		64~127:S4
		else       addr = 0xA0600;		//F1	0~63:S3		64~127:S1
		ret = spi_read_io_porcess_S2(addr, (int *)&count[i][0], 512);
		if(ret != 0) db_error("Read_Color_Temperature() Err! i=%d\n", i);
	}

	for(i = 0; i < 64; i++) {
		Ctrl->T_Count[i] = count[0][i] + count[0][i+64] + count[1][i] + count[1][i+64];
		//db_debug("Read_Color_Temperature() i=%d T_Count=%d count[0][i]=%d count[0][i+64]=%d count[1][i]=%d count[1][i+64]=%d\n\0",
		//		i, Para->T_Count[i], count[0][i], count[0][i+64], count[1][i], count[1][i+64]);
	}

	//檢查資料為哪個Step, 後面32筆相同為Step1(找G值), 不同則為Step0(找RB)
	step = 1;
	for(i = 32; i < 64; i++) {
		if(Ctrl->T_Count[32] != Ctrl->T_Count[i]) {
			step = 0;
			break;
		}
		else
			step = 1;
	}

	return step;
}

void SetColorRate(int idx, int value) {
	color_temp_ctrl.Parameter[idx].Rate = value;
}

int GetColorRate(int idx) {
	return color_temp_ctrl.Parameter[idx].Rate;
}

/*
 *	取得第一階段最大值後, 固定RB, G則+-7%, 32個
 */
void Cal_Delta_G_Table(void)
{
	int i, sub;
	float rate;
	Color_Temperature_Struct gain;
	Color_Temperature_Parameter_Struct	*Para0, *Para1;

	Para0 = &color_temp_ctrl.Parameter[0];
	Para1 = &color_temp_ctrl.Parameter[1];

	sub = 4000 - Para0->T_Gain[Para0->P0].T;
	if(sub > 0) rate = (float)sub / 20000.0 + Color_Temp_G_Rate;		// sub / 100 / 2 / 100
	else		rate = Color_Temp_G_Rate;

	memset(&Para1->T_Gain[0], 0, sizeof(Para1->T_Gain) );
	gain = Para0->T_Gain[Para0->P0];
	for(i = -16; i < 16; i++) {
		Para1->T_Gain[i+16].R = gain.R;
		Para1->T_Gain[i+16].B = gain.B;

		//+-7%: gain.G * 0.93 ~ gain.G * 1.07;
		Para1->T_Gain[i+16].G = gain.G + gain.G * (float)i * rate / 16.0;			//gain.G + gain.G * i / 16 * 7 / 100;

//db_debug("Cal_Delta_G_Table() Color: i=%d R=%f G=%f B=%f\n", i, Para1->T_Gain[i+16].R, Para1->T_Gain[i+16].G, Para1->T_Gain[i+16].B);
	}
}
int Tint_Idx = 0;
float Tint_Rate=0.0;
float Get_Delta_G_Rate(int idx)
{
	int sub;
	float rate;
	Color_Temperature_Parameter_Struct	*Para0;

	Para0 = &color_temp_ctrl.Parameter[0];
	sub = 4000 - Para0->T_Gain[Para0->P0].T;							//4000K以下擴大G的搜尋範圍
	if(sub > 0) rate = (float)sub / 20000.0 + Color_Temp_G_Rate;		// sub / 100 / 2 / 100
	else		rate = Color_Temp_G_Rate;

	return ( ( (float)idx - 16.0) * rate * 100.0 / 16.0);		// -4.0% ~ +4.0%
}

/*
 * 讀取資料, 找最大值, 決定色溫
 */
void Do_Next_Color_T(void)
{
  int i;   int Max_P, Max_V, Next_P0;
  int flag, ret;
  int *ch_flag = &color_temp_ctrl.Step1_Change_Flag;			//如果Step0改變, 則Step1強制改變一次, 不需判斷超過1%
  int r_step = 0;												//依據讀回的資料判斷為哪個Step的資料
  Color_Temperature_Control_Struct		*Ctrl;
  Color_Temperature_Parameter_Struct	*Para;
  static int P0_lst=-1, Next_P0_lst=-1, Next_P0_lst_cnt=0, P0_G_lst=-1;
  static int wb_m_lst=-1;
  int count_tmp, temp;
  int start, end;
  int wb_mode = getWhiteBalanceMode();

  Ctrl = &color_temp_ctrl;
  r_step = Read_Color_Temperature(Ctrl);

  //找最大值
  Para = &color_temp_ctrl.Parameter[r_step];
  Ctrl->T_Count[64] = 100;
  Max_V = -1;	//0;
  if(r_step == 0) { start = 0; end = 63; }
  else			  { start = 1; end = 31; }
  for (i = start; i <= end; i++) {
	  temp = Para->T_Gain[i].T; 	//Para->T_Gain[i].T;
	  if(r_step == 0 && temp > 6000)
		  count_tmp = Ctrl->T_Count[i] - Ctrl->T_Count[i] * (temp - 6000) * 2 / 100000;		// -0.2%
	  else if(r_step == 0 && temp < 5000)
		  count_tmp = Ctrl->T_Count[i] + Ctrl->T_Count[i] * (5000 - temp) * 2 / 100000;		// +0.2%
	  else
		  count_tmp = Ctrl->T_Count[i];
	  if (count_tmp > Max_V) {
		  Max_V = Ctrl->T_Count[i];
		  Max_P = i;
	  }
  }
  if(Para->P0 < 0 || Para->P0 >= 65){
      db_error("Do_Next_Color_T: e1! start=%d end=%d r_step=%d temp=%d V=%d P=%d P0=%d count_tmp=%d\n",
          start, end, r_step, temp, Max_V, Max_P, Para->P0, count_tmp);
      return;
  }
  if(Max_P > end) {
	  Max_V = Ctrl->T_Count[end];
	  Max_P = end;
  }
  else if(Max_P < start) {
	  Max_V = Ctrl->T_Count[start];
	  Max_P = start;
  }
  Para->P1 = Max_P;
  Para->V1 = Max_V;
  Para->V0 = Ctrl->T_Count[Para->P0];


  Next_P0 = -1;
  if (Para->V1 > 100) {
	  if(r_step == 1 && *ch_flag == 1) {
		  *ch_flag = 0;
		  Next_P0 = Para->P1;
	  }
	  else if (Para->V1 > (Para->V0 * Para->Rate / 100)) {
		 Next_P0 = Para->P1;
	  }
  }
  else {
	  if (Para->V1 < 50) {
		  if (r_step == 0) Next_P0 = 64;
		  else			   Next_P0 = 16;
	  }
  }


  flag = 0;
  if (Next_P0 != -1) {
	  if(r_step == 0) {
		  if(Next_P0 != Next_P0_lst) {				//連續兩次相同才設定色溫
			  Next_P0_lst = Next_P0;
		  }
		  else if(Next_P0 == Next_P0_lst) {
			  Para->P0 = Next_P0;
			  flag = 1;
		  }
	  }
	  else {
		  Para->P0 = Next_P0;
		  flag = 1;
	  }
  }
  if (Para->P0 == 64) flag = 1;

  if(Para->P0 < 0 || Para->P0 >= 65){
      //Do_Next_Color_T: e2! r_step=1 Para={-813953628,-813953628,-4,-1} Next={-813953628,26} Max={-813953628,-1} Rate=102
      db_debug("Do_Next_Color_T: e2! r_step=%d Para={%d,%d,%d,%d} Next={%d,%d} Max={%d,%d} Rate=%d\n", 
          r_step, Para->P0, Para->V0, Para->P1, Para->V1, Next_P0, Next_P0_lst, Max_P, Max_V, Para->Rate);
      return;
  }
  if(r_step == 0) {
	  if(Para->P0 != P0_lst || wb_mode != wb_m_lst) {
		  db_debug("Do_Next_Color_T: Now=%dK Lst=%dK\n", Para->T_Gain[Para->P0].T, Para->T_Gain[P0_lst].T);
		  P0_lst = Para->P0;
		  wb_m_lst = wb_mode;
		  *ch_flag = 1;
	  }
	  if(wb_mode == 0) set_A2K_JPEG_Color(Para->T_Gain[Para->P0].T);
  }
  else {
	  WB_Mode_Table2[0][0] = Para->T_Gain[Para->P0].R / 100.0;
	  WB_Mode_Table2[0][1] = Para->T_Gain[Para->P0].G / 100.0;
	  WB_Mode_Table2[0][2] = Para->T_Gain[Para->P0].B / 100.0;
	  Tint_Idx = Para->P0 - 16;
	  Tint_Rate = Get_Delta_G_Rate(Para->P0);
	  if(wb_mode == 0) set_A2K_JPEG_Tint( (int)(Tint_Rate * 1000) );

	  if(Para->P0 != P0_G_lst) {
		  db_debug("Do_Next_Color_T() G: Now=%d Tint=%.2f Lst=%d Tint_Lst=%.2f Tint_Idx=%d\n", Para->P0, Tint_Rate, P0_G_lst, Get_Delta_G_Rate(P0_G_lst), Tint_Idx);
		  P0_G_lst = Para->P0;
	  }
  }
  //db_debug("Do_Next_Color_T() r_step=%d P0=%d ch_flag=%d", r_step, Para->P0, *ch_flag);

  // Send Table
  Send_Color_Temperature_Table();
}

int GetColorTemperature() {
	int step = color_temp_ctrl.Step;
	Color_Temperature_Parameter_Struct	*Para;
	Para = &color_temp_ctrl.Parameter[0];
	return (Para->T_Gain[Para->P0].T / 100);
}
int GetTint() {
	return Tint_Idx;	//Tint_Rate;
}

/*
 * 	設定 WB 模式
 * 	mode: 0:Auto 1:Filament Lamp 2:Daylight Lamp 3:Sun 4:Cloudy
 */
void setWBMode(int wb_mode)
{
	Color_Temperature_Struct colStruct;

	if(wb_mode > 1000){
		setWhiteBalanceMode(WHITE_BALANCE_MODE_RGB);
	}else{
		int trgTemp = 2700;
		switch(wb_mode){
		case 1: trgTemp = 2700; break;
		case 2: trgTemp = 4000; break;
		case 3: trgTemp = 5600; break;
		case 4: trgTemp = 8000; break;
		}
		if(wb_mode > 0 && wb_mode < 5){
			Temperature2RGB_Gain(trgTemp, &colStruct);
			WB_Mode_Table2[wb_mode][0] = colStruct.R / 100;
			WB_Mode_Table2[wb_mode][1] = colStruct.G / 100;
			WB_Mode_Table2[wb_mode][2] = colStruct.B / 100;
			set_A2K_JPEG_Color(trgTemp);
			set_A2K_JPEG_Tint(0);
			Tint_Idx = 0;
			db_debug("setWB_Mode %d: R=%f G=%f B=%f\n", wb_mode, WB_Mode_Table2[wb_mode][0],
					WB_Mode_Table2[wb_mode][1], WB_Mode_Table2[wb_mode][2]);
		}
	}
	set_A2K_JPEG_WB_Mode(wb_mode);
}

/*
 * 	設定 WB RGB 資料
 * 	R/G/B: wRed,wGreen,wBule
 */
void setWBTemperature(int temp, int tint)
{
	int sub;
	float rate;
	Color_Temperature_Struct colStruct;

	if(temp < ColorTemperature_Min){
		temp = ColorTemperature_Min;
	}
	if(temp > ColorTemperature_Max){
		temp = ColorTemperature_Max;
	}
	Temperature2RGB_Gain(temp, &colStruct);
	set_A2K_JPEG_Color(temp);
	set_A2K_JPEG_Tint(tint);
	Tint_Idx = tint;

	WB_Mode_Table2[5][0] = colStruct.R / 100;
	//WB_Mode_Table2[5][1] = colStruct.G / 100;
	WB_Mode_Table2[5][2] = colStruct.B / 100;

	//cal tint rate
	sub = 4000 - temp;
	if(sub > 0) rate = (float)sub / 20000.0 + Color_Temp_G_Rate;		// sub / 100 / 2 / 100
	else		rate = Color_Temp_G_Rate;
	WB_Mode_Table2[5][1] = (colStruct.G + colStruct.G * (float)tint * rate / 16.0) / 100.0;

	db_debug("setWBTemperature: R=%f G=%f B=%f Tint_Idx=%d\n", WB_Mode_Table2[5][0], WB_Mode_Table2[5][1], WB_Mode_Table2[5][2], Tint_Idx);
	Awb_Quick_Times = 3;
}

void getWBRgbData(int *value) {
	*(value + 0) = Adjust_R_Gain;
	*(value + 1) = Adjust_G_Gain;
	*(value + 2) = Adjust_B_Gain;
}

int EVValue=0;
/*
 * 	調整AE目標亮度(EV), (-6~6)
 * 	y_idx: -6~6
 */
void setAETergetRateWifiCmd(int y_idx)
{
	if(y_idx > 6)  y_idx = 6;
	if(y_idx < -6) y_idx = -6;

	switch(y_idx) {
	case  6: AEG_Y_ev = 1.677; break;	//+2
	case  5: AEG_Y_ev = 1.563; break;
	case  4: AEG_Y_ev = 1.459; break;
	case  3: AEG_Y_ev = 1.355; break;	//+1
	case  2: AEG_Y_ev = 1.236; break;
	case  1: AEG_Y_ev = 1.118; break;
	case  0: AEG_Y_ev = 1.0;   break;	//0
	case -1: AEG_Y_ev = 0.903; break;
	case -2: AEG_Y_ev = 0.807; break;
	case -3: AEG_Y_ev = 0.711; break;	//-1
	case -4: AEG_Y_ev = 0.64;  break;
	case -5: AEG_Y_ev = 0.57;  break;
	case -6: AEG_Y_ev = 0.5;   break;	//-2
	}
}
void doSetAETergetRateWifiCmd(int y_idx)
{
    EVValue = y_idx;
    setAETergetRateWifiCmd(y_idx);
}
// call: jpeg_header.c
int getEVValue(void)
{
    return EVValue;
}
/*
 *	調整曝光時間
 *	manual_mode: 0:自動調整 1:手動調整
 *	ep_idx: 300(4s) ~ -40(1/32000)
 */
void setExposureTimeWifiCmd(int manual_mode, int ep_idx)
{
    int playmode, res;
    db_debug("setExposureTimeWifiCmd: mode=%d idx=%d\n", manual_mode, ep_idx);
    AEG_EP_manual = manual_mode;
    if(AEG_EP_manual == 1) {
        //AEG_Long_EP_idx = (ep_idx - 120) * 64;
        //if(AEG_Long_EP_idx < (80*64))         // 小於80=(1/8)s, AEG_EP_idx直接設定
        //    AEG_EP_idx = AEG_Long_EP_idx;	    // -120, wifi數值換算, 定義不同
        //else
        //    AEG_EP_idx = (80*64);
        AEG_EP_idx = (ep_idx - 120) * 64;       // -120, wifi數值換算, 定義不同
    }
    //else AEG_Long_EP_idx = 0;
}

/*
 * 	調整AE的Gani值(ISO), (0~140)
 * 	manual_mode: 0:自動調整 1:手動調整
 * 	gain_idx: 0~140
 */
void setAEGGainWifiCmd(int manual_mode, int gain_idx)
{
	AEG_Gain_manual = manual_mode;
	db_debug("setAEGGainWifiCmd() mode=%d idx=%d\n", manual_mode, gain_idx);
	if(AEG_Gain_manual == 1) {
		AEG_Gain_idx = gain_idx * 64;
		if(AEG_Gain_idx > 140*64) AEG_Gain_idx = 140*64;
		if(AEG_Gain_idx < 0) AEG_Gain_idx = 0;
	}
}

/*
 * 	調整畫面銳利度(0~15)
 * 	offset: 0~15
 */
void setStrengthWifiCmd(int offset)
{
	int idx;
	Adj_NR2D_Strength = (offset + 8) * 2;
	if(Adj_NR2D_Strength < 0)  Adj_NR2D_Strength = 0;
	if(Adj_NR2D_Strength > 46) Adj_NR2D_Strength = 46;
	Block2_2DNR_ready = 0;
	idx = (Adj_NR2D_Strength >> 1) - 16;
	set_A2K_JPEG_Sharpness(idx);
}

/*
 * 	取得銳利度的值, value[1]
 * 	value[0]: Adj_NR2D_Strength
 */
//void getStrength(int *value)
//{
//	*value = Adj_NR2D_Strength;
//}


/*
    Y = ((0.299  * R + 0.587 * G  + 0.114 * B));
    U = ((-0.147 * R - 0.289 * G  + 0.436 * B)) + 0x80;
    V = ((0.615  * R - 0.515 * G  - 0.100 * B)) + 0x80;
 */
//int Color_Saturation_Adj=0;
//int Color_Saturation[9] = {
////    Y_R     Y_G     Y_B     U_R     U_G     U_B     V_R     V_G     V_B
////  x0.299, x0.587, x0.114, x0.147, x0.289, x0.436, x0.615, x0.515, x0.100	// sam, 彩度不足
////   0x4C9,  0x964,  0x1D3,  0x25A,  0x4A0,  0x6FA,  0x9D7,  0x83D,  0x19A	// 高對比偏紅
//
////    Y_R     Y_G     Y_B     U_R     U_G     U_B     V_R     V_G     V_B
////  x0.299, x0.587, x0.114, x0.169, x0.331, x0.500, x0.500, x0.419, x0.081	// google
//     0x4C9,  0x964,  0x1D3,  0x3B4,  0x74C,  0xB00,  0xB00,  0x8B4,  0x24C	// 夜間比較黑	//161212  搭配FPGA(US360_7A50T_I4G.03.12_275B8_Tom)
////     0x4C9,  0x964,  0x1D3,  0x2B4,  0x54C,  0x800,  0x800,  0x6B4,  0x14C	// 夜間比較黑
//
////    Y_R     Y_G     Y_B     U_R     U_G     U_B     V_R     V_G     V_B
////  x0.224, x0.412, x0.364, x0.059, x0.921, x0.979, x0.846, x0.716, x0.130	// miller, 夜間偏藍, 濃度太多
////   0x395,  0x699,  0x5D2,   0xF1,  0xEBC,  0xFAD,  0xD88,  0xB74,  0x214	// 係數x1
//
////    Y_R     Y_G     Y_B     U_R     U_G     U_B     V_R     V_G     V_B
////  x0.224, x0.412, x0.364, x0.030, x0.460, x0.490, x0.592, x0.501, x0.091	// miller, 140701, adjust, excel計算值, 夜間濛濛的
////   0x395,  0x699,  0x5D2,   0x7A,  0x75C,  0x7D6,  0x978,  0x804,  0x174	// 係數x0.6
//};

void setFpgaEpFreq(int freq)
{
	int input_mode = Get_Input_Scale();
	int ep_d_line = EP_FRM_LENGTH_60Hz_D3_DEFAULT;
	if(freq == 50) {
		ISP_AEG_EP_IMX222_Freq = 1;
		AEG_EP_FRM_LENGTH = 0;
	}
	else {
		ISP_AEG_EP_IMX222_Freq = 0;
		AEG_EP_FRM_LENGTH = 0;
	}
	set_AEG_System_Freq_NP(ISP_AEG_EP_IMX222_Freq);
	db_debug("setFpgaEpFreq() Freq=%d  AEG_EP_FRM_LENGTH=0x%x\n", ISP_AEG_EP_IMX222_Freq, AEG_EP_FRM_LENGTH);
}

/*
 * 取得各個參數
 * [0]: AEG_EP_idx
 * [1]: AEG_Gain_idx
 * [2]: ISP_All_Command_DAEG_Y_target
 * [3]: ISP_All_State_D.AVG_YRGB.real_Y
 * [4]: ISP_All_Set_Reg.ISP_Block1.MR
 * [5]: ISP_All_Set_Reg.ISP_Block1.MG
 * [6]: ISP_All_Set_Reg.ISP_Block1.MB
 * [7]: ISP_All_Set_Reg.ISP_Block1.Gain_R
 * [8]: ISP_All_Set_Reg.ISP_Block1.Gain_G
 * [9]: ISP_All_Set_Reg.ISP_Block1.Gain_B
 */
void getScreenParameters(int *val)
{
	int tmp[6];

	*val     = AEG_EP_idx;
	*(val+1) = gain_tmp_idx; //AEG_Gain_idx;
	*(val+2) = ISP_All_Command_D.AEG_Y_target;
	*(val+3) = ISP_All_State_D.AVG_YRGB.real_Y;
	*(val+4) = ISP_All_Set_Reg.ISP_Block1.MR;
	*(val+5) = ISP_All_Set_Reg.ISP_Block1.MG;
	*(val+6) = ISP_All_Set_Reg.ISP_Block1.MB;
	*(val+7) = ISP_All_Set_Reg.ISP_Block1.Gain_R;
	*(val+8) = ISP_All_Set_Reg.ISP_Block1.Gain_G;
	*(val+9) = ISP_All_Set_Reg.ISP_Block1.Gain_B;

//tmp	getLuxValue(&tmp[0]);
	*(val+10) = tmp[0];
	*(val+11) = tmp[1];
	*(val+12) = tmp[2];
}

void setAdjWB(int idx, int value)
{
	switch(idx) {
	case 0: Adj_WB_R = (float)value / 1000; break;
	case 1: Adj_WB_G = (float)value / 1000; break;
	case 2: Adj_WB_B = (float)value / 1000; break;
	}

}

void getAdjWB(int *val)
{
    int wb_mode = getWhiteBalanceMode();
	*val     = (int)(Adj_WB_R * 1000);
	*(val+1) = (int)(Adj_WB_G * 1000);
	*(val+2) = (int)(Adj_WB_B * 1000);
	*(val+3) = (int)(WB_Mode_Table2[wb_mode][0] * 1000);
	*(val+4) = (int)(WB_Mode_Table2[wb_mode][1] * 1000);
	*(val+5) = (int)(WB_Mode_Table2[wb_mode][2] * 1000);
}

void getAEGParameters(int *val)
{
	*val     = ISP_All_State_D.AVG_YRGB.real_Y;
	*(val+1) = AEG_EP_idx;
	*(val+2) = gain_tmp_idx;
	*(val+3) = ISP_AEG_EP_IMX222_Freq;
}

void FPGAdriverInit(void)
{
	ISP_command_ready = 0;
}

void do2DNR(int en) {
	Block2_2DNR_ready = 0;
	do_Focus_2DNR = en;
}

int getBaseIntensity(void)
{
	int gain_max,exp_max,exp_min,DT_Gain,freq = 0;

	freq = ISP_AEG_EP_IMX222_Freq;
	int sflash = (((60-(10*freq))*2)*10)/getFPS();       // freq: 0->60Hz, 1->50Hz
                                                            // sflash: 4 = 30fps, 120/30=4
                                                            //         12 = 10fps, 120/10=12
                                                            //         120 = 1fps, 120/1=120

	
	if(AEG_Gain_manual == 1) gain_max = (140*64);	        // 140;
	else                     gain_max = (100*64);           // DT_Gain = 2;

	if(AEG_EP_manual == 1) exp_max = (138*64);              //8832;	//138*64: 1sec
	else                   exp_max = Flash_2_Gain_Table[sflash-1];

	exp_min = EP_Line_2_Gain_Table[freq].min_idx;
	//db_debug("send MCU base : AEG %d , expmin %d , expmax %d , gain_max %d\n",AEG_idx,exp_min,exp_max,gain_max);
	return ((AEG_idx - exp_min) * 160) / (gain_max + exp_max - exp_min);
}

// rex+ 180223
int get_ISP_AEG_EP_IMX222_Freq(void)
{
    return ISP_AEG_EP_IMX222_Freq;
}
// rex+ 180223
int get_AEG_EP_FPS(void)
{
    return AEG_EP_FPS;
}
void get_AEG_EP_Value(int *epH, int *epL, int *gainH)
{
    *epH = (AEG_EP_FRM_LENGTH >> 8) & 0xFF;         //AEG_EP_H;
    *epL = (AEG_EP_FRM_LENGTH) & 0xFF;              //AEG_EP_L;
    *gainH = AEG_gain_H;
}
void get_Temp_gain_RGB(int *r, int *g, int *b)
{
    *r = ISP_All_State_D.AWB_Gain.gain_R;           //Temp_gain_R;
    *g = ISP_All_State_D.AWB_Gain.gain_G;           //Temp_gain_G;
    *b = ISP_All_State_D.AWB_Gain.gain_B;           //Temp_gain_B;
}
// rex+ 180223
int Ep_Change_Init() {
    ep_init_change = 0;
}
// rex+ 180223
void set_AE_adj_tout(int rec_state, int time_lapse)
{
    if(rec_state != -2 && time_lapse != 0) {
    	switch(time_lapse) {
    	case 1: AE_adj_tout = 1000000*30;  break;   // 1sec
    	case 2: AE_adj_tout = 2000000*30;  break;   // 2sec
    	case 3: AE_adj_tout = 5000000*30;  break;   // 5sec
    	case 4: AE_adj_tout = 10000000*30; break;   // 10sec
    	case 5: AE_adj_tout = 30000000*30; break;   // 30sec
    	case 6: AE_adj_tout = 60000000*30; break;   // 60sec
    	case 7: AE_adj_tout = 100000*30;   break;   // 100ms
    	default: AE_adj_tout = 0; break;
    	}
    }
    else {
    	AE_adj_tout = 0;
    }
}

//void SetUVtoRGB(int idx, int value)
//{
//	set_A2K_ISP2_UVtoRGB(idx, value);
//}

//void GetUVtoRGB(int *value)
//{
//	get_A2K_ISP2_UVtoRGB(value);
//}

int HDR_Tone   = 0;		//-10 ~ +10
int Gamma_Rate[4] = {1, 30, 70, 110};	//HDR_Tone: -10:1	 	0:30		+10:70		+20:110
void SetHDRTone(int value)
{
	HDR_Tone = value;
	if(HDR_Tone < -10) HDR_Tone = -10;
	if(HDR_Tone >  10) HDR_Tone =  10;
	Gamma_Line_Ready = 1;
	set_A2K_JPEG_Tone(HDR_Tone);
}

int GetHDRTone(void)
{
	return HDR_Tone;
}

int Gamma_Max = 255;
void SetGammaMax(int value)
{
	Gamma_Max = value;
	if(Gamma_Max < 0)   Gamma_Max = 0;
	if(Gamma_Max > 255) Gamma_Max = 255;
	Gamma_Line_Ready = 1;
}

int GetGammaMax(void)
{
	return Gamma_Max;
}

int Gamma_Offset[2] = {35, 15};		//[0]:Cap / Rec / Timelapse [1]:Night / M-Mode
void SetGammaOffset(int idx, int value) {
	Gamma_Offset[idx&0x1] = value;
	Gamma_Line_Ready = 1;
}

int GetGammaOffset(int idx) {
	return Gamma_Offset[idx&0x1];
}

// rex+ 180911, 
//   Gamma_Table
unsigned short G_Table[3][1024];                     // 0x400為1倍, [0]:Init	 [1]:Live [2]:Cap
short G_Table_Conv[768];
unsigned long long pixel_Max=0; unsigned pixel_Sum[1024];
int run_Gamma_Set_Func = 0;
int Gamma_Adj_A=32, Gamma_Adj_B=1;           // A:調整(Y0~Y64)倍率, B:調整斜率
int Contrast_Adj = 0, Gamma_Line_Off = 0;
int Gamma_Line_Ready = 0;		//-1:none 0:init 1:make gamma line table
void AS2_Make_Gamma_Line(int c_mode)
{
    int i, j, s;
    unsigned DP[4] = {0x2A34,0x299B,0x135A,0xA03};
    unsigned DC[4] = {0xB12,0xB08,0x840,0x680};
    unsigned temp=0, tmax, lst_Y;
    float table_tmp[1024], table_tmp2[1024], table_tmp3[1024];	//table_tmp2:Gamma	table_tmp3:直線,	  合成table_tmp
    unsigned g_table_tmp;
    int offset;
    
    static unsigned lst_adj_b=5;
    // update "G_Table[]"
    if(Gamma_Line_Ready == 0){
    	Gamma_Line_Ready = 1;                          // rex- 190121, miller說先不做對比的調整, gamma使用固定table
        for(i = 0; i < 1024; i ++){
            s = i / 0x100;
            temp = DC[s] - (i * DP[s]) / 0x400;    // 0x2c48-0x13AE, 21BC-175F
            G_Table[1][i] = temp;                   // 0x400為1倍

            // temp -> 0x400 = 1倍, Y_Table[i] -> 256*64 = 1倍
            // Y_Table[] -> 256*64 = 最亮
            if((i&0x3) == 0x3){
                // Y_Table[i>>2] = (((i>>2) * (temp>>2)) >> 10) * 64;   // 倍率->'Y'絕對值x64
                Y_Table[i>>2] = ((i*16) * (temp)) >> 10;
                if(Y_Table[i>>2] > (255<<6)) Y_Table[i>>2] = (255<<6);
            }
        }
        lst_adj_b = Gamma_Adj_B;
        Gamma_Table_Init = 0;
    }
    else{
    	Gamma_Line_Ready = -1;

		int newRang = 10 * Contrast_Adj;
		int toMin = - newRang;
		int toMax = 255 + newRang;
		int adj_idx;
		int sub_rate, rate, r1, r2;

		if(Check_Is_HDR_Mode(c_mode) == 1 || c_mode == 3) {		//HDR / AEB
			offset = 0;
			adj_idx = (HDR_Tone + 10);	//0~20
		}
		else {										//WDR
			if(c_mode == 1 || c_mode == 2 || c_mode == 6 || c_mode == 10 || c_mode == 11 ||c_mode == 12) 	//Night / M-Mode / Rec / Timelapse
				offset = Gamma_Offset[1];
			else
				offset = Gamma_Offset[0];
			adj_idx = (HDR_Tone + 10 + 5);		//5~25, 解因offset調高, Gamma倍率過高問題
		}

		r1 = adj_idx / 10;
		if(r1 < 0) r1 = 0; if(r1 > 3) r1 = 3;
		r2 = r1 + 1;
		if(r2 < 0) r2 = 0; if(r2 > 3) r2 = 3;
		sub_rate = ( (adj_idx % 10) * 1024) / 10;
		rate = (Gamma_Rate[r1] * (1024-sub_rate) + Gamma_Rate[r2] * sub_rate) / 1024;

		if (toMin  > 0) toMin = 0;
		table_tmp[0] = toMin;
		for(i = 1; i < 1024; i++) {
			table_tmp[i] = table_tmp[i-1] + ( toMax - table_tmp[i-1]) * rate / 10000;
		}

		if(offset > 0) {
			for(i = 0; i < 1024; i++)
				table_tmp[i] += offset;
		}

		for(i = 1; i < 1024; i++) {
			table_tmp[i] = table_tmp[i] * (float)Gamma_Max / table_tmp[1023];
			if(table_tmp[i] < 0)   table_tmp[i] = 0;
			if(table_tmp[i] > Gamma_Max) table_tmp[i] = Gamma_Max;
			g_table_tmp = table_tmp[i] * 0x1000 / i;		//table_tmp2[i] * 0x400 * 4 / i;
			G_Table[1][i] = g_table_tmp;
			//if(g_table_tmp > 0xFFFF) G_Table[1][i] = 0xFFFF;		//0x400 = 1倍, 最多64倍, 雜訊過重
			if(g_table_tmp > 0x4000) G_Table[1][i] = 0x4000;		//0x400 = 1倍, 最多16倍
			if(g_table_tmp < 0x400)  G_Table[1][i] = 0x400;

			if( (i&0x3) == 0x3) {
				Y_Table[i>>2] = ( ( (i<<4) * G_Table[1][i]) >> 10);
				if(Y_Table[i>>2] > (255<<6)) Y_Table[i>>2] = (255<<6);
			}

			WDR_Live_Target_Table[i] = table_tmp[i];
		}
		G_Table[1][0] = G_Table[1][1];
		WDR_Live_Target_Table[0] = 0;
    }
    run_Gamma_Set_Func = 1;
    Send_WDR_Live_Table_Flag = 1;
}
int FX_Gamma_Addr_Idx = 0;	//1;
int get_FX_Gamma_Addr_Idx(void)
{
    return FX_Gamma_Addr_Idx;
}


int Init_Gamma_Table_En = 1;
// call from: FPGA_Download2()
void set_Init_Gamma_Table_En(void) {
    Init_Gamma_Table_En = 1;
}
int get_Init_Gamma_Table_En(void) {
    return Init_Gamma_Table_En;
}

void check_Init_Gamma_Table(void)
{
    int i, s_addr, t_addr, id, mode=1;
    if(Init_Gamma_Table_En == 1){
        for(i = 0; i < 1024; i++){
            G_Table[0][i] = 0x400;
            WDR_Live_Target_Table[i] = (i >> 2);
        }
        ua360_spi_ddr_write(F2_GAMMA_ADDR, &G_Table[0][0], 2048);
        for(i = 0; i < 2; i++){
            s_addr = F2_GAMMA_ADDR;
            t_addr = FX_GAMMA_ADDR;
            if(i == 0) id = AS2_MSPI_F0_ID;
            else       id = AS2_MSPI_F1_ID;
            AS2_Write_F2_to_FX_CMD(s_addr, t_addr, 2048, mode, id, 1);
        }
        FX_Gamma_Addr_Idx = 1;
        Init_Gamma_Table_En = 0;
        Gamma_Line_Ready = 0;
        db_debug("Init_Gamma_Table: ~\n");
    }
}

int Adj_R_sY = 150, Adj_R_nX = 47;//190;
int Adj_G_sY = 150, Adj_G_nX = 25;//100;
int Adj_B_sY = 150, Adj_B_nX = 0;
static int Adj_R_lst=0, Adj_G_lst=0, Adj_B_lst=0;

/*void AS2_Gamma_Conv_Table2(short *I_Table, short *O_Table, int RGB, int isWdr){
    int i, j;
    int Dif,Slope;
    short temp0, temp1, temp2;
    short adj_en=0, adj_sY, adj_nX, show=0;
    float delta=0.0;
    static int lst_Adj_R=0, lst_Adj_G=0, lst_Adj_B=0;

    switch(RGB){
    case 0: adj_nX = Adj_R_nX; adj_sY = Adj_R_sY;
            if(lst_Adj_R != Adj_R_nX){ lst_Adj_R = Adj_R_nX; show = 1; }
            break;
    case 1: adj_nX = Adj_G_nX; adj_sY = Adj_G_sY;
            if(lst_Adj_G != Adj_G_nX){ lst_Adj_G = Adj_G_nX; show = 1; }
            break;
    case 2: adj_nX = Adj_B_nX; adj_sY = Adj_B_sY;
            if(lst_Adj_B != Adj_B_nX){ lst_Adj_B = Adj_B_nX; show = 1; }
            break;
    }
    if(RGB == 1 && isWdr == 1) adj_nX = 0;
    
    if(adj_nX > 0){
        if(adj_en == 0){
            for(j = 1023; j >= 0; j--)   // 取得gamma轉換後的數值=j
                if(((j*I_Table[j]) >> 12) < adj_sY*4) break;        // 0x1000 = 1倍
            if(j > 1023) j = 1023;
            if(j < 0) j = 0;
            adj_sY = j;
            delta = (float)(adj_nX) / (float)(1024 - j);            // Adj_R_nX, 256=1倍
            adj_en = 1;
        }
    }

    for(i = 0 ; i < 1024 ; i +=4){
        temp0 = I_Table[i] >> 4;
        if(adj_en == 1 && i >= adj_sY){
            temp2 = temp0 + ((float)(i - adj_sY + 1) * delta);
            if(show == 1 && (i&0x3f) == 0)
                db_debug("rgb=%d i=%d sY=%d temp0=%d->%d delta=%f\n", RGB, i, adj_sY, temp0, temp2, ((float)(i - adj_sY + 1) * delta));
            temp0 = temp2;
        }
        if(temp0 > 0x7FF) temp0 = 0x7FF;
        if(i < 1020){
            temp1 = I_Table[i + 4] >> 4;
            if(adj_en == 1 && (i+4) >= adj_sY){
                temp1 = temp1 + ((float)((i+4) - adj_sY + 1) * delta);
            }
            if(temp1 > 0x7FF) temp1 = 0x7FF;
            Dif = temp0 - temp1;
            if(Dif < 0) Dif = 0;
            Slope = Dif / 4;
            //Slope = Dif;
        }else{
            temp1 = I_Table[i + 3] >> 4;
            if(adj_en == 1 && (i+3) >= adj_sY){
                temp2 = temp1 + ((float)((i+3) - adj_sY + 1) * delta);
                if(show == 1 && (i+3) == 1023)
                    db_debug("rgb=%d i=%d sY=%d temp1=%d->%d delta=%f\n", RGB, i+3, adj_sY, temp1, temp2, ((float)((i+3) - adj_sY + 1) * delta));
                temp1 = temp2;
            }
            if(temp1 > 0x7FF) temp1 = 0x7FF;
            Dif = temp0 - temp1;
            if(Dif < 0) Dif = 0;
            Slope = Dif / 3;
            //Slope = (Dif * 4) / 3;
        }
        if(Slope < 32) Slope = Slope;
        else           Slope = 0x1F;
        O_Table[i/4] = (Slope << 11) | (temp0 & 0x7FF);
    }
}*/

void Gamma_Set_Function(void)
{
    int i, s_addr, t_addr, id, mode=1;      // mode: 0->DDR_to_IO, 1->DDR_to_DDR
    int d_cnt = read_F_Com_In_Capture_D_Cnt();

    // 拍照時，不運算GAMMA
    if(d_cnt != 0) return;
    if(run_Gamma_Set_Func != 1) return;
    run_Gamma_Set_Func = 0;

    // 1. Init G_Table[0]
    check_Init_Gamma_Table();

    // 2. Live G_Table[1]
    ua360_spi_ddr_write(F2_GAMMA_ADDR + 2048, &G_Table[1][0], 2048);
    for(i = 0; i < 2; i++) {
        s_addr = F2_GAMMA_ADDR + 2048;
        t_addr = FX_GAMMA_ADDR + 2048;     //FPGA_REG_SET_GAMMA;
        if(i == 0) id = AS2_MSPI_F0_ID;
        else       id = AS2_MSPI_F1_ID;
        AS2_Write_F2_to_FX_CMD(s_addr, t_addr, 2048, mode, id, 1);
    }

    if(Send_WDR_Live_Table_Flag == 1) {
    	Send_WDR_Live_Table_Flag = 0;
    	send_WDR_Live_Diffusion_Table();
    }
}

void Cap_Gamma_Set_Function(int gain)
{
    int i, s_addr, t_addr, id, mode=1;      // mode: 0->DDR_to_IO, 1->DDR_to_DDR
    int max;
    float rate;

    // 3. Cap G_Table[2]
	rate = (float)gain / 1280.0;
	max = 16 * 0x400 / pow(2, rate);
	if(max > 0xFFFF)      max = 0xFFFF;
	else if(max < 0x2000) max = 0x2000;

	for(i = 0; i < 1024; i++) {
		if(G_Table[1][i] > max)
			G_Table[2][i] = max;
		else
			G_Table[2][i] = G_Table[1][i];
	}

    ua360_spi_ddr_write(F2_GAMMA_ADDR + 4096, &G_Table[2][0], 2048);
    for(i = 0; i < 2; i++) {
        s_addr = F2_GAMMA_ADDR + 4096;
        t_addr = FX_GAMMA_ADDR + 4096;     //FPGA_REG_SET_GAMMA;
        if(i == 0) id = AS2_MSPI_F0_ID;
        else       id = AS2_MSPI_F1_ID;
        AS2_Write_F2_to_FX_CMD(s_addr, t_addr, 2048, mode, id, 1);
    }
}

/*
 * 	設定對比值資料
 * 	val: 對比強度 -7 ~ +7
 */
int saveOrgOffest = -1;
void setContrast(int val)
{
	Contrast_Adj = val;
	int rgbOffest[2];
	getISP1RGBOffset(rgbOffest);
	if(saveOrgOffest == -1){
		saveOrgOffest = rgbOffest[1];
	}
	if(val >= 0){
		setISP1RGBOffset(1,saveOrgOffest);
	}else{
		setISP1RGBOffset(1,(val * -7) + saveOrgOffest);
	}
	Gamma_Line_Ready = 1;
	set_A2K_JPEG_Contrast(Contrast_Adj);
}

/*
 * 	設定G_Table是否固定為0x1000
 * 	val: 0: off 1: on
 */
void setGammaLineOff(int val)
{
	Gamma_Line_Off = val;
}

/*
 * 	取得G_Table是否固定為0x1000
 */
int getGammaLineOff()
{
	return Gamma_Line_Off;
}

/*
 * 	取得FPGA溫度偵測
 */
int Read_FPGA_Pmw(void)
{
    int addr, err=0;
    unsigned value=0;

    addr = FPGA_PWM_IO_ADDR;
    spi_read_io_porcess_S2(addr, &value, 4);

    int fpgaA0 = value & 0x000000FF;
    int fpgaA1 = (value & 0x0000FF00) >> 8;

    //db_debug("fpga temp: %d,%d\n",fpgaA0,fpgaA1);
    int avg = (fpgaA0+ fpgaA1) / 2;

    return avg;
}

void setPhotoLuxValue(){
	float pow_tmp, pow_tmp2,mLux = 0;
	pow_tmp  = pow(2, ((float)(exp_tmp_idx + gain_tmp_idx) / 1280.0) );
	pow_tmp2 = pow(( (float)Brightness256to1024(ISP_All_State_D.AVG_YRGB.real_Y) / (float)Brightness256to1024(100) ),2 );
	mLux = 4900.0 * 100.0 / pow_tmp * pow_tmp2;
	set_A2K_LuxValue((int)mLux);
}

int Live_EXP_N=1, Live_EXP_M=15, Live_ISO=100;
void setLiveShowValue(int exp_n, int exp_m, int iso)
{
    Live_EXP_N = exp_n;
    Live_EXP_M = exp_m;
    Live_ISO = iso;
}
void getLiveShowValue(int *value)
{
    *(value + 0) = Live_EXP_N;
    *(value + 1) = Live_EXP_M;
    *(value + 2) = Live_ISO;
}

void setAEGBExp1Sec(int sec) {
	AEG_B_Exp_1Sec = sec;
}
void setAEGBExpGain(int gain) {
	AEG_B_Exp_Gain = gain * 64;
	if(AEG_B_Exp_Gain > 140*64) AEG_B_Exp_Gain = 140*64;
	if(AEG_B_Exp_Gain < 0) AEG_B_Exp_Gain = 0;
}
#ifdef ANDROID_CODE
void setGammaAdjA(int adj_a){ Gamma_Adj_A = adj_a; }       // 159
void setGammaAdjB(int adj_b){ Gamma_Adj_B = adj_b; }       // 160
void setYOffValue(int value){ Y_Off_Value = value; }
int getGammaAdjA(){ return Gamma_Adj_A; }
int getGammaAdjB(){ return Gamma_Adj_B; }
int getYOffValue(){ return Y_Off_Value; }
int getAEGBExp1Sec(){ return AEG_B_Exp_1Sec; }
int getAEGBExpGain(){ return AEG_B_Exp_Gain; }

void setAdjRsY(int R){ Adj_R_sY = R; Gamma_Line_Ready = 1; }
void setAdjGsY(int G){ Adj_G_sY = G; Gamma_Line_Ready = 1; }
void setAdjBsY(int B){ Adj_B_sY = B; Gamma_Line_Ready = 1; }
void setAdjRnX(int R){ Adj_R_nX = R; Gamma_Line_Ready = 1; }
void setAdjGnX(int G){ Adj_G_nX = G; Gamma_Line_Ready = 1; }
void setAdjBnX(int B){ Adj_B_nX = B; Gamma_Line_Ready = 1; }
int getAdjRsY(){ return Adj_R_sY; }
int getAdjGsY(){ return Adj_G_sY; }
int getAdjBsY(){ return Adj_B_sY; }
int getAdjRnX(){ return Adj_R_nX; }
int getAdjGnX(){ return Adj_G_nX; }
int getAdjBnX(){ return Adj_B_nX; }

void setAWBCrCbG(int G){ AWB_CrCb_G = G; }
int getAWBCrCbG(){ return AWB_CrCb_G; }
void setLOGEEn(int en){ LOGE_Enable = en; }
int getLOGEEn(){ return LOGE_Enable; }
#endif

struct Sensor_ADT_Struct  S_ADT[512];
float  S_AD_Pint[16][2] = {
	{	  0	     ,	  0			},
	{	 15.566  ,	 17.081		},
	{	 31.266  ,	 34.162		},
	{	 46.866  ,	 51.244		},
	{	 63.333  ,	 68.325		},
	{	 78.833  ,	 85.407		},
	{	 96.066  ,	102.488		},
	{	112.266  ,	119.57		},
	{	127.766  ,	136.651		},
	{	142.7	 ,	153.733		},
	{	159.666  ,	170.814		},
	{	180.6	 ,	187.895		},
	{	201.0	 ,	204.977		},
	{	219.4	 ,	222.058		},
	{	240.633  ,	239.14		},
	{	255.999  ,	254.0		}
};

int  S_AD_B[512];
int  S_AD_C[512];

int ADT_Adj_Idx = 374, ADT_Adj_Value = 0;
void SetADTAdj(int idx, int value)
{
	if(idx == 0) ADT_Adj_Idx = value;
	else		 ADT_Adj_Value = value;
	Senser_AD_Trans(ADT_ISO);
}

int GetADTAdj(int idx)
{
	int value;
	if(idx == 0) value = ADT_Adj_Idx;
	else		 value = ADT_Adj_Value;
	return value;
}

void GetSADPint(int *value)
{
	*value 	    = S_AD_Pint[0][1] * 1000;
	*(value+1)  = S_AD_Pint[1][1] * 1000;
	*(value+2)  = S_AD_Pint[2][1] * 1000;
	*(value+3)  = S_AD_Pint[3][1] * 1000;
	*(value+4)  = S_AD_Pint[4][1] * 1000;
	*(value+5)  = S_AD_Pint[5][1] * 1000;
	*(value+6)  = S_AD_Pint[6][1] * 1000;
	*(value+7)  = S_AD_Pint[7][1] * 1000;
	*(value+8)  = S_AD_Pint[8][1] * 1000;
	*(value+9)  = S_AD_Pint[9][1] * 1000;
	*(value+10) = S_AD_Pint[10][1] * 1000;
	*(value+11) = S_AD_Pint[11][1] * 1000;
	*(value+12) = S_AD_Pint[12][1] * 1000;
	*(value+13) = S_AD_Pint[13][1] * 1000;
	*(value+14) = S_AD_Pint[14][1] * 1000;
	*(value+15) = S_AD_Pint[15][1] * 1000;
}

int Senser_One_Trans(int In)
{
  int i;
  int p0, p1;
  int D0, D1;
  int delta1;
  int delta2;
  int tmp;

  for (i = 0; i < 16; i++) {
	 if (S_AD_Pint[i][0] * 2 > In) break;
  }

  p0 = i-1; p1 = i;
  if (i >= 16) p1 = 15;

  delta1 =  S_AD_Pint[p1][0] * 2000 - S_AD_Pint[p0][0] * 2000;
  delta2 =  S_AD_Pint[p1][1] * 2000 - S_AD_Pint[p0][1] * 2000;

  if(delta1 == 0)
	  tmp = S_AD_Pint[p0][1] * 2000 + (In * 1000 - S_AD_Pint[p0][0] * 2000);
  else
	  tmp = S_AD_Pint[p0][1] * 2000 + (In * 1000 - S_AD_Pint[p0][0] * 2000)* delta2 / delta1;

  return tmp;
}

void Write_ADT_Table(void)
{
	int i, f_id;
	int Addr, s_addr, t_addr, size, id, mode;
	unsigned Data[512];
	unsigned *ptr;
	memset(&Data[0], 0, sizeof(Data) );

	for(i = 0; i < 256; i++) {
		ptr = &S_ADT[i*2];
		Data[i*2] = 0xCCA33000 + i;
		Data[i*2+1] = *ptr;
	}

	Addr = ADT_TABLE_ADDR;
	ua360_spi_ddr_write(Addr, &Data[0], sizeof(Data) );
	for(f_id = 0; f_id < 2; f_id++) {
		s_addr = Addr;
		t_addr = 0;
		size = sizeof(Data);
		mode = 0;
		if(f_id == 0) id = AS2_MSPI_F0_ID;
		else		  id = AS2_MSPI_F1_ID;
		AS2_Write_F2_to_FX_CMD(s_addr, t_addr, size, mode, id, 1);
	}
}

void Senser_AD_Trans(int ISO)
{
  int i,j,k,l;
  int Temp;
  int ISO_Scale;
  int ISO_B;
  int len;
  int adj_b, adj_value;


  int adj_v;
  for (i = 0; i < 512; i++) {
	 S_AD_B[i] = Senser_One_Trans(i) ;
	//db_debug("Senser_AD_Trans() ISO=%d i=%d S_AD_B=%d adj_v=%d\n", ISO, i, S_AD_B[i], adj_v);
  }

  adj_value = ADT_Adj_Value * 1000;
  for (i = 0; i < 512; i++) {
	  if(i > ADT_Adj_Idx) {
		  len = 512 - ADT_Adj_Idx;
		  adj_b = ( (len - i + ADT_Adj_Idx) * adj_value / len);
		  S_AD_B[i] += adj_b;
	  }
	  else {
		  len = ADT_Adj_Idx;
		  adj_b = (i * adj_value / len);
		  S_AD_B[i] += adj_b;
	  }
  }

  for (i = 0; i < 512; i++) {
	 S_AD_C[i] = S_AD_B[i] * 0xFF0 / S_AD_B[511];
  }

  S_ADT[i].Base = S_AD_C[0];
  S_ADT[i].Rate = S_AD_C[i];

  for (i = 0; i < 511; i++) {
	 S_ADT[i].Base = S_AD_C[i];
	 S_ADT[i].Rate = S_AD_C[i+1] - S_AD_C[i];
  }

  S_ADT[511].Base = S_AD_C[511];
  S_ADT[511].Rate = S_ADT[510].Rate;

  Write_ADT_Table();
}
