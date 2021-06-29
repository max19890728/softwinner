/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Cmd/dna.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <malloc.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "Device/US363/Cmd/us363_spi.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::DNA"

void readDNA(unsigned *dna_h, unsigned *dna_l) {
    unsigned DNA_I[2]={0}, DNA_O[2]={0}, Data[2]={0};
    unsigned addr, temp;
    unsigned X_Code = 0x4658A5C3;
    int i;

    Data[0] = 0xFF0;
    Data[1] = 0;
    SPI_Write_IO_S2(0x9, (int *) &Data[0], 8);

    addr = 0xFF0;
    spi_read_io_porcess_S2(addr, (int *) &DNA_I[1], 4);
    DNA_I[1] = DNA_I[1] & 0x01FFFFFF;

    addr = 0xFF4;
    spi_read_io_porcess_S2(addr, (int *) &DNA_I[0], 4);
    if(DNA_I[1] == 0 || DNA_I[1] == 0xffffffff ||
       DNA_I[0] == 0 || DNA_I[0] == 0xffffffff)
    {
        db_error("readDNA: dna={0x%x,0x%x}\n", DNA_I[0], DNA_I[1]);
        return;
    }

    DNA_O[1] = 0;
    DNA_O[0] = 0;
    for(i = 0 ; i < 32 ; i ++){
        if((X_Code >> i) & 1 == 1){
            DNA_O[1] = DNA_O[1] ^ DNA_I[1];
            DNA_O[0] = DNA_O[0] ^ DNA_I[0];
        }
        temp = (DNA_I[1] >> 24) & 1;
        DNA_I[1] = (((DNA_I[1] << 1) | (DNA_I[0] >> 31)) & 0x1FFFFFF);
        DNA_I[0] = (DNA_I[0] << 1) | temp;
    }

    *dna_h = DNA_O[1];
    *dna_l = DNA_O[0];
}

int dnaCheck() {
    int ret;
	unsigned FPGA_DNA[2]={0}, TOOL_DNA[2]={0};
	FILE *fp;

    readDNA(&FPGA_DNA[1], &FPGA_DNA[0]);

    fp = fopen("/cache/dna_check.bin", "rb");
    if(fp != NULL) {
		fread(&TOOL_DNA[0], 8, 1, fp);
		fclose(fp);

		if(FPGA_DNA[1] == TOOL_DNA[1] && FPGA_DNA[0] == TOOL_DNA[0])
			ret = 1;
		else
			ret = 0;
    }
    else
    	ret = -1;

    db_debug("dnaCheck: FPGA_DNA={0x%x,0x%x} TOOL_DNA={0x%x,0x%x} check=%d\n", FPGA_DNA[0], FPGA_DNA[1], TOOL_DNA[0], TOOL_DNA[1], ret);
    return ret;
}

/*
 * 由治具呼叫執行存DNA檔案
 */
void makeDNAfile() {
    unsigned DNA_O[2];
    FILE *fp = NULL;

    readDNA(&DNA_O[1], &DNA_O[0]);
    fp = fopen("/cache/dna_check.bin", "wb");
    if(fp == NULL) return;

    fwrite(&DNA_O[0], 8, 1, fp);
    fclose(fp);
    db_debug("makeDNAfile: dna={0x%x,0x%x}\n", DNA_O[0], DNA_O[1]);
}
