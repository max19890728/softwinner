/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __US363_SPI_H__
#define __US363_SPI_H__

#ifdef __cplusplus
extern "C" {
#endif

#define DDR_ON_DVR_SIZE		0x20000000	// 512MB

typedef struct{
	unsigned    S_DDR_P   :24;		//bust
	unsigned	FX_SizeX   :6;		//0:512byte*4 1:1024byte*2 3:2048byte*1
	unsigned    rev        :2;
}FX_DDR_Cmd_struct;

typedef struct dev_fpga_cmd_struct_h {
    unsigned char   rw;                 // 0xF0 -> write, 0xF1 -> read
    unsigned char   cmd;
    unsigned char   size[2];            // 0x0123 -> size[0]=0x01, size[1]=0x23
} dev_fpga_cmd_struct;

typedef struct F2_DATA_HI_LO_STRUCT_H {
    // F2: data_lo, (temp_y & temp_x) = ddr_addr (23 downto 3), DW
    unsigned temp_x            : 7;
    unsigned temp_y            : 14;
    unsigned offset_y          : 4;    //110103
    unsigned y14               : 3;
    unsigned rev1              : 4;    //110103

    // F2: data_hi,
    unsigned x_start_addr      : 7;
    unsigned x_stop_addr       : 8;
    unsigned y_counter         : 11;
    unsigned rev2              : 1;
    unsigned burst_mode        : 1;
    unsigned addr_mask_mode    : 4;   // 4 bytes
}F2_DATA_HI_LO_STRUCT;

int US363_SPI_Open();
void US363_SPI_Close();
int SPI_Write_IO_S2( int io_idx, int *data , int size);
int SPI_Read_IO_S2( int io_idx, int *data, int size);
int ua360_spi_ddr_read( int read_addr, int *read_buf, int read_size, int f_id, int fx_x);
int ua360_spi_ddr_write(int write_addr, int *write_buff, int write_size);
int spi_read_io_porcess_S2(int addr, int *data, int size);

int ua360_qspi_ddr_write(int write_addr, int *write_buff, int write_size);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__US363_SPI_H__