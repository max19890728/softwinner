#pragma once

#define CMD_READY_FILE_NAME				"fpga_dbt_cmd_ready.bin"
#define DATA_READY_FILE_NAME		    "fpga_dbt_data_ready.bin"
#define FPGA_DBT_CMD_FILE_NAME			"fpga_dbt_cmd.bin"
#define FPGA_DBT_DDR_DATA_FILE_NAME		"fpga_dbt_ddr_data.bin"
#define FPGA_DBT_REG_DATA_FILE_NAME		"fpga_dbt_reg_data.bin"

/* SD Card Speed = 1.2 MB/s */
#define FPGA_DBT_DDR_BUF_MAX		0x1000000

#define FPGA_DBT_READ_TIMEOUT		20

enum {
	DBT_STATE_NONE = 0,
	DBT_STATE_WRITE_CMD = 1,
	DBT_STATE_CHECK_DATA_READY = 2,
	DBT_STATE_READ_DATA = 3,
	DBT_STATE_SHOW_DATA = 4
};

enum {
	FPGA_RW_TYPE_DDR = 0,
	FPGA_RW_TYPE_REG
};

enum {
	FPGA_READ = 0,
	FPGA_WRITE
};

enum {
	DBT_INTERFACE_SPI  = 0,
	DBT_INTERFACE_QSPI = 1,
    DBT_INTERFACE_MIPI = 2
};

typedef struct fpga_dbt_rw_cmd_struct_h {
	unsigned type;			    //0:DDR 1:Reg
	unsigned rw;				//0:read 1:write
	unsigned addr;
	unsigned size;			    //byte
    unsigned intf;         //0:spi 1:qspi 2:mipi
	char rev[44];
}fpga_dbt_rw_cmd_struct;		//64 byte

typedef struct fpga_ddr_rw_struct_h {
	fpga_dbt_rw_cmd_struct cmd;
//#ifndef __linux__    
//	char buf[FPGA_DBT_DDR_BUF_MAX];
//#else
    char *buf;
//#endif    
}fpga_ddr_rw_struct;

typedef struct fpga_reg_rw_struct_h {
	fpga_dbt_rw_cmd_struct cmd;
	unsigned data;
}fpga_reg_rw_struct;

//#ifdef __linux__
int malloc_fpga_ddr_rw_buf(fpga_ddr_rw_struct* ddr_p);
void free_fpga_ddr_rw_buf(fpga_ddr_rw_struct* ddr_p);
//#endif
unsigned long long getSystemTimeSec();
int findDiskPath();
int diskPathIsExist();
void setFpagDbtDdrState(int state);
int getFpagDbtDdrState();
void setFpagDbtRegState(int state);
int getFpagDbtRegState();
void fpgaDbtReadWriteDdrProc(fpga_ddr_rw_struct* ddr_p);
void fpgaDbtReadWriteRegProc(fpga_reg_rw_struct* reg_p);

int readFpgaDbtCmdHeader(fpga_dbt_rw_cmd_struct* cmd_p);
int readFpgaDbtDdrCmd(fpga_ddr_rw_struct *ddr_p);
int readFpgaDbtRegCmd(fpga_reg_rw_struct* reg_p);
int writeFpgaDbtDdrCmd(fpga_ddr_rw_struct *ddr_p);
int writeFpgaDbtRegCmd(fpga_reg_rw_struct* reg_p);
void deleteFpgaDbtCmd();

int writeFpgaDbtCmdReady();
int writeFpgaDbtDataReady();
int checkFpgaDbtCmdReady();
int checkFpgaDbtDataReady();
void deleteFpgaDbtCmdReady();
void deleteFpgaDbtDataReady();

int readFpgaDbtDdr(fpga_ddr_rw_struct* ddr_p);
int writeFpgaDbtDdr(fpga_ddr_rw_struct* ddr_p);
int readFpgaDbtReg(fpga_reg_rw_struct *reg_p);
int writeFpgaDbtReg(fpga_reg_rw_struct *reg_p);


