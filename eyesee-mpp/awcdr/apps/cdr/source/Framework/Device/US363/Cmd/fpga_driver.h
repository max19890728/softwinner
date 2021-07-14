/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __FPGA_DRIVER_H__
#define __FPGA_DRIVER_H__

#ifdef __cplusplus
extern "C" {
#endif

#define DDR_ISP_ALL_COMMAND_ADDR	0x0803C000
#define DDR_ISP_ALL_STATE_ADDR		0x0803E000
#define DDR_DOWN_SCALE_RGB_ADDR		0x08040000

#define FPGA_REG_SET_ISP_MOTION		0xFE8		// read
//--- ISP Gamma						// for 1920 IPCAM
#define FPGA_REG_SET_MOCOM		    0xCCACF000
//#define FPGA_REG_SET_GAMMA		    0xDD000

#define DDR_RW_SIZE			        512
//#define STATE_MAX_NUM			    8
//#define STATE_MAX_NUM			    2
//#define CMD_MAX_NUM			        64

#define ADJUST_R_GAIN_DEFAULT 0
#define ADJUST_G_GAIN_DEFAULT 0
#define ADJUST_B_GAIN_DEFAULT 0

#define WB_SPEED 3             // 180331, =30


#define AEG_Y_TARGET_DEFAULT 			120        //0x78, 0x64
#define AEG_Y_TARGET_EV_DEFAULT 		1.0


#define ADJ_NR2D_STRENGTH_DEFAULT 32
#define COLOR_MATRIX_RATE_DEFAULT 250

#define AEG_Step 20

#define ColorTemperature_Max 8200
#define ColorTemperature_Min 2200

//===============================================================

typedef struct color_correct_Struct_H
{
    unsigned			en;
    unsigned			M11;
    unsigned			M12;
    unsigned			M13;

    unsigned			M21;
    unsigned			M22;
    unsigned			M23;

    unsigned			M31;
    unsigned			M32;
    unsigned			M33;
} color_correct_Struct;					// 40 byte

typedef struct NR2D_Table_Struct_H
{
    int			C_PI0;                     	// PI0 = 0xA		// rex+ 140821
    int			C_PV0;                     	// PV0 = -40
    int			C_PI1;                     	// PI1 = 0x18
    int			C_PV1;                     	// PV1 = 40;
    
    int			Y_PI0;                     	// PI0 = 0x8; 
    int			Y_PV0;                     	// PV0 = -32; 
    int			Y_PI1;                     	// PI1 = 0x10;	// 240 (0xF0)
    int			Y_PV1;                     	// PV1 = 60;  
} NR2D_Table_Struct;					// 36 byte

typedef struct ISP_Command_Struct_H
{
    unsigned 		get_avg_rgb_en;
    unsigned 		AWB_en_do;

    unsigned 		MWB_Rgain;
    unsigned 		MWB_Ggain;
    unsigned 		MWB_Bgain;
    
    unsigned 		Brightness;
    unsigned 		Contrast;
    unsigned 		Saturation;

    unsigned 		AEG_en;
    unsigned 		AEG_Y_target;			// 40
    unsigned 		Fix_AEG_table;
    unsigned 		AEG_table_idx;
    
    unsigned 		DWDR_en;
    unsigned 		DWDR_up_limit;
    unsigned 		DWDR_dw_limit;

    color_correct_Struct color_correct;

    unsigned 		auto_motion_threshold_en;
    unsigned 		motion_sill;
    
    unsigned 		motion_debug;
    unsigned 		motion_debug2;
    
    unsigned 		Black_Level;			// 120

    unsigned 		DN2D_en;			//1'
    unsigned 		DN2D_edge_offset;		//4'
    unsigned 		DN2D_motion_score;		//4'
    unsigned 		DN2D_user_score;		//5'
    
    unsigned 		BW_EN;
    unsigned 		DWDR_YUV;
    
    unsigned 		V_start;			// sam+ 140530
    unsigned 		H_start;

    unsigned 		FLIP_ts;
    unsigned 		H_FLIP;				// 160 (0xA0)
    unsigned 		V_FLIP;

    unsigned 		MASK;				// Tom + 140819
    unsigned 		Position;
    unsigned 		MoCom;

    unsigned 		Mix_Rate;			// 0x00~0x7F=Debug家Α眏砞3D把计, 0x80=家Α穦把σLevel&Rate砞﹚
    unsigned 		Level2;
    unsigned 		Level1;
    unsigned 		Level0;
    unsigned 		Rate3;
    unsigned 		Rate2;				// 200 (0xC8)
    unsigned 		Rate1;
    unsigned 		Rate0;

    unsigned 		SH_Mode;
    unsigned 		raw_h_s;
    int			NR3D_Strength;			// SW 3DNR秨闽, 0~15=砞﹚3DNR眏, 0xff=debug家Α
    int			NR2D_Strength;			// SW 2DNR秨闽, 0~15=砞﹚2DNR眏, 0xff=debug家Α

    int			NR2D_Table_Start;		// SW 癘拘砰把σ翴, debugㄏノ
    NR2D_Table_Struct	NR2D_Table[4];
} ISP_Command_Struct;					// total: 356 byte  程8kb (=DSZ_ISP_ALL_COMMAND_ADDR)
extern ISP_Command_Struct ISP_All_Command_D;

typedef struct AVG_RGBY_Struct_HEADER
{
    unsigned			real_Y;
    unsigned			AVG_Y;
    unsigned			AVG_R;
    unsigned			AVG_G;
    unsigned			AVG_B;
} AVG_RGBY_Struct;

typedef struct AWB_gain_Struct_H
{
    unsigned			gain_R;
    unsigned			gain_G;
    unsigned			gain_B;
} AWB_gain_Struct;

typedef struct AEG_Struct_H
{
    unsigned			AEG_table_index;
    unsigned			AEG_EP_H;
    unsigned			AEG_EP_L;
    unsigned			AEG_gain_H;
    unsigned			AEG_gain_L;
} AEG_Struct;

typedef struct DWDR_Stati_Struct_H
{
    unsigned			Stati_Y00;
    unsigned			Stati_Y20;
    unsigned			Stati_Y40;
    unsigned			Stati_Y60;
    unsigned			Stati_Y80;
    unsigned			Stati_YA0;
    unsigned			Stati_YC0;
    unsigned			Stati_YE0;
} DWDR_Stati_Struct;

typedef struct ISP_Stati_Struct_H
{
    //unsigned              DWDR_gamma_table[1024];     // 4096 byte
    AVG_RGBY_Struct         AVG_YRGB;                   //   20 byte
    AWB_gain_Struct         AWB_Gain;                   //   12 byte
    AEG_Struct              AEG_coef;                   //   20 byte
    DWDR_Stati_Struct       DWDR_coef;                  //   32 byte
} ISP_Stati_Struct;                                     // total: 4180byte    程8kb (=DSZ_ISP_ALL_STATE_ADDR)
extern ISP_Stati_Struct ISP_All_State_D;

typedef struct ISP_Block1_Set_Reg_Struct_H
{
    unsigned			M11;
    unsigned			M12;
    unsigned			M13;

    unsigned			M21;
    unsigned			M22;
    unsigned			M23;

    unsigned			M31;
    unsigned			M32;
    unsigned			M33;

    unsigned			MR;
    unsigned			MG;
    unsigned			MB;

    unsigned			Gain_R;
    unsigned			Gain_G;
    unsigned			Gain_B;

    unsigned			Y00;
    unsigned			Y20;
    unsigned			Y40;
    unsigned			Y60;
    unsigned			Y80;
    unsigned			YA0;
    unsigned			YC0;
    unsigned			YE0;
} ISP_Block1_Set_Reg_Struct;

typedef struct ISP_Block2_Set_Reg_Struct_H
{
    unsigned			Now_Addr;
    unsigned			Pre_Addr;
    unsigned			Tar_Addr;
    unsigned			MoR_Addr;
    unsigned			S320_Addr;
    unsigned			S160_Addr;

    unsigned			Out_Addr;
    unsigned			Scale1_Addr;
    unsigned			Scale2_Addr;

    unsigned			DO_SizeX		: 8;
    unsigned			DO_LastX		: 4;
    unsigned			DO_SizeRev1		: 4;
    unsigned			DO_SizeY		: 11;
    unsigned			Do_SizeRev2		: 4;
    unsigned			Do_StartEnable		: 1;	// 1 恶把计, 0 铬ぃ磅︽

    unsigned			Edge_Mo_V1		: 10;
    unsigned			Edge_Mo_V2		: 10;
    unsigned			Edge_Mo_V3		: 10;
    unsigned			Edge_Mo_VRev		: 2;
    unsigned			Edge_UnMo_V1		: 10;
    unsigned			Edge_UnMo_V2		: 10;
    unsigned			Edge_UnMo_V3		: 10;
    unsigned			Edge_UnMo_VRev		: 2;
    unsigned			UnEdge_Mo_V1		: 10;
    unsigned			UnEdge_Mo_V2		: 10;
    unsigned			UnEdge_Mo_V3		: 10;
    unsigned			UnEdge_Mo_VRev		: 2;
    unsigned			UnEdge_UnMo_V1		: 10;
    unsigned			UnEdge_UnMo_V2		: 10;
    unsigned			UnEdge_UnMo_V3		: 10;
    unsigned			UnEdge_UnMo_VRev	: 2;

    unsigned			Edge_MaxV_X		: 8;
    unsigned			Edge_MaxV_Rev1		: 8;
    unsigned			Edge_MaxV_Y		: 8;
    unsigned			Edge_MaxV_Rev2		: 8;

    unsigned			U_R_mulV		: 12;
    unsigned			U_G_mulV		: 12;
    unsigned			U_B_mulV		: 12;
    unsigned			V_R_mulV		: 12;
    unsigned			V_G_mulV		: 12;
    unsigned			V_B_mulV		: 12;
    unsigned			UV_RGB_Rev		: 24;
} ISP_Block2_Set_Reg_Struct;

typedef struct ISP_All_Set_Reg_Struct_H
{
    ISP_Block1_Set_Reg_Struct	ISP_Block1;
    ISP_Block2_Set_Reg_Struct	ISP_Block2;
} ISP_All_Set_Reg_Struct;
extern ISP_All_Set_Reg_Struct ISP_All_Set_Reg;

typedef struct Color_Temperature_Struct_H {
  int      T;
  float    R;
  float    G;
  float    B;
} Color_Temperature_Struct;

typedef struct{
	unsigned    Gain_R   :10;
	unsigned	Gain_G   :10;
	unsigned    Gain_B   :11;
	unsigned    rev      :1;
}RGB_Gain_struct;

struct     Sensor_ADT_Struct {
  unsigned short     Rate:4;
  unsigned short     Base:12;
};

//===============================================================

//extern unsigned char ddr_avg_rgb[138240];

//extern int Y_value_status[8];
//extern unsigned short int Y_level_count[256], UV_level_count[256];

extern int WB_Mode;

extern int Delta_R, Delta_G, Delta_B;
extern int Adjust_R_Gain, Adjust_G_Gain, Adjust_B_Gain;

extern unsigned char AWB_buf[2][0x28000];

extern int Temp_gain_R, Temp_gain_G, Temp_gain_B;

extern int AEG_EP_manual, AEG_EP_idx, AEG_EP_idx_lst, AEG_EP_idx_tmp, AEG_EP_idx_tmp2, AEG_EP_FPS;
extern int AEG_Gain_manual, AEG_Gain_idx;
//extern const unsigned ISP_AEG_EP_IMX222[9][2];

//extern int AEG_Y_target_rate_flag;
extern float AEG_Y_ev;

//extern int AEG_adj_count, AEG_weighted[5];
//extern int AEG_exposure_level[7];
extern int AEG_Black_Level, AEG_Black_Manual;
extern unsigned AEG_EP;

extern int AEG_gain_H;      // AEG_EP_L, AEG_EP_H, 

extern int AEG_Y_target;

extern int ep_mode, ep_mode_lst;
extern int AEG_EP_FRM_LENGTH;
extern int AEG_EP_INTEG_TIME;

extern int Integ_Lock;

extern int ISP_BC_Value[1024];
//extern unsigned int Y_gamma_count[256][2];
//extern unsigned short int Y_gamma_value[256];
//extern int ISP_DWDR2_Mode, ISP_DWDR2_Black_Level;

//extern int adj_gamma_table[17];

extern int NR3D_sill_table[8];
extern int NR3D_strength_tbl[16][3];

extern int color_matrix_rate;
extern int color_matrix_manual, color_matrix_idx;
extern int color_matrix_tbl[7][9];

//extern unsigned gamma_buf[2048];

extern unsigned isp2_motion_value[2];
extern char ISP_2NDR_Value[512];
extern int ISP_DDR_command_init_finish;
extern int Adj_NR2D_Strength;

extern int ISP_command_init;

extern int Color_Saturation_Adj;
extern int Color_Saturation[9];

extern int ISP_AEG_EP_IMX222_Freq;

extern int run_Gamma_Set_Func;
extern int Gamma_Line_Ready;

//extern int ep_init_change;

//===============================================================
//void set_isp1_2m_data(unsigned cmd_num, unsigned data);
//void set_isp2_2m_data(unsigned cmd_num, unsigned data);
//void set_512_2m_data(unsigned cmd_num, unsigned data);
int YUV2RGB(int y, int u, int v, int *r, int *g, int *b);
int YUV2RGB_2(int y, int u, int v, int *r, int *g, int *b);

void readAWBbuf(int st_mode);
int get_avg_rgby(void);
void ISP_AWB(void);
int ISP_AEG_IMX222( int c_mode, int m_mode);
void AEG_IMX222_write(void);
void Set_ISP_Block2_3DNR(int init_flag);
int Get_3DNR_Rate(int strength);
void ISP_Command_Porcess(int c_mode);
//int Do_Color_Matrix(void);
int Do_RGB_Gain(void);
//int Save_Parameter_Tmp_File(void);

void CMOS_ISP_Block1_SET(void);
void CMOS_ISP_Block2_SET(void);
void Set_ISP_Block2_2DNR(int gain, int ready, int c_mode, int m_mode, int strength);
void ISP_DDR_command_init(void);
int readSitchingIdx(void);
int Brightness256to1024(int value);
void Brightness1024to256Init(void);
int get_ep_time(int idx, int freq);

void getAEGParameters(int *val);
void setLiveShowValue(int exp_n, int exp_m, int iso);

int Senser_One_Trans(int In);
void Write_ADT_Table(void);
void Senser_AD_Trans(int ISO);
int get_Init_Gamma_Table_En(void);
void setAEGBExp1Sec(int sec);
void setAEGBExpGain(int gain);
int Read_FPGA_Pmw(void);
void FPGAdriverInit(void);
void setAETergetRateWifiCmd(int y_idx);
void setAEGGainWifiCmd(int manual_mode, int gain_idx);
void setWBMode(int wb_mode);
void setFpgaEpFreq(int freq);
void setStrengthWifiCmd(int offset);
void setContrast(int val);
void set_Init_Gamma_Table_En(void);
int Ep_Change_Init();
void Set_Init_Image_State(int state);
int Get_Init_Image_State();

//---------------------------------------
void setExposureTimeWifiCmd(int manual_mode, int ep_idx);
void setWBTemperature(int temp, int tint);
void setPhotoLuxValue();
int GetColorTemperature();
int GetTint();
void do2DNR(int en);
void SetHDRTone(int value);
void getWBRgbData(int *value);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__FPGA_DRIVER_H__