/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __FPGA_PIPE_H__
#define __FPGA_PIPE_H__

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#define log2(x) (log(x) * 1.44269504088896340736)
#define log2f(x) ((float)log2(x))

typedef struct FPGA_Engine_One_Struct_H {
  int  	No;          // Picture No
  int  	I_Page;
  int  	O_Page;
  int  	Time_Buf;    // unit uSec
  int   M_Mode;
  int   rsv;         // Sub_Code;
} FPGA_Engine_One_Struct;

#define FPGA_ENGINE_P_MAX        256 /*256*/
struct FPGA_Engine_Resource_Struct {
  int                               Idx;
  int                               Page[2];
  FPGA_Engine_One_Struct            P[FPGA_ENGINE_P_MAX];       // 24(B) x 128 = 3072(B)
};                                                              // 3072 + 12 = 3084

struct F_Pipe_Struct {
  int                         Pic_No;
  int                         HW_Idx_B64;
  int                         Cmd_Idx;      // rex+ 180425, Big_D_Cnt;
  int                         Small_D_Cnt;
  int                         Pipe_Time_Base;
  int                         FPGA_Queue_Max;
  int                         Jpeg_Id;

  struct FPGA_Engine_Resource_Struct  Sensor;       // 3084 x 7 = 21588(B)
  struct FPGA_Engine_Resource_Struct  ISP1;
  struct FPGA_Engine_Resource_Struct  ISP2;
  struct FPGA_Engine_Resource_Struct  Stitch;
  struct FPGA_Engine_Resource_Struct  Smooth;
  struct FPGA_Engine_Resource_Struct  Jpeg[2];
  struct FPGA_Engine_Resource_Struct  H264;
  struct FPGA_Engine_Resource_Struct  DMA;
  struct FPGA_Engine_Resource_Struct  USB;
};
extern struct F_Pipe_Struct  F_Pipe;                // 21612(B)

// 0: don't care  1: Big  2: Small 3: Eeternal P
struct F_Command_One_Struct {
  int                         R_Id;  // Resource ID
  int                         I_Mode;
  int                         O_Mode;
};

typedef struct F_Com_In_Struct_H {
  int                         Checkcode;                  // 0x55aa55aa
  int                         Run_ID;
  int                         Record_Mode;                // 0:Capture 1:Record 2:Timelapse 3:HDR 4:RAW 5:WDR 6:Night 7:NightWDR
  int                         Capture_Resolution;         // 0:12K 1:8K 2:6K 3:4K 4:3K 5:2K
  int                         Timelapse_Resolution;       // 0:12K 1:8K 2:6K 3:4K 4:3K 5:2K
  int                         Record_Resolution;          // 0:12K 1:8K 2:6K 3:4K 4:3K 5:2K

  int                         Capture_D_Cnt;
  int                         Big_Mode;              // 0:12K 1:8K 2:6K 3:4K 4:3K 5:2K
  int                         Small_Mode;            // 0:12K 1:8K 2:6K 3:4K 4:3K 5:2K
  int                         rsv1;     //Big_Div;        // 大圖幾張取一張 ( <= 1 每張都取)
  int                         rsv2;     //Small_Div;      // 小圖幾張取一張 ( <= 1 每張都取)
  int                         Shuter_Speed;               // 曝光時間(us)
  int                         Pipe_Time_Base;             // 單位:us, pipeline基礎1份的時間
  int                         Reset_Sensor;
  int                         Sensor_Change_Binn;	  // 0:non 1~3:分次送完306個REG 11:做Sync調整
  int                         do_Rec_Thm;                 // 0:non 1: 錄影開始 pipe 安排壓縮圖的cmd
  int						  encode_type;			 // 0:JPEG 1:H264

  unsigned long long		  Capture_T1;				// SetCapEn() ~ Get Big Img
  unsigned long long		  Capture_T2;				// Get Big Img ~ Get Small Img
  int 						  Capture_Get_Img[2];		// [0]:Big Img	[1]:Small Img
  int 						  Capture_Step;				// 0:none 1:SetCmd 2:WaitImg 3:GetImg
  int 						  Capture_Lose_Flag;		// 0:none 1:lose big 2:lose small 3:lose big+small
  int						  Capture_Lose_Page[2];		// 0:Big Page 1:Small Page
  int						  Capture_USB_Idx[2];		// [0]:Big USB Idx	[1]:Small USB Idx
  int						  Capture_Img_Err_Cnt;		// 防止影像資料錯誤, 導致狀態卡死, 10次錯誤則跳出重送機制

  int						  Smooth_En;			 	// 0:Off 1:On
} F_Com_In_Struct;
extern F_Com_In_Struct   F_Com_In;

struct F_Com_In_Buf_Struct {
  int                         Run_ID;
  int                         Pipe_Init;                // 0xaaaa5555
  int                         Capture_D_Cnt;
  int                         Reset_Sensor;
  int 						  Sensor_Change_Binn;	 // 0:non 1~3:分次送完306個REG 11:做Sync調整
};
extern struct F_Com_In_Buf_Struct   F_Com_In_Buf;


#define F_SUBDATA_MAX           FPGA_ENGINE_P_MAX           /*256*/
// Sensor Command Sub Data
typedef struct F_Temp_Sensor_SubData_H
{
    int             HDR_7Idx;
    int             EP_LONG_T;      // 長時間曝光時使用
    int             Small_F;
    int             Frame;
    int             Integ;
    int             DGain;
} F_Temp_Sensor_SubData;
extern F_Temp_Sensor_SubData F_Sensor_SubData[F_SUBDATA_MAX];
void set_F_Temp_Sensor_SubData(int idx, int hdr_7idx, int ep_long_t, int small_f, int frame, int integ, int dgain);
void get_F_Temp_Sensor_SubData(int idx, int *hdr_7idx, int *ep_long_t, int *small_f, int *frame, int *integ, int *dgain);
// ISP1 Command Sub Data
typedef struct F_Temp_ISP1_SubData_H
{
    int             HDR_7Idx;
    int             HDR_fCnt;
    int             Now_Mode;
    int             WDR_Live_Idx;
    int             WDR_Live_Page;
    int				LC_En;					//做壞點時需要關閉Sensor亮度補償
} F_Temp_ISP1_SubData;
extern F_Temp_ISP1_SubData F_ISP1_SubData[F_SUBDATA_MAX];
void set_F_Temp_ISP1_SubData(int idx, int hdr_7idx, int hdr_fcnt, int now_mode, int wdr_live_idx, int wdr_live_page, int lc_en);
void get_F_Temp_ISP1_SubData(int idx, int *hdr_7idx, int *hdr_fcnt, int *now_mode, int *wdr_live_idx, int *wdr_live_page, int *lc_en);
// WDR Command Sub Data
typedef struct F_Temp_WDR_SubData_H
{
    int             HDR_7Idx;                   // 0-7, DDR Index
    int             WDR_Index;                  // 0-2, HDR Table
    int             WDR_Mode;				    // 1:ISP1.WDR(Live) 2:ISP2.WDR, 3-5-7:HDR7P
} F_Temp_WDR_SubData;
extern F_Temp_WDR_SubData F_WDR_SubData[F_SUBDATA_MAX];
void set_F_Temp_WDR_SubData(int idx, int hdr_7idx, int wdr_idx, int wdr_mode);
void get_F_Temp_WDR_SubData(int idx, int *hdr_7idx, int *wdr_idx, int *wdr_mode);
// ISP2 Command Sub Data
typedef struct F_Temp_ISP2_SubData_H
{
    int             ISP2_NR3D_Rate;                 // Make_ISP2_Cmd: ISP2_NR3D_Rate
    int             ISP2_HDR_7Idx;                  // rex+ 190325
    int             ISP2_HDR_fCnt;                  // 1:AEB, 3-5-7:HDR7P
    int             ISP2_Removal_St;                // Removal Step(0-3)
} F_Temp_ISP2_SubData;
extern F_Temp_ISP2_SubData F_ISP2_SubData[F_SUBDATA_MAX];
void set_F_Temp_ISP2_SubData(int idx, int nr3d, int hdr_7idx, int hdr_fcnt, int removal_st);
void get_F_Temp_ISP2_SubData(int idx, int *nr3d, int *hdr_7idx, int *hdr_fcnt, int *removal_st);
// Stitch Command Sub Data
typedef struct F_Temp_Stitch_SubData_H
{
    int             YUVZ_EN;
    int             DDR_5S_CNT;
    int             ST_Step;            // -1:all, 1:yuvz, 2:map
    int             YUV_DDR;            // 0,-1:normal, 1~3:offset
    int             I_Page;             // 0:ISP2-0, 1:ISP2-1, -1:F_Pipe_p->I_Page
} F_Temp_Stitch_SubData;
extern F_Temp_Stitch_SubData F_Stitch_SubData[F_SUBDATA_MAX];
void set_F_Temp_Stitch_SubData(int idx, int yuvz_st, int ddr_5s, int st_step, int yuv_ddr, int i_page);
void get_F_Temp_Stitch_SubData(int idx, int *yuvz_st, int *ddr_5s, int *st_step, int *yuv_ddr, int *i_page);
// Smooth Command Sub Data
typedef struct F_Temp_Smooth_SubData_H
{
    int             En;
} F_Temp_Smooth_SubData;
extern F_Temp_Smooth_SubData F_Smooth_SubData[F_SUBDATA_MAX];
void set_F_Temp_Smooth_SubData(int idx, int en);
void get_F_Temp_Smooth_SubData(int idx, int *en);
// Jpeg Command Sub Data
typedef struct F_Temp_Jpeg_SubData_H
{
    int             CAP_THM;
    int             DDR_5S_CNT;
    int             HDR_3S_CNT;
    int				Header_Page;
    int				Frame;
    int				Integ;
    int				DGain;
} F_Temp_Jpeg_SubData;
extern F_Temp_Jpeg_SubData F_Jpeg_SubData[F_SUBDATA_MAX][2];
void set_F_Temp_Jpeg_SubData(int idx, int eng, int cap_thm, int ddr_5s, int hdr_3s, int h_page, int frame, int integ, int dgain);
void get_F_Temp_Jpeg_SubData(int idx, int eng, int *cap_thm, int *ddr_5s, int *hdr_3s, int *h_page, int *frame, int *integ, int *dgain);
// H264 Command Sub Data
typedef struct F_Temp_H264_SubData_H
{
    int             do_H264;
    int				IP_Mode;
    int 			Frame_Stamp;
    int				Frame_Cnt;
} F_Temp_H264_SubData;
extern F_Temp_H264_SubData F_H264_SubData[F_SUBDATA_MAX];
void set_F_Temp_H264_SubData(int idx, int do_h264, int ip_mode, int fs, int fc);
void get_F_Temp_H264_SubData(int idx, int *do_h264, int *ip_mode, int *fs, int *fc);
// DMA Command Sub Data
typedef struct F_Temp_DMA_SubData_H
{
    int             Btm_Mode;
    int             Btm_Size;
    int             CP_Mode;
    int				BtmText_Mode;
    int 			Btm_Idx;
} F_Temp_DMA_SubData;
extern F_Temp_DMA_SubData F_DMA_SubData[F_SUBDATA_MAX];
void set_F_Temp_DMA_SubData(int idx, int btm_mode, int btm_size, int cp_mode, int btm_t_m, int btm_idx);
void get_F_Temp_DMA_SubData(int idx, int *btm_mode, int *btm_size, int *cp_mode, int *btm_t_m, int *btm_idx);
// USB Command Sub Data
typedef struct F_Temp_USB_SubData_H
{
    int             Encode_Type;
    int				USB_En;
} F_Temp_USB_SubData;
extern F_Temp_USB_SubData F_USB_SubData[F_SUBDATA_MAX];
void set_F_Temp_USB_SubData(int idx, int enc_type, int usb_en);
void get_F_Temp_USB_SubData(int idx, int *enc_type, int *usb_en);

//計算精準的成像時間
#define JPEG_HEADER_PAGE_MAX 	16
typedef struct F_Temp_JPEG_Header_Time_H
{
	//unsigned             	Now_Idx;		// FPGA目前Idx
    unsigned 				ISP1_Idx;		// 預計ISP1的Idx
    //unsigned long long		Now_Time;		// 讀取FPGA Idx時的系統時間
    unsigned long long 		ISP1_Time;		// 成像時間, Now_Time + (ISP1_Idx - Now_Idx) * F2_Cnt
} F_Temp_JPEG_Header_Time;

typedef struct F_Temp_JPEG_Header_H
{
	unsigned Init_Flag;
	unsigned Start_Idx;
	unsigned HW_Lst_Idx;
	unsigned ISP1_Lst_Idx;
	unsigned long long HW_Cnt;
	unsigned long long ISP1_Cnt;
	unsigned long long F2_Cnt;
	unsigned long long Now_Time;
	unsigned Page;
	F_Temp_JPEG_Header_Time Time[JPEG_HEADER_PAGE_MAX];
} F_Temp_JPEG_Header;
extern F_Temp_JPEG_Header F_JPEG_Header;

typedef struct HDR7P_Auto_Parameter_Struct_h {
	int Step;								// 0:none  1:Read Live(+0)  3.4:Add Small(-5) 5:Add Small(Live)
	int R_En;								// 確認是否可讀取Sensor統計值
	int ISP1_Idx[2];						// [0]:+0 ISP1 Idx  [1]:-5 ISP1 Idx
	float EV_Inc_T[2];						// 做Auto時2張的EV, +0, -5,	目標亮度
	float Rate[2];							// 運算後2張的倍率
	float EV_Inc[2];						// 運算後2張的EV_Inc
	unsigned Sensor_Y_Total[2];				// [0]:+0	[1]:-5, 0~255總合
	unsigned short Sensor_Y[5][256];		// 5顆sensor統計
	unsigned short Sensor_Y_Sum[2][256];	// [0]:+0  [1]:-5,  5顆sensor總合
	unsigned long long R_Time;
	int Shuter_Time;
}HDR7P_Auto_Parameter_Struct;
extern HDR7P_Auto_Parameter_Struct HDR7P_Auto_P;

typedef struct HDR7P_AEG_Parameter_Struct_h {
	int aeg_idx;
	int frame;
	int integ;
	int gain;
	int ep_line;
}HDR7P_AEG_Parameter_Struct;
extern HDR7P_AEG_Parameter_Struct HDR7P_AEG_Parameter[7];

void FPGA_Com_In_Sensor_Reset(int EP_Time, int FPS);
void FPGA_Com_In_Change_Mode(int EP_Time, int FPS);
int FPGA_Com_In_Sensor_Adjust(int EP_Time, int FPS);
void FPGA_Pipe_Init(int force);
void F_Pipe_Add_Small(int Img_Mode, int C_Mode, int hdr_step, int *isp1_idx, int *shuter_t, int *ts_max);
int F_Pipe_Add_Big(int Img_Mode, int *Finish_Idx, int *Sensor_Idx, int C_Mode);
int get_HW_Idx_B64(void);
void F_Pipe_Run(void);

void FPGA_Com_In_Set_CAP(int Resolution, int EP_Time, int FPS, int enc_type, int C_Mode);
void FPGA_Com_In_Set_Record(int Resolution, int EP_Time, int FPS, int *doThm, int enc_type, int C_Mode);
void FPGA_Com_In_Set_Timelapse(int Resolution, int EP_Time, int FPS, int enc_type, int C_Mode);
//void FPGA_Com_In_Set_HDR(int Resolution, int EP_Time, int FPS, int enc_type);
void FPGA_Com_In_Set_RAW(int Resolution, int EP_Time, int FPS, int enc_type);
void FPGA_Com_In_Set_WDR(int Resolution, int EP_Time, int FPS, int enc_type);     // rex+ 180731
int FPGA_Com_In_Add_Capture(int cnt);
void FPGA_Pipe_ReStart(void);
int check_F_Com_In(void);
int get_USB_Spend_Time(int mode);

float get_HDR_Auto_Ev(int idx);
HDR7P_AEG_Parameter_Struct Get_HDR7P_AEG_Idx(int idx);
int read_F_Com_In_Capture_D_Cnt(void);

//---------------------------------
void setting_AEB(int frame_cnt,int ae_scale);
void SetAntiAliasingEn(int en);
void SetRemovalAntiAliasingEn(int en);
void SetPipeReStart();
int getCapturePrepareTime();
void setting_HDR7P(int manual, int frame_cnt, int ae_scale, int strength);
void setting_Removal_HDR(int manual, int ae_scale, int strength);
void SetHDR7PAutoTh(int idx, int value);
void SetHDR7PAutoTarget(int idx, int value);
void SetRemovalVariable(int sel, int val);
int getCaptureAddTime();
int getCaptureEpTime();

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__FPGA_PIPE_H__



