/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Cmd/defect.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <malloc.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "common/app_log.h"
#include "Device/US363/Cmd/us363_spi.h"
#include "Device/US363/Cmd/us360_define.h"
#include "Device/US363/Cmd/spi_cmd.h"
#include "Device/US363/Cmd/us360_func.h"
#include "Device/US363/Cmd/fpga_driver.h"
#include "Device/US363/Kernel/k_spi_cmd.h"

#undef LOG_TAG
#define LOG_TAG "US363::Defect"

int Defect_Type = 0;    //0:test tool 1:wifi(user)
int Defect_Step = 0;
int Defect_State  = 0; 		//-1:err 	0:none	1:ok
int Defect_Th = 4;
int Defect_Ep = 30;
int Defect_X_Offset = 35;		// (FPGA迴路偏移:3) + (ISP2->ST DDR偏移:32)
int Defect_Y_Offset = 4;		// (FPGA迴路偏移:4)
int Defect_Cnt[DEFECT_IMG_Y >> 3][DEFECT_IMG_X >> 3];
unsigned char Defect_Y_Table[DEFECT_IMG_Y][DEFECT_IMG_X][4];		// Bitmap 1pix = RGBA, 4Byte
int Defect_Avg_Th = 30;
int defect_cnt = 0;
char Defect_Copy_Buf[DEFECT_COPY_BUF_MAX];
int Defect_Debug_En = 0;

Defect_Avg_Y_Struct Defect_Avg_Y_Table[DEFECT_AVGY_Y][DEFECT_AVGY_X];
Defect_Cmd_Struct Defect_Cmd_Tabel[DEFECT_CMD_TABLE_Y][DEFECT_CMD_TABLE_X];		//[205][1104]
Defect_Posi_Struct Defect_Y_3X3_Table[DEFECT_IMG_Y][DEFECT_IMG_X];
Defect_8x8_Struct Defect_8X8_Tabel;

//儲存抓壞點前的參數
int Defect_CMode_lst = 0;
int Defect_Res_lst   = 0;
int Defect_Ep_lst    = 30;
int Defect_Gani_lst  = 0;
unsigned long long Defect_T1 = 0;
unsigned long long Defect_T2 = 0;

void SetDefectStep(int step) {
	Defect_Step = step;
}

int GetDefectStep() {
	return Defect_Step;
}

/*
 * 3x3 Table
 */
int Make_Defect_3X3_Table(unsigned s_id, unsigned size_x, unsigned size_y) {
	unsigned i, j, k;
	unsigned i2, j2;
	unsigned i3, j3;
	unsigned sum, avg, cnt;
	unsigned s_x2 = (size_x-1), s_y2 = (size_y-1-2);				// -2條掃描線: Sensor 畫面下方有2條垃圾畫面

	memset(&Defect_Avg_Y_Table[0][0], 0, sizeof(Defect_Avg_Y_Table) );
	memset(&Defect_Y_3X3_Table[0][0], 0, sizeof(Defect_Y_3X3_Table) );
	db_debug("Get_Defect_3X3_Table() 00 s_id=%d\n", s_id);
	//計算3X3平均值, 並判斷門檻值
	for(i = 1; i < s_y2; i++) {
		for(j = 1; j < s_x2; j++) {
			sum = 0; cnt = 0;
			for(k = 0; k < 9; k++) {
				i2 = i + (k/3) - 1;
				if(i2 < 0 || i2 >= DEFECT_IMG_Y) continue;
				j2 = j + (k%3) - 1;
				if(j2 < 0 || j2 >= DEFECT_IMG_X) continue;
				sum += Defect_Y_Table[i2][j2][1];
				cnt++;
			}
			if(cnt == 0) avg = 0;
			else 		 avg = sum / cnt;

			Defect_Avg_Y_Table[i>>3][j>>3].Sum += Defect_Y_Table[i][j][1];
			Defect_Avg_Y_Table[i>>3][j>>3].Cnt++;

			i3 = i + Defect_Y_Offset;
			if(i3 < 0 || i3 >= DEFECT_IMG_Y) continue;
			j3 = j + Defect_X_Offset;
			if(j3 < 0 || j3 >= DEFECT_IMG_X) continue;
			Defect_Y_3X3_Table[i3][j3].value = avg;
			if(avg > Defect_Th)
				Defect_Y_3X3_Table[i3][j3].flag = 1;
			else
				Defect_Y_3X3_Table[i3][j3].flag = 0;
        }
	}

	// Cal Avg Y
	int avg_max = 0, err_cnt = 0, err_cnt_max = 0;
	for(i = 0; i < DEFECT_AVGY_Y; i++) {
		for(j = 0; j < DEFECT_AVGY_X; j++) {
			if(Defect_Avg_Y_Table[i][j].Cnt == 0)
				Defect_Avg_Y_Table[i][j].Avg = 0;
			else
				Defect_Avg_Y_Table[i][j].Avg = Defect_Avg_Y_Table[i][j].Sum / Defect_Avg_Y_Table[i][j].Cnt;
			if(Defect_Avg_Y_Table[i][j].Avg > Defect_Avg_Th) {
				err_cnt++;
				if(err_cnt > err_cnt_max) err_cnt_max = err_cnt;
				if(err_cnt >= 16) {		//連續16個 block 超過門檻表示有漏光
					db_error("Make_Defect_3X3_Table() Avg > Defect_Avg_Th !! (i=%d j=%d Avg=%d) cnt_max=%d\n", i, j, Defect_Avg_Y_Table[i][j].Avg, err_cnt_max);
					return -1;
				}
			}
			else
				err_cnt = 0;
			if(Defect_Avg_Y_Table[i][j].Avg > avg_max)
				avg_max = Defect_Avg_Y_Table[i][j].Avg;
		}
	}

	db_debug("Get_Defect_3X3_Table() 01 avg_max=%d err_cnt_max=%d\n", avg_max, err_cnt_max);
	//找重心
	for(i = 1; i < s_y2; i++) {
		for(j = 1; j < s_x2; j++) {
			if(Defect_Y_3X3_Table[i][j].flag == 1) {
				for(k = 0; k < 9; k++) {		//周圍必須小於中心的值
					i2 = i + (k/3) - 1;
					if(i2 < 0) i2 = 0; if(i2 >= DEFECT_IMG_Y) i2 = (DEFECT_IMG_Y-1);
					j2 = j + (k%3) - 1;
					if(j2 < 0) j2 = 0; if(j2 >= DEFECT_IMG_X) j2 = (DEFECT_IMG_X-1);
					if(i2 == i && j2 == j) continue;
					if(Defect_Y_3X3_Table[i2][j2].value >= Defect_Y_3X3_Table[i][j].value &&
							Defect_Y_3X3_Table[i2][j2].flag == 1) {
						Defect_Y_3X3_Table[i][j].flag = 0;
						break;
					}
				}
			}
		}
	}
	return 0;
}

void Set_8x8_Table(int idx, int s_x, int s_y, int value, int t_x, int t_y, int cmd_x, int cmd_y) {

	if(idx == 0) {			// P0
		Defect_8X8_Tabel.P0[t_y][t_x].x       = s_x;			//紀錄目前P0使用3x3 table的哪個值
		Defect_8X8_Tabel.P0[t_y][t_x].y       = s_y;
		Defect_8X8_Tabel.P0[t_y][t_x].value   = value;
		Defect_8X8_Tabel.Table[t_y][t_x].P0.X = cmd_x & 0xF;	//實際下給FPGA的座標
		Defect_8X8_Tabel.Table[t_y][t_x].P0.Y = cmd_y & 0xF;
	}
	else {					// P1
		Defect_8X8_Tabel.P1[t_y][t_x].x       = s_x;			//紀錄目前P1使用3x3 table的哪個值
		Defect_8X8_Tabel.P1[t_y][t_x].y       = s_y;
		Defect_8X8_Tabel.P1[t_y][t_x].value   = value;
		Defect_8X8_Tabel.Table[t_y][t_x].P1.X = cmd_x & 0xF;	//實際下給FPGA的座標
		Defect_8X8_Tabel.Table[t_y][t_x].P1.Y = cmd_y & 0xF;
	}
}

/*
 * 3x3 to 8x8 Table
 */
void Make_Defect_8X8_Table(unsigned s_id, unsigned size_x, unsigned size_y) {
	int i, j, m, n;
	int i2, j2;
	int i2_ofs, j2_ofs;
	//int i3, j3;
	int s_x2 = (size_x >> 3), s_y2 = (size_y >> 3);
	int cmd_x, cmd_y;

	defect_cnt = 0;
	memset(&Defect_Cnt[0][0], 0, sizeof(Defect_Cnt) );
	memset(&Defect_8X8_Tabel, 0, sizeof(Defect_8X8_Tabel) );
	memset(&Defect_8X8_Tabel.Table[0][0], 0xEE, sizeof(Defect_8X8_Tabel.Table) );
	db_debug("Get_Defect_8X8_Table() 00 s_id=%d\n", s_id);
	//3X3 to 8X8
	for(i = 0; i < s_y2; i++) {
		for(j = 0; j < s_x2; j++) {
			for(m = -1; m < 9; m++) {
				for(n = -1; n < 9; n++) {
					i2 = i * 8 + m;
					if(i2 < 0 || i2 >= DEFECT_IMG_Y) continue;
					j2 = j * 8 + n;
					if(j2 < 0 || j2 >= DEFECT_IMG_X) continue;

					if(Defect_Y_3X3_Table[i2][j2].flag == 1) {
						if(Defect_Y_3X3_Table[i2][j2].value >= Defect_8X8_Tabel.P0[i][j].value &&
								Defect_Y_3X3_Table[i2][j2].value >= Defect_8X8_Tabel.P1[i][j].value) {

							if(Defect_8X8_Tabel.P0[i][j].value <= Defect_8X8_Tabel.P1[i][j].value)
								Set_8x8_Table(0, j2, i2, Defect_Y_3X3_Table[i2][j2].value, j, i, n, m);
							else
								Set_8x8_Table(1, j2, i2, Defect_Y_3X3_Table[i2][j2].value, j, i, n, m);
						}
						else if(Defect_Y_3X3_Table[i2][j2].value > Defect_8X8_Tabel.P0[i][j].value)
							Set_8x8_Table(0, j2, i2, Defect_Y_3X3_Table[i2][j2].value, j, i, n, m);
						else if(Defect_Y_3X3_Table[i2][j2].value > Defect_8X8_Tabel.P1[i][j].value)
							Set_8x8_Table(1, j2, i2, Defect_Y_3X3_Table[i2][j2].value, j, i, n, m);

						Defect_Cnt[i][j]++;

						defect_cnt++;
					}
				}
			}
		}
	}
	db_debug("Get_Defect_8X8_Table() 02 defect_cnt=%d\n", defect_cnt);
}

/*
 * 	Cmd Table
 * 	1 block = 8x8, 205 * 1152 block
 *
 *		192 byte			192 byte
 *	 24 block * 2		 24 block * 2
 * 	|X0 Y0| |X0 Y1| 	|X1 Y0| |X1 Y1|		...
 * 	|X0 Y2| |X0 Y3| 	|X1 Y2| |X1 Y3|		...
 * 	|X0 Y4| |X0 Y5| 	|X1 Y4| |X1 Y5|
 * 	|X0 Y6| |X0 Y7| 	|X1 Y6| |X1 Y7|
 * 	...
 * 	...
 */
/*
 * 8x8 to Cmd Table
 */
void Make_Defect_Cmd_Table(unsigned s_id, unsigned size_x, unsigned size_y) {
	int i, j, k;
	int sx2, sy2;
	int x_idx, y_idx;
	int x_mod, y_mod;
	int x_tmp, y_tmp;

	memset(&Defect_Cmd_Tabel[0][0], 0xEE, sizeof(Defect_Cmd_Tabel) );

	//8x8 to cmd table
	sx2 = (size_x >> 3);
	sy2 = (size_y >> 3);
	for(i = 0; i < sy2; i++) {			//410 block
		for(j = 0; j < sx2; j++) {		//540 block
			x_idx = (j / 24) * 48;  x_mod = j % 24;		//24 block=192/8
			y_idx = (i >> 1); 		y_mod = (i & 0x1);
			x_tmp = x_idx + 24 * y_mod + x_mod;
			y_tmp = y_idx;
			Defect_Cmd_Tabel[y_tmp][x_tmp] = Defect_8X8_Tabel.Table[i][j];
		}
	}
}

int Write_Defect_File(unsigned s_id) {
	FILE *fp;
	char path[128];

	sprintf(path, "/mnt/sdcard/US360/Defect/Tmp/defect_s%d_tmp.bin\0", s_id);
	fp = fopen(path, "wb");
	if(fp != NULL) {
		fwrite(&Defect_Cmd_Tabel[0][0], sizeof(Defect_Cmd_Tabel), 1, fp);
		fclose(fp);
	}
	else
		return -1;

	return 0;
}

int CopyDefectFile(int type) {
	int i, size, err=0;
	FILE *fp;
	char path1[128], path2[128];
	struct stat sti;

	for(i = 0; i < 5; i++) {
		//copy img
		err = 0;
		memset(&Defect_Copy_Buf[0], 0, sizeof(Defect_Copy_Buf) );
		sprintf(path1, "/mnt/sdcard/US360/Defect/Tmp/Defect_S%d_tmp.jpg\0", i);
		if(stat(path1,&sti) == 0) {
			size = sti.st_size;
			fp = fopen(path1, "rb");
			if(fp != NULL) {
				fread(&Defect_Copy_Buf[0], size, 1, fp);
				fclose(fp);
				err = 0;
			}
			else
				err = 1;

			if(err == 0) {
				if(type == 0) sprintf(path2, "/mnt/sdcard/US360/Defect/Defect_S%d.jpg\0", i);
				else		  sprintf(path2, "/mnt/sdcard/US360/Defect/Defect_S%d_user.jpg\0", i);
				fp = fopen(path2, "wb");
				if(fp != NULL) {
					fwrite(&Defect_Copy_Buf[0], size, 1, fp);
					fclose(fp);
				}
			}
		}

		//copy bin
		err = 0;
		memset(&Defect_Copy_Buf[0], 0, sizeof(Defect_Copy_Buf) );
		sprintf(path1, "/mnt/sdcard/US360/Defect/Tmp/defect_s%d_tmp.bin\0", i);
		if(stat(path1,&sti) == 0) {
			size = sti.st_size;
			fp = fopen(path1, "rb");
			if(fp != NULL) {
				fread(&Defect_Copy_Buf[0], size, 1, fp);
				fclose(fp);
				err = 0;
			}
			else
				err = 1;

			if(err == 0) {
				if(type == 0) sprintf(path2, "/mnt/sdcard/US360/Defect/defect_s%d.bin\0", i);
				else		  sprintf(path2, "/mnt/sdcard/US360/Defect/defect_s%d_user.bin\0", i);
				fp = fopen(path2, "wb");
				if(fp != NULL) {
					fwrite(&Defect_Copy_Buf[0], size, 1, fp);
					fclose(fp);
				}
			}
		}
	}

	return 0;
}

int Read_Defect_File(unsigned s_id) {
	FILE *fp;
	char path[128];
	struct stat sti;
	int old_file=0;

	memset(&Defect_Cmd_Tabel[0][0], 0xEE, sizeof(Defect_Cmd_Tabel) );

	sprintf(path, "/mnt/sdcard/US360/Defect/defect_s%d_user.bin\0", s_id);
	if(stat(path, &sti) != 0) {
		sprintf(path, "/mnt/sdcard/US360/Defect/defect_s%d.bin\0", s_id);
		if(stat(path, &sti) != 0)
			old_file = 1;
	}

	if(old_file == 1) {			//相容舊檔名
		sprintf(path, "/mnt/sdcard/US360/BadDot/bad_dot_s%d_user.bin\0", s_id);
		if(stat(path, &sti) != 0)
			sprintf(path, "/mnt/sdcard/US360/BadDot/bad_dot_s%d.bin\0", s_id);
			if(stat(path, &sti) != 0)
				return -1;
	}

	db_debug("Read_Defect_File: path=%s\n", path);
	fp = fopen(path, "rb");
	if(fp != NULL) {
		fread(&Defect_Cmd_Tabel[0][0], sizeof(Defect_Cmd_Tabel), 1, fp);
		fclose(fp);
	}
	else
		return -1;

	return 0;
}

/*
 * Write Cmd Table to DDR
 */
void Send_Defect_Table(unsigned s_id, unsigned size_x, unsigned size_y) {
	int i, j, k;
	unsigned addr, size, cmd_mode, id, tmp_s;
	unsigned s_addr, t_addr;
	unsigned f2_addr, fx_addr;
	unsigned FPGA_ID, MT;
	unsigned char *buf;

	Get_FId_MT(s_id, &FPGA_ID, &MT);
	f2_addr = F2_DEFECT_TABLE_ADDR + s_id * 0xC00;
	fx_addr = FX_DEFECT_TABLE_ADDR + MT * 0xC00;

	for(i = 0; i < DEFECT_CMD_TABLE_Y; i++) {
		buf = (unsigned char *) &Defect_Cmd_Tabel[i][0];
		addr = f2_addr + i * 0x8000;
		size = sizeof(Defect_Cmd_Tabel[i]);
		tmp_s = 1024;
		for(j = 0; j < size; j += tmp_s) {
			ua360_spi_ddr_write(addr + j, (int *) &buf[j], tmp_s);

			s_addr = (f2_addr + i * 0x8000 + j);
			t_addr = (fx_addr + i * 0x8000 + j);
			cmd_mode = 1;
			if(FPGA_ID == 0) id = AS2_MSPI_F0_ID;
			else             id = AS2_MSPI_F1_ID;
			AS2_Write_F2_to_FX_CMD(s_addr, t_addr, tmp_s, cmd_mode, id, 1);
		}
	}
}

/*
 * decode img / make 3x3 table
 */
/*int Make_Defect_3x3_Table(int type)	//Main.java
{
	int i, ret;
   	String file_name;
    	
	for(i = 0; i < 5; i++) {  
		file_name = "/mnt/sdcard/US360/Defect/Tmp/Defect_S" + i + "_tmp.jpg";
        Bitmap bitmap = BitmapFactory.decodeFile(file_name);
        if(bitmap != null) {
          	int size = bitmap.getByteCount();
	        ByteBuffer buf = ByteBuffer.allocate(size);
	        bitmap.copyPixelsToBuffer(buf);
		    byte[] byteArray = buf.array();
       
			ret = MakeDefectTable(i, byteArray, size);
			if(ret == -1) return -1; 
			        
		    bitmap.recycle();
		    bitmap = null;
        }
        else return -1; 
	}

	return 0;
}*/
	
int MakeDefectTable() {
	int s_id, ret;

//tmp for(s_id = 0; s_id < 5; s_id++) {  
	// decode "/mnt/sdcard/US360/Defect/Tmp/Defect_S" + s_id + "_tmp.jpg" to bitmap,
	// than save to Defect_Y_Table[0][0][0]
//tmp    jbyte *p = (*env)->GetByteArrayElements(env, buf, NULL);
//tmp    memcpy(&Defect_Y_Table[0][0][0], p, size);
//tmp    (*env)->ReleaseByteArrayElements(env, buf, p, 0);

    ret = Make_Defect_3X3_Table(s_id, DEFECT_IMG_X, DEFECT_IMG_Y);	//Make 3x3
    if(ret == -1) return -1;
	
	Make_Defect_8X8_Table(s_id, DEFECT_IMG_X, DEFECT_IMG_Y);			//3X3 to 8X8
	Make_Defect_Cmd_Table(s_id, DEFECT_IMG_X, DEFECT_IMG_Y);			//8x8 to Cmd Table
	
	ret = Write_Defect_File(s_id);										//Save Cmd Table
	if(ret == -1) return -1;
	
    Send_Defect_Table(s_id, DEFECT_IMG_X, DEFECT_IMG_Y);				//Send Table
//tmp }	
    return 0;
}

int DefectInit() {
	int s_id, ret;
	for(s_id = 0; s_id < 5; s_id++) {
		ret = Read_Defect_File(s_id);										//Save Cmd Table
		if(ret == -1) return -1;
		Send_Defect_Table(s_id, DEFECT_IMG_X, DEFECT_IMG_Y);				//Send Table
	}
    return 0;
}

void SetDefectDebugEn(int en) {
	int i, ret, addr, size, id, cmd_mode;
    unsigned Data[64];

    memset(&Data[0], 0, sizeof(Data) );

    Defect_Debug_En = en & 0x1;

    addr = 0x1F800400;
    size = sizeof(Data);
    Data[0] = 0xCCAAA019;
    Data[1] = Defect_Debug_En;
    ua360_spi_ddr_write(addr, (int *) &Data[0], size);

	unsigned s_addr, t_addr;
	for(i = 0; i < 2; i++) {
		s_addr = addr;
		t_addr = FPGA_REG_SET_MOCOM;
		cmd_mode = 0;
		if(i == 0) id = AS2_MSPI_F0_ID;
		else       id = AS2_MSPI_F1_ID;
		AS2_Write_F2_to_FX_CMD(s_addr, t_addr, size, cmd_mode, id, 1);
	}
}

void SetDefectState(int state) {
	Defect_State = state;
}
int GetDefectState() {
	return Defect_State;
}

void Delete_Defect_File(int type) {
	int i;
	char path[64];
	struct stat sti;

    for(i = 0; i < 5; i++) {
	   	//delete tmp file
    	sprintf(path, "/mnt/sdcard/US360/Defect/Tmp/Defect_S%d_tmp.jpg\0", i);
	   	if(stat(path, &sti) == 0) {
	   		remove(path);
	   		db_debug("Delete_Defect_File() delete defect img i=%d\n", i);
	   	}

	   	sprintf(path, "/mnt/sdcard/US360/Defect/Tmp/defect_s%d_tmp.bin\0", i);
	   	if(stat(path, &sti) == 0) {
	   		remove(path);
	   		db_debug("Delete_Defect_File() delete defect file i=%d\n", i);
	   	}

    	if(type == 0) {
	   		//delete user file
	    	sprintf(path, "/mnt/sdcard/US360/Defect/Defect_S%d_user.jpg\0", i);
	    	if(stat(path, &sti) == 0) {
	    		remove(path);
	    		db_debug("Delete_Defect_File() delete defect img i=%d\n", i);
	    	}

	    	sprintf(path, "/mnt/sdcard/US360/Defect/defect_s%d_user.bin\0", i);
	    	if(stat(path, &sti) == 0) {
	    		remove(path);
	    		db_debug("Delete_Defect_File() delete defect file i=%d\n", i);
	    	}
    	}
    }

	sprintf(path, "/mnt/sdcard/US360/Defect/DefectState.bin\0");
	if(stat(path, &sti) == 0) {
		remove(path);
		db_debug("Delete_Defect_File() delete DefectState.bin\n");
	}
}

void Write_Defect_State(int state) {
    char dirStr[64]  = "/mnt/sdcard/US360\0";
    char fileStr[64] = "/mnt/sdcard/US360/Defect/DefectState.bin\0";
    char dirTestStr[64]  = "/mnt/sdcard/US360/Defect\0";
	struct stat sti;
	FILE *fp = NULL;

    if(stat(dirStr, &sti) != 0) {
        if(mkdir(dirStr, S_IRWXU) != 0)
            db_error("Write_Defect_State: create %s folder fail\n", dirStr);
    }

    if(stat(dirTestStr, &sti) != 0) {
        if(mkdir(dirTestStr, S_IRWXU) != 0)
            db_error("Write_Defect_State: create %s folder fail\n", dirTestStr);
    }

    fp = fopen(fileStr, "wb");
    if(fp != NULL) {
    	fwrite(&state, sizeof(state), 1, fp);
    	fclose(fp);
    }
}

void Write_Defect_Ep(int ep) {
	char dirStr[64]  = "/mnt/sdcard/US360\0";
	char fileStr[64] = "/mnt/sdcard/US360/Defect/DefectEp.bin\0";
	char dirTestStr[64]  = "/mnt/sdcard/US360/Defect\0";
	struct stat sti;
	FILE *fp = NULL;

    if(stat(dirStr, &sti) != 0) {
        if(mkdir(dirStr, S_IRWXU) != 0)
            db_error("Write_Defect_Ep: create %s folder fail\n", dirStr);
    }

    if(stat(dirTestStr, &sti) != 0) {
        if(mkdir(dirTestStr, S_IRWXU) != 0)
            db_error("Write_Defect_Ep: create %s folder fail\n", dirTestStr);
    }

    fp = fopen(fileStr, "wb");
    if(fp != NULL) {
    	fwrite(&ep, sizeof(ep), 1, fp);
    	fclose(fp);
    }
}

int do_Defect_Func() {
   	int i, ret = 0;
   	unsigned long long nowTime;

   	if(Defect_Step == 0) return 0;

   	switch(Defect_Step) {
   	case 1:		// Init / Change Mode
//tmp   		Defect_CMode_lst = CameraMode;
//tmp   		Defect_Res_lst   = ResolutionMode;
//tmp   		Defect_Ep_lst    = Get_DataBin_BmodeSec();
//tmp   		Defect_Gani_lst  = Get_DataBin_BmodeGain();

    	Delete_Defect_File(Defect_Type);
    	set_A2K_ISP2_Defect_En(0);
    	Defect_State = 0;

//tmp    	SetLedBrightness(0);		//ledBrightness = 0;
//tmp    	setOledControl(1);

    	//change mode
//tmp    	CameraMode     = 12;
//tmp    	ResolutionMode = 1;
    	setAEGBExp1Sec(Defect_Ep);
    	setAEGBExpGain(0);
//tmp        ModeTypeSelectS2(jniEnv, 0, ResolutionMode, HDMI_State, User_Ctrl, CameraMode);        // return: FPS、ResolutionWidth、ResolutionHeight
//tmp        stopREC(12);
//tmp        choose_mode_flag = 1;

//tmp        get_current_usec(&Defect_T1);
        Defect_Step++;
        break;
   	case 2:		//delay
//tmp   		get_current_usec(&Defect_T2);
   		if( (Defect_T2 - Defect_T1) >= 3000000)
   			Defect_Step++;
   		break;
   	case 3:		// Cap
//tmp   		ret = doTakePicture(jniEnv, 8);
   		if(ret == 1) Defect_Step++;
//tmp   		get_current_usec(&Defect_T1);
   		break;
   	case 4:		// Check Get Img (5P)
//tmp   		get_current_usec(&Defect_T2);
//tmp   		ret = Capture_Is_Finish();
		if( (Defect_T2 - Defect_T1) > 90000000) {		// 60秒未取到5張圖timeout
			Defect_Step = 6;
			Defect_State  = -1;
			db_error("do_Defect_Func() 04 err\n");
		}
		else if(ret == 1)
			Defect_Step++;
   		break;
   	case 5:		// decode img / make 3x3 tabel / save file
   		ret = MakeDefectTable();
   		if(ret == 0)
   			Defect_Step++;
   		else {
   			Defect_Step = 6;
   			Defect_State = -1;
   			db_error("do_Defect_Func() 05 err\n");
   		}
   		break;
   	case 6:		// Finish / Change Mode
   		if(Defect_State == 0) {
   			CopyDefectFile(Defect_Type);
   			set_A2K_ISP2_Defect_En(1);
   			Defect_State = 1;
   		}
   		else
   			set_A2K_ISP2_Defect_En(0);

		if(Defect_Type == 0) {		//Test Tool Write State
			Write_Defect_State(Defect_State);
		}

//tmp    	SetLedBrightness(Get_DataBin_LedBrightness() );
//tmp    	setOledControl(Get_DataBin_OledControl() );

   		//change mode
//tmp    	CameraMode     = Defect_CMode_lst;
//tmp    	ResolutionMode = Defect_Res_lst;
    	setAEGBExp1Sec(Defect_Ep_lst);
    	setAEGBExpGain(Defect_Gani_lst);
//tmp        ModeTypeSelectS2(jniEnv, 0, ResolutionMode, HDMI_State, User_Ctrl, CameraMode);
//tmp        stopREC(13);
//tmp        choose_mode_flag = 1;
        Defect_Step = 0;
//tmp        get_current_usec(&nowTime);
//tmp        Set_Cap_Rec_Finish_Time(nowTime, POWER_SAVING_CMD_OVERTIME_5S, 11);
   		break;
   	}

    return 0;
}