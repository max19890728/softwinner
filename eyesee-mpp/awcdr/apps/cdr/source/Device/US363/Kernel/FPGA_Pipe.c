/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Kernel/FPGA_Pipe.h"

#ifndef ANDROID_CODE
  #include <string.h>
#endif

#include <stdio.h>
#include <sys/stat.h>

//#include "Device/US363/Cmd/us360_func.h"
//#include "Device/US363/Cmd/Smooth.h"
#include "Device/US363/Kernel/k_spi_cmd.h"
#include "Device/US363/Kernel/variable.h"
#include "Device/US363/Kernel/us360_define.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "FPGA::Pipe"


F_Com_In_Struct                 F_Com_In;
struct F_Com_In_Buf_Struct      F_Com_In_Buf;
struct F_Pipe_Struct            F_Pipe;
struct F_Pipe_Struct            F_Temp;

F_Temp_JPEG_Header F_JPEG_Header;
void Set_F_JPEG_Header_Now(int now_idx)
{
	F_JPEG_Header.Start_Idx    = now_idx;
	F_JPEG_Header.HW_Lst_Idx   = now_idx;
	F_JPEG_Header.ISP1_Lst_Idx = now_idx;
	F_JPEG_Header.HW_Cnt       = 0;
	F_JPEG_Header.ISP1_Cnt     = 0;
}

void Set_F_JPEG_Header_ISP1(int isp1_idx, int *jtbl_idx)
{
    unsigned page = F_JPEG_Header.Page;
    unsigned long long f2_cnt = F_JPEG_Header.F2_Cnt;
    F_JPEG_Header.ISP1_Cnt += ( (isp1_idx - F_JPEG_Header.ISP1_Lst_Idx) & 0x7F);
    if(F_JPEG_Header.ISP1_Cnt > 0xFFFFFFFF) F_JPEG_Header.ISP1_Cnt = 0;
    F_JPEG_Header.Time[page].ISP1_Time = F_JPEG_Header.ISP1_Cnt * f2_cnt / 100000;		//ms
    //db_debug("jpeg_header ISP:%d,%d,%lld\n",isp1_idx,F_JPEG_Header.ISP1_Lst_Idx,F_JPEG_Header.Time[page].ISP1_Time);
    F_JPEG_Header.ISP1_Lst_Idx = isp1_idx;
    *jtbl_idx = page;

    F_JPEG_Header.Page++;
    if(F_JPEG_Header.Page >= JPEG_HEADER_PAGE_MAX)
        F_JPEG_Header.Page = 0;
}

void Set_F_JPEG_Header_Now_Time(int now_idx)
{
    unsigned long long f2_cnt = F_JPEG_Header.F2_Cnt;
    F_JPEG_Header.HW_Cnt += ( (now_idx - F_JPEG_Header.HW_Lst_Idx) & 0x7F);
    if(F_JPEG_Header.HW_Cnt > 0xFFFFFFFF) F_JPEG_Header.HW_Cnt = 0;
    F_JPEG_Header.Now_Time = F_JPEG_Header.HW_Cnt * f2_cnt / 100000;		//ms
    //db_debug("jpeg_header:%d,%d,%lld\n",now_idx,F_JPEG_Header.HW_Lst_Idx,F_JPEG_Header.Now_Time);
    F_JPEG_Header.HW_Lst_Idx = now_idx;

}

void Get_F_JPEG_Header_Now_Time(unsigned long long* time) {
	*time = F_JPEG_Header.Now_Time;
}

int readSyncIdx(void)
{
    static int lst_idx=0;
    int i, idx = lst_idx, addr = 0;
    unsigned i1, i2;

#ifdef ANDROID_CODE
    addr = SYNC_IDX_IO_ADDR;             // SYNC_IDX_IO_ADDR   0x60040
    for(i = 0; i < 3; i ++){
    	spi_read_io_porcess_S2(addr, &i1, 4);
        spi_read_io_porcess_S2(addr, &i2, 4);
        if(i1 >= M_CMD_PAGE_N || i1 < 0 ||
           i2 >= M_CMD_PAGE_N || i2 < 0)
        {
            db_error("readSyncIdx: err! i1=%d i2=%d\n", i1, i2);
        }
        if(i1 == i2 && i1 < M_CMD_PAGE_N){
            idx = i1;
            break;
        }
    }
    if(idx >= M_CMD_PAGE_N) db_error("readSyncIdx: idx=%d err!\n", idx);
#else
    static int SyncIdx=0;
    idx = SyncIdx;
    SyncIdx = (SyncIdx+1) & (M_CMD_PAGE_N-1);
#endif
    lst_idx = idx;
    return(idx & (M_CMD_PAGE_N-1));             // M_CMD_PAGE_N    64
}
void FPGA_Com_In_Set(int EP_Time, int FPS)
{
  F_Com_In.Checkcode    = 0;
  F_Com_In.Run_ID++;
  F_Com_In.Shuter_Speed   = EP_Time;

  F_Com_In.Pipe_Time_Base = 1000000 / FPS;              // ISP1使用時間
  switch (F_Com_In.Record_Mode) {
      case 0:   // Cap (1p)
      case 3:   // HDR (3p)
      case 4:   // RAW (5p)
      case 5:   // WDR (1p)
      case 6:   // Night
      case 7:   // Night + WDR
      case 8:   // Sport
      case 9:   // Sport + WDR
      case 12:  // M-Mode
      case 13:  // Removal
      case 14:  // 3D-Model
			F_Com_In.Big_Mode      = F_Com_In.Capture_Resolution;
			switch (F_Com_In.Capture_Resolution) {
				case 0:   F_Com_In.Small_Mode    = 3; break;	//12K
				case 1:   F_Com_In.Small_Mode    = 3; break;	//8K
				case 2:   F_Com_In.Small_Mode    = 3; break;	//6K
				default : F_Com_In.Small_Mode    = 3; break;
			}
			break;
      case 1:   // Record
      case 10:  // Record + WDR
			F_Com_In.Big_Mode      = 0;
			F_Com_In.Small_Mode    = F_Com_In.Record_Resolution;
			break;
      case 2:   // Timelapse
      case 11:  // Timelapse + WDR
			F_Com_In.Big_Mode      = F_Com_In.Timelapse_Resolution;
			switch (F_Com_In.Timelapse_Resolution) {
				case 0:   F_Com_In.Small_Mode    = 3; break;
				case 1:   F_Com_In.Small_Mode    = 4; break;
				case 2:   F_Com_In.Small_Mode    = 4; break;
				case 3:   F_Com_In.Small_Mode    = 3; break;
				default : F_Com_In.Small_Mode    = 3; break;
			}
			break;
  }
  F_Com_In.Checkcode     = 0x55aa55aa;
};

int Cap_Smooth_En = 0;		// (AntiAliasing_En)
int Removal_Cap_Smooth_En = 0;
void SetAntiAliasingEn(int en) {
	Cap_Smooth_En = (en & 0x1);
}
void SetRemovalAntiAliasingEn(int en) {
	Removal_Cap_Smooth_En = (en & 0x1);
}

void FPGA_Com_In_Set_CAP(int Resolution, int EP_Time, int FPS, int enc_type, int C_Mode)
{
  F_Com_In.Record_Mode = C_Mode;        // C_Mode=0,3,5,6,8,9,12,13
  F_Com_In.Capture_Resolution  = Resolution;
  F_Com_In.encode_type   = enc_type;
  if(C_Mode == 3)       F_Com_In.Smooth_En = 0;								//AEB
  else if(C_Mode == 13) F_Com_In.Smooth_En = Removal_Cap_Smooth_En;			//Removal
  else			        F_Com_In.Smooth_En = Cap_Smooth_En;
  set_A2K_Smooth_En(F_Com_In.Smooth_En);
  FPGA_Com_In_Set(EP_Time, FPS);
  FPGA_Pipe_Init(0);
};
void FPGA_Com_In_Set_Record(int Resolution, int EP_Time, int FPS, int *doThm, int enc_type, int C_Mode)
{
  F_Com_In.Record_Mode = C_Mode;        // Record
  F_Com_In.Record_Resolution   = Resolution;
  F_Com_In.encode_type   = enc_type;
  F_Com_In.Smooth_En   	= 0;
  set_A2K_Smooth_En(F_Com_In.Smooth_En);
  if(*doThm == 1) {
	  F_Com_In.do_Rec_Thm = 1;
	  *doThm = 0;
  }
  FPGA_Com_In_Set(EP_Time, FPS);
  FPGA_Pipe_Init(0);
};
void FPGA_Com_In_Set_Timelapse(int Resolution, int EP_Time, int FPS, int enc_type, int C_Mode)
{
  F_Com_In.Record_Mode = C_Mode;        // Timelapse
  F_Com_In.Timelapse_Resolution = Resolution;
  F_Com_In.encode_type   = enc_type;
  F_Com_In.Smooth_En   	= 0;
  set_A2K_Smooth_En(F_Com_In.Smooth_En);
  FPGA_Com_In_Set(EP_Time, FPS);
  FPGA_Pipe_Init(0);
};
void FPGA_Com_In_Set_RAW(int Resolution, int EP_Time, int FPS, int enc_type)
{
  F_Com_In.Record_Mode = 4;             // Capture_RAW
  F_Com_In.Capture_Resolution = Resolution;
  F_Com_In.encode_type   = enc_type;
  F_Com_In.Smooth_En   	= 0;
  set_A2K_Smooth_En(F_Com_In.Smooth_En);
  FPGA_Com_In_Set(EP_Time, FPS);
  FPGA_Pipe_Init(0);
};

int read_F_Com_In_Capture_D_Cnt(void)
{
    return F_Com_In.Capture_D_Cnt;
}
/*
 * return 1: ok
 *       -1: fail
 */
int FPGA_Com_In_Add_Capture(int cnt)
{
  int c_mode = F_Com_In.Record_Mode;
//tmp  int picture = get_C_Mode_Picture(c_mode);
  int picture = 1;
  if (picture == 1) {
     if(F_Com_In.Capture_D_Cnt == 0){
    	HDR7P_Auto_Init(c_mode);
	    F_Com_In.Capture_D_Cnt = cnt;
	    F_Com_In.Run_ID++;
        SetWaveDebug(4, F_Com_In.Capture_D_Cnt);
        return 1;   // ok
     }
  }
  db_debug("FPGA_Com_In_Add_Capture: err! cnt=%d mode=%d D=%d\n", cnt, F_Com_In.Record_Mode, F_Com_In.Capture_D_Cnt);
  return -1;    // fail
};

int Get_FPGA_Com_In_Sensor_Change_Binn(void) {
	return F_Com_In.Sensor_Change_Binn;
}

// for test capture 12K
void FPGA_Com_In_Sensor_Reset(int EP_Time, int FPS)
{
  //db_debug("FPGA_Com_In_Sensor_Reset: Change_Binn=%d EP_Time=% FPS=%d\n", F_Com_In.Sensor_Change_Binn, EP_Time, FPS);
  if(F_Com_In.Sensor_Change_Binn == 0) {
	  F_Com_In.Reset_Sensor = 1;
	  F_Com_In.Sensor_Change_Binn = 3;
	  FPGA_Com_In_Set(EP_Time, FPS);
  }
};

void FPGA_Com_In_Change_Mode(int EP_Time, int FPS)
{
  F_Com_In.Sensor_Change_Binn = 3;
  FPGA_Com_In_Set(EP_Time, FPS);
};

int FPGA_Com_In_Sensor_Adjust(int EP_Time, int FPS)
{
  if(F_Com_In.Sensor_Change_Binn == 0){
    F_Com_In.Sensor_Change_Binn = 11;		//adj sensor sync
    FPGA_Com_In_Set(EP_Time, FPS);
    return 0;     // ok
  }
  else{
    db_debug("FPGA_Com_In_Sensor_Adjust: binn=%d\n", F_Com_In.Sensor_Change_Binn);
  }
  return -1;     // warning
};


// =======================================================================
/*
 * 解重新Download Pipe Idx 未歸0
 */
int Cap_Finish_Idx = -1;        // 紀錄capture是否執行完畢
int Cap_Sensor_Idx = -1;
void SetPipeReStart()
{
	F_Com_In_Buf.Pipe_Init = 0;
	memset(&F_Pipe, 0, sizeof(F_Pipe) );
	memset(&F_Temp, 0, sizeof(F_Temp) );
	memset(&F_Com_In, 0, sizeof(F_Com_In) );
	memset(&F_JPEG_Header, 0, sizeof(F_JPEG_Header) );
	Cap_Finish_Idx = -1;
}
int rst_HW_Page_En=0;

void FPGA_Pipe_Init(int force)
{
    int i; int s_idx;
    int c_mode = F_Com_In.Record_Mode;
//tmp  int picture = get_C_Mode_Picture(c_mode);
    int picture = 1;

    if (F_Pipe.Pipe_Time_Base != F_Com_In.Pipe_Time_Base || force == 1){
        memset(&F_Pipe, 0, sizeof(F_Pipe) );
	    memset(&F_Temp, 0, sizeof(F_Temp) );                                // rex+ 190402
        memset(&F_JPEG_Header, 0, sizeof(F_JPEG_Header) );
        F_Pipe.Pipe_Time_Base = F_Com_In.Pipe_Time_Base;
        F_Pipe.Pic_No      = 100;
        F_Pipe.Small_D_Cnt = 0;

        for (i = 0; i < FPGA_ENGINE_P_MAX; i++) {
            F_Pipe.Sensor.P[i].Time_Buf = -1;
            F_Pipe.ISP1.P[i].Time_Buf   = -1;
            F_Pipe.ISP2.P[i].Time_Buf   = -1;
            F_Pipe.Stitch.P[i].Time_Buf = -1;
            F_Pipe.Jpeg[0].P[i].Time_Buf   = -1;
            F_Pipe.Jpeg[1].P[i].Time_Buf   = -1;
            F_Pipe.DMA.P[i].Time_Buf    = -1;
            F_Pipe.USB.P[i].Time_Buf    = -1;
        }

        rst_HW_Page_En = 1;
        F_Pipe.HW_Idx_B64 = readSyncIdx();
        s_idx = (F_Pipe.HW_Idx_B64 + 3) & 0x3f;
    	if(F_JPEG_Header.Init_Flag == 0) {
    		F_JPEG_Header.Init_Flag = 1;
    		Set_F_JPEG_Header_Now(s_idx);
    	}
        db_debug("FPGA_Pipe_Init: s_idx=%d\n", s_idx);

        F_Pipe.Cmd_Idx    = s_idx;                  // rex+ 180425, Write Command Index
        F_Pipe.Sensor.Idx = s_idx;
        F_Pipe.ISP1.Idx   = s_idx;
        F_Pipe.ISP2.Idx   = s_idx;
        F_Pipe.Stitch.Idx = s_idx;
        F_Pipe.Jpeg[0].Idx   = s_idx;
        F_Pipe.Jpeg[1].Idx   = s_idx;
        F_Pipe.DMA.Idx    = s_idx;
        F_Pipe.USB.Idx    = s_idx;

        F_Pipe.Smooth.Page[0] = 1;		//反鋸齒, 與縫合共用記憶體, Smooth寫在縫合的另一塊記憶體
        F_Pipe.Smooth.Page[1] = 1;

        memcpy(&F_Temp, &F_Pipe, sizeof(F_Temp));

        F_Com_In.Capture_D_Cnt = 0;
        if(HDR7P_Auto_P.Step != 0) HDR7P_Auto_P.Step = 0;			//解拍照HDR Auto狀態卡死問題

    	F_Com_In.Capture_T1 = 0;
    	F_Com_In.Capture_T2 = 0;
    	F_Com_In.Capture_Step = 0;
    	F_Com_In.Capture_Lose_Flag = 0;
    	F_Com_In.Capture_Img_Err_Cnt = 0;
    	for(i = 0; i < 2; i++) {
    		F_Com_In.Capture_Get_Img[i] = 0;
    		F_Com_In.Capture_Lose_Page[i] = 0;
    	}
    }

    //F_Pipe.FPGA_Queue_Max = 900000 / F_Pipe.Pipe_Time_Base;
    //0:Cap 3:HDR 4:RAW 5:WDR 6:Night 7:NightWDR 8:Sport 9:SportWDR
    if (picture == 1)
        F_Pipe.FPGA_Queue_Max = 400000 / F_Pipe.Pipe_Time_Base;
    else
        F_Pipe.FPGA_Queue_Max = 900000 / F_Pipe.Pipe_Time_Base;
    if(F_Pipe.FPGA_Queue_Max < 4) F_Pipe.FPGA_Queue_Max = 4;        // 解長時間曝光, 預下太少道CMD, FPGA停下問題
};

int Do_Pipe_Init = 0;
int F_Pipe_Add_Sub(int Img_Mode,int New_Job, int P0_Id, int P1_Id, int P2_Id, int Timer, int *Sub_Idx, int *T)
{
    int i;
    int P01, P02;
    int Check_Flag;
    int Times; static int tTotal, enDebug=0;
    int Len;
    int P0_SId, P1_SId, P2_SId;
    int B_S1,B_S2;
    struct FPGA_Engine_Resource_Struct *P0, *P1, *P2;

    if(Img_Mode < 0){     // only debug enable
        if(Img_Mode == -1) enDebug = 1;
        if(Img_Mode == -2) enDebug = 0;
        tTotal = 0;
        return 0;
    }

    P0_SId = P0_Id % 100;
    P1_SId = P1_Id % 100; B_S1 = P1_Id / 100;
    P2_SId = P2_Id % 100; B_S2 = P2_Id / 100;

    switch (P0_SId) {
         case 1: P0 = &F_Temp.Sensor; break;
         case 2: P0 = &F_Temp.ISP1;   break;
         case 3: P0 = &F_Temp.ISP2;   break;
         case 4: P0 = &F_Temp.Stitch; break;
         case 5: P0 = &F_Temp.Jpeg[F_Temp.Jpeg_Id];   break;
         case 6: P0 = &F_Temp.DMA;    break;
         case 7: P0 = &F_Temp.USB;    break;
         case 8: P0 = &F_Temp.H264;    break;
         case 9: P0 = &F_Temp.Smooth;  break;
         default: P0 = &F_Temp.USB;    break;
    };

    switch (P1_SId) {
         case 1: P1 = &F_Temp.Sensor; break;
         case 2: P1 = &F_Temp.ISP1;   break;
         case 3: P1 = &F_Temp.ISP2;   break;
         case 4: P1 = &F_Temp.Stitch; break;
         case 5: P1 = &F_Temp.Jpeg[F_Temp.Jpeg_Id];   break;
         case 6: P1 = &F_Temp.DMA;    break;
         case 7: P1 = &F_Temp.USB;    break;
         case 8: P1 = &F_Temp.H264;    break;
         case 9: P1 = &F_Temp.Smooth;  break;
         default: P1 = &F_Temp.USB;    break;
    };

    switch (P2_SId) {
         case 1: P2 = &F_Temp.Sensor; break;
         case 2: P2 = &F_Temp.ISP1;   break;
         case 3: P2 = &F_Temp.ISP2;   break;
         case 4: P2 = &F_Temp.Stitch; break;
         case 5: P2 = &F_Temp.Jpeg[F_Temp.Jpeg_Id];  break;
         case 6: P2 = &F_Temp.DMA;    break;
         case 7: P2 = &F_Temp.USB;    break;
         case 8: P2 = &F_Temp.H264;    break;
         case 9: P2 = &F_Temp.Smooth;  break;
         default: P2 = &F_Temp.USB;    break;
    };

    Times = (Timer + F_Pipe.Pipe_Time_Base - 10000) / F_Pipe.Pipe_Time_Base;
    tTotal += Times;

    int p_mask = (FPGA_ENGINE_P_MAX-1);             // (128-1)=0x7f
    int p_limit = FPGA_ENGINE_P_MAX-32; //(FPGA_ENGINE_P_MAX>>2)*3;         // 256/4*3=192

    Check_Flag = 1;
    Len = (P1->Idx - P0->Idx) & p_mask;             // FPGA_ENGINE_P_MAX=128
    if ((Len < 32) && (Len != 0)) {                 // rex+s 180528, 32->42, 解USB Times>=17,19 會誤動作
        P01 = P1->Idx;
        if (New_Job == 1){
            Check_Flag = 0;
            *Sub_Idx = -1;
        }
    }
    else {
        P01 = P0->Idx;
    }

    int Len2 = (P2->Idx - P01) & p_mask;
    if ((Len2 < 32) && (P2_SId < 10)){        //P2_SId >=10 不參照下一級
        P02 = P01;
        for (i = P01 + Len2 - 1; i >= P01; i--) {
            //P02 = i & p_mask;
            if(P1_SId == 5 && P2_SId == 7) {
            	if (P2->P[i & p_mask].I_Page == (P1->Page[B_S2] + F_Temp.Jpeg_Id * 2 + P1_Id * 10))
            		break;
            }
            else {
            	if (P2->P[i & p_mask].I_Page == (P1->Page[B_S2] + P1_Id * 10))
            		break;
            }
            P02 = i & p_mask;
        };
        P01 = P02;
    }
    int p_max = p_limit;                         // 224
    //if(B_S1 > 0 || B_S2 > 0){  p_max = FPGA_ENGINE_P_MAX>>2; }      // Add_Small() < 64
    if(B_S1 > 0 || B_S2 > 0){  p_max = 48; }      // Add_Small() < 48, 解拍攝Removal + 平滑化後, Pipe沒有重新Init, 導致後面小圖F_Pipe.Sensor.Idx與F_Pipe.Cmd_Idx差距過大(50)

    if(((P01 - F_Pipe.Cmd_Idx) & p_mask) > p_max){                  // HW_idx 超過 SW_idx
        static int err_cnt=0;
        //F_Pipe_Add_Sub: P01-Idx=65 Cmd_Idx=45 Len=251(105-110) P01=110 Len2=251 P02=-369590040 SId={5,7,9}
        db_debug("F_Pipe_Add_Sub: P01-Idx=%d Cmd_Idx=%d Len=%d(%d-%d) P01=%d Len2=%d P02=%d SId={%d,%d,%d}\n", 
            ((P01 - F_Pipe.Cmd_Idx) & p_mask), F_Pipe.Cmd_Idx, Len, P1->Idx, P0->Idx, P01, Len2, P02, P0_SId, P1_SId, P2_SId);
        //SetWaveDebug(3, (P01<<16) | (F_Pipe.Cmd_Idx<<8) | err_cnt);
        Do_Pipe_Init = 1;
        return(0);
    }

    if (Check_Flag == 1) {
          Len = (P01 - P1->Idx) & p_mask;
          if (Len < p_limit) {
              for (i = P1->Idx; i < P1->Idx+Len; i++) {
                 P1->P[i & p_mask].No       = -1;
                 P1->P[i & p_mask].Time_Buf = -1;
                 P1->P[i & p_mask].I_Page   = -1;
                 P1->P[i & p_mask].O_Page   = -1;
                 P1->P[i & p_mask].M_Mode   = Img_Mode;
                 //P1->P[i & p_mask].Sub_Code = -1;
              };
          }

          P1->P[P01].No       = F_Pipe.Pic_No;
          P1->P[P01].Time_Buf = Timer;
          if(P1_SId == 7 && P0_SId == 5)
              P1->P[P01].I_Page   = P0_Id * 10 + F_Temp.Jpeg_Id * 2 + P0->Page[B_S1];
          else
              P1->P[P01].I_Page   = P0_Id * 10 + P0->Page[B_S1];
          P1->P[P01].O_Page   = P1_Id * 10 + P1->Page[B_S1];
          P1->P[P01].M_Mode   = Img_Mode;
          //P1->P[P01].Sub_Code = sub_code;
          *Sub_Idx = P01;

          for (i = P01+1; i < P01+Times; i++) {
             P1->P[i & p_mask].No       = F_Pipe.Pic_No;
             P1->P[i & p_mask].Time_Buf = 0;
             if(P1_SId == 7 && P0_SId == 5)
                 P1->P[i & p_mask].I_Page   = P0_Id * 10 + F_Temp.Jpeg_Id * 2 + P0->Page[B_S1];
             else
                 P1->P[i & p_mask].I_Page   = P0_Id * 10 + P0->Page[B_S1];
             P1->P[i & p_mask].O_Page   = P1_Id * 10 + P1->Page[B_S1];
             P1->P[i & p_mask].M_Mode   = Img_Mode;
             //P1->P[i & p_mask].Sub_Code = sub_code;
          };
          P1->Idx = i & p_mask;
          *T = Times;
    }

    if(enDebug == 1 || *Sub_Idx == -1){
        char str[4];
        switch (P1_SId) {
         case 1: memcpy(str, "SEN\0", 4); break;
         case 2: memcpy(str, "IP1\0", 4); break;
         case 3: memcpy(str, "IP2\0", 4); break;
         case 4: memcpy(str, "STI\0", 4); break;
         case 5: if(F_Temp.Jpeg_Id == 0) memcpy(str, "JP0\0", 4);
        	 	 else					 memcpy(str, "JP1\0", 4);
        	 	 break;
         case 6: memcpy(str, "DMA\0", 4); break;
         case 7: memcpy(str, "USB\0", 4); break;
         case 8: memcpy(str, "264\0", 4); break;
         case 9: memcpy(str, "SMO\0", 4); break;
         default:memcpy(str, "ERR\0", 4); break;
        };
        db_debug("o: M=%d J=%d P=%02d-%02d-%02d (%s) Idx=%03d-%03d-%03d %03d={%d,%d} T=%d\n", Img_Mode, New_Job, 
            P0_Id, P1_Id, P2_Id, str, P0->Idx, P1->Idx, P2->Idx, *Sub_Idx, 
            P1->P[*Sub_Idx].I_Page, P1->P[*Sub_Idx].O_Page, Times);
    }
    return(Check_Flag);
}



void Make_F_Pipe(int Img_Mode, int Check_Flag, int C_Mode)
{
  int i,k;
  int P0_Idx;
  int Len=0;
  int Timer, Times;
  struct FPGA_Engine_Resource_Struct *P0, *P1, *P2;
  int p_mask = (FPGA_ENGINE_P_MAX-1);
  int p_limit = (FPGA_ENGINE_P_MAX>>2)*3;
  
  if(C_Mode != 13) Len = (F_Temp.USB.Idx - F_Temp.Sensor.Idx) & p_mask;
  if (Len > p_limit){                                       // 限制一批CMD最大96*100ms, p_limit=96
      Check_Flag = 0;
      db_error("Make_F_Pipe: err! USB.Idx=%d Sensor.Idx=%d\n", F_Temp.USB.Idx, F_Temp.Sensor.Idx);
  }
  int expT = 180;	//120;    // 最長4秒改12秒, 190329
  Len = (F_Temp.Sensor.Idx - F_Pipe.Sensor.Idx) & p_mask;   // 限制最大可下10張畫面的CMD, 10*100ms
  if (Len > (F_Pipe.FPGA_Queue_Max + expT)){                // FPGA_Queue_Max=4
      Check_Flag = 0;
      db_error("Make_F_Pipe: err! Len=%d expT=%d Max=%d F_Temp.S=%d F_Pipe.S=%d\n", Len, expT,
              F_Pipe.FPGA_Queue_Max, F_Temp.Sensor.Idx, F_Pipe.Sensor.Idx);
  }

  if (Check_Flag == 1) {
      memcpy( &F_Pipe,&F_Temp, sizeof(F_Pipe) );
  }
  else {
      P0_Idx = F_Pipe.Cmd_Idx;
      for (k = 1; k <= 10; k++) {
          switch (k) {
              case 1: P1 = &F_Pipe.Sensor;  break;
              case 2: P1 = &F_Pipe.ISP1;    break;
              case 3: P1 = &F_Pipe.ISP2;    break;
              case 4: P1 = &F_Pipe.Stitch;  break;
              case 5: P1 = &F_Pipe.Jpeg[0]; break;
              case 6: P1 = &F_Pipe.DMA;     break;
              case 7: P1 = &F_Pipe.USB;     break;
              case 8: P1 = &F_Pipe.Jpeg[1]; break;
              case 9: P1 = &F_Pipe.H264;   	break;
              case 10: P1 = &F_Pipe.Smooth; break;
          };

          Len = (P0_Idx - P1->Idx) & p_mask;
          if ( Len < (FPGA_ENGINE_P_MAX>>2)) {        // FPGA_ENGINE_P_MAX=128
              P1->P[P0_Idx].No       = -1;
              P1->P[P0_Idx].Time_Buf = -1;
              P1->P[P0_Idx].I_Page   = -1;
              P1->P[P0_Idx].O_Page   = -1;
              P1->P[P0_Idx].M_Mode   = Img_Mode;
              P1->Idx =  (P1->Idx + 1) & p_mask;
          }
      };
  }
};

/*
 * Sensor Command Sub Data
 */
F_Temp_Sensor_SubData F_Sensor_SubData[F_SUBDATA_MAX];
void set_F_Temp_Sensor_SubData(int idx, int hdr_7idx, int ep_long_t, int small_f, int frame, int integ, int dgain)
{
    if(idx >= F_SUBDATA_MAX || idx < 0) return;
    F_Sensor_SubData[idx].HDR_7Idx  = hdr_7idx;
    F_Sensor_SubData[idx].EP_LONG_T = ep_long_t;
    F_Sensor_SubData[idx].Small_F   = small_f;
    F_Sensor_SubData[idx].Frame     = frame;        // 設定曝光張數
    F_Sensor_SubData[idx].Integ     = integ;        // 設定曝光掃描線
    F_Sensor_SubData[idx].DGain     = dgain;
    SetWaveDebug(6, (small_f<<16) | (hdr_7idx<<8) | idx);
}
void get_F_Temp_Sensor_SubData(int idx, int *hdr_7idx, int *ep_long_t, int *small_f, int *frame, int *integ, int *dgain)
{
    if(idx >= F_SUBDATA_MAX || idx < 0) return;
    *hdr_7idx  = F_Sensor_SubData[idx].HDR_7Idx;
    *ep_long_t = F_Sensor_SubData[idx].EP_LONG_T;
    *small_f   = F_Sensor_SubData[idx].Small_F;
    *frame     = F_Sensor_SubData[idx].Frame;
    *integ     = F_Sensor_SubData[idx].Integ;
    *dgain     = F_Sensor_SubData[idx].DGain;
    SetWaveDebug(7, (*small_f<<16) | (*hdr_7idx<<8) | idx);
}
void cle_F_Temp_Sensor_SubData(int idx)
{
    if(idx >= F_SUBDATA_MAX || idx < 0) return;
    F_Sensor_SubData[idx].HDR_7Idx  = -1;
    F_Sensor_SubData[idx].EP_LONG_T = -1;
    F_Sensor_SubData[idx].Small_F   = -1;
    F_Sensor_SubData[idx].Frame     = -1;        // 設定曝光張數
    F_Sensor_SubData[idx].Integ     = -1;        // 設定曝光掃描線
    F_Sensor_SubData[idx].DGain     = -1;
}
void init_F_Temp_Sensor_SubData(void)
{
    int i;
    for(i = 0; i < F_SUBDATA_MAX; i++){
        F_Sensor_SubData[i].HDR_7Idx  = -1;
        F_Sensor_SubData[i].EP_LONG_T = -1;
        F_Sensor_SubData[i].Small_F   = -1;
        F_Sensor_SubData[i].Frame     = -1;        // 設定曝光張數
        F_Sensor_SubData[i].Integ     = -1;        // 設定曝光掃描線
        F_Sensor_SubData[i].DGain     = -1;
    }
}

/*
 * ISP1 Command Sub Data
 */
F_Temp_ISP1_SubData F_ISP1_SubData[F_SUBDATA_MAX];
void set_F_Temp_ISP1_SubData(int idx, int hdr_7idx, int hdr_fcnt, int now_mode, int wdr_live_idx, int wdr_live_page, int lc_en)
{
    if(idx >= F_SUBDATA_MAX || idx < 0) return;
    F_ISP1_SubData[idx].HDR_7Idx      = hdr_7idx;
    F_ISP1_SubData[idx].HDR_fCnt      = hdr_fcnt;
    F_ISP1_SubData[idx].Now_Mode      = now_mode;
    F_ISP1_SubData[idx].WDR_Live_Idx  = wdr_live_idx;
    F_ISP1_SubData[idx].WDR_Live_Page = wdr_live_page;
    F_ISP1_SubData[idx].LC_En 		  = lc_en;
}
void get_F_Temp_ISP1_SubData(int idx, int *hdr_7idx, int *hdr_fcnt, int *now_mode, int *wdr_live_idx, int *wdr_live_page, int *lc_en)
{
    if(idx >= F_SUBDATA_MAX || idx < 0) return;
    *hdr_7idx      = F_ISP1_SubData[idx].HDR_7Idx;
    *hdr_fcnt      = F_ISP1_SubData[idx].HDR_fCnt;
    *now_mode      = F_ISP1_SubData[idx].Now_Mode;
    *wdr_live_idx  = F_ISP1_SubData[idx].WDR_Live_Idx;
    *wdr_live_page = F_ISP1_SubData[idx].WDR_Live_Page;
    *lc_en		   = F_ISP1_SubData[idx].LC_En;
}

/*
 * WDR Command Sub Data
 * wdr_mode: 0=n/a
 *           1=sport mode
 *           2=HDR7P
 *          -1=normal
 */
F_Temp_WDR_SubData F_WDR_SubData[F_SUBDATA_MAX];
void set_F_Temp_WDR_SubData(int idx, int hdr_7idx, int wdr_idx, int wdr_mode)
{
    if(idx >= F_SUBDATA_MAX || idx < 0) return;
    F_WDR_SubData[idx].HDR_7Idx  = hdr_7idx;
    F_WDR_SubData[idx].WDR_Index = wdr_idx;
    F_WDR_SubData[idx].WDR_Mode  = wdr_mode;
}
void get_F_Temp_WDR_SubData(int idx, int *hdr_7idx, int *wdr_idx, int *wdr_mode)
{
    if(idx >= F_SUBDATA_MAX || idx < 0) return;
    *hdr_7idx = F_WDR_SubData[idx].HDR_7Idx;
    *wdr_idx  = F_WDR_SubData[idx].WDR_Index;
    *wdr_mode = F_WDR_SubData[idx].WDR_Mode;
}
void cle_F_Temp_WDR_SubData(int idx)
{
    if(idx >= F_SUBDATA_MAX || idx < 0) return;
    F_WDR_SubData[idx].HDR_7Idx  = -1;
    F_WDR_SubData[idx].WDR_Index = -1;
    F_WDR_SubData[idx].WDR_Mode  = -1;
}
void init_F_Temp_WDR_SubData(void)
{
    int i;
    for(i = 0; i < F_SUBDATA_MAX; i++){
        F_WDR_SubData[i].HDR_7Idx  = -1;
        F_WDR_SubData[i].WDR_Index = -1;
        F_WDR_SubData[i].WDR_Mode  = -1;
    }
}
/*
 * ISP2 Command Sub Data
 */
F_Temp_ISP2_SubData F_ISP2_SubData[F_SUBDATA_MAX];
void set_F_Temp_ISP2_SubData(int idx, int nr3d, int hdr_7idx, int hdr_fcnt, int removal_st)
{
    if(idx >= F_SUBDATA_MAX || idx < 0) return;
    F_ISP2_SubData[idx].ISP2_NR3D_Rate   = nr3d;
    F_ISP2_SubData[idx].ISP2_HDR_7Idx    = hdr_7idx;
    F_ISP2_SubData[idx].ISP2_HDR_fCnt    = hdr_fcnt;
    F_ISP2_SubData[idx].ISP2_Removal_St  = removal_st;
}
void get_F_Temp_ISP2_SubData(int idx, int *nr3d, int *hdr_7idx, int *hdr_fcnt, int *removal_st)
{
    if(idx >= F_SUBDATA_MAX || idx < 0) return;
    *nr3d       = F_ISP2_SubData[idx].ISP2_NR3D_Rate;
    *hdr_7idx   = F_ISP2_SubData[idx].ISP2_HDR_7Idx;
    *hdr_fcnt   = F_ISP2_SubData[idx].ISP2_HDR_fCnt;
    *removal_st = F_ISP2_SubData[idx].ISP2_Removal_St;
}
/*
 * Stitch Command Sub Data
 */
F_Temp_Stitch_SubData F_Stitch_SubData[F_SUBDATA_MAX];
void set_F_Temp_Stitch_SubData(int idx, int yuvz_en, int ddr_5s, int st_step, int yuv_ddr, int i_page)
{
    if(idx >= F_SUBDATA_MAX || idx < 0) return;
    F_Stitch_SubData[idx].YUVZ_EN    = yuvz_en;
    F_Stitch_SubData[idx].DDR_5S_CNT = ddr_5s;
    F_Stitch_SubData[idx].ST_Step    = st_step;
    F_Stitch_SubData[idx].YUV_DDR    = yuv_ddr;
    F_Stitch_SubData[idx].I_Page     = i_page;
}
void get_F_Temp_Stitch_SubData(int idx, int *yuvz_en, int *ddr_5s, int *st_step, int *yuv_ddr, int *i_page)
{
    if(idx >= F_SUBDATA_MAX || idx < 0) return;
    *yuvz_en = F_Stitch_SubData[idx].YUVZ_EN;
    *ddr_5s  = F_Stitch_SubData[idx].DDR_5S_CNT;
    *st_step = F_Stitch_SubData[idx].ST_Step;
    *yuv_ddr = F_Stitch_SubData[idx].YUV_DDR;
    *i_page  = F_Stitch_SubData[idx].I_Page;
}
/*
 * Smooth Command Sub Data
 */
F_Temp_Smooth_SubData F_Smooth_SubData[F_SUBDATA_MAX];
void set_F_Temp_Smooth_SubData(int idx, int en)
{
    if(idx >= F_SUBDATA_MAX || idx < 0) return;
    F_Smooth_SubData[idx].En     = en;
}
void get_F_Temp_Smooth_SubData(int idx, int *en)
{
    if(idx >= F_SUBDATA_MAX || idx < 0) return;
    *en     = F_Smooth_SubData[idx].En;
}
void cle_F_Temp_Smooth_SubData(int idx)
{
    if(idx >= F_SUBDATA_MAX || idx < 0) return;
    F_Smooth_SubData[idx].En     = -1;
}
void init_F_Temp_Smooth_SubData(void)
{
    int i;
    for(i = 0; i < F_SUBDATA_MAX; i++){
    	F_Smooth_SubData[i].En     = -1;
    }
}
/*
 * Jpeg Command Sub Data
 *    jpeg engine x2
 */
F_Temp_Jpeg_SubData F_Jpeg_SubData[F_SUBDATA_MAX][2];
void set_F_Temp_Jpeg_SubData(int idx, int eng, int cap_thm, int ddr_5s, int hdr_3s, int h_page, int frame, int integ, int dgain)
{
    if(idx >= F_SUBDATA_MAX || idx < 0 || eng > 1 || eng < 0) return;
    F_Jpeg_SubData[idx][eng].CAP_THM     = cap_thm;
    F_Jpeg_SubData[idx][eng].DDR_5S_CNT  = ddr_5s;
    F_Jpeg_SubData[idx][eng].HDR_3S_CNT  = hdr_3s;
    F_Jpeg_SubData[idx][eng].Header_Page = h_page;
    F_Jpeg_SubData[idx][eng].Frame       = frame;
    F_Jpeg_SubData[idx][eng].Integ       = integ;
    F_Jpeg_SubData[idx][eng].DGain       = dgain;
}
void get_F_Temp_Jpeg_SubData(int idx, int eng, int *cap_thm, int *ddr_5s, int *hdr_3s, int *h_page, int *frame, int *integ, int *dgain)
{
    if(idx >= F_SUBDATA_MAX || idx < 0 || eng > 1 || eng < 0) return;
    *cap_thm = F_Jpeg_SubData[idx][eng].CAP_THM;
    *ddr_5s  = F_Jpeg_SubData[idx][eng].DDR_5S_CNT;
    *hdr_3s  = F_Jpeg_SubData[idx][eng].HDR_3S_CNT;
    *h_page  = F_Jpeg_SubData[idx][eng].Header_Page;
    *frame   = F_Jpeg_SubData[idx][eng].Frame;
    *integ   = F_Jpeg_SubData[idx][eng].Integ;
    *dgain   = F_Jpeg_SubData[idx][eng].DGain;
}
/*
 * H264 Command Sub Data
 */
F_Temp_H264_SubData F_H264_SubData[F_SUBDATA_MAX];
void set_F_Temp_H264_SubData(int idx, int do_h264, int ip_mode, int fs, int fc)
{
    if(idx >= F_SUBDATA_MAX || idx < 0) return;
    F_H264_SubData[idx].do_H264 = do_h264;
    F_H264_SubData[idx].IP_Mode     = ip_mode;
    F_H264_SubData[idx].Frame_Stamp = fs;
    F_H264_SubData[idx].Frame_Cnt   = fc;
}
void get_F_Temp_H264_SubData(int idx, int *do_h264, int *ip_mode, int *fs, int *fc)
{
    if(idx >= F_SUBDATA_MAX || idx < 0) return;
    *do_h264 = F_H264_SubData[idx].do_H264;
    *ip_mode = F_H264_SubData[idx].IP_Mode;
    *fs      = F_H264_SubData[idx].Frame_Stamp;
    *fc      = F_H264_SubData[idx].Frame_Cnt;
}
/*
 * DMA Command Sub Data
 * btm_idx: 0:	DMA底圖
 * 			1:	DMA文字
 * 			2:	12K DMA成 8K/6K
 * 			3:	Removal
 * 			4:	小圖的底圖合併DMA(底圖跟文字)
 * 			5:  Timelapse 8K/6K的底圖合併DMA(底圖跟文字)
 * 			6:  Timelapse DMA成小圖(12K->4K, 8K/6K->3K)
 */
F_Temp_DMA_SubData F_DMA_SubData[F_SUBDATA_MAX];
void set_F_Temp_DMA_SubData(int idx, int btm_mode, int btm_size, int cp_mode, int btm_t_m, int btm_idx)
{
    if(idx >= F_SUBDATA_MAX || idx < 0) return;
    F_DMA_SubData[idx].Btm_Mode		= btm_mode;
    F_DMA_SubData[idx].Btm_Size 	= btm_size;
    F_DMA_SubData[idx].CP_Mode  	= cp_mode;
    F_DMA_SubData[idx].BtmText_Mode = btm_t_m;
    F_DMA_SubData[idx].Btm_Idx      = btm_idx;
}
void get_F_Temp_DMA_SubData(int idx, int *btm_mode, int *btm_size, int *cp_mode, int *btm_t_m, int *btm_idx)
{
    if(idx >= F_SUBDATA_MAX || idx < 0) return;
    *btm_mode = F_DMA_SubData[idx].Btm_Mode;    // 0:關, 1:延伸, 2:加底圖(default), 3:鏡像, 4:加底圖(user)
    *btm_size = F_DMA_SubData[idx].Btm_Size;
    *cp_mode  = F_DMA_SubData[idx].CP_Mode;     // 0:手動, 1:自動(G Sensor)
    *btm_t_m  = F_DMA_SubData[idx].BtmText_Mode;
    *btm_idx  = F_DMA_SubData[idx].Btm_Idx;
}
/*
 * USB Command Sub Data
 */
F_Temp_USB_SubData F_USB_SubData[F_SUBDATA_MAX];
void set_F_Temp_USB_SubData(int idx, int enc_type, int usb_en)
{
    if(idx >= F_SUBDATA_MAX || idx < 0) return;
    F_USB_SubData[idx].Encode_Type = enc_type;
    F_USB_SubData[idx].USB_En      = usb_en;
}
void get_F_Temp_USB_SubData(int idx, int *enc_type, int *usb_en)
{
    if(idx >= F_SUBDATA_MAX || idx < 0) return;
    *enc_type = F_USB_SubData[idx].Encode_Type;
    *usb_en   = F_USB_SubData[idx].USB_En;
}

int show_small_debug_en = 0;
int WDR_Live_Page = 0;
extern struct FPGA_Speed_Struct FPGA_Speed[6];
void F_Pipe_Add_Small(int Img_Mode, int C_Mode, int hdr_step, int *isp1_idx, int *shuter_t, int *ts_max)
{
    int Stitch_All;
    int Check_Flag=1, Sub_Idx;
    static int yuvz_st_idx = 0;
    int yuvz_en = -1;
    int thm_mode = 5;
    int usb_thm_spend_time = get_USB_Spend_Time(thm_mode);
    int isp1_idx_tmp = 0;
    int hdr7_idx = -1, small_f = 1;
    int shuter_time;
	int Now_Mode, S_Mode;
	int Times, T_Max=0;
	int btm_m, btm_s=100, btm_t_m, cp_mode;
	Get_M_Mode(&Now_Mode, &S_Mode);
#ifdef ANDROID_CODE
    get_A2K_DMA_BottomMode(&btm_m, &btm_s);
    get_A2K_DMA_BottomTextMode(&btm_t_m);
    get_A2K_DMA_CameraPosition(&cp_mode);
#endif

    if(C_Mode == 1 || C_Mode == 10) {			//錄影 4K(15fps) 3K(24fps) 縫合效能不足, 分三次執行YUVZ縫合
    	Stitch_All = FPGA_Speed[Img_Mode].Stitch_Img;
    	switch(yuvz_st_idx) {
    	case 0: yuvz_en = PIPE_SUBCODE_ST_YUVZ_0; break;
    	case 1: yuvz_en = PIPE_SUBCODE_ST_YUVZ_1; break;
    	case 2: yuvz_en = PIPE_SUBCODE_ST_YUVZ_2; break;
    	}
    	yuvz_st_idx++;
    	if(yuvz_st_idx > 2) yuvz_st_idx = 0;
    }
    else {
    	Stitch_All = (FPGA_Speed[Img_Mode].Stitch_Img + FPGA_Speed[Img_Mode].Stitch_ZYUV * 3);
    	yuvz_en = PIPE_SUBCODE_ST_YUVZ_ALL;
    }

    memcpy(&F_Temp, &F_Pipe, sizeof(F_Pipe) );
    if(show_small_debug_en == 1) F_Pipe_Add_Sub(-1, -1, -1, -1, -1, -1, &Sub_Idx, &Times);
    
    int frame=-1, integ=-1, dgain=-1, exp_n=-1, exp_m=-1, iso=-1;
    /*if(hdr_step == 1 || hdr_step == 2) {		//+0
    	do_AEB_sensor_cal(3, 0, F_Com_In.Record_Mode, &frame, &integ, &dgain, &exp_n, &exp_m, &iso);
    	hdr7_idx = PIPE_SUBCODE_AEB_STEP_0 + hdr_step;
    	small_f = 0;
    	shuter_time = F_Com_In.Shuter_Speed * (frame+1);
    }
    else*/ if(hdr_step == 3 || hdr_step == 4) {		//-5
    	do_AEB_sensor_cal(2, 100, F_Com_In.Record_Mode, &frame, &integ, &dgain, &exp_n, &exp_m, &iso);
    	hdr7_idx = PIPE_SUBCODE_AEB_STEP_0 + hdr_step;
    	small_f = 0;
    	shuter_time = F_Com_In.Shuter_Speed * (frame+1);
    }
    else if(hdr_step == 5) {		//live, 解長時間曝光(+0)轉短時間曝光(HDR7P)錯誤問題
    	frame = -1; integ = -1; dgain = -1; exp_n = -1; exp_m = -1; iso = -1;
    	hdr7_idx = PIPE_SUBCODE_AEB_STEP_0 + hdr_step;
    	small_f = 1;
    	shuter_time = F_Com_In.Shuter_Speed;
    }
    else {
		frame = -1; integ = -1; dgain = -1; exp_n = -1; exp_m = -1; iso = -1;
		hdr7_idx = -1;
		small_f = 1;
		shuter_time = F_Com_In.Shuter_Speed;
    }
    *shuter_t = shuter_time;

    Check_Flag &= F_Pipe_Add_Sub(Img_Mode,1, 1,101,102, shuter_time, &Sub_Idx, &Times);      //[].Sensor
    if(Times > T_Max) T_Max = Times;
    set_F_Temp_Sensor_SubData(Sub_Idx, hdr7_idx, -1, small_f, frame, integ, dgain);  // small_f=1
    Check_Flag &= F_Pipe_Add_Sub(Img_Mode,1, 1,102,103, F_Com_In.Pipe_Time_Base, &Sub_Idx, &Times);    //[].ISP1
    if(Times > T_Max) T_Max = Times;
    isp1_idx_tmp = Sub_Idx;

    int jtbl_idx=0;

    if(C_Mode == 9 || C_Mode == 10 || C_Mode == 11) 						 // Sport WDR / Rec WDR / TimeLapse WDR
    {
        set_F_Temp_ISP1_SubData(Sub_Idx, -1, -1, Now_Mode, -1, WDR_Live_Page, 1);
        Set_F_JPEG_Header_ISP1(Sub_Idx, &jtbl_idx);
        set_F_Temp_WDR_SubData(Sub_Idx, WDR_Live_Page, 1, 1);
    }
    else {
        set_F_Temp_ISP1_SubData(Sub_Idx, -1, -1, Now_Mode, -1, -1, 1);
        Set_F_JPEG_Header_ISP1(Sub_Idx, &jtbl_idx);
        set_F_Temp_WDR_SubData(Sub_Idx, -1, -1, -1);
    }
    WDR_Live_Page = (WDR_Live_Page+1) & 0x1;
    Check_Flag &= F_Pipe_Add_Sub(Img_Mode,1, 2,103,104, FPGA_Speed[Img_Mode].ISP2, &Sub_Idx, &Times);  //[].ISP2
    if(Times > T_Max) T_Max = Times;
    set_F_Temp_ISP2_SubData(Sub_Idx, -1, -1, -1, -1);
    Check_Flag &= F_Pipe_Add_Sub(Img_Mode,1, 3,104,106, Stitch_All, &Sub_Idx, &Times);                 //[].Stitch
    if(Times > T_Max) T_Max = Times;
    set_F_Temp_Stitch_SubData(Sub_Idx, yuvz_en, -1, -1, -1, -1);
    Check_Flag &= F_Pipe_Add_Sub(Img_Mode,1, 4,109,110, 0, &Sub_Idx, &Times);                 			//[].Smooth
    if(Times > T_Max) T_Max = Times;
    set_F_Temp_Smooth_SubData(Sub_Idx, -1);

	int dma_time, jpeg_p0=4;
	if(btm_m == 0 && btm_t_m == 0) {
	    Check_Flag &= F_Pipe_Add_Sub(Img_Mode,1, 4,106,110,0, &Sub_Idx, &Times);                           //[].DMA
	    if(Times > T_Max) T_Max = Times;
	    set_F_Temp_DMA_SubData(Sub_Idx, -1, -1, -1, -1, -1);
		jpeg_p0 = 4;
	}
	else {
		switch(btm_m) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4: F_Temp.DMA.Page[1] = F_Temp.Stitch.Page[1];
				if(btm_m == 3) dma_time = FPGA_Speed[Img_Mode].DMA;		//鏡像
				else           dma_time = FPGA_Speed[5].DMA;
				Check_Flag &= F_Pipe_Add_Sub(Img_Mode,1, 4, 106, 105, dma_time, &Sub_Idx, &Times);                  //[].DMA
				if(Times > T_Max) T_Max = Times;
				if(btm_t_m == 1 && (btm_m == 1 || btm_m == 2 || btm_m == 3) )
					set_F_Temp_DMA_SubData(Sub_Idx, btm_m, btm_s, cp_mode, btm_t_m, 4);		//4:合併DMA
				else
					set_F_Temp_DMA_SubData(Sub_Idx, btm_m, btm_s, cp_mode, btm_t_m, 0);
				if(Img_Mode == 4) jpeg_p0 = 4;
				else			  jpeg_p0 = 6;
				break;
		}
	}

    Check_Flag &= F_Pipe_Add_Sub(Img_Mode,1, jpeg_p0,105,107, FPGA_Speed[Img_Mode].Jpeg, &Sub_Idx, &Times);  //[].Jpeg
    if(Times > T_Max) T_Max = Times;
    set_F_Temp_Jpeg_SubData(Sub_Idx, F_Temp.Jpeg_Id, -1, -1, -1, jtbl_idx, -1, -1, -1);
    Check_Flag &= F_Pipe_Add_Sub(Img_Mode ,1, 4, 8, 7, 0, &Sub_Idx, &Times);						//[].H264
    if(Times > T_Max) T_Max = Times;
    set_F_Temp_H264_SubData(Sub_Idx, 0, 0, 0, 0);
    Check_Flag &= F_Pipe_Add_Sub(Img_Mode,1, 5,107,110, FPGA_Speed[Img_Mode].USB, &Sub_Idx, &Times);   //[].USB
    if(Times > T_Max) T_Max = Times;
    set_F_Temp_USB_SubData(Sub_Idx, F_Com_In.encode_type, 1);

    F_Temp.Jpeg[F_Temp.Jpeg_Id].Page[1]   = (F_Temp.Jpeg[F_Temp.Jpeg_Id].Page[1]+1) % 2;
    F_Temp.Jpeg_Id   = (F_Temp.Jpeg_Id+1) % 2;
    F_Temp.USB.Page[1]    = (F_Temp.USB.Page[1]+1) % 2;
    F_Temp.Pic_No++;

    if( (C_Mode == 1 || C_Mode == 10) && F_Com_In.do_Rec_Thm == 1) {		//錄影開始壓一張縮圖 1024*1024
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 4, 105, 107,FPGA_Speed[thm_mode].Jpeg, &Sub_Idx, &Times);  //[].Jpeg
        if(Times > T_Max) T_Max = Times;
        set_F_Temp_Jpeg_SubData(Sub_Idx, F_Temp.Jpeg_Id, PIPE_SUBCODE_CAP_THM, -1, -1, jtbl_idx, -1, -1, -1);                                     		//[].Jpeg
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode ,0, 4, 8, 7, 0, &Sub_Idx, &Times);						//[].H264
        if(Times > T_Max) T_Max = Times;
        set_F_Temp_H264_SubData(Sub_Idx, 0, 0, 0, 0);
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 4, 6, 10,0, &Sub_Idx, &Times);                            //[].DMA
        if(Times > T_Max) T_Max = Times;
        set_F_Temp_DMA_SubData(Sub_Idx, -1, -1, -1, -1, -1);
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 5, 107, 110,usb_thm_spend_time, &Sub_Idx, &Times);         //[].USB
        if(Times > T_Max) T_Max = Times;
        set_F_Temp_USB_SubData(Sub_Idx, F_Com_In.encode_type, 1);

        F_Temp.Jpeg[F_Temp.Jpeg_Id].Page[1]   = (F_Temp.Jpeg[F_Temp.Jpeg_Id].Page[1]+1) % 2;
    	F_Temp.Jpeg_Id   = (F_Temp.Jpeg_Id+1) % 2;
        F_Temp.USB.Page[1]    = (F_Temp.USB.Page[1]+1) % 2;
        F_Temp.Pic_No++;

        F_Com_In.do_Rec_Thm = 0;
    }

    if(Check_Flag == 1) {
    	*isp1_idx = isp1_idx_tmp;
    	*ts_max = T_Max;
    }
    else {
    	*isp1_idx = -1;
    	*ts_max = 1;
    }

    F_Temp.Sensor.Page[1] = (F_Temp.Sensor.Page[1]+1) % 2;
    F_Temp.ISP1.Page[1]   = (F_Temp.ISP1.Page[1]+1) % 2;
    F_Temp.ISP2.Page[1]   = (F_Temp.ISP2.Page[1]+1) % 2;
    F_Temp.Stitch.Page[1] = (F_Temp.Stitch.Page[1]+1) % 3;		//Page_2為了解錄影3K ST&JPEG同時讀寫問題
    F_Temp.Smooth.Page[1] = (F_Temp.Smooth.Page[1]+1) % 3;
    F_Temp.DMA.Page[1]    = (F_Temp.DMA.Page[1]+1) % 3;

    if(show_small_debug_en == 1) F_Pipe_Add_Sub(-2, -2, -2, -2, -2, -2, &Sub_Idx, &Times);
    Make_F_Pipe( Img_Mode, Check_Flag, C_Mode);                 // F_Pipe_Add_Small
};


int get_USB_Spend_Time(int mode)
{
    int frm, time, gain;
    int stime;
#ifdef ANDROID_CODE
    get_AEG_EP_Var(&frm, &time, &gain);
    if     (gain >= 100*64) stime = (FPGA_Speed[mode].USB * 1500) / 1000;      // x1.5 , ISO3200
    else if(gain >= 60*64)  stime = (FPGA_Speed[mode].USB * 1333) / 1000;      // x1.33, ISO800
    else                    stime = (FPGA_Speed[mode].USB);                    // x1
#else
    stime = (FPGA_Speed[mode].USB);
#endif
    return stime;
}

void Get_Smooth_Page(int en, int mode, int *st_p, int *dma_p, int *smooth_t)
{
    if(en == 1) {
    	*smooth_t = FPGA_Speed[mode].Smooth;
    	*st_p = 9; *dma_p = 9;
    }
    else {
    	*smooth_t = 0;
    	*st_p = 6; *dma_p = 4;
    }
}

int F_Pipe_Add_Big(int Img_Mode, int *Finish_Idx, int *Sensor_Idx, int C_Mode)
{
    int i, time = 0;
    int Check_Flag = 1, Sub_Idx;
    int P0_Idx;
    int thm_mode = 5;
    struct FPGA_Engine_Resource_Struct *P1;
    int usb_spend_time;
    int usb_thm_spend_time = get_USB_Spend_Time(thm_mode);
    int p_mask = (FPGA_ENGINE_P_MAX-1);
    int thm_jpeg_Idx = 0;
    int btm_m=0, btm_s=100, cp_mode=0, np=0, btm_t_m=0;
    int shuter_time = F_Com_In.Shuter_Speed;
    int Now_Mode, S_Mode;
    int mode, res;
    int sensor_idx_tmp = -1;
    int Times;
    get_Stitching_Out(&mode, &res);
    if(res == 7)       Now_Mode = 1;			//8K
    else if(res == 12) Now_Mode = 2;			//6K
    else               Now_Mode = Img_Mode;
    usb_spend_time = get_USB_Spend_Time(Now_Mode);
#ifdef ANDROID_CODE
    get_A2K_DMA_BottomMode(&btm_m, &btm_s);
    get_A2K_DMA_BottomTextMode(&btm_t_m);
    get_A2K_DMA_CameraPosition(&cp_mode);
    np = get_AEG_System_Freq_NP();
    shuter_time = get_A2K_Shuter_Speed();       // F_Pipe_Add_Big
#endif
    if(C_Mode == 12)
        shuter_time = 1000000;

    P1 = &F_Pipe.Sensor;
    P0_Idx = P1->Idx;
    P1->P[P0_Idx].No       = -1;
    P1->P[P0_Idx].Time_Buf = -1;
    P1->P[P0_Idx].I_Page   = -1;
    P1->P[P0_Idx].O_Page   = -1;
    P1->P[P0_Idx].M_Mode   = Img_Mode;
    P1->Idx =  (P1->Idx + 1) & p_mask;

    memcpy(&F_Temp, &F_Pipe, sizeof(F_Pipe) );
    F_Pipe_Add_Sub(-1, -1, -1, -1, -1, -1, &Sub_Idx, &Times);           // debug
    if(btm_s != 0) F_Temp.DMA.Page[0] = F_Temp.Stitch.Page[0];  // rex+ 180626, 解DMA錯誤，拍成前一張畫面

    int long_ep = (np == 0)? 66667: 80000;
    int jtbl_idx=0;
    if(shuter_time <= long_ep){      // EP <= 1/15(66666) or 1/12.5(80000)
        for (i = 0; i < 3; i++) {
            //[].Sensor
            Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 1, 1, 2, shuter_time, &Sub_Idx, &Times);
            set_F_Temp_Sensor_SubData(Sub_Idx, -1, -1, 0, -1, -1, -1);  // small_f=0

            //[].ISP1
            Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 1, 2, 3, F_Com_In.Pipe_Time_Base, &Sub_Idx, &Times);
            if(/*C_Mode == 8 ||*/ C_Mode == 9) {		//Sport WDR
                set_F_Temp_ISP1_SubData(Sub_Idx, -1, -1, Img_Mode, -1, WDR_Live_Page, 1);
                if(i == 0) Set_F_JPEG_Header_ISP1(Sub_Idx, &jtbl_idx);
                set_F_Temp_WDR_SubData(Sub_Idx, WDR_Live_Page, 1, 1);
            }
            else {
                set_F_Temp_ISP1_SubData(Sub_Idx, -1, -1, Img_Mode, -1, -1, 1);
                if(i == 0) Set_F_JPEG_Header_ISP1(Sub_Idx, &jtbl_idx);
                set_F_Temp_WDR_SubData(Sub_Idx, -1, -1, -1);
            }
            sensor_idx_tmp = Sub_Idx;
            //[].ISP2, 0~3 ISP2_NR3D_Rate
            Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 2, 3, 4, FPGA_Speed[Img_Mode].ISP2, &Sub_Idx, &Times);
            set_F_Temp_ISP2_SubData(Sub_Idx, i, -1, -1, -1);
            if(i < 2) {         // isp2page idx要給下一級使用, 不能先加1
                F_Temp.Sensor.Page[0] = (F_Temp.Sensor.Page[0]+1) % 2;
                F_Temp.ISP1.Page[0]   = (F_Temp.ISP1.Page[0]+1) % 3;
                F_Temp.ISP2.Page[0]   = (F_Temp.ISP2.Page[0]+1) % 2;
                F_Temp.Pic_No++;
            }
        }
    }
    else{
        int ep_long_t = 1;
        int isp1_time;
        if(C_Mode == 12)
            isp1_time = 1000000 + F_Com_In.Pipe_Time_Base*2;
        else
            isp1_time = F_Com_In.Pipe_Time_Base*2;

        if(shuter_time > F_Com_In.Pipe_Time_Base){              // 解長時間拍照時會殘留上一張畫面(鋸齒狀)
            shuter_time -= F_Com_In.Pipe_Time_Base;
            isp1_time += F_Com_In.Pipe_Time_Base;
        }
        //[].Sensor
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 1, 1, 2, shuter_time, &Sub_Idx, &Times);
        set_F_Temp_Sensor_SubData(Sub_Idx, -1, ep_long_t, 0, -1, -1, -1);          // Add_Big, ep_long_t=n, small_f=0

        //[].ISP1
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 1, 2, 3, isp1_time, &Sub_Idx, &Times);     // Pipe_Time_Base*2, 解長時間會有鋸齒畫面
        if(/*C_Mode == 8 ||*/ C_Mode == 9) {		//Sport WDR
            set_F_Temp_ISP1_SubData(Sub_Idx, -1, -1, Img_Mode, -1, WDR_Live_Page, 1);
            Set_F_JPEG_Header_ISP1(Sub_Idx, &jtbl_idx);
            set_F_Temp_WDR_SubData(Sub_Idx, WDR_Live_Page, 1, 1);
        }
        else {
            set_F_Temp_ISP1_SubData(Sub_Idx, -1, -1, Img_Mode, -1, -1, 1);
            Set_F_JPEG_Header_ISP1(Sub_Idx, &jtbl_idx);
            set_F_Temp_WDR_SubData(Sub_Idx, -1, -1, -1);
        }
        sensor_idx_tmp = Sub_Idx;
        //[].ISP2, 0~3 ISP2_NR3D_Rate
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 2, 3, 4, FPGA_Speed[Img_Mode].ISP2, &Sub_Idx, &Times);
        set_F_Temp_ISP2_SubData(Sub_Idx, 0, -1, -1, -1);
        
        // 恢復原狀, 拍完不閃爍
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 2, 1, 2, F_Com_In.Shuter_Speed, &Sub_Idx, &Times);
        set_F_Temp_Sensor_SubData(Sub_Idx, -1, -1, 0, -1, -1, -1);
    }

    int st_p2, dma_p0;
    int smooth_time;
    Get_Smooth_Page(F_Com_In.Smooth_En, Img_Mode, &st_p2, &dma_p0, &smooth_time);
    if(get_run_big_smooth() == 0){
    	//[].Stitch
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 3, 4, st_p2, FPGA_Speed[Img_Mode].Stitch_Img, &Sub_Idx, &Times);
        set_F_Temp_Stitch_SubData(Sub_Idx, -1, -1, -1, -1, -1);
        //[].Smooth
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 4, 9, 6, smooth_time, &Sub_Idx, &Times);
        set_F_Temp_Smooth_SubData(Sub_Idx, F_Com_In.Smooth_En);
    }
    else{
    	//[].Stitch
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 3, 4, st_p2, F_Com_In.Pipe_Time_Base*3+200000, &Sub_Idx, &Times);       // time=0.5s~0.8s
        set_F_Temp_Stitch_SubData(Sub_Idx, -1, -1, 1, 1, -1);
        //[].Smooth
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 4, 9, 6, 0, &Sub_Idx, &Times);
        set_F_Temp_Smooth_SubData(Sub_Idx, -1);
        //[].Stitch
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 3, 4, st_p2, FPGA_Speed[Img_Mode].Stitch_Img, &Sub_Idx, &Times);
        set_F_Temp_Stitch_SubData(Sub_Idx, -1, -1, 2, 1, -1);
        //[].Smooth
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 4, 9, 6, smooth_time, &Sub_Idx, &Times);
        set_F_Temp_Smooth_SubData(Sub_Idx, F_Com_In.Smooth_En);
    }
    int dma_time;
    switch(btm_m) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4: if(F_Com_In.Smooth_En == 1) F_Temp.DMA.Page[0] = F_Temp.Smooth.Page[0];
			else						F_Temp.DMA.Page[0] = F_Temp.Stitch.Page[0];
    		if(btm_m == 3) dma_time = FPGA_Speed[Img_Mode].DMA;		//鏡像
            else           dma_time = FPGA_Speed[5].DMA;
            Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, dma_p0, 6, 5, dma_time, &Sub_Idx, &Times);                  //[].DMA
            set_F_Temp_DMA_SubData(Sub_Idx, btm_m, btm_s, cp_mode, btm_t_m, 0);
            if(btm_t_m == 1 && (btm_m == 1 || btm_m == 2 || btm_m == 3) ) {
            	dma_time = FPGA_Speed[5].DMA;
            	Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 6, 6, 5, dma_time, &Sub_Idx, &Times);                  //[].DMA
            	set_F_Temp_DMA_SubData(Sub_Idx, btm_m, btm_s, cp_mode, btm_t_m, 1);
    		}
    	    break;
    }
    if(Now_Mode == 1 || Now_Mode == 2) {		//12K DMA成 6K/8K
    	if(F_Com_In.Smooth_En == 1) F_Temp.DMA.Page[0] = (F_Temp.Smooth.Page[0]+1) % 2;
    	else						F_Temp.DMA.Page[0] = (F_Temp.Stitch.Page[0]+1) % 2;
    	Check_Flag &= F_Pipe_Add_Sub(Now_Mode,0, dma_p0, 6, 5, FPGA_Speed[Img_Mode].DMA, &Sub_Idx, &Times);                  //[].DMA,	12K -> 8K 6K
    	set_F_Temp_DMA_SubData(Sub_Idx, -1, -1, -1, -1, 2);
    }
    Check_Flag &= F_Pipe_Add_Sub(Now_Mode,0, 6, 5, 7, FPGA_Speed[Now_Mode].Jpeg, &Sub_Idx, &Times);         //[].Jpeg
    set_F_Temp_Jpeg_SubData(Sub_Idx, F_Temp.Jpeg_Id, -1, -1, -1, jtbl_idx, -1, -1, -1);
    Check_Flag &= F_Pipe_Add_Sub(Now_Mode ,0, 4, 8, 7, 0, &Sub_Idx, &Times);						        //[].H264
    set_F_Temp_H264_SubData(Sub_Idx, 0, 0, 0, 0);
    Check_Flag &= F_Pipe_Add_Sub(Now_Mode,0, 5, 7, 10, usb_spend_time, &Sub_Idx, &Times);                    //[].USB
    set_F_Temp_USB_SubData(Sub_Idx, F_Com_In.encode_type, 1);
    F_Com_In.Capture_Lose_Page[0] = F_Temp.USB.P[Sub_Idx&(FPGA_ENGINE_P_MAX-1)].I_Page;

    F_Temp.Jpeg[F_Temp.Jpeg_Id].Page[0]   = (F_Temp.Jpeg[F_Temp.Jpeg_Id].Page[0]+1) % 2;
    F_Temp.Jpeg_Id   = (F_Temp.Jpeg_Id+1) % 2;
    F_Temp.USB.Page[0]    = (F_Temp.USB.Page[0]+1) % 2;
    F_Temp.Pic_No++;

    //[].Jpeg
    Check_Flag &= F_Pipe_Add_Sub(Now_Mode,0, 7, 5, 7, FPGA_Speed[thm_mode].Jpeg, &Sub_Idx, &Times);
    set_F_Temp_Jpeg_SubData(Sub_Idx, F_Temp.Jpeg_Id, PIPE_SUBCODE_CAP_THM, -1, -1, jtbl_idx, -1, -1, -1);
    //[].H264
    Check_Flag &= F_Pipe_Add_Sub(Now_Mode ,0, 7, 8, 7, 0, &Sub_Idx, &Times);
    set_F_Temp_H264_SubData(Sub_Idx, 0, 0, 0, 0);
    //[].DMA
    Check_Flag &= F_Pipe_Add_Sub(Now_Mode,0, dma_p0, 6, 10,0, &Sub_Idx, &Times);
    set_F_Temp_DMA_SubData(Sub_Idx, -1, -1, -1, -1, -1);
    //[].USB
    Check_Flag &= F_Pipe_Add_Sub(Now_Mode,0, 5, 7, 10,usb_thm_spend_time, &Sub_Idx, &Times);
    set_F_Temp_USB_SubData(Sub_Idx, F_Com_In.encode_type, 1);
    F_Com_In.Capture_Lose_Page[1] = F_Temp.USB.P[Sub_Idx&(FPGA_ENGINE_P_MAX-1)].I_Page;
    
    F_Temp.Sensor.Page[0] = (F_Temp.Sensor.Page[0]+1) % 2;
    F_Temp.ISP1.Page[0]   = (F_Temp.ISP1.Page[0]+1) % 3;
    F_Temp.ISP2.Page[0]   = (F_Temp.ISP2.Page[0]+1) % 2;
    F_Temp.Stitch.Page[0] = (F_Temp.Stitch.Page[0]+1) % 2;
    F_Temp.Smooth.Page[0] = (F_Temp.Smooth.Page[0]+1) % 2;
    F_Temp.Jpeg[F_Temp.Jpeg_Id].Page[0]   = (F_Temp.Jpeg[F_Temp.Jpeg_Id].Page[0]+1) % 2;
    F_Temp.Jpeg_Id   = (F_Temp.Jpeg_Id+1) % 2;
    F_Temp.DMA.Page[0]    = (F_Temp.DMA.Page[0]+1) % 2;
    F_Temp.USB.Page[0]    = (F_Temp.USB.Page[0]+1) % 2;

    if(Check_Flag == 1) {
        *Finish_Idx = Sub_Idx;                      // 紀錄最後一筆 cmd_idx
        *Sensor_Idx = sensor_idx_tmp;
    }
    
    F_Pipe_Add_Sub(-2, -2, -2, -2, -2, -2, &Sub_Idx, &Times);
    Make_F_Pipe( Now_Mode, Check_Flag, C_Mode);                 // F_Pipe_Add_Big
    return Check_Flag;
};
/*
 * do_AEB_sensor_calculate
 */
void do_AEB_sensor_cal(int istep, int ae_scale, int c_mode, int *o_frame, int *o_integ, int *o_dgain, int *exp_n, int *exp_m, int *iso)
{
    int aeg_idx, aeg_live, aeg_max, aeg_min, gain_max;          // live mode aeg_idx
    int exp_max;                                                // 60=1/15s, 80= 1/8s, 100=1/4s, 120=1/2s, 140=1s, 160=2s
    int exp_tmp_idx=0, gain_tmp_idx=0;
    int frame=0, integ=3666, dgain=0, fps=100;
    extern int AEB_Frame_Cnt;

    get_PipeCmd_AEG_idx(&aeg_live, &aeg_max, &aeg_min, &gain_max);
    exp_max = aeg_max - gain_max;
    get_real_exp_gain_idx(c_mode, aeg_live, exp_max, &exp_tmp_idx, &gain_tmp_idx, -1);
    if(exp_tmp_idx < -10240) exp_tmp_idx = -10240;                  // 1/32000, -10240=-160*64
    if(exp_tmp_idx > exp_max) exp_tmp_idx = exp_max;
    if(gain_tmp_idx < 0) gain_tmp_idx = 0;
    if(gain_tmp_idx > gain_max) gain_tmp_idx = gain_max;
    aeg_live = exp_tmp_idx + gain_tmp_idx;                      // 取得真實含手動設定後的值

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
 *                                                -20*64  1/240  (n<0)
 *                                                ...
 */
    // 3: istep=2,3,4
    // 5: istep=1,2,3,4,5
    // 7: istep=0,1,2,3,4,5,6
    switch(istep){
        case 0: aeg_idx = aeg_live - (ae_scale*64*3); exp_max = exp_tmp_idx; break;     // ISP1-A   , ae_scale=20(1倍)
        case 1: aeg_idx = aeg_live - (ae_scale*64*2); exp_max = exp_tmp_idx; break;     // ISP1-B
        case 2: aeg_idx = aeg_live - (ae_scale*64);   exp_max = exp_tmp_idx; break;     // ISP1-C
        default:
        case 3: aeg_idx = aeg_live;                   exp_max = exp_tmp_idx;  break;                    // ISP2-A0
        case 4: aeg_idx = aeg_live + (ae_scale*64);   exp_max = exp_tmp_idx + (ae_scale*64); break;     // ISP2-A0, 120+40->160=2s, 60+40->100=1/4s
        case 5: aeg_idx = aeg_live + (ae_scale*64*2); exp_max = exp_tmp_idx + (ae_scale*64*2); break;   // ISP2-A1
        case 6: aeg_idx = aeg_live + (ae_scale*64*3); exp_max = exp_tmp_idx + (ae_scale*64*3); break;   // ISP2-A1
    }
    int pi_max;
    switch(istep){
        default:
        case 0: 
        case 1:
        case 2:
        case 3: if(c_mode == 3 && AEB_Frame_Cnt == 7) pi_max = (120*64);     // AEB, 120=0.5s, 60=1/15
                else if(c_mode == 7) pi_max = (120*64);     // N-HDR
                else                 pi_max = (60*64);
                break;
        case 4: 
        case 5: 
        case 6: if(c_mode == 3 && AEB_Frame_Cnt == 7) pi_max = (120*64);     // AEB, 120=0.5s
                else if(c_mode == 7) pi_max = (160*64);     // N-HDR, 160=2S
                else                 pi_max = (140*64);     // D-HDR, 140=1s
                break; 
    }
    if(exp_max > pi_max) exp_max = pi_max;

    aeg_max = exp_max + gain_max;
    Set_HDR7P_Auto_EV_Inc_T(aeg_live, aeg_idx, aeg_max, aeg_min);
    get_real_exp_gain_idx(c_mode, aeg_idx, exp_max, &exp_tmp_idx, &gain_tmp_idx, istep);

    //db_debug("do_AEB_sensor_cal: aeg_idx=%d max=%d:%d:%d tmp=%d:%d\n", 
    //    aeg_idx>>6, aeg_max>>6, exp_max>>6, gain_max>>6, exp_tmp_idx>>6, gain_tmp_idx>>6);
    calculate_real_sensor_exp(exp_tmp_idx, &frame, &integ, &fps);
    if(gain_tmp_idx > 100*64) gain_tmp_idx = 100*64;                  // 0->100 20->200 40->400 60->800 80->ISO1600
    Set_HDR7P_AEG_Idx(istep, aeg_idx, aeg_max, aeg_min, frame, integ, gain_tmp_idx);

    *o_frame = frame;
    *o_integ = integ;
    *o_dgain = gain_tmp_idx;

    int np = get_AEG_System_Freq_NP();
    int b_sec, b_gain;
    get_AEG_B_Exp(&b_sec, &b_gain);
    cal_exp_iso(np, exp_tmp_idx, gain_tmp_idx, c_mode, b_sec, b_gain, exp_n, exp_m, iso);
    //db_debug("do_AEB_sensor_cal: istep=%d c_mode=%d aexp=%d gain=%d\n", istep, c_mode, exp_tmp_idx>>6, gain_tmp_idx>>6);

    //db_debug("do_AEB_sensor_cal() HDR7P: istep=%d ae_scale=%d aeg_live=%d aeg_idx=%d  frame=%d integ=%d gain=%d  exp_idx=%d gain_idx=%d exp_n=%d exp_m=%d max_line=%d\n",
    //  		istep, ae_scale, aeg_live, aeg_idx, frame, integ, gain_tmp_idx, exp_tmp_idx, gain_tmp_idx, *exp_n, *exp_m, get_ep_ln_maximum() );
}
/*
 * rex+ 190321 AEB Function
 *   AEB 3P、5P 存檔
 */
int AEB_ST_Enable = 0;
int read_AEB_ST_Enable(void)
{
    return AEB_ST_Enable;
}
void set_AEB_ST_Enable(int en)
{
    AEB_ST_Enable = en;
}
int AEB_Frame_Cnt = 3;          // 3,5,7
int AEB_AE_Scale = 20;          // 1.0x20=20,1.5x20=30,2.0x20=40,2.5x20=50, 20(AE+-1.0),30(AE+-1.5),40(AE+-2.0),50(AE+-2.5)
int AEB_Pixel_Diff = 30;        // 10-60
int AEB_Strength = 100;         // 100%,75%,50%
int AEB_Auto_HDR_En = 0;        // AEB Auto HDR Debug

int F_Pipe_Add_AEB(int Img_Mode, int *Finish_Idx, int *Sensor_Idx, int C_Mode)
{
    int i;
    int Check_Flag = 1, Sub_Idx, P0_Idx;
    struct FPGA_Engine_Resource_Struct *P1;
    int usb_spend_time;
    int p_mask = (FPGA_ENGINE_P_MAX-1);
    int shuter_time, isp1_time;
    int hdr_level = get_A2K_Sensor_HDR_Level();     // 3,4,6
    int Now_Mode, S_Mode;
    int mode, res;
    int sensor_idx_tmp = -1;
    int Times;
    get_Stitching_Out(&mode, &res);
    if(res == 7)       Now_Mode = 1;			//8K
    else if(res == 12) Now_Mode = 2;			//6K
    else               Now_Mode = Img_Mode;
    usb_spend_time = get_USB_Spend_Time(Now_Mode);

    P1 = &F_Pipe.Sensor;
    P0_Idx = P1->Idx;
    P1->P[P0_Idx].No       = -1;
    P1->P[P0_Idx].Time_Buf = -1;
    P1->P[P0_Idx].I_Page   = -1;
    P1->P[P0_Idx].O_Page   = -1;
    P1->P[P0_Idx].M_Mode   = Img_Mode;
    P1->Idx =  (P1->Idx + 1) & p_mask;

    memcpy(&F_Temp, &F_Pipe, sizeof(F_Pipe) );
    F_Pipe_Add_Sub(-1, -1, -1, -1, -1, -1, &Sub_Idx, &Times);

    int stitch_time;

    if(AEB_Frame_Cnt < 0 || AEB_Frame_Cnt > 7){
        db_error("F_Pipe_Add_AEB: Variable Error! AEB_Frame_Cnt=%d\n", AEB_Frame_Cnt);
        return;
    }
    
    int isp1_page_start = F_Temp.ISP1.Page[0];
    int frame[7], integ[7], dgain[7], exp_n[7], exp_m[7], iso[7];
    int frameE, integE, dgainE, exp_nE, exp_mE, isoE;
    int istart = (7 - AEB_Frame_Cnt) >> 1;                      // 3:2, 5:1, 7:0
    int istop = AEB_Frame_Cnt - 1;
    int jpg_page[7], scale;

    for(i = 0; i < AEB_Frame_Cnt; i++){                         // AEB_Frame_Cnt=3,5,7
    	if(AEB_Auto_HDR_En == 1) {
    		if(i < (AEB_Frame_Cnt / 2) ) scale = HDR7P_Auto_P.EV_Inc[1] * 20;
    		else                         scale = HDR7P_Auto_P.EV_Inc[0] * 20;
    	}
    	else scale = AEB_AE_Scale;

        do_AEB_sensor_cal(i+istart, scale, C_Mode, &frame[i], &integ[i], &dgain[i], &exp_n[i], &exp_m[i], &iso[i]);

        shuter_time = F_Com_In.Shuter_Speed * (frame[i]+1);
        if(i == istop) isp1_time = F_Com_In.Pipe_Time_Base * 2;                   // 最後一張, ISP1時間延長
        else           isp1_time = F_Com_In.Pipe_Time_Base;

        //[].Sensor, 33=不執行AE調整=-1
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 1, 1, 2, shuter_time, &Sub_Idx, &Times);
        set_F_Temp_Sensor_SubData(Sub_Idx, PIPE_SUBCODE_AEB_STEP_0+i, -1, 0, frame[i], integ[i], dgain[i]);     // AEB

        //[].ISP1
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 1, 2, 3, isp1_time, &Sub_Idx, &Times);         // AEB
        set_F_Temp_ISP1_SubData(Sub_Idx, PIPE_SUBCODE_AEB_STEP_0+i, AEB_Frame_Cnt, Now_Mode, -1, -1, 1);  // AEB
        sensor_idx_tmp = Sub_Idx;
        // Jpeg Header
        Set_F_JPEG_Header_ISP1(Sub_Idx, &jpg_page[i]);
        // WDR Command
        set_F_Temp_WDR_SubData(Sub_Idx, -1, -1, -1);

        F_Temp.Sensor.Page[0] = (F_Temp.Sensor.Page[0]+1) % 2;
        F_Temp.ISP1.Page[0]   = (F_Temp.ISP1.Page[0]+1) % 3;
        F_Temp.Pic_No++;        // debug used
    }

    get_AEG_EP_Var(&frameE, &integE, &dgainE);
    for(i = 0; i < 2; i++) {				//解拍攝完Live畫面亮度錯誤問題
    	Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 1, 1, 2, F_Com_In.Shuter_Speed, &Sub_Idx, &Times);     // HDR.Sensor, 恢復原狀, 拍完不閃爍
    	set_F_Temp_Sensor_SubData(Sub_Idx, PIPE_SUBCODE_AEB_STEP_E, -1, 0, frameE, integE, dgainE);
    }
    
    F_Temp.ISP1.Page[0] = isp1_page_start;
    F_Temp.DMA.Page[0] = (F_Temp.Stitch.Page[0] + 1) % 2;

	int st_p2 = 5, dma_p2 = 9, jpeg_p0 = 4, h264_p0 = 4;
	int dma_t = 0, dma_mode = -1;
	if(Now_Mode == 1 || Now_Mode == 2) { // ISP2(3) -> ST(4) -> DMA(6) -> JPG(5) -> USB(7)
		st_p2 = 6; dma_p2 = 5; jpeg_p0 = 6; h264_p0 = 6;
		dma_t = FPGA_Speed[Img_Mode].DMA; dma_mode = 2;
	}
	else {                              // ISP2(3) -> ST(4) -> JPG(5) -> USB(7)
		st_p2 = 5; dma_p2 = 9; jpeg_p0 = 4; h264_p0 = 4;
		dma_t = 0; dma_mode = -1;
	}
    set_AEB_ST_Enable(1);
    for(i = 0; i < AEB_Frame_Cnt; i++){   // 再慢慢壓縮
        //[].ISP2, 0~3 ISP2_NR3D_Rate
        if(i == 0) Check_Flag &= F_Pipe_Add_Sub(Img_Mode, 0, 2, 3, 4, FPGA_Speed[Img_Mode].ISP2, &Sub_Idx, &Times);
        else       Check_Flag &= F_Pipe_Add_Sub(Img_Mode, 0, 4, 3, 4, FPGA_Speed[Img_Mode].ISP2, &Sub_Idx, &Times);
        set_F_Temp_ISP2_SubData(Sub_Idx, 0, PIPE_SUBCODE_AEB_STEP_0+i, AEB_Frame_Cnt, -1);              // AEB, aeb_idx=i
        //[].Stitch
        if(i < 2) stitch_time = FPGA_Speed[Img_Mode].Stitch_ZYUV * 3 + 500000;
        else      stitch_time = FPGA_Speed[Img_Mode].Stitch_ZYUV * 3 + 1200000;
        //stitch_time += 500000;
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode, 0, 3, 4, st_p2, stitch_time, &Sub_Idx, &Times);  // time=0.5s~0.8s
        set_F_Temp_Stitch_SubData(Sub_Idx, -1, -1, 1, i+1, 1);                          // AEB, 色縫合, st_step=1, yuv_ddr=1+i, hdr_7idx=1
        //[].Smooth
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode, 0, 4, 9, 10, 0, &Sub_Idx, &Times);
        set_F_Temp_Smooth_SubData(Sub_Idx, -1);
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode, 0, 3, 4, st_p2, FPGA_Speed[Img_Mode].Stitch_Img, &Sub_Idx, &Times);
        set_F_Temp_Stitch_SubData(Sub_Idx, -1, -1, 2, i+1, 1);                          // AEB, 大圖縫合, st_step=2, yuv_ddr=1+i, hdr_7idx=1
        //[].Smooth
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode, 0, 4, 9, 10, 0, &Sub_Idx, &Times);
        set_F_Temp_Smooth_SubData(Sub_Idx, -1);

        Check_Flag &= F_Pipe_Add_Sub(Now_Mode,0, 4, 6, dma_p2, dma_t, &Sub_Idx, &Times);                        //[].DMA
        set_F_Temp_DMA_SubData(Sub_Idx, -1, -1, -1, -1, dma_mode);
        Check_Flag &= F_Pipe_Add_Sub(Now_Mode,0, jpeg_p0, 5, 7, FPGA_Speed[Now_Mode].Jpeg, &Sub_Idx, &Times);   //[].Jpeg
        int aeb_f_mid = (AEB_Frame_Cnt >> 1);
        int aeg_idx = (i + aeb_f_mid) % AEB_Frame_Cnt;
        
        set_F_Temp_Jpeg_SubData(Sub_Idx, F_Temp.Jpeg_Id, -1, -1, i+1, jpg_page[i], exp_n[aeg_idx], exp_m[aeg_idx], iso[aeg_idx]);                     // hdr_3s=-1
        Check_Flag &= F_Pipe_Add_Sub(Now_Mode ,0, h264_p0, 8, 7, 0, &Sub_Idx, &Times);							//[].H264
        set_F_Temp_H264_SubData(Sub_Idx, 0, 0, 0, 0);
        if(usb_spend_time > 1500000) usb_spend_time = 1500000;      // rex+ 180528, 解USB Times>=17,19 會誤動作
        Check_Flag &= F_Pipe_Add_Sub(Now_Mode,0, 5, 7, 10, usb_spend_time, &Sub_Idx, &Times);             	    //[].USB
        set_F_Temp_USB_SubData(Sub_Idx, F_Com_In.encode_type, 1);
        
        F_Temp.ISP1.Page[0]   = (F_Temp.ISP1.Page[0]+1) % 3;
        F_Temp.ISP2.Page[0]   = (F_Temp.ISP2.Page[0]+1) % 2;
        if(Now_Mode != 1 && Now_Mode != 2) {
            F_Temp.Stitch.Page[0] = (F_Temp.Stitch.Page[0]+1) % 2;
            F_Temp.Smooth.Page[0] = (F_Temp.Smooth.Page[0]+1) % 2;
        }
        F_Temp.Jpeg[F_Temp.Jpeg_Id].Page[0]   = (F_Temp.Jpeg[F_Temp.Jpeg_Id].Page[0]+1) % 2;
        F_Temp.Jpeg_Id   = (F_Temp.Jpeg_Id+1) % 2;
        //F_Temp.DMA.Page[0]    = (F_Temp.DMA.Page[0]+1) % 2;
        F_Temp.USB.Page[0]    = (F_Temp.USB.Page[0]+1) % 2;
    }
    F_Temp.Jpeg[F_Temp.Jpeg_Id].Page[0]   = (F_Temp.Jpeg[F_Temp.Jpeg_Id].Page[0]+1) % 2;		//解JPEG做奇數次, 轉回小圖後JPEG引擎會用錯
    F_Temp.Jpeg_Id   = (F_Temp.Jpeg_Id+1) % 2;

    if(Check_Flag == 1){
        *Finish_Idx = Sub_Idx;
        *Sensor_Idx = sensor_idx_tmp;
        db_debug("F_Pipe_Add_AEB: Finish_Idx=%d\n", *Finish_Idx);
    }
    else{
        db_error("F_Pipe_Add_AEB: err! Check_Flag=%d\n", Check_Flag);
    }
    F_Pipe_Add_Sub(-2, -2, -2, -2, -2, -2, &Sub_Idx, &Times);
    Make_F_Pipe( Img_Mode, Check_Flag, C_Mode);                 // F_Pipe_Add_AEB
    return Check_Flag;
};
/* 
 * rex+ 190402
 *    HDR 3、5、7張合成1P
 */
int HDR7P_Manual = 0;			  // 0:手動關  1:手動開  2:Auto
int HDR7P_Frame_Cnt = 3;          // 3,5,7
int HDR7P_AE_Scale = 20;          // 1.0x20=20,1.5x20=30,2.0x20=40,2.5x20=50, 20(AE+-1.0),30(AE+-1.5),40(AE+-2.0),50(AE+-2.5)
int HDR7P_Strength = 60;         // 100%,75%,50%

//int HDR7P_Auto_Strength = 60;
int HDR7P_Auto_Th[2][2] = { {7000, 3000}, {10000, 3000} };				//[CapHDR/NightHDR][+0/-5]
int HDR7P_Auto_Target[2][2] = { {24, 192}, {20, 192} };					//[CapHDR/NightHDR][+0/-5]
HDR7P_Auto_Parameter_Struct HDR7P_Auto_P;
HDR7P_AEG_Parameter_Struct HDR7P_AEG_Parameter[7];

int Removal_HDR_Mode = 0;			  	// 0:關 1:自動 2:弱 3:中 4:強 5:自訂
int Removal_HDR_Frame_Cnt = 5;          // 3,5,7
int Removal_HDR_AE_Scale = 30;          // 1.0x20=20,1.5x20=30,2.0x20=40,2.5x20=50, 20(AE+-1.0),30(AE+-1.5),40(AE+-2.0),50(AE+-2.5)
int Removal_HDR_Strength = 60;         // 100%,75%,50%

void Set_HDR7P_AEG_Idx(int idx, int aeg_idx, int aeg_max, int aeg_min, int frame, int integ, int gain)
{
	int ep_ln_max;
	int tmp_idx = aeg_idx;
	int sys_fps = getFPS();

	ep_ln_max = get_ep_ln_maximum(sys_fps);
	if(tmp_idx >= aeg_max) tmp_idx = aeg_max;
	if(tmp_idx <= aeg_min) tmp_idx = aeg_min;

	HDR7P_AEG_Parameter[idx].aeg_idx = tmp_idx;
	HDR7P_AEG_Parameter[idx].frame   = frame;
	HDR7P_AEG_Parameter[idx].integ   = integ;
	HDR7P_AEG_Parameter[idx].gain    = gain;
	HDR7P_AEG_Parameter[idx].ep_line = (frame+1) * ep_ln_max - integ;
//db_debug("Set_HDR7P_AEG_Idx() idx=%d aeg_idx=%d max=%d min=%d HDR7P_AEG_Idx=%d frame=%d integ=%d gain=%d line=%d\n",
//		idx, aeg_idx, aeg_max, aeg_min, tmp_idx, frame, integ, gain, HDR7P_AEG_Parameter[idx].ep_line);
}
HDR7P_AEG_Parameter_Struct Get_HDR7P_AEG_Idx(int idx)
{
	return HDR7P_AEG_Parameter[idx];
}

void SetHDR7PAutoTh(int idx, int value) {
	int c_mode = F_Com_In.Record_Mode;
	if(c_mode == 7) HDR7P_Auto_Th[1][idx&0x1] = value;
	else			HDR7P_Auto_Th[0][idx&0x1] = value;
}
int GetHDR7PAutoTh(int idx) {
	int value;
	int c_mode = F_Com_In.Record_Mode;
	if(c_mode == 7) value = HDR7P_Auto_Th[1][idx&0x1];
	else			value = HDR7P_Auto_Th[0][idx&0x1];
	return value;
}
void SetHDR7PAutoTarget(int idx, int value) {
	int c_mode = F_Com_In.Record_Mode;
	if(c_mode == 7) HDR7P_Auto_Target[1][idx&0x1] = value;
	else			HDR7P_Auto_Target[0][idx&0x1] = value;
}
int GetHDR7PAutoTarget(int idx) {
	int value;
	int c_mode = F_Com_In.Record_Mode;
	if(c_mode == 7) value = HDR7P_Auto_Target[1][idx&0x1];
	else			value = HDR7P_Auto_Target[0][idx&0x1];
	return value;
}

int Check_Is_HDR_Auto(void)
{
	int c_mode = F_Com_In.Record_Mode;
	if(c_mode == 3){
		if(AEB_Auto_HDR_En == 1) return 1;
	}
	else if(c_mode == 5 || c_mode == 7) {
		if(HDR7P_Manual == 2) return 1;
	}
	else if(c_mode == 13) {
		if(Removal_HDR_Mode == 1) return 1;
	}
	return 0;
}

int Check_Is_HDR_Mode(int c_mode)
{
	if(c_mode == 5 || c_mode == 7 || c_mode == 13)
		return 1;
	else
		return 0;
}

void HDR7P_Auto_Init(int c_mode)
{
	int i;
	if(Check_Is_HDR_Auto() == 1) {
		HDR7P_Auto_P.Step = 1;
		for(i = 0; i < 2; i++) {
			HDR7P_Auto_P.ISP1_Idx[i] = -1;
			if(i == 0) HDR7P_Auto_P.EV_Inc_T[i] = 0;
			else	   HDR7P_Auto_P.EV_Inc_T[i] = -5;
			HDR7P_Auto_P.Rate[i] = 0;
			HDR7P_Auto_P.EV_Inc[i] = 0;
			HDR7P_Auto_P.Sensor_Y_Total[i] = 0;
		}
		HDR7P_Auto_P.Shuter_Time = 100000;
		memset(&HDR7P_Auto_P.Sensor_Y[0][0], 0, sizeof(HDR7P_Auto_P.Sensor_Y) );
		memset(&HDR7P_Auto_P.Sensor_Y_Sum[0][0], 0, sizeof(HDR7P_Auto_P.Sensor_Y_Sum) );
	}
	else
		HDR7P_Auto_P.Step = 0;
}

int Get_HDR7P_Auto_Step(void) {
	return HDR7P_Auto_P.Step;
}

/*
 * 判斷上下限, 算出實際的目標亮度EV值
 */
void Set_HDR7P_Auto_EV_Inc_T(int now_aeg_idx, int aeg_idx, int aeg_max, int aeg_min)
{
	int idx = 0, now_idx = now_aeg_idx, t_idx = aeg_idx;
	int step = HDR7P_Auto_P.Step;
	if(/*step == 2 ||*/ step == 4) {
		if(step == 2)     idx = 0;
		else if(step = 4) idx = 1;
		if(t_idx >= aeg_max) t_idx = aeg_max;
		if(t_idx <= aeg_min) t_idx = aeg_min;
		if(t_idx >= now_idx) HDR7P_Auto_P.EV_Inc_T[idx] =  ((float)t_idx - (float)now_idx) / 1280.0;
		else				 HDR7P_Auto_P.EV_Inc_T[idx] = -((float)now_idx - (float)t_idx) / 1280.0;
//db_debug("Set_HDR7P_Auto_EV_Inc_T() idx=%d now_idx=%d t_idx=%d max=%d min=%d EV_Inc_T=%f\n",
//		idx, now_aeg_idx, aeg_idx, aeg_max, aeg_min, HDR7P_Auto_P.EV_Inc_T[idx]);
	}
}


/*
 * 檢查FPGA已經執行完, 且可以讀取Sensor統計值
 */
int Get_HDR7P_Auto_Read_En(void) {
	return HDR7P_Auto_P.R_En;
}
int Get_HDR7P_Auto_Add_Idx(void) {
	int add_idx = 4;
	add_idx = (HDR7P_Auto_P.Shuter_Time * 2) / F_Com_In.Pipe_Time_Base;
	if(add_idx < 0) add_idx = 0;
	return add_idx;
}
unsigned short Sensor_Y[5][256];
void Check_HDR7P_Auto_Read_En(int step, int hw_idx, int c_mode)
{
	int isp1_idx, t_idx;
	int tmp;
	unsigned long long now_time, timeout;
	unsigned long long TsumM=0, TsumP=0;

	if(HDR7P_Auto_P.R_En == 0 && (/*step == 2 ||*/ step == 4) ) {
		/*if(step == 2) isp1_idx = HDR7P_Auto_P.ISP1_Idx[0];
		else*/		  isp1_idx = HDR7P_Auto_P.ISP1_Idx[1];
		if(isp1_idx != -1) {
//tmp			get_current_usec(&now_time);
			int add_idx = Get_HDR7P_Auto_Add_Idx();
			
			t_idx = (isp1_idx + add_idx) & (FPGA_ENGINE_P_MAX-1);
			tmp = (hw_idx - t_idx) & (FPGA_ENGINE_P_MAX-1);
			timeout = (now_time - HDR7P_Auto_P.R_Time);
			if(tmp < (FPGA_ENGINE_P_MAX>>1) || timeout > 10000000) {
				HDR7P_Auto_P.R_En = 1;
				db_debug("Check_HDR7P_Auto_Read_En: step=%d hw_idx=%d isp1_idx=%d t_idx=%d add_idx=%d tmp=%d time=%lld\n",
						step, hw_idx, isp1_idx, t_idx, add_idx, tmp, timeout);

				read_Sensor_Y(&Sensor_Y[0][0], &TsumM, &TsumP);
				Copy_to_HDR7P_Sensor_Y(&Sensor_Y[0][0], c_mode);
			}
		}
	}
}

/*
 * 由Sensor統計值, 計算+-EV值
 * EV -5: 由後面開始累加, 直到超過門檻
 * EV +0: 由前面開始累加, 直到超過門檻
 */
void Cal_HDR7P_Auto_Proc(int idx, int c_mode)
{
	int i, i2;
	int ev, th;
	unsigned target, t_idx;
	unsigned sum;
	float r1, r2, r3;
	float pow_tmp;

	sum = 0;
	ev = HDR7P_Auto_P.EV_Inc_T[idx];
	if(c_mode == 7) {		//Night
		th = HDR7P_Auto_Target[1][idx];
		target = HDR7P_Auto_Th[1][idx];
	}
	else {
		th = HDR7P_Auto_Target[0][idx];
		target = HDR7P_Auto_Th[0][idx];
	}
	for(i = 0; i < 256; i++) {				//亮度0~255, 尋找超過門檻的位置
		if(idx == 0) i2 = i;
		else		 i2 = 255 - i;
		sum += HDR7P_Auto_P.Sensor_Y_Sum[idx][i2];
		if(sum >= target) {
			t_idx = i2+1;
			break;
		}
	}

	if(idx == 0) {		//EV: +0
		if(t_idx < 2) t_idx = 2;
		pow_tmp = pow(2, ev);
		r1 = pow_tmp * (float)th / (float)t_idx;
		r2 = log2f(r1);
	}
	else {				//EV: -5
		if(t_idx < 4) t_idx = 4;
		pow_tmp = 1 / pow(2, ev);
		r1 = pow_tmp * (float)t_idx / (float)th;
		r2 = log2f(r1);
	}
	if(r2 < 0) r2 = 0;

	//確認AEG上下限
	/*int c_mode = F_Com_In.Record_Mode;
	int aeg_idx, aeg_live, aeg_max, aeg_min, gain_max;
	int exp_max = (c_mode == 7)? (120*64): (60*64);

	get_PipeCmd_AEG_idx(&aeg_live, &aeg_max, &aeg_min, &gain_max);
	if(idx == 0) aeg_idx = aeg_live - (r2*20*64);			//EV: -5
	else		 aeg_idx = aeg_live + (r2*20*64);			//EV: +0

	exp_max += (40*64);
	aeg_max = exp_max + gain_max;
	if(aeg_idx > aeg_max) aeg_idx = aeg_max;
	if(aeg_idx < aeg_min) aeg_idx = aeg_min;
	if(aeg_idx >= aeg_live) r3 =  ((float)aeg_idx - (float)aeg_live) / 1280.0;
	else				 	r3 = -((float)aeg_live - (float)aeg_idx) / 1280.0;*/

	HDR7P_Auto_P.Rate[idx] = r2;

db_debug("Cal_HDR7P_Auto_Proc() idx=%d ev=%d total=%d target=%d th=%d t_idx=%d r1=%f r2=%f r3=%f pow=%f\n",
	idx, ev, HDR7P_Auto_P.Sensor_Y_Total[idx], target, th, t_idx, r1, r2, r3, pow_tmp);
}

/*
 * 由計算結果, 設定HDR參數
 */
void Set_HDR7P_Auto_Parameter(int c_mode)
{
	int cnt;
	float rate;

	if(HDR7P_Auto_P.Rate[0] > HDR7P_Auto_P.Rate[1])
		rate = HDR7P_Auto_P.Rate[0];
	else
		rate = HDR7P_Auto_P.Rate[1];

	if(c_mode == 13) {									//Removal
		Removal_HDR_Frame_Cnt = 5;
		//Removal_HDR_Strength = HDR7P_Auto_Strength;
		cnt = (Removal_HDR_Frame_Cnt >> 1);
	}
	else if(c_mode == 3){								//AEB
		//if(rate < 1)      AEB_Frame_Cnt = 3;
		//else if(rate < 2) AEB_Frame_Cnt = 5;
		//else	            AEB_Frame_Cnt = 7;
		cnt = (AEB_Frame_Cnt >> 1);
	}
	else if(c_mode == 7){								//NightHDR
		if(rate < 1)      HDR7P_Frame_Cnt = 3;
		else			  HDR7P_Frame_Cnt = 5;
		//HDR7P_Strength = HDR7P_Auto_Strength;
		cnt = (HDR7P_Frame_Cnt >> 1);
	}
	else {
		if(rate < 1)      HDR7P_Frame_Cnt = 3;
		else if(rate < 2) HDR7P_Frame_Cnt = 5;
		else			  HDR7P_Frame_Cnt = 7;
		//HDR7P_Strength = HDR7P_Auto_Strength;
		cnt = (HDR7P_Frame_Cnt >> 1);
	}

	HDR7P_Auto_P.EV_Inc[0] = HDR7P_Auto_P.Rate[0] / (float)cnt;
	if(HDR7P_Auto_P.EV_Inc[0] < 0.5) HDR7P_Auto_P.EV_Inc[0] = 0.5;
	HDR7P_Auto_P.EV_Inc[1] = HDR7P_Auto_P.Rate[1] / (float)cnt;
	if(HDR7P_Auto_P.EV_Inc[1] < 0.5) HDR7P_Auto_P.EV_Inc[1] = 0.5;

db_debug("Set_HDR7P_Auto_Parameter() cnt=%d EV_Inc[0]=%f EV_Inc[1]=%f Strength=%d\n",
		HDR7P_Frame_Cnt, HDR7P_Auto_P.EV_Inc[0], HDR7P_Auto_P.EV_Inc[1], HDR7P_Strength);
}

/*
 * 讀取Sensor統計值到Buf
 */
void Copy_to_HDR7P_Sensor_Y(unsigned short *table, int c_mode)
{
	int i, j, idx;
	db_debug("Copy_to_HDR7P_Sensor_Y: Step=%d\n", HDR7P_Auto_P.Step);

	if(HDR7P_Auto_P.Step <= 2) idx = 0;		//step = 2, EV+0
	else					   idx = 1;		//step = 4, EV-5
	memcpy(&HDR7P_Auto_P.Sensor_Y[0][0], table, sizeof(HDR7P_Auto_P.Sensor_Y) );
	memset(&HDR7P_Auto_P.Sensor_Y_Sum[idx][0], 0, sizeof(HDR7P_Auto_P.Sensor_Y_Sum[idx]) );
	HDR7P_Auto_P.Sensor_Y_Total[idx] = 0;
	for(i = 0; i < 5; i++) {
		for(j = 0; j < 256; j++) {
			HDR7P_Auto_P.Sensor_Y_Sum[idx][j] += HDR7P_Auto_P.Sensor_Y[i][j];
			HDR7P_Auto_P.Sensor_Y_Total[idx] += HDR7P_Auto_P.Sensor_Y[i][j];
		}
	}
	HDR7P_Auto_P.Step++;
	HDR7P_Auto_P.R_En = 0;

	//for(i = 0; i < 256; i++)
	//	db_debug("Copy_to_HDR7P_Sensor_Y() idx=%d i=%3d Sum=%d\n", idx, i, HDR7P_Auto_P.Sensor_Y_Sum[idx][i]);

	Cal_HDR7P_Auto_Proc(idx, c_mode);
}

/*
 * 預估執行時間
 */
int Cap_Add_Time = -1, Cap_Add_Time2 = 0;
int Cap_Ep_Time = -1, Cap_Ep_Time2 = 0;
void setCaptureAddTime(int start_idx, int end_idx, int sensor_idx, int c_mode, int i_mode)
{
	extern void getLiveShowValue(int *value);               // fpga_driver.c
	extern int getCapturePrepareTime(void);
    //i_mode = F_Com_In.Big_Mode;
    //c_mode = F_Com_In.Record_Mode;
    int usb_t = get_USB_Spend_Time(i_mode) / 100000;        // 1us -> 100ms
	int add_time = 0;

	if(start_idx == -1 || end_idx == -1) {
		Cap_Add_Time = -1;
		Cap_Ep_Time = -1;
		Cap_Add_Time2 = 0;
		Cap_Ep_Time2 = 0;
	}
	else {
		int re = 0;
		if(end_idx > start_idx){
			re = end_idx - start_idx;
		}else{
			re = FPGA_ENGINE_P_MAX - 1 + end_idx - start_idx;
		}
		if(c_mode == 6 || c_mode == 7){
			re = re * 2;
		}
		Cap_Add_Time = re + usb_t;          // unit: 100ms
		Cap_Add_Time2 = re + usb_t;

		/*
		 *(value + 0) = Live_EXP_N;     // 分子
		 *(value + 1) = Live_EXP_M;     // 分母
		 *(value + 2) = Live_ISO;
		 */
		int value[3], long_t=10;

		re = 0;
		//add_time = getCapturePrepareTime();
		if(c_mode == 13) add_time = 20;		//Removal 多2s, 為了讓APP有時間收到新的曝光時間
		else			 add_time = 0;
		if(sensor_idx > 0){
			if(sensor_idx > start_idx){
				re = sensor_idx - start_idx;
			}else{
				re = FPGA_ENGINE_P_MAX - 1 + sensor_idx - start_idx;
			}
			if(c_mode == 6 || c_mode == 7){     // 夜間200ms
				re = re * 2;
			}
			getLiveShowValue(value);
			if(value[1] == 1){                  // >= 1s, 長時間曝光
				long_t = value[0] * 10;
				if(long_t > re) re = long_t;
			}
			
			Cap_Ep_Time = re + add_time;		//APP
			Cap_Ep_Time2 = re + add_time;		//OLED
		}
	}

	db_debug("setCaptureAddTime start:%d, end:%d, sensor:%d, cap_time:%d, ep_time:%d add_time:%d\n",
			start_idx, end_idx, sensor_idx, Cap_Add_Time, Cap_Ep_Time, add_time);
}
/*
 * 整張處理含曝光時間, APP使用
 */
int getCaptureAddTime(void){
	int re = -1;
	if(Cap_Add_Time != -1){
		re = Cap_Add_Time;
		Cap_Add_Time = -1;
	}
	return re;
}
/*
 * 曝光時間, APP使用
 */
int getCaptureEpTime(void){
	int re = -1;
	if(Cap_Ep_Time != -1){
		re = Cap_Ep_Time;
		Cap_Ep_Time = -1;
	}
	return re;
}
/*
 * 曝光時間, OLED使用
 */
int getCaptureEpTime2(void)
{
    int re = 0;
    if(Cap_Ep_Time2 > 0){
        re = Cap_Ep_Time2;
        Cap_Ep_Time2 = 0;               // n*100ms
    }
    return re;
}
int getCap_Add_Time2(void)
{
    int re = 0;
    if(Cap_Add_Time2 > 0){
        re = Cap_Add_Time2;
        Cap_Add_Time2 = 0;
    }
    return re;
}
/*
 * 拍照前準備時間, [Auto HDR]約需多花2-4秒
 */
int getCapturePrepareTime(void)
{
    int c_mode = F_Com_In.Record_Mode;
    int par_T=10, add_time=15;
    if(Check_Is_HDR_Auto() == 1){
    	par_T = 10 + add_time;
        if(c_mode == 7) par_T *= 2;		// N-HDR
    }
    return(par_T);         // n*100ms
}
/*
 * HDR7P Step Control
 */
int HDR7P_Auto_Proc(int c_mode)
{
	int isp1_idx;
	int shuter_time;
	int ts_max=0;
	static int live_cnt = 0, add_idx = 0;
	unsigned long long TsumM, TsumP;
	switch(HDR7P_Auto_P.Step) {
	case 1:
	//case 2:
	case 3:
		if(HDR7P_Auto_P.Step == 1) {	//+0, 直接讀取Live的值
			read_Sensor_Y(&Sensor_Y[0][0], &TsumM, &TsumP);
			Copy_to_HDR7P_Sensor_Y(&Sensor_Y[0][0], c_mode);
			HDR7P_Auto_P.Step = 3;
		}

		F_Pipe_Add_Small(F_Com_In.Small_Mode, F_Com_In.Record_Mode, HDR7P_Auto_P.Step, &isp1_idx, &shuter_time, &ts_max);		//-5
		if(HDR7P_Auto_P.ISP1_Idx[1] == -1) {
			HDR7P_Auto_P.ISP1_Idx[1] = isp1_idx;
			HDR7P_Auto_P.Shuter_Time = shuter_time;
			HDR7P_Auto_P.R_En = 0;
//tmp			get_current_usec(&HDR7P_Auto_P.R_Time);
			if(ts_max > 1) add_idx = Get_HDR7P_Auto_Add_Idx();
			else		   add_idx = -1;
			HDR7P_Auto_P.Step = 4;
		}
		live_cnt = 0;
		break;
	case 4:
		if(HDR7P_Auto_P.Step == 4 && add_idx != -1) {
			if(add_idx <= 0) break;
			else			 add_idx--;
		}
		F_Pipe_Add_Small(F_Com_In.Small_Mode, F_Com_In.Record_Mode, HDR7P_Auto_P.Step, &isp1_idx, &shuter_time, &ts_max);
		break;
	case 5:		//live
		F_Pipe_Add_Small(F_Com_In.Small_Mode, F_Com_In.Record_Mode, HDR7P_Auto_P.Step, &isp1_idx, &shuter_time, &ts_max);
		live_cnt++;
		if(live_cnt >= 3){
			Set_HDR7P_Auto_Parameter(c_mode);
			HDR7P_Auto_P.Step = 0;
		}
		add_idx = 0;
		break;
	}

	return ts_max;
}

int F_Pipe_Add_HDR7P(int Img_Mode, int *Finish_Idx, int *Sensor_Idx, int C_Mode)
{
    int i, P0_Idx; struct FPGA_Engine_Resource_Struct *P1;
    int Check_Flag = 1, Sub_Idx;
    
    int usb_spend_time;
    int p_mask = (FPGA_ENGINE_P_MAX-1);
    int hdr_level = 4;
    int shuter_time, isp1_time;
    int thm_mode = 5;
    int btm_m=0, btm_s=100, cp_mode=0, btm_t_m=0; //btm_m -> 0:關, 1:延伸, 2:加底圖(default), 3:鏡像, 4:加底圖(user)
    int jid = F_Temp.Jpeg_Id;
    int scale;
    int Now_Mode, S_Mode;
    int mode, res;
    int sensor_idx_tmp = -1;
    int Times;
    get_Stitching_Out(&mode, &res);
    if(res == 7)       Now_Mode = 1;			//8K
    else if(res == 12) Now_Mode = 2;			//6K
    else               Now_Mode = Img_Mode;
    usb_spend_time = get_USB_Spend_Time(Now_Mode);
#ifdef ANDROID_CODE
    get_A2K_DMA_BottomMode(&btm_m, &btm_s);
    get_A2K_DMA_BottomTextMode(&btm_t_m);
    get_A2K_DMA_CameraPosition(&cp_mode);
    hdr_level = get_A2K_Sensor_HDR_Level();     // 3,4,6
#endif
    P1 = &F_Pipe.Sensor;
    P0_Idx = P1->Idx;
    P1->P[P0_Idx].No       = -1;
    P1->P[P0_Idx].Time_Buf = -1;
    P1->P[P0_Idx].I_Page   = -1;
    P1->P[P0_Idx].O_Page   = -1;
    P1->P[P0_Idx].M_Mode   = Img_Mode;
    P1->Idx =  (P1->Idx + 1) & p_mask;

    db_debug("F_Pipe_Add_WDR: Img_Mode=%d\n", Img_Mode);

    memcpy(&F_Temp, &F_Pipe, sizeof(F_Pipe) );
    F_Pipe_Add_Sub(-1, -1, -1, -1, -1, -1, &Sub_Idx, &Times);
    if(btm_s != 0) F_Temp.DMA.Page[0] = F_Temp.Stitch.Page[0];  // rex+ 180626, 解DMA錯誤，拍成前一張畫面

    int isp1_page_start = F_Temp.ISP1.Page[0];
    int frame, integ, dgain, exp_n, exp_m, iso;
    int istart = (7 - HDR7P_Frame_Cnt) >> 1;                    // 3:2, 5:1, 7:0
    int istop = HDR7P_Frame_Cnt - 1;
    int jtbl_idx;

    for(i = 0; i < HDR7P_Frame_Cnt; i++){     // 先3張到ISP1 Buffer
    	if(HDR7P_Manual == 2) {
    		if(i < (HDR7P_Frame_Cnt / 2) ) scale = HDR7P_Auto_P.EV_Inc[1] * 20;
    		else                           scale = HDR7P_Auto_P.EV_Inc[0] * 20;
    	}
    	else scale = HDR7P_AE_Scale;

    	do_AEB_sensor_cal(i+istart, scale, C_Mode, &frame, &integ, &dgain, &exp_n, &exp_m, &iso);

        shuter_time = F_Com_In.Shuter_Speed * (frame+1);
        //[].Sensor, 33=不執行AE調整=-1
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 1, 1, 2, shuter_time, &Sub_Idx, &Times);               // HDR7P.Sensor, 做3-7次
        set_F_Temp_Sensor_SubData(Sub_Idx, PIPE_SUBCODE_AEB_STEP_0+i, -1, 0, frame, integ, dgain);     // HDR7P

        if(i == istop) isp1_time = F_Com_In.Pipe_Time_Base * HDR7P_Frame_Cnt;                   // WDR cmd執行時間
        else           isp1_time = F_Com_In.Pipe_Time_Base;
        //[].ISP1
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 1, 2, 3, isp1_time, &Sub_Idx, &Times);                 // HDR7P.ISP1
        set_F_Temp_ISP1_SubData(Sub_Idx, PIPE_SUBCODE_AEB_STEP_0+i, HDR7P_Frame_Cnt, Img_Mode, -1, -1, 1);          // HDR7P
        sensor_idx_tmp = Sub_Idx;

        if(i == 0) Set_F_JPEG_Header_ISP1(Sub_Idx, &jtbl_idx);

        F_Temp.Sensor.Page[0] = (F_Temp.Sensor.Page[0]+1) % 2;
        F_Temp.ISP1.Page[0]   = (F_Temp.ISP1.Page[0]+1) % 3;
    }
    int s_idx = 1 + Sub_Idx;
    for(i = 0; i < HDR7P_Frame_Cnt; i++){
        //[].WDR
        set_F_Temp_WDR_SubData(((s_idx+i)&p_mask), PIPE_SUBCODE_AEB_STEP_0+i, -1, HDR7P_Frame_Cnt); // HDR7P
    }
    
    get_AEG_EP_Var(&frame, &integ, &dgain);
    for(i = 0; i < 2; i++) {				//解拍攝完Live畫面亮度錯誤問題
		Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 1, 1, 2, F_Com_In.Shuter_Speed, &Sub_Idx, &Times); // HDR7P.Sensor, 恢復原狀, 拍完不閃爍
		set_F_Temp_Sensor_SubData(Sub_Idx, PIPE_SUBCODE_AEB_STEP_E, -1, 0, frame, integ, dgain);
    }

    F_Temp.ISP1.Page[0] = isp1_page_start;
    for(i = 0; i < HDR7P_Frame_Cnt; i++){
        //[].ISP2, ISP2_NR3D_Rate=0
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 2, 3, 4, FPGA_Speed[Img_Mode].ISP2, &Sub_Idx, &Times);
        set_F_Temp_ISP2_SubData(Sub_Idx, 0, PIPE_SUBCODE_AEB_STEP_0+i, HDR7P_Frame_Cnt, -1);        // HDR7P.ISP2: nr3d=0, hdr_7idx=i, hdr_mode=3,5,7

        if(i < HDR7P_Frame_Cnt){
            F_Temp.ISP1.Page[0]   = (F_Temp.ISP1.Page[0]+1) % 3;
            F_Temp.ISP2.Page[0]   = (F_Temp.ISP2.Page[0]+1) % 2;
        }
        F_Temp.Pic_No++;        // debug used
    }

    int st_p2, dma_p0;
    int smooth_time;
    Get_Smooth_Page(F_Com_In.Smooth_En, Img_Mode, &st_p2, &dma_p0, &smooth_time);
    //[].Stitch
    Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 3, 4, st_p2, 700000, &Sub_Idx, &Times);            // 為了取得色縫合係數, time=0.5sec
    set_F_Temp_Stitch_SubData(Sub_Idx, -1, -1, 1, 1, 1);   // HDR7P, st_step=1, yuv_ddr=1, i_page=1
    //[].Smooth
    Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 4, 9, 6, 0, &Sub_Idx, &Times);
    set_F_Temp_Smooth_SubData(Sub_Idx, -1);
    //[].Stitch
    Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 3, 4, st_p2, FPGA_Speed[Img_Mode].Stitch_Img, &Sub_Idx, &Times);
    set_F_Temp_Stitch_SubData(Sub_Idx, -1, -1, 2, 1, 1);   // HDR7P, st_step=2, yuv_ddr=1, i_page=1
    //[].Smooth
    Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 4, 9, 6, smooth_time, &Sub_Idx, &Times);
    set_F_Temp_Smooth_SubData(Sub_Idx, F_Com_In.Smooth_En);

    int dma_time;
    switch(btm_m) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4: if(F_Com_In.Smooth_En == 1) F_Temp.DMA.Page[0] = F_Temp.Smooth.Page[0];
			else						F_Temp.DMA.Page[0] = F_Temp.Stitch.Page[0];
    		if(btm_m == 3) dma_time = FPGA_Speed[Img_Mode].DMA;
            else           dma_time = FPGA_Speed[5].DMA;
            Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, dma_p0, 6, 5, dma_time, &Sub_Idx, &Times);                  //[].DMA
            set_F_Temp_DMA_SubData(Sub_Idx, btm_m, btm_s, cp_mode, btm_t_m, 0);
            if(btm_t_m == 1 && (btm_m == 1 || btm_m == 2 || btm_m == 3) ) {
            	dma_time = FPGA_Speed[5].DMA;
                Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 6, 6, 5, dma_time, &Sub_Idx, &Times);              //[].DMA
                set_F_Temp_DMA_SubData(Sub_Idx, btm_m, btm_s, cp_mode, btm_t_m, 1);
            }
    	    break;
    }
    // ISP2(3) -> ST(4) -> DMA(6) -> JPG(5) -> USB(7)
    if(Now_Mode == 1 || Now_Mode == 2) {
    	if(F_Com_In.Smooth_En == 1) F_Temp.DMA.Page[0] = (F_Temp.Smooth.Page[0]+1) % 2;
    	else						F_Temp.DMA.Page[0] = (F_Temp.Stitch.Page[0]+1) % 2;
    	Check_Flag &= F_Pipe_Add_Sub(Now_Mode,0, dma_p0, 6, 5, FPGA_Speed[Img_Mode].DMA, &Sub_Idx, &Times);      //[].DMA
    	set_F_Temp_DMA_SubData(Sub_Idx, -1, -1, -1, -1, 2);
    }
    do_AEB_sensor_cal(3, HDR7P_AE_Scale, C_Mode, &frame, &integ, &dgain, &exp_n, &exp_m, &iso);
    Check_Flag &= F_Pipe_Add_Sub(Now_Mode,0, 6, 5, 7, FPGA_Speed[Now_Mode].Jpeg, &Sub_Idx, &Times);         //[].Jpeg, HDR7P
    set_F_Temp_Jpeg_SubData(Sub_Idx, F_Temp.Jpeg_Id, -1, -1, -1, jtbl_idx, exp_n, exp_m, iso);
    Check_Flag &= F_Pipe_Add_Sub(Now_Mode ,0, 4, 8, 7, 0, &Sub_Idx, &Times);						        //[].H264
    set_F_Temp_H264_SubData(Sub_Idx, 0, 0, 0, 0);
    Check_Flag &= F_Pipe_Add_Sub(Now_Mode,0, 5, 7, 10, usb_spend_time, &Sub_Idx, &Times);                    //[].USB
    set_F_Temp_USB_SubData(Sub_Idx, F_Com_In.encode_type, 1);
    F_Com_In.Capture_Lose_Page[0] = F_Temp.USB.P[Sub_Idx&(FPGA_ENGINE_P_MAX-1)].I_Page;

    F_Temp.Jpeg[jid].Page[0] = (F_Temp.Jpeg[jid].Page[0]+1) % 2;
    F_Temp.Jpeg_Id           = (F_Temp.Jpeg_Id+1) % 2;
    F_Temp.USB.Page[0]       = (F_Temp.USB.Page[0]+1) % 2;
    F_Temp.Pic_No++;

    //[].Jpeg
    Check_Flag &= F_Pipe_Add_Sub(Now_Mode,0, 7, 5, 7, FPGA_Speed[thm_mode].Jpeg, &Sub_Idx, &Times);
    set_F_Temp_Jpeg_SubData(Sub_Idx, F_Temp.Jpeg_Id, PIPE_SUBCODE_CAP_THM, -1, -1, jtbl_idx, -1, -1, -1);
    //[].H264
    Check_Flag &= F_Pipe_Add_Sub(Now_Mode ,0, 7, 8, 7, 0, &Sub_Idx, &Times);
    set_F_Temp_H264_SubData(Sub_Idx, 0, 0, 0, 0);
    //[].DMA
    Check_Flag &= F_Pipe_Add_Sub(Now_Mode,0, dma_p0, 6, 10,0, &Sub_Idx, &Times);
    set_F_Temp_DMA_SubData(Sub_Idx, -1, -1, -1, -1, -1);
    //[].USB
    Check_Flag &= F_Pipe_Add_Sub(Now_Mode,0, 5, 7, 10, get_USB_Spend_Time(thm_mode), &Sub_Idx, &Times);
    set_F_Temp_USB_SubData(Sub_Idx, F_Com_In.encode_type, 1);
    F_Com_In.Capture_Lose_Page[1] = F_Temp.USB.P[Sub_Idx&(FPGA_ENGINE_P_MAX-1)].I_Page;
    
    F_Temp.Sensor.Page[0]    = (F_Temp.Sensor.Page[0]+1) % 2;
    F_Temp.ISP1.Page[0]      = (F_Temp.ISP1.Page[0]+1) % 3;
    F_Temp.ISP2.Page[0]      = (F_Temp.ISP2.Page[0]+1) % 2;
    F_Temp.Stitch.Page[0]    = (F_Temp.Stitch.Page[0]+1) % 2;
    F_Temp.Smooth.Page[0]    = (F_Temp.Smooth.Page[0]+1) % 2;
    F_Temp.Jpeg[jid].Page[0] = (F_Temp.Jpeg[jid].Page[0]+1) % 2;
    F_Temp.Jpeg_Id           = (F_Temp.Jpeg_Id+1) % 2;
    F_Temp.DMA.Page[0]       = (F_Temp.DMA.Page[0]+1) % 2;
    F_Temp.USB.Page[0]       = (F_Temp.USB.Page[0]+1) % 2;

    if(Check_Flag == 1){
        *Finish_Idx = Sub_Idx;
        *Sensor_Idx = sensor_idx_tmp;
        db_debug("F_Pipe_Add_WDR: Finish_Idx=%d\n", *Finish_Idx);
    }
    else{
        db_error("F_Pipe_Add_WDR: err! Check_Flag=%d\n", Check_Flag);
    }
    F_Pipe_Add_Sub(-2, -2, -2, -2, -2, -2, &Sub_Idx, &Times);
    Make_F_Pipe( Now_Mode, Check_Flag, C_Mode);                 // F_Pipe_Add_HDR7P
    return Check_Flag;
};

/*
 * rex+ 190618
 */
typedef struct GT_pt_H
{
    int df      : 4;
    int y       : 12;
    int x       : 16;
} GT_pt;        // 4byte
typedef struct GT_link_struct_H
{
    GT_pt       pre;
    GT_pt       next;
} GT_link_struct;
typedef struct GT_link_header_H
{
    GT_pt       start;
    GT_pt       stop;
    int         count;
} GT_link_header;

GT_link_struct     GT_link[4][432][576];
GT_link_header     GT_head[0x10000];

int debug = 0;

void add_GT_link(int lnk, int df, int y, int x)
{
    int start_df, start_y, start_x, stop_df, stop_y, stop_x;

    if(lnk >= 0x10000){ db_error("add_GT_link: lnk=%d\n", lnk); return; }
    if(GT_head[lnk].count == 0){
        GT_head[lnk].count    = 1;
        GT_head[lnk].start.df = df;
        GT_head[lnk].start.y  = y;
        GT_head[lnk].start.x  = x;
        GT_head[lnk].stop.df  = df;
        GT_head[lnk].stop.y   = y;
        GT_head[lnk].stop.x   = x;

        GT_link[df][y][x].pre.df  = -1;
        GT_link[df][y][x].pre.y   = -1;
        GT_link[df][y][x].pre.x   = -1;
        GT_link[df][y][x].next.df = -1;
        GT_link[df][y][x].next.y  = -1;
        GT_link[df][y][x].next.x  = -1;
    }
    else{
        GT_head[lnk].count ++;

        stop_df = GT_head[lnk].stop.df;
        stop_y  = GT_head[lnk].stop.y;
        stop_x  = GT_head[lnk].stop.x;
        if(stop_df < 0 || stop_y < 0 || stop_x < 0){ 
            db_error("add_GT_link: lnk=%d df=%d y=%d x=%d\n", lnk, stop_df, stop_y, stop_x);
            return;
        }
        GT_link[stop_df][stop_y][stop_x].next.df = df;
        GT_link[stop_df][stop_y][stop_x].next.y  = y;
        GT_link[stop_df][stop_y][stop_x].next.x  = x;
        GT_link[stop_df][stop_y][stop_x].pre.df = -1;
        GT_link[stop_df][stop_y][stop_x].pre.y  = -1;
        GT_link[stop_df][stop_y][stop_x].pre.x  = -1;

        GT_link[df][y][x].pre.df = stop_df;
        GT_link[df][y][x].pre.y  = stop_y;
        GT_link[df][y][x].pre.x  = stop_x;
        GT_link[df][y][x].next.df = -1;
        GT_link[df][y][x].next.y  = -1;
        GT_link[df][y][x].next.x  = -1;

        GT_head[lnk].stop.df = df;
        GT_head[lnk].stop.y  = y;
        GT_head[lnk].stop.x  = x;
    }
}
void del_GT_link(int lnk, int df, int y, int x)
{
    int start_df, start_y, start_x;
    int stop_df, stop_y, stop_x;
    int pre_df, pre_y, pre_x;
    int next_df, next_y, next_x;

    if(GT_head[lnk].count <= 0){ db_debug("del_GT_link: lnk=%d count=%d \n", lnk, GT_head[lnk&0xffff].count); return; }
    GT_head[lnk].count --;
    if(GT_head[lnk].count > 0){
        start_df = GT_head[lnk].start.df;
        start_y  = GT_head[lnk].start.y;
        start_x  = GT_head[lnk].start.x;
        stop_df = GT_head[lnk].stop.df;
        stop_y  = GT_head[lnk].stop.y;
        stop_x  = GT_head[lnk].stop.x;
        if(df == start_df && y == start_y && x == start_x){
            GT_head[lnk].start.df = GT_link[df][y][x].next.df;
            GT_head[lnk].start.y  = GT_link[df][y][x].next.y;
            GT_head[lnk].start.x  = GT_link[df][y][x].next.x;
            GT_link[df][y][x].pre.df = -1;
            GT_link[df][y][x].pre.y  = -1;
            GT_link[df][y][x].pre.x  = -1;
        }
        else if(df = stop_df && y == stop_y && x == stop_x){
            GT_head[lnk].stop.df = GT_link[df][y][x].pre.df;
            GT_head[lnk].stop.y  = GT_link[df][y][x].pre.y;
            GT_head[lnk].stop.x  = GT_link[df][y][x].pre.x;
            GT_link[df][y][x].next.df = -1;
            GT_link[df][y][x].next.y  = -1;
            GT_link[df][y][x].next.x  = -1;
        }
        else{
            pre_df = GT_link[df][y][x].pre.df;
            pre_y  = GT_link[df][y][x].pre.y;
            pre_x  = GT_link[df][y][x].pre.x;
            next_df = GT_link[df][y][x].next.df;
            next_y  = GT_link[df][y][x].next.y;
            next_x  = GT_link[df][y][x].next.x;
            GT_link[pre_df][pre_y][pre_x].next.df = next_df;
            GT_link[pre_df][pre_y][pre_x].next.y  = next_y;
            GT_link[pre_df][pre_y][pre_x].next.x  = next_x; 
            GT_link[next_df][next_y][next_x].pre.df = pre_df;
            GT_link[next_df][next_y][next_x].pre.y  = pre_y;
            GT_link[next_df][next_y][next_x].pre.x  = pre_x;
            GT_link[df][y][x].pre.df = -1;
            GT_link[df][y][x].pre.y  = -1;
            GT_link[df][y][x].pre.x  = -1;
            GT_link[df][y][x].next.df = -1;
            GT_link[df][y][x].next.y  = -1;
            GT_link[df][y][x].next.x  = -1;
        }
    }
    else{
        GT_head[lnk].count    = 0;
        GT_head[lnk].start.df = -1;
        GT_head[lnk].start.y  = -1;
        GT_head[lnk].start.x  = -1;
        GT_head[lnk].stop.df  = -1;
        GT_head[lnk].stop.y   = -1;
        GT_head[lnk].stop.x   = -1;
        GT_link[df][y][x].pre.df = -1;
        GT_link[df][y][x].pre.y  = -1;
        GT_link[df][y][x].pre.x  = -1;
        GT_link[df][y][x].next.df = -1;
        GT_link[df][y][x].next.y  = -1;
        GT_link[df][y][x].next.x  = -1;
    }
}

int Removal_G_Number = 1;
short int Removal_G_Table[4][432][576];
int Removal_G_Count[0x10000];

void replace_GT_link(int n1, int n2)
{
    int cnt1 = GT_head[n1].count;
    int cnt2 = GT_head[n2].count;
    if(cnt1 <= 0 || cnt2 <= 0){ db_debug("replace_GT_link: n1=%d cnt1=%d n2=%d cnt2=%d\n", n1, cnt1, n2, cnt2); return; }
    int i, df, y, x, df2, y2, x2;
    df = GT_head[n1].start.df;
    y  = GT_head[n1].start.y;
    x  = GT_head[n1].start.x;

    for(i = 0; i < cnt1; i++){
        df2 = GT_link[df][y][x].next.df;
        y2  = GT_link[df][y][x].next.y;
        x2  = GT_link[df][y][x].next.x;
        del_GT_link(n1, df, y, x);
        add_GT_link(n2, df, y, x);
        df = df2;
        y  = y2;
        x  = x2;
    }
}
void get_GT_start(int lnk, int *df, int *y, int *x)
{
    int i, df2, y2, x2, df3, y3, x3;

    *df = GT_head[lnk].start.df;
    *y  = GT_head[lnk].start.y;
    *x  = GT_head[lnk].start.x;
}
void get_GT_next(int df, int y, int x, int *ndf, int *ny, int *nx)
{
    *ndf = GT_link[df][y][x].next.df;
    *ny  = GT_link[df][y][x].next.y;
    *nx  = GT_link[df][y][x].next.x;
}

void init_Removal_Group(void)
{
    memset(Removal_G_Table, 0, sizeof(Removal_G_Table));
    memset(Removal_G_Count, 0, sizeof(Removal_G_Count));
    memset(GT_link, 0, sizeof(GT_link));
    memset(GT_head, 0, sizeof(GT_head));
    Removal_G_Number = 1;
}
// 1. 擴散距離變數
int Removal_Distance = 3;	//5;	//3;
// 2. 擴散斜率變數
int Removal_Divide = 3;		//2;                         // 不可為0,1
// 3. 比對門檻變數
int Removal_Compare = 9;	//7;	//8;
// 4. Debug Image
int Removal_Debug_Img = 0;
void add_Removal_Group(int df, int f01, int y, int ost, int x)
{
    int i, num, n1=0, n2=0, n3=0, n4=0, n5=0;
    for(i = 1; i <= Removal_Distance; i++){        // 左
        if(x-i > 0){
            if(Removal_G_Table[df][y][x-i] != 0){
                num = Removal_G_Table[df][y][x-i];
                n1 = num;
                Removal_G_Table[df][y][x] = n1;
                Removal_G_Count[n1]++;
                add_GT_link(n1, df, y, x);
                break;
            }
        }
    }
    for(i = 1; i <= Removal_Distance; i++){        // 上
        if(y-i > 0){
            if(Removal_G_Table[df][y-i][x] != 0){
                num = Removal_G_Table[df][y-i][x];
                n2 = num;
                if(n1 == 0){
                    Removal_G_Table[df][y][x] = n2;
                    Removal_G_Count[n2]++;
                    add_GT_link(n2, df, y, x);
                }
                else if(n1 > 0 && n1 != n2){
                    int n, cnt1 = Removal_G_Count[n1];
                    int df2, y2, x2;
                    get_GT_start(n1, &df2, &y2, &x2);
                    for(n = 0; n < cnt1; n++){
                        if(df2 < 0 || y2 < 0 || x2 < 0){ db_debug("add_Removal_Group: n1=%d n2=%d n=%d\n", n1, n2, n); return; }
                        Removal_G_Table[df2][y2][x2] = n2;
                        Removal_G_Count[n1] --;
                        Removal_G_Count[n2] ++;
                        get_GT_next(df2, y2, x2, &df2, &y2, &x2);
                    }
                    replace_GT_link(n1, n2);
                }
                break;
            }
        }
    }
    if(Removal_G_Table[df][y][x] == 0){
        n5 = Removal_G_Number;
        Removal_G_Table[df][y][x] = n5;
        if(n5 < 0x10000){
            Removal_G_Count[n5]++;
            Removal_G_Number ++;
            add_GT_link(n5, df, y, x);
        }
    }
}
// 6912*864 pixel = 0x5B2000*2(byte)
char Removal_Pixel_Buffer[0xB64000];            // 11.39mb, 0xB64000=3888*3072(整除3k)
int Removal_Transparent[5][432*576];            // sensor=5, 4608/8=576, 3456/8=432
char Removal_D_Table[4][2][432][576*3];         // [4]Page, [2]F0/F1, [576*432*3]Sensor, 0x5B2000=>6MB
char Removal_D2_Table[4][2][432][576*3];

int get_Removal_Debug_Img(void){ return Removal_Debug_Img; }
void SetRemovalVariable(int sel, int val)
{
    switch(sel){
    case 0: Removal_Distance = val; break;
    case 1: Removal_Divide = val; break;
    case 2: Removal_Compare = val; break;
    case 3: Removal_Debug_Img = val; break;
    }
}
int GetRemovalVariable(int sel)
{
	int val;
    switch(sel){
    case 0: val = Removal_Distance; break;
    case 1: val = Removal_Divide; break;
    case 2: val = Removal_Compare; break;
    case 3: val = Removal_Debug_Img; break;
    }
    return val;
}

void cal_Removal_D_Table(int *trans_in)
{
    int i, x, y, sensor, f01;
    int df, tran, meet, cnt, ost;
    db_debug("cal_Removal_D_Table: start!\n");
    //memset(&Removal_D_Table[0], 0xff, sizeof(Removal_D_Table));
    for(sensor = 0; sensor < 5; sensor++){
        switch(sensor){
        case 0: f01 = 0; ost = 0;     break;
        case 1: f01 = 0; ost = 576;   break;
        case 2: f01 = 1; ost = 0;     break;
        case 3: f01 = 1; ost = 576;   break;
        case 4: f01 = 1; ost = 1152;  break;
        }
        // [0][1][x][0][1][x][0][1][x][0][1][x], F0 DDR
        // [2][3][4][2][3][4][2][3][4][2][3][4], F1 DDR
        for(y = 0; y < 432; y++){           // line
            for(x = 0; x < 576; x++){       // pixel
                tran = Removal_Transparent[sensor][y*576 + x];
                for(df = 0; df < 4; df++){
                    meet = (tran >> (df*4)) & 0xf;
                    if(meet > 0) Removal_D_Table[df][f01][y][ost+x] = 100;
                    else         Removal_D_Table[df][f01][y][ost+x] = 0;        // 比對不到相同
                    Removal_D2_Table[df][f01][y][ost+x] = Removal_D_Table[df][f01][y][ost+x];
                }//*/
            }
        }
    }
    // 擴散機制
    int num, val, v, omax;
    for(df = 0; df < 4; df++){
        for(f01 = 0; f01 < 2; f01++){
            omax = (f01 == 0)? 1152: 1728;
            for(y = 0; y < 406; y++){           // 432  // line     // 3252/8=406.5
                for(x = 0; x < 540; x++){       // 576  // pixel    // 4320/8=540
                    for(ost = 0; ost < omax; ost+=576){
                        num = 0; val = 0;
                        val += Removal_D_Table[df][f01][y][ost+x]; num++;
                        for(i = 1; i <= Removal_Distance; i++){
                            if(x+i < ost+540){ val += Removal_D_Table[df][f01][y][ost+x+i]; num++; }
                            if(x-i > ost){ val += Removal_D_Table[df][f01][y][ost+x-i]; num++; }
                        }
                        Removal_D2_Table[df][f01][y][ost+x] = val / num;    // 左右7點平均
                    }
                }
            }
        }
    }//*/
    //int div_n = 100 / Removal_Divide;
    for(df = 0; df < 4; df++){
        for(f01 = 0; f01 < 2; f01++){
            omax = (f01 == 0)? 1152: 1728;
            for(y = 0; y < 406; y++){           // 432  // line     // 3252/8=406.5
                for(x = 0; x < 540; x++){       // 576  // pixel    // 4320/8=540
                    for(ost = 0; ost < omax; ost+=576){
                        num = 0; val = 0;
                        val += Removal_D2_Table[df][f01][y][ost+x]; num++;
                        for(i = 1; i <= Removal_Distance; i++){
                            if(y+i < 406){ val += Removal_D2_Table[df][f01][y+i][ost+x]; num++; }
                            if(y-i > 0){ val += Removal_D2_Table[df][f01][y-i][ost+x]; num++; }
                        }
                        /*100 - (100 - 10) * 3 = 0
                        100 - (100 - 50) * 3 = 0
                        100 - (100 - 66) * 3 = 1
                        100 - (100 - 80) * 3 = 40   
                        100 - (100 - 85) * 3 = 55                        
                        100 - (100 - 90) * 3 = 70//*/
                        v = 100 - (100 - (val / num)) * Removal_Divide; // 上下7點平均
                        if(v > 100) v = 100;
                        if(v < 0) v = 0;
                        Removal_D_Table[df][f01][y][ost+x] = v;
                        //Removal_D2_Table[df][f01][y][ost+x] = val / num;
                    }
                }
            }
        }
    }//*/
    for(df = 0; df < 4; df++){
        for(f01 = 0; f01 < 2; f01++){
            omax = (f01 == 0)? 1152: 1728;
            for(y = 0; y < 432; y++){           // 432  // line     // 3252/8=406.5
                for(x = 0; x < 576; x++){       // 576  // pixel    // 4320/8=540
                    for(ost = 0; ost < omax; ost+=576){
                        if(Removal_D_Table[df][f01][y][ost+x] >= 50)            // > 83
                            Removal_D_Table[df][f01][y][ost+x] = 100;
                        else
                            Removal_D_Table[df][f01][y][ost+x] = 0;
                    }
                }
            }
        }
    }//*/
    //f01 = 0; ost = 0;
    for(f01 = 0; f01 < 2; f01++){           // 2塊F1 DDR
        omax = (f01 == 0)? 1152: 1728;
        for(ost = 0; ost < omax; ost+=576){
            init_Removal_Group();
            // 統計移動範圍大小
            for(df = 0; df < 4; df++){              // 4組圖
                for(y = 0; y < 406; y++){           // 432 line         // 3252/8=406.5
                    for(x = 0; x < 540; x++){       // 576*3  pixel     // 4320/8=540
                        if(Removal_D_Table[df][f01][y][ost+x] == 0){
                            add_Removal_Group(df, f01, y, ost, x);
                        }
                    }
                }
            }
            // 取移動範圍最小的設定不透明
            for(y = 0; y < 406; y++){           // 432 line         // 3252/8=406.5
                for(x = 0; x < 540; x++){       // 576*3  pixel     // 4320/8=540
                    if(Removal_D_Table[0][f01][y][ost+x] == 0 &&    // 表示畫面都比對不到相同
                       Removal_D_Table[1][f01][y][ost+x] == 0 &&
                       Removal_D_Table[2][f01][y][ost+x] == 0 &&
                       Removal_D_Table[3][f01][y][ost+x] == 0)
                    {
                        int n1, df1=-1, min=-1, cnt=0;
                        for(i = 0; i < 4; i++){
                            n1 = Removal_G_Table[i][y][x];
                            if(n1 > 0){
                                cnt = Removal_G_Count[n1];
                                if(cnt > 0 && min == -1){
                                    min = cnt; df1 = i;
                                }
                                else if(cnt > 0 && cnt < min){
                                    min = cnt; df1 = i;
                                }
                            }
                            else{ db_error("cal_Removal_D_Table: e1! i=%d y=%d x=%d n1=%d\n", i, y, x, n1); }
                        }
                        int df2, y2, x2, diff=0;
                        if(df1 >= 0){
                            n1 = Removal_G_Table[df1][y][x];
                            cnt = Removal_G_Count[n1];
                            get_GT_start(n1, &df2, &y2, &x2);
                            for(i = 0; i < cnt; i++){
                                if(df2 < 0 || y2 < 0 || x2 < 0){ 
                                    db_error("cal_Removal_D_Table: e2! i=%d y=%d x=%d n1=%d\n", i, y, x, n1); 
                                    break;
                                }
                                else{
                                    Removal_D_Table[df2][f01][y2][ost+x2] = 101;        // 挑範圍最小設定顯示, 特殊碼"101"
                                    get_GT_next(df2, y2, x2, &df2, &y2, &x2);
                                }
                            }
                        }
                    }
                }
            }
        }
    }//*/
    /*int err=0;
    for(df = 0; df < 4; df++){
        for(f01 = 0; f01 < 2; f01++){
            omax = (f01 == 0)? 1152: 1728;
            for(y = 0; y < 406; y++){           // 432  // line     // 3252/8=406.5
                for(x = 0; x < 540; x++){       // 576  // pixel    // 4320/8=540
                    for(ost = 0; ost < omax; ost+=576){
                        if(y < 406 && x < 540 &&
                           Removal_D_Table[0][f01][y][ost+x] == 0 &&
                           Removal_D_Table[1][f01][y][ost+x] == 0 &&
                           Removal_D_Table[2][f01][y][ost+x] == 0 &&
                           Removal_D_Table[3][f01][y][ost+x] == 0)
                        {
                            err++;
                        }
                        if(Removal_D_Table[df][f01][y][ost+x] == 0) Removal_D2_Table[df][f01][y][ost+x] = 0xFF;
                        else                                        Removal_D2_Table[df][f01][y][ost+x] = 0;
                    }
                }
            }
        }
    }
    if(err > 0){
        db_error("cal_Removal_D_Table: e3! err=%d\n", err);
    }//*/
    int removal_warning = 0;
    for(df = 0; df < 4; df++){
        for(f01 = 0; f01 < 2; f01++){
            omax = (f01 == 0)? 1152: 1728;
            for(y = 0; y < 406; y++){           // 432  // line     // 3252/8=406.5
                for(x = 0; x < 540; x++){       // 576  // pixel    // 4320/8=540
                    for(ost = 0; ost < omax; ost+=576){
                        if(Removal_D_Table[df][f01][y][ost+x] == 101){
                            //Removal_D2_Table[df][f01][y][ost+x] = 0;
                            for(i = 0; i < 4; i ++){                                // 避免半透明出現
                                if(df != i){
                                    if(Removal_D_Table[i][f01][y][ost+x] == 101){
                                        removal_warning ++;
                                    }
                                    //Removal_D2_Table[i][f01][y][ost+x] = 0xFF;      // rex+ 190704
                                    Removal_D_Table[i][f01][y][ost+x] = 0;
                                }
                            }
                        }
                        //else if(Removal_D_Table[df][f01][y][ost+x] == 0)
                        //    Removal_D2_Table[df][f01][y][ost+x] = 0xFF;
                        //else
                        //    Removal_D2_Table[df][f01][y][ost+x] = 0;
                    }
                }
            }
        }
    }//*/
    if(removal_warning > 0){
        db_error("cal_Removal_D_Table: w1! warning=%d\n", removal_warning);
    }

    // 擴散機制
    for(df = 0; df < 4; df++){
        for(f01 = 0; f01 < 2; f01++){
            omax = (f01 == 0)? 1152: 1728;
            for(y = 0; y < 406; y++){           // 432  // line     // 3252/8=406.5
                for(x = 0; x < 540; x++){       // 576  // pixel    // 4320/8=540
                    for(ost = 0; ost < omax; ost+=576){
                        num = 0; val = 0;
                        val += Removal_D_Table[df][f01][y][ost+x]; num++;
                        for(i = 1; i <= Removal_Distance; i++){
                            if(x+i < ost+540){ val += Removal_D_Table[df][f01][y][ost+x+i]; num++; }
                            if(x-i > ost){ val += Removal_D_Table[df][f01][y][ost+x-i]; num++; }
                        }
                        Removal_D2_Table[df][f01][y][ost+x] = val / num;    // 左右7點平均
                    }
                }
            }
        }
    }//*/
    //int div_n = 100 / Removal_Divide;
    for(df = 0; df < 4; df++){
        for(f01 = 0; f01 < 2; f01++){
            omax = (f01 == 0)? 1152: 1728;
            for(y = 0; y < 406; y++){           // 432  // line     // 3252/8=406.5
                for(x = 0; x < 540; x++){       // 576  // pixel    // 4320/8=540
                    for(ost = 0; ost < omax; ost+=576){
                        num = 0; val = 0;
                        val += Removal_D2_Table[df][f01][y][ost+x]; num++;
                        for(i = 1; i <= Removal_Distance; i++){
                            if(y+i < 406){ val += Removal_D2_Table[df][f01][y+i][ost+x]; num++; }
                            if(y-i > 0){ val += Removal_D2_Table[df][f01][y-i][ost+x]; num++; }
                        }
                        v = 100 - (100 - (val / num)) * Removal_Divide; // 上下7點平均
                        if(v > 100) v = 100;
                        if(v < 0) v = 0;
                        Removal_D_Table[df][f01][y][ost+x] = v;
                    }
                }
            }
        }
    }//*/
    // 透明度計算
    //cnt = 0;
    int v1, v2, v3, v4;
        for(f01 = 0; f01 < 2; f01++){
            omax = (f01 == 0)? 1152: 1728;
            for(y = 0; y < 432; y++){           // 432  // line     // 3252/8=406.5
                for(x = 0; x < 576; x++){       // 576  // pixel    // 4320/8=540
                    for(ost = 0; ost < omax; ost+=576){
                        v1 = Removal_D_Table[0][f01][y][ost+x];                // 0:沒有比對相同, 100:有比對相同
                        v2 = Removal_D_Table[1][f01][y][ost+x];
                        v3 = Removal_D_Table[2][f01][y][ost+x];
                        v4 = Removal_D_Table[3][f01][y][ost+x];

                        if(v1 > 0) Removal_D2_Table[0][f01][y][ost+x] = 0xFF - (0xFF * v1 / (v1));
                        else       Removal_D2_Table[0][f01][y][ost+x] = 0xFF;
                        if(v2 > 0) Removal_D2_Table[1][f01][y][ost+x] = 0xFF - (0xFF * v2 / (v1 + v2));
                        else       Removal_D2_Table[1][f01][y][ost+x] = 0xFF;
                        if(v3 > 0) Removal_D2_Table[2][f01][y][ost+x] = 0xFF - (0xFF * v3 / (v1 + v2 + v3));
                        else       Removal_D2_Table[2][f01][y][ost+x] = 0xFF;
                        if(v4 > 0) Removal_D2_Table[3][f01][y][ost+x] = 0xFF - (0xFF * v4 / (v1 + v2 + v3 + v4));
                        else       Removal_D2_Table[3][f01][y][ost+x] = 0xFF;

                        if(Removal_D2_Table[0][f01][y][ost+x] == 0xFF &&        // 0x00:不透明, 0xFF:全透明
                           Removal_D2_Table[1][f01][y][ost+x] == 0xFF &&
                           Removal_D2_Table[2][f01][y][ost+x] == 0xFF &&
                           Removal_D2_Table[3][f01][y][ost+x] == 0xFF)
                        {
                            Removal_D2_Table[0][f01][y][ost+x] = 0;
                        }
                    }
                }
            }
        }
    //*/

    int id, mode = 1;
    int t_addr, s_addr;
    int isp2_w_addr_a, isp2_w_addr_b, isp2_w_addr_c;

    //save_Removal_bin_file("Removal_D_Table.bin", Removal_D_Table, sizeof(Removal_D_Table));
    for(df = 0; df < 4; df++){
        //FX_WDR_DIF_2_A_P0_ADDR    0x00D07500 
        get_wdr_dif_addr(df, &isp2_w_addr_a, &isp2_w_addr_b, &isp2_w_addr_c);
        for(y = 0; y < 432; y++){
            for(f01 = 0; f01 < 2; f01++){
                s_addr = H264_STM1_P0_M_BUF_ADDR + y*0x8000 + f01*432*0x8000 + df*576*3;        // df*576*3, 576*0x8000防止H.264執行壓到記憶體
                ua360_spi_ddr_write(s_addr, &Removal_D2_Table[df][f01][y][0], 576*3);         // 暫時使用H264_STM1_P0_M_BUF_ADDR

                if(f01 == 0) id = AS2_MSPI_F0_ID;
                else         id = AS2_MSPI_F1_ID;
                t_addr = isp2_w_addr_a + y*0x8000;             // 3個sensor的資料一起寫入
                AS2_Write_F2_to_FX_CMD(s_addr, t_addr, 576*3+320, mode, id, 1);      // 576*3+320, size需要是512的倍數
            }
        }
    }
    db_debug("cal_Removal_D_Table: done!\n");
}
void debug_Removal_D_Table(int t_addr)
{
    int df;
    int s_addr, y, f01, id;
    if(Removal_Debug_Img == 1){
        for(df = 0; df < 4; df++){
            //FX_WDR_DIF_2_A_P0_ADDR    0x00D07500 
            for(y = 0; y < 432; y++){
                for(f01 = 0; f01 < 2; f01++){
                    s_addr = t_addr + y*0x8000 + f01*432*0x8000 + df*576*3;                         // df*576*3
                    ua360_spi_ddr_write(s_addr, &Removal_D_Table[df][f01][y][0], 576*3);         // 暫時使用H264_STM1_P0_M_BUF_ADDR
                }
            }
        }
    }
}
/*
 * HDR -> 5張合成1張 / F1 DDR
 * Removal -> 4張HDR合成1張 / F1 DDR
 */
int F_Pipe_Add_Removal(int Img_Mode, int *Finish_Idx, int *Sensor_Idx, int C_Mode, int Removal_St)
{
    int i, j, y, P0_Idx; struct FPGA_Engine_Resource_Struct *P1;
    int Check_Flag = 1, Sub_Idx = *Finish_Idx;
    int usb_spend_time;
    int p_mask = (FPGA_ENGINE_P_MAX-1);
    int hdr_level = 4;
    int shuter_time, isp1_time;
    int thm_mode = 5;
    int btm_m=0, btm_s=100, cp_mode=0, btm_t_m=0; //btm_m -> 0:關, 1:延伸, 2:加底圖(default), 3:鏡像, 4:加底圖(user)
    int jid = F_Temp.Jpeg_Id;
    int Now_Mode;
    int mode, res;
    int sensor_idx_tmp = -1;
    int Times;
    get_Stitching_Out(&mode, &res);
    if(res == 7)       Now_Mode = 1;			//8K
    else if(res == 12) Now_Mode = 2;			//6K
    else               Now_Mode = Img_Mode;
    usb_spend_time = get_USB_Spend_Time(Now_Mode);
#ifdef ANDROID_CODE
    get_A2K_DMA_BottomMode(&btm_m, &btm_s);
    get_A2K_DMA_BottomTextMode(&btm_t_m);
    get_A2K_DMA_CameraPosition(&cp_mode);
    hdr_level = get_A2K_Sensor_HDR_Level();     // 3,4,6
#endif
    P1 = &F_Pipe.Sensor;
    P0_Idx = P1->Idx;
    P1->P[P0_Idx].No       = -1;
    P1->P[P0_Idx].Time_Buf = -1;
    P1->P[P0_Idx].I_Page   = -1;
    P1->P[P0_Idx].O_Page   = -1;
    P1->P[P0_Idx].M_Mode   = Img_Mode;
    P1->Idx =  (P1->Idx + 1) & p_mask;

    db_debug("F_Pipe_Add_Removal: Img_Mode=%d St=%d\n", Img_Mode, Removal_St);

    memcpy(&F_Temp, &F_Pipe, sizeof(F_Pipe) );
    F_Pipe_Add_Sub(-1, -1, -1, -1, -1, -1, &Sub_Idx, &Times);
    if(btm_s != 0) F_Temp.DMA.Page[0] = F_Temp.Stitch.Page[0];  // rex+ 180626, 解DMA錯誤，拍成前一張畫面

    int isp1_page_start = F_Temp.ISP1.Page[0];
    int frame, integ, dgain, exp_n, exp_m, iso;
    int HDR_fCnt = 5;                                       // 做5張HDR x 4次
    int Scale = Removal_HDR_AE_Scale;	//30;				// +- 1.5
    int istart = (7 - HDR_fCnt) >> 1;                       // 3:2, 5:1, 7:0
    int istop = HDR_fCnt - 1;
    static int jtbl_idx;
    int Removal_fCnt = 4;
    char *fptr;
    
    F_Temp.Pic_No++;        // debug used
    // Removal_St: 0,1,2,3  .. 取得Live影像
    switch(Removal_St){
    case 0:
    case 1:
    case 2:
    case 3:
        for(i = 0; i < HDR_fCnt; i++){
        	if(Removal_HDR_Mode == 1) {
        		if(i < (HDR_fCnt / 2) ) Scale = HDR7P_Auto_P.EV_Inc[1] * 20;
        		else                    Scale = HDR7P_Auto_P.EV_Inc[0] * 20;
        	}
        	else Scale = Removal_HDR_AE_Scale;

            do_AEB_sensor_cal(i+istart, Scale, C_Mode, &frame, &integ, &dgain, &exp_n, &exp_m, &iso);            // 1,2,3,4,5
    
            shuter_time = F_Com_In.Shuter_Speed * (frame+1);
            //[].Sensor, 33=不執行AE調整=-1
            if(i == 0)
               Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 3, 1, 2, shuter_time, &Sub_Idx, &Times);                       // Removal
            else
               Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 1, 1, 2, shuter_time, &Sub_Idx, &Times);                       // Removal
            set_F_Temp_Sensor_SubData(Sub_Idx, PIPE_SUBCODE_AEB_STEP_0+i, -1, 0, frame, integ, dgain);      // Removal
    
            if(i == istop) isp1_time = F_Com_In.Pipe_Time_Base * HDR_fCnt;                                  // Removal
            else           isp1_time = F_Com_In.Pipe_Time_Base;
            //[].ISP1
            Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 1, 2, 3, isp1_time, &Sub_Idx, &Times);                         // Removal
            set_F_Temp_ISP1_SubData(Sub_Idx, PIPE_SUBCODE_AEB_STEP_0+i, HDR_fCnt, Img_Mode, -1, -1, 1);     // Removal
            if(i == 0) Set_F_JPEG_Header_ISP1(Sub_Idx, &jtbl_idx);
            sensor_idx_tmp = Sub_Idx;
    
            if(i < HDR_fCnt){
                F_Temp.Sensor.Page[0] = (F_Temp.Sensor.Page[0]+1) % 2;
                F_Temp.ISP1.Page[0]   = (F_Temp.ISP1.Page[0]+1) % 3;
            }
        }
        do_AEB_sensor_cal(3, Scale, C_Mode, &frame, &integ, &dgain, &exp_n, &exp_m, &iso);
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode, 0, 1, 1, 2, F_Com_In.Shuter_Speed, &Sub_Idx, &Times);
        set_F_Temp_Sensor_SubData(Sub_Idx, PIPE_SUBCODE_AEB_STEP_E, -1, 0, frame, integ, dgain);      // Removal
        
        F_Temp.ISP1.Page[0] = isp1_page_start;
        int s_idx = 1 + Sub_Idx;
        for(i = 0; i < HDR_fCnt; i++){
            //[].WDR
            set_F_Temp_WDR_SubData(((s_idx+i)&p_mask), PIPE_SUBCODE_AEB_STEP_0+i, -1, HDR_fCnt);            // Removal#WDR
        }
        for(i = 0; i < HDR_fCnt; i++){
            //[].ISP2, ISP2_NR3D_Rate=0
            Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 2, 3, 4, FPGA_Speed[Img_Mode].ISP2, &Sub_Idx, &Times);
            set_F_Temp_ISP2_SubData(Sub_Idx, 0, PIPE_SUBCODE_AEB_STEP_0+i, HDR_fCnt, Removal_St);           // Removal#ISP2: nr3d=0, hdr_7idx=i, hdr_mode=3,5,7

            if(i < HDR_fCnt){
                F_Temp.ISP1.Page[0]   = (F_Temp.ISP1.Page[0]+1) % 3;
                F_Temp.ISP2.Page[0]   = (F_Temp.ISP2.Page[0]+1) % 2;
            }
        }
        F_Temp.Sensor.Page[0] = (F_Temp.Sensor.Page[0]+1) % 2;
        F_Temp.ISP1.Page[0]   = (F_Temp.ISP1.Page[0]+1) % 3;
        F_Temp.ISP2.Page[0]   = (F_Temp.ISP2.Page[0]+1) % 2;
        break;
    case 4:     // 製作4x5合成圖, ISP2, ST, JPEG, USB
        //[].Stitch
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 3, 4, 5, FPGA_Speed[Img_Mode].Stitch_Img, &Sub_Idx, &Times);
        set_F_Temp_Stitch_SubData(Sub_Idx, -1, PIPE_SUBCODE_REMOVAL_CNT_0, -1, -1, -1);
        //[].Smooth
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode, 0, 4, 9, 10, 0, &Sub_Idx, &Times);
        set_F_Temp_Smooth_SubData(Sub_Idx, -1);
        //[].DMA
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 4, 6, 5, FPGA_Speed[Img_Mode].DMA, &Sub_Idx, &Times);
        set_F_Temp_DMA_SubData(Sub_Idx, -1, -1, -1, -1, 3);
        Sub_Idx += ((FPGA_Speed[Img_Mode].DMA*3 + F_Pipe.Pipe_Time_Base - 10000) / F_Pipe.Pipe_Time_Base);
        Sub_Idx &= p_mask;
/*
        //[].Jpeg
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 6, 5, 7, FPGA_Speed[Img_Mode].Jpeg, &Sub_Idx, &Times);
        set_F_Temp_Jpeg_SubData(Sub_Idx, F_Temp.Jpeg_Id, -1, PIPE_SUBCODE_REMOVAL_CNT_0, -1, jtbl_idx, -1, -1, -1);
        //[].H264
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode ,0, 4, 8, 7, 0, &Sub_Idx, &Times);
        set_F_Temp_H264_SubData(Sub_Idx, 0, 0, 0, 0);
        //[].USB
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 5, 7, 10,usb_spend_time, &Sub_Idx, &Times);
        set_F_Temp_USB_SubData(Sub_Idx, F_Com_In.encode_type, 1);
        F_Temp.Jpeg[F_Temp.Jpeg_Id].Page[0]   = (F_Temp.Jpeg[F_Temp.Jpeg_Id].Page[0]+1) % 2;
        F_Temp.Jpeg_Id        = (F_Temp.Jpeg_Id+1) % 2;
        F_Temp.USB.Page[0]    = (F_Temp.USB.Page[0]+1) % 2;
*/
        //F_Temp.Stitch.Page[0] = (F_Temp.Stitch.Page[0]+1) % 2;
        break;
    case 5:     // 比對&寫入WDR Cmd
        // 讀取DDR資料
        // DMA後6912x864x2(b), ST_STM1_P1_T_ADDR
        // 需花費7sec, char Removal_Pixel_Buffer[0xB64000];
        fptr = (char *)&Removal_Pixel_Buffer[0];
        for(y = 0; y < 864; y++){           // 432*2
            // 1條line, 576*3*4=6912 6912*2(B)=13824(B)
            ua360_spi_ddr_read(ST_STM1_P1_T_ADDR+(y*0x8000)+3072*0, fptr, 3072, 2, 0); fptr += 3072;
            ua360_spi_ddr_read(ST_STM1_P1_T_ADDR+(y*0x8000)+3072*1, fptr, 3072, 2, 0); fptr += 3072;
            ua360_spi_ddr_read(ST_STM1_P1_T_ADDR+(y*0x8000)+3072*2, fptr, 3072, 2, 0); fptr += 3072;
            ua360_spi_ddr_read(ST_STM1_P1_T_ADDR+(y*0x8000)+3072*3, fptr, 3072, 2, 0); fptr += 3072;
            ua360_spi_ddr_read(ST_STM1_P1_T_ADDR+(y*0x8000)+3072*4, fptr, 1536, 2, 0); fptr += 1536;
        }
        // 576x432
        //[0][1][ ][0][1][ ][0][1][ ][0][1][ ]
        //[2][3][4][2][3][4][2][3][4][2][3][4]
        //Removal_Pixel_Buffer[576*432*24*2]
        int ln_ost = 13824;         // 13824=576*3*4*2(B)
        int fr_ost = 1152;          // 1152=576*2
        fptr = (char *)&Removal_Pixel_Buffer[0];
        Obj_Removal(fptr                      , fr_ost*3, ln_ost, &Removal_Transparent[0][0]);
        Obj_Removal(fptr+(fr_ost)             , fr_ost*3, ln_ost, &Removal_Transparent[1][0]);
        Obj_Removal(fptr+(ln_ost*432)         , fr_ost*3, ln_ost, &Removal_Transparent[2][0]);
        Obj_Removal(fptr+(ln_ost*432+fr_ost)  , fr_ost*3, ln_ost, &Removal_Transparent[3][0]);
        Obj_Removal(fptr+(ln_ost*432+fr_ost*2), fr_ost*3, ln_ost, &Removal_Transparent[4][0]);
        
        //save_Removal_bin_file((char *)&Removal_Transparent[0], sizeof(Removal_Transparent));
        //get_wdr_dif_addr(i_page, &isp2_w_addr_a, &isp2_w_addr_b, &isp2_w_addr_c);
        cal_Removal_D_Table(&Removal_Transparent[0]);
        Sub_Idx = (Sub_Idx + 40) & p_mask;

        F_Temp.ISP2.Idx = Sub_Idx;
        break;
    case 6:     // 製作ISP2合成圖(4->1)

        for(i = 0; i < Removal_fCnt; i++){
            //[].ISP2, ISP2_NR3D_Rate=0
            Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 3, 3, 4, FPGA_Speed[Img_Mode].ISP2, &Sub_Idx, &Times);
            set_F_Temp_ISP2_SubData(Sub_Idx, 0, PIPE_SUBCODE_AEB_STEP_0+i, Removal_fCnt, Removal_St);           // Removal#ISP2: nr3d=0, hdr_7idx=i, hdr_mode=3,5,7
    
            if(i < Removal_fCnt){
                F_Temp.ISP1.Page[0]   = (F_Temp.ISP1.Page[0]+1) % 3;
                F_Temp.ISP2.Page[0]   = (F_Temp.ISP2.Page[0]+1) % 2;
            }
            F_Temp.Pic_No++;        // debug used
        }

        int st_p2, dma_p0;
        int smooth_time;
        Get_Smooth_Page(F_Com_In.Smooth_En, Img_Mode, &st_p2, &dma_p0, &smooth_time);
        //[].Stitch
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 3, 4, st_p2, 700000, &Sub_Idx, &Times);            // 為了取得色縫合係數, time=0.5sec
        set_F_Temp_Stitch_SubData(Sub_Idx, -1, -1, 1, 1, 0);                            // Removal#ST, st_step=1, yuv_ddr=1, i_page=1
        //[].Smooth
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode, 0, 4, 9, 6, 0, &Sub_Idx, &Times);
        set_F_Temp_Smooth_SubData(Sub_Idx, -1);
        //[].Stitch
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 3, 4, st_p2, FPGA_Speed[Img_Mode].Stitch_Img, &Sub_Idx, &Times);
        set_F_Temp_Stitch_SubData(Sub_Idx, -1, -1, 2, 1, 0);                            // Removal#ST, st_step=2, yuv_ddr=1, i_page=1
        //[].Smooth
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode, 0, 4, 9, 6, smooth_time, &Sub_Idx, &Times);
        set_F_Temp_Smooth_SubData(Sub_Idx, F_Com_In.Smooth_En);
        if(Removal_Debug_Img == 1){
            //[].Stitch, Debug小圖
            // S=ISP2_6X6_BIN_ADDR_A_P0, T=ST_STM1_P0_T_ADDR
            Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 3, 4, 6, FPGA_Speed[Img_Mode].Stitch_Img, &Sub_Idx, &Times);
            set_F_Temp_Stitch_SubData(Sub_Idx, -1, PIPE_SUBCODE_REMOVAL_CNT_1, -1, -1, -1);
            //[].Smooth
            Check_Flag &= F_Pipe_Add_Sub(Img_Mode, 0, 4, 9, 10, 0, &Sub_Idx, &Times);
            set_F_Temp_Smooth_SubData(Sub_Idx, -1);
        }
        int dma_time;   
        switch(btm_m) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4: if(F_Com_In.Smooth_En == 1) F_Temp.DMA.Page[0] = F_Temp.Smooth.Page[0];
				else						F_Temp.DMA.Page[0] = F_Temp.Stitch.Page[0];
        		if(btm_m == 3) dma_time = FPGA_Speed[Img_Mode].DMA;
                else           dma_time = FPGA_Speed[5].DMA;
                Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, dma_p0, 6, 5, dma_time, &Sub_Idx, &Times);                  //[].DMA
                set_F_Temp_DMA_SubData(Sub_Idx, btm_m, btm_s, cp_mode, btm_t_m, 0);
                if(btm_t_m == 1 && (btm_m == 1 || btm_m == 2 || btm_m == 3) ) {
                	dma_time = FPGA_Speed[5].DMA;
                    Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 6, 6, 5, dma_time, &Sub_Idx, &Times);              //[].DMA
                    set_F_Temp_DMA_SubData(Sub_Idx, btm_m, btm_s, cp_mode, btm_t_m, 1);
                }
        	    break;
        }
        // ISP2(3) -> ST(4) -> DMA(6) -> JPG(5) -> USB(7)
        if(Now_Mode == 1 || Now_Mode == 2) {
        	if(F_Com_In.Smooth_En == 1) F_Temp.DMA.Page[0] = (F_Temp.Smooth.Page[0]+1) % 2;
        	else						F_Temp.DMA.Page[0] = (F_Temp.Stitch.Page[0]+1) % 2;
        	Check_Flag &= F_Pipe_Add_Sub(Now_Mode,0, dma_p0, 6, 5, FPGA_Speed[Img_Mode].DMA, &Sub_Idx, &Times);      //[].DMA
        	set_F_Temp_DMA_SubData(Sub_Idx, -1, -1, -1, -1, 2);
        }
        do_AEB_sensor_cal(3, Scale, C_Mode, &frame, &integ, &dgain, &exp_n, &exp_m, &iso);
        Check_Flag &= F_Pipe_Add_Sub(Now_Mode,0, 6, 5, 7, FPGA_Speed[Now_Mode].Jpeg, &Sub_Idx, &Times);         //[].Jpeg, Removal
        set_F_Temp_Jpeg_SubData(Sub_Idx, F_Temp.Jpeg_Id, -1, -1, -1, jtbl_idx, exp_n, exp_m, iso);
        Check_Flag &= F_Pipe_Add_Sub(Now_Mode ,0, 4, 8, 7, 0, &Sub_Idx, &Times);					            //[].H264, Removal
        set_F_Temp_H264_SubData(Sub_Idx, 0, 0, 0, 0);
        Check_Flag &= F_Pipe_Add_Sub(Now_Mode,0, 5, 7, 10, usb_spend_time, &Sub_Idx, &Times);                    //[].USB
        set_F_Temp_USB_SubData(Sub_Idx, F_Com_In.encode_type, 1);
        F_Com_In.Capture_Lose_Page[0] = F_Temp.USB.P[Sub_Idx&(FPGA_ENGINE_P_MAX-1)].I_Page;

        F_Temp.Jpeg[jid].Page[0] = (F_Temp.Jpeg[jid].Page[0]+1) % 2;
        F_Temp.Jpeg_Id           = (F_Temp.Jpeg_Id+1) % 2;
        F_Temp.USB.Page[0]       = (F_Temp.USB.Page[0]+1) % 2;
        F_Temp.Pic_No++;
    
        //[].Jpeg
        Check_Flag &= F_Pipe_Add_Sub(Now_Mode,0, 7, 5, 7, FPGA_Speed[thm_mode].Jpeg, &Sub_Idx, &Times);
        set_F_Temp_Jpeg_SubData(Sub_Idx, F_Temp.Jpeg_Id, PIPE_SUBCODE_CAP_THM, -1, -1, jtbl_idx, -1, -1, -1);
        //[].H264
        Check_Flag &= F_Pipe_Add_Sub(Now_Mode ,0, 7, 8, 7, 0, &Sub_Idx, &Times);
        set_F_Temp_H264_SubData(Sub_Idx, 0, 0, 0, 0);
        //[].DMA
        Check_Flag &= F_Pipe_Add_Sub(Now_Mode,0, dma_p0, 6, 10,0, &Sub_Idx, &Times);
        set_F_Temp_DMA_SubData(Sub_Idx, -1, -1, -1, -1, -1);
        //[].USB
        Check_Flag &= F_Pipe_Add_Sub(Now_Mode,0, 5, 7, 10, get_USB_Spend_Time(thm_mode), &Sub_Idx, &Times);
        set_F_Temp_USB_SubData(Sub_Idx, F_Com_In.encode_type, 1);
        F_Com_In.Capture_Lose_Page[1] = F_Temp.USB.P[Sub_Idx&(FPGA_ENGINE_P_MAX-1)].I_Page;

        F_Temp.Sensor.Page[0]    = (F_Temp.Sensor.Page[0]+1) % 2;
        F_Temp.ISP1.Page[0]      = (F_Temp.ISP1.Page[0]+1) % 3;
        F_Temp.ISP2.Page[0]      = (F_Temp.ISP2.Page[0]+1) % 2;
        F_Temp.Stitch.Page[0]    = (F_Temp.Stitch.Page[0]+1) % 2;
        F_Temp.Smooth.Page[0] 	 = (F_Temp.Smooth.Page[0]+1) % 2;
        F_Temp.Jpeg[jid].Page[0] = (F_Temp.Jpeg[jid].Page[0]+1) % 2;
        F_Temp.Jpeg_Id           = (F_Temp.Jpeg_Id+1) % 2;
        F_Temp.DMA.Page[0]       = (F_Temp.DMA.Page[0]+1) % 2;
        F_Temp.USB.Page[0]       = (F_Temp.USB.Page[0]+1) % 2;
//*/
        break;
    }

    if(Check_Flag == 1){
        *Finish_Idx = Sub_Idx;
        *Sensor_Idx = sensor_idx_tmp;
        db_debug("F_Pipe_Add_Removal: Finish_Idx=%d\n", *Finish_Idx);
    }
    else{
        db_debug("F_Pipe_Add_Removal: err! Check_Flag=%d\n", Check_Flag);
    }
    F_Pipe_Add_Sub(-2, -2, -2, -2, -2, -2, &Sub_Idx, &Times);
    Make_F_Pipe( Now_Mode, Check_Flag, C_Mode);                 // F_Pipe_Add_Removal
    return Check_Flag;
};

int F_Pipe_Add_RAW(int Img_Mode, int *Finish_Idx, int *Sensor_Idx)
{
    int i;
    int Check_Flag = 1, Sub_Idx;
    int P0_Idx;
    int sen_mode = 1;
    struct FPGA_Engine_Resource_Struct *P1;
    int usb_spend_time = 155985;	            //get_USB_Spend_Time(sen_mode);     // 1->8k spend time  2->6K spend time		//180515 避免JPEG與USB使用到同一個BUF
    int p_mask = (FPGA_ENGINE_P_MAX-1);
    int np=0;
    int shuter_time = F_Com_In.Shuter_Speed;
	int Now_Mode, S_Mode;
	int sensor_idx_tmp = -1;
	int Times;
	Get_M_Mode(&Now_Mode, &S_Mode);

#ifdef ANDROID_CODE
    np = get_AEG_System_Freq_NP();
    shuter_time = get_A2K_Shuter_Speed();       // F_Pipe_Add_RAW
#endif
    db_debug("F_Pipe_Add_RAW() Img_Mode=%d\n", Img_Mode);

    P1 = &F_Pipe.Sensor;
    P0_Idx = P1->Idx;
    P1->P[P0_Idx].No       = -1;
    P1->P[P0_Idx].Time_Buf = -1;
    P1->P[P0_Idx].I_Page   = -1;
    P1->P[P0_Idx].O_Page   = -1;
    P1->P[P0_Idx].M_Mode   = Img_Mode;
    P1->Idx =  (P1->Idx + 1) & p_mask;
    memcpy(&F_Temp, &F_Pipe, sizeof(F_Pipe) );

    int long_ep = (np == 0)? 66667: 80000;
    int jtbl_idx=0;
    if(shuter_time <= long_ep){      // EP <= 1/15(66666) or 1/12.5(80000)
        for (i = 0; i < 3; i++) {
            //[].Sensor
            Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 1, 1, 2, shuter_time, &Sub_Idx, &Times);
            set_F_Temp_Sensor_SubData(Sub_Idx, -1, -1, 0, -1, -1, -1);
            //[].ISP1
            Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 1, 2, 3, F_Com_In.Pipe_Time_Base, &Sub_Idx, &Times);
            set_F_Temp_ISP1_SubData(Sub_Idx, -1, -1, Now_Mode, -1, -1, 1);
            if(i == 0) Set_F_JPEG_Header_ISP1(Sub_Idx, &jtbl_idx);
            set_F_Temp_WDR_SubData(Sub_Idx, -1, -1, -1);
            sensor_idx_tmp = Sub_Idx;
            //[].ISP2, 0~3 ISP2_NR3D_Rate, Big_5
            Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 2, 3, 4, FPGA_Speed[Img_Mode].ISP2, &Sub_Idx, &Times);
            set_F_Temp_ISP2_SubData(Sub_Idx, i, -1, -1, -1);                            // RAW
            if(i < 2) {         // isp2page idx要給下一級使用, 不能先加1
                F_Temp.Sensor.Page[0] = (F_Temp.Sensor.Page[0]+1) % 2;
                F_Temp.ISP1.Page[0]   = (F_Temp.ISP1.Page[0]+1) % 3;
                F_Temp.ISP2.Page[0]   = (F_Temp.ISP2.Page[0]+1) % 2;
                F_Temp.Pic_No++;
            }
        }
    }
    else{
        //[].Sensor
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 1, 1, 2, shuter_time, &Sub_Idx, &Times);
        set_F_Temp_Sensor_SubData(Sub_Idx, -1, 1, 0, -1, -1, -1);
        //[].ISP1
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 1, 2, 3, F_Com_In.Pipe_Time_Base, &Sub_Idx, &Times);
        set_F_Temp_ISP1_SubData(Sub_Idx, -1, -1, Now_Mode, -1, -1, 1);
        Set_F_JPEG_Header_ISP1(Sub_Idx, &jtbl_idx);
        set_F_Temp_WDR_SubData(Sub_Idx, -1, -1, -1);
        sensor_idx_tmp = Sub_Idx;
        //[].ISP2, 0~3 ISP2_NR3D_Rate
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 2, 3, 4, FPGA_Speed[Img_Mode].ISP2, &Sub_Idx, &Times);
        set_F_Temp_ISP2_SubData(Sub_Idx, 0, -1, -1, -1);                                // RAW
    }
    for (i = 0; i < 5; i++) {
        //[].Stitch
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 3, 4, 5, FPGA_Speed[sen_mode].Stitch_Img, &Sub_Idx, &Times);
        set_F_Temp_Stitch_SubData(Sub_Idx, -1, PIPE_SUBCODE_5S_CNT_0+i, -1, -1, -1);    // RAW
        //[].Smooth
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode, 0, 4, 9, 10, 0, &Sub_Idx, &Times);
        set_F_Temp_Smooth_SubData(Sub_Idx, -1);
        //[].Jpeg
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 4, 5, 7, FPGA_Speed[sen_mode].Jpeg, &Sub_Idx, &Times);
        set_F_Temp_Jpeg_SubData(Sub_Idx, F_Temp.Jpeg_Id, -1, PIPE_SUBCODE_5S_CNT_0+i, -1, jtbl_idx, -1, -1, -1);
        //[].H264
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode ,0, 4, 8, 7, 0, &Sub_Idx, &Times);
        set_F_Temp_H264_SubData(Sub_Idx, 0, 0, 0, 0);
        //[].DMA
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 4, 6, 10, 0, &Sub_Idx, &Times);
        set_F_Temp_DMA_SubData(Sub_Idx, -1, -1, -1, -1, -1);
        //[].USB
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 5, 7, 10,usb_spend_time, &Sub_Idx, &Times);
        set_F_Temp_USB_SubData(Sub_Idx, F_Com_In.encode_type, 1);


        if(i < 4) {
            F_Temp.Stitch.Page[0] = (F_Temp.Stitch.Page[0]+1) % 2;
            F_Temp.Smooth.Page[0] = (F_Temp.Smooth.Page[0]+1) % 2;
            F_Temp.Jpeg[F_Temp.Jpeg_Id].Page[0]   = (F_Temp.Jpeg[F_Temp.Jpeg_Id].Page[0]+1) % 2;
			F_Temp.Jpeg_Id   = (F_Temp.Jpeg_Id+1) % 2;
            F_Temp.USB.Page[0]    = (F_Temp.USB.Page[0]+1) % 2;
            F_Temp.Pic_No++;
        }
    }
    F_Temp.Jpeg[F_Temp.Jpeg_Id].Page[0]   = (F_Temp.Jpeg[F_Temp.Jpeg_Id].Page[0]+1) % 2;	//解JPEG做奇數次, 轉回小圖後JPEG引擎會用錯
    F_Temp.Jpeg_Id   = (F_Temp.Jpeg_Id+1) % 2;

    F_Temp.Sensor.Page[0] = (F_Temp.Sensor.Page[0]+1) % 2;
    F_Temp.ISP1.Page[0]   = (F_Temp.ISP1.Page[0]+1) % 3;
    F_Temp.ISP2.Page[0]   = (F_Temp.ISP2.Page[0]+1) % 2;
    F_Temp.Stitch.Page[0] = (F_Temp.Stitch.Page[0]+1) % 2;
    F_Temp.Smooth.Page[0] = (F_Temp.Smooth.Page[0]+1) % 2;
    F_Temp.Jpeg[F_Temp.Jpeg_Id].Page[0]   = (F_Temp.Jpeg[F_Temp.Jpeg_Id].Page[0]+1) % 2;
	F_Temp.Jpeg_Id   = (F_Temp.Jpeg_Id+1) % 2;
    F_Temp.USB.Page[0]    = (F_Temp.USB.Page[0]+1) % 2;

    if(Check_Flag == 1) {
        *Finish_Idx = Sub_Idx;
        *Sensor_Idx = sensor_idx_tmp;
    }

    Make_F_Pipe( Img_Mode, Check_Flag, 4);                      // F_Pipe_Add_RAW
    return Check_Flag;
};

int F_Pipe_Add_Defect(int *Finish_Idx, int *Sensor_Idx, int C_Mode)
{
    int i, time = 0;
    int Check_Flag = 1, Sub_Idx;
    int P0_Idx;
    struct FPGA_Engine_Resource_Struct *P1;
    int usb_spend_time;
    int p_mask = (FPGA_ENGINE_P_MAX-1);
    int btm_m=0, btm_s=100, cp_mode=0, np=0, btm_t_m=0;
    int shuter_time = F_Com_In.Shuter_Speed;
    int sen_mode = 1;
    int Img_Mode = 0;
    int sensor_idx_tmp = -1;
    int Times;
    usb_spend_time = 155985;
#ifdef ANDROID_CODE
    np = get_AEG_System_Freq_NP();
    shuter_time = get_A2K_Shuter_Speed();       // F_Pipe_Add_Big
#endif
    //if(C_Mode == 12)
        shuter_time = 1000000;

    P1 = &F_Pipe.Sensor;
    P0_Idx = P1->Idx;
    P1->P[P0_Idx].No       = -1;
    P1->P[P0_Idx].Time_Buf = -1;
    P1->P[P0_Idx].I_Page   = -1;
    P1->P[P0_Idx].O_Page   = -1;
    P1->P[P0_Idx].M_Mode   = Img_Mode;
    P1->Idx =  (P1->Idx + 1) & p_mask;

    memcpy(&F_Temp, &F_Pipe, sizeof(F_Pipe) );
    F_Pipe_Add_Sub(-1, -1, -1, -1, -1, -1, &Sub_Idx, &Times);           // debug
    if(btm_s != 0) F_Temp.DMA.Page[0] = F_Temp.Stitch.Page[0];  // rex+ 180626, 解DMA錯誤，拍成前一張畫面

    int jtbl_idx=0;
    int long_ep = (np == 0)? 66667: 80000;
    if(shuter_time <= long_ep){      // EP <= 1/15(66666) or 1/12.5(80000)
        for (i = 0; i < 3; i++) {
            //[].Sensor
            Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 1, 1, 2, shuter_time, &Sub_Idx, &Times);
            set_F_Temp_Sensor_SubData(Sub_Idx, -1, -1, 0, -1, -1, -1);  // small_f=0
            //[].ISP1
            Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 1, 2, 3, F_Com_In.Pipe_Time_Base, &Sub_Idx, &Times);
            /*if(C_Mode == 9) {		//Sport WDR
            	set_F_Temp_ISP1_SubData(Sub_Idx, -1, -1, Img_Mode, -1, WDR_Live_Page, 0);
                if(i == 0) Set_F_JPEG_Header_ISP1(Sub_Idx, &jtbl_idx);
                set_F_Temp_WDR_SubData(Sub_Idx, WDR_Live_Page, 1, 1);
            }
            else {*/
                set_F_Temp_ISP1_SubData(Sub_Idx, -1, -1, Img_Mode, -1, -1, 0);
                if(i == 0) Set_F_JPEG_Header_ISP1(Sub_Idx, &jtbl_idx);
                set_F_Temp_WDR_SubData(Sub_Idx, -1, -1, -1);
            //}
            sensor_idx_tmp = Sub_Idx;
            //[].ISP2, 0~3 ISP2_NR3D_Rate
            Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 2, 3, 4, FPGA_Speed[Img_Mode].ISP2, &Sub_Idx, &Times);
            set_F_Temp_ISP2_SubData(Sub_Idx, i, -1, -1, -1);
            if(i < 2) {         // isp2page idx要給下一級使用, 不能先加1
                F_Temp.Sensor.Page[0] = (F_Temp.Sensor.Page[0]+1) % 2;
                F_Temp.ISP1.Page[0]   = (F_Temp.ISP1.Page[0]+1) % 3;
                F_Temp.ISP2.Page[0]   = (F_Temp.ISP2.Page[0]+1) % 2;
                F_Temp.Pic_No++;
            }
        }
    }
    else{
        int ep_long_t = 1;
        int isp1_time;
        //if(C_Mode == 12)
            isp1_time = 1000000 + F_Com_In.Pipe_Time_Base*2;
        //else
        //    isp1_time = F_Com_In.Pipe_Time_Base*2;

        if(shuter_time > F_Com_In.Pipe_Time_Base){              // 解長時間拍照時會殘留上一張畫面(鋸齒狀)
            shuter_time -= F_Com_In.Pipe_Time_Base;
            isp1_time += F_Com_In.Pipe_Time_Base;
        }
        //[].Sensor
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 1, 1, 2, shuter_time, &Sub_Idx, &Times);
        set_F_Temp_Sensor_SubData(Sub_Idx, -1, ep_long_t, 0, -1, -1, -1);          // Add_Big, ep_long_t=n, small_f=0

        //[].ISP1
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 1, 2, 3, isp1_time, &Sub_Idx, &Times);     // Pipe_Time_Base*2, 解長時間會有鋸齒畫面
        /*if(C_Mode == 9) {		//Sport WDR
            set_F_Temp_ISP1_SubData(Sub_Idx, -1, -1, Img_Mode, -1, WDR_Live_Page, 0);
            Set_F_JPEG_Header_ISP1(Sub_Idx, &jtbl_idx);
            set_F_Temp_WDR_SubData(Sub_Idx, WDR_Live_Page, 1, 1);
        }
        else {*/
            set_F_Temp_ISP1_SubData(Sub_Idx, -1, -1, Img_Mode, -1, -1, 0);
            Set_F_JPEG_Header_ISP1(Sub_Idx, &jtbl_idx);
            set_F_Temp_WDR_SubData(Sub_Idx, -1, -1, -1);
        //}
        sensor_idx_tmp = Sub_Idx;

        //[].ISP2, 0~3 ISP2_NR3D_Rate
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 2, 3, 4, FPGA_Speed[Img_Mode].ISP2, &Sub_Idx, &Times);
        set_F_Temp_ISP2_SubData(Sub_Idx, 0, -1, -1, -1);

        // 恢復原狀, 拍完不閃爍
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 2, 1, 2, F_Com_In.Shuter_Speed, &Sub_Idx, &Times);
        set_F_Temp_Sensor_SubData(Sub_Idx, -1, -1, 0, -1, -1, -1);
    }

    for (i = 0; i < 5; i++) {
        //[].Stitch
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 3, 4, 5, FPGA_Speed[sen_mode].Stitch_Img, &Sub_Idx, &Times);
        set_F_Temp_Stitch_SubData(Sub_Idx, -1, PIPE_SUBCODE_5S_CNT_0+i, -1, -1, -1);    // Bad_Dot
        //[].Smooth
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode, 0, 4, 9, 10, 0, &Sub_Idx, &Times);
        set_F_Temp_Smooth_SubData(Sub_Idx, -1);
        //[].Jpeg
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 4, 5, 7, FPGA_Speed[sen_mode].Jpeg, &Sub_Idx, &Times);
        set_F_Temp_Jpeg_SubData(Sub_Idx, F_Temp.Jpeg_Id, -1, PIPE_SUBCODE_5S_CNT_0+i, -1, jtbl_idx, -1, -1, -1);
        //[].H264
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode ,0, 4, 8, 7, 0, &Sub_Idx, &Times);
        set_F_Temp_H264_SubData(Sub_Idx, 0, 0, 0, 0);
        //[].DMA
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 4, 6, 10, 0, &Sub_Idx, &Times);
        set_F_Temp_DMA_SubData(Sub_Idx, -1, -1, -1, -1, -1);
        //[].USB
        Check_Flag &= F_Pipe_Add_Sub(Img_Mode,0, 5, 7, 10,usb_spend_time, &Sub_Idx, &Times);
        set_F_Temp_USB_SubData(Sub_Idx, F_Com_In.encode_type, 1);

        if(i < 4) {
            F_Temp.Stitch.Page[0] = (F_Temp.Stitch.Page[0]+1) % 2;
            F_Temp.Smooth.Page[0] = (F_Temp.Smooth.Page[0]+1) % 2;
            F_Temp.Jpeg[F_Temp.Jpeg_Id].Page[0]   = (F_Temp.Jpeg[F_Temp.Jpeg_Id].Page[0]+1) % 2;
			F_Temp.Jpeg_Id   = (F_Temp.Jpeg_Id+1) % 2;
            F_Temp.USB.Page[0]    = (F_Temp.USB.Page[0]+1) % 2;
            F_Temp.Pic_No++;
        }
    }
    F_Temp.Jpeg[F_Temp.Jpeg_Id].Page[0]   = (F_Temp.Jpeg[F_Temp.Jpeg_Id].Page[0]+1) % 2;	//解JPEG做奇數次, 轉回小圖後JPEG引擎會用錯
    F_Temp.Jpeg_Id   = (F_Temp.Jpeg_Id+1) % 2;

    F_Temp.Sensor.Page[0] = (F_Temp.Sensor.Page[0]+1) % 2;
    F_Temp.ISP1.Page[0]   = (F_Temp.ISP1.Page[0]+1) % 3;
    F_Temp.ISP2.Page[0]   = (F_Temp.ISP2.Page[0]+1) % 2;
    F_Temp.Stitch.Page[0] = (F_Temp.Stitch.Page[0]+1) % 2;
    F_Temp.Smooth.Page[0] = (F_Temp.Smooth.Page[0]+1) % 2;
    F_Temp.Jpeg[F_Temp.Jpeg_Id].Page[0]   = (F_Temp.Jpeg[F_Temp.Jpeg_Id].Page[0]+1) % 2;
    F_Temp.Jpeg_Id   = (F_Temp.Jpeg_Id+1) % 2;
    F_Temp.DMA.Page[0]    = (F_Temp.DMA.Page[0]+1) % 2;
    F_Temp.USB.Page[0]    = (F_Temp.USB.Page[0]+1) % 2;

    if(Check_Flag == 1) {
        *Finish_Idx = Sub_Idx;                      // 紀錄最後一筆 cmd_idx
        *Sensor_Idx = sensor_idx_tmp;
    }

    F_Pipe_Add_Sub(-2, -2, -2, -2, -2, -2, &Sub_Idx, &Times);
    Make_F_Pipe( Img_Mode, Check_Flag, C_Mode);                 // F_Pipe_Add_Big
    return Check_Flag;
};

int F_Pipe_Add_Timelapse_DMA_Bottom(int Big_Mode, int Check_Flag, int btm_m, int btm_t_m, int btm_s, int cp_mode, int *dma_p0, int *jpeg_p0, int *h264_p0, int enc)
{
	int Sub_Idx, Times, dma_time;
	int ck_flag = Check_Flag;
	int dma_p1, dma_p2;
	if(Big_Mode >= 3) {
		dma_p1 = 106;
		if(enc == 0 || enc == 2) dma_p2 = 105;		//JPEG
		else					 dma_p2 = 108;		//H264
	}
	else {
		dma_p1 = 6;
		if(enc == 0 || enc == 2) dma_p2 = 5;		//JPEG
		else					 dma_p2 = 8;		//H264
	}
	if(btm_m == 0 && btm_t_m == 0) {
		if(Big_Mode > 2) ck_flag &= F_Pipe_Add_Sub(Big_Mode,0, 4, dma_p1, 10, 0, &Sub_Idx, &Times);
		*dma_p0=4; *jpeg_p0=4; *h264_p0=4;
	}
	else {		//DMA底圖
		switch(btm_m) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4: if(Big_Mode >= 3) F_Temp.DMA.Page[1] = F_Temp.Stitch.Page[1];
				else			  F_Temp.DMA.Page[0] = F_Temp.Stitch.Page[0];
				if(btm_m == 3) dma_time = FPGA_Speed[Big_Mode+1].DMA;		//解時間過長導致縫合和壓縮同時讀寫問題, FPGA_Speed[Big_Mode].DMA;		//鏡像
				else           dma_time = FPGA_Speed[5].DMA;
				ck_flag &= F_Pipe_Add_Sub(Big_Mode,1, 4, dma_p1, dma_p2, dma_time, &Sub_Idx, &Times);                  //[].DMA
				if(Big_Mode == 0) {		//12K
					set_F_Temp_DMA_SubData(Sub_Idx, btm_m, btm_s, cp_mode, btm_t_m, 0);
					if(btm_t_m == 1 && (btm_m == 1 || btm_m == 2 || btm_m == 3) ) {
						if(Big_Mode >= 3) F_Temp.DMA.Page[1] = F_Temp.Stitch.Page[1];
						else			  F_Temp.DMA.Page[0] = F_Temp.Stitch.Page[0];
						dma_time = FPGA_Speed[5].DMA;
						ck_flag &= F_Pipe_Add_Sub(Big_Mode,1, 6, dma_p1, dma_p2, dma_time, &Sub_Idx, &Times);                  //[].DMA
						set_F_Temp_DMA_SubData(Sub_Idx, btm_m, btm_s, cp_mode, btm_t_m, 1);
					}
				}
				else if(Big_Mode == 1 || Big_Mode == 2) {		//8K 6K
					if(btm_t_m == 1 && (btm_m == 1 || btm_m == 2 || btm_m == 3) )
						set_F_Temp_DMA_SubData(Sub_Idx, btm_m, btm_s, cp_mode, btm_t_m, 5);						//8K 6K 底圖合併DMA
					else
						set_F_Temp_DMA_SubData(Sub_Idx, btm_m, btm_s, cp_mode, btm_t_m, 0);
				}
				else {		//4K
					if(btm_t_m == 1 && (btm_m == 1 || btm_m == 2 || btm_m == 3) )
						set_F_Temp_DMA_SubData(Sub_Idx, btm_m, btm_s, cp_mode, btm_t_m, 4);
					else
						set_F_Temp_DMA_SubData(Sub_Idx, btm_m, btm_s, cp_mode, btm_t_m, 0);
				}
				break;
		}
		*dma_p0=6; *jpeg_p0=6; *h264_p0=6;
	}
	return ck_flag;
}

void F_Pipe_Add_Timelapse(int Big_Mode, int *Finish_Idx, int C_Mode, int Small_Mode)
{
    int Check_Flag, Sub_Idx;
    static int yuvz_st_idx = 0;
    int yuvz_en = -1;
    int Stitch_All;
	int Now_Mode, S_Mode;
    int c_mode=0, t_mode=0, m_mode=0, s_mode=0, tl_ms=0, f2_ms=100;
    int h264_p1_idx=-1, h264_p2_idx=-1;
    static int h264_p_lst=-1, sub_p=0;
    int f2_base_time = 100;		//100ms
    int do_h264_flag=0;
    int h264_time, usb_time;
    int IP_M = 0, KeyInterval = 8, Frame_Stamp_Init = 0;
    int Times;
    static unsigned F_Cnt = 0, Frame_Stamp = 0;
	int btm_m, btm_s=100, btm_t_m, cp_mode;
	int dma_time, dma_p0=4, jpeg_p0=4, h264_p0=4;
    int sen_p1, sen_p2, isp1_p1, isp1_p2;
    int isp2_p1, isp2_p2, st_p1, st_p2;
    int jpeg_p1, jpeg_p2, h264_p1, h264_p2;
    int usb_p1, usb_p2;

	Get_M_Mode(&Now_Mode, &S_Mode);
#ifdef ANDROID_CODE
    get_A2K_Live_CMD(&c_mode, &t_mode, &m_mode, &s_mode);
    get_A2K_DMA_BottomMode(&btm_m, &btm_s);
    get_A2K_DMA_BottomTextMode(&btm_t_m);
    get_A2K_DMA_CameraPosition(&cp_mode);
#endif

	switch(t_mode) {
	case 0: tl_ms =     0; break;
	case 1: tl_ms =   900; break;
	case 2: tl_ms =  1900; break;
	case 3: tl_ms =  4900; break;
	case 4: tl_ms =  9900; break;
	case 5: tl_ms = 29900; break;
	case 6: tl_ms = 59900; break;
	case 7:    								// Full Spees(0.1s)
		if(m_mode == 0)      tl_ms = 900;	//12K
		else if(m_mode == 1) tl_ms = 490;	//8K
		else if(m_mode == 2) tl_ms = 290;	//6K
		else if(m_mode == 3) tl_ms =  90;	//4K
		break;
	default: tl_ms =    0; break;
	}

    Stitch_All = (FPGA_Speed[Big_Mode].Stitch_Img + FPGA_Speed[Big_Mode].Stitch_ZYUV * 3);
    yuvz_en = PIPE_SUBCODE_ST_YUVZ_ALL;

	if(Big_Mode >= 3) {
		sen_p1  = 101; sen_p2  = 102;
		isp1_p1 = 102; isp1_p2 = 103;
		isp2_p1 = 103; isp2_p2 = 104;
		st_p1   = 104;
		if(F_Com_In.encode_type == 0 || F_Com_In.encode_type == 2)
			st_p2 = 105;
		else
			st_p2 = 108;
		jpeg_p1 = 105; jpeg_p2 = 107;
		h264_p1 = 108; h264_p2 = 107;
		usb_p1  = 107; usb_p2  = 110;
	}
	else {
		sen_p1  = 1; sen_p2  = 2;
		isp1_p1 = 2; isp1_p2 = 3;
		isp2_p1 = 3; isp2_p2 = 4;
		st_p1   = 4;
		if(F_Com_In.encode_type == 0 || F_Com_In.encode_type == 2)
			st_p2 = 5;
		else
			st_p2 = 8;
		jpeg_p1 = 5; jpeg_p2 = 7;
		h264_p1 = 8; h264_p2 = 7;
		usb_p1  = 7; usb_p2  = 10;
	}

    Check_Flag = 1;
    memcpy(&F_Temp, &F_Pipe, sizeof(F_Pipe) );
    //[].Sensor
    Check_Flag &= F_Pipe_Add_Sub( Big_Mode ,1, 1, sen_p1, sen_p2, F_Com_In.Shuter_Speed, &Sub_Idx, &Times);
    set_F_Temp_Sensor_SubData(Sub_Idx, -1, -1, 0, -1, -1, -1);
    int jtbl_idx=0;
    if(/*C_Mode == 2 ||*/ C_Mode == 11) {		//Time Lapse WDR
        if(Big_Mode >= 3) {
            //[].ISP1
            Check_Flag &= F_Pipe_Add_Sub( Big_Mode ,1, 2, isp1_p1, isp1_p2, F_Com_In.Pipe_Time_Base, &Sub_Idx, &Times);
            set_F_Temp_ISP1_SubData(Sub_Idx, -1, -1, Now_Mode, 1, WDR_Live_Page, 1);
            Set_F_JPEG_Header_ISP1(Sub_Idx, &jtbl_idx);
            set_F_Temp_WDR_SubData(Sub_Idx, WDR_Live_Page, 1, 1);
            WDR_Live_Page = (WDR_Live_Page+1) & 0x1;
        }
        else {
            //[].ISP1,	關閉Sensor Binn & FPGA Binn, 只開啟WDR
            Check_Flag &= F_Pipe_Add_Sub( Big_Mode ,1, 1, isp1_p1, isp1_p1, F_Com_In.Pipe_Time_Base, &Sub_Idx, &Times);
            set_F_Temp_ISP1_SubData(Sub_Idx, -1, -1, Now_Mode, 0, WDR_Live_Page, 1);
            set_F_Temp_WDR_SubData(Sub_Idx, WDR_Live_Page, -1, 1);
            WDR_Live_Page = (WDR_Live_Page+1) & 0x1;
            F_Temp.ISP1.Page[0]   = (F_Temp.ISP1.Page[0]+1) % 3;
            //[].ISP1
            Check_Flag &= F_Pipe_Add_Sub( Big_Mode ,1, 2, isp1_p1, isp1_p2, F_Com_In.Pipe_Time_Base, &Sub_Idx, &Times);
            set_F_Temp_ISP1_SubData(Sub_Idx, -1, -1, Now_Mode, 1, WDR_Live_Page, 1);
            Set_F_JPEG_Header_ISP1(Sub_Idx, &jtbl_idx);
            set_F_Temp_WDR_SubData(Sub_Idx, WDR_Live_Page, 1, 1);
            WDR_Live_Page = (WDR_Live_Page+1) & 0x1;
        }
    }
    else {
    	//[].ISP1
        Check_Flag &= F_Pipe_Add_Sub( Big_Mode ,1, 2, isp1_p1, isp1_p2, F_Com_In.Pipe_Time_Base, &Sub_Idx, &Times);
        set_F_Temp_ISP1_SubData(Sub_Idx, -1, -1, Now_Mode, -1, -1, 1);
        Set_F_JPEG_Header_ISP1(Sub_Idx, &jtbl_idx);
        set_F_Temp_WDR_SubData(Sub_Idx, -1, -1, -1);
    }
    //[].ISP2
    Check_Flag &= F_Pipe_Add_Sub( Big_Mode ,1, 2, isp2_p1, isp2_p2, FPGA_Speed[Big_Mode].ISP2, &Sub_Idx, &Times);
    set_F_Temp_ISP2_SubData(Sub_Idx, -1, -1, -1, -1);                                   // Timelapse

    if(F_Com_In.encode_type == 0 || F_Com_In.encode_type == 2) {		//JPEG
    	usb_time  = FPGA_Speed[Big_Mode].USB;
    	if(t_mode == 1) {
			if((tl_ms*1000) > usb_time)
				usb_time = (tl_ms*1000);
    	}

		//[].Stitch
		Check_Flag &= F_Pipe_Add_Sub( Big_Mode ,1, 3, st_p1, st_p2,Stitch_All, &Sub_Idx, &Times);
		set_F_Temp_Stitch_SubData(Sub_Idx, yuvz_en, -1, -1, -1, -1);                // Timelapse
        //[].Smooth
        Check_Flag &= F_Pipe_Add_Sub( Big_Mode, 0, 4, 9, 10, 0, &Sub_Idx, &Times);
		set_F_Temp_Smooth_SubData(Sub_Idx, -1);
		//[].DMA
		Check_Flag &= F_Pipe_Add_Timelapse_DMA_Bottom(Big_Mode, Check_Flag, btm_m, btm_t_m, btm_s, cp_mode, &dma_p0, &jpeg_p0, &h264_p0, F_Com_In.encode_type);
		//[].Jpeg
		Check_Flag &= F_Pipe_Add_Sub( Big_Mode ,1, jpeg_p0, jpeg_p1, jpeg_p2,FPGA_Speed[Big_Mode].Jpeg, &Sub_Idx, &Times);
		set_F_Temp_Jpeg_SubData(Sub_Idx, F_Temp.Jpeg_Id, -1, -1, -1, jtbl_idx, -1, -1, -1);
		//[].H264
		Check_Flag &= F_Pipe_Add_Sub( Big_Mode ,1, h264_p0, h264_p1, 10, 0, &Sub_Idx, &Times);
		set_F_Temp_H264_SubData(Sub_Idx, 0, 0, 0, 0);
		//[].USB
		Check_Flag &= F_Pipe_Add_Sub( Big_Mode ,1, 5, usb_p1, usb_p2,usb_time, &Sub_Idx, &Times);
		set_F_Temp_USB_SubData(Sub_Idx, F_Com_In.encode_type, 1);

		h264_p1_idx = 0; h264_p2_idx = 0; h264_p_lst = -1; sub_p = 0;
		F_Cnt = 0; Frame_Stamp = 0;
    }
    else {		//H264
#ifdef ANDROID_CODE
    	get_A2K_H264_Init(&Frame_Stamp_Init);
#endif
    	if(Frame_Stamp_Init == 1) {
    	  	Frame_Stamp = 0;
#ifdef ANDROID_CODE
    	  	set_A2K_H264_Init(0);
#endif
    	}
    	if( (Frame_Stamp % KeyInterval) == 0) { IP_M = 0; F_Cnt = 0; }
    	else 								  { IP_M = 1;            }

    	if(IP_M == 0) {		//I Frame
			h264_time = FPGA_Speed[Big_Mode].H264;
			usb_time  = FPGA_Speed[Big_Mode].USB_H264;
    	}
    	else {
			h264_time = FPGA_Speed[Big_Mode].H264 * 0.56;		// * 0.4
			usb_time  = FPGA_Speed[Big_Mode].USB_H264 * 3;		// * 2
    	}

	    //[].Stitch
		Check_Flag &= F_Pipe_Add_Sub( Big_Mode ,1, 3, st_p1, st_p2,Stitch_All, &Sub_Idx, &Times);
		set_F_Temp_Stitch_SubData(Sub_Idx, yuvz_en, -1, -1, -1, -1);                // Timelapse
        //[].Smooth
        Check_Flag &= F_Pipe_Add_Sub( Big_Mode, 0, 4, 9, 10, 0, &Sub_Idx, &Times);
		set_F_Temp_Smooth_SubData(Sub_Idx, -1);
		//[].DMA
		Check_Flag &= F_Pipe_Add_Timelapse_DMA_Bottom(Big_Mode, Check_Flag, btm_m, btm_t_m, btm_s, cp_mode, &dma_p0, &jpeg_p0, &h264_p0, F_Com_In.encode_type);
		//[].Jpeg
		Check_Flag &= F_Pipe_Add_Sub( Big_Mode ,1, jpeg_p0, jpeg_p1, 10, 0, &Sub_Idx, &Times);
		set_F_Temp_Jpeg_SubData(Sub_Idx, F_Temp.Jpeg_Id, -1, -1, -1, jtbl_idx, -1, -1, -1);
		//[].H264
		Check_Flag &= F_Pipe_Add_Sub( Big_Mode ,1, h264_p0, h264_p1, h264_p2, h264_time, &Sub_Idx, &Times);
		h264_p1_idx = Sub_Idx;
		//set_F_Temp_H264_SubData(Sub_Idx, do_h264_flag, IP_M, Frame_Stamp, F_Cnt);
		//[].USB
		Check_Flag &= F_Pipe_Add_Sub( Big_Mode ,1, 8, usb_p1, usb_p2,usb_time, &Sub_Idx, &Times);
		h264_p2_idx = Sub_Idx;
		//FPGA H264間 隔 tl_ms 壓一張
		if(Check_Flag == 1 && h264_p2_idx != -1) {
			if(h264_p_lst == -1) {
				do_h264_flag = 1;
				h264_p_lst = h264_p2_idx;
			}
			else {
				if(h264_p2_idx > h264_p_lst)
					sub_p += (h264_p2_idx - h264_p_lst);
				else if(h264_p_lst > h264_p2_idx)
					sub_p += (FPGA_ENGINE_P_MAX - h264_p_lst + h264_p2_idx);
				h264_p_lst = h264_p2_idx;

				if( (sub_p * f2_base_time) > tl_ms) {
					do_h264_flag = 1;
					sub_p = 0;
				}
				else
					do_h264_flag = 0;
			}
		}
		else
			do_h264_flag = 0;
		set_F_Temp_H264_SubData(h264_p1_idx, do_h264_flag, IP_M, Frame_Stamp, F_Cnt);
		set_F_Temp_USB_SubData(h264_p2_idx, F_Com_In.encode_type, do_h264_flag);

		if(do_h264_flag == 1) {
			F_Cnt++; Frame_Stamp++;
			if(Frame_Stamp > 0xFFFFFFF0) Frame_Stamp = 0;
		}
    }

    if(F_Com_In.encode_type == 0 || F_Com_In.encode_type == 2) {
		F_Temp.Jpeg[F_Temp.Jpeg_Id].Page[0]   = (F_Temp.Jpeg[F_Temp.Jpeg_Id].Page[0]+1) % 2;
		F_Temp.Jpeg_Id   = (F_Temp.Jpeg_Id+1) % 2;
    	F_Temp.Jpeg[F_Temp.Jpeg_Id].Page[0]   = (F_Temp.Jpeg[F_Temp.Jpeg_Id].Page[0]+1) % 2;						//解縮時切其他MODE, JPEG PAGE使用錯誤問題
    	F_Temp.Jpeg[ (F_Temp.Jpeg_Id+1)%2 ].Page[1]   = (F_Temp.Jpeg[ (F_Temp.Jpeg_Id+1)%2 ].Page[1]+1) % 2;		//解縮時切其他MODE, JPEG PAGE使用錯誤問題
    }
    else {
    	F_Temp.H264.Page[0]   = (F_Temp.H264.Page[0]+1) % 2;
    }
    F_Temp.USB.Page[0]    = (F_Temp.USB.Page[0]+1) % 2;

    if(Big_Mode <= 2 && (F_Com_In.encode_type == 0 || F_Com_In.encode_type == 2)/*&& get_A2K_Rec_State_CMD() == -2*/) {        //JPEG 12K 8K 6K 才需要DMA
        //[].DMA
    	Check_Flag &= F_Pipe_Add_Sub( Big_Mode ,1, dma_p0, 106,105,FPGA_Speed[Big_Mode].DMA, &Sub_Idx, &Times);
        set_F_Temp_DMA_SubData(Sub_Idx, -1, -1, -1, -1, 6);
        //[].Jpeg
        Check_Flag &= F_Pipe_Add_Sub(Small_Mode,1, 106,105,107,FPGA_Speed[Small_Mode].Jpeg, &Sub_Idx, &Times);
        set_F_Temp_Jpeg_SubData(Sub_Idx, F_Temp.Jpeg_Id, -1, -1, -1, jtbl_idx, -1, -1, -1);
        //[].USB
        Check_Flag &= F_Pipe_Add_Sub(Small_Mode,0, 105,107,110,FPGA_Speed[Small_Mode].USB, &Sub_Idx, &Times);
        set_F_Temp_USB_SubData(Sub_Idx, F_Com_In.encode_type, 1);

        F_Temp.DMA.Page[1]    = (F_Temp.DMA.Page[1]+1) % 3;
    	F_Temp.Jpeg[F_Temp.Jpeg_Id].Page[1]   = (F_Temp.Jpeg[F_Temp.Jpeg_Id].Page[1]+1) % 2;
    	F_Temp.Jpeg_Id   = (F_Temp.Jpeg_Id+1) % 2;
    	F_Temp.USB.Page[1]    = (F_Temp.USB.Page[1]+1) % 2;
    }

    if(Big_Mode >= 3) {
        F_Temp.Sensor.Page[1] = (F_Temp.Sensor.Page[1]+1) % 2;
        F_Temp.ISP1.Page[1]   = (F_Temp.ISP1.Page[1]+1) % 2;
        F_Temp.ISP2.Page[1]   = (F_Temp.ISP2.Page[1]+1) % 2;
		F_Temp.Stitch.Page[1] = (F_Temp.Stitch.Page[1]+1) % 3;
		F_Temp.Smooth.Page[1] = (F_Temp.Smooth.Page[1]+1) % 3;
		if(btm_m != 0 || btm_t_m != 0)
			F_Temp.DMA.Page[1]    = (F_Temp.DMA.Page[1]+1) % 3;
    }
    else {
    	F_Temp.Sensor.Page[0] = (F_Temp.Sensor.Page[0]+1) % 2;
    	F_Temp.ISP1.Page[0]   = (F_Temp.ISP1.Page[0]+1) % 3;
    	F_Temp.ISP2.Page[0]   = (F_Temp.ISP2.Page[0]+1) % 2;
    	F_Temp.Stitch.Page[0] = (F_Temp.Stitch.Page[0]+1) % 2;
    	F_Temp.Smooth.Page[0] = (F_Temp.Smooth.Page[0]+1) % 2;
		if(btm_m != 0 || btm_t_m != 0)
			F_Temp.DMA.Page[0]    = (F_Temp.DMA.Page[0]+1) % 2;
    }

    if(Check_Flag == 1)
        *Finish_Idx = Sub_Idx;
    F_Temp.Pic_No++;
    Make_F_Pipe( Big_Mode, Check_Flag, C_Mode);                 // F_Pipe_Add_Timelapse
};

int get_HW_Idx_B64(void)
{
    int hw_idx = readSyncIdx();
#ifdef ANDROID_CODE
    SetWaveDebug(0, hw_idx);                //F_Pipe.HW_Idx_B64;
#endif
    return hw_idx;
};
/*
 * return: 1 -> check ok
 *        -1 -> check failed
 */
int check_F_Com_In(void)
{
    if(F_Com_In_Buf.Pipe_Init != 0xaaaa5555) 
        return -1;

    return 1;
}

void F_Cmd_In_Capture_Start(void)
{
	int i;
	F_Com_In.Capture_T1 = 0;
	F_Com_In.Capture_T2 = 0;
	F_Com_In.Capture_Step = 0;
	F_Com_In.Capture_Lose_Flag = 0;
	F_Com_In.Capture_Img_Err_Cnt = 0;
	for(i = 0; i < 2; i++) {
		F_Com_In.Capture_Get_Img[i] = 0;
		F_Com_In.Capture_Lose_Page[i] = 0;
	}
}

void Set_F_Cmd_In_Capture_T1(void)
{
//tmp	get_current_usec(&F_Com_In.Capture_T1);
	db_debug("Set_F_Cmd_In_Capture_T1() T1=%lld\n", F_Com_In.Capture_T1);
}
void Get_F_Cmd_In_Capture_T1(unsigned long long *t1)
{
	*t1 =  F_Com_In.Capture_T1;
}

void Set_F_Cmd_In_Capture_T2(void)
{
//tmp	get_current_usec(&F_Com_In.Capture_T2);
	db_debug("Set_F_Cmd_In_Capture_T2() T2=%lld\n", F_Com_In.Capture_T2);
}
void Get_F_Cmd_In_Capture_T2(unsigned long long *t2)
{
	*t2 =  F_Com_In.Capture_T2;
}

void Set_F_Cmd_In_Capture_Get_Img(int idx, int flag)
{
	F_Com_In.Capture_Get_Img[idx&0x1] = flag;
	db_debug("Set_F_Cmd_In_Capture_Get_Img() idx=%d flag=%d\n", idx, F_Com_In.Capture_Get_Img[idx&0x1]);
}
int Get_F_Cmd_In_Capture_Get_Img(int idx)
{
	return F_Com_In.Capture_Get_Img[idx&0x1];
}

void Set_F_Cmd_In_Capture_Step(int step)
{
	F_Com_In.Capture_Step = step;
	db_debug("Set_F_Cmd_In_Capture_Step() Step=%d\n", F_Com_In.Capture_Step);
}
int Get_F_Cmd_In_Capture_Step(void)
{
	return F_Com_In.Capture_Step;
}

void Set_F_Cmd_In_Capture_Lose_Flag(int flag)
{
	F_Com_In.Capture_Lose_Flag = flag;
	db_debug("Set_F_Cmd_In_Capture_Lose_Flag() flag=%d\n", F_Com_In.Capture_Lose_Flag);
}
int Get_F_Cmd_In_Capture_Lose_Flag(void)
{
	return F_Com_In.Capture_Lose_Flag;
}

void Set_F_Cmd_In_Capture_Lose_Page(int idx, int page)
{
	F_Com_In.Capture_Lose_Page[idx&0x1] = page;
}
int Get_F_Cmd_In_Capture_Lose_Page(int idx)
{
	return F_Com_In.Capture_Lose_Page[idx&0x1];
}

void Set_F_Cmd_In_Capture_Img_Err_Cnt(int cnt)
{
	F_Com_In.Capture_Img_Err_Cnt = cnt;
	db_debug("Set_F_Cmd_In_Capture_Img_Err_Cnt() cnt=%d\n", F_Com_In.Capture_Img_Err_Cnt);
}
int Get_F_Cmd_In_Capture_Img_Err_Cnt(void)
{
	return F_Com_In.Capture_Img_Err_Cnt;
}

int read_F_Pipe_HW_Idx(void)
{
    return F_Pipe.HW_Idx_B64;
}
int Next_Cmd_Delay = 0;
int get_Pipe_Next_Cmd_Delay(void)
{
    return Next_Cmd_Delay;
}
void Set_New_Pipe_Idx(int c_mode)
{
    int p_mask = (FPGA_ENGINE_P_MAX-1);
    int p_max = FPGA_ENGINE_P_MAX>>2;
    //int usb_dt = (get_USB_Spend_Time(F_Com_In.Big_Mode) + F_Pipe.Pipe_Time_Base - 10000) / F_Pipe.Pipe_Time_Base;
    int s_idx;	//(F_Pipe.Cmd_Idx + usb_dt) & p_mask;                     //F_Pipe.USB.Idx;
    /*if(c_mode == 12)*/ s_idx = (F_Pipe.Cmd_Idx+1) & p_mask;				//解M-Mode亮度錯誤問題
    F_Pipe.Sensor.Idx  = s_idx;
    F_Pipe.ISP1.Idx    = s_idx;
    F_Pipe.ISP2.Idx    = s_idx;
    F_Pipe.Stitch.Idx  = s_idx;
    F_Pipe.Jpeg[0].Idx = s_idx;
    F_Pipe.Jpeg[1].Idx = s_idx;
    F_Pipe.DMA.Idx     = s_idx;
    //F_Pipe.USB.Idx     = s_idx;
}
int State_Bulb_Exp = 0;
int State_Removal_Step = 0;
void F_Pipe_Run(void)
{
    int i, idx;
    int Len1, Len2, Len3, Len4;
    static unsigned long long curTime, lstTime=0, cycTime, bstTime=0, shwTime=0;
    static int SW_Page=-1, HW_Page=-1, Lst_Idx=-1;
    static int Idx_B64, Cmd_Finish_Idx;
#ifdef ANDROID_CODE
//tmp    get_current_usec(&curTime);
    F_Pipe.HW_Idx_B64 = get_HW_Idx_B64();
#else
	F_Pipe.HW_Idx_B64 = (F_Pipe.Cmd_Idx - 1) & 0x7F;
#endif
    int c_mode = F_Com_In.Record_Mode;
//tmp  int picture = get_C_Mode_Picture(c_mode);
	int picture = 1;
    int isp1_idx = 0;
    int shuter_time, ts_max=0, cnt;

    if (F_Com_In.Checkcode == 0x55aa55aa) {
        if (F_Com_In_Buf.Pipe_Init != 0xaaaa5555) {
        	init_F_Temp_Sensor_SubData();
            init_F_Temp_WDR_SubData();          // rex+ 181107
            init_F_Temp_Smooth_SubData();
            FPGA_Pipe_Init(0);
            F_Com_In_Buf.Pipe_Init = 0xaaaa5555;
            Cmd_Finish_Idx = F_Pipe.Cmd_Idx;
        }
        if (Do_Pipe_Init == 1){
            Do_Pipe_Init = 0;
            FPGA_Pipe_Init(1);
        }

        //if (F_Com_In.Run_ID != F_Com_In_Buf.Run_ID) {
        //    F_Com_In_Buf.Run_ID = F_Com_In.Run_ID;
/*
//  Len = (F_Temp.USB.Idx - F_Temp.Sensor.Idx) & 0x3f;
//  if (Len > 50) ...                                             // 限制一批CMD最大50*100ms
//  Len = (F_Temp.Sensor.Idx - F_Pipe.Sensor.Idx) & 0x3f;         // 限制最大可下6張畫面的CMD, 6*100ms
//  if (Len > 6) ...
*/
            int p_mask = (FPGA_ENGINE_P_MAX-1);                   // FPGA_ENGINE_P_MAX=128
            int p_limit = (FPGA_ENGINE_P_MAX>>2)*3;

            if(SW_Page == -1 && HW_Page == -1 && Lst_Idx == -1) {  // rex+ 180628, init
                SW_Page = (F_Pipe.Cmd_Idx>>6);
                HW_Page = SW_Page;
                Lst_Idx = ((F_Pipe.HW_Idx_B64+(HW_Page*64)) & p_mask);
                Idx_B64 = F_Pipe.HW_Idx_B64;
            }
            SW_Page = (F_Pipe.Cmd_Idx>>6);      // Idx/64
            if(rst_HW_Page_En == 1){
                rst_HW_Page_En = 0;
                db_debug("rst_HW_Page: Idx={%d,%d} Page={%d,%d}\n", F_Pipe.Cmd_Idx, F_Pipe.HW_Idx_B64+(HW_Page*64), SW_Page, HW_Page);
                HW_Page = SW_Page;
                if(F_Pipe.HW_Idx_B64+(HW_Page*64) > F_Pipe.Cmd_Idx){
                    HW_Page -= 1;
                    HW_Page &= 0x3;
                }
            }

            Len1 = (F_Pipe.Cmd_Idx - (F_Pipe.HW_Idx_B64+(HW_Page*64))) & p_mask;

            Check_HDR7P_Auto_Read_En(HDR7P_Auto_P.Step, (F_Pipe.HW_Idx_B64+(HW_Page*64)), F_Com_In.Record_Mode);

            if((SW_Page != HW_Page) && (F_Pipe.HW_Idx_B64 < Idx_B64)){
                HW_Page ++;
                HW_Page &= 0x3;
                if(Len1 >= 64){
                    Len1 -= 64;
                }
            }
            Idx_B64 = F_Pipe.HW_Idx_B64;
            SetWaveDebug(3, (Len1<<8) | (F_Pipe.HW_Idx_B64+(HW_Page*64)));

            for(i == 0; i < p_mask; i++){         // 判斷HW是否執行完畢
                if(Lst_Idx == ((F_Pipe.HW_Idx_B64+(HW_Page*64)) & p_mask))
                    break;
#ifdef ANDROID_CODE
                run_Line_YUV_Offset_Step_S1S4(Lst_Idx);     // check HW_Idx
#endif
                Lst_Idx ++;
                Lst_Idx &= p_mask;
            }

            if(Cap_Finish_Idx != -1) Len2 = 16 - Len1;           // 拍照中
            else                     Len2 = F_Pipe.FPGA_Queue_Max - Len1;             // FPGA_Queue_Max=9
            if(Len1 > p_limit || Len2 < -16){
                cycTime = curTime - lstTime;
                db_debug("F_Pipe_Run: err! Idx={%d,%d} Len{%d,%d} Page={%d,%d} cycTime=%lld\n", 
                        F_Pipe.Cmd_Idx, (F_Pipe.HW_Idx_B64+(HW_Page*64)), Len1, Len2, 
                        SW_Page, HW_Page, cycTime);
            }
            lstTime = curTime;
            int capture = 1;
            for(i = 0; i < Len2; i++){          // 填滿10道FPGA_Queue, F_Pipe.FPGA_Queue_Max=10
            	ts_max = 0;
            	if(F_JPEG_Header.Init_Flag == 0) {
            		F_JPEG_Header.Init_Flag = 1;
            		Set_F_JPEG_Header_Now( (HW_Page << 6) + F_Pipe.HW_Idx_B64);
            	}
            	Set_F_JPEG_Header_Now_Time( (HW_Page << 6) + F_Pipe.HW_Idx_B64);

            	if(F_Com_In.Capture_Step == 2) continue;

                if(State_Bulb_Exp > 0){                         // rex+ 190108, B快門啟動
                    if(curTime < bstTime) bstTime = curTime;    // rex+ 防止例外錯誤
                    else if((curTime - bstTime) >= State_Bulb_Exp){
                        State_Bulb_Exp = 0;
                        db_debug("F_Pipe_Run: State_Bulb_Exp=%dms BstTime=%lldms\n", State_Bulb_Exp/1000, (curTime-bstTime)/1000);
                    }
                    else{
                        if((curTime - shwTime) >= 1000000){
                            shwTime = curTime;
                            db_debug("F_Pipe_Run: Bulb_Exp=%ds BstTime=%llds\n", State_Bulb_Exp/1000000, (curTime-bstTime)/1000000);
                        }
                        return;
                    }
                }
                if(picture == 1){           // c_mode=0,3,4,5,6,7,8,9,12,13
                    //FPGA_Pipe: F_Pipe_Run: Removal=1 D_Cnt=1 Finish=250 Busy=0 Cmd_Idx=242 capture=0
                    if(F_Com_In.Capture_D_Cnt <= 0 || (Cap_Finish_Idx != -1 && State_Removal_Step == 0) || read_Sensor_Busy() == 1){        // 小圖模式(live)
                        if(Next_Cmd_Delay > 0){
                            Next_Cmd_Delay --;
                            if(Next_Cmd_Delay == 0)
                            	Set_New_Pipe_Idx(c_mode);		//防止Cmd_Idx超過F_Pipe..Idx
                        }
                        else{
                            F_Pipe_Add_Small(F_Com_In.Small_Mode, F_Com_In.Record_Mode, 0, &isp1_idx, &shuter_time, &ts_max);
                            if(show_small_debug_en == 1) show_small_debug_en = 0;
                        }
                        capture = 0; // 沒有按下快門
                    }
                    else{
                        //db_debug("F_Pipe_Run: mode=%d len1=%d max=%d\n", F_Com_In.Record_Mode, Len1, F_Pipe.FPGA_Queue_Max);
                        if(show_small_debug_en == 0) show_small_debug_en = 1;
                    }
                    if(Check_Is_HDR_Auto() == 1 && HDR7P_Auto_P.Step != 0){
                        Len4 = (F_Temp.Sensor.Idx - F_Pipe.Cmd_Idx) & p_mask;

                        db_debug("F_Pipe_Run: Is_HDR_Auto=1 Idx={%d,%d} Step=%d Len1=%d Len2=%d Len4=%d\n",
                            F_Pipe.Cmd_Idx, (F_Pipe.HW_Idx_B64+(HW_Page*64)), HDR7P_Auto_P.Step, Len1, Len2, Len4);

                        int qmax = F_Pipe.FPGA_Queue_Max;
                        if(Len4 > 11 && Len4 < (qmax>>1)){      // 小畫面最後一道Sensor.Idx, 不能差距Cmd_Idx太多, 會造成delay
                            capture = 0;
                        }
                    }

                    if(capture == 1 && Next_Cmd_Delay > 0 && State_Removal_Step == 0 && HDR7P_Auto_P.Step <= 1)
                    	Set_New_Pipe_Idx(c_mode);		//防止Cmd_Idx超過F_Pipe..Idx
                }
                if(State_Removal_Step > 0){ // debug
                    if((curTime - shwTime) >= 1000000){
                        shwTime = curTime;
                        db_debug("F_Pipe_Run: Removal=%d D_Cnt=%d Finish=%d Busy=%d Cmd_Idx=%d capture=%d\n", State_Removal_Step, 
                                F_Com_In.Capture_D_Cnt, Cap_Finish_Idx, read_Sensor_Busy(), F_Pipe.Cmd_Idx, capture);
                    }
                }
                switch (F_Com_In.Record_Mode) {
                case 0: // Capture
                case 6: // Night
                case 8: // Sport
                case 9: // Sport + WDR
                case 12:// M-Mode
                case 14:// 3D-Model
                        if(capture == 1 && F_Com_In.Capture_Step == 0) {
                            F_Com_In.Capture_Lose_Page[0] = -1;
                            F_Com_In.Capture_Lose_Page[1] = -1;
                            if(F_Com_In.Record_Mode == 12 && get_A2K_Debug_Defect_Step() != 0) {
                                F_Pipe_Add_Defect(&Cap_Finish_Idx, &Cap_Sensor_Idx, F_Com_In.Record_Mode);   				// 5張
                            }
                            else {
                            	F_Com_In.Capture_Step = 1;
                            	F_Pipe_Add_Big(F_Com_In.Big_Mode, &Cap_Finish_Idx, &Cap_Sensor_Idx, F_Com_In.Record_Mode);   // 3張取1張
                            }
                            setCaptureAddTime(F_Pipe.Cmd_Idx, Cap_Finish_Idx, Cap_Sensor_Idx, F_Com_In.Record_Mode, F_Com_In.Big_Mode);
                        }
                        break;
                case 3: // AEB (3p,5p,7p)
                        if(capture == 1){ 
                            if(AEB_Auto_HDR_En == 1 && HDR7P_Auto_P.Step != 0){
                            	ts_max = HDR7P_Auto_Proc(F_Com_In.Record_Mode);
                            }
                            if(HDR7P_Auto_P.Step == 0) {
                                F_Pipe_Add_AEB(F_Com_In.Big_Mode, &Cap_Finish_Idx, &Cap_Sensor_Idx, F_Com_In.Record_Mode);   // 3,5,7張
                                setCaptureAddTime(F_Pipe.Cmd_Idx, Cap_Finish_Idx, Cap_Sensor_Idx, F_Com_In.Record_Mode, F_Com_In.Big_Mode);
                            }
                        }  
                        break;
                case 4: // RAW (5p)
                        if(capture == 1){
                            F_Pipe_Add_RAW(F_Com_In.Big_Mode, &Cap_Finish_Idx, &Cap_Sensor_Idx);                         // 5張
                            setCaptureAddTime(F_Pipe.Cmd_Idx, Cap_Finish_Idx, Cap_Sensor_Idx, F_Com_In.Record_Mode, F_Com_In.Big_Mode);
                        }  
                        break;
                case 13:// Removal
                        if(Cap_Finish_Idx != -1){
                            Len3 = (Cap_Finish_Idx - F_Pipe.Cmd_Idx) & p_mask;
                            if(Len3 > 16) capture = 0;
                        }
                        if(capture == 1){
                            if(Removal_HDR_Mode == 1 && HDR7P_Auto_P.Step != 0) {
                            	ts_max = HDR7P_Auto_Proc(F_Com_In.Record_Mode);
                            }
                            if(HDR7P_Auto_P.Step == 0) {
                                if(State_Removal_Step == 5){
                                    if(F_Pipe.Cmd_Idx == Cap_Finish_Idx){
                                        //讀取DDR資料
                                        F_Pipe_Add_Removal(F_Com_In.Big_Mode, &Cap_Finish_Idx, &Cap_Sensor_Idx, F_Com_In.Record_Mode, State_Removal_Step);
                                        State_Removal_Step ++;
                                        if(State_Removal_Step == 7) State_Removal_Step = 0;     // 做4張判斷影像
                                    }
                                }
                                else{
                                    F_Pipe_Add_Removal(F_Com_In.Big_Mode, &Cap_Finish_Idx, &Cap_Sensor_Idx, F_Com_In.Record_Mode, State_Removal_Step);
                                    
                                    if(State_Removal_Step == 4)
                                        setCaptureAddTime(F_Pipe.Cmd_Idx, (Cap_Finish_Idx+180)&(p_mask), Cap_Sensor_Idx, F_Com_In.Record_Mode, F_Com_In.Big_Mode); // 加18秒
                                    else
                                        setCaptureAddTime(F_Pipe.Cmd_Idx, Cap_Finish_Idx, Cap_Sensor_Idx, F_Com_In.Record_Mode, F_Com_In.Big_Mode);
                                    State_Removal_Step ++;
                                    if(State_Removal_Step == 7) State_Removal_Step = 0;     // 做4張判斷影像
                                }
                            }
                        }
                        break;
                case 5: // HDR
                case 7: // Night + HDR
                        if(capture == 1) {
                        	if(HDR7P_Manual == 2 && HDR7P_Auto_P.Step != 0) {
                        		ts_max = HDR7P_Auto_Proc(F_Com_In.Record_Mode);
                        	}
                        	if(HDR7P_Auto_P.Step == 0) {
								F_Com_In.Capture_Lose_Page[0] = -1;
								F_Com_In.Capture_Lose_Page[1] = -1;
								F_Com_In.Capture_Step = 1;
								F_Pipe_Add_HDR7P(F_Com_In.Big_Mode, &Cap_Finish_Idx, &Cap_Sensor_Idx, F_Com_In.Record_Mode);   // 3,5,7張
								setCaptureAddTime(F_Pipe.Cmd_Idx, Cap_Finish_Idx, Cap_Sensor_Idx, F_Com_In.Record_Mode, F_Com_In.Big_Mode);
                        	}
                        }
                        break;
                case 1: // Record
                case 10:// Record WDR
                        F_Pipe_Add_Small(F_Com_In.Small_Mode, F_Com_In.Record_Mode, 0, &isp1_idx, &shuter_time, &ts_max);
                        break;
                case 2: // Timelapse
                case 11:// Timelapse WDR
                        Cmd_Finish_Idx = -1;
                        F_Pipe_Add_Timelapse(F_Com_In.Big_Mode, &Cmd_Finish_Idx, F_Com_In.Record_Mode, F_Com_In.Small_Mode);
                        if(Cmd_Finish_Idx != -1){
                            Next_Cmd_Delay = ((Cap_Finish_Idx - F_Pipe.Cmd_Idx) & p_mask) - 1;
                            if(Next_Cmd_Delay > 10) Next_Cmd_Delay = 10;
                        }
                        break;
                }
                int skip = 0;
                if(Check_Is_HDR_Auto() == 1 && HDR7P_Auto_P.Step != 0){
                    skip = 1;           // 避免改到Len2，造成[Auto HDR]時間拉長
                }
                if(capture == 1 && Cap_Finish_Idx != -1 && skip == 0){
                    Len2 = 16;
                    Next_Cmd_Delay = ((Cap_Finish_Idx + 15 - F_Pipe.Cmd_Idx) & p_mask) - 1;     // 避免F_Pipe_Add_Small()破壞結構
                }
                if(Cap_Finish_Idx != -1 && State_Removal_Step == 0){
                    if(F_Pipe.Cmd_Idx == Cap_Finish_Idx){
                        Cap_Finish_Idx = -1;            // cmd queue執行完畢
                        F_Com_In.Capture_D_Cnt = 0;     // == 0 才可以加新的CAP動作
                        SetWaveDebug(4, F_Com_In.Capture_D_Cnt);
                        if(F_Com_In.Capture_Step == 1) F_Com_In.Capture_Step = 2;
                        //Set_New_Pipe_Idx(c_mode);
                    }
                    //db_debug("F_Pipe_Run: D_Cnt=%d Cmd_Idx=%d Finish_Idx=%d\n", F_Com_In.Capture_D_Cnt, F_Pipe.Cmd_Idx, Cap_Finish_Idx);
                }
			#ifdef ANDROID_CODE
                Make_Main_Cmd(F_Pipe.Cmd_Idx, &State_Bulb_Exp);      // ((F_Pipe.Sensor.Idx-1)&0x3f)
			#endif
                F_Pipe.Cmd_Idx ++;
                F_Pipe.Cmd_Idx &= p_mask;

                if(State_Bulb_Exp > 0){                                 // rex+ 190108, B快門啟動
//tmp                    get_current_usec(&bstTime);
                    shwTime = bstTime;
                    break;
                }
            }
        //}
    }
}
/*
 * lester+ 190410
 *    HDR參數設定
 */
void setting_HDR7P(int manual, int frame_cnt, int ae_scale, int strength){
	if(manual >= 0 && manual <= 2) {
		HDR7P_Manual = manual;
	}
	if(frame_cnt >= 0 && frame_cnt <= 7){
		HDR7P_Frame_Cnt = frame_cnt;
	}
	if(ae_scale > 0){
		HDR7P_AE_Scale = ae_scale;
	}
	HDR7P_Strength = strength;
	if(HDR7P_Strength < 30)      HDR7P_Strength = 30;
	else if(HDR7P_Strength > 90) HDR7P_Strength = 90;

	db_debug("setting_HDR7P end: manual=%d frame_cnt=%d, ae_scale=%d, strength=%d\n",
			HDR7P_Manual, HDR7P_Frame_Cnt,HDR7P_AE_Scale,HDR7P_Strength);
}
int get_HDRManual(void)
{
	return HDR7P_Manual;
}
int get_HDR7P(void)
{
	return HDR7P_Frame_Cnt;
}
int get_HDREv(void)
{
	return HDR7P_AE_Scale;
}

void setting_Removal_HDR(int manual, int ae_scale, int strength){
	if(manual >= 0 && manual <= 5) {
		Removal_HDR_Mode = manual;
	}
	if(ae_scale > 0){
		Removal_HDR_AE_Scale = ae_scale;
	}
	Removal_HDR_Strength = strength;
	if(Removal_HDR_Strength < 30)      Removal_HDR_Strength = 30;
	else if(Removal_HDR_Strength > 90) Removal_HDR_Strength = 90;

	db_debug("setting_Removal_HDR end: manual=%d ae_scale=%d strength=%d\n",
			Removal_HDR_Mode, Removal_HDR_AE_Scale, Removal_HDR_Strength);
}
int get_Removal_HDRMode(void)
{
	return Removal_HDR_Mode;
}
int get_Removal_HDREv(void)
{
	return Removal_HDR_AE_Scale;
}

float get_HDR_Auto_Ev(int idx)
{
	return HDR7P_Auto_P.EV_Inc[idx];
}
/*
 * lester+ 190410
 *   AEB參數設定
 */
void setting_AEB(int frame_cnt,int ae_scale){
	if(frame_cnt >= 0 && frame_cnt <= 7){
		AEB_Frame_Cnt = frame_cnt;
	}
	if(ae_scale > 0){
		AEB_AE_Scale = ae_scale;
	}
	db_debug("setting_AEB end: frame_cnt=%d, ae_scale=%d\n", AEB_Frame_Cnt,AEB_AE_Scale);
}
int get_AEBFrameCnt(void)
{
	return AEB_Frame_Cnt;
}
int get_AEBEv(void)
{
	return AEB_AE_Scale;
}
/*
 * lester+ 190410
 *   讀取AEB拍攝張數
 */
int get_AEB_Frame_Cnt(void)
{
    if(AEB_Frame_Cnt > 0)
        return AEB_Frame_Cnt;
    return 3;       // default
}
/*
 * lester+ 190410
 *   讀取HDR強度值
 */
int get_HDR7P_Strength(){
	return HDR7P_Strength;
}

int get_Removal_HDR_Strength(){
	return Removal_HDR_Strength;
}
