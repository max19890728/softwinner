/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __LIDAR_H__
#define __LIDAR_H__

#ifdef __cplusplus
extern "C" {
#endif

void free_lidar_buf();
int malloc_lidar_buf();
void read3Datas();
void createPCDFile();
void createLas_1_3_File();
void createMyFile();
void sort3Datas();
void createTriData();
void setLidarSavePath(char *dir_path,char *isid,int file_cnt);
void createLidarData(int *buf,int width,int height);
int inputSinCosData(int *buf, int size);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__LIDAR_H__
