/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Test/focus.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>              /* low-level i/o */
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <math.h>

#include "Device/US363/Cmd/us360_func.h"
#include "Device/US363/Kernel/k_spi_cmd.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::Focus"

struct Focus_Posi_Struct Adj_Foucs_Center_Default[5][4] = {
	{ { 903, 2396}, {3454, 2375}, { 914,  975}, {3441,  947} },
	{ {2662,  324}, {2649, 2863}, { 510,  705}, { 443, 2509} },
	{ {1140, 2870}, {1072,  447}, {3357, 2568}, {3311,  793} },
	{ {2742,  363}, {2765, 2951}, { 509,  725}, { 501, 2565} },
	{ {1061, 2922}, {1044,  397}, {3315, 2525}, {3273,  734} }
};
struct Focus_Posi_Struct Adj_Foucs_Center[5][4];

struct Focus_Posi_Struct Adj_Foucs_Posi_Default[4][5][4] = {				//Foucs_XY[tool_id][sensor][idx]
	  { { { 460, 2145}, {3015, 2122}, { 472,  722}, {3005,  690} },
		{ {2435, -155}, {2435, 2385}, { 280,  230}, { 225, 2033} },
		{ { 915, 2385}, { 842,  -40}, {3103, 2085}, {3092,  302} },
		{ {2533, -103}, {2545, 2480}, { 283,  250}, { 273, 2080} },
		{ { 830, 2440}, { 820,  -80}, {3095, 2050}, {3053,  258} } },

	  { { { 459, 2158}, {3041, 2200}, { 462,  704}, {3018,  688} },
		{ {2409, -156}, {2421, 2418}, { 251,  236}, { 203, 2027} },
		{ { 936, 2480}, { 939,  -87}, {3140, 2095}, {3122,  312} },
		{ {2521, -165}, {2511, 2435}, { 238,  295}, { 224, 1990} },
		{ { 835, 2467}, { 865, -109}, {3133, 1962}, {3090,  300} } },

	  { { { 460, 2145}, {3015, 2122}, { 472,  722}, {3005,  690} },
		{ {2435, -155}, {2435, 2385}, { 280,  230}, { 225, 2033} },
		{ { 915, 2385}, { 842,  -40}, {3103, 2085}, {3092,  302} },
		{ {2533, -103}, {2545, 2480}, { 283,  250}, { 273, 2080} },
		{ { 830, 2440}, { 820,  -80}, {3095, 2050}, {3053,  258} } },

	  { { { 460, 2145}, {3015, 2122}, { 472,  722}, {3005,  690} },
		{ {2435, -155}, {2435, 2385}, { 280,  230}, { 225, 2033} },
		{ { 915, 2385}, { 842,  -40}, {3103, 2085}, {3092,  302} },
		{ {2533, -103}, {2545, 2480}, { 283,  250}, { 273, 2080} },
		{ { 830, 2440}, { 820,  -80}, {3095, 2050}, {3053,  258} } }
};
struct Focus_Posi_Struct Adj_Foucs_Posi[5][4];

float Search_Foucs_Temp[5][4][2];

int Adj_Focus_Error[5][4];

int Focus_Scan_Table[FOCUS_SCAN_SIZE][FOCUS_SCAN_SIZE] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
	0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0,
	0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
	0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
	0, 0, 1, 0, 0, 0, 2, 2, 2, 0, 0, 0, 1, 0, 0,
	0, 0, 1, 0, 0, 0, 2, 2, 2, 0, 0, 0, 1, 0, 0,
	0, 0, 1, 0, 0, 0, 2, 2, 2, 0, 0, 0, 1, 0, 0,
	0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
	0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0,
	0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0,
	0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
int Focus_Scan_Table_Tran[FOCUS_SCAN_SIZE][FOCUS_SCAN_SIZE];

int Focus_Tool_Num = 1;
void SetFocusToolNum(int num)
{
	Focus_Tool_Num = num;
	if(Focus_Tool_Num < 1) 		Focus_Tool_Num = 1;
	else if(Focus_Tool_Num > 4) Focus_Tool_Num = 4;

#ifdef ANDROID_CODE
	set_A2K_Debug_Focus_Tool(Focus_Tool_Num);
#endif
}

int GetFocusToolNum(void)
{
	return Focus_Tool_Num;
}

//int Focus_Error[5][4];

void FocusScanTableTranInit() {
	int m, n;
	int cnt1, cnt2;
	int Scan_Size = FOCUS_SCAN_SIZE;

 	cnt1 = 0; cnt2 = 0;
	for(m = 0; m < Scan_Size; m++) {
		for(n = 0; n < Scan_Size; n++) {
			if(Focus_Scan_Table[m][n] == 1)
				cnt1++;
			else if(Focus_Scan_Table[m][n] == 2)
				cnt2++;
		}
	}

	for(m = 0; m < Scan_Size; m++) {
		for(n = 0; n < Scan_Size; n++) {
			switch(Focus_Scan_Table[m][n]) {
			case 0: Focus_Scan_Table_Tran[m][n] = 0; break;
			case 1: Focus_Scan_Table_Tran[m][n] = -256; break;
			case 2: Focus_Scan_Table_Tran[m][n] = 256 * cnt1 / cnt2; break;
			}
		}
	}
}

unsigned long long focus_init_t;
int check_focus_time(unsigned long long time) {
	if( (time - focus_init_t) > 3000000)
		return 1;
	else
		return 0;
}

void FocusResultInit() {
	int i, j;

	for(i = 0; i < 5; i++) {
		Focus_Result[i].Tool_Num = 0;
		sprintf(&Focus_Result[i].Sample_Num[0], "0000\0");

#ifdef ANDROID_CODE
		Focus_Result[i].Pass = 0;
		Focus_Result[i].Focus_Min = 99999999;
		Focus_Result[i].Focus_Min_Idx = 0;
		Get_Focus_Degree(i, &Focus_Result[i].Degree);
#endif
        for(j = 0; j < 4; j++) {
#ifdef ANDROID_CODE
        	Get_Focus_XY(Focus_Tool_Num, i, j, &Focus_Result[i].FocusData[j].S_Posi.X, &Focus_Result[i].FocusData[j].S_Posi.Y);
#endif
			Focus_Result[i].FocusData[j].Focus_Rate = 1.00;
        	Focus_Result[i].FocusData[j].W_Posi.X = (j&0x1) * FOCUS_WIN_X;
        	Focus_Result[i].FocusData[j].W_Posi.Y = (j>>1)  * FOCUS_WIN_Y;

        	Focus_Result[i].FocusData[j].Posi_Offset.X = 0;
        	Focus_Result[i].FocusData[j].Posi_Offset.Y = 0;

        	if(i == 0)
        		Focus_Result[i].FocusData[j].Focus_Th = 900;
        	else {
        		if(j == 0 || j == 1)
        			Focus_Result[i].FocusData[j].Focus_Th = 900;
        		else
        			Focus_Result[i].FocusData[j].Focus_Th = 650;
        	}
        	Focus_Result[i].FocusData[j].Focus_Max = 0;
        }
	}
#ifdef ANDROID_CODE
//tmp	get_current_usec(&focus_init_t);
#endif
}

//Focus_Block_Struct Focus_Block_Data;
void Get_Focus_Block_Y(char *img, struct Focus_Parameter_Struct *fs)
{
	int i, j;
	int idx, idx2;
	char *img2;
	char P1;
	int px, py;
	int SizeX, SizeY;

	img2 = img;
	SizeX = FOCUS_SEARCH_SIZE_1;
	SizeY = FOCUS_SEARCH_SIZE_1;
	for(i = 0; i < SizeY; i++) {
		for (j = 0; j < SizeX; j++) {
			px = j - (SizeX>>1) + fs->W_Posi.X + (FOCUS_WIN_X>>1) + fs->Posi_Offset.X;
			py = i - (SizeY>>1) + fs->W_Posi.Y + (FOCUS_WIN_Y>>1) + fs->Posi_Offset.Y;
			P1 = *(img2 + py*FOCUS_IMG_X + px);
			fs->Pixel_I[i][j] = 255 - (P1 & 0xff);
		}
	}

	img2 = img;
	SizeX = FOCUS_WIN_X;
	SizeY = FOCUS_WIN_Y;
	for(i = 0; i < SizeY; i++) {
		for (j = 0; j < SizeX; j++) {
			px = fs->W_Posi.X + j;
			py = fs->W_Posi.Y + i;
			P1 = *(img2 + py*FOCUS_IMG_X + px);
			fs->Pixel_W[i][j] = 255 - (P1 & 0xff);
		}
	}
}

void focus_256_to_64(short *I_Buf, short *O_Buf)
{
	int i, j;
	int i2, j2;
	int Sum[64][64], Cnt[64][64];
	int size1 = FOCUS_SEARCH_SIZE_1;
	int size2 = FOCUS_SEARCH_SIZE_2;
	int rate = size1 / size2;

	memset(&Sum[0][0], 0, sizeof(Sum) );
	memset(&Cnt[0][0], 0, sizeof(Cnt) );

	for(i = 0; i < size1; i++) {
		for(j = 0; j < size1; j++) {
			i2 = i/rate;
			j2 = j/rate;
			Sum[i2][j2] += *(I_Buf + i*size1 + j);
			Cnt[i2][j2]++;
        }
	}

	for(i = 0; i < size2; i++) {
		for(j = 0; j < size2; j++) {
			*(O_Buf + i*size2 + j) = Sum[i][j] / Cnt[i][j];
        }
    }
}

//int Focus_Th = 500;
int White_Th = 20;
int Black_Th = 70;
int focus_cal_center_scan(int s_id, int idx, int *cx, int *cy)
{
	int i, j, m, n;
	int Temp_P;
	int Scan_Size = FOCUS_SCAN_SIZE, Scan_Half;
	int Search_Size = FOCUS_SEARCH_SIZE_2;
	int Pixel_tmp[FOCUS_SEARCH_SIZE_2][FOCUS_SEARCH_SIZE_2];
	int pixel_max, get_max_flag;
	int px, py;
	int c_total, c_cnt, c_avg;
	int th = White_Th, c_th = Black_Th;
	int max, min;
	int over_size_flag;

	memset(&Pixel_tmp[0][0], 0, sizeof(Pixel_tmp) );

	Scan_Half = (Scan_Size >> 1);
	for(i = Scan_Half; i < (Search_Size-Scan_Half); i++) {
		for (j = Scan_Half; j < (Search_Size-Scan_Half); j++) {

			//取得外圍最高亮度&最低亮度, 取得中心範圍平均亮度
			c_avg = 0; c_total = 0; c_cnt = 0;
			max = 0; min = 99999999;
			for(m = 0; m < Scan_Size; m++) {
				for(n = 0; n < Scan_Size; n++) {
					px = j-Scan_Half+n;
					py = i-Scan_Half+m;
					if(Focus_Scan_Table[m][n] == 1) {
						if(Focus_Result[s_id].FocusData[idx].Pixel_O[py][px] > max)
							max = Focus_Result[s_id].FocusData[idx].Pixel_O[py][px];
						if(Focus_Result[s_id].FocusData[idx].Pixel_O[py][px] < min)
							min = Focus_Result[s_id].FocusData[idx].Pixel_O[py][px];
					}
					else if(Focus_Scan_Table[m][n] == 2) {
						c_total += Focus_Result[s_id].FocusData[idx].Pixel_O[py][px];
						c_cnt++;
					}
				}
			}
			if(c_cnt == 0) c_avg = 0;
			else		   c_avg = c_total / c_cnt;


			Temp_P = 0; over_size_flag = 0;
			if( (max - min) < th && c_avg > c_th) {			//外圍亮度差必須小於門檻值, 中心平均亮度必須大於門檻值
				for(m = 0; m < Scan_Size; m++) {
					for(n = 0; n < Scan_Size; n++) {
						px = j-Scan_Half+n;
						py = i-Scan_Half+m;
						Temp_P += Focus_Result[s_id].FocusData[idx].Pixel_O[py][px] * Focus_Scan_Table_Tran[m][n];
					}
				}
				Pixel_tmp[i][j] = Temp_P;
			}
			else
				Pixel_tmp[i][j] = 0;

		}
	}

	//取最大值
	pixel_max = 0;
	get_max_flag = -1;
	for(i = 0; i < Search_Size; i++) {
		for (j = 0; j < Search_Size; j++) {
			if(Pixel_tmp[i][j] > pixel_max) {
				pixel_max = Pixel_tmp[i][j];
				*cx = (j << 2) + 2;
				*cy = (i << 2) + 2;
				get_max_flag = 1;
			}
		}
	}

	return get_max_flag;
}

int focus_cal_center(int s_id, int idx)
{
	int ret = 0;
	int i, j, m, n;
	int Sum_C_X; int Sum_C_Y;
	int Sum_All, Sum_Dot, Avg;
	int Temp_P;
	int scan_ret = -1;
	int Search_Size = FOCUS_SEARCH_SIZE_1, Search_Size_Half;
	int Center_X = -1, Center_Y = -1;
	int i2,j2;
	int Lock_Size = FOCUS_LOCK_SIZE, Lock_Size_Half;

	Search_Size_Half = (Search_Size >> 1);
	Lock_Size_Half = (Lock_Size >> 1);

	scan_ret = focus_cal_center_scan(s_id, idx, &Center_X, &Center_Y);

	Sum_All = 0;
	for(i = -Lock_Size_Half; i < Lock_Size_Half; i++) {
		for (j = -Lock_Size_Half; j < Lock_Size_Half; j++) {
		  i2 = Center_Y + i;
		  j2 = Center_X + j;
		  Sum_All += Focus_Result[s_id].FocusData[idx].Pixel_I[i2][j2];
		}
	}
	Avg = Sum_All / (Lock_Size * Lock_Size);

	if (scan_ret == 1) {
		Sum_Dot = 0; Sum_C_X = 0; Sum_C_Y = 0;  //P3->Error_Cnt = 0;
		for(i = -Lock_Size_Half; i < Lock_Size_Half; i++) {
			for (j = -Lock_Size_Half; j < Lock_Size_Half; j++) {
				i2 = Center_Y + i;
				j2 = Center_X + j;
				Temp_P = Focus_Result[s_id].FocusData[idx].Pixel_I[i2][j2] - Avg - 20;
				if (Temp_P > 0) {
					Sum_Dot += Temp_P;
					Sum_C_X += Temp_P * j2;
					Sum_C_Y += Temp_P * i2;
				}
			}
		}

		if (Sum_Dot != 0) {
			Focus_Result[s_id].FocusData[idx].W_Center_Offset.X = (float) Sum_C_X / Sum_Dot - Search_Size_Half;
			Focus_Result[s_id].FocusData[idx].W_Center_Offset.Y = (float) Sum_C_Y / Sum_Dot - Search_Size_Half;
			//因Sensor影像擺正, Sensor中心點座標 XY需做對應的調整
			if(Focus_Result[s_id].Degree == 0) {
				Focus_Result[s_id].FocusData[idx].S_Center_Offset.X =  Focus_Result[s_id].FocusData[idx].W_Center_Offset.X;
				Focus_Result[s_id].FocusData[idx].S_Center_Offset.Y =  Focus_Result[s_id].FocusData[idx].W_Center_Offset.Y;

				Focus_Result[s_id].FocusData[idx].Center.X = Focus_Result[s_id].FocusData[idx].S_Posi.X + Focus_Result[s_id].FocusData[idx].Posi_Offset.X +		\
						(FOCUS_WIN_X>>1) + Focus_Result[s_id].FocusData[idx].S_Center_Offset.X;
				Focus_Result[s_id].FocusData[idx].Center.Y = Focus_Result[s_id].FocusData[idx].S_Posi.Y + Focus_Result[s_id].FocusData[idx].Posi_Offset.Y +		\
						(FOCUS_WIN_Y>>1) + Focus_Result[s_id].FocusData[idx].S_Center_Offset.Y;
			}
			else if(Focus_Result[s_id].Degree == -90) {
				Focus_Result[s_id].FocusData[idx].S_Center_Offset.X = -Focus_Result[s_id].FocusData[idx].W_Center_Offset.Y;
				Focus_Result[s_id].FocusData[idx].S_Center_Offset.Y =  Focus_Result[s_id].FocusData[idx].W_Center_Offset.X;

				Focus_Result[s_id].FocusData[idx].Center.X = Focus_Result[s_id].FocusData[idx].S_Posi.X + Focus_Result[s_id].FocusData[idx].Posi_Offset.X +		\
						(FOCUS_WIN_Y>>1) + Focus_Result[s_id].FocusData[idx].S_Center_Offset.X;
				Focus_Result[s_id].FocusData[idx].Center.Y = Focus_Result[s_id].FocusData[idx].S_Posi.Y + Focus_Result[s_id].FocusData[idx].Posi_Offset.Y +		\
						(FOCUS_WIN_X>>1) + Focus_Result[s_id].FocusData[idx].S_Center_Offset.Y;
			}
			else if(Focus_Result[s_id].Degree == 90) {
				Focus_Result[s_id].FocusData[idx].S_Center_Offset.X =  Focus_Result[s_id].FocusData[idx].W_Center_Offset.Y;
				Focus_Result[s_id].FocusData[idx].S_Center_Offset.Y = -Focus_Result[s_id].FocusData[idx].W_Center_Offset.X;

				Focus_Result[s_id].FocusData[idx].Center.X = Focus_Result[s_id].FocusData[idx].S_Posi.X + Focus_Result[s_id].FocusData[idx].Posi_Offset.X +		\
						(FOCUS_WIN_Y>>1) + Focus_Result[s_id].FocusData[idx].S_Center_Offset.X;
				Focus_Result[s_id].FocusData[idx].Center.Y = Focus_Result[s_id].FocusData[idx].S_Posi.Y + Focus_Result[s_id].FocusData[idx].Posi_Offset.Y +		\
						(FOCUS_WIN_X>>1) + Focus_Result[s_id].FocusData[idx].S_Center_Offset.Y;
			}

			if( (FOCUS_SENSOR_WIDTH - Focus_Result[s_id].FocusData[idx].Center.X) > Focus_Result[s_id].FocusData[idx].Center.X)
				Focus_Result[s_id].FocusData[idx].D_XY[0] = Focus_Result[s_id].FocusData[idx].Center.X;
			else
				Focus_Result[s_id].FocusData[idx].D_XY[0] = (FOCUS_SENSOR_WIDTH - Focus_Result[s_id].FocusData[idx].Center.X);

			if( (FOCUS_SENSOR_HEIGHT - Focus_Result[s_id].FocusData[idx].Center.Y) > Focus_Result[s_id].FocusData[idx].Center.Y)
				Focus_Result[s_id].FocusData[idx].D_XY[1] = Focus_Result[s_id].FocusData[idx].Center.Y;
			else
				Focus_Result[s_id].FocusData[idx].D_XY[1] = (FOCUS_SENSOR_HEIGHT - Focus_Result[s_id].FocusData[idx].Center.Y);

			Focus_Result[s_id].FocusData[idx].Degree = atan2( abs(FOCUS_SENSOR_C_Y-Focus_Result[s_id].FocusData[idx].Center.Y),
					abs(FOCUS_SENSOR_C_X-Focus_Result[s_id].FocusData[idx].Center.X) ) * 180.0 / pi;
		}
		else {
			ret = 10000;
			Focus_Result[s_id].FocusData[idx].W_Center_Offset.X = -1;
			Focus_Result[s_id].FocusData[idx].W_Center_Offset.Y = -1;
			Focus_Result[s_id].FocusData[idx].S_Center_Offset.X = -1;
			Focus_Result[s_id].FocusData[idx].S_Center_Offset.Y = -1;
			Focus_Result[s_id].FocusData[idx].Center.X = -1;
			Focus_Result[s_id].FocusData[idx].Center.Y = -1;
			Focus_Result[s_id].FocusData[idx].D_XY[0] = -1;
			Focus_Result[s_id].FocusData[idx].D_XY[1] = -1;
			Focus_Result[s_id].FocusData[idx].Degree = -1;
		}
	}
	else {
		ret = 10001;
		Focus_Result[s_id].FocusData[idx].W_Center_Offset.X = -1;
		Focus_Result[s_id].FocusData[idx].W_Center_Offset.Y = -1;
		Focus_Result[s_id].FocusData[idx].S_Center_Offset.X = -1;
		Focus_Result[s_id].FocusData[idx].S_Center_Offset.Y = -1;
		Focus_Result[s_id].FocusData[idx].Center.X = -1;
		Focus_Result[s_id].FocusData[idx].Center.Y = -1;
		Focus_Result[s_id].FocusData[idx].D_XY[0] = -1;
		Focus_Result[s_id].FocusData[idx].D_XY[1] = -1;
		Focus_Result[s_id].FocusData[idx].Degree = -1;
	}

	return ret;
}

void Search_Focus_Dot(int s_id, char *img)
{
	int j, cnt=4;
	for(j = 0; j < cnt; j++) {
		//Search Dot
		Focus_Result[s_id].FocusData[j].Error = 0;
		Get_Focus_Block_Y(img, &Focus_Result[s_id].FocusData[j]);
		focus_256_to_64(&Focus_Result[s_id].FocusData[j].Pixel_I[0][0], &Focus_Result[s_id].FocusData[j].Pixel_O[0][0]);
		Focus_Result[s_id].FocusData[j].Error = focus_cal_center(s_id, j);
	}
}

void doFocus(struct Focus_Parameter_Struct *fs, float th_rate)
{
	int WinX_Half = (FOCUS_WIN_X >> 1), WinY_Half = (FOCUS_WIN_Y >> 1);
	int CenterX, CenterY;
	int sx, sy;
	int sx2, sy2;
	int SizeX=256, SizeY=256;

	CenterX = fs->Posi_Offset.X + fs->W_Center_Offset.X + WinX_Half;
	CenterY = fs->Posi_Offset.Y + fs->W_Center_Offset.Y + WinY_Half;

	sx  = CenterX - (SizeX >> 1);
    if(sx < 0) sx = 0;

    sx2 = CenterX + (SizeX >> 1);
    if(sx2 > FOCUS_WIN_X) sx2 = FOCUS_WIN_X;

    sy  = CenterY - (SizeY >> 1);
    if(sy < 0) sy = 0;

    sy2 = CenterY + (SizeY >> 1);
    if(sy2 > FOCUS_WIN_Y) sy2 = FOCUS_WIN_Y;

	Focus_Block(fs, sx, sy, sx2, sy2);

	if(fs->Error == 0 && (fs->Focus*th_rate) > fs->Focus_Th)
		fs->IsPass = 1;
	else
		fs->IsPass = 0;
}

void Focus_Block(struct Focus_Parameter_Struct *fs, int x1, int y1, int x2, int y2)
{
	unsigned i,j;
	short *P, *P2;
    unsigned TD, Temp;
	unsigned Sum = 0;
    unsigned Block_Size = 256;
    unsigned SizeX = Block_Size;
    unsigned SizeY = Block_Size;

    SizeX = x2 - x1;
    SizeY = y2 - y1;

	//X軸
	P2 = &fs->Pixel_W[y1][x1];
	for (i = 0; i < SizeY; i++) {
		P = P2;
		TD = *P; P += 1;
		for (j = 1; j < SizeX; j++) {
			Temp = abs(TD - *P);
			TD =  *P;
			Sum += (Temp * Temp);
			P += 1;
		}
		P2 += FOCUS_WIN_X;
	}

	//Y軸
	P2 = &fs->Pixel_W[y1][x1];
    for (j = 0; j < SizeX; j++) {
		P = P2;
		TD = *P; P += FOCUS_WIN_X;
		for (i = 1; i < SizeY; i++) {
			Temp = abs(TD - *P);
			TD =  *P;
			Sum += (Temp * Temp);
			P += FOCUS_WIN_X;
		}
		P2 += 1;
	}

    fs->Focus = (Sum >> 14);
}

void getFocusvalue(int s_id, int *value)
{
	int i, cnt=4;
	//if(s_id == 0) cnt = 4;
	//else          cnt = 3;
	for(i = 0; i < cnt; i++) {
		*(value+i) = (int)(Focus_Result[s_id].FocusData[i].Focus * Focus_Result[s_id].FocusData[i].Focus_Rate);
	}
}

void getFocusCenterOffsetXY(int s_id, int *value)
{
	int i, cnt=8;
	//if(s_id == 0) cnt = 8;
	//else          cnt = 6;
	for(i = 0; i < cnt; i+=2) {
		*(value+i)   = (int)Focus_Result[s_id].FocusData[i>>1].W_Center_Offset.X;
		*(value+i+1) = (int)Focus_Result[s_id].FocusData[i>>1].W_Center_Offset.Y;
	}
}

void getFocusCenterXY(int s_id, int *value)
{
	int i, cnt=8;
	//if(s_id == 0) cnt = 8;
	//else          cnt = 6;
	for(i = 0; i < cnt; i+=2) {
		*(value+i)   = (int)Focus_Result[s_id].FocusData[i>>1].Center.X;
		*(value+i+1) = (int)Focus_Result[s_id].FocusData[i>>1].Center.Y;
	}
}

void getFocusDXY(int s_id, int *value)
{
	int i, cnt=8;
	for(i = 0; i < cnt; i+=2) {
		*(value+i)   = (int)Focus_Result[s_id].FocusData[i>>1].D_XY[0];
		*(value+i+1) = (int)Focus_Result[s_id].FocusData[i>>1].D_XY[1];
	}
}

void getFocusDegree(int s_id, float *value)
{
	int i, cnt=4;
	for(i = 0; i < cnt; i++)
		*(value+i)   = Focus_Result[s_id].FocusData[i].Degree;
}

void getFocusSensorPosiXY(int s_id, int *value)
{
	int i, cnt=8;
	//if(s_id == 0) cnt = 8;
	//else          cnt = 6;
	for(i = 0; i < cnt; i+=2) {
		*(value+i)   = (int)Focus_Result[s_id].FocusData[i>>1].S_Posi.X;
		*(value+i+1) = (int)Focus_Result[s_id].FocusData[i>>1].S_Posi.Y;
	}
}

void getFocusPosiXY(int s_id, int *value)
{
	int i, cnt=8;
	//if(s_id == 0) cnt = 8;
	//else          cnt = 6;
	for(i = 0; i < cnt; i+=2) {
		*(value+i)   = (int)Focus_Result[s_id].FocusData[i>>1].W_Posi.X;
		*(value+i+1) = (int)Focus_Result[s_id].FocusData[i>>1].W_Posi.Y;
	}
}

void setFocusPosiOffsetXY(int s_id, int idx, int xy, int value)
{
	if(xy == 0) Focus_Result[s_id].FocusData[idx].Posi_Offset.X = value;
	else		Focus_Result[s_id].FocusData[idx].Posi_Offset.Y = value;
}

void getFocusPosiOffsetXY(int s_id, int *value)
{
	int i, cnt=8;
	//if(s_id == 0) cnt = 8;
	//else          cnt = 6;
	for(i = 0; i < cnt; i+=2) {
		*(value+i)   = (int)Focus_Result[s_id].FocusData[i>>1].Posi_Offset.X;
		*(value+i+1) = (int)Focus_Result[s_id].FocusData[i>>1].Posi_Offset.Y;
	}
}

int getFocusError(int s_id, int idx)
{
	return Focus_Result[s_id].FocusData[idx].Error;
}

int getFocusIsPass(int s_id, int idx)
{
	return Focus_Result[s_id].FocusData[idx].IsPass;
}

void getFocusMinTh(int s_id, int *value)
{
	*value     = (int)Focus_Result[s_id].Focus_Min;
	*(value+1) = (int)Focus_Result[s_id].FocusData[0].Focus_Th;
	*(value+2) = (int)Focus_Result[s_id].FocusData[2].Focus_Th;
	*(value+3) = (int)Focus_Result[s_id].Focus_Min_Idx;
}

void getFocusMax(int s_id, int *value)
{
	*value     = Focus_Result[s_id].FocusData[0].Focus_Max;
	*(value+1) = Focus_Result[s_id].FocusData[1].Focus_Max;
	*(value+2) = Focus_Result[s_id].FocusData[2].Focus_Max;
	*(value+3) = Focus_Result[s_id].FocusData[3].Focus_Max;
}

struct Focus_Result_Struct Focus_Result[5];		//[sensor]
void WriteFocusResult(int s_id)
{
	int i, cnt=4;
	FILE *fp;
	int fp2fd;
	char path[128];

	//if(s_id == 0) cnt = 4;
	//else          cnt = 3;
	Focus_Result[s_id].CheckSum = FOCUS_ADJ_CHECK_SUM_1;
	for(i = 0; i < cnt; i++) {
		if(Focus_Result[s_id].FocusData[i].IsPass == 0) {
			Focus_Result[s_id].Pass = 0;
			break;
		}
		else
			Focus_Result[s_id].Pass = 1;
	}
	sprintf(path, "/mnt/sdcard/US360/Test/Focus_S%d.bin\0", s_id);

	fp = fopen(path, "wb");
	if(fp != NULL) {
		fwrite(&Focus_Result[s_id], sizeof(Focus_Result[s_id]), 1, fp);
		fclose(fp);
	}

#ifdef ANDROID_CODE
    fflush(fp);
    fp2fd = fileno(fp);
    fsync(fp2fd);
    fclose(fp);
    close(fp2fd);
#endif
}

int ReadFocusResult(int s_id, int type)
{
	FILE *fp;
	char path[128];

	if(type == 0) sprintf(path, "/mnt/sdcard/US360/Test/Focus_S%d.bin\0", s_id);
	else		  sprintf(path, "C:\\US360Tmp\\Focus_S%d.bin\0", s_id);
	fp = fopen(path, "rb");
	if(fp != NULL) {
		fread(&Focus_Result[s_id], sizeof(Focus_Result[s_id]), 1, fp);
		fclose(fp);
	}
	else
		return -1;

	return 0;
}

struct Test_Tool_Focus_Adj_Struct Test_Tool_Focus_Adj;
//Test Tool Init用
int Test_Tool_Focus_Adj_Init(int tool_num)
{
	int i, j;
	FILE *fp;
	char path[128];

	sprintf(path, "C:\\US360Tmp\\test\\Focus_Adj_%d.bin\0", tool_num);
	fp = fopen(path, "rb");
	if(fp != NULL) {
		fread(&Test_Tool_Focus_Adj, sizeof(Test_Tool_Focus_Adj), 1, fp);
		fclose(fp);
		return 0;
    }
	else {
		Test_Tool_Focus_Adj.CheckSum = FOCUS_ADJ_CHECK_SUM_1;
		Test_Tool_Focus_Adj.White_Th = FOCUS_NOT_CHANGE_CODE;
		Test_Tool_Focus_Adj.Black_Th = FOCUS_NOT_CHANGE_CODE;
		Test_Tool_Focus_Adj.Ep		 = FOCUS_NOT_CHANGE_CODE;
		Test_Tool_Focus_Adj.Gain	 = FOCUS_NOT_CHANGE_CODE;
		for(i = 0; i < 5; i++) {
			Test_Tool_Focus_Adj.Offset[i].X = FOCUS_NOT_CHANGE_CODE;
			Test_Tool_Focus_Adj.Offset[i].Y = FOCUS_NOT_CHANGE_CODE;
			for(j = 0; j < 4; j++) {
				Test_Tool_Focus_Adj.Focus_Th[i][j]   = FOCUS_NOT_CHANGE_CODE;
				Test_Tool_Focus_Adj.Focus_Rate[i][j] = FOCUS_NOT_CHANGE_CODE;
			}
		}
		Test_Tool_Focus_Adj.Tool_Num = tool_num;
		sprintf(Test_Tool_Focus_Adj.Sample_Num, "US_4702\0");
		Test_Tool_Focus_Adj.Tool_Focus_Rate	= FOCUS_NOT_CHANGE_CODE;
		Test_Tool_Focus_Adj.Adj_Posi_Flag = FOCUS_NOT_CHANGE_CODE;
		return 1;
	}
}

//Test Tool 寫檔給本機, 調整 Focus 參數
void WriteTestToolFocusAdjFile(int tool_num)
{
	int i;
	FILE *fp;
	char path[128];
	for(i = 0; i < 2; i++) {
		if(i == 0) sprintf(path, "C:\\US360Tmp\\test\\Focus_Adj_%d.bin\0", tool_num);
		else       sprintf(path, "C:\\US360Tmp\\test\\Focus_Adj.bin\0");
	fp = fopen(path, "wb");
	if(fp != NULL) {
		fwrite(&Test_Tool_Focus_Adj, sizeof(Test_Tool_Focus_Adj), 1, fp);
		fclose(fp);
	}
	}
}

//本機讀取 Test Tool, 調整 Focus 參數
void ReadTestToolFocusAdjFile(int s_id) {
	int j;
	FILE *fp;
	char path[128];
	int tool_num;

	sprintf(path, "/mnt/sdcard/US360/Test/Focus_Adj.bin\0");
	fp = fopen(path, "rb");
	if(fp != NULL) {
		fread(&Test_Tool_Focus_Adj, sizeof(Test_Tool_Focus_Adj), 1, fp);

		Focus_Result[s_id].Tool_Num = Test_Tool_Focus_Adj.Tool_Num;
		memcpy(&Focus_Result[s_id].Sample_Num[0], &Test_Tool_Focus_Adj.Sample_Num[0], 16);

		tool_num = Focus_Result[s_id].Tool_Num;
		SetFocusToolNum(tool_num);
		if(Test_Tool_Focus_Adj.Adj_Posi_Flag != FOCUS_NOT_CHANGE_CODE)
			ReadAdjFocusPosi(s_id, tool_num);
		else {
#ifdef ANDROID_CODE
			for(j = 0; j < 4; j++)
				Set_Focus_XY(tool_num, s_id, j, Adj_Foucs_Posi_Default[tool_num-1][s_id][j].X, Adj_Foucs_Posi_Default[tool_num-1][s_id][j].Y);
#endif
		}
#ifdef ANDROID_CODE
		for(j = 0; j < 4; j++) {
			Get_Focus_XY(Focus_Tool_Num, s_id, j, &Focus_Result[s_id].FocusData[j].S_Posi.X, &Focus_Result[s_id].FocusData[j].S_Posi.Y);
		}
#endif

		Focus_Result[s_id].Sensor_Offset.X = Test_Tool_Focus_Adj.Offset[s_id].X;
		Focus_Result[s_id].Sensor_Offset.Y = Test_Tool_Focus_Adj.Offset[s_id].Y;
#ifdef ANDROID_CODE
		Set_Focus_XY_Offset(s_id, 0, Test_Tool_Focus_Adj.Offset[s_id].X);
		Set_Focus_XY_Offset(s_id, 1, Test_Tool_Focus_Adj.Offset[s_id].Y);
#endif
		for(j = 0; j < 4; j++) {
			if(Test_Tool_Focus_Adj.Focus_Th[s_id][j] != FOCUS_NOT_CHANGE_CODE)
				Focus_Result[s_id].FocusData[j].Focus_Th = Test_Tool_Focus_Adj.Focus_Th[s_id][j];

			if(Test_Tool_Focus_Adj.Focus_Rate[s_id][j] != FOCUS_NOT_CHANGE_CODE)
				Focus_Result[s_id].FocusData[j].Focus_Rate = Test_Tool_Focus_Adj.Focus_Rate[s_id][j];
		}
		if(Test_Tool_Focus_Adj.White_Th != FOCUS_NOT_CHANGE_CODE)
			White_Th = Test_Tool_Focus_Adj.White_Th;
		if(Test_Tool_Focus_Adj.Black_Th != FOCUS_NOT_CHANGE_CODE)
			Black_Th = Test_Tool_Focus_Adj.Black_Th;

#ifdef ANDROID_CODE
		if(Test_Tool_Focus_Adj.Ep != FOCUS_NOT_CHANGE_CODE)
			setFocusEPIdx(Test_Tool_Focus_Adj.Ep);
		if(Test_Tool_Focus_Adj.Gain != FOCUS_NOT_CHANGE_CODE)
			setFocusGainIdx(Test_Tool_Focus_Adj.Gain);
#endif

		fclose(fp);
	}
}

//TestTool 寫檔給本機, Focus 座標
void WriteAdjFocusPosi(int tool_num)
{
	FILE *fp;
	char path[128];
	sprintf(path, "C:\\US360Tmp\\test\\Adj_Foucs_Posi_%d.bin\0", tool_num);
	fp = fopen(path, "wb");
	if(fp != NULL) {
		fwrite(&Adj_Foucs_Posi[0][0], sizeof(Adj_Foucs_Posi), 1, fp);
		fclose(fp);
	}
	return;
}

//本機讀取 TestTool Focus 座標
void ReadAdjFocusPosi(int s_id, int tool_num)
{
	int i;
	FILE *fp;
	char path[128];

	sprintf(path, "/mnt/sdcard/US360/Test/Adj_Foucs_Posi.bin\0");
	fp = fopen(path, "rb");
	if(fp != NULL) {
		fread(&Adj_Foucs_Posi[0][0], sizeof(Adj_Foucs_Posi), 1, fp);
#ifdef ANDROID_CODE
		for(i = 0; i < 4; i++)
			Set_Focus_XY(tool_num, s_id, i, Adj_Foucs_Posi[s_id][i].X, Adj_Foucs_Posi[s_id][i].Y);
#endif
		fclose(fp);
	}
	else {
#ifdef ANDROID_CODE
		for(i = 0; i < 4; i++)
			Set_Focus_XY(tool_num, s_id, i, Adj_Foucs_Posi_Default[tool_num-1][s_id][i].X, Adj_Foucs_Posi_Default[tool_num-1][s_id][i].Y);
#endif
	}
	return;
}

