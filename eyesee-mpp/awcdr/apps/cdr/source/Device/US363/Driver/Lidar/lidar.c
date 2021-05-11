/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Driver/Lidar/lidar.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <dirent.h>

//tmp #include "us360.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::Lidar"


int imageH = 1920;
int imageW = 3840;

int listcnt = 0;
//float pointValue[1200][2];
int saveValue[1200][3][6];
int saveCnt[6];
//tmp float save3D[1024][6][2048][1];	//48M
float ****save3D = NULL;
//tmp int save3DSer[1024][2048][1];		//8M
int ***save3DSer = NULL;
//int save3DSqlit[1024][2048][1];
int nearOtherNum[1];
int nearOtherDist[1];
int save3DCnt[2048][1];

//tmp float triData[0x200000][15][1];	//120M
float ***triData = NULL;
// 0-2:p1_loc 3-5:p2_loc  6-8:p3_loc 9-10:p1_img 11-12:p2_img 13-14:p3_img
//tmp float triSub[0x200000][10][1];	//80M
float ***triSub = NULL;
// 0:status 1:deg 2:dist 3-5: center_loc 6:p1_Number 7:p2_Number 8:p3_number
int triCnt[5];
float bottomLen[5];
//float pointList[0x200000][6];
//int pointSer[0x200000];
//float projection[2400][5];
int pointCnt;
int *lidarBuf;

//tmp int lidarSinCosData[0x200000];			// 8MB
int *lidarSinCosData = NULL;			// 8MB
int *lidarDistanceData = NULL;

double PI = 3.14159;

int likeRange = 3;
int nummx[5],nummy[5];
int startNum[5];
int endNum[5];
int totScan = 0;
float avgDeg[5];
int SerialNumber = 1;
int reads = 1;
int showChange = 1;
float limitDeg = 10;
char objPath[128];
char mtlPath[128];
char pcdPath[128];
char l63Path[128];
char lasPath[128];
char imageName[64];
char csid[5];
int pictureCnt = 0;
int lidarW = 1024;
int lidarH = 512;
int readMaxX,readMaxY,readMaxZ;
int readMinX,readMinY,readMinZ;
int readCount;

int malloc_save3D_buf() {
	int i, j, k;
	//float save3D[1024][6][2048][1];	//48M
	save3D = (float ****)malloc(sizeof(float***) * 1024);
	if(save3D == NULL) goto error;
	for(i = 0; i < 1024; i++) {
		save3D[i] = (float ***)malloc(sizeof(float**) * 6);
		if(save3D[i] == NULL) goto error;
		for(j = 0; j < 6; j++) {
			save3D[i][j] = (float **)malloc(sizeof(float*) * 2048);
			if(save3D[i][j] == NULL) goto error;
			for(k = 0; k < 2048; k++) {
				save3D[i][j][k] = (float *)malloc(sizeof(float) * 1);
				if(save3D[i][j][k] == NULL) goto error;
			}
		}
	}
	return 0;
error:
	db_error("malloc_save3D_buf() malloc error!");
	return -1;
}

void free_save3D_buf() {
	int i, j, k;
	if(save3D != NULL) {
		for(i = 0; i < 1024; i++) {
			if(save3D[i] != NULL) {
				for(j = 0; j < 6; j++) {
					if(save3D[i][j] != NULL) {
						for(k = 0; k < 2048; k++) {
							if(save3D[i][j][k] != NULL)
								free(save3D[i][j][k]);
						}
						free(save3D[i][j]);
					}
				}
				free(save3D[i]);
			}
		}
		free(save3D);
	}
	save3D = NULL;
}

int malloc_save3DSer_buf() {
	int i, j;
	//int save3DSer[1024][2048][1];		//8M
	save3DSer = (int ***)malloc(sizeof(int**) * 1024);
	if(save3DSer == NULL) goto error;
	for(i = 0; i < 1024; i++) {
		save3DSer[i] = (int **)malloc(sizeof(int*) * 2048);
		if(save3DSer[i] == NULL) goto error;
		for(j = 0; j < 2048; j++) {
			save3DSer[i][j] = (int *)malloc(sizeof(int) * 1);
			if(save3DSer[i][j] == NULL) goto error;
		}
	}
	return 0;
error:
	db_error("malloc_save3DSer_buf() malloc error!");
	return -1;
}

void free_save3DSer_buf() {
	int i, j;
	if(save3DSer != NULL) {
		for(i = 0; i < 1024; i++) {
			if(save3DSer[i] != NULL) {
				for(j = 0; j < 2048; j++) {
					if(save3DSer[i][j] != NULL)
						free(save3DSer[i][j]);
				}
				free(save3DSer[i]);
			}
		}
		free(save3DSer);
	}
	save3DSer = NULL;
}

int malloc_triData_buf() {		//120M過大
	int i, j;
	//float triData[0x200000][15][1];	//120M
	triData = (float ***)malloc(sizeof(float**) * 0x200000);
	if(triData == NULL) goto error;
	for(i = 0; i < 0x200000; i++) {
		triData[i] = (float **)malloc(sizeof(float*) * 15);
		if(triData[i] == NULL) goto error;
		for(j = 0; j < 15; j++) {
			triData[i][j] = (float *)malloc(sizeof(float) * 1);
			if(triData[i][j] == NULL) goto error;
		}
	}
	return 0;
error:
	db_error("malloc_triData_buf() malloc error!");
	return -1;
}

void free_triData_buf() {
	int i, j;
	if(triData != NULL) {
		for(i = 0; i < 0x200000; i++) {
			if(triData[i] != NULL) {
				for(j = 0; j < 15; j++) {
					if(triData[i][j] != NULL)
						free(triData[i][j]);
				}
				free(triData[i]);
			}
		}
		free(triData);
	}
	triData = NULL;
}

int malloc_triSub_buf() {
	int i, j;
	//float triSub[0x200000][10][1];	//80M
	triSub = (float ***)malloc(sizeof(float**) * 0x200000);
	if(triSub == NULL) goto error;
	for(i = 0; i < 0x200000; i++) {
		triSub[i] = (float **)malloc(sizeof(float*) * 10);
		if(triSub[i] == NULL) goto error;
		for(j = 0; j < 10; j++) {
			triSub[i][j] = (float *)malloc(sizeof(float) * 1);
			if(triSub[i][j] == NULL) goto error;
		}
	}
	return 0;
error:
	db_error("malloc_triSub_buf() malloc error!");
	return -1;
}

void free_triSub_buf() {
	int i, j;
	if(triSub != NULL) {
		for(i = 0; i < 0x200000; i++) {
			if(triSub[i] != NULL) {
				for(j = 0; j < 10; j++) {
					if(triSub[i][j] != NULL)
						free(triSub[i][j]);
				}
				free(triSub[i]);
			}
		}
		free(triSub);
	}
	triSub = NULL;
}

int malloc_lidarSinCosData_buf() {
	//int lidarSinCosData[0x200000];			// 8MB
	lidarSinCosData = (int *)malloc(sizeof(int) * 0x200000);
	if(lidarSinCosData == NULL) goto error;
	lidarDistanceData = lidarSinCosData;
	return 0;
error:
	db_error("malloc_lidarSinCosData_buf() malloc error!");
	return -1;
}

void free_lidarSinCosData_buf() {
	if(lidarSinCosData != NULL)
		free(lidarSinCosData);
	lidarSinCosData = NULL;
}

void free_lidar_buf() {
	free_save3D_buf();
	free_save3DSer_buf();
	free_triData_buf();
	free_triSub_buf();
	free_lidarSinCosData_buf();
}

/*
 * 直接宣告一塊大記憶體, 程式會跳掉,
 * 所以改為動態配置記憶體
 */
int malloc_lidar_buf() {
	if(malloc_save3D_buf() < 0)
		goto error;

	if(malloc_save3DSer_buf() < 0)
		goto error;

	if(malloc_triData_buf() < 0)
		goto error;

	if(malloc_triSub_buf() < 0)
		goto error;

	if(malloc_lidarSinCosData_buf() < 0)
		goto error;
	return 0;
error:
	db_error("malloc_lidar_buf() malloc error!");
	free_lidar_buf();
	return -1;
}

int max(int a,int b){
	return (a > b ? a: b);
}
int min(int a,int b){
	return (a < b ? a: b);
}
float getAbs(float a){
	if(a >= 0){
		return a;
	}else{
		return a * -1.0;
	}
}
float getTanAngel(float x1,float y1){
	float re = 0;
	if(x1 == 0) return 90.0;
	re = atan(y1 / x1) * 180.0 / PI;
	return re;
}

float getOrgDeg(int r,float x1,float y1,float z1,float x2,float y2,float z2,float x3,float y3,float z3){
	float re = 0;
	float a = (y2 - y1) * (z3 - z1) - (y3 - y1 ) * (z2 - z1);
	float b = (z2 - z1) * (x3 - x1) - (z3 - z1 ) * (x2 - x1);
	float c = (x2 - x1) * (y3 - y1) - (x3 - x1 ) * (y2 - y1);
	float cx = ((x1 + x2 + x3) / 3) - nummx[r];
	float cy = ((y1 + y2 + y3) / 3) - nummy[r];
	float cz = ((z1 + z2 + z3) / 3);
	float vectorCos = (a * cx + b * cy + c * cz) / ( sqrt(a * a + b * b + c * c) * sqrt(cx * cx + cy * cy + cz * cz));
	re = acos(vectorCos) * 180.0 / PI;
	return re;
}

double get2Dist(float x1,float y1,float x2,float y2){
	 float t1,t2,dist;
	 t1 = x1 - x2;
	 t2 = y1 - y2;
	 dist = sqrt(t1 * t1 + t2 * t2);
	 return dist;
}

//已知3點座標，求平面ax+by+cz+d=0;
float pa = 0,pb = 0,pc = 0,pd = 0;
void get_panel(float p1_x,float p1_y,float p1_z,
float p2_x,float p2_y,float p2_z,
float p3_x,float p3_y,float p3_z)
{
	pa = ( (p2_y - p1_y) * (p3_z - p1_z) - (p2_z - p1_z) * (p3_y - p1_y) );
	pb = ( (p2_z - p1_z) * (p3_x - p1_x) - (p2_x - p1_x) * (p3_z - p1_z) );
	pc = ( (p2_x - p1_x) * (p3_y - p1_y) - (p2_y - p1_y) * (p3_x - p1_x) );
	pd = ( 0-(pa * p1_x + pb * p1_y + pc * p1_z) );
}

float dis_pt2panel(float pt_x,float pt_y,float pt_z,float a,float b,float c,float d){

 return getAbs(a*pt_x+b*pt_y+c*pt_z+d)/sqrt(a*a+b*b+c*c);

}

float line_p1[3],line_p2[3],line_t[3];
float lineRange = 0.01;
int isLine() {
	float cx = (line_p1[0] + line_p2[0]) / 2.0;
	float cy = (line_p1[1] + line_p2[1]) / 2.0;
	float cz = (line_p1[2] + line_p2[2]) / 2.0;
	float bx = line_t[0] - cx;
	float by = line_t[1] - cy;
	float bz = line_t[2] - cz;
	float dist = sqrt(bx * bx + by * by + bz * bz);
	if(dist < lineRange){
		return 1;
	}else{
		return -1;
	}
}

/*
void readDataFile(int num,int cnt, float *data) {

	int y = 0;
	char fvalue[10];
	float deg = 0;
	float dist = 0;
	listcnt = 0;
	int maxDist = 0;
	totoalPoint = 0;
	int dataNum = cnt;
	if(dataNum > 1000000) dataNum = 1000000;

	for(y = 0; y < dataNum; y++){
		int cc = y * 2;
		deg = *(data + cc);
		dist = *(data + cc + 1);
		pointValue[listcnt][0] = deg;
		pointValue[listcnt][1] = dist;
		int sx = (dist * sin(deg * PI / 180) / 10 );
		int sy = -1 * (dist * cos(deg * PI / 180) / 10 );
		saveValue[listcnt][0][num] = sx;
		saveValue[listcnt][1][num] = sy;
		saveValue[listcnt][2][num] = deg;
		if(dist > maxDist) maxDist = dist;
		listcnt++;
	}
	saveCnt[num] = listcnt;
}
*/

void theEndtoStart() {
	int r,i,c;
	for(r = 0; r < reads; r++){
		int last = endNum[r];
		int first = startNum[r];
		for(i = 0; i < save3DCnt[first][r]; i++){
			int loc = save3DCnt[first][r] - i - 1;
			for(c = 0 ; c < 6; c++){
				float val = save3D[i][c][first][r];
				if(c == 0) val = 360 - val;
				save3D[loc][c][last][r] = val;
			}
			//save3DSqlit[loc][last][r] = save3DSqlit[i][first][r];
			save3DSer[loc][last][r] = save3DSer[i][first][r];
		}
		save3DCnt[last][r] = save3DCnt[first][r];
	}
}


int findNearPoint(float tx,float ty,int outR) {
	int k;
	float dist,minDist;
	int minR;
	minDist = get2Dist(tx,ty,nummx[0],nummy[0]);
	minR = 0;
	if(outR == 0) minDist = minDist - 10;
	for(k = 1; k < reads; k++){
		dist = get2Dist(tx,ty,nummx[k],nummy[k]);
		if(outR == k) dist = dist - 10;
		if(dist < minDist){
			minDist = dist;
			minR = k;
		}
	}
	return minR;
}

float dax[20000][4];
float record[50000][3];
float data[2000][2000][3];
void sort3Datas() {
	int r,i,j,k,c,q;
	/*
	for(r = 0; r < reads; r++){
		float t[10];
		for(i = startNum[r]; i < endNum[r]; i++){
			for(j = 0; j < save3DCnt[i][r] - 1; j++){
				for(k = 0; k < save3DCnt[i][r] - 1; k++){
					if(save3D[k + 1][0][i][r] < save3D[k][0][i][r]){
						for(c = 0 ; c < 6; c++){
							t[c] = save3D[k][c][i][r];
							save3D[k][c][i][r] = save3D[k + 1][c][i][r];
							save3D[k + 1][c][i][r] = t[c];
						}
					}
				}
			}
		}
	}
	*/

	int minR;
	float tx,ty;
	float dist,minDist;
	for(r = 0; r < reads; r++){
		minDist = get2Dist(nummx[r],nummy[r],nummx[0],nummy[0]);
		minR = 0;
		if(r == 0 && reads > 1){
			minDist = get2Dist(nummx[r],nummy[r],nummx[1],nummy[1]);
			minR = 1;
		}
		for(i = 0; i < reads; i++){
			if(i == r) continue;
			dist = get2Dist(nummx[r],nummy[r],nummx[i],nummy[i]);
			if(dist < minDist){
				minDist = dist;
				minR = i;
			}
		}
		nearOtherNum[r] = minR;
	}

	/*
	//bottomLen
	for(r = 0; r < reads; r++){
		float bottomData[500],tot = 0;
		int bottomCnt = 0;
		for(j = 0; j < 7; j++){
			for(i = startNum[r] + 1; i < endNum[r] - 1; i++){
				bottomData[bottomCnt] = save3D[j][4][i][r];
				bottomData[bottomCnt + 1] = save3D[save3DCnt[i][r] - 1 - j][4][i][r];
				bottomCnt += 2;
				if(bottomCnt >= 500){
					break;
				}
			}
			if(bottomCnt >= 500){
				break;
			}
		}
		for(i = 0; i < bottomCnt - 1; i++){
			for(j = 0; j < bottomCnt - 1; j++){
				if(bottomData[j] > bottomData[j + 1]){
					float tmp;
					tmp = bottomData[j];
					bottomData[j] = bottomData[j + 1];
					bottomData[j + 1] = tmp;
				}
			}
		}
		for(i = 0; i < 100; i++) tot += bottomData[i];
		bottomLen[r] = tot / 100.0;
	}*/


	float tdata[5];
	float pdata[5][4];
	int inext,ibef;
	float likeDeg = 0.4,minDeg,deg;
	int minNum;

	/*
	int themax1[30][2];
	int themax2[30][2];
	int q1 = 0;
	int qval = 0;
	for(q = 0; q < 2; q++){
		for(r = 0; r < reads; r++){
			for(i = 0; i < 1200; i++){
				projection[i][r] = 0;
				if(i < 30 && r == 0){
					themax1[i][0] = 0;
					themax1[i][1] = 0;
				}
				if(i < 30 && r == 1){
					themax2[i][0] = 0;
					themax2[i][1] = 0;
				}
			}
			for(i = startNum[r]; i < endNum[r]; i++){
				for(j = 0; j < save3DCnt[i][r]; j++){
					float dist = save3D[j][1][i][r] / 100;
					float base = 1;
					int x;
					if(q == 0){
						x = save3D[j][2][i][r];
					}else{
						x = save3D[j][3][i][r];
					}
					int z = save3D[j][4][i][r];
					if(z > 120 || z < -120) continue;
					if(x > 1199) x = 1199;
					if(x < -1200) x = -1200;
					x = x + 1200;
					int val = base * (dist * dist);
					projection[x][r] += val;
				}
			}
			q1 = 0;
			qval = 0;
			for(i = 0; i < 2400; i++){
				if(projection[i][r] > qval){
					int m = projection[i][r];
					if(r == 0){
						themax1[q1][0] = m;
						themax1[q1][1] = i;
						qval = m;
						for(j = 0; j < 30; j++){
							if(themax1[j][0] < qval){
								qval = themax1[j][0];
								q1 = j;
							}
						}
					}else{
						themax2[q1][0] = m;
						themax2[q1][1] = i;
						qval = m;
						for(j = 0; j < 30; j++){
							if(themax2[j][0] < qval){
								qval = themax2[j][0];
								q1 = j;
							}
						}
					}
				}
			}
		}
		int defp[1500];
		int mindp = 999999999, minVal = 0;;
		for(i = 0; i < 1000; i++){
			int mx = i - 500;
			int dex = 0;
			for(j = 0; j < 1400; j++){
				int mx2 = j + 500;
				int mx3 = mx2 + mx;
				int d = getAbs(projection[mx2][0] - projection[mx3][1]);
				dex += d;
			}
			defp[i] = dex;
			if(mindp > dex){
				minVal = mx;
				mindp = dex;
			}
		}
		if(q == 0){
			nummx[1] = nummx[0] - minVal;
		}else{
			nummy[1] = nummy[0] - minVal;
		}
	}*/

	/*
	int skip = 0;
	for(r = 0; r < reads; r++){
		for(i = startNum[r]; i < endNum[r]; i++){
			int bef = 0;
			inext = i + 1;
			ibef = i - 1;
			for(j = 0; j < save3DCnt[i][r]; j++){
				save3DSqlit[j][i][r] = 0;
			}
			if(ibef < startNum[r] || inext >= endNum[r]){
				continue;
			}

			for(j = 1; j < save3DCnt[i][r] - 1; j++){
				tdata[0] = save3D[j][0][i][r];
				tdata[2] = save3D[j][2][i][r];
				tdata[3] = save3D[j][3][i][r];
				tdata[4] = save3D[j][4][i][r];

				minNum = 0;
				minDeg = getAbs(tdata[0] - save3D[0][0][ibef][r]);
				for(k = 1; k < save3DCnt[ibef][r]; k++){
					deg = getAbs(tdata[0] - save3D[k][0][ibef][r]);
					if(deg < minDeg){
						minDeg = deg;
						minNum = k;
					}
				}
				if(minDeg <= likeDeg){
					pdata[0][0] = save3D[minNum][0][ibef][r];
					pdata[2][0] = save3D[minNum][2][ibef][r];
					pdata[3][0] = save3D[minNum][3][ibef][r];
					pdata[4][0] = save3D[minNum][4][ibef][r];
				}else{
					continue;
				}
				minNum = 0;
				minDeg = getAbs(tdata[0] - save3D[0][0][inext][r]);
				for(k = 1; k < save3DCnt[inext][r]; k++){
					deg = getAbs(tdata[0] - save3D[k][0][inext][r]);
					if(deg < minDeg){
						minDeg = deg;
						minNum = k;
					}
				}
				if(minDeg <= likeDeg){
					pdata[0][1] = save3D[minNum][0][inext][r];
					pdata[2][1] = save3D[minNum][2][inext][r];
					pdata[3][1] = save3D[minNum][3][inext][r];
					pdata[4][1] = save3D[minNum][4][inext][r];
				}else{
					continue;
				}

				pdata[0][2] = save3D[j - 1][0][i][r];
				pdata[2][2] = save3D[j - 1][2][i][r];
				pdata[3][2] = save3D[j - 1][3][i][r];
				pdata[4][2] = save3D[j - 1][4][i][r];
				pdata[0][3] = save3D[j + 1][0][i][r];
				pdata[2][3] = save3D[j + 1][2][i][r];
				pdata[3][3] = save3D[j + 1][3][i][r];
				pdata[4][3] = save3D[j + 1][4][i][r];

				line_t[0] = tdata[2];
				line_t[1] = tdata[3];
				line_t[2] = tdata[4];
				line_p1[0] = pdata[2][0];
				line_p1[1] = pdata[3][0];
				line_p1[2] = pdata[4][0];
				line_p2[0] = pdata[2][1];
				line_p2[1] = pdata[3][1];
				line_p2[2] = pdata[4][1];
				if(isLine() == 1){
					line_p1[0] = pdata[2][2];
					line_p1[1] = pdata[3][2];
					line_p1[2] = pdata[4][2];
					line_p2[0] = pdata[2][3];
					line_p2[1] = pdata[3][3];
					line_p2[2] = pdata[4][3];
					if(isLine() == 1){
						save3DSqlit[j][i][r] = -1;
						skip++;
					}
				}
			}
		}
	}

	for(r = 0; r < reads; r++){
		for(i = startNum[r]; i < endNum[r]; i++){
			int cnt = 1;
			for(j = 1; j < save3DCnt[i][r]; j++){
				int state = save3DSqlit[j][i][r];
				if(state == 0){
					for(c = 0 ; c < 6; c++){
						save3D[cnt][c][i][r] = save3D[j][c][i][r];
					}
					cnt++;
				}
			}
			save3DCnt[i][r] = cnt;
		}
	}
	*/
	//theEndtoStart();

}

void read3Datas() {
	int r,i,j,k,h;
	int i1,i2,i3,tmp,mode;
	float norDeg,planeDeg,tx,ty,tz,td,totDeg,el;
	float nowHor,befHor[2],horx[2],hory[2];
	int horCnt;
	int totScan;
	float minPlane,plane,perDegH;
	SerialNumber = 1;
	int nCnt = 0;
	int nData = 0;
	readMaxX = -99999;
	readMaxY = -99999;
	readMaxZ = -99999;
	readMinX = 99999;
	readMinY = 99999;
	readMinZ = 99999;
	readCount = 0;

	for(r = 0; r < reads; r++){
		horCnt = 0;
		totDeg = 0;
		minPlane = 130000;
		startNum[r] = 0;
		endNum[r] = lidarW;
		totScan = endNum[r] - startNum[r];
		avgDeg[r] = 360.0 / totScan;
		perDegH = 180.0 / lidarH;

		for(i = startNum[r]; i < endNum[r]; i++){

			float deg = 0;
			float dist = 0;
			float inte = 0;
			listcnt = 0;
			befHor[0] = 360;
			befHor[1] = 360;
			int maxDist = 0;

			for(j = 0; j < lidarH; j++){

				k = j + i * lidarH;
				deg = perDegH * j;
				int rx = (  (unsigned int)(*(lidarBuf + k)) & 0xFFFF0000) >> 16;
				dist = rx;
				rx = ( (unsigned int)(*(lidarBuf + k)) & 0x0000FFFF);
				inte = rx;

				if(deg < 90){norDeg = 90 - deg; planeDeg = 180.0 + totDeg; el = -1.0;}
				else if(deg < 180){norDeg = -90 + deg; planeDeg = 180.0 + totDeg; el = 1.0;}
				else if(deg < 270){norDeg = 270 - deg; planeDeg = totDeg; el = 1.0;}
				else{norDeg = -270 + deg; planeDeg = totDeg; el = -1.0;}

				tz = dist * sin(norDeg * PI / 180) * el;
				td = dist * cos(norDeg * PI / 180);
				tx = td * cos(planeDeg * PI / 180);
				ty = td * sin(planeDeg * PI / 180) * -1.0;
				//if(listcnt > 300) maxDist = maxDist + 1;
				save3D[listcnt][0][i][r] = deg;
				save3D[listcnt][1][i][r] = dist;
				save3D[listcnt][2][i][r] = tx;
				save3D[listcnt][3][i][r] = ty;
				save3D[listcnt][4][i][r] = tz;
				save3D[listcnt][5][i][r] = inte;
				save3DSer[listcnt][i][r] = SerialNumber;
				SerialNumber++;
				if(listcnt == 0){
					db_debug("Lidar first data: %.2f, %.2f, %.2f, %.2f, %.2f %.2f\n", deg, dist, tx, ty ,tz, inte);
				}
				//db_debug("model 3ddata: %.2f, %.2f, %.2f, %.2f, %.2f\n", deg, dist, tx, ty ,tz);
				if(dist > maxDist) maxDist = dist;

				plane = sqrt(tx * tx + ty * ty);
				if(plane < minPlane && tz < -100){
					minPlane = plane;
				}
				nowHor = getAbs(90 - deg);
				if(nowHor < 1.2){
					if(nowHor < befHor[0]){
						befHor[0] = nowHor;
						horx[0] = tx;
						hory[0] = ty;
					}
				}
				nowHor = getAbs(270 - deg);
				if(nowHor < 1.2){
					if(nowHor < befHor[1]){
						befHor[1] = nowHor;
						horx[1] = tx;
						hory[1] = ty;
					}
				}
				listcnt++;
				nData += 2;

				if(tx > readMaxX) readMaxX = (int)tx;
				if(ty > readMaxY) readMaxY = (int)ty;
				if(tz > readMaxZ) readMaxZ = (int)tz;
				if(tx < readMinX) readMinX = (int)tx;
				if(ty < readMinY) readMinY = (int)ty;
				if(tz < readMinZ) readMinZ = (int)tz;
				readCount++;
			}

			for(h = 0; h < 2; h++){
				if(befHor[h] < 1.2){
					saveValue[horCnt][0][r] = horx[h];
					saveValue[horCnt][1][r] = hory[h];
					horCnt++;
				}
			}
			save3DCnt[i][r] = listcnt;
			totDeg += avgDeg[r];
			nCnt++;
		}
		saveCnt[r] = horCnt;
		nearOtherDist[r] = minPlane;
	}
}

//---------------------------------------------------------------------------

float seeHorDeg(float sx, float sy,float tx,float ty) {
	float a = getAbs(sy - ty);
	float b = getAbs(sx - tx);
	float val = 90;
	if(b != 0){
		val = atan( a / b ) * 180.0f / PI;
	}
	if(tx >= sx && ty >= sy){
		//return val;
	}else if(tx <= sx && ty >= sy){
		val = 180.0 - val;
	}else if(tx <= sx && ty <= sy){
		val = 180.0 + val;
	}else{
		val = 360 - val;
	}
	val = val + 180.0;
	if(val > 360.0) val = val - 360.0;

	return val;
}
float seeVerDeg(float sx, float sy,float sz,float tx,float ty,float tz) {
	float a = getAbs(sy - ty);
	float b = getAbs(sx - tx);
	float c = getAbs(sz - tz);
	float dist = sqrt(a * a + b * b);
	if(dist == 0){
		if(tz >= sz){
			return 90;
		}else{
        	return -90;
		}
	}
	float val = atan( c / dist ) * 180.0f / PI;
	if(tz <= sz){
		return -1 * val;
	}else{
		return val;
	}
}
/*
int findPointSer(int ser) {
	int i;
	bool isFind = false;
	for(i = 0; i < pointCnt; i++){
		if(pointSer[i] == ser){
			isFind = true;
			break;
		}
	}
	if(i == 10746){
		isFind = true;
	}
	return i + 1;
}
*/
void write3Ddata2OBJ() {
	int r,i,j,k,q;
	float t1,t2,t3,t4,t5;
	float wp1[3],wp2[3],wp3[3];
	float vp1[2],vp2[2],vp3[2];
	float nDeg,nDist,nx,ny,nz,nDupl;
	float cDeg,cDist,cx,cy,cz;
	float dist;
	int nStatus,cStatus;
	bool isDelete;
	float maxDupl = 10000,minDupl = 100,rangeDupl = 4000;

	for(r = 0; r < reads; r++){

		for(i = 0; i < triCnt[r]; i++){
			nStatus = triSub[i][0][r];
			nDeg = triSub[i][1][r];
			nDist = triSub[i][2][r];
			nx = triSub[i][3][r];
			ny = triSub[i][4][r];
			nz = triSub[i][5][r];
			isDelete = false;
			if(nStatus < 0 || nStatus > 1){
				continue;
			}
			if(nDist > maxDupl) t1 = maxDupl;
			else if(nDist < minDupl) t1 = minDupl;
			else t1 = nDist;
			nDupl = (1 - (maxDupl - t1) / (maxDupl - minDupl)) * rangeDupl;
			for(q = 0; q < reads; q++){
				if(q == r) continue;
				for(j = 0; j < triCnt[q]; j++){
					cStatus = triSub[j][0][q];
					cDeg = triSub[j][1][q];
					if(cStatus < 0) continue;
					if(getAbs(cDeg - nDeg) > 45) continue;
					cDist = triSub[j][2][q];
					cx = triSub[j][3][q];
					cy = triSub[j][4][q];
					cz = triSub[j][5][q];
					dist = get2Dist(nx,ny,cx,cy);
					if(dist < nDupl){
						if(nDist > cDist){
							isDelete = true;
							break;
						}else{
							if(cStatus < 1){
								triSub[j][0][q] = -1;
							}
						}
					}
				}
				if(isDelete){
					break;
				}
			}
			//if(nz > 102){
			//	isDelete = true;
			//}
			if(isDelete){
				//db_debug("model_3d tri del : %d\n",i);
				triSub[i][0][r] = -1;
			}else{
				//db_debug("model_3d tri show : %d\n",i);
				triSub[i][0][r] = 2;
			}
		}
	}
	/*
	for(r = 0;r < reads; r++){
		for(i = 0;i < triCnt[r]; i++){
			nStatus = triSub[i][0][r];
			if(nStatus > 1){
				for(j = 0; j < 3; j++){
					int ser = triSub[i][6 + j][r];
					bool isFind = false;
					for(k = 0; k < pointCnt; k++){
						if(pointSer[k] == ser){
							isFind = true;
							break;
						}
					}
					if(!isFind){
						int w1 = j * 3;
						int w2 = 9 + j * 2;
						pointList[pointCnt][0] = triData[i][w1][r] * -1.0f;
						pointList[pointCnt][1] = triData[i][w1 + 1][r];
						pointList[pointCnt][2] = triData[i][w1 + 2][r];
						pointList[pointCnt][3] = triData[i][w2][r];
						pointList[pointCnt][4] = 1.0f - triData[i][w2 + 1][r];
						pointList[pointCnt][5] = r;
						pointSer[pointCnt] = ser;
						pointCnt++;
					}
				}
			}
		}
	}
	*/
	FILE *fp;
	FILE *vp;
	fp = fopen(objPath,"w");
	vp = fopen(mtlPath,"w");
	fprintf(fp,"# tritest OBJ data\n\n");
	fprintf(vp,"# tritest MYL data\n\n");
	fprintf(fp,"mtllib ./triShow.mtl\n\n");

	float max;
	int tr;
	int perce = 0;
	for(i = 0; i < 3; i++){
		pointCnt = 0;
		for(j = 0; j < triCnt[0]; j++){
			int pe = j * 100 / triCnt[0];
			if(pe % 5 == 0 && perce != pe){
				perce = pe;
				db_debug("Lidar obj write %d %d/100\n",i,pe);
			}
			nStatus = triSub[j][0][0];
			if(nStatus < 1){
				continue;
			}
			pointCnt += 3;
			for(k = 0; k < 3;k++){
				int w1 = k * 3;
				int w2 = 9 + k * 2;
				switch(i){
					case 0:
						fprintf(fp,"v %.1f %.1f %.1f\n",
						triData[j][w1][0] * -1.0f,triData[j][w1 + 1][0],triData[j][w1 + 2][0]);
					break;
					case 1:
						t1 = triData[j][w1][0] * -1.0f;
						t2 = triData[j][w1 + 1][0];
						t3 = triData[j][w1 + 2][0];
						max = getAbs(t1);
						if(max < getAbs(t2)) max = getAbs(t2);
						if(max < getAbs(t3)) max = getAbs(t3);
						if(max == 0) max = 1;
						t1 = t1 / max;
						t2 = t2 / max;
						t3 = t3 / max;
						fprintf(fp,"vn %.4f %.4f %.4f\n",t1,t2,t3);
					break;
					case 2:
						fprintf(fp,"vt %.4f %.4f %.4f\n",
								triData[j][w2][0],1.0f - triData[j][w2 + 1][0],0.0f);
					break;
				}
			}
		}
		switch(i){
			case 0:
				fprintf(fp,"# %d vertices\n\n",pointCnt);
				db_debug("Lidar obj write v %d\n", pointCnt);
				break;
			case 1:
				fprintf(fp,"# %d normals\n\n",pointCnt);
				db_debug("Lidar obj write vn %d\n", pointCnt);
				break;
			case 2:
				fprintf(fp,"# %d texture coordinates\n\n",pointCnt);
				db_debug("Lidar obj write vt %d\n", pointCnt);
				break;
		}
	}
	/*
	for(i = 0; i < 3; i++){
		for(j = 0; j < pointCnt; j++){
			switch(i){
				case 0:
					fprintf(fp,"v %.4f %.4f %.4f\n",
					pointList[j][0],pointList[j][1],pointList[j][2]);
				break;
				case 1:
					tr = pointList[pointCnt][5];
					t1 = pointList[j][0] - nummx[tr];
					t2 = pointList[j][1] - nummy[tr];
					t3 = pointList[j][2];
					max = getAbs(t1);
					if(max < getAbs(t2)) max = getAbs(t2);
					if(max < getAbs(t3)) max = getAbs(t3);
					if(max == 0) max = 1;
					t1 = t1 / max;
					t2 = t2 / max;
					t3 = t3 / max;
					fprintf(fp,"vn %.4f %.4f %.4f\n",t1,t2,t3);
				break;
				case 2:
					fprintf(fp,"vt %.8f %.8f %.8f\n",
					pointList[j][3],pointList[j][4],0.0f);
				break;
			}
		}
		switch(i){
			case 0:
				fprintf(fp,"# %d vertices\n\n",pointCnt);
				break;
			case 1:
				fprintf(fp,"# %d normals\n\n",pointCnt);
				break;
			case 2:
				fprintf(fp,"# %d texture coordinates\n\n",pointCnt);
				break;
		}
	}
	*/
	for(r = 0;r < reads;r++){
		fprintf(fp,"o gro%d\n",r);
		fprintf(fp,"usemtl img.%d\n",r);

		fprintf(vp,"newmtl img.%d\n",r);
		fprintf(vp,"Ka 1 1 1\n");
		fprintf(vp,"Kd 0.8 0.8 0.8\n");
		fprintf(vp,"map_Kd %s\n",imageName);
		fprintf(vp,"Ks 1 1 1\n");
		fprintf(vp,"Ns 50\n");
		fprintf(vp,"illum 7\n\n");


		int tot = 1;
		for(i = 0;i < triCnt[r]; i++){
			nStatus = triSub[i][0][r];
			if(nStatus > 1){
				//int t1 = findPointSer(triSub[i][6][r]);
				//int t2 = findPointSer(triSub[i][7][r]);
				//int t3 = findPointSer(triSub[i][8][r]);
				fprintf(fp,"f %d/%d/%d %d/%d/%d %d/%d/%d\n",
						tot + 1, tot + 1, tot + 1,
						tot,tot,tot,
						tot + 2, tot + 2, tot + 2);
				tot += 3;
			}
		}

		fprintf(fp,"\n");
	}

	fclose(fp);
	fclose(vp);
	db_debug("Lidar create MTL File : %s\n",mtlPath);
	db_debug("Lidar create OBJ File : %s\n",objPath);
}

void createTriData() {
	int r,i,j,q;
	float deg1,deg2,deg3;
	int triTot = 0;
	db_debug("model_3d create ck1\n");
	for(r = 0; r < reads; r++){
		float wp1[3],wp2[3],wp3[3];
		float vp1[2],vp2[2],vp3[2];

		int triNum = 0;
		int inext = 0,isR = 0;
		int cnt1,cnt2;
		int pnt[3][2];
		float pntDeg[3],tDeg1,tDeg2;
		int s1,s2,s3;
		int p[4];
		int cc = 0;
		float t1,t2,t3,t4,t5,t6;
		float v1,v2,v3,v4;
		for(i = startNum[r]; i < endNum[r]; i++){
			inext = i + 1;
			if(i % 100 == 0){
				int perce = i * 100 / lidarW;
				db_debug("Lidar create obj data %d\n",perce);
			}
			if(inext == endNum[r]){
				inext = 0;
			}

			bool isFinish = false,isMyArea1,isMyArea2;
			bool isEnd1 = false,isEnd2 = false,isFirst = false;
			int spCnt = 1;
			cnt1 = 0;
			cnt2 = 0;
			while(!isFinish){
				isEnd1 = false;
				isEnd2 = false;
				pnt[0][0] = cnt1;
				pnt[0][1] = 0;
				pntDeg[0] = save3D[cnt1][0][i][r];
				pnt[2][0] = cnt2;
				pnt[2][1] = 1;
				pntDeg[2] = save3D[cnt2][0][inext][r];
				cnt1++;
				cnt2++;
				isFirst = true;
				while(!isEnd1 || !isEnd2){
					if(cnt1 < save3DCnt[i][r] && !isEnd1){
						tDeg1 = save3D[cnt1][0][i][r];
						if(tDeg1 > 180.0){
							isEnd1 = true;
							tDeg1 = 370;
						}
					}else{
						isEnd1 = true;
						tDeg1 = 370;
					}
					if(cnt2 < save3DCnt[inext][r] && !isEnd2){
						tDeg2 = save3D[cnt2][0][inext][r];
						if(tDeg2 > 180.0){
							isEnd2 = true;
							tDeg2 = 370;
						}
					}else{
						isEnd2 = true;
						tDeg2 = 370;
					}
					if(isEnd1 && isEnd2){
						break;
					}
					if(isFirst){
						isFirst = false;
					}else{
						if(isR == 0){
							pnt[0][0] = pnt[1][0];
							pnt[0][1] = pnt[1][1];
							pntDeg[0] = pntDeg[1];
						}else{
							pnt[2][0] = pnt[1][0];
							pnt[2][1] = pnt[1][1];
							pntDeg[2] = pntDeg[1];
						}
					}
					if(tDeg1 < tDeg2){
						pnt[1][0] = cnt1;
						pnt[1][1] = 0;
						pntDeg[1] = tDeg1;
						isR = 0;
						cnt1++;
					}else{
						pnt[1][0] = cnt2;
						pnt[1][1] = 1;
						pntDeg[1] = tDeg2;
						isR = 1;
						cnt2++;
					}
					if(pnt[0][1] == 0){s2 = i;}else{s2 = inext;}
					s1 = pnt[0][0];
					v1 = 1.0 - (s2 - startNum[r]) * avgDeg[r] / 360.0;
					v2 = (180.0 - pntDeg[0]) / 180.0;
					wp1[0] = save3D[s1][2][s2][r] + nummx[r];
					wp1[1] = save3D[s1][3][s2][r] + nummy[r];
					wp1[2] = save3D[s1][4][s2][r];
					p[1]   = save3DSer[s1][s2][r];
					vp1[0] = v1;
					vp1[1] = v2;
					if(pnt[1][1] == 0){s2 = i;}else{s2 = inext;}
					s1 = pnt[1][0];
					v1 = 1.0 - (s2 - startNum[r]) * avgDeg[r] / 360.0;
					v2 = (180.0 - pntDeg[1]) / 180.0;
					wp2[0] = save3D[s1][2][s2][r] + nummx[r];
					wp2[1] = save3D[s1][3][s2][r] + nummy[r];
					wp2[2] = save3D[s1][4][s2][r];
					p[2]   = save3DSer[s1][s2][r];
					vp2[0] = v1;
					vp2[1] = v2;
					if(pnt[2][1] == 0){s2 = i;}else{s2 = inext;}
					s1 = pnt[2][0];
					v1 = 1.0 - (s2 - startNum[r]) * avgDeg[r] / 360.0;
					v2 = (180.0 - pntDeg[2]) / 180.0;
					wp3[0] = save3D[s1][2][s2][r] + nummx[r];
					wp3[1] = save3D[s1][3][s2][r] + nummy[r];
					wp3[2] = save3D[s1][4][s2][r];
					p[3]   = save3DSer[s1][s2][r];
					vp3[0] = v1;
					vp3[1] = v2;

					bool isDraw = true;
					if(isDraw){
						t1 = (wp1[0] + wp2[0] + wp3[0]) / 3;
						t2 = (wp1[1] + wp2[1] + wp3[1]) / 3;
						t3 = (wp1[2] + wp2[2] + wp3[2]) / 3;
						triTot++;
						t4 = seeHorDeg(nummx[r],nummy[r],t1,t2);
						t5 = get2Dist(nummx[r],nummy[r],t1,t2);
						triSub[triNum][0][r] = 0;
						triSub[triNum][1][r] = t4;
						triSub[triNum][2][r] = t5;
						triSub[triNum][3][r] = t1;
						triSub[triNum][4][r] = t2;
						triSub[triNum][5][r] = t3;

						t1 = getOrgDeg(r,wp1[0],wp1[1],wp1[2],wp2[0],wp2[1],wp2[2],wp3[0],wp3[1],wp3[2]);
						if(t1 < (limitDeg + 90) && t1 > (90 - limitDeg)){
							//db_debug("model_3d out see deg %.4f\n", t1);
							triSub[triNum][0][r] = -1;
						}
						//if(wp1[2] > 1200 || wp2[2] > 1200 || wp3[2] > 1200){
						//	triSub[triNum][0][r] = -1;
						//}

						if(showChange == 1){
							triData[triNum][0][r] = wp2[0];
							triData[triNum][1][r] = wp2[1];
							triData[triNum][2][r] = wp2[2];
							triData[triNum][3][r] = wp1[0];
							triData[triNum][4][r] = wp1[1];
							triData[triNum][5][r] = wp1[2];
							triData[triNum][6][r] = wp3[0];
							triData[triNum][7][r] = wp3[1];
							triData[triNum][8][r] = wp3[2];
							triData[triNum][9][r] = vp2[0];
							triData[triNum][10][r] = vp2[1];
							triData[triNum][11][r] = vp1[0];
							triData[triNum][12][r] = vp1[1];
							triData[triNum][13][r] = vp3[0];
							triData[triNum][14][r] = vp3[1];
							triSub[triNum][6][r] = p[2];
							triSub[triNum][7][r] = p[1];
							triSub[triNum][8][r] = p[3];

						}else{
							triData[triNum][0][r] = wp1[0];
							triData[triNum][1][r] = wp1[1];
							triData[triNum][2][r] = wp1[2];
							triData[triNum][3][r] = wp2[0];
							triData[triNum][4][r] = wp2[1];
							triData[triNum][5][r] = wp2[2];
							triData[triNum][6][r] = wp3[0];
							triData[triNum][7][r] = wp3[1];
							triData[triNum][8][r] = wp3[2];
							triData[triNum][9][r] = vp1[0];
							triData[triNum][10][r] = vp1[1];
							triData[triNum][11][r] = vp2[0];
							triData[triNum][12][r] = vp2[1];
							triData[triNum][13][r] = vp3[0];
							triData[triNum][14][r] = vp3[1];
							triSub[triNum][6][r] = p[1];
							triSub[triNum][7][r] = p[2];
							triSub[triNum][8][r] = p[3];
						}
						triNum++;
					}
				}
				break;
			}

			/*isFinish = false;
			while(!isFinish){
				isEnd1 = false;
				isEnd2 = false;
				pnt[0][0] = cnt1;
				pnt[0][1] = 0;
				pntDeg[0] = save3D[cnt1][0][i][r];
				pnt[2][0] = cnt2;
				pnt[2][1] = 1;
				pntDeg[2] = save3D[cnt2][0][inext][r];
				cnt1++;
				cnt2++;
				isFirst = true;
				while(!isEnd1 || !isEnd2){
					if(cnt1 < save3DCnt[i][r] && !isEnd1){
						tDeg1 = save3D[cnt1][0][i][r];
					}else{
						isEnd1 = true;
						tDeg1 = 370;
					}
					if(cnt2 < save3DCnt[inext][r] && !isEnd2){
						tDeg2 = save3D[cnt2][0][inext][r];
					}else{
						isEnd2 = true;
						tDeg2 = 370;
					}
					if(isEnd1 && isEnd2){
						break;
					}
					if(tDeg1 < tDeg2){
						pnt[1][0] = pnt[0][0];
						pnt[1][1] = pnt[0][1];
						pntDeg[1] = pntDeg[0];
						pnt[0][0] = cnt1;
						pnt[0][1] = 0;
						pntDeg[0] = tDeg1;
						isR = 0;
						cnt1++;
					}else{
						pnt[1][0] = pnt[2][0];
						pnt[1][1] = pnt[2][1];
						pntDeg[1] = pntDeg[2];
						pnt[2][0] = cnt2;
						pnt[2][1] = 1;
						pntDeg[2] = tDeg2;
						isR = 1;
						cnt2++;
					}
					if(pnt[0][1] == 0){s2 = i;}else{s2 = inext;}
					s1 = pnt[0][0];
					v1 = 1.0 - ((s2 - startNum[r]) * avgDeg[r] + 180.0) / 360.0;
					v2 = (pntDeg[0] - 180.0) / 180.0;
					wp1[0] = save3D[s1][2][s2][r] + nummx[r];
					wp1[1] = save3D[s1][3][s2][r] + nummy[r];
					wp1[2] = save3D[s1][4][s2][r];
					p[1]   = save3DSer[s1][s2][r];
					vp1[0] = v1;
					vp1[1] = v2;
					if(pnt[1][1] == 0){s2 = i;}else{s2 = inext;}
					s1 = pnt[1][0];
					v1 = 1.0 - ((s2 - startNum[r]) * avgDeg[r] + 180.0) / 360.0;
					v2 = (pntDeg[1] - 180.0) / 180.0;
					wp2[0] = save3D[s1][2][s2][r] + nummx[r];
					wp2[1] = save3D[s1][3][s2][r] + nummy[r];
					wp2[2] = save3D[s1][4][s2][r];
					p[2]   = save3DSer[s1][s2][r];
					vp2[0] = v1;
					vp2[1] = v2;
					if(pnt[2][1] == 0){s2 = i;}else{s2 = inext;}
					s1 = pnt[2][0];
					v1 = 1.0 - ((s2 - startNum[r]) * avgDeg[r] + 180.0) / 360.0;
					v2 = (pntDeg[2] - 180.0) / 180.0;
					wp3[0] = save3D[s1][2][s2][r] + nummx[r];
					wp3[1] = save3D[s1][3][s2][r] + nummy[r];
					wp3[2] = save3D[s1][4][s2][r];
					p[3]   = save3DSer[s1][s2][r];
					vp3[0] = v1;
					vp3[1] = v2;

					bool isDraw = true;
					if(isDraw){
						t1 = (wp1[0] + wp2[0] + wp3[0]) / 3;
						t2 = (wp1[1] + wp2[1] + wp3[1]) / 3;
						t3 = (wp1[2] + wp2[2] + wp3[2]) / 3;
						triTot++;
						t4 = seeHorDeg(nummx[r],nummy[r],t1,t2);
						t5 = get2Dist(nummx[r],nummy[r],t1,t2);
						triSub[triNum][0][r] = 0;
						triSub[triNum][1][r] = t4;
						triSub[triNum][2][r] = t5;
						triSub[triNum][3][r] = t1;
						triSub[triNum][4][r] = t2;
						triSub[triNum][5][r] = t3;

						t1 = getOrgDeg(r,wp1[0],wp1[1],wp1[2],wp2[0],wp2[1],wp2[2],wp3[0],wp3[1],wp3[2]);
						if(t1 < (limitDeg + 90) && t1 > (90 - limitDeg)){
							triSub[triNum][0][r] = -1;
						}
						if(wp1[2] > 120 || wp2[2] > 120 || wp3[2] > 120){
							triSub[triNum][0][r] = -1;
						}
						if(showChange == 1){
							triData[triNum][0][r] = wp2[0];
							triData[triNum][1][r] = wp2[1];
							triData[triNum][2][r] = wp2[2];
							triData[triNum][3][r] = wp1[0];
							triData[triNum][4][r] = wp1[1];
							triData[triNum][5][r] = wp1[2];
							triData[triNum][6][r] = wp3[0];
							triData[triNum][7][r] = wp3[1];
							triData[triNum][8][r] = wp3[2];
							triData[triNum][9][r] = vp2[0];
							triData[triNum][10][r] = vp2[1];
							triData[triNum][11][r] = vp1[0];
							triData[triNum][12][r] = vp1[1];
							triData[triNum][13][r] = vp3[0];
							triData[triNum][14][r] = vp3[1];
							triSub[triNum][6][r] = p[2];
							triSub[triNum][7][r] = p[1];
							triSub[triNum][8][r] = p[3];

						}else{
							triData[triNum][0][r] = wp1[0];
							triData[triNum][1][r] = wp1[1];
							triData[triNum][2][r] = wp1[2];
							triData[triNum][3][r] = wp2[0];
							triData[triNum][4][r] = wp2[1];
							triData[triNum][5][r] = wp2[2];
							triData[triNum][6][r] = wp3[0];
							triData[triNum][7][r] = wp3[1];
							triData[triNum][8][r] = wp3[2];
							triData[triNum][9][r] = vp1[0];
							triData[triNum][10][r] = vp1[1];
							triData[triNum][11][r] = vp2[0];
							triData[triNum][12][r] = vp2[1];
							triData[triNum][13][r] = vp3[0];
							triData[triNum][14][r] = vp3[1];
							triSub[triNum][6][r] = p[1];
							triSub[triNum][7][r] = p[2];
							triSub[triNum][8][r] = p[3];
						}
						triNum++;
					}

				}
				break;
			}*/

		}

		/*
		for(q = 0; q < reads; q++){
			if(q == r) continue;
			if(nearOtherNum[q] == r){
				float mx = nummx[q];
				float my = nummy[q];
				float mz = bottomLen[q];
				float sx = nummx[r];
				float sy = nummy[r];
				float mHor = seeHorDeg(sx,sy,mx,my);
				float mVer = seeVerDeg(sx,sy,0,mx,my,mz);
				float hor1,hor2,hor3,ver1,ver2,ver3;
				float maxV1,minV1;

				p[1] = SerialNumber;
				SerialNumber++;
				int putArr[3];
				bool first1 = true;
				for(i = startNum[q]; i < endNum[q]; i++){
					inext = i + 1;

					wp1[0] = mx;
					wp1[1] = my;
					wp1[2] = mz;
					v1 = mHor / 360.0;
					v2 = 0.5 - mVer / 180.0;
					vp1[0] = v1;
					vp1[1] = v2;
					maxV1 = v1;
					minV1 = v1;

					wp2[0] = save3D[0][2][i][q] + nummx[q];
					wp2[1] = save3D[0][3][i][q] + nummy[q];
					wp2[2] = save3D[0][4][i][q];
					p[2] = SerialNumber;
					SerialNumber++;
					hor2 = seeHorDeg(sx,sy,wp2[0],wp2[1]);
					ver2 = seeVerDeg(sx,sy,0,wp2[0],wp2[1],wp2[2]);
					v1 = hor2 / 360.0;
					v2 = 0.5 - ver2 / 180.0;
					vp2[0] = v1;
					vp2[1] = v2;
					if(v1 > maxV1){ maxV1 = v1;}
					if(v1 < minV1){ minV1 = v1;}
					wp3[0] = save3D[0][2][inext][q] + nummx[q];
					wp3[1] = save3D[0][3][inext][q] + nummy[q];
					wp3[2] = save3D[0][4][inext][q];
					p[3] = SerialNumber;
					SerialNumber++;
					hor3 = seeHorDeg(sx,sy,wp3[0],wp3[1]);
					ver3 = seeVerDeg(sx,sy,0,wp3[0],wp3[1],wp3[2]);
					v1 = hor3 / 360.0;
					v2 = 0.5 - ver3 / 180.0;
					vp3[0] = v1;
					vp3[1] = v2;
					if(v1 > maxV1){ maxV1 = v1;}
					if(v1 < minV1){ minV1 = v1;}

					if(maxV1 - minV1 > 0.7){
						if(vp1[0] < 0.5){ vp1[0] = 1.0 + vp1[0];}
						if(vp2[0] < 0.5){ vp2[0] = 1.0 + vp2[0];}
						if(vp3[0] < 0.5){ vp3[0] = 1.0 + vp3[0];}
					}

					putArr[0] = 2;
					putArr[1] = 3;
					putArr[2] = 1;

					if(showChange == 1){
						int g = putArr[1];
						putArr[1] = putArr[2];
						putArr[2] = g;
					}

					t1 = (wp1[0] + wp2[0] + wp3[0]) / 3;
					t2 = (wp1[1] + wp2[1] + wp3[1]) / 3;
					t3 = (wp1[2] + wp2[2] + wp3[2]) / 3;
					triTot++;
					t4 = seeHorDeg(nummx[r],nummy[r],t1,t2);
					t5 = get2Dist(nummx[r],nummy[r],t1,t2);
					triSub[triNum][0][r] = 2;
					triSub[triNum][1][r] = t4;
					triSub[triNum][2][r] = t5;
					triSub[triNum][3][r] = t1;
					triSub[triNum][4][r] = t2;
					triSub[triNum][5][r] = t3;
					int j1,j2;
					for(j = 0; j < 3; j++){
						j1 = j * 3;
						j2 = 9 + j * 2;
						int w = putArr[j];
						switch(w){
							case 1:
								triData[triNum][j1][r] = wp1[0];
								triData[triNum][j1 + 1][r] = wp1[1];
								triData[triNum][j1 + 2][r] = wp1[2];
								triData[triNum][j2][r] = vp1[0];
								triData[triNum][j2 + 1][r] = vp1[1];
								triSub[triNum][6 + j][r] = p[w];
							break;
							case 2:
								triData[triNum][j1][r] = wp2[0];
								triData[triNum][j1 + 1][r] = wp2[1];
								triData[triNum][j1 + 2][r] = wp2[2];
								triData[triNum][j2][r] = vp2[0];
								triData[triNum][j2 + 1][r] = vp2[1];
								triSub[triNum][6 + j][r] = p[w];
							break;
							default:
								triData[triNum][j1][r] = wp3[0];
								triData[triNum][j1 + 1][r] = wp3[1];
								triData[triNum][j1 + 2][r] = wp3[2];
								triData[triNum][j2][r] = vp3[0];
								triData[triNum][j2 + 1][r] = vp3[1];
								triSub[triNum][6 + j][r] = p[w];
							break;
						}

					}
					triNum++;

					wp1[0] = mx;
					wp1[1] = my;
					wp1[2] = mz;
					v1 = mHor / 360.0;
					v2 = 0.5 - mVer / 180.0;
					vp1[0] = v1;
					vp1[1] = v2;
					maxV1 = v1;
					minV1 = v1;

					wp2[0] = save3D[save3DCnt[i][q] - 1][2][i][q] + nummx[q];
					wp2[1] = save3D[save3DCnt[i][q] - 1][3][i][q] + nummy[q];
					wp2[2] = save3D[save3DCnt[i][q] - 1][4][i][q];
					p[2] = SerialNumber;
					SerialNumber++;
					hor2 = seeHorDeg(sx,sy,wp2[0],wp2[1]);
					ver2 = seeVerDeg(sx,sy,0,wp2[0],wp2[1],wp2[2]);
					v1 = hor2 / 360.0;
					v2 = 0.5 - ver2 / 180.0;
					vp2[0] = v1;
					vp2[1] = v2;
					if(v1 > maxV1){ maxV1 = v1;}
					if(v1 < minV1){ minV1 = v1;}
					wp3[0] = save3D[save3DCnt[inext][q] - 1][2][inext][q] + nummx[q];
					wp3[1] = save3D[save3DCnt[inext][q] - 1][3][inext][q] + nummy[q];
					wp3[2] = save3D[save3DCnt[inext][q] - 1][4][inext][q];
					p[3] = SerialNumber;
					SerialNumber++;
					hor3 = seeHorDeg(sx,sy,wp3[0],wp3[1]);
					ver3 = seeVerDeg(sx,sy,0,wp3[0],wp3[1],wp3[2]);
					v1 = hor3 / 360.0;
					v2 = 0.5 - ver3 / 180.0;
					vp3[0] = v1;
					vp3[1] = v2;
					if(v1 > maxV1){ maxV1 = v1;}
					if(v1 < minV1){ minV1 = v1;}

					if(maxV1 - minV1 > 0.7){
						if(vp1[0] < 0.5){ vp1[0] = 1.0 + vp1[0];}
						if(vp2[0] < 0.5){ vp2[0] = 1.0 + vp2[0];}
						if(vp3[0] < 0.5){ vp3[0] = 1.0 + vp3[0];}
					}

					putArr[0] = 2;
					putArr[1] = 3;
					putArr[2] = 1;

					if(showChange == 1){
						int g = putArr[1];
						putArr[1] = putArr[2];
						putArr[2] = g;
					}

					t1 = (wp1[0] + wp2[0] + wp3[0]) / 3;
					t2 = (wp1[1] + wp2[1] + wp3[1]) / 3;
					t3 = (wp1[2] + wp2[2] + wp3[2]) / 3;
					triTot++;
					t4 = seeHorDeg(nummx[r],nummy[r],t1,t2);
					t5 = get2Dist(nummx[r],nummy[r],t1,t2);
					triSub[triNum][0][r] = 2;
					triSub[triNum][1][r] = t4;
					triSub[triNum][2][r] = t5;
					triSub[triNum][3][r] = t1;
					triSub[triNum][4][r] = t2;
					triSub[triNum][5][r] = t3;
					for(j = 0; j < 3; j++){
						int w1 = j * 3;
						int w2 = 9 + j * 2;
						int w = putArr[j];
						switch(w){
							case 1:
								triData[triNum][w1][r] = wp1[0];
								triData[triNum][w1 + 1][r] = wp1[1];
								triData[triNum][w1 + 2][r] = wp1[2];
								triData[triNum][w2][r] = vp1[0];
								triData[triNum][w2 + 1][r] = vp1[1];
								triSub[triNum][6 + j][r] = p[w];
							break;
							case 2:
								triData[triNum][w1][r] = wp2[0];
								triData[triNum][w1 + 1][r] = wp2[1];
								triData[triNum][w1 + 2][r] = wp2[2];
								triData[triNum][w2][r] = vp2[0];
								triData[triNum][w2 + 1][r] = vp2[1];
								triSub[triNum][6 + j][r] = p[w];
							break;
							default:
								triData[triNum][w1][r] = wp3[0];
								triData[triNum][w1 + 1][r] = wp3[1];
								triData[triNum][w1 + 2][r] = wp3[2];
								triData[triNum][w2][r] = vp3[0];
								triData[triNum][w2 + 1][r] = vp3[1];
								triSub[triNum][6 + j][r] = p[w];
							break;
						}

					}
					triNum++;
				}
			}
		}
		*/
		triCnt[r] = triNum;
	}
	db_debug("Lidar create ck2 %d\n" , triCnt[0]);
	write3Ddata2OBJ();
	db_debug("Lidar create ck3\n");
}

unsigned char byte_buf[32];
void writeByte(unsigned char *pData, int len) {
	int a;
	for(a = 0; a < len; a++){
		byte_buf[a] = *pData++;
	}
}

void createLas_1_3_File() {
   int r,i,j,k,a;
   float t1,t2,t3,t4,t5;
   float px,py,pz;
   int pr,pg,pb,prgb;
   unsigned char temp[4];
   unsigned char longTemp[8];
   unsigned short q = 0;
   unsigned char *pData;
   char desc[48];
   unsigned short unsignval;
   unsigned long unlong;
   unsigned long long unllong;
   double doubleVal;
   FILE *fp;
   fp = fopen(lasPath,"w");

   char signature[4] = {'L','A','S','F'};
   fwrite(signature,4,1,fp);
   unsignval = 1;
   pData = (unsigned char *)&unsignval;
   for(a = 0; a < 2; a++){
       temp[a] = *pData++;
   }
   fwrite(temp,2,1,fp);    // File Source ID
   unsignval = 0;
   pData = (unsigned char *)&unsignval;
   for(a = 0; a < 2; a++){
       temp[a] = *pData++;
   }
   fwrite(temp,2,1,fp);    // Global Encodin
   for(i = 0; i < 32; i++) desc[i] = 0;
   fwrite(desc,4,1,fp);    //GUID data 1
   fwrite(desc,2,1,fp);    //GUID data 2
   fwrite(desc,2,1,fp);    //GUID data 3
   fwrite(desc,8,1,fp);    //GUID data 4
   desc[0] = 1;
   fwrite(desc,1,1,fp);    //Version Major
   desc[0] = 3;
   fwrite(desc,1,1,fp);    //Version Minor
   desc[0] = 0;
   fwrite(desc,32,1,fp);    //System Identifier
   fwrite(desc,32,1,fp);    //Generating Software

   time_t timep;
   struct tm *p;
   time(&timep);
   p=gmtime(&timep);
   db_debug("Lidar las date: %d%d%d\n",(1900+p->tm_year), (1+p->tm_mon),p->tm_mday );
   db_debug("Lidar las Y/D: %d%d\n",(1900+p->tm_year), (1 + p->tm_yday) );
   unsignval = (unsigned short)(1 + p->tm_yday);
   pData = (unsigned char *)&unsignval;
   for(a = 0; a < 2; a++){
       temp[a] = *pData++;
   }
   fwrite(temp,2,1,fp);    // Flight Date Julian
   unsignval = (unsigned short)(1 + p->tm_year);;
   pData = (unsigned char *)&unsignval;
   for(a = 0; a < 2; a++){
       temp[a] = *pData++;
   }
   fwrite(temp,2,1,fp);    // Year
   unsignval = 235;
   pData = (unsigned char *)&unsignval;
   for(a = 0; a < 2; a++){
       temp[a] = *pData++;
   }
   fwrite(temp,2,1,fp);    // Header Size
   unlong = 295;
   pData = (unsigned char *)&unlong;
   for(a = 0; a < 4; a++){
       temp[a] = *pData++;
   }
   fwrite(temp,4,1,fp);    // Offset to data
   unlong = 1;
   pData = (unsigned char *)&unlong;
   for(a = 0; a < 4; a++){
       temp[a] = *pData++;
   }
   fwrite(temp,4,1,fp);    // Number of variable length records
   desc[0] = 2;
   fwrite(desc,1,1,fp);    //Point Data Format ID
   unsignval = 26;
   pData = (unsigned char *)&unsignval;
   for(a = 0; a < 2; a++){
       temp[a] = *pData++;
   }
   fwrite(temp,2,1,fp);    // Point Data Record Length
   unlong = (unsigned long)readCount;
   pData = (unsigned char *)&unlong;
   for(a = 0; a < 4; a++){
       temp[a] = *pData++;
   }
   fwrite(temp,4,1,fp);    // Number of point recordss
   desc[0] = 0;
   fwrite(desc,20,1,fp);    //Number of points by return
   doubleVal = 1;
   pData = (unsigned char *)&doubleVal;
   for(a = 0; a < 8; a++){
       longTemp[a] = *pData++;
   }
   fwrite(longTemp,8,1,fp);    //X scale factor
   fwrite(longTemp,8,1,fp);    //Y scale factor
   fwrite(longTemp,8,1,fp);    //Z scale factor
   doubleVal = 0;
   pData = (unsigned char *)&doubleVal;
   for(a = 0; a < 8; a++){
       longTemp[a] = *pData++;
   }
   fwrite(longTemp,8,1,fp);    //X offset
   fwrite(longTemp,8,1,fp);    //Y offset
   fwrite(longTemp,8,1,fp);    //Z offset
   for(i = 0; i < 6; i++){
       switch(i){
           case 0: doubleVal = readMaxX; break;
           case 1: doubleVal = readMinX; break;
           case 2: doubleVal = readMaxY; break;
           case 3: doubleVal = readMinY; break;
           case 4: doubleVal = readMaxZ; break;
           case 5: doubleVal = readMinZ; break;
           default:break;
       }
       pData = (unsigned char *)&doubleVal;
       for(a = 0; a < 8; a++){
           longTemp[a] = *pData++;
       }
       fwrite(longTemp,8,1,fp);    //Max & Min
   }
   unllong = 0;
   pData = (unsigned char *)&unllong;
   for(a = 0; a < 8; a++){
       longTemp[a] = *pData++;
   }
   fwrite(longTemp,8,1,fp);    //Start of Waveform Data Packet Record

   //variable
   unsignval = 0xAABB;
   pData = (unsigned char *)&unsignval;
   for(a = 0; a < 2; a++){
       temp[a] = *pData++;
   }
   fwrite(temp,2,1,fp);    // Record Signature (0xAABB)
   for(i = 0; i < 32; i++) desc[i] = 0;
   desc[0] = 'G';desc[1] = 'e';desc[2] = 'o';desc[3] = 'K';desc[4] = 'e';desc[5] = 'y';
   desc[6] = 'D';desc[7] = 'i';desc[8] = 'r';desc[9] = 'e';desc[10] = 'c';desc[11] = 't';
   desc[12] = 'o';desc[13] = 'r';desc[14] = 'y';
   fwrite(desc,16,1,fp);        // User ID
   unsignval = 0;
   pData = (unsigned char *)&unsignval;
   for(a = 0; a < 2; a++){
       temp[a] = *pData++;
   }
   fwrite(temp,2,1,fp);        //Record ID
   unsignval = 0;
   pData = (unsigned char *)&unsignval;
   for(a = 0; a < 2; a++){
       temp[a] = *pData++;
   }
   fwrite(temp,2,1,fp);        //Record Length After Header
   for(i = 0; i < 38; i++) desc[i] = 0;
   fwrite(desc,38,1,fp);        //Description

   //points
   /*unsignval = 0xDDCC;
   pData = (unsigned char *)&unsignval;
   for(a = 0; a < 2; a++){
       temp[a] = *pData++;
   }
   fwrite(temp,2,1,fp);    // Record Signature (0xDDCC)
   */
   float dist = 0;
   float inte = 0;
   int wrtCnt = 0;
   long lData[4];

   for(r = 0; r < reads; r++){
       for(i = startNum[r]; i < endNum[r]; i++){
           int c = save3DCnt[i][r];
           for(j = 0; j < c; j++){
               lData[0] = (long)save3D[j][2][i][r] * -1;
               lData[1] = (long)save3D[j][3][i][r] * 1;
               lData[2] = (long)save3D[j][4][i][r] * 1;
               lData[3] = (long)save3D[j][5][i][r];
               //fData[0] = i;
               //fData[1] = j;
               //fData[2] = 5;
               pr = 0;
               pg = 0;
               pb = 0;
               if(i < 20){
                   pr = 128 * 256;
               }else if(i < 50){
                   pg = 128 * 256;
               }else{
                   pb = 128 * 256;
               }
               prgb = (pr << 16) + (pg << 8) + pb;

               for(k = 0; k < 3; k++){
                   pData = (unsigned char *)&lData[k];
                   for(a = 0; a < 4; a++){
                       longTemp[a] = *pData++;
                   }
                   fwrite(longTemp,4,1,fp);
               }
               unsignval = 10;
               pData = (unsigned char *)&unsignval;
               for(a = 0; a < 2; a++){
                   temp[a] = *pData++;
               }
               fwrite(temp,2,1,fp);        //

               temp[0] = 0x80;
               fwrite(temp,1,1,fp);        //
               temp[0] = 0;
               fwrite(temp,1,1,fp);        //
               temp[0] = 1;
               fwrite(temp,1,1,fp);        //
               temp[0] = 0;
               fwrite(temp,1,1,fp);        //
               unsignval = 1;
               pData = (unsigned char *)&unsignval;
               for(a = 0; a < 2; a++){
                   temp[a] = *pData++;
               }
               fwrite(temp,2,1,fp);        //

               pData = (unsigned char *)&pr;
               for(a = 0; a < 2; a++){
                   temp[a] = *pData++;
               }
               fwrite(temp,2,1,fp);        //
               pData = (unsigned char *)&pg;
               for(a = 0; a < 2; a++){
                   temp[a] = *pData++;
               }
               fwrite(temp,2,1,fp);        //
               pData = (unsigned char *)&pb;
               for(a = 0; a < 2; a++){
                   temp[a] = *pData++;
               }
               fwrite(temp,2,1,fp);        //

               wrtCnt++;
           }
       }
   }
   fclose(fp);
   db_debug("Lidar create LAS Cnt : %d\n",wrtCnt);
   db_debug("Lidar create LAS File : %s\n",lasPath);
}

void createMyFile() {
	int r,i,j,k,a;
	float t1,t2,t3,t4,t5;
	float px,py,pz;
	int pr,pg,pb,prgb;
	unsigned char temp[4];
	unsigned short q = 0;
	unsigned char *pData;
	int fData[4];
	FILE *fp;
	fp = fopen(l63Path,"w");
	fprintf(fp,"# LD363 scan data    \n");
	fprintf(fp,"VERSION 0.1\n");
	fprintf(fp,"FIELDS d i\n");
	fprintf(fp,"TYPE i i\n");
	fprintf(fp,"SIZE 4 4\n");
	fprintf(fp,"WIDTH %d\n",lidarW);
	fprintf(fp,"HEIGHT %d\n",lidarH);
	fprintf(fp,"DATA binary\n");
	int dist = 0;
	int inte = 0;
	for(i = 0; i < lidarW; i++){
		for(j = 0; j < lidarH; j++){
			k = j + i * lidarH;
			dist = ( (unsigned int)(*(lidarBuf + k)) & 0xFFFF0000) >> 16;
			inte = ( (unsigned int)(*(lidarBuf + k)) & 0x0000FFFF);
			fData[0] = dist;
			fData[1] = inte;
			for(k = 0; k < 2; k++){
				pData = (unsigned char *)&fData[k];
				for(a = 0; a < 4; a++){
					temp[a] = *pData++;
				}
				fwrite(temp,4,1,fp);
			}
		}
	}
	fclose(fp);
}

void createPCDFile() {
	int r,i,j,k,a;
	float t1,t2,t3,t4,t5;
	float px,py,pz;
	int pr,pg,pb,prgb;
	unsigned char temp[4];
	unsigned short q = 0;
	unsigned char *pData;
	float fData[4];
	FILE *fp;
	fp = fopen(pcdPath,"w");
	fprintf(fp,"# .PCD v0.7 - Point Cloud Data file format\n");
	fprintf(fp,"VERSION 0.7\n");
	fprintf(fp,"FIELDS x y z intensity rgb\n");
	fprintf(fp,"SIZE 4 4 4 4 4\n");
	fprintf(fp,"TYPE F F F F F\n");
	fprintf(fp,"COUNT 1 1 1 1 1\n");
	fprintf(fp,"WIDTH %d\n",readCount);
	fprintf(fp,"HEIGHT 1\n");
	fprintf(fp,"VIEWPOINT 0 0 0 1 0 0 0\n");
	fprintf(fp,"POINTS %d\n",readCount);
	fprintf(fp,"DATA binary\n");
	for(r = 0; r < reads; r++){
		for(i = startNum[r]; i < endNum[r]; i++){
			int c = save3DCnt[i][r];
			for(j = 0; j < c; j++){
				fData[0] = save3D[j][2][i][r] * -1.0f;
				fData[1] = save3D[j][3][i][r] * 1.0f;
				fData[2] = save3D[j][4][i][r] * 1.0f;
				fData[3] = save3D[j][5][i][r];
				//fData[0] = i;
				//fData[1] = j;
				//fData[2] = 5;
				pr = 0;
				pg = 0;
				pb = 0;
				if(i < 20){
					pr = 128;
				}else if(i < 50){
					pg = 128;
				}else{
					pb = 128;
				}
				prgb = (pr << 16) + (pg << 8) + pb;
				for(k = 0; k < 4; k++){
					pData = (unsigned char *)&fData[k];
					for(a = 0; a < 4; a++){
						temp[a] = *pData++;
					}
					fwrite(temp,4,1,fp);
				}
				pData = (unsigned char *)&prgb;
				for(a = 0; a < 4; a++){
					temp[a] = *pData++;
				}
				fwrite(temp,4,1,fp);
			}
		}
	}
	fclose(fp);
	db_debug("Lidar create PCD File : %s\n",pcdPath);
}

void setLidarSavePath(char *dir_path,char *isid,int file_cnt) {
	csid[0] = isid[3];                          // 取 US_0000 的 0000
	csid[1] = isid[4];
	csid[2] = isid[5];
	csid[3] = isid[6];
	csid[4] = '\0';
	pictureCnt = file_cnt;

	sprintf(objPath, "%s/DCIM/%s/M%s_%04d.obj\0", dir_path, isid , csid , pictureCnt);
	sprintf(mtlPath, "%s/DCIM/%s/M%s_%04d.mtl\0", dir_path, isid , csid , pictureCnt);
	sprintf(pcdPath, "%s/DCIM/%s/M%s_%04d.pcd\0", dir_path, isid , csid , pictureCnt);
	sprintf(l63Path, "%s/DCIM/%s/M%s_%04d.l63\0", dir_path, isid , csid , pictureCnt);
	sprintf(lasPath, "%s/DCIM/%s/M%s_%04d.las\0", dir_path, isid , csid , pictureCnt);
	sprintf(imageName, "P%s_%04d.jpg\0", csid , pictureCnt);
	db_debug("set lidar path name %s\n",imageName);
}

void createLidarData(int *buf,int width,int height) {
	int i,j,k;
	lidarW = width;
	lidarH = height;
	lidarBuf = buf;
	SetLidarThreadState(1);
	create_lidar_thread();
}

/*
 * rex+ 201031
 *   lidar function
 */
void sincosTransferDist(int pSize) {
	short int *sin, *cos;
	int i;

	int SIN_LD;
	int COS_LD;
	unsigned long long sum;
	double sq_an;
	float Data_sum;
	int avg_Data;			// 資料強度
	int Distance;			// 距離
	float ToF;
	float PI = 3.14159265359;
	float FT_PER_PLD = 650;
	float LED_Freq = 9.9;
	double tan;
	float tan_rate;

	sin = (short int *)&lidarSinCosData[0];		//
	cos = sin++;								//
	for(i = 0; i < pSize; i++, sin+=2, cos+=2){
		SIN_LD = -(*sin);
		COS_LD = -(*cos);

		sum = (((long long)(SIN_LD))*((long long)(SIN_LD))+((long long)(COS_LD))*((long long)(COS_LD)));
		sq_an = sqrt(sum);
		Data_sum = sq_an*3.1416/4;
		avg_Data = sq_an/(FT_PER_PLD)*4/3.1416;			// (FT_PER-PLD)=650

		if(COS_LD ==0){
			tan_rate = 0;
		}else{
			tan_rate = (float)SIN_LD / (float)COS_LD;
		}
		if (COS_LD >= 0 && SIN_LD >=0) tan = atan (tan_rate);
		if (COS_LD < 0  && SIN_LD >0)  tan = PI + atan (tan_rate);
		if (COS_LD > 0  && SIN_LD <0)  tan = atan (tan_rate);
		if (COS_LD < 0  && SIN_LD <0)  tan = PI + atan (tan_rate);
		ToF =  -(tan * 1000/ LED_Freq / (2*PI) ) + 1.5 + 5;
		Distance =  ToF *1000 * 0.299792458 /2;

		lidarDistanceData[i] = (Distance<<16) | (avg_Data&0xffff);
	}
}

void wrtieLidar2File() {
	//readLidarData(sd_path,mSSID,cap_file_cnt,512,256);
	int lidarW = 512;
	int lidarH = 256;
	/*
	int i,j,k;
	for(i = 0; i < lidarW; i++){
		for(j = 0; j < lidarH; j++){
			k = j + lidarH * i;
			lidarSinCosData[k] = (1000 << 16) + 10;
		}
	}
	*/
	createLidarData(lidarSinCosData,512,256);
}

int inputSinCosData(int *buf, int size) {
	int max = sizeof(lidarSinCosData);
	int ptr = size >> 2;
    if(size > max)
    	size = max;

    memcpy(lidarSinCosData, buf, size);

    int i;
    for(i = 0; i < 16; i+=4){
    	db_debug("input: data[%x]={%08x,%08x,%08x,%08x}\n", i,
    			lidarSinCosData[i+0], lidarSinCosData[i+1], lidarSinCosData[i+2], lidarSinCosData[i+3]);
    }
    for(i = ptr-16; i < ptr; i+=4){
    	db_debug("input: data[%x]={%08x,%08x,%08x,%08x}\n", i,
    			lidarSinCosData[i+0], lidarSinCosData[i+1], lidarSinCosData[i+2], lidarSinCosData[i+3]);
    }
    sincosTransferDist(size);

    for(i = 0; i < 16; i+=4){
    	db_debug("output: data[%x]={%08x,%08x,%08x,%08x}\n", i,
    			lidarSinCosData[i+0], lidarSinCosData[i+1], lidarSinCosData[i+2], lidarSinCosData[i+3]);
    }
    for(i = ptr-16; i < ptr; i+=4){
    	db_debug("output: data[%x]={%08x,%08x,%08x,%08x}\n", i,
    			lidarSinCosData[i+0], lidarSinCosData[i+1], lidarSinCosData[i+2], lidarSinCosData[i+3]);
    }
    wrtieLidar2File();
    return size;
}