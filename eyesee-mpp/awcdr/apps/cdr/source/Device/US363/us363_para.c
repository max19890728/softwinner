/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/us363_para.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>                  /* for videodev2.h */
#include <time.h>
#include <pthread.h>

#include "Device/US363/Cmd/us360_func.h"
#include "Device/US363/Cmd/jpeg_header.h"
//#include "fpga_driver.h"
//#include "test.h"
#include "Device/US363/Cmd/us360_define.h"
//#include "us360.h"
#include "Device/US363/Cmd/Smooth.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::US363Para"

int mVideoWidth          = 3840;
int mVideoHeight         = 1920;

static pthread_mutex_t pthread_f2_io_mutex;

// ==========================================================
//-------------Around camera US360 miller 20150530-----------
// ==========================================================

int Sitching_Idx = 0;

//debug
void getSensor2(int sensor, int *value)
{
	int i;
	//for(i = 0; i < 4; i++) {
		i = sensor;
		value[0]  = Adj_Sensor[LensCode][i].Rotate_R1;
		value[1]  = Adj_Sensor[LensCode][i].Rotate_R2;
		value[2]  = Adj_Sensor[LensCode][i].Rotate_R3;
		value[3]  = Adj_Sensor[LensCode][i].Zoom_X;
		value[4]  = Adj_Sensor[LensCode][i].Zoom_Y;
		value[5]  = Adj_Sensor[LensCode][i].S[0].X;
		value[6]  = Adj_Sensor[LensCode][i].S[0].Y;
		value[7]  = Adj_Sensor[LensCode][i].S[1].X;
		value[8]  = Adj_Sensor[LensCode][i].S[1].Y;
		value[9]  = Adj_Sensor[LensCode][i].S[2].X;
		value[10] = Adj_Sensor[LensCode][i].S[2].Y;
		value[11] = Adj_Sensor[LensCode][i].S[3].X;
		value[12] = Adj_Sensor[LensCode][i].S[3].Y;
		value[13] = Adj_Sensor[LensCode][i].S[4].X;
		value[14] = Adj_Sensor[LensCode][i].S[4].Y;
	//}
}

//debug
void setSensor(int sensor, int idx, int value)
{
	int i;

	i = sensor;
	switch(idx) {
	case 0:  Adj_Sensor[LensCode][i].Rotate_R1 = value; break;
	case 1:  Adj_Sensor[LensCode][i].Rotate_R2 = value; break;
	case 2:  Adj_Sensor[LensCode][i].Rotate_R3 = value; break;
	case 3:  Adj_Sensor[LensCode][i].Zoom_X = value; break;
	case 4:  Adj_Sensor[LensCode][i].Zoom_Y = value; break;
	case 5:  Adj_Sensor[LensCode][i].S[0].X = value; break;
	case 6:  Adj_Sensor[LensCode][i].S[0].Y = value; break;
	case 7:  Adj_Sensor[LensCode][i].S[1].X = value; break;
	case 8:  Adj_Sensor[LensCode][i].S[1].Y = value; break;
	case 9:  Adj_Sensor[LensCode][i].S[2].X = value; break;
	case 10: Adj_Sensor[LensCode][i].S[2].Y = value; break;
	case 11: Adj_Sensor[LensCode][i].S[3].X = value; break;
	case 12: Adj_Sensor[LensCode][i].S[3].Y = value; break;
	case 13: Adj_Sensor[LensCode][i].S[4].X = value; break;
	case 14: Adj_Sensor[LensCode][i].S[4].Y = value; break;
	}

	Adj_Sensor_Command[i] = Adj_Sensor[LensCode][i];
	do_ST_Line_Offset();
}

int Sensor_Mask_Line_Shift_Adj = -5;
void setSensorMaskLineShiftAdj(int value)
{
    Sensor_Mask_Line_Shift_Adj = value;
}

void getSensorMaskLineShiftAdj(int *val)
{
    *val = Sensor_Mask_Line_Shift_Adj;
}

int DebugJPEGMode = 0;    // 0:normal 1:debug
int DebugJPEGaddr = 0x00080000;
int ISP2_Sensor = -1;    // 0~3:sensor0~3 4:all

/*
 *     下FPGA主CMD
 *     sub_state: 0:ISP1  1:ISP2  2:Stitch  3:DMA  4:JPEG  5:USB  6:REV  7:512 Table
 *     play_mode: 0: Global 1: Front 2: 360 3: 240 4: 180+180 5: Split x 4 6: PIP
 *     res_mode: 0:Fix  1:10M  2:2.5M  3:2M  4:1M  5:D1 6:3072x576 7:5M
 *     fps: 每秒幾張 * 10 (ex: 30fps = 300)
 *     flag: 當設定JPEG CMD時, 若flag=1, JPEG壓縮位置為DebugJPEGaddr
 *     isInit: 0:下FPS Standby 1:不下FPS Standby    (Init時不下)
 *     usb_en: 0:FPGA不送影像出來 1:FPGA送影像出來    (切MODE時先不送, DELAY後再開啟)
 */
int writeCmdTable(int sub_state, int res_mode, int fps, int flag, int isInit, int usb_en)
{
    if(sub_state < 0 || sub_state > 6){
           db_error("writeCmdTable: err-1 sub_state=%d\n", sub_state);
           return -2;
    }

    if(fps == 0) {
        db_error("writeCmdTable: err-2 fps=0\n");
        return -1;
    }

    if(sub_state == 4 && flag == 1) {
        DebugJPEGMode = 1;
    }
    else if(sub_state == 4 && flag == 0){
        DebugJPEGMode = 0;
    }


    if(sub_state == 0) {
        // ISP1 CMD
        /*
        RUL_S2_66M_11500_5760       fps = 100
        RUL_S2_33M_7680_3840        fps = 100
        RUL_S2_16M_6144_3072        fps = 100
        RUL_S2_8M_3840_1920         fps = 100
        RUL_S2_4M_2880_1440         fps = 150 or 120
        RUL_S2_2M_1920_960          fps = 300 or 250
        */
//tmp        set_live555_frame_rate(fps/10);
    }
    else if(sub_state == 1) {} // ISP2 CMD
    else if(sub_state == 2) {} // Stitch CMD
    else if(sub_state == 4) {} // JPEG CMD
    else if(sub_state == 5) {} // DMA CMD
        
    

    return 0;
}

// [mode][res_mode]
unsigned ResolutionEn[7][15] = {
        {0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1},            // Global
        {0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0},            // Front
        {0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0},            // 360
        {0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},            // 240
        {0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0},            // 180
        {0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0},            // Split * 4
        {0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0}             // PIP
};

/*
 *     設定現在MODE ResolutionMode FPS
 *     c_mode: 0:Cap, 1:Rec, 2:TimeLapse, 3:HDR(3P), 4:RAW(5P), 5:WDR, 6:Night 7:NightWDR 8:Sport 9:SportWDR 10:RecWDR 11:TimeLapseWDR  12:M-Mode 13:remove 14:3D-Model
 *     mode: StitchingMode
 *     res_mode: ResolutionMode
 *     fps: FPS(ex: 300=30fps 150=15fps 60=6fps)
 */
void setStitchingOut(int c_mode, int mode, int res_mode, int fps)
{
    int i, j;
    int res, fps_tmp;
    int jpeg_3d_mode = get_A2K_JPEG_3D_Res_Mode();

    res = res_mode;
    while(ResolutionEn[mode][res] != 1) {
        res++;
        if(res > 14) res = 0;
        if(res == res_mode) {
            db_error("setStitchingOut: res_mode err!\n");
            break;
        }
    }

    Stitching_Out.Output_Mode = mode;
    Stitching_Out.OC[mode].Resolution_Mode = res;
    db_debug("setStitchingOut: FPS=%d mode=%d res=%d\n", fps, mode, res);

    switch(res) {
    default:
    case 1:        //12K
            if(c_mode == 14) {
            	if(jpeg_3d_mode == 0) { mVideoWidth  = S2_RES_3D1K_WIDTH; mVideoHeight = S2_RES_3D1K_HEIGHT; }
            	else				  { mVideoWidth  = S2_RES_3D4K_WIDTH; mVideoHeight = S2_RES_3D4K_HEIGHT; }
            }
            else {
            	mVideoWidth  = S2_RES_4K_WIDTH;
            	mVideoHeight = S2_RES_4K_HEIGHT;
            }
            break;
    case 2:        //4K
            mVideoWidth  = S2_RES_4K_WIDTH;
            mVideoHeight = S2_RES_4K_HEIGHT;
            break;
    case 7:        //8K
            if(c_mode == 2) { mVideoWidth  = S2_RES_3K_WIDTH; mVideoHeight = S2_RES_3K_HEIGHT; }
            else            { mVideoWidth  = S2_RES_4K_WIDTH; mVideoHeight = S2_RES_4K_HEIGHT; }
            break;
    case 8:        //10K
            if(c_mode == 2) { mVideoWidth  = S2_RES_3K_WIDTH; mVideoHeight = S2_RES_3K_HEIGHT; }
            else            { mVideoWidth  = S2_RES_4K_WIDTH; mVideoHeight = S2_RES_4K_HEIGHT; }
    case 12:        //6K
            if(c_mode == 2) { mVideoWidth  = S2_RES_3K_WIDTH; mVideoHeight = S2_RES_3K_HEIGHT; }
            else            { mVideoWidth  = S2_RES_4K_WIDTH; mVideoHeight = S2_RES_4K_HEIGHT; }
            break;
    case 13:        //3K
            mVideoWidth  = S2_RES_3K_WIDTH;
            mVideoHeight = S2_RES_3K_HEIGHT;
            break;
    case 14:        //2K
            mVideoWidth  = S2_RES_2K_WIDTH;
            mVideoHeight = S2_RES_2K_HEIGHT;
            break;
    }
}

/*
 * 設定FPGA壓縮位址
 */
void setJPEGaddr(int mode, int addr, int sensor)
{
    DebugJPEGMode = mode;
    if(DebugJPEGMode == 1) {
        DebugJPEGaddr = addr;
        if(DebugJPEGaddr < 0) DebugJPEGaddr = 0;
        ISP2_Sensor = sensor;
    }
    else if(DebugJPEGMode == 2) {
        DebugJPEGaddr = addr;
        if(DebugJPEGaddr < 0) DebugJPEGaddr = 0;
        ISP2_Sensor = -1;
    }
    else {
        DebugJPEGaddr = 0;
        ISP2_Sensor = -1;
    }
}

/*
 * 參考文件
 * https://docs.google.com/spreadsheets/d/1RSTA72kUnzRpOYWGbEZJvK_4SjPv-1vnI3w53I2hfms/edit?ts=56f22e35#gid=1438385027
 * */
int check_err_vsync_count(void)
{
    static int sen0_cnt=0, sen1_cnt=0, sen2_cnt=0, sen3_cnt=0, err=0;
    unsigned temp;
    int addr = 0xBF0;

    spi_read_io_porcess_S2(addr &temp, 4);
    if(sen0_cnt == (temp&0xff)) err++;
    else sen0_cnt = (temp&0xff);
    if(sen1_cnt == ((temp>>8)&0xff)) err++;
    else sen1_cnt = ((temp>>8)&0xff);
    if(sen2_cnt == ((temp>>16)&0xff)) err++;
    else sen2_cnt = ((temp>>16)&0xff);
    if(sen3_cnt == ((temp>>24)&0xff)) err++;
    else sen3_cnt = ((temp>>24)&0xff);
    return err;
}

//Parameter_struct US360parameter;
int A_L_I_Global_phi_idx = -9;
int A_L_I_Global_phi2_idx = -9;
int NowSpeed = 0;
int getNowSpeed()
{
    int speed;
    if(getCameraPositionMode() == 0) speed = NowSpeed;
    else                             speed = -NowSpeed;
    return speed;
}

// weber+ 160413
/*
 * 設定JPEG檔頭時戳
 * time: 時戳     (單位:ms)
 */
unsigned long long Timestamp = 0;
void setOutModePlane0Timestamp(unsigned long long time){
    Timestamp = time;
}

// weber+ 160413
/*
 * 取得JPEG檔頭時戳
 * time: 時戳     (單位:ms)
 */
unsigned long long getOutModePlane0Timestamp(void){
    return Timestamp;
}

extern int Waiting_State;

/*
 * 取得40點縫合位置
 * idx: 哪一點(0~39)
 * value[2]
 */
void getALIThitaPhi(int idx, int *value)
{
    *value     = A_L_I2[idx].phi_offset;
    *(value+1) = A_L_I2[idx].thita_offset;
}

int get_global_phi(int idx)
{
	idx += 27;
	if(idx < 0)       idx = 0;
	else if(idx > 63) idx = 63;
	return idx;		//(idx * RULE_UNIT[LensCode] / 64.0);	//0~63
}

/*
 *  遠端調整Global_phi_idx (-32~32)
 *  phi_idx: -32~32
 */
void setALIGlobalPhiWifiCmd(int phi_idx)
{
	int global_phi1_tmp;
	A_L_I_Global_phi_idx = phi_idx;
	Smooth_Com.Global_phi_Mid = get_global_phi(A_L_I_Global_phi_idx);
	db_debug("setALIGlobalPhiWifiCmd() idx=%d Global_phi_Mid=%d\n", phi_idx, Smooth_Com.Global_phi_Mid);
}

/*
 *  調整Global_phi2_idx (-32~32)
 *  phi_idx: -32~32
 */
void setALIGlobalPhi2WifiCmd(int phi_idx)
{
	int global_phi2_tmp;
	A_L_I_Global_phi2_idx = phi_idx;
	Smooth_Com.Global_phi_Top = get_global_phi(A_L_I_Global_phi2_idx);
	db_debug("setALIGlobalPhi2WifiCmd() idx=%d Global_phi_Top=%d\n", phi_idx, Smooth_Com.Global_phi_Top);
}

//extern unsigned short Trans_ZY_phi[128][256];
//extern unsigned short Trans_ZY_thita[128][256];
//extern unsigned short Trans_Sin[256];
/*
 *     儲存 Trans_ZY_phi[][] Trans_ZY_thita[][] Trans_Sin[][] Table, Debug用
 */
void GetZYTable(void)
{
    FILE *fp;
    char path[100];

    sprintf(path, "/mnt/sdcard/ZY_phi.bin");
    fp = fopen(path, "wb");
    if(fp != NULL) {
        fwrite(&Trans_ZY_phi[0][0], sizeof(Trans_ZY_phi), 1, fp);
        fclose(fp);
    }

    sprintf(path, "/mnt/sdcard/ZY_thita.bin");
    fp = fopen(path, "wb");
    if(fp != NULL) {
        fwrite(&Trans_ZY_thita[0][0], sizeof(Trans_ZY_thita), 1, fp);
        fclose(fp);
    }

    sprintf(path, "/mnt/sdcard/Sin.bin");
    fp = fopen(path, "wb");
    if(fp != NULL) {
        fwrite(&Trans_Sin[0], sizeof(Trans_Sin), 1, fp);
        fclose(fp);
    }
}

/*unsigned sensor_clk_mode;
unsigned short US360_Sen_CK_Par[3][5] =
{
    {0x179e,0x179e,0x179e,0x179e,0x13cf},        //sensor clk / 3
    {0x1cb2,0x1cb2,0x1cb2,0x1cb2,0x1659},        //sensor clk / 5
    {0x128a,0x128a,0x128a,0x128a,0x1145},        //sensor clk / 1

};*/
/*unsigned US360_Sen_CK_Addr[5] = {0x20,0x28,0x30,0x38,0x40};
void US360_Sen_CK_Set(int sel, int flag)
{
    unsigned Data[2];
    int i,j,Addr;
    static int clk_sel_lst = -1;

    if(sel == clk_sel_lst && flag != 1) return;
    clk_sel_lst = sel;
    //i2cCmosStandby(0);

    Addr = 0x40000;
db_debug("US360_Sen_CK_Set() sel=%d\n", sel);
    for(i = 0; i < 5; i++) {
        Data[0] = US360_Sen_CK_Addr[i] + Addr;
        Data[1] = US360_Sen_CK_Par[sel][i];
        SPI_Write_IO(0x9, Data, 8);
    }
}*/

extern CIS_CMD_struct FS_TABLE[CIS_TAB_N];
extern CIS_CMD_struct D2_TABLE[CIS_TAB_N];
extern CIS_CMD_struct D3_TABLE[CIS_TAB_N];

int GPS_En=0;
double GPS_Latitude=0, GPS_Longitude=0, GPS_Altitude=0;
void setGPSLocation(int en, double latitude, double longitude, double altitude)
{
	static double latitude_lst=0, longitude_lst=0;
	static int Latitude[3]={0,0,0};
	static char *Latitude_CC;
	static int Longitude[3]={0,0,0};
	static char *Longitude_CC;
	double latitude_tmp=0, longitude_tmp=0;
	int Altitude=0;

	GPS_En			= en;
	GPS_Latitude	= latitude;
	GPS_Longitude	= longitude;
	GPS_Altitude	= altitude;

	if(latitude > 0) { latitude_tmp = latitude;    Latitude_CC = "N   "; }  // CC("N   ");
	else             { latitude_tmp = -(latitude); Latitude_CC = "S   "; }  // CC("S   ");
	if(latitude_lst != latitude) {
		Latitude[0] = latitude_tmp;
		Latitude[1] = (latitude_tmp - (double)Latitude[0]) * 60.0;
		Latitude[2] = ( ( (latitude_tmp - (double)Latitude[0]) * 60.0) - (double)Latitude[1]) * 60.0 * (double)JPEG_GPS_MUL;
		latitude_lst = latitude;
	}
	
	if(longitude > 0) { longitude_tmp = longitude;    Longitude_CC = "E   "; }   // CC("E   ");
	else              { longitude_tmp = -(longitude); Longitude_CC = "W   "; }   // CC("W   ");
	if(longitude_lst != longitude) {
		Longitude[0] = longitude_tmp;
		Longitude[1] = (longitude_tmp - (double)Longitude[0]) * 60.0;
		Longitude[2] = ( ( (longitude_tmp - (double)Longitude[0]) * 60.0) - (double)Longitude[1]) * 60.0 * (double)JPEG_GPS_MUL;
		longitude_lst = longitude;
	}
	Altitude = altitude * 1000;
	set_A2K_JPEG_GPS_v(en, Latitude, *(int *)Latitude_CC,
                      Longitude, *(int *)Longitude_CC, Altitude);
}

void getGPSLocation(int *val)
{
	*val     = GPS_En;
	*(val+1) = (int)(GPS_Latitude * 1000);
	*(val+2) = (int)(GPS_Longitude * 1000);
	*(val+3) = (int)(GPS_Altitude * 1000);
}

//extern int Capture_Long_EP, Save_Jpeg_Now_Cnt, Save_Jpeg_End_Cnt;

void SendMainCmdPipe(int c_mode, int t_mode, int sync_mode)
{
    static unsigned long long curTime, lstTime=0, runTime;
    static int Init_Start = 0;
	int M_Mode, S_Mode;
	int ret = 0;

//tmp    get_current_usec(&curTime);
    if(curTime < lstTime || lstTime == 0) lstTime = curTime;    // rex+ 151229, 防止例外錯誤
    else if((curTime - lstTime) >= 500000){
        runTime = curTime - lstTime;
        db_debug("SendMainCmdPipe: runTime > %lld(ms)\n", runTime/1000);
    }
    //lstTime = curTime;
        
    if(Init_Start == 0){
        Init_Start = 1;
        do_sc_cmd_Tx_func("CIST", "CPDS");
        do_sc_cmd_Tx_func("CIST", "CPFS");
        do_sc_cmd_Tx_func("CIST", "CPD2");
        do_sc_cmd_Tx_func("CIST", "CPD3");
        init_k_spi_proc();
    }
    // rex+ test, 181106
    //if(c_mode == CAMERA_MODE_AEB)   c_mode = CAMERA_MODE_NIGHT_HDR;   // 用 3:HDR 來測試 7:Night+WDR
    //if(c_mode == CAMERA_MODE_RAW)   c_mode = CAMERA_MODE_NIGHT;       // 用 4:RAW 來測試 6:Night
    //if(c_mode == CAMERA_MODE_NIGHT) c_mode = CAMERA_MODE_M_MODE;      // 用 6:Night 來測試 12:M-Mode
    if(get_Removal_Debug_Img() == 1 && c_mode == CAMERA_MODE_AEB) c_mode = CAMERA_MODE_REMOVAL;        // 用 3:HDR 來測試 13:Removal

    Get_M_Mode(&M_Mode, &S_Mode);

    int fps = getFPS();
    int EP_Time = 10000000 / fps;
    set_A2K_Live_CMD(c_mode, t_mode, M_Mode, S_Mode);   // step1. mode需要先設定

//tmp    int type = GetFPGAEncodeType();
	int type = 0;
    do_A2K_Live_CMD(type);                  // step1. add pipeline command
    //if(Save_Jpeg_Now_Cnt == Save_Jpeg_End_Cnt) Capture_Long_EP = 0;
    //if(Capture_Long_EP == 0) do_A2K_Sensor_CMD();       // 長曝時，不要調整sensor
    do_A2K_Sensor_CMD(); 

    VinInterrupt(c_mode);                               // step2. change exp or gain, 解拍照時Exp/Gain即時調整誤動作
    
    set_A2K_Debug_Mode(DebugJPEGMode, DebugJPEGaddr, ISP2_Sensor);
    set_A2K_Sensor_CMD(sync_mode, EP_Time, fps, hdrLevel);

    if(sync_mode == 3) {		//change mode
        if(c_mode == CAMERA_MODE_SPORT_WDR || c_mode == CAMERA_MODE_REC_WDR || c_mode == CAMERA_MODE_TIMELAPSE_WDR)
        	SetGammaMax(220);
        else
        	SetGammaMax(255);
        Set_Angle_3DModel_Init(1);
    }

    F_Pipe_Run();                                       // step3. run pipeline command

//tmp    get_current_usec(&lstTime);
}

// rex+ 180222
void reset_JPEG_Quality(int quality)
{
    //JPEG_Quality = quality;
    //JPEG_Quality_lst = 0;
    set_A2K_JPEG_Quality_lst(0);
}

// rex+ 180213
int get_DebugJPEGMode(void)
{
    return DebugJPEGMode;
}
// rex+ 180213
int get_ISP2_Sensor(void) {
    return ISP2_Sensor;
}

// rex+ 180213
void get_mVideo_WidthHeight(int *w, int *h)       // call from us360.c
{
    *w = mVideoWidth;
    *h = mVideoHeight;
}

/*int cardboard_v1_offset = 40;
int cardboard_h1_offset = 0;
int cardboard_h2_offset = 0;
// rex+ 180223
void get_cardboard_Resolution(int *w, int *h)
{
    *w = 640 * 2 + cardboard_h1_offset * 2 + cardboard_h2_offset;
    *h = 640 + cardboard_v1_offset * 2;
}*/

//Debug
void setSTCmdDebugMode(int value)
{
 Set_ST_S_Cmd_Debug_Flag(value);
}

int getSTCmdDebugMode(void)
{
	return Get_ST_S_Cmd_Debug_Flag();
}
