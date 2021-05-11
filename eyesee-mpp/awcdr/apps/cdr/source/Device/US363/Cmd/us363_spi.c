/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Cmd/us363_spi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <malloc.h>
#include <pthread.h>

#include "Device/spi.h"
#include "Device/qspi.h"
#include "Device/US363/Kernel/k_spi_cmd.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363::US363SPI"

#define swap4(x) ( (x & 0xFF) << 24) | ( (x & 0xFF00) << 8) | ( (x >> 8) & 0xFF00) | (( x >> 24) & 0xFF)

char spi_fpga_cmd_buf[4100];
char spi_fpga_read_buf[5120];
char spi_ddr_read_buf[5120];
char spi_ddr_write_buf[6400];

/*int US363_SPI_Open() {
	return SPI_Open();
}

void US363_SPI_Close() {
	SPI_Close();
}*/

void make_F2_Addr_Cmd(void *data, unsigned addr, unsigned size ,unsigned char mode) {
    F2_DATA_HI_LO_STRUCT *dp;

    dp                 = (F2_DATA_HI_LO_STRUCT *)data;

    dp->temp_x         = addr >> 5;         // 1 burst = 32 bytes
    dp->temp_y         = addr >> 12;// + mode ; //( Form1->RadioGroup_mem_ctl2->ItemIndex == 0 ) ? addr >> 12 : (addr >> 12) + 1;        // 1 line = 4096 bytes
    dp->offset_y       = 1;
    dp->y14            = ( addr >> 26 ) & 0x7;
    dp->rev1           = 0;

    dp->x_start_addr   = addr >> 5;
    dp->x_stop_addr    = (addr + size) > 4096 ?  (addr + size - dp->temp_y * 0x1000) >> 5 : (addr + size ) >> 5 ;
    dp->y_counter      = ((addr - (dp->temp_y << 12)) + size + 4095) >> 12 ;
    dp->burst_mode       = 0;//mode ; //( Form1->RadioGroup_mem_ctl2->ItemIndex == 0 ) ? 0 : 1;
    dp->addr_mask_mode = 0;
    dp->rev2           = 0;
}

void make_ddr_read_cmd(unsigned char *cbuf, unsigned addr, unsigned burst ,unsigned char mode, unsigned f_id) {
    unsigned cmd_len, size;
    unsigned *uP1;
    dev_fpga_cmd_struct fpga_cmd;

    size         = burst << 5;             //For DDR2 size   ,32 bytes?1??

    // FPGA cmd
    cmd_len          = sizeof(dev_fpga_cmd_struct);
    if(f_id == 0) 	   fpga_cmd.rw = 0xF2;
    else if(f_id == 1) fpga_cmd.rw = 0xF3;
    else if(f_id == 2) fpga_cmd.rw = 0xF1;
    fpga_cmd.cmd     = 0x01;
    fpga_cmd.size[0] = ((burst>>8)&0xff);
    fpga_cmd.size[1] = (burst&0xff);
    memcpy(cbuf, &fpga_cmd, cmd_len);
    cbuf += cmd_len;

    // DDR HI LO command
    make_F2_Addr_Cmd((void *)cbuf, addr, size, mode);

    uP1  = (unsigned *)cbuf;
    *uP1 = swap4(*uP1);
    uP1  ++;
    *uP1 = swap4(*uP1);
    cbuf += sizeof(F2_DATA_HI_LO_STRUCT);
}

void make_ddr_write_cmd(unsigned char *cbuf, unsigned char *data, unsigned addr, unsigned burst ,unsigned char mode) {
    unsigned i, cmd_len, size;
    unsigned *uP1;
    dev_fpga_cmd_struct fpga_cmd;

    size = burst << 5;

    // FPGA command
    cmd_len          = sizeof(dev_fpga_cmd_struct);
    fpga_cmd.rw      = 0xF0;
    fpga_cmd.cmd     = 0x01;
    fpga_cmd.size[0] = ((burst>>8)&0xff);
    fpga_cmd.size[1] = (burst&0xff);
    memcpy(cbuf, &fpga_cmd, cmd_len);
    cbuf += cmd_len;

    // DDR HI LO command
    make_F2_Addr_Cmd((void *)cbuf, addr, size, mode);

    uP1  =  (unsigned *)cbuf;
    *uP1 =  swap4(*uP1);
    uP1  ++;
    *uP1 =  swap4(*uP1);
    cbuf += sizeof(F2_DATA_HI_LO_STRUCT);
    memcpy(cbuf, data, size);
}

void set_spi_transfer(unsigned long *tbuf, unsigned long *rbuf, int nbits, int len, struct spi_ioc_transfer *xfer) {
	xfer->tx_buf        = tbuf;
	xfer->rx_buf        = rbuf;
    xfer->len           = len;
	xfer->tx_nbits      = nbits;
	xfer->rx_nbits      = nbits;
	xfer->delay_usecs   = 0;
	xfer->cs_change     = 0;
	xfer->bits_per_word = 8;
	xfer->speed_hz      = 0;			//使用spi原本的設定
}

int spi_fpga_cmd(int rw, int cmd, int *data, int size) {
    int len=4, dw, status, ret=1;
    dev_fpga_cmd_struct cmd_fpga;
	
	if(SPI_Open() < 0) {
		db_error("spi_fpga_cmd spi open error!");
		return -1;
	}

    // check
    if((size > 4096) || ((size & 0x3) != 0))
    {    // 1次最多 4096 bytes
        db_error("spi_fpga_cmd: (size > 4096) || ((size & 0x3)  err!");
        return -6;                                // run_ua360_spi_overflow
    }

//	pthread_mutex_lock(&pthread_spi_mutex);
    memset(&spi_fpga_cmd_buf[0], 0, sizeof(spi_fpga_cmd_buf));

    if(rw == 0) cmd_fpga.rw = 0xF1;             // read
    else        cmd_fpga.rw = 0xF0;             // write

    cmd_fpga.cmd     = cmd;                     // 0x09 -> MIPS I/O
    dw = ((size+3)>>2);                         // FPGA I/O 以 4bytes 為單位
    cmd_fpga.size[0] = ((dw>>8)&0xff);          // 高低位元對調
    cmd_fpga.size[1] = (dw&0xff);

    memcpy(&spi_fpga_cmd_buf[0], &cmd_fpga, len);    // cmd
    if(rw == 1){        // write
        memcpy(&spi_fpga_cmd_buf[len], data, size);    // data
		status = SPI_Write(spi_fpga_cmd_buf, len+size);
    }
    else if(rw == 0){   // read
		struct spi_ioc_transfer xfer[2];
		set_spi_transfer((unsigned long*) spi_fpga_cmd_buf, NULL, 1, len, &xfer[0]);
		set_spi_transfer(NULL, (unsigned long*) spi_fpga_read_buf, 1, (size+20), &xfer[1]);
		status = SPI_IOCTL(SPI_IOC_MESSAGE(2), &xfer[0]);
        memcpy(data, &spi_fpga_read_buf[20], size);
    }
    if (status < 0) { 
        db_error("spi_fpga_cmd: status < 0  err!");
//		pthread_mutex_unlock(&pthread_spi_mutex);
        return -7;        // "SPI Write Err
    }
    else
        ret = 1;
	
//	pthread_mutex_unlock(&pthread_spi_mutex);
    return ret;
}

int SPI_Write_IO_S2( int io_idx, int *data , int size) {
    return spi_fpga_cmd( 1, io_idx, data, size);
}

int SPI_Read_IO_S2( int io_idx, int *data, int size) {
    return spi_fpga_cmd( 0, io_idx, data, size);
}

int ua360_spi_ddr_read( int read_addr, int *read_buf, int read_size, int f_id, int fx_x) {
    unsigned char cmd_buf[128], mode='\0';
    unsigned cmd_len=0, i, num=0, status;
	struct spi_ioc_transfer xfer[2];
	
    if(SPI_Open() < 0) {
		db_error("ua360_spi_ddr_read spi open error!");
		return -1;
	}

    if(f_id < 2 && read_size > 2048) {		//Read F0/F1 Size Max = 2048 byte
    	db_error("ua360_spi_ddr_read: f_id=%d size=%d\n", f_id, read_size);
    	return -3;
    }

    cmd_len = sizeof(dev_fpga_cmd_struct) + sizeof(F2_DATA_HI_LO_STRUCT);
    if((read_size <= 0) || ((read_size % 32) != 0) || ((read_addr % 4) != 0) ||
       (cmd_len > sizeof(cmd_buf)) || ((read_size+20) > sizeof(spi_ddr_read_buf)))
    {
        db_error("ua360_spi_ddr_read: err!");
        return -3;
    }

    //Write IO to Read F0/F1
    unsigned Data[2];
    unsigned *cmd_p;
    FX_DDR_Cmd_struct r_fx_cmd;
    if(f_id < 2) {
    	r_fx_cmd.FX_SizeX = fx_x;	// 0:512 Byte * 4 Line,	1:1024 Byte * 2 Line, 2:2048 Byte * 1 Line
    	r_fx_cmd.S_DDR_P  = (read_addr >> 5);

    	cmd_p = (unsigned *) &r_fx_cmd;
		Data[0] = 0xF10;
  		Data[1] = *cmd_p;
   		SPI_Write_IO_S2(0x9, (int *) &Data[0], 8);    // SPI_W
    }

	pthread_mutex_lock(&pthread_spi_mutex);

    make_ddr_read_cmd(cmd_buf, read_addr, read_size>>5, mode, f_id);
	set_spi_transfer((unsigned long*) cmd_buf, NULL, 1, cmd_len, &xfer[0]);
	set_spi_transfer(NULL, (unsigned long*) spi_ddr_read_buf, 1, (read_size+20), &xfer[1]);
	status = SPI_IOCTL(SPI_IOC_MESSAGE(2), &xfer[0]);
    if (status < 0) {
        db_error("SPI_Read Fail");
		pthread_mutex_unlock(&pthread_spi_mutex);
        return -4;
    }

    // 前 20 byte 是空的, 重新做陣列
    memcpy(&read_buf[0], &spi_ddr_read_buf[20], read_size);
	
	pthread_mutex_unlock(&pthread_spi_mutex);
	return 1;
}

int ua360_spi_ddr_write(int write_addr, int *write_buff, int write_size) {
    int status;
    unsigned char mode='\0';        // cmd_buf[5120]
    unsigned cmd_len=0, write_len=0, i, num;

    if(SPI_Open() < 0) {
        db_error("ua360_spi_ddr_write spi open error!");
        return -1;
    }

    cmd_len = sizeof(dev_fpga_cmd_struct) + sizeof(F2_DATA_HI_LO_STRUCT);
    //size對齊512, addr對齊4
    if((write_size <= 0) || ((write_size % 512) != 0) || ((cmd_len + write_size) > sizeof(spi_ddr_write_buf)) ||
       ((write_addr % 4) != 0) || ((write_addr + write_size) > DDR_ON_DVR_SIZE) || write_size > 4096)
    {
        db_error("ua360_spi_ddr_write: err! addr=0x%x size=0x%x", write_addr, write_size);
        return -3;
    }
	
	pthread_mutex_lock(&pthread_spi_mutex);
    make_ddr_write_cmd((unsigned char *)spi_ddr_write_buf, (unsigned char *)write_buff, (unsigned)write_addr, write_size>>5, mode);
	status = SPI_Write(spi_ddr_write_buf, cmd_len + write_size);
    if (status < 0) {
        db_error("ua360_spi_ddr_write err!  size=%d status=%d", write_size, status);
		pthread_mutex_unlock(&pthread_spi_mutex);
        return -4;        // "SPI Write Err
    }
	pthread_mutex_unlock(&pthread_spi_mutex);
    return 1;
}

int spi_read_io_porcess_S2(int addr, int *data, int size) {
    int ret = 0, cmd=1;

	pthread_mutex_lock(&pthread_spi_mutex);

    ret = SPI_Write_IO_S2(0x9, &addr, 4);                               // spi_read_io_porcess_S2
    if(ret < 0) {
        db_error("spi_read_io_porcess_S2: write I/O err!");
		pthread_mutex_unlock(&pthread_spi_mutex);
        return -1;
    }
    if(size > 4){
        ret = SPI_Write_IO_S2(0x7, &cmd, 4);
        if(ret < 0) {
            db_error("spi_read_io_porcess_S2: cmd I/O err!");
			pthread_mutex_unlock(&pthread_spi_mutex);
            return -1;
        }
    }
	
    ret = SPI_Read_IO_S2(0x9, data, size);                              // spi_read_io_porcess_S2
    if(ret < 0) {
        db_error("spi_read_io_porcess_S2: read I/O err!");
		pthread_mutex_unlock(&pthread_spi_mutex);
        return -2;
    }

	pthread_mutex_unlock(&pthread_spi_mutex);
    return 0;
}


int ua360_qspi_ddr_write(int write_addr, int *write_buff, int write_size) {
    int status;
    unsigned char mode='\0';        // cmd_buf[5120]
    unsigned cmd_len=0, write_len=0, i, num;
	struct spi_ioc_transfer xfer[2];
	char qspi_cmd = 0x38;
	char *ptr = NULL;
	int addr;

    if(QSPI_Open() < 0) {
        db_error("ua360_qspi_ddr_write spi open error!");
        return -1;
    }

    cmd_len = 0;
    //size對齊512, addr對齊32
	//addr 至少要 2 brust, 1 brust 資料會對調
    if((write_size <= 0) || ((write_size % 512) != 0) || ((cmd_len + write_size) > sizeof(spi_ddr_write_buf)) ||
       ((write_addr % 32) != 0) || ((write_addr + write_size) > DDR_ON_DVR_SIZE) || write_size > 4096 ||
	   write_addr < 64)
    {
        db_error("ua360_qspi_ddr_write: err! addr=0x%x size=0x%x", write_addr, write_size);
        return -3;
    }
	
	pthread_mutex_lock(&pthread_spi_mutex);
	addr = swap4(write_addr >> 5);
	ptr = &spi_ddr_write_buf[0];
	memcpy(ptr, &addr, sizeof(addr) );	
	ptr += sizeof(addr);
	cmd_len += sizeof(addr);
	
	memcpy(ptr, write_buff, write_size);	
	ptr += write_size;

	set_spi_transfer((unsigned long*) &qspi_cmd, NULL, 1, 1, &xfer[0]);	
	set_spi_transfer((unsigned long*) spi_ddr_write_buf, NULL, 4, (cmd_len + write_size), &xfer[1]);	
	status = QSPI_IOCTL(SPI_IOC_MESSAGE(2), &xfer[0]);		//SPI_IOC_MESSAGE(2): 2 = xfer[2], 2組cmd
    if (status < 0) {
		db_error("ua360_qspi_ddr_write: write data err! (s=%d l=%d s=%d)", status, cmd_len, write_size);
		pthread_mutex_unlock(&pthread_spi_mutex);
        return -4;        // "SPI Write Err
    }
	pthread_mutex_unlock(&pthread_spi_mutex);
    return 1;
}