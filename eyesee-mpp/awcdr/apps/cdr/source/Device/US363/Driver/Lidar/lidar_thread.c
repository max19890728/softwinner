/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Driver/Lidar/lidar_thread.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "Device/US363/Driver/Lidar/lidar.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::LidarThread"

pthread_mutex_t mut_lidar_buf;
static int thread_lidar_id = 2;

int writeState = 0;

void SetLidarThreadState(int state) {
	writeState = state;
}

int GetLidarThreadState() {
	return writeState;
}

void *thread_lidar(void) {
	db_debug("Lidar read ck1\n");
	read3Datas();
	SetLidarThreadState(2);
	createPCDFile();
	createLas_1_3_File();
	SetLidarThreadState(3);
	createMyFile();
	db_debug("Lidar read ck2\n");
	//sort3Datas();
	SetLidarThreadState(4);
	db_debug("Lidar read ck3\n");
	//createTriData();
	SetLidarThreadState(0);
}

void create_lidar_thread() {
	pthread_mutex_init(&mut_lidar_buf, NULL);
	if(pthread_create(&thread_lidar_id, NULL, thread_lidar, NULL) != 0){
		db_error("Create lidar_thread failed!\n");
	}	
}