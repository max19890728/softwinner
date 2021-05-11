/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/
 
#ifndef __US360_DEFINE_H__
#define __US360_DEFINE_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
	unsigned ChipID		:8;
	unsigned Addr_H		:8;
	unsigned Addr_L		:8;
	unsigned Data		:8;
}CIS_CMD_struct;

#define FS_A_GAIN_OFFSET 	0
#define D2_A_GAIN_OFFSET 	64
#define D3_A_GAIN_OFFSET 	202

#define	CIS_TAB_N	68
#define	CIS_TAB_N2	CIS_TAB_N*2

#define EP_FRM_LENGTH_60Hz_FS_DEFAULT	0x04C6
#define EP_FRM_LENGTH_60Hz_D2_DEFAULT	0x0781
#define EP_FRM_LENGTH_60Hz_D3_DEFAULT	0x0476
#define EP_FRM_LENGTH_50Hz_FS_DEFAULT	0x05BA
#define EP_FRM_LENGTH_50Hz_D2_DEFAULT	0x0901
#define EP_FRM_LENGTH_50Hz_D3_DEFAULT	0x055A

#define ISP2_A_ENABLE		1
#define ISP2_B_ENABLE		1
#define ISP2_C_ENABLE		1
#define AS2_MSPI_ENABLE		1
#define AS2_MSPI_F0_ID		6
#define AS2_MSPI_F1_ID		5

#define M_CMD_PAGE_N			64
#define FX_START_MODE       	0x0

#define F2_J_HIGH_QUALITY       90
#define F2_J_MIDDLE_QUALITY_2   85
#define F2_J_MIDDLE_QUALITY     80
#define F2_J_LOW_QUALITY        75
#define F2_J_QUALITY_DEFAULT    F2_J_MIDDLE_QUALITY
#define F2_J_PIXEL_X        	3840
#define F2_J_PIXEL_Y        	1920

#define AS2_FS_V_START			72
#define AS2_FS_H_START			12
#define AS2_D2_V_START			40
#define AS2_D2_H_START			0
#define AS2_D3_V_START			30
#define AS2_D3_H_START			0

#define AS2_S3_V_START			34
#define AS2_S3_H_START			4

//Stream 2: 738x559 (768) ---------------------------------------------------------
//#define STREAM2_ISP1_BUF_ADDR		0xA200000
//#define STREAM2_ISP2_BUF_ADDR		STREAM2_ISP1_BUF_ADDR + (768 * 6)
//#define STREAM2_NR3D_BUF_ADDR		STREAM2_ISP2_BUF_ADDR + (768 * 6 * 2)

//---------------------------------------------------------------------------------

//Pipe Sub Code
#define PIPE_SUBCODE_CAP_CNT_0			0
#define PIPE_SUBCODE_CAP_CNT_1			1
#define PIPE_SUBCODE_CAP_CNT_2			2
#define PIPE_SUBCODE_AEB_STEP_0			20
#define PIPE_SUBCODE_AEB_STEP_1			21
#define PIPE_SUBCODE_AEB_STEP_2			22
#define PIPE_SUBCODE_AEB_STEP_3			23
#define PIPE_SUBCODE_AEB_STEP_4			24
#define PIPE_SUBCODE_AEB_STEP_5			25
#define PIPE_SUBCODE_AEB_STEP_6			26
#define PIPE_SUBCODE_AEB_STEP_E			27      // END
#define PIPE_SUBCODE_HDR_CNT_0			30
#define PIPE_SUBCODE_HDR_CNT_1			31
#define PIPE_SUBCODE_HDR_CNT_2			32
#define PIPE_SUBCODE_HDR_CNT_E			33      // END
#define PIPE_SUBCODE_ST_YUVZ_0			41
#define PIPE_SUBCODE_ST_YUVZ_1			42
#define PIPE_SUBCODE_ST_YUVZ_2			44
#define PIPE_SUBCODE_ST_YUVZ_ALL		47
#define PIPE_SUBCODE_5S_CNT_0			50      // RAW
#define PIPE_SUBCODE_5S_CNT_1			51
#define PIPE_SUBCODE_5S_CNT_2			52
#define PIPE_SUBCODE_5S_CNT_3			53
#define PIPE_SUBCODE_5S_CNT_4			54
#define PIPE_SUBCODE_REMOVAL_CNT_0		60      // Removal
#define PIPE_SUBCODE_REMOVAL_CNT_1		61
#define PIPE_SUBCODE_REMOVAL_CNT_2		62
#define PIPE_SUBCODE_REMOVAL_CNT_3		63
#define PIPE_SUBCODE_SENSOR_RESET		99
#define PIPE_SUBCODE_CAP_THM			100
#define PIPE_SUBCODE_TL_LIVE			101

//---------------------------------------------------------------------------------

#define PIPE_USB_SPEED_12K	   	1269332
#define PIPE_USB_SPEED_8K		 544000
#define PIPE_USB_SPEED_6K		 453332
#define PIPE_USB_SPEED_4K		  63124
#define PIPE_USB_SPEED_3K		  33759
#define PIPE_USB_SPEED_2K		  22718

/*#define PIPE_USB_SPEED_12K		933333
#define PIPE_USB_SPEED_8K		400000
#define PIPE_USB_SPEED_6K		333333
#define PIPE_USB_SPEED_4K		 63124
//#define PIPE_USB_SPEED_3K		 44759
#define PIPE_USB_SPEED_3K		 33759
#define PIPE_USB_SPEED_2K		 22718*/

//---------------------------------------------------------------------------------

#define FPGA_CNT_FS_TEST 9997695

#define FPGA_CNT_FS_100FPS 10000228
#define FPGA_CNT_FS_80FPS  12500285
#define FPGA_CNT_FS_75FPS  13333638
#define FPGA_CNT_FS_50FPS  20000457
#define FPGA_CNT_FS_40FPS  25000572         // rex+ 181101
#define FPGA_CNT_FS_33FPS  30000687
#define FPGA_CNT_FS_10FPS  100002295
#define FPGA_CNT_FS_6FPS   150003444

#define FPGA_CNT_D2_300FPS 3332884
#define FPGA_CNT_D2_240FPS 4166105
#define FPGA_CNT_D2_150FPS 6665770
#define FPGA_CNT_D2_100FPS 9998657
#define FPGA_CNT_D2_75FPS  13331543
#define FPGA_CNT_D2_60FPS  16664429
#define FPGA_CNT_D2_50FPS  19997315
#define FPGA_CNT_D2_30FPS  33328860
#define FPGA_CNT_D2_10FPS  99986585

#define FPGA_CNT_D3_300FPS 3332251
#define FPGA_CNT_D3_240FPS 4165314
#define FPGA_CNT_D3_150FPS 6664503
#define FPGA_CNT_D3_100FPS 9996756
#define FPGA_CNT_D3_75FPS  13329009
#define FPGA_CNT_D3_60FPS  16661261
#define FPGA_CNT_D3_50FPS  19993514
#define FPGA_CNT_D3_30FPS  33322524
#define FPGA_CNT_D3_10FPS  99967577

//---------------------------------------------------------------------------------

//S2 Resolution Width & Height
#define S2_RES_12K_WIDTH	11520
#define S2_RES_10K_WIDTH	10240
#define S2_RES_8K_WIDTH		7680
#define S2_RES_6K_WIDTH		6144
#define S2_RES_4K_WIDTH		3840
#define S2_RES_3K_WIDTH		3072
#define S2_RES_2K_WIDTH		2048
#define S2_RES_3D1K_WIDTH	1152
#define S2_RES_3D4K_WIDTH	3840
#define S2_RES_THM_WIDTH	1024
#define S2_RES_S_FS_WIDTH	4352
#define S2_RES_S_D2_WIDTH	2240
#define S2_RES_S_D3_WIDTH	1536

#define S2_RES_12K_HEIGHT	5760
#define S2_RES_10K_HEIGHT	5120
#define S2_RES_8K_HEIGHT	3840
#define S2_RES_6K_HEIGHT	3072
#define S2_RES_4K_HEIGHT	1920
#define S2_RES_3K_HEIGHT	1536
#define S2_RES_2K_HEIGHT	1024
#define S2_RES_3D1K_HEIGHT	1152
#define S2_RES_3D4K_HEIGHT	64
#define S2_RES_THM_HEIGHT	1024
#define S2_RES_S_FS_HEIGHT	3280
#define S2_RES_S_D2_HEIGHT	1728
#define S2_RES_S_D3_HEIGHT	1152

#define BOTTOM_S_WIDTH 		1024
#define BOTTOM_S_HEIGHT 	1024
#define BOTTOM_T_WIDTH 		3840
#define BOTTOM_T_HEIGHT 	512
#define BOTTOM_T_WIDTH_2 	1024
#define BOTTOM_T_HEIGHT_2 	128

#define MAP_S_HEIGHT	1152
#define MAP_S_WIDTH		1152
#define MAP_4K_HEIGHT	64
#define MAP_4K_WIDTH	3840

//---------------------------------------------------------------------------------

// us
#define FPGA_FS_TIME_OUT 120000
//#define FPGA_D2_TIME_OUT 40000
//#define FPGA_D3_TIME_OUT 40000
#define FPGA_D2_TIME_OUT FPGA_FS_TIME_OUT
#define FPGA_D3_TIME_OUT FPGA_FS_TIME_OUT

//---------------------------------------------------------------------------------

//Read Sensor RGB IO ADDR
#define S0_R_IO_ADDR 0x60034        // �L���v��RGB�έp��
#define S0_G_IO_ADDR 0x60038
#define S0_B_IO_ADDR 0x6003C
#define S1_R_IO_ADDR 0x60028
#define S1_G_IO_ADDR 0x6002C
#define S1_B_IO_ADDR 0x60030
#define S2_R_IO_ADDR 0x60004
#define S2_G_IO_ADDR 0x60008
#define S2_B_IO_ADDR 0x6000C
#define S3_R_IO_ADDR 0x6001C
#define S3_G_IO_ADDR 0x60020
#define S3_B_IO_ADDR 0x60024
#define S4_R_IO_ADDR 0x60010
#define S4_G_IO_ADDR 0x60014
#define S4_B_IO_ADDR 0x60018

#define S0_R2_IO_ADDR 0x600B0       // �����v��RGB�έp��
#define S0_G2_IO_ADDR 0x600B4
#define S0_B2_IO_ADDR 0x600B8
#define S1_R2_IO_ADDR 0x600A4
#define S1_G2_IO_ADDR 0x600A8
#define S1_B2_IO_ADDR 0x600AC
#define S2_R2_IO_ADDR 0x60080
#define S2_G2_IO_ADDR 0x60084
#define S2_B2_IO_ADDR 0x60088
#define S3_R2_IO_ADDR 0x60098
#define S3_G2_IO_ADDR 0x6009C
#define S3_B2_IO_ADDR 0x600A0
#define S4_R2_IO_ADDR 0x6008C
#define S4_G2_IO_ADDR 0x60090
#define S4_B2_IO_ADDR 0x60094


//#define REV        0x60018
//#define REV        0x6001C
//#define REV        0x60020

#define SYNC_IDX_IO_ADDR 0x60040
#define IMCP_IDX_IO_ADDR 0x60044

#define F0_CNT_IO_ADDR 0x60048
#define F2_CNT_IO_ADDR 0x60050

#define SYNC_F0_S0_IO_ADDR	0x60054
#define SYNC_F0_S1_IO_ADDR	0x60058
#define SYNC_F1_S0_IO_ADDR	0x6005C
#define SYNC_F1_S1_IO_ADDR	0x60060
#define SYNC_F1_S2_IO_ADDR	0x60064

#define F0_DDR_CHECKSUM_IO_ADDR 	0x600D8
#define F1_DDR_CHECKSUM_IO_ADDR 	0x600DC

#define F0_DDR_CHECKSUM_IO_ADDR 	0x600D8
#define F1_DDR_CHECKSUM_IO_ADDR 	0x600DC

#define FPGA_PWM_IO_ADDR	0x600E0
//---------------------------------------------------------------------------------

#define SENSOR_BINN_FS_W_PIXEL	   		4432
#define SENSOR_BINN_FS_H_PIXEL	   		3354
#define SENSOR_BINN_2X2_W_PIXEL	   		2216
#define SENSOR_BINN_2X2_H_PIXEL	   		1677
#define SENSOR_BINN_3X3_W_PIXEL	   		1477
#define SENSOR_BINN_3X3_H_PIXEL	   		1118

//---------------------------------------------------------------------------------

//ISP1_BINN_MODE
#define DS_ISP1_BINN_OFF		0
#define DS_ISP1_BINN_2X2		1
#define DS_ISP1_BINN_3X3		2
#define DS_ISP1_BINN_8X8		0               // fpga_binn=1�~���@��

//---------------------------------------------------------------------------------

#define F0_M_CMD_ADDR			0x0F200000
#define F2_M_CMD_ADDR			0x0F280000

//4608*3392
//P0: 0x0000000 ~ 0x6A00000
//P1: 0x0003600 ~ 0x6A03600
//P2: 0x6A00000 ~ 0xD400000
#define ISP1_STM1_T_P0_BUF_ADDR     0x0000000
#define ISP1_STM1_T_P0_A_BUF_ADDR   (ISP1_STM1_T_P0_BUF_ADDR + 0 * 4608)
#define ISP1_STM1_T_P0_B_BUF_ADDR   (ISP1_STM1_T_P0_BUF_ADDR + 1 * 4608)
#define ISP1_STM1_T_P0_C_BUF_ADDR   (ISP1_STM1_T_P0_BUF_ADDR + 2 * 4608)
#define ISP1_STM1_T_P1_BUF_ADDR     0x0003600
#define ISP1_STM1_T_P1_A_BUF_ADDR   (ISP1_STM1_T_P1_BUF_ADDR + 0 * 4608)
#define ISP1_STM1_T_P1_B_BUF_ADDR   (ISP1_STM1_T_P1_BUF_ADDR + 1 * 4608)
#define ISP1_STM1_T_P1_C_BUF_ADDR   (ISP1_STM1_T_P1_BUF_ADDR + 2 * 4608)
#define ISP1_STM1_T_P2_BUF_ADDR     0x6A00000
#define ISP1_STM1_T_P2_A_BUF_ADDR   (ISP1_STM1_T_P2_BUF_ADDR + 0 * 4608)
#define ISP1_STM1_T_P2_B_BUF_ADDR   (ISP1_STM1_T_P2_BUF_ADDR + 1 * 4608)
#define ISP1_STM1_T_P2_C_BUF_ADDR   (ISP1_STM1_T_P2_BUF_ADDR + 2 * 4608)

//#define DS_ISP1_BINN_DDR_ADDR	0x10000000   /////////////
#define DS_ISP1_S0_BINN_EN		1
#define DS_ISP1_S1_BINN_EN	 	1
#define DS_ISP1_S2_BINN_EN		1

//1536*1152
//P0: 0xD400000 ~ 0xF800000
//P1: 0xD401200 ~ 0xF801200
#define ISP1_STM2_T_P0_BUF_ADDR		0xD400000
#define ISP1_STM2_T_P0_A_BUF_ADDR	(ISP1_STM2_T_P0_BUF_ADDR + 0 * 1536)
#define ISP1_STM2_T_P0_B_BUF_ADDR	(ISP1_STM2_T_P0_BUF_ADDR + 1 * 1536)
#define ISP1_STM2_T_P0_C_BUF_ADDR	(ISP1_STM2_T_P0_BUF_ADDR + 2 * 1536)
#define ISP1_STM2_T_P1_BUF_ADDR		0xD401200
#define ISP1_STM2_T_P1_A_BUF_ADDR	(ISP1_STM2_T_P1_BUF_ADDR + 0 * 1536)
#define ISP1_STM2_T_P1_B_BUF_ADDR	(ISP1_STM2_T_P1_BUF_ADDR + 1 * 1536)
#define ISP1_STM2_T_P1_C_BUF_ADDR	(ISP1_STM2_T_P1_BUF_ADDR + 2 * 1536)

#define ISP1_WB_P0_ADDR				0x06A06D40
#define ISP1_WB_P0_A_ADDR			(ISP1_WB_P0_ADDR + 0 * 0x240)
#define ISP1_WB_P0_B_ADDR			(ISP1_WB_P0_ADDR + 1 * 0x240)
#define ISP1_WB_P0_C_ADDR			(ISP1_WB_P0_ADDR + 2 * 0x240)
#define ISP1_WD_P0_ADDR				0x08406D40
#define ISP1_WD_P0_A_ADDR			(ISP1_WD_P0_ADDR + 0 * 0x240)
#define ISP1_WD_P0_B_ADDR			(ISP1_WD_P0_ADDR + 1 * 0x240)
#define ISP1_WD_P0_C_ADDR			(ISP1_WD_P0_ADDR + 2 * 0x240)
#define ISP1_WD_2_P0_ADDR			0x15806D00
#define ISP1_WD_2_P0_A_ADDR			(ISP1_WD_2_P0_ADDR + 0 * 0x240)
#define ISP1_WD_2_P0_B_ADDR			(ISP1_WD_2_P0_ADDR + 1 * 0x240)
#define ISP1_WD_2_P0_C_ADDR			(ISP1_WD_2_P0_ADDR + 2 * 0x240)

#define ISP1_WB_P1_ADDR				0x09E06D40
#define ISP1_WB_P1_A_ADDR			(ISP1_WB_P1_ADDR + 0 * 0x240)
#define ISP1_WB_P1_B_ADDR			(ISP1_WB_P1_ADDR + 1 * 0x240)
#define ISP1_WB_P1_C_ADDR			(ISP1_WB_P1_ADDR + 2 * 0x240)
#define ISP1_WD_P1_ADDR				0x0B806D40
#define ISP1_WD_P1_A_ADDR			(ISP1_WD_P1_ADDR + 0 * 0x240)
#define ISP1_WD_P1_B_ADDR			(ISP1_WD_P1_ADDR + 1 * 0x240)
#define ISP1_WD_P1_C_ADDR			(ISP1_WD_P1_ADDR + 2 * 0x240)
#define ISP1_WD_2_P1_ADDR			0x17206D00
#define ISP1_WD_2_P1_A_ADDR			(ISP1_WD_2_P1_ADDR + 0 * 0x240)
#define ISP1_WD_2_P1_B_ADDR			(ISP1_WD_2_P1_ADDR + 1 * 0x240)
#define ISP1_WD_2_P1_C_ADDR			(ISP1_WD_2_P1_ADDR + 2 * 0x240)

// 0xF400000 ~ 0xF540000
#define F2_LC_BUF_FS_Addr             	0xF400000
#define F2_LC_FS_A_BUF_Addr            (F2_LC_BUF_FS_Addr + 0 * 0x8000)
#define F2_LC_FS_B_BUF_Addr            (F2_LC_BUF_FS_Addr + 1 * 0x8000)
#define F2_LC_FS_C_BUF_Addr            (F2_LC_BUF_FS_Addr + 2 * 0x8000)
#define F2_LC_FS_D_BUF_Addr            (F2_LC_BUF_FS_Addr + 3 * 0x8000)
#define F2_LC_FS_E_BUF_Addr            (F2_LC_BUF_FS_Addr + 4 * 0x8000)
#define F2_LC_BUF_D2_Addr             	0xF430000
#define F2_LC_D2_A_BUF_Addr            (F2_LC_BUF_D2_Addr + 0 * 0x8000)
#define F2_LC_D2_B_BUF_Addr            (F2_LC_BUF_D2_Addr + 1 * 0x8000)
#define F2_LC_D2_C_BUF_Addr            (F2_LC_BUF_D2_Addr + 2 * 0x8000)
#define F2_LC_D2_D_BUF_Addr            (F2_LC_BUF_D2_Addr + 3 * 0x8000)
#define F2_LC_D2_E_BUF_Addr            (F2_LC_BUF_D2_Addr + 4 * 0x8000)
#define F2_LC_BUF_D3_Addr             	0xF460000
#define F2_LC_D3_A_BUF_Addr            (F2_LC_BUF_D3_Addr + 0 * 0x8000)
#define F2_LC_D3_B_BUF_Addr            (F2_LC_BUF_D3_Addr + 1 * 0x8000)
#define F2_LC_D3_C_BUF_Addr            (F2_LC_BUF_D3_Addr + 2 * 0x8000)
#define F2_LC_D3_D_BUF_Addr            (F2_LC_BUF_D3_Addr + 3 * 0x8000)
#define F2_LC_D3_E_BUF_Addr            (F2_LC_BUF_D3_Addr + 4 * 0x8000)
// 0xFA00000 ~ 0xFAC0000
#define FX_LC_FS_BUF_Addr              0x1F100000
#define FX_LC_FS_A_BUF_Addr            (FX_LC_FS_BUF_Addr + 0 * 0x8000)
#define FX_LC_FS_B_BUF_Addr            (FX_LC_FS_BUF_Addr + 1 * 0x8000)
#define FX_LC_FS_C_BUF_Addr            (FX_LC_FS_BUF_Addr + 2 * 0x8000)
#define FX_LC_D2_BUF_Addr              0x1F120000
#define FX_LC_D2_A_BUF_Addr            (FX_LC_D2_BUF_Addr + 0 * 0x8000)
#define FX_LC_D2_B_BUF_Addr            (FX_LC_D2_BUF_Addr + 1 * 0x8000)
#define FX_LC_D2_C_BUF_Addr            (FX_LC_D2_BUF_Addr + 2 * 0x8000)
#define FX_LC_D3_BUF_Addr              0x1F140000
#define FX_LC_D3_A_BUF_Addr            (FX_LC_D3_BUF_Addr + 0 * 0x8000)
#define FX_LC_D3_B_BUF_Addr            (FX_LC_D3_BUF_Addr + 1 * 0x8000)
#define FX_LC_D3_C_BUF_Addr            (FX_LC_D3_BUF_Addr + 2 * 0x8000)

//0x6A03600 ~ 0xD403600
#define NR3D_STM1_BUF_ADDR   	            (0x6A03600 + 64)
#define NR3D_STM1_A_BUF_ADDR                (NR3D_STM1_BUF_ADDR + 0 * 4608)
#define NR3D_STM1_B_BUF_ADDR                (NR3D_STM1_BUF_ADDR + 1 * 4608)
#define NR3D_STM1_C_BUF_ADDR                (NR3D_STM1_BUF_ADDR + 2 * 4608)
#define ISP1_STM1_T_P6_A_BUF_ADDR           (NR3D_STM1_BUF_ADDR + 0 * 4608)                /* �@��NR3D 0x6A03600 */
#define ISP1_STM1_T_P6_B_BUF_ADDR           (NR3D_STM1_BUF_ADDR + 1 * 4608)
#define ISP1_STM1_T_P6_C_BUF_ADDR           (NR3D_STM1_BUF_ADDR + 2 * 4608)

//0x10006C00 ~ 0x16A06C00
#define NR3D_STM2_BUF_ADDR                  (0x10006D00 + 64)
#define NR3D_STM2_A_BUF_ADDR                (NR3D_STM2_BUF_ADDR + 0 * 1536)
#define NR3D_STM2_B_BUF_ADDR                (NR3D_STM2_BUF_ADDR + 1 * 1536)
#define NR3D_STM2_C_BUF_ADDR                (NR3D_STM2_BUF_ADDR + 2 * 1536)

//4608*3392
//P0: 0x10000000 ~ 0x16A00000
//P1: 0x16A00000 ~ 0x1D400000
#define ISP2_STM1_T_P0_BUF_ADDR             (0x10000000 + 128 * 2)                          /* fpga st_addr bug, +(128*2) */
#define ISP2_STM1_T_P0_A_BUF_ADDR           (ISP2_STM1_T_P0_BUF_ADDR + 0 * 4608 * 2)        /* 1pixel = 2byte*/
#define ISP2_STM1_T_P0_B_BUF_ADDR           (ISP2_STM1_T_P0_BUF_ADDR + 1 * 4608 * 2)
#define ISP2_STM1_T_P0_C_BUF_ADDR           (ISP2_STM1_T_P0_BUF_ADDR + 2 * 4608 * 2)
#define ISP2_STM1_T_P1_BUF_ADDR             (0x16A00000 + 128 * 2)                          // 379584512
#define ISP2_STM1_T_P1_A_BUF_ADDR           (ISP2_STM1_T_P1_BUF_ADDR + 0 * 4608 * 2)
#define ISP2_STM1_T_P1_B_BUF_ADDR           (ISP2_STM1_T_P1_BUF_ADDR + 1 * 4608 * 2)
#define ISP2_STM1_T_P1_C_BUF_ADDR           (ISP2_STM1_T_P1_BUF_ADDR + 2 * 4608 * 2)

#define ISP2_REMOVAL_P0_A_BUF_ADDR           (ISP2_STM1_T_P1_BUF_ADDR + 0 * 4608)           // rex+ 190509, Removal used.
#define ISP2_REMOVAL_P0_B_BUF_ADDR           (ISP2_STM1_T_P1_BUF_ADDR + 1 * 4608)
#define ISP2_REMOVAL_P0_C_BUF_ADDR           (ISP2_STM1_T_P1_BUF_ADDR + 2 * 4608)
#define ISP2_REMOVAL_P1_A_BUF_ADDR           (ISP2_STM1_T_P1_BUF_ADDR + 3 * 4608)
#define ISP2_REMOVAL_P1_B_BUF_ADDR           (ISP2_STM1_T_P1_BUF_ADDR + 4 * 4608)
#define ISP2_REMOVAL_P1_C_BUF_ADDR           (ISP2_STM1_T_P1_BUF_ADDR + 5 * 4608)

#define ISP2_STM1_S_P0_BUF_ADDR             (ISP1_STM1_T_P0_BUF_ADDR  )
#define ISP2_STM1_S_P0_A_BUF_ADDR           (ISP1_STM1_T_P0_A_BUF_ADDR)
#define ISP2_STM1_S_P0_B_BUF_ADDR           (ISP1_STM1_T_P0_B_BUF_ADDR)
#define ISP2_STM1_S_P0_C_BUF_ADDR           (ISP1_STM1_T_P0_C_BUF_ADDR)
#define ISP2_STM1_S_P1_BUF_ADDR             (ISP1_STM1_T_P1_BUF_ADDR  )
#define ISP2_STM1_S_P1_A_BUF_ADDR           (ISP1_STM1_T_P1_A_BUF_ADDR)
#define ISP2_STM1_S_P1_B_BUF_ADDR           (ISP1_STM1_T_P1_B_BUF_ADDR)
#define ISP2_STM1_S_P1_C_BUF_ADDR           (ISP1_STM1_T_P1_C_BUF_ADDR)
#define ISP2_STM1_S_P2_BUF_ADDR             (ISP1_STM1_T_P2_BUF_ADDR  )
#define ISP2_STM1_S_P2_A_BUF_ADDR           (ISP1_STM1_T_P2_A_BUF_ADDR)
#define ISP2_STM1_S_P2_B_BUF_ADDR           (ISP1_STM1_T_P2_B_BUF_ADDR)
#define ISP2_STM1_S_P2_C_BUF_ADDR           (ISP1_STM1_T_P2_C_BUF_ADDR)

#define ISP1_STM1_T_P3_A_BUF_ADDR           (ISP2_STM1_T_P1_BUF_ADDR + 1 * 4608)
#define ISP1_STM1_T_P3_B_BUF_ADDR           (ISP2_STM1_T_P1_BUF_ADDR + 3 * 4608)
#define ISP1_STM1_T_P3_C_BUF_ADDR           (ISP2_STM1_T_P1_BUF_ADDR + 5 * 4608)
#define ISP1_STM1_T_P4_A_BUF_ADDR           (ISP2_STM1_T_P0_BUF_ADDR + 0 * 4608)                /* �@��ISP2 0x10000000 */
#define ISP1_STM1_T_P4_B_BUF_ADDR           (ISP2_STM1_T_P0_BUF_ADDR + 1 * 4608)
#define ISP1_STM1_T_P4_C_BUF_ADDR           (ISP2_STM1_T_P0_BUF_ADDR + 2 * 4608)
#define ISP1_STM1_T_P5_A_BUF_ADDR           (ISP2_STM1_T_P0_BUF_ADDR + 3 * 4608)
#define ISP1_STM1_T_P5_B_BUF_ADDR           (ISP2_STM1_T_P0_BUF_ADDR + 4 * 4608)
#define ISP1_STM1_T_P5_C_BUF_ADDR           (ISP2_STM1_T_P0_BUF_ADDR + 5 * 4608)

//P0: 0xD402400 ~ 0xF802400
//P1: 0xD404800 ~ 0xF804800
#define ISP2_STM2_T_P0_BUF_ADDR             (0xD402400 + 64 + 128 * 2)
#define ISP2_STM2_T_P0_A_BUF_ADDR           (ISP2_STM2_T_P0_BUF_ADDR + 0 * 1536 * 2)
#define ISP2_STM2_T_P0_B_BUF_ADDR           (ISP2_STM2_T_P0_BUF_ADDR + 1 * 1536 * 2)
#define ISP2_STM2_T_P0_C_BUF_ADDR           (ISP2_STM2_T_P0_BUF_ADDR + 2 * 1536 * 2)
#define ISP2_STM2_T_P1_BUF_ADDR             (0xD404800 + 64 + 128 * 2)
#define ISP2_STM2_T_P1_A_BUF_ADDR           (ISP2_STM2_T_P1_BUF_ADDR + 0 * 1536 * 2)
#define ISP2_STM2_T_P1_B_BUF_ADDR           (ISP2_STM2_T_P1_BUF_ADDR + 1 * 1536 * 2)
#define ISP2_STM2_T_P1_C_BUF_ADDR           (ISP2_STM2_T_P1_BUF_ADDR + 2 * 1536 * 2)

#define ISP2_STM2_S_P0_BUF_ADDR             (ISP1_STM2_T_P0_BUF_ADDR  )
#define ISP2_STM2_S_P0_A_BUF_ADDR           (ISP1_STM2_T_P0_A_BUF_ADDR)
#define ISP2_STM2_S_P0_B_BUF_ADDR           (ISP1_STM2_T_P0_B_BUF_ADDR)
#define ISP2_STM2_S_P0_C_BUF_ADDR           (ISP1_STM2_T_P0_C_BUF_ADDR)
#define ISP2_STM2_S_P1_BUF_ADDR             (ISP1_STM2_T_P1_BUF_ADDR  )
#define ISP2_STM2_S_P1_A_BUF_ADDR           (ISP1_STM2_T_P1_A_BUF_ADDR)
#define ISP2_STM2_S_P1_B_BUF_ADDR           (ISP1_STM2_T_P1_B_BUF_ADDR)
#define ISP2_STM2_S_P1_C_BUF_ADDR           (ISP1_STM2_T_P1_C_BUF_ADDR)

//(H_Pixel = 192 * DS_ISP2_Column)
#define DS_ISP2_Column			8
#define DS_ISP2_Column_A		DS_ISP2_Column - 1
#define DS_ISP2_Column_B		DS_ISP2_Column - 1
#define DS_ISP2_Column_C		DS_ISP2_Column - 1

#define DS_ISP2_Line			1152
#define DS_ISP2_Line_A			DS_ISP2_Line / 2
#define DS_ISP2_Line_B			DS_ISP2_Line / 2
#define DS_ISP2_Line_C			DS_ISP2_Line / 2

#define F2_DEFECT_TABLE_ADDR	0x1F901C00
#define F2_DEFECT_TABLE_A_ADDR	(F2_DEFECT_TABLE_ADDR + 0 * 0xC00)
#define F2_DEFECT_TABLE_B_ADDR	(F2_DEFECT_TABLE_ADDR + 1 * 0xC00)
#define F2_DEFECT_TABLE_C_ADDR	(F2_DEFECT_TABLE_ADDR + 2 * 0xC00)
#define F2_DEFECT_TABLE_D_ADDR	(F2_DEFECT_TABLE_ADDR + 3 * 0xC00)
#define F2_DEFECT_TABLE_E_ADDR	(F2_DEFECT_TABLE_ADDR + 4 * 0xC00)
#define FX_DEFECT_TABLE_ADDR	0x1F901C00
#define FX_DEFECT_TABLE_A_ADDR	(FX_DEFECT_TABLE_ADDR + 0 * 0xC00)
#define FX_DEFECT_TABLE_B_ADDR	(FX_DEFECT_TABLE_ADDR + 1 * 0xC00)
#define FX_DEFECT_TABLE_C_ADDR	(FX_DEFECT_TABLE_ADDR + 2 * 0xC00)

#define DS_ST_YSIZE			    18
#define DS_ST_XSIZE			    24
#define DS_ST_BSIZE			    DS_ST_YSIZE * DS_ST_XSIZE

//P0: 0x0000000 ~ 0xB400000
//P1: 0x0004000 ~ 0xB404000
#define ST_STM1_P0_S_ADDR       (ISP2_STM1_T_P0_BUF_ADDR - 128 * 2 - 128 *0x8000)
#define ST_STM1_P1_S_ADDR       (ISP2_STM1_T_P1_BUF_ADDR - 128 * 2 - 128 *0x8000)
//size:0x68C0000
#define ST_STM1_P0_T_ADDR       0x00000000
#define ST_STM1_P1_T_ADDR       0x14000000
//P0: 0xB400000 ~ 0xF000000
//P1: 0xB404000 ~ 0xF004000
#define ST_STM2_P0_S_ADDR       (ISP2_STM2_T_P0_BUF_ADDR - 128 * 2 - 128 *0x8000)
#define ST_STM2_P1_S_ADDR       (ISP2_STM2_T_P1_BUF_ADDR - 128 * 2 - 128 *0x8000)
#define ST_STM2_P0_T_ADDR       0x07405BC0
#define ST_STM2_P1_T_ADDR       0x1B405BC0
#define ST_STM2_P2_T_ADDR       0x16005BC0
//ST CMD
#define F2_0_ST_CMD_ADDR        0x0F000000
#define F2_1_ST_CMD_ADDR        0x0F100000
#define FX_ST_CMD_ADDR          0x1F000000
//YUV
//size: 12:180*90*8byte*8page
#define F0_ST_YUV_ADDR          0x10000000
#define F1_ST_YUV_ADDR          0x10020000

//size:0x80000 * 2
#define F2_0_TEST_ST_CMD_ADDR	0x0F600000
#define F2_1_TEST_ST_CMD_ADDR	0x0F640000
#define FX_TEST_ST_CMD_ADDR		0x1D800000

//ST Sensor Cmd Addr
#define F2_0_ST_S_CMD_ADDR		0x0F680000
#define F2_1_ST_S_CMD_ADDR		0x0F6C0000
#define FX_ST_S_CMD_ADDR		0x1D840000

#define F2_0_ST_TRAN_CMD_ADDR   0x0F700000
#define F2_1_ST_TRAN_CMD_ADDR   0x0F720000
#define FX_ST_TRAN_CMD_ADDR     0x1F600000

#define ST_YUV_LINE_STM1_P0_T_ADDR	(ST_STM1_P0_T_ADDR + 0x0005DC0)
#define ST_YUV_LINE_STM1_P1_T_ADDR	(ST_STM1_P0_T_ADDR + 0x0005DC0)
#define ST_Z_V_LINE_STM1_P0_T_ADDR	(ST_STM1_P0_T_ADDR + 0x00065C0)
#define ST_Z_V_LINE_STM1_P1_T_ADDR	(ST_STM1_P0_T_ADDR + 0x00065C0)
#define ST_Z_H_LINE_STM1_P0_T_ADDR	(ST_STM1_P0_T_ADDR + 0x0006DC0)
#define ST_Z_H_LINE_STM1_P1_T_ADDR	(ST_STM1_P0_T_ADDR + 0x0006DC0)

#define ST_YUV_LINE_STM2_P0_T_ADDR	(ST_STM1_P0_T_ADDR + 0x0005DC0)
#define ST_YUV_LINE_STM2_P1_T_ADDR	(ST_STM1_P0_T_ADDR + 0x0005DC0)
#define ST_Z_V_LINE_STM2_P0_T_ADDR	(ST_STM1_P0_T_ADDR + 0x00065C0)
#define ST_Z_V_LINE_STM2_P1_T_ADDR	(ST_STM1_P0_T_ADDR + 0x00065C0)
#define ST_Z_H_LINE_STM2_P0_T_ADDR	(ST_STM1_P0_T_ADDR + 0x0006DC0)
#define ST_Z_H_LINE_STM2_P1_T_ADDR	(ST_STM1_P0_T_ADDR + 0x0006DC0)

//size: 12K: (424+632)*8byte*8page
#define YUV_LINE_YUV_OFFSET			0x2000
#define F0_YUV_LINE_YUV_ADDR        0x10070000		                    /* F2_DDR */
#define F0_YUV_LINE_YUV_P0_ADDR     (F0_YUV_LINE_YUV_ADDR + 0 * YUV_LINE_YUV_OFFSET)
#define F0_YUV_LINE_YUV_P1_ADDR     (F0_YUV_LINE_YUV_ADDR + 1 * YUV_LINE_YUV_OFFSET)
#define F0_YUV_LINE_YUV_P2_ADDR     (F0_YUV_LINE_YUV_ADDR + 2 * YUV_LINE_YUV_OFFSET)
#define F0_YUV_LINE_YUV_P3_ADDR     (F0_YUV_LINE_YUV_ADDR + 3 * YUV_LINE_YUV_OFFSET)
#define F0_YUV_LINE_YUV_P4_ADDR     (F0_YUV_LINE_YUV_ADDR + 4 * YUV_LINE_YUV_OFFSET)
#define F0_YUV_LINE_YUV_P5_ADDR     (F0_YUV_LINE_YUV_ADDR + 5 * YUV_LINE_YUV_OFFSET)
#define F0_YUV_LINE_YUV_P6_ADDR     (F0_YUV_LINE_YUV_ADDR + 6 * YUV_LINE_YUV_OFFSET)
#define F0_YUV_LINE_YUV_P7_ADDR     (F0_YUV_LINE_YUV_ADDR + 7 * YUV_LINE_YUV_OFFSET)
#define F1_YUV_LINE_YUV_ADDR        0x10080000		                    /* F2_DDR */
#define F1_YUV_LINE_YUV_P0_ADDR     (F1_YUV_LINE_YUV_ADDR + 0 * YUV_LINE_YUV_OFFSET)
#define F1_YUV_LINE_YUV_P1_ADDR     (F1_YUV_LINE_YUV_ADDR + 1 * YUV_LINE_YUV_OFFSET)
#define F1_YUV_LINE_YUV_P2_ADDR     (F1_YUV_LINE_YUV_ADDR + 2 * YUV_LINE_YUV_OFFSET)
#define F1_YUV_LINE_YUV_P3_ADDR     (F1_YUV_LINE_YUV_ADDR + 3 * YUV_LINE_YUV_OFFSET)
#define F1_YUV_LINE_YUV_P4_ADDR     (F1_YUV_LINE_YUV_ADDR + 4 * YUV_LINE_YUV_OFFSET)
#define F1_YUV_LINE_YUV_P5_ADDR     (F1_YUV_LINE_YUV_ADDR + 5 * YUV_LINE_YUV_OFFSET)
#define F1_YUV_LINE_YUV_P6_ADDR     (F1_YUV_LINE_YUV_ADDR + 6 * YUV_LINE_YUV_OFFSET)
#define F1_YUV_LINE_YUV_P7_ADDR     (F1_YUV_LINE_YUV_ADDR + 7 * YUV_LINE_YUV_OFFSET)

#define F0_Z_V_LINE_YUV_ADDR        0x10050000
#define F1_Z_V_LINE_YUV_ADDR        0x10052000
#define F0_Z_H_LINE_YUV_ADDR        0x10054000
#define F1_Z_H_LINE_YUV_ADDR        0x10056000

#define IMCP_Z_V_LINE_STM1_P0_S_ADDR	ST_Z_V_LINE_STM1_P0_T_ADDR
#define IMCP_Z_V_LINE_STM1_P1_S_ADDR	ST_Z_V_LINE_STM1_P1_T_ADDR
#define IMCP_Z_V_LINE_STM2_P0_S_ADDR	ST_Z_V_LINE_STM2_P0_T_ADDR
#define IMCP_Z_V_LINE_STM2_P1_S_ADDR	ST_Z_V_LINE_STM2_P1_T_ADDR
#define IMCP_Z_V_LINE_T_ADDR	    0x10058000
#define IMCP_Z_V_LINE_P0_T_ADDR		(IMCP_Z_V_LINE_T_ADDR + 0x1000 * 0)
#define IMCP_Z_V_LINE_P1_T_ADDR		(IMCP_Z_V_LINE_T_ADDR + 0x1000 * 1)
#define IMCP_Z_V_LINE_P2_T_ADDR		(IMCP_Z_V_LINE_T_ADDR + 0x1000 * 2)
#define IMCP_Z_V_LINE_P3_T_ADDR		(IMCP_Z_V_LINE_T_ADDR + 0x1000 * 3)

#define IMCP_Z_H_LINE_STM1_P0_S_ADDR	ST_Z_H_LINE_STM1_P0_T_ADDR
#define IMCP_Z_H_LINE_STM1_P1_S_ADDR	ST_Z_H_LINE_STM1_P1_T_ADDR
#define IMCP_Z_H_LINE_STM2_P0_S_ADDR	ST_Z_H_LINE_STM2_P0_T_ADDR
#define IMCP_Z_H_LINE_STM2_P1_S_ADDR	ST_Z_H_LINE_STM2_P1_T_ADDR
#define IMCP_Z_H_LINE_T_ADDR	    0x1005C000
#define IMCP_Z_H_LINE_P0_T_ADDR	(IMCP_Z_H_LINE_T_ADDR + 0x1000 * 0)
#define IMCP_Z_H_LINE_P1_T_ADDR	(IMCP_Z_H_LINE_T_ADDR + 0x1000 * 1)
#define IMCP_Z_H_LINE_P2_T_ADDR	(IMCP_Z_H_LINE_T_ADDR + 0x1000 * 2)
#define IMCP_Z_H_LINE_P3_T_ADDR	(IMCP_Z_H_LINE_T_ADDR + 0x1000 * 3)

#define F2_F0_SPEED_ADDR		0x0F740000
#define F2_F1_SPEED_ADDR		0x0F740100

#define F2_NR2D_C_ADDR		 	0x0F741000
#define F2_NR2D_Y_ADDR		 	0x0F741800

#define DS_H_PIXEL_SIZE_FS	   		4608
#define DS_H_PIXEL_SIZE_D2	   		2560
#define DS_H_PIXEL_SIZE_D3	   		1536

//size:2048 byte, 16page
#define JPEG_HEADER_12K_ADDR            0xF300000
#define JPEG_HEADER_8K_ADDR             0xF308000
#define JPEG_HEADER_6K_ADDR             0xF310000
#define JPEG_HEADER_4K_ADDR             0xF318000
#define JPEG_HEADER_3K_ADDR             0xF320000
#define JPEG_HEADER_2K_ADDR             0xF328000
#define JPEG_HEADER_THM_ADDR            0xF330000
#define JPEG_HEADER_S_ADDR            	0xF338000
#define JPEG_HEADER_TEST_ADDR           0xF340000
#define JPEG_HEADER_3D_ADDR             0xF348000
// JPEG 16M  P0:0x10B00000 ~ 0x11B00000  P1:0x11B00000 ~ 0x12B00000
//size:0x1800000 * 2
#define JPEG_STM1_P0_T_BUF_ADDR         0x10200000
#define JPEG_STM1_P1_T_BUF_ADDR         0x11A00000
#define JPEG_STM1_P0_S_BUF_ADDR         ST_STM1_P0_T_ADDR
#define JPEG_STM1_P1_S_BUF_ADDR         ST_STM1_P1_T_ADDR
// JPEG 3M  P0:0x12B00000 ~ 0x12E00000  P1:0x12E00000 ~ 0x13100000
//size:0x300000 * 2
#define JPEG_STM2_P0_T_BUF_ADDR         0x13200000
#define JPEG_STM2_P1_T_BUF_ADDR         0x13400000
#define JPEG_STM2_P2_T_BUF_ADDR         0x13600000
#define JPEG_STM2_P3_T_BUF_ADDR         0x13800000
#define JPEG_STM2_P0_S_BUF_ADDR         ST_STM2_P0_T_ADDR
#define JPEG_STM2_P1_S_BUF_ADDR         ST_STM2_P1_T_ADDR
#define JPEG_STM2_P2_S_BUF_ADDR         ST_STM2_P2_T_ADDR

#define H264_STM1_P0_S_BUF_ADDR         ST_STM1_P0_T_ADDR
#define H264_STM1_P1_S_BUF_ADDR         ST_STM1_P1_T_ADDR
#define H264_STM2_P0_S_BUF_ADDR         ST_STM2_P0_T_ADDR
#define H264_STM2_P1_S_BUF_ADDR         ST_STM2_P1_T_ADDR
#define H264_STM1_P0_T_BUF_ADDR         JPEG_STM1_P0_T_BUF_ADDR
#define H264_STM1_P1_T_BUF_ADDR         JPEG_STM1_P1_T_BUF_ADDR
#define H264_STM2_P0_T_BUF_ADDR         JPEG_STM2_P0_T_BUF_ADDR
#define H264_STM2_P1_T_BUF_ADDR         JPEG_STM2_P1_T_BUF_ADDR
#define H264_STM2_P2_T_BUF_ADDR         JPEG_STM2_P2_T_BUF_ADDR
#define H264_STM2_P3_T_BUF_ADDR         JPEG_STM2_P3_T_BUF_ADDR
//size:7680*3840*2
//�ݹ��0x8000*2
#define H264_STM1_P0_M_BUF_ADDR         0x0B500000              /* 189792256 */
//size:0x100000
//#define H264_MOTION_BUF_ADDR         	0x10100000
#define H264_MOTION_BUF_ADDR         	0x1F400000

//3840*512*2
#define DMA_BOTTOM_IMG_BUF_ADDR			0x14005DC0
#define DMA_BOTTOM_IMG_BUF_2_ADDR		0x15005DC0

//1024*128*2
#define DMA_BOTTOM_IMG_BUF_3_ADDR		0x1F500000
#define DMA_BOTTOM_IMG_BUF_4_ADDR		0x1F500800

#define USB_STM1_P0_S_BUF_ADDR         JPEG_STM1_P0_T_BUF_ADDR
#define USB_STM1_P1_S_BUF_ADDR         JPEG_STM1_P1_T_BUF_ADDR
#define USB_STM2_P0_S_BUF_ADDR         JPEG_STM2_P0_T_BUF_ADDR
#define USB_STM2_P1_S_BUF_ADDR         JPEG_STM2_P1_T_BUF_ADDR
#define USB_STM2_P2_S_BUF_ADDR         JPEG_STM2_P2_T_BUF_ADDR
#define USB_STM2_P3_S_BUF_ADDR         JPEG_STM2_P3_T_BUF_ADDR

#define USB_STM1_H264_P0_S_BUF_ADDR    H264_STM1_P0_T_BUF_ADDR
#define USB_STM1_H264_P1_S_BUF_ADDR    H264_STM1_P1_T_BUF_ADDR
#define USB_STM2_H264_P0_S_BUF_ADDR    H264_STM2_P0_T_BUF_ADDR
#define USB_STM2_H264_P1_S_BUF_ADDR    H264_STM2_P1_T_BUF_ADDR
#define USB_STM2_H264_P2_S_BUF_ADDR    H264_STM2_P2_T_BUF_ADDR
#define USB_STM2_H264_P3_S_BUF_ADDR    H264_STM2_P3_T_BUF_ADDR

//---------------------------------------------------------------------------------
//F2_0:0x13100000 ~ 0x13300000  F2_1:0x13300000 ~ 0x13500000
//size:0x200000 * 2
#define F2_0_MP_GAIN_ADDR	   0x0F800000
#define F2_1_MP_GAIN_ADDR	   0x0FA00000
#define F2_0_MP_GAIN_2_ADDR	   0x13A00000
#define F2_1_MP_GAIN_2_ADDR	   0x13C00000
//FX:0xFC00000 ~ 0xFE00000
#define FX_MP_GAIN_ADDR	       0x1F300000
#define FX_MP_GAIN_2_ADDR	   0x1F700000
//F2:0x13500000 ~ 0x13600000
//size:0x30000
#define F2_0_SPI_IO_SMOOTH_PHI_DATA 		0x0FC01000
#define F2_0_SPI_IO_SMOOTH_PHI_P0_DATA 		(F2_0_SPI_IO_SMOOTH_PHI_DATA + 0 * 0x1000)
#define F2_0_SPI_IO_SMOOTH_PHI_P1_DATA 		(F2_0_SPI_IO_SMOOTH_PHI_DATA + 1 * 0x1000)
#define F2_0_SPI_IO_SMOOTH_PHI_P2_DATA 		(F2_0_SPI_IO_SMOOTH_PHI_DATA + 2 * 0x1000)
#define F2_0_SPI_IO_SMOOTH_PHI_P3_DATA 		(F2_0_SPI_IO_SMOOTH_PHI_DATA + 3 * 0x1000)
#define F2_1_SPI_IO_SMOOTH_PHI_DATA 		0x0FC05000
#define F2_1_SPI_IO_SMOOTH_PHI_P0_DATA 		(F2_1_SPI_IO_SMOOTH_PHI_DATA + 0 * 0x1000)
#define F2_1_SPI_IO_SMOOTH_PHI_P1_DATA 		(F2_1_SPI_IO_SMOOTH_PHI_DATA + 1 * 0x1000)
#define F2_1_SPI_IO_SMOOTH_PHI_P2_DATA 		(F2_1_SPI_IO_SMOOTH_PHI_DATA + 2 * 0x1000)
#define F2_1_SPI_IO_SMOOTH_PHI_P3_DATA 		(F2_1_SPI_IO_SMOOTH_PHI_DATA + 3 * 0x1000)
//#define F2_0_SPI_IO_SMOOTH_PHI_2_DATA 		0x0FC08000
//#define F2_0_SPI_IO_SMOOTH_PHI_2_P0_DATA 	(F2_0_SPI_IO_SMOOTH_PHI_2_DATA + 0 * 0x1000)
//#define F2_0_SPI_IO_SMOOTH_PHI_2_P1_DATA 	(F2_0_SPI_IO_SMOOTH_PHI_2_DATA + 1 * 0x1000)
//#define F2_0_SPI_IO_SMOOTH_PHI_2_P2_DATA 	(F2_0_SPI_IO_SMOOTH_PHI_2_DATA + 2 * 0x1000)
//#define F2_0_SPI_IO_SMOOTH_PHI_2_P3_DATA 	(F2_0_SPI_IO_SMOOTH_PHI_2_DATA + 3 * 0x1000)
//#define F2_1_SPI_IO_SMOOTH_PHI_2_DATA 		0x0FC0C000
//#define F2_1_SPI_IO_SMOOTH_PHI_2_P0_DATA 	(F2_1_SPI_IO_SMOOTH_PHI_2_DATA + 0 * 0x1000)
//#define F2_1_SPI_IO_SMOOTH_PHI_2_P1_DATA 	(F2_1_SPI_IO_SMOOTH_PHI_2_DATA + 1 * 0x1000)
//#define F2_1_SPI_IO_SMOOTH_PHI_2_P2_DATA 	(F2_1_SPI_IO_SMOOTH_PHI_2_DATA + 2 * 0x1000)
//#define F2_1_SPI_IO_SMOOTH_PHI_2_P3_DATA 	(F2_1_SPI_IO_SMOOTH_PHI_2_DATA + 3 * 0x1000)
//FX:0xFE00000 ~ 0xFF00000
#define FX_SPI_IO_SMOOTH_PHI_DATA 			0x1F500000
#define FX_SPI_IO_SMOOTH_PHI_P0_DATA 		(FX_SPI_IO_SMOOTH_PHI_DATA + 0 * 0x1000)
#define FX_SPI_IO_SMOOTH_PHI_P1_DATA 		(FX_SPI_IO_SMOOTH_PHI_DATA + 1 * 0x1000)
#define FX_SPI_IO_SMOOTH_PHI_P2_DATA 		(FX_SPI_IO_SMOOTH_PHI_DATA + 2 * 0x1000)
#define FX_SPI_IO_SMOOTH_PHI_P3_DATA 		(FX_SPI_IO_SMOOTH_PHI_DATA + 3 * 0x1000)
//#define FX_SPI_IO_SMOOTH_PHI_2_DATA 		0x1F504000
//#define FX_SPI_IO_SMOOTH_PHI_2_P0_DATA 		(FX_SPI_IO_SMOOTH_PHI_2_DATA + 0 * 0x1000)
//#define FX_SPI_IO_SMOOTH_PHI_2_P1_DATA 		(FX_SPI_IO_SMOOTH_PHI_2_DATA + 1 * 0x1000)
//#define FX_SPI_IO_SMOOTH_PHI_2_P2_DATA 		(FX_SPI_IO_SMOOTH_PHI_2_DATA + 2 * 0x1000)
//#define FX_SPI_IO_SMOOTH_PHI_2_P3_DATA 		(FX_SPI_IO_SMOOTH_PHI_2_DATA + 3 * 0x1000)



//FX:1D400000 ~ 1D800000
#define ST_O_CMD_ADDR          0x1D400000
#define ST_O_CMD_P0_ADDR       (ST_O_CMD_ADDR + 0 * 0x100000)
#define ST_O_CMD_P1_ADDR       (ST_O_CMD_ADDR + 1 * 0x100000)
#define ST_O_CMD_P2_ADDR       (ST_O_CMD_ADDR + 2 * 0x100000)
#define ST_O_CMD_P3_ADDR       (ST_O_CMD_ADDR + 3 * 0x100000)
//FX:1D900000 ~ 1DD00000
#define ST_O_2_CMD_ADDR         0x1D900000
#define ST_O_2_CMD_P0_ADDR      (ST_O_2_CMD_ADDR + 0 * 0x100000)
#define ST_O_2_CMD_P1_ADDR      (ST_O_2_CMD_ADDR + 1 * 0x100000)
#define ST_O_2_CMD_P2_ADDR      (ST_O_2_CMD_ADDR + 2 * 0x100000)
#define ST_O_2_CMD_P3_ADDR      (ST_O_2_CMD_ADDR + 3 * 0x100000)
//F2:
//size:64
#define ST_CMD_MIX_ADDR        		0x0FC10000
#define SPI_IO_SMOOTH_PHI_ADDR 		0xCCAA0984
#define SPI_IO_MP_BASE_ADDR    		0xCCAA0988
#define SPI_IO_MP_GAIN_ADDR    		0xCCAA098C
#define SPI_IO_MP_SIZE_ADDR    		0xCCAA0990
#define SPI_IO_MP_ST_O_ADDR    		0xCCAA0994
#define SPI_IO_SMOOTH_PHI_2_ADDR 	0xCCAA09A4
#define SPI_IO_MP_BASE_2_ADDR    	0xCCAA09A8
#define SPI_IO_MP_GAIN_2_ADDR    	0xCCAA09AC
#define SPI_IO_MP_SIZE_2_ADDR    	0xCCAA09B0
#define SPI_IO_MP_ST_O_2_ADDR    	0xCCAA09B4
#define SPI_IO_MP_ST_O_EN_ADDR 		0xCCAA09C0

#define SPI_IO_MP_BASE_DATA    		FX_ST_CMD_ADDR
#define SPI_IO_MP_GAIN_DATA    		FX_MP_GAIN_ADDR
#define SPI_IO_MP_ST_O_P0_DATA  	ST_O_CMD_P0_ADDR
#define SPI_IO_MP_ST_O_P1_DATA  	ST_O_CMD_P1_ADDR
#define SPI_IO_MP_ST_O_P2_DATA  	ST_O_CMD_P2_ADDR
#define SPI_IO_MP_ST_O_P3_DATA  	ST_O_CMD_P3_ADDR
#define SPI_IO_MP_BASE_2_DATA    	ST_O_CMD_P0_ADDR
#define SPI_IO_MP_GAIN_2_DATA    	FX_MP_GAIN_2_ADDR
#define SPI_IO_MP_ST_O_2_P0_DATA  	ST_O_2_CMD_P0_ADDR
#define SPI_IO_MP_ST_O_2_P1_DATA  	ST_O_2_CMD_P1_ADDR
#define SPI_IO_MP_ST_O_2_P2_DATA  	ST_O_2_CMD_P2_ADDR
#define SPI_IO_MP_ST_O_2_P3_DATA  	ST_O_2_CMD_P3_ADDR



//#define F2_LENS_SKIP_F0_A_ADDR	0x0F500200
//#define F2_LENS_SKIP_F0_B_ADDR	0x0F500400
//#define F2_LENS_SKIP_F0_C_ADDR	0x0F500600
//#define F2_LENS_SKIP_F1_A_ADDR	0x0F500800
//#define F2_LENS_SKIP_F1_B_ADDR	0x0F500A00
//#define F2_LENS_SKIP_F1_C_ADDR	0x0F500C00

#define FX_WDR_TABLE_ADDR       0x1F640000                         /* 0x1F640000 = 526647296 */
#define F2_WDR_TABLE_ADDR       0x0F502000

#define FX_WDR_LIVE_TABLE_ADDR	0x1F642000
#define F2_WDR_LIVE_TABLE_ADDR  0x0F507000

#define FX_WDR_IMG_P0_ADDR      0x0006D40                          /* 0x0006D40 = 27968 */
#define FX_WDR_IMG_A_P0_ADDR	(FX_WDR_IMG_P0_ADDR + 0 * 768)     /* 0x300 = 768 */
#define FX_WDR_IMG_B_P0_ADDR	(FX_WDR_IMG_P0_ADDR + 1 * 768)
#define FX_WDR_IMG_C_P0_ADDR	(FX_WDR_IMG_P0_ADDR + 2 * 768)
#define FX_WDR_MO_P0_ADDR   	0x0D06D40                          /* 0x0D06D40 = 13659456 */
#define FX_WDR_MO_A_P0_ADDR		(FX_WDR_MO_P0_ADDR + 0 * 768)     /* 0x300 = 768 */
#define FX_WDR_MO_B_P0_ADDR		(FX_WDR_MO_P0_ADDR + 1 * 768)
#define FX_WDR_MO_C_P0_ADDR		(FX_WDR_MO_P0_ADDR + 2 * 768)
#define FX_WDR_MO_T_P0_ADDR   	0x15807700                        	/* 0x15807700 = 360740608 */
#define FX_WDR_MO_T_A_P0_ADDR	(FX_WDR_MO_T_P0_ADDR + 0 * 768)     /* 0x300 = 768 */
#define FX_WDR_MO_T_B_P0_ADDR	(FX_WDR_MO_T_P0_ADDR + 1 * 768)
#define FX_WDR_MO_T_C_P0_ADDR	(FX_WDR_MO_T_P0_ADDR + 2 * 768)
#define FX_WDR_MO_DIFF_P0_ADDR  	0x0D406D40
#define FX_WDR_MO_DIFF_A_P0_ADDR	(FX_WDR_MO_DIFF_P0_ADDR + 0 * 768)     /* 0x300 = 768 */
#define FX_WDR_MO_DIFF_B_P0_ADDR	(FX_WDR_MO_DIFF_P0_ADDR + 1 * 768)
#define FX_WDR_MO_DIFF_C_P0_ADDR	(FX_WDR_MO_DIFF_P0_ADDR + 2 * 768)
#define FX_WDR_DIF_P0_ADDR      0x0007640                          /* 0x0007640 = 30272 */
#define FX_WDR_DIF_A_P0_ADDR	(FX_WDR_DIF_P0_ADDR + 0 * 576)     /* �̤� 544 */
#define FX_WDR_DIF_B_P0_ADDR	(FX_WDR_DIF_P0_ADDR + 1 * 576)
#define FX_WDR_DIF_C_P0_ADDR	(FX_WDR_DIF_P0_ADDR + 2 * 576)
#define FX_WDR_DIF_2_P0_ADDR    0x00D07640                         /* 0x0007500 = 29952 */
#define FX_WDR_DIF_2_A_P0_ADDR	(FX_WDR_DIF_2_P0_ADDR + 0 * 576)     /* �̤� 544 */
#define FX_WDR_DIF_2_B_P0_ADDR	(FX_WDR_DIF_2_P0_ADDR + 1 * 576)
#define FX_WDR_DIF_2_C_P0_ADDR	(FX_WDR_DIF_2_P0_ADDR + 2 * 576)

#define FX_WDR_IMG_P1_ADDR		0x1A86D40                           /* 0x1A86D40 = 27815232 */
#define FX_WDR_IMG_A_P1_ADDR	(FX_WDR_IMG_P1_ADDR + 0 * 768)
#define FX_WDR_IMG_B_P1_ADDR	(FX_WDR_IMG_P1_ADDR + 1 * 768)
#define FX_WDR_IMG_C_P1_ADDR	(FX_WDR_IMG_P1_ADDR + 2 * 768)
#define FX_WDR_MO_P1_ADDR		0x2786D40                           /* 0x2786D40 = 41446720 */
#define FX_WDR_MO_A_P1_ADDR		(FX_WDR_MO_P1_ADDR + 0 * 768)
#define FX_WDR_MO_B_P1_ADDR		(FX_WDR_MO_P1_ADDR + 1 * 768)
#define FX_WDR_MO_C_P1_ADDR		(FX_WDR_MO_P1_ADDR + 2 * 768)
#define FX_WDR_MO_T_P1_ADDR   	0x16507700                        	/* 0x16507700 = 374372096 */
#define FX_WDR_MO_T_A_P1_ADDR	(FX_WDR_MO_T_P1_ADDR + 0 * 768)     /* 0x300 = 768 */
#define FX_WDR_MO_T_B_P1_ADDR	(FX_WDR_MO_T_P1_ADDR + 1 * 768)
#define FX_WDR_MO_T_C_P1_ADDR	(FX_WDR_MO_T_P1_ADDR + 2 * 768)
#define FX_WDR_MO_DIFF_P1_ADDR  	0x0D407640
#define FX_WDR_MO_DIFF_A_P1_ADDR	(FX_WDR_MO_DIFF_P1_ADDR + 0 * 768)     /* 0x300 = 768 */
#define FX_WDR_MO_DIFF_B_P1_ADDR	(FX_WDR_MO_DIFF_P1_ADDR + 1 * 768)
#define FX_WDR_MO_DIFF_C_P1_ADDR	(FX_WDR_MO_DIFF_P1_ADDR + 2 * 768)
#define FX_WDR_DIF_P1_ADDR      0x1A87640
#define FX_WDR_DIF_A_P1_ADDR	(FX_WDR_DIF_P1_ADDR + 0 * 576)
#define FX_WDR_DIF_B_P1_ADDR	(FX_WDR_DIF_P1_ADDR + 1 * 576)
#define FX_WDR_DIF_C_P1_ADDR	(FX_WDR_DIF_P1_ADDR + 2 * 576)
#define FX_WDR_DIF_2_P1_ADDR    0x02787640
#define FX_WDR_DIF_2_A_P1_ADDR	(FX_WDR_DIF_2_P1_ADDR + 0 * 576)
#define FX_WDR_DIF_2_B_P1_ADDR	(FX_WDR_DIF_2_P1_ADDR + 1 * 576)
#define FX_WDR_DIF_2_C_P1_ADDR	(FX_WDR_DIF_2_P1_ADDR + 2 * 576)

#define FX_WDR_IMG_P2_ADDR		0x3506D40                           /* 0x3506C00 = 55602496 */
#define FX_WDR_IMG_A_P2_ADDR	(FX_WDR_IMG_P2_ADDR + 0 * 768)
#define FX_WDR_IMG_B_P2_ADDR	(FX_WDR_IMG_P2_ADDR + 1 * 768)
#define FX_WDR_IMG_C_P2_ADDR	(FX_WDR_IMG_P2_ADDR + 2 * 768)
#define FX_WDR_MO_P2_ADDR		0x4206D40                           /* 0x4206D40 = 69233984 */
#define FX_WDR_MO_A_P2_ADDR		(FX_WDR_MO_P2_ADDR + 0 * 768)
#define FX_WDR_MO_B_P2_ADDR		(FX_WDR_MO_P2_ADDR + 1 * 768)
#define FX_WDR_MO_C_P2_ADDR		(FX_WDR_MO_P2_ADDR + 2 * 768)
#define FX_WDR_MO_T_P2_ADDR   	0x17207700                        	/* 0x17207700 = 388003584 */
#define FX_WDR_MO_T_A_P2_ADDR	(FX_WDR_MO_T_P2_ADDR + 0 * 768)     /* 0x300 = 768 */
#define FX_WDR_MO_T_B_P2_ADDR	(FX_WDR_MO_T_P2_ADDR + 1 * 768)
#define FX_WDR_MO_T_C_P2_ADDR	(FX_WDR_MO_T_P2_ADDR + 2 * 768)
#define FX_WDR_MO_DIFF_P2_ADDR  	0x0E106D40
#define FX_WDR_MO_DIFF_A_P2_ADDR	(FX_WDR_MO_DIFF_P2_ADDR + 0 * 768)     /* 0x300 = 768 */
#define FX_WDR_MO_DIFF_B_P2_ADDR	(FX_WDR_MO_DIFF_P2_ADDR + 1 * 768)
#define FX_WDR_MO_DIFF_C_P2_ADDR	(FX_WDR_MO_DIFF_P2_ADDR + 2 * 768)
#define FX_WDR_DIF_P2_ADDR      0x3507640
#define FX_WDR_DIF_A_P2_ADDR	(FX_WDR_DIF_P2_ADDR + 0 * 576)
#define FX_WDR_DIF_B_P2_ADDR	(FX_WDR_DIF_P2_ADDR + 1 * 576)
#define FX_WDR_DIF_C_P2_ADDR	(FX_WDR_DIF_P2_ADDR + 2 * 576)
#define FX_WDR_DIF_2_P2_ADDR    0x04207640
#define FX_WDR_DIF_2_A_P2_ADDR	(FX_WDR_DIF_2_P2_ADDR + 0 * 576)
#define FX_WDR_DIF_2_B_P2_ADDR	(FX_WDR_DIF_2_P2_ADDR + 1 * 576)
#define FX_WDR_DIF_2_C_P2_ADDR	(FX_WDR_DIF_2_P2_ADDR + 2 * 576)

#define FX_WDR_IMG_P3_ADDR		0x04F06D40                           /* 0x04F06D40 = 82865472 */
#define FX_WDR_IMG_A_P3_ADDR	(FX_WDR_IMG_P3_ADDR + 0 * 768)
#define FX_WDR_IMG_B_P3_ADDR	(FX_WDR_IMG_P3_ADDR + 1 * 768)
#define FX_WDR_IMG_C_P3_ADDR	(FX_WDR_IMG_P3_ADDR + 2 * 768)
#define FX_WDR_MO_P3_ADDR		0x05C06D40                           /* 0x05C06D40 = 96496960 */
#define FX_WDR_MO_A_P3_ADDR		(FX_WDR_MO_P3_ADDR + 0 * 768)
#define FX_WDR_MO_B_P3_ADDR		(FX_WDR_MO_P3_ADDR + 1 * 768)
#define FX_WDR_MO_C_P3_ADDR		(FX_WDR_MO_P3_ADDR + 2 * 768)
#define FX_WDR_MO_T_P3_ADDR   	0x17F07700                        	/* 0x17F07700 = 401635072 */
#define FX_WDR_MO_T_A_P3_ADDR	(FX_WDR_MO_T_P3_ADDR + 0 * 768)     /* 0x300 = 768 */
#define FX_WDR_MO_T_B_P3_ADDR	(FX_WDR_MO_T_P3_ADDR + 1 * 768)
#define FX_WDR_MO_T_C_P3_ADDR	(FX_WDR_MO_T_P3_ADDR + 2 * 768)
#define FX_WDR_MO_DIFF_P3_ADDR  	0x0E107640
#define FX_WDR_MO_DIFF_A_P3_ADDR	(FX_WDR_MO_DIFF_P3_ADDR + 0 * 768)     /* 0x300 = 768 */
#define FX_WDR_MO_DIFF_B_P3_ADDR	(FX_WDR_MO_DIFF_P3_ADDR + 1 * 768)
#define FX_WDR_MO_DIFF_C_P3_ADDR	(FX_WDR_MO_DIFF_P3_ADDR + 2 * 768)
#define FX_WDR_DIF_P3_ADDR      0x04F07640
#define FX_WDR_DIF_A_P3_ADDR	(FX_WDR_DIF_P3_ADDR + 0 * 576)
#define FX_WDR_DIF_B_P3_ADDR	(FX_WDR_DIF_P3_ADDR + 1 * 576)
#define FX_WDR_DIF_C_P3_ADDR	(FX_WDR_DIF_P3_ADDR + 2 * 576)
#define FX_WDR_DIF_2_P3_ADDR    0x05C07640
#define FX_WDR_DIF_2_A_P3_ADDR	(FX_WDR_DIF_2_P3_ADDR + 0 * 576)
#define FX_WDR_DIF_2_B_P3_ADDR	(FX_WDR_DIF_2_P3_ADDR + 1 * 576)
#define FX_WDR_DIF_2_C_P3_ADDR	(FX_WDR_DIF_2_P3_ADDR + 2 * 576)

#define FX_WDR_IMG_P4_ADDR      0x12406D40                           /* 0x12406D40 = 306212160 */
#define FX_WDR_IMG_A_P4_ADDR	(FX_WDR_IMG_P4_ADDR + 0 * 768)
#define FX_WDR_IMG_B_P4_ADDR	(FX_WDR_IMG_P4_ADDR + 1 * 768)
#define FX_WDR_IMG_C_P4_ADDR	(FX_WDR_IMG_P4_ADDR + 2 * 768)
#define FX_WDR_MO_P4_ADDR		0x13106D40                           /* 0x13106D40 = 319843648 */
#define FX_WDR_MO_A_P4_ADDR		(FX_WDR_MO_P4_ADDR + 0 * 768)
#define FX_WDR_MO_B_P4_ADDR		(FX_WDR_MO_P4_ADDR + 1 * 768)
#define FX_WDR_MO_C_P4_ADDR		(FX_WDR_MO_P4_ADDR + 2 * 768)
#define FX_WDR_MO_T_P4_ADDR   	0x1A607700                        	/* 0x1A607700 = 442529536 */
#define FX_WDR_MO_T_A_P4_ADDR	(FX_WDR_MO_T_P4_ADDR + 0 * 768)     /* 0x300 = 768 */
#define FX_WDR_MO_T_B_P4_ADDR	(FX_WDR_MO_T_P4_ADDR + 1 * 768)
#define FX_WDR_MO_T_C_P4_ADDR	(FX_WDR_MO_T_P4_ADDR + 2 * 768)
#define FX_WDR_MO_DIFF_P4_ADDR  	0x0EE06D40
#define FX_WDR_MO_DIFF_A_P4_ADDR	(FX_WDR_MO_DIFF_P4_ADDR + 0 * 768)     /* 0x300 = 768 */
#define FX_WDR_MO_DIFF_B_P4_ADDR	(FX_WDR_MO_DIFF_P4_ADDR + 1 * 768)
#define FX_WDR_MO_DIFF_C_P4_ADDR	(FX_WDR_MO_DIFF_P4_ADDR + 2 * 768)
#define FX_WDR_DIF_P4_ADDR      0x12407640
#define FX_WDR_DIF_A_P4_ADDR	(FX_WDR_DIF_P4_ADDR + 0 * 576)
#define FX_WDR_DIF_B_P4_ADDR	(FX_WDR_DIF_P4_ADDR + 1 * 576)
#define FX_WDR_DIF_C_P4_ADDR	(FX_WDR_DIF_P4_ADDR + 2 * 576)
#define FX_WDR_DIF_2_P4_ADDR    0x13107640
#define FX_WDR_DIF_2_A_P4_ADDR	(FX_WDR_DIF_2_P4_ADDR + 0 * 576)
#define FX_WDR_DIF_2_B_P4_ADDR	(FX_WDR_DIF_2_P4_ADDR + 1 * 576)
#define FX_WDR_DIF_2_C_P4_ADDR	(FX_WDR_DIF_2_P4_ADDR + 2 * 576)

#define FX_WDR_IMG_P5_ADDR      0x13E06D40                           /* 0x13E06D40 = 333475136 */
#define FX_WDR_IMG_A_P5_ADDR	(FX_WDR_IMG_P5_ADDR + 0 * 768)
#define FX_WDR_IMG_B_P5_ADDR	(FX_WDR_IMG_P5_ADDR + 1 * 768)
#define FX_WDR_IMG_C_P5_ADDR	(FX_WDR_IMG_P5_ADDR + 2 * 768)
#define FX_WDR_MO_P5_ADDR		0x14B06D40                           /* 0x14B06D40 = 347106624 */
#define FX_WDR_MO_A_P5_ADDR		(FX_WDR_MO_P5_ADDR + 0 * 768)
#define FX_WDR_MO_B_P5_ADDR		(FX_WDR_MO_P5_ADDR + 1 * 768)
#define FX_WDR_MO_C_P5_ADDR		(FX_WDR_MO_P5_ADDR + 2 * 768)
//#define FX_WDR_MO_T_P5_ADDR   	0x1B306D40                        	/* 0x1B306D40 = 456158528 */
#define FX_WDR_MO_T_P5_ADDR   	0x1B307700                        	/* 0x1B306D40 = 456161024 */
#define FX_WDR_MO_T_A_P5_ADDR	(FX_WDR_MO_T_P5_ADDR + 0 * 768)     /* 0x300 = 768 */
#define FX_WDR_MO_T_B_P5_ADDR	(FX_WDR_MO_T_P5_ADDR + 1 * 768)
#define FX_WDR_MO_T_C_P5_ADDR	(FX_WDR_MO_T_P5_ADDR + 2 * 768)
#define FX_WDR_MO_DIFF_P5_ADDR  	0x0EE07640
#define FX_WDR_MO_DIFF_A_P5_ADDR	(FX_WDR_MO_DIFF_P5_ADDR + 0 * 768)     /* 0x300 = 768 */
#define FX_WDR_MO_DIFF_B_P5_ADDR	(FX_WDR_MO_DIFF_P5_ADDR + 1 * 768)
#define FX_WDR_MO_DIFF_C_P5_ADDR	(FX_WDR_MO_DIFF_P5_ADDR + 2 * 768)
#define FX_WDR_DIF_P5_ADDR      0x13E07640
#define FX_WDR_DIF_A_P5_ADDR	(FX_WDR_DIF_P5_ADDR + 0 * 576)
#define FX_WDR_DIF_B_P5_ADDR	(FX_WDR_DIF_P5_ADDR + 1 * 576)
#define FX_WDR_DIF_C_P5_ADDR	(FX_WDR_DIF_P5_ADDR + 2 * 576)
#define FX_WDR_DIF_2_P5_ADDR    0x14B07640
#define FX_WDR_DIF_2_A_P5_ADDR	(FX_WDR_DIF_2_P5_ADDR + 0 * 576)
#define FX_WDR_DIF_2_B_P5_ADDR	(FX_WDR_DIF_2_P5_ADDR + 1 * 576)
#define FX_WDR_DIF_2_C_P5_ADDR	(FX_WDR_DIF_2_P5_ADDR + 2 * 576)

#define FX_WDR_IMG_P6_ADDR      0x18C06D40                           /* 0x18C06D40 = 415264064 */
#define FX_WDR_IMG_A_P6_ADDR	(FX_WDR_IMG_P6_ADDR + 0 * 768)
#define FX_WDR_IMG_B_P6_ADDR	(FX_WDR_IMG_P6_ADDR + 1 * 768)
#define FX_WDR_IMG_C_P6_ADDR	(FX_WDR_IMG_P6_ADDR + 2 * 768)
#define FX_WDR_MO_P6_ADDR		0x19906D40                           /* 0x19906D40 = 428895552 */
#define FX_WDR_MO_A_P6_ADDR		(FX_WDR_MO_P6_ADDR + 0 * 768)
#define FX_WDR_MO_B_P6_ADDR		(FX_WDR_MO_P6_ADDR + 1 * 768)
#define FX_WDR_MO_C_P6_ADDR		(FX_WDR_MO_P6_ADDR + 2 * 768)
//#define FX_WDR_MO_T_P6_ADDR   	0x1B307700                        	/* 0x1B307700 = 456161024 */
#define FX_WDR_MO_T_P6_ADDR   	0x1C007700                        	/* 0x1B307700 = 469792512 */
#define FX_WDR_MO_T_A_P6_ADDR	(FX_WDR_MO_T_P6_ADDR + 0 * 768)     /* 0x300 = 768 */
#define FX_WDR_MO_T_B_P6_ADDR	(FX_WDR_MO_T_P6_ADDR + 1 * 768)
#define FX_WDR_MO_T_C_P6_ADDR	(FX_WDR_MO_T_P6_ADDR + 2 * 768)
#define FX_WDR_MO_DIFF_P6_ADDR  	0x1A606D40
#define FX_WDR_MO_DIFF_A_P6_ADDR	(FX_WDR_MO_DIFF_P6_ADDR + 0 * 768)     /* 0x300 = 768 */
#define FX_WDR_MO_DIFF_B_P6_ADDR	(FX_WDR_MO_DIFF_P6_ADDR + 1 * 768)
#define FX_WDR_MO_DIFF_C_P6_ADDR	(FX_WDR_MO_DIFF_P6_ADDR + 2 * 768)
#define FX_WDR_DIF_P6_ADDR      0x18C07640
#define FX_WDR_DIF_A_P6_ADDR	(FX_WDR_DIF_P6_ADDR + 0 * 576)
#define FX_WDR_DIF_B_P6_ADDR	(FX_WDR_DIF_P6_ADDR + 1 * 576)
#define FX_WDR_DIF_C_P6_ADDR	(FX_WDR_DIF_P6_ADDR + 2 * 576)
#define FX_WDR_DIF_2_P6_ADDR    0x19907640
#define FX_WDR_DIF_2_A_P6_ADDR	(FX_WDR_DIF_2_P6_ADDR + 0 * 576)
#define FX_WDR_DIF_2_B_P6_ADDR	(FX_WDR_DIF_2_P6_ADDR + 1 * 576)
#define FX_WDR_DIF_2_C_P6_ADDR	(FX_WDR_DIF_2_P6_ADDR + 2 * 576)

#define FX_GAMMA_ADDR       		0x1F641000                      /* rex+ 180911, size = 2KB*2 */
#define F2_GAMMA_ADDR       		0x0F504000                      /* rex+ 180911, size = 2KB*2 */

#define F2_SATURATION_UV_ADDR			0x0F506000
#define F2_0_ISP2_6X6_BIN_ST_CMD_ADDR	0x0FD00000
#define F2_1_ISP2_6X6_BIN_ST_CMD_ADDR	0x0FD20000
#define FX_ISP2_6X6_BIN_ST_CMD_ADDR		0x1D880000

#define ISP2_6X6_BIN_ADDR_A_P0			0x1DD00000
#define ISP2_6X6_BIN_ADDR_B_P0			0x1DD00600
#define ISP2_6X6_BIN_ADDR_C_P0			0x1DD00C00
#define ISP2_6X6_BIN_ADDR_A_P1			0x1DD01200
#define ISP2_6X6_BIN_ADDR_B_P1			0x1DD01800
#define ISP2_6X6_BIN_ADDR_C_P1			0x1DD01E00
#define ISP2_6X6_BIN_ADDR_A_P2			0x1DD02400
#define ISP2_6X6_BIN_ADDR_B_P2			0x1DD02A00
#define ISP2_6X6_BIN_ADDR_C_P2			0x1DD03000
#define ISP2_6X6_BIN_ADDR_A_P3			0x1DD03600
#define ISP2_6X6_BIN_ADDR_B_P3			0x1DD03C00
#define ISP2_6X6_BIN_ADDR_C_P3			0x1DD04200

#define COLOR_TEMP_TABLE_ADDR			0x0FD40000					/* size:0x200 * 8 */
#define COLOR_TEMP_TH_ADDR				0x0FD41000					/* size:0x200 */
#define ADT_TABLE_ADDR					0x0FD41200					/* size:0x200 */

#define F2_0_DDR_CHECKSUM_ADDR			0x0FD48000
#define F2_1_DDR_CHECKSUM_ADDR			0x0FD48100

#define ST_EMPTY_T_ADDR					0x1F105C00

//Plant ST_I Cmd Addr
//512*32, size: 0x4000*8 = 0x20000 (128KByte)
#define PLANT_ST_CMD_PAGE_OFFSET		0x4000
#define F2_0_PLANT_ST_CMD_ADDR			0x13F00000
#define F2_1_PLANT_ST_CMD_ADDR			0x13F20000
#define FX_PLANT_ST_CMD_ADDR			0x1F910000
#define PLANT_ST_O_CMD_ADDR				0x1F930000
#define PLANT_ST_O_2_CMD_ADDR			0x1F950000
//512*64, size: 0x8000*8 = 0x40000 (256KByte)
#define PLANT_MP_GAIN_PAGE_OFFSET		0x8000
#define F2_0_PLANT_MP_GAIN_ADDR			0x13E00000
#define F2_1_PLANT_MP_GAIN_ADDR			0x13E40000
#define FX_PLANT_MP_GAIN_ADDR			0x1F970000
#define F2_0_PLANT_MP_GAIN_2_ADDR		0x13E80000
#define F2_1_PLANT_MP_GAIN_2_ADDR		0x13EC0000
#define FX_PLANT_MP_GAIN_2_ADDR			0x1F9B0000
//512*4, size: 0x800*8 = 0x4000 (16KByte)
#define PLANT_TRAN_PAGE_OFFSET			0x800
#define F2_0_PLANT_ST_TRAN_CMD_ADDR		0x13F40000
#define F2_1_PLANT_ST_TRAN_CMD_ADDR		0x13F44000
#define FX_PLANT_ST_TRAN_CMD_ADDR		0x1F9F0000

#ifdef __cplusplus
}   // extern "C"
#endif

//*/
#endif    // __US360_DEFINE_H__
