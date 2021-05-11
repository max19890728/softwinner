/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Data/us360_config.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>

#include "Device/US363/us360.h"
#include "Device/US363/Cmd/us360_func.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::US360Config"

const char cfgPath1[64] = "/mnt/sdcard/US360Config.bin\0";
const char cfgPath2[64] = "/mnt/sdcard/US360Config2.bin\0";

/*
 * 存調整後Sensor參數  US360Config.bin
 * idx: 存第幾個檔案, 目前最多2個
 */
int SaveConfig(int idx) {
    int i;
    int size;
    unsigned char buf[sizeof(Adj_Sensor[LensCode])];
    FILE *SaveFile = NULL;
    int fp2fd = 0;

    size = sizeof(Adj_Sensor[LensCode]);

    memcpy(&buf[0], &Adj_Sensor[LensCode][0], size);

    if(idx == 1) SaveFile = fopen(cfgPath1, "w+b");
    else         SaveFile = fopen(cfgPath2, "w+b");
    if(SaveFile == NULL) {
        db_error("Save US360Config.bin err!");
        return -1;
    }
    fwrite(&buf[0], size, 1, SaveFile);

    fflush(SaveFile);
    fp2fd = fileno(SaveFile);
    fsync(fp2fd);

    //if(SaveFile)
    {
        fclose(SaveFile);
        close(fp2fd);
        SaveFile = NULL;
    }

    return 0;
}

/*
 * 讀取Sensor參數  US360Config.bin
 * idx: 讀取第幾個檔案, 目前最多2個
 */
int LoadConfig(int idx) {
    FILE *fp;
    int i;
    int size;
    struct stat stFileInfo;
    unsigned char buf[sizeof(Adj_Sensor[LensCode])];
    struct Adj_Sensor_Struct   Sensor_tmp[4];
    memset(&buf[0], 0, size);

    size = sizeof(Adj_Sensor[LensCode]);
    for(i = 0; i < 5; i++) Adj_Sensor_Command[i] = Adj_Sensor[LensCode][i];

    int intStat;
    if(idx == 1) intStat = stat(cfgPath1, &stFileInfo);
    else         intStat = stat(cfgPath2, &stFileInfo);
    if(intStat != 0) {
        db_error("US360Config.bin not exist!");
        return -1;
    }
    else if(size != stFileInfo.st_size) {
        db_error("US360Config.bin size not same!");
        return -2;
    }

    if(idx == 1) fp = fopen(cfgPath1, "r+b");
    else         fp = fopen(cfgPath2, "r+b");
    if(fp == NULL) {
        db_error("Load US360Config.bin err!");
        return -3;
    }
    fread(&buf[0], size, 1, fp);
    memcpy(&Adj_Sensor[LensCode][0], &buf[0], size);
    memcpy(&Adj_Sensor_Command[0], &buf[0], size);
    //memcpy(&Adj_Sensor_Command[0], &Adj_Sensor[LensCode][0], size);

    do_ST_Line_Offset();

    SensorZoom[LensCode] = Adj_Sensor[LensCode][0].Zoom_X;
    if(fp)
        fclose(fp);
    return 0;
}