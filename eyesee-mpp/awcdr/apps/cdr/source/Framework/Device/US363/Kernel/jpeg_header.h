/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __JPEG_HEADER_H__
#define __JPEG_HEADER_H__

#ifdef __KERNEL__
  #include <linux/types.h>
#else
  #include <stdint.h>
#endif

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SWAP32(x) (((x) >> 24) | (((x)&0x00ff0000) >> 8) | (((x)&0x0000ff00) << 8) | ((x) << 24))
#define SWAP24(x) (((x) >> 16) | ((x)&0x00ff00) | ((x) << 16) )
#define SWAP16(x) (((x) >> 8) | ((x&0xff) << 8))
#define CC(s) (*((uint32_t *)(s)))

#define EXIF_SIZE_TYPE_1  1
#define EXIF_SIZE_TYPE_2  1
#define EXIF_SIZE_TYPE_3  2
#define EXIF_SIZE_TYPE_4  4
#define EXIF_SIZE_TYPE_5  8
#define EXIF_SIZE_TYPE_6  1
#define EXIF_SIZE_TYPE_7  1
#define EXIF_SIZE_TYPE_8  2
#define EXIF_SIZE_TYPE_9  4
#define EXIF_SIZE_TYPE_10 8
#define EXIF_SIZE_TYPE_11 4
#define EXIF_SIZE_TYPE_12 8

#define JPEG_GPS_MUL	10000.0

// =================================================================

// ------------------- Exif -------------------------
typedef struct J_IFD_Header_Struct_h {
	uint16_t tag;
	uint16_t size_type;
	uint32_t cnt;
	uint32_t data;								//如果 size*cnt > 8byte  data=addr offset
}__attribute__((packed)) J_IFD_Header_Struct;							//12 byte

typedef struct J_Exif_Header1_Struct_h {
	uint32_t Exif_str;  						//"Exif"= 45 78 69 66
	uint16_t rev1;								//00 00
	uint16_t TIFF_type;							//"II"=0x4949 "MM"=0x4D4D, 使用 "MM"
	uint16_t TIFF_header1;						//00 2A
	uint32_t TIFF_header2;						//00 00 00 08
	uint16_t IFD_cnt;

	J_IFD_Header_Struct IFD_Make;				//01 0F   00 02   00 00 00 04	製造商
	J_IFD_Header_Struct IFD_Model;				//01 10   00 02   00 00 00 08	產品名稱
	J_IFD_Header_Struct IFD_Orientation;		//01 12   00 03   00 00 00 01	相機相對於場景的方向
	J_IFD_Header_Struct IFD_XResolution;		//01 1A   00 05	  00 00 00 01	分辨率
	J_IFD_Header_Struct IFD_YResolution;		//01 1B   00 05   00 00 00 01	分辨率
	J_IFD_Header_Struct IFD_ResolutionUnit;		//01 28   00 03   00 00 00 01	分辨率單位
	J_IFD_Header_Struct IFD_SoftwareVersion;	//01 31   00 02   00 00 00 14	版本
	J_IFD_Header_Struct IFD_DateTime;			//01 32   00 02   00 00 00 14	日期時間 "YYYY:MM:DD HH:MM:SS"+0x00
	J_IFD_Header_Struct IFD_YCbCrPositioning;	//02 13   00 03   00 00 00 01	表示圖像使用YCbCr, 並且使用子採樣
	J_IFD_Header_Struct IFD_ExifOffset;			//87 69   00 04   00 00 00 01	Exif 子IFD的偏移量
	J_IFD_Header_Struct IFD_GPS;				//88 25   00 04   00 00 00 01	GPS

}__attribute__((packed)) J_Exif_Header1_Struct;							//16 byte + 11 * 12 byte = 148 byte
extern J_Exif_Header1_Struct J_Exif_Header1;

typedef struct J_Exif_Header2_Struct_h {
	uint16_t IFD_cnt;

	J_IFD_Header_Struct IFD_ExposureTime;				//82 9A   00 05   00 00 00 01	曝光時間
	J_IFD_Header_Struct IFD_FNumber;					//82 9D   00 05   00 00 00 01	光圈
	J_IFD_Header_Struct IFD_ExposureProgram;			//88 22   00 03   00 00 00 01	曝光模式
	J_IFD_Header_Struct IFD_ISOSpeedRatings;			//88 27   00 03   00 00 00 01	感光度
	J_IFD_Header_Struct IFD_ExifVersion;				//90 00   00 07   00 00 00 04 	Exif版本
	J_IFD_Header_Struct IFD_DateTimeOriginal;			//90 03   00 02   00 00 00 14	日期時間 "YYYY:MM:DD HH:MM:SS"+0x00
	J_IFD_Header_Struct IFD_DateTimeDigitized;			//90 04   00 02   00 00 00 14	日期時間 "YYYY:MM:DD HH:MM:SS"+0x00
//	J_IFD_Header_Struct IFD_ComponentsConfiguration;	//91 01   00 07   00 00 00 04	像素數據的順序	RGB:04 05 06 00		YCbCr:01 02 03 00
//	J_IFD_Header_Struct IFD_CompressedBitsPerPixel;		//91 02   00 05   00 00 00 01	JPEG壓縮率
	J_IFD_Header_Struct IFD_ShutterSpeedValue;			//92 01   00 0A   00 00 00 01 	用APEX表示快門速度
	J_IFD_Header_Struct IFD_ApertureValue;				//92 02   00 05   00 00 00 01	用APEX表示光圈
//	J_IFD_Header_Struct IFD_BrightnessValue;			//92 03   00 0A   00 00 00 01	亮度
	J_IFD_Header_Struct IFD_ExposureBiasValue;			//92 04   00 0A   00 00 00 01	曝光補償, 單位APEX(EV)
	J_IFD_Header_Struct IFD_MaxApertureValue;			//92 05   00 05   00 00 00 01	最大光圈值
	J_IFD_Header_Struct IFD_MeteringMode;				//92 07   00 03   00 00 00 01	曝光的測光方式, 0=Unknown 1=平均測光 2=中央加權平均 3=點測光 4=多點測光 5=分區測光 6=局部測光
	J_IFD_Header_Struct IFD_Flash;						//92 09   00 03   00 00 00 01	閃光燈模式
	J_IFD_Header_Struct IFD_FocalLength;				//92 0A   00 05   00 00 00 01	焦距長度, 單位mm
	J_IFD_Header_Struct IFD_ImageNumber;				//92 11   00 04   00 00 00 01	替換為照度值
	J_IFD_Header_Struct IFD_MakerNote;					//92 7C   00 07   00 00 14 1B	製造商內部數據
	J_IFD_Header_Struct IFD_SubsecTimeOriginal;			//92 91   00 02   00 00 00 04	紀錄時間 ms
//	J_IFD_Header_Struct IFD_FlashPixVersion;			//A0 00   00 07   00 00 00 04  	儲存FlashPix 的版本信息
//	J_IFD_Header_Struct IFD_ColorSpace;					//A0 01   00 03   00 00 00 01	色彩空間
	J_IFD_Header_Struct IFD_ExifImageWidth;				//A0 02   00 04   00 00 00 01	影像寬
	J_IFD_Header_Struct IFD_ExifImageHeight;			//A0 03   00 04   00 00 00 01	影像高
//	J_IFD_Header_Struct IFD_ExifInteroperabilityOffset;	//A0 05   00 04   00 00 00 01
	J_IFD_Header_Struct IFD_CustomRendered;				//A4 01   00 03   00 00 00 01	渲染方式
//	J_IFD_Header_Struct IFD_ExposureMode;				//A4 02   00 03   00 00 00 01	曝光模式
//	J_IFD_Header_Struct IFD_WhiteBalance;				//A4 03   00 03   00 00 00 01	白平衡設置
//	J_IFD_Header_Struct IFD_DigitalZoomRatio;			//A4 04   00 05   00 00 00 01
//	J_IFD_Header_Struct IFD_FocalLengthIn35mmFilm;		//A4 05   00 03   00 00 00 01
//	J_IFD_Header_Struct IFD_SceneCaptureType;			//A4 06   00 03   00 00 00 01
//	J_IFD_Header_Struct IFD_GainControl;				//A4 07   00 03   00 00 00 01
//	J_IFD_Header_Struct IFD_Contrast;					//A4 08   00 03   00 00 00 01
//	J_IFD_Header_Struct IFD_Saturation;					//A4 09   00 03   00 00 00 01
//	J_IFD_Header_Struct IFD_Sharpness;					//A4 0A   00 03   00 00 00 01

}__attribute__((packed)) J_Exif_Header2_Struct;									// 2 byte + 16 * 12 byte = 194 byte
extern J_Exif_Header2_Struct J_Exif_Header2;

typedef struct J_Exif_GPS_Header_Struct_h {
	uint16_t IFD_cnt;

	J_IFD_Header_Struct IFD_GPSLatitudeRef;				//00 01   00 02   00 00 00 02	 緯度 	"N" = 4E 00 00 00
	J_IFD_Header_Struct IFD_GPSLatitude;				//00 02   00 05   00 00 00 03	 緯度 	前4byte=分子 後4byte=分母
	J_IFD_Header_Struct IFD_GPSLongitudeRef;			//00 03   00 02   00 00 00 02	 經度 	"E" = 45 00 00 00
	J_IFD_Header_Struct IFD_GPSLongitude;				//00 04   00 05   00 00 00 03	 經度  	前4byte=分子 後4byte=分母
	J_IFD_Header_Struct IFD_GPSAltitudeRef;				//00 05   00 01   00 00 00 01	 海拔
	J_IFD_Header_Struct IFD_GPSAltitude;				//00 06   00 05   00 00 00 01	 海拔  	前4byte=分子 後4byte=分母

//	J_IFD_Header_Struct IFD_GPSTimeStamp;				//00 07   00 05   00 00 00 03	 Time	???
//	J_IFD_Header_Struct IFD_GPSProcessingMethod;		//00 1B   00 02   00 00 00 10	 Time	???
//	J_IFD_Header_Struct IFD_GPSDateStamp;				//00 1D   00 02   00 00 00 0B	 Date
}__attribute__((packed)) J_Exif_GPS_Header_Struct;			// 2 byte + 6 * 12 byte = 74 byte
extern J_Exif_GPS_Header_Struct J_Exif_GPS_Header;


typedef struct J_APP1_Data_Struct_h{
	unsigned long long Time;					//us
	unsigned C_Mode;
	unsigned HDR_nP;
	int 	 IncEV;
	unsigned AEB_nP;
	unsigned HDR_Manual;
	unsigned HDR_Auto_EV[2];
	unsigned HDR_Strength;
	int		 WB_Mode;
	unsigned Color;								//色溫
	float    Tint;								//色溫G值 +-%
	unsigned Battery;							//電量
	unsigned deGhost_en;
	unsigned smooth_en;							//反鋸齒
    int 	 Smooth_Auto_Rate;					//即時縫合
    int 	 Sharpness;							//銳利度
    int 	 Tone;								//WDR
    int 	 Contrast;							//對比
    int 	 Saturation;	 					//彩度
}J_APP1_Data_Struct;
extern J_APP1_Data_Struct app1_data;

#define EXIF_H_BUF1_MAX 88
#define EXIF_H_BUF2_MAX 96
#define EXIF_H_GPS_BUF_MAX 64
typedef struct J_APP2_Data_Struct_h{
	J_Exif_Header1_Struct	 exif_h1;
	char exif_buf1[EXIF_H_BUF1_MAX];
	J_Exif_Header2_Struct	 exif_h2;
	char exif_buf2[EXIF_H_BUF2_MAX];
	J_Exif_GPS_Header_Struct exif_gps_h;
	char exif_gps_buf[EXIF_H_GPS_BUF_MAX];
}__attribute__((packed)) J_APP2_Data_Struct;
extern J_APP2_Data_Struct app2_data;

typedef struct J_SOI_Struct_h{
	char Label_H;
	char Label_L;
}J_SOI_Struct;

typedef struct J_APP0_Struct_h{
	char Label_H;
	char Label_L;
	char Length_H;
	char Length_L;
	char  Data[14];
}J_APP0_Struct;

typedef struct J_APP1_Struct_h{
	char Label_H;
	char Label_L;
	char Length_H;
	char Length_L;
	char Data[384];
}J_APP1_Struct;

typedef struct J_APP2_Struct_h{
	char Label_H;
	char Label_L;
	char Length_H;
	char Length_L;
	char Data[768];	//2624
}J_APP2_Struct;

typedef struct J_DQT_Struct_h{
	char Label_H;
	char Label_L;
	char Length_H;
	char Length_L;
	char QT_ID;
	char Data[64];
}J_DQT_Struct;

typedef struct J_SOF0_Struct_h{
	char Label_H;
	char Label_L;
	char Length_H;
	char Length_L;
	char Precision;
	char Image_H_H;
	char Image_H_L;
	char Image_W_H;
	char Image_W_L;
	char N_Components;
	char Component_ID0;
	char VH_Factor0;
	char QT_ID0;
	char Component_ID1;
	char VH_Factor1;
	char QT_ID1;
	char Component_ID2;
	char VH_Factor2;
	char QT_ID2;
}J_SOF0_Struct;

typedef struct J_DHT_DC_Struct_h{
	char Label_H;
	char Label_L;
	char Length_H;
	char Length_L;
	char HT_ID;
	char Data[28];
}J_DHT_DC_Struct;

typedef struct J_DHT_AC_Struct_h{
	char Label_H;
	char Label_L;
	char Length_H;
	char Length_L;
	char HT_ID;
	char Data[178];
}J_DHT_AC_Struct;

typedef struct J_SOS_Struct_h{
	char Label_H;
	char Label_L;
	char Length_H;
	char Length_L;
	char N_Components;
	char Component_ID0;
	char HT_ID0;
	char Component_ID1;
	char HT_ID1;
	char Component_ID2;
	char HT_ID2;
	char Data[3];
}J_SOS_Struct;

typedef struct J_Hder_Struct_h{
	J_SOI_Struct SOI;
	J_APP0_Struct APP0;
	J_APP2_Struct APP2;
	J_APP1_Struct APP1;
	J_DQT_Struct DQT_Y;
	J_DQT_Struct DQT_C;
	J_SOF0_Struct SOF0;
	J_DHT_DC_Struct DHT_Y_DC;
	J_DHT_AC_Struct DHT_Y_AC;
	J_DHT_DC_Struct DHT_C_DC;
	J_DHT_AC_Struct DHT_C_AC;
	J_SOS_Struct SOS;
}J_Hder_Struct;
extern J_Hder_Struct jpeg_header;

typedef struct JPEG_HDR_AEB_Info_Struct_h {
    int hdr_manual;
    int hdr_np;
    float hdr_ev;				//與下一張的倍率
    float hdr_ev_mid;			//與中間的倍率
    int hdr_auto_ev[2];
    int strength;

    int aeb_np;
    int aeb_ev;
}JPEG_HDR_AEB_Info_Struct;

typedef struct JPEG_UI_Info_Struct_h {
	int wb_mode;
    int color;
    int tint;
    int smooth;
    int sharpness;
    int tone;
    int contrast;
    int saturation;
}JPEG_UI_Info_Struct;

struct Jpeg_sensor_data {
	int PoseHeadingDegrees;
	float PosePitchDegrees;
	float PoseRollDegrees;
};

// =================================================================

//extern char header_p[0x280];
extern int iQY[64];
extern int iQC[64];
extern unsigned QY[64];
extern unsigned QC[64];
extern char tzQY[4][64], tzQC[4][64];
extern unsigned char header_buf[2048];
extern char HT_Y_DC[28];
extern char HT_Y_AC[178];
extern char HT_C_DC[28];
extern char HT_C_AC[178];

// =================================================================
void Quality_Set(int Quality);
void Set_JPEG_Header(int doQuality, int Quality, unsigned short Y, unsigned short X, unsigned addr, int hdr_3s, unsigned long long time, unsigned c_mode,
		int exp_n, int exp_m, int iso, int deGhost_en, int smooth_en, JPEG_HDR_AEB_Info_Struct info, JPEG_UI_Info_Struct ui_info);
void SPI_W(int Q_Sel);
void set_app1_data(unsigned long long time, unsigned c_mode, unsigned battery, unsigned deGhost_en, unsigned smooth_en, JPEG_HDR_AEB_Info_Struct info, JPEG_UI_Info_Struct ui_info);
void Make_JPEG_Header(J_Hder_Struct *P1, short IH, short IW, short Quality);
void Make_JPEG_Exif_Header(int width, int height, int hdr_3s, int frame, int integ, int dgain);
void Set_Jpeg_Sensor_Head(int idx, int value);
void Set_Jpeg_Sensor_Pitch(int idx, int value);
void Set_Jpeg_Sensor_Roll(int idx, int value);
int Add_Panorama_Header(int width, int height, char *img, FILE *fp);
void del_jpeg_error_code(unsigned char *img, int size, int *error_cnt, int *len);
int Write_JPEG_Real_Size(char *img, int r_size);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__JPEG_HEADER_H__