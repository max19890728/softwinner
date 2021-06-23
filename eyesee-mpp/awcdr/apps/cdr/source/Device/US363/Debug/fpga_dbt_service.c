/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 * Fpga Debug Tool: V536 Side Code
 ******************************************************************************/

#include "Device/US363/Debug/fpga_dbt_service.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "Device/US363/us360.h"
#include "Device/US363/Cmd/spi_cmd.h"
#include "Device/US363/Cmd/spi_cmd_s3.h"
#include "Device/US363/Cmd/us363_spi.h"
#include "Device/US363/Debug/fpga_dbt.h"
#include "Device/US363/Net/ux360_wifiserver.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "FpgaDbtService"


//fpga_ddr_rw_struct ddrReadWriteCmd;
//fpga_reg_rw_struct regReadWriteCmd;

int fpgaDbtReadWriteDdr(fpga_ddr_rw_struct* ddr_p) {
    int i, addr = 0;
    int total_size = 0, step_size = 0, size = 0;
    int *buf_p;
    if(ddr_p->cmd.rw == FPGA_READ) {
        if(ddr_p->cmd.intf == DBT_INTERFACE_SPI) {
            step_size = 3072;
            total_size = ddr_p->cmd.size;
            for(i = 0; i < total_size; i += step_size) {
                if((total_size-i) < step_size)
                    size = (total_size-i);
                else
                    size = step_size; 
                addr = ddr_p->cmd.addr + i;
                buf_p = (int *)&ddr_p->buf[i];
                ua360_spi_ddr_read(addr, buf_p, size, 2, 0);
            }
        }
        /*else if(ddr_p->cmd.intf == DBT_INTERFACE_QSPI) {
            //
        }
        else if(ddr_p->cmd.intf == DBT_INTERFACE_MIPI){
            //
        }*/
    }
    else {
        if(ddr_p->cmd.intf == DBT_INTERFACE_SPI || ddr_p->cmd.intf == DBT_INTERFACE_QSPI) {
            step_size = 3072;
            total_size = ddr_p->cmd.size;
            for(i = 0; i < total_size; i += step_size) {
                if((total_size-i) < step_size)
                    size = (total_size-i);
                else
                    size = step_size; 
                addr = ddr_p->cmd.addr + i;
                buf_p = (int *)&ddr_p->buf[i];
                
                if(ddr_p->cmd.intf == DBT_INTERFACE_SPI)
                    ua360_spi_ddr_write(addr, buf_p, size);
                else if(ddr_p->cmd.intf == DBT_INTERFACE_QSPI)
                    ua360_qspi_ddr_write(addr, buf_p, size);
            }
        }
        /*else if(ddr_p->cmd.intf == DBT_INTERFACE_MIPI){
            //
        }*/
    }
    return 0;
}

int fpgaDbtReadWriteReg(fpga_reg_rw_struct* reg_p) {
	unsigned Data[2];
    if(reg_p->cmd.rw == FPGA_READ) {
        memset(&Data[0], 0, sizeof(Data) );
        spi_read_io_porcess_S2(reg_p->cmd.addr, (int *) &Data[0], 4);
        reg_p->data = Data[0];
    }
    else {
        Data[0] = reg_p->cmd.addr;
        Data[1] = reg_p->data;
        SPI_Write_IO_S2(0x9, (int *)&Data[0], 8);
    }    
}

void fpgaDbtReadWriteDdrService(fpga_ddr_rw_struct* ddr_p) {          
    fpgaDbtReadWriteDdr(ddr_p);
    if(ddr_p->cmd.rw == FPGA_READ) 
        setDbtOutputDdrDataEn(1);    
    else
        setDbtInputDdrDataFinish(1);
}

void fpgaDbtReadWriteRegService(fpga_reg_rw_struct* reg_p) {            
    fpgaDbtReadWriteReg(reg_p);
    if(reg_p->cmd.rw == FPGA_READ)
        setDbtOutputRegDataEn(1);
    else
        setDbtInputRegDataFinish(1);  
}