/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 * Fpga Debug Tool: PC Side Code
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <malloc.h>

#ifndef __linux__
 #include <io.h>
 #include <string>
 #include "fpga_dbt.h"
 
 using namespace System;
 using namespace System::IO;

#else
 #include <sys/stat.h>
 #include <sys/time.h>
 #include "Device/US363/Debug/fpga_dbt.h"

#endif


/* Read/Write File */
// PC:
/* Write DDR: 1.Write Cmd File -> 2.Write Cmd Ready File */
/* Read DDR: 1.Write Cmd File -> 2.Write Cmd Ready File -> 9.Check Data Ready FIle -> 10.Read Data File -> 11.Delete Data Ready File */
// V536:
/* Write DDR: 3.Check Cmd Ready File -> 4.Read Cmd File -> 5.Delete Cmd Ready File -> 6.Write DDR */
/* Read DDR:  3.Check Cmd Ready File -> 4.Read Cmd File -> 5.Delete Cmd Ready File -> 6.Read DDR -> 7.Write Data File -> 8.Write Data Ready File */

/* Event */
// PC:
/* Write DDR: 1.Write Cmd File -> 2.Send Key Event */
/* Read DDR: 1.Write Cmd File -> 2.Send Key Event -> 8.Check Data Ready FIle -> 9.Read Data File -> 10.Delete Data Ready File */
// V536:
/* Write DDR: 3.Get Key Event -> 4.Read Cmd File -> 5.Write DDR */
/* Read DDR:  3.Get Key Event -> 4.Read Cmd File -> 5.Read DDR -> 6.Write Data File -> 7.Write Data Ready File */

#ifndef __linux__
 char diskPath[128] = "C:\\DBT\0";
#else
 char diskPath[128] = "/mnt/extsd/\0";   
#endif

//int fpgaDbtDdrState = DBT_STATE_NONE;
//int fpgaDbtRegState = DBT_STATE_NONE;

//void setFpagDbtDdrState(int state) { fpgaDbtDdrState = state; }
//int getFpagDbtDdrState() { return fpgaDbtDdrState; }

//void setFpagDbtRegState(int state) { fpgaDbtRegState = state; }
//int getFpagDbtRegState() { return fpgaDbtRegState; }

//#ifdef __linux__
int malloc_fpga_ddr_rw_buf(fpga_ddr_rw_struct* ddr_p) {
	ddr_p->buf = (char *)malloc(sizeof(char) * FPGA_DBT_DDR_BUF_MAX);
	if(ddr_p->buf == NULL) goto error;
	return 0;
error:
	printf("malloc_fpga_ddr_rw_buf() malloc error!\n");
	return -1;
}

void free_fpga_ddr_rw_buf(fpga_ddr_rw_struct* ddr_p) {
	if(ddr_p->buf != NULL)
		free(ddr_p->buf);
	ddr_p->buf = NULL;
}
//#endif

unsigned long long getSystemTimeSec() {
	unsigned long long sys_time;
#ifndef __linux__    
	time_t timer = time(NULL);
	tm* tt;
	tt = localtime(&timer);
	sys_time = (tt->tm_hour*60*60 + tt->tm_min*60 + tt->tm_sec);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    sys_time = (unsigned long long)tv.tv_sec;
#endif    
	return sys_time;
}

//array<DriveInfo^>^ allDrives = DriveInfo::GetDrives();
int findDiskPath() {
	int ret = 0;
#ifndef __linux__
	/*
	cmd > MOUNTVOL
	\\ ? \Volume{ 95244fbc - 87f1 - 11eb - bf52 - 88d7f6a48e99 }\
		G:\
	*/
	FILE *fp;
	char cmd_buf[128];
	char disk_line[128];
	char disk_name = '\0';
 #ifndef __linux__     
	if ((fp = _popen("MOUNTVOL", "rt")) == NULL)
        return disk_name;
 #else
    if ((fp = popen("MOUNTVOL", "rt")) == NULL)
        return disk_name;
 #endif    

	/* 
	Read pipe until end of file, or an error occurs. 
	Find v536 Disk. 
	*/
	while (fgets(cmd_buf, 128, fp)) {
		puts(cmd_buf);
		if (strstr(cmd_buf, "95244fbc-87f1-11eb-bf52-88d7f6a48e99") != NULL) {
			fgets(disk_line, 128, fp);
			disk_name = *(strchr(disk_line, ':')-1);
			snprintf(diskPath, sizeof(diskPath), "%C://\0", disk_name);
			ret = 1;
			break;
		}
	}

	/* Close pipe and print return value of pPipe. */
	if (feof(fp)) {
 #ifndef __linux__        
		printf("\nProcess returned %d\n", _pclose(fp));
 #else
        printf("\nProcess returned %d\n", pclose(fp));
 #endif    
	} else {
		printf("Error: Failed to read the pipe to the end.\n");
	}
#else
    snprintf(diskPath, sizeof(diskPath), "/mnt/extsd/\0");
    ret = 1;
#endif
	return ret;
}

int diskPathIsExist() {
    struct stat st;
    if(stat(diskPath, &st) == 0)		//exist
		return 1;
	else                                            
		return 0;
}

int readFpgaDbtCmdHeader(fpga_dbt_rw_cmd_struct* cmd_p) {
    FILE* fp;
    char path[128];
    snprintf(path, sizeof(path), "%s%s\0", diskPath, FPGA_DBT_CMD_FILE_NAME);
	fp = fopen(path, "rb");
	if (fp != NULL) {
		if (fread(cmd_p, 1, sizeof(fpga_dbt_rw_cmd_struct), fp) <= 0)
			return -1;
		fclose(fp);
	}
	else
		return -1;
	return 1;
}

int readFpgaDbtCmd(fpga_dbt_rw_cmd_struct* cmd_p, char* data_p) {
	FILE* fp;
    char path[128];
    snprintf(path, sizeof(path), "%s%s\0", diskPath, FPGA_DBT_CMD_FILE_NAME);
	fp = fopen(path, "rb");
	if (fp != NULL) {
		if (fread(cmd_p, 1, sizeof(fpga_dbt_rw_cmd_struct), fp) <= 0)
			return -1;
		if (cmd_p->rw == FPGA_WRITE) {
			if (fread(data_p, 1, cmd_p->size, fp) <= 0)
				return -1;
		}
		fclose(fp);
	}
	else
		return -1;
	return 1;
}

int readFpgaDbtDdrCmd(fpga_ddr_rw_struct *ddr_p) {
	char *data_p;
	fpga_dbt_rw_cmd_struct* cmd_p;

	cmd_p = &ddr_p->cmd;
	data_p = &ddr_p->buf[0];
	return readFpgaDbtCmd(cmd_p, data_p);
}

int readFpgaDbtRegCmd(fpga_reg_rw_struct* reg_p) {
	char* data_p;
	fpga_dbt_rw_cmd_struct* cmd_p;

	cmd_p = &reg_p->cmd;
	data_p = (char* )&reg_p->data;
	return readFpgaDbtCmd(cmd_p, data_p);
}

int writeFpgaDbtCmd(fpga_dbt_rw_cmd_struct* cmd_p, char* data_p) {
	FILE* fp;
    char path[128];
    snprintf(path, sizeof(path), "%s%s\0", diskPath, FPGA_DBT_CMD_FILE_NAME);
	fp = fopen(path, "wb");
	if (fp != NULL) {
		if (fwrite(cmd_p, 1, sizeof(fpga_dbt_rw_cmd_struct), fp) <= 0)
			return -1;
		if (cmd_p->rw == FPGA_WRITE) {
			if (fwrite(data_p, 1, cmd_p->size, fp) <= 0)
				return -1;
		}
		fclose(fp);
	}
	else
		return -1;
	return 1;
}

int writeFpgaDbtDdrCmd(fpga_ddr_rw_struct *ddr_p) {
	char *data_p;
	fpga_dbt_rw_cmd_struct* cmd_p;

	cmd_p = &ddr_p->cmd;
	data_p = &ddr_p->buf[0];
	return writeFpgaDbtCmd(cmd_p, data_p);
}

int writeFpgaDbtRegCmd(fpga_reg_rw_struct* reg_p) {
	char* data_p;
	fpga_dbt_rw_cmd_struct* cmd_p;

	cmd_p = &reg_p->cmd;
	data_p = (char* )&reg_p->data;
	return writeFpgaDbtCmd(cmd_p, data_p);
}

void deleteFpgaDbtCmd() {
	char path[128];
	snprintf(path, sizeof(path), "%s%s\0", diskPath, FPGA_DBT_CMD_FILE_NAME);
	remove(&path[0]);
}

int writeReadyFile(char* path) {
	FILE* fp;
	int ready = 1;

	fp = fopen(path, "wb");
	if (fp != NULL) {
		if (fwrite(&ready, 1, sizeof(ready), fp) <= 0) 
			return -1;
		fclose(fp);
	}
	else
		return -1;
	return 1;
}

int writeFpgaDbtCmdReady() {
	char path[128];
	snprintf(path, sizeof(path), "%s%s\0", diskPath, CMD_READY_FILE_NAME);
	if (writeReadyFile(&path[0]) < 0) {
		printf("writeFpgaDbtCmdReady: Write File Error!\n");
		return -1;
	}
	else
		return 1;
}

int writeFpgaDbtDataReady() {
	char path[128];
	snprintf(path, sizeof(path), "%s%s\0", diskPath, DATA_READY_FILE_NAME);
	if (writeReadyFile(&path[0]) < 0) {
		printf("writeFpgaDbtDataReady: Write File Error!\n");
		return -1;
	}
	else
		return 1;
}

int checkFpgaDbtCmdReady() {
    struct stat st;
	char path[128];
	snprintf(path, sizeof(path), "%s%s\0", diskPath, CMD_READY_FILE_NAME);
printf("checkFpgaDbtCmdReady: path=%s\n", path);    
    if(stat(path, &st) == 0)		//exist
		return 1;
	else
		return 0;
}

int checkFpgaDbtDataReady() {
    struct stat st;
	char path[128];
	snprintf(path, sizeof(path), "%s%s\0", diskPath, DATA_READY_FILE_NAME);
    if(stat(path, &st) == 0)		//exist
		return 1;
	else
		return 0;
}

void deleteFpgaDbtCmdReady() {
	char path[128];
	snprintf(path, sizeof(path), "%s%s\0", diskPath, CMD_READY_FILE_NAME);
	remove(&path[0]);
}

void deleteFpgaDbtDataReady() {
	char path[128];
	snprintf(path, sizeof(path), "%s%s\0", diskPath, DATA_READY_FILE_NAME);
	remove(&path[0]);
}

int readFpgaDbtDdr(fpga_ddr_rw_struct* ddr_p) {
	int size;
	FILE* fp;
	struct stat st;
	char path[128];

	snprintf(path, sizeof(path), "%s%s\0", diskPath, FPGA_DBT_DDR_DATA_FILE_NAME);
	stat(path, &st);
	size = st.st_size;
	if (size >= FPGA_DBT_DDR_BUF_MAX)
		size = FPGA_DBT_DDR_BUF_MAX;

	fp = fopen(path, "rb");
	if (fp != NULL) {
		if (fread(&ddr_p->buf[0], 1, size, fp) <= 0)
			return -1;
		fclose(fp);
	}
	else
		return -1;
	return 1;
}

int writeFpgaDbtDdr(fpga_ddr_rw_struct* ddr_p) {
	int size;
	FILE* fp;
	struct stat st;
	char path[128];

    size = ddr_p->cmd.size;
    if(size >= FPGA_DBT_DDR_BUF_MAX)
        size = FPGA_DBT_DDR_BUF_MAX;
    
	snprintf(path, sizeof(path), "%s%s\0", diskPath, FPGA_DBT_DDR_DATA_FILE_NAME);
	fp = fopen(path, "wb");
	if (fp != NULL) {
		if (fwrite(&ddr_p->buf[0], 1, size, fp) <= 0)
			return -1;
		fclose(fp);
	}
	else
		return -1;
	return 1;
}

void deleteFpgaDbtDdr() {
	char path[128];
	snprintf(path, sizeof(path), "%s%s\0", diskPath, FPGA_DBT_DDR_DATA_FILE_NAME);
	remove(&path[0]);
}

int readFpgaDbtReg(fpga_reg_rw_struct *reg_p) {
	FILE* fp;
	char path[128];

	snprintf(path, sizeof(path), "%s%s\0", diskPath, FPGA_DBT_REG_DATA_FILE_NAME);
	fp = fopen(path, "rb");
	if (fp != NULL) {
		if (fread(&reg_p->data, 1, sizeof(reg_p->data), fp) <= 0)
			return -1;
		fclose(fp);
	}
	else
		return -1;
	return 1;
}

int writeFpgaDbtReg(fpga_reg_rw_struct *reg_p) {
	FILE* fp;
	char path[128];

	snprintf(path, sizeof(path), "%s%s\0", diskPath, FPGA_DBT_REG_DATA_FILE_NAME);
	fp = fopen(path, "wb");
	if (fp != NULL) {
		if (fwrite(&reg_p->data, 1, sizeof(reg_p->data), fp) <= 0)
			return -1;
		fclose(fp);
	}
	else
		return -1;
	return 1;
}

void deleteFpgaDbtReg() {
	char path[128];
	snprintf(path, sizeof(path), "%s%s\0", diskPath, FPGA_DBT_REG_DATA_FILE_NAME);
	remove(&path[0]);
}

/*void fpgaDbtReadWriteDdrProc(fpga_ddr_rw_struct* ddr_p)
{
	static unsigned long long read_time, now_time;

	switch (fpgaDbtDdrState) {
	case DBT_STATE_WRITE_CMD:
		if (writeFpgaDbtDdrCmd(ddr_p) == 1) {
			if (writeFpgaDbtCmdReady() == 1) {
				if(ddr_p->cmd.rw == FPGA_READ)
					fpgaDbtDdrState = DBT_STATE_CHECK_DATA_READY;
				else
					fpgaDbtDdrState = DBT_STATE_NONE;
			}
			else
				fpgaDbtDdrState = DBT_STATE_NONE;
		}
		else
			fpgaDbtDdrState = DBT_STATE_NONE;
		read_time = getSystemTimeSec();
		break;
	case DBT_STATE_CHECK_DATA_READY:
		//check ready
		if(checkFpgaDbtDataReady())
			fpgaDbtDdrState = DBT_STATE_READ_DATA;
		//check timeout
		now_time = getSystemTimeSec();
		if((now_time- read_time) >= FPGA_DBT_READ_TIMEOUT)
			fpgaDbtDdrState = DBT_STATE_NONE;
		break;
	case DBT_STATE_READ_DATA:
		readFpgaDbtDdr(ddr_p);
        deleteFpgaDbtDdr();
		deleteFpgaDbtCmdReady();
		fpgaDbtDdrState = DBT_STATE_SHOW_DATA;
		read_time = getSystemTimeSec();
		break;
	case DBT_STATE_SHOW_DATA:
		//check timeout
		now_time = getSystemTimeSec();
		if ((now_time - read_time) >= FPGA_DBT_READ_TIMEOUT)
			fpgaDbtDdrState = DBT_STATE_NONE;
		break;
	}
}*/

/*void fpgaDbtReadWriteRegProc(fpga_reg_rw_struct* reg_p)
{
	static unsigned long long read_time, now_time;

	switch (fpgaDbtRegState) {
	case DBT_STATE_WRITE_CMD:
		if (writeFpgaDbtRegCmd(reg_p) == 1) {
			if (writeFpgaDbtCmdReady() == 1) {
				if(reg_p->cmd.rw == FPGA_READ)
					fpgaDbtRegState = DBT_STATE_CHECK_DATA_READY;
				else
					fpgaDbtRegState = DBT_STATE_NONE;
			}
			else
				fpgaDbtRegState = DBT_STATE_NONE;
		}
		else
			fpgaDbtRegState = DBT_STATE_NONE;
		read_time = getSystemTimeSec();
		break;
	case DBT_STATE_CHECK_DATA_READY:
		//check ready
		if (checkFpgaDbtDataReady())
			fpgaDbtRegState = DBT_STATE_READ_DATA;
		//check timeout
		now_time = getSystemTimeSec();
		if ((now_time - read_time) >= FPGA_DBT_READ_TIMEOUT)
			fpgaDbtRegState = DBT_STATE_NONE;
		break;
	case DBT_STATE_READ_DATA:
		readFpgaDbtReg(reg_p);
        deleteFpgaDbtReg();
		deleteFpgaDbtCmdReady();
		fpgaDbtRegState = DBT_STATE_SHOW_DATA;
		read_time = getSystemTimeSec();
		break;
	case DBT_STATE_SHOW_DATA:
		//check timeout
		now_time = getSystemTimeSec();
		if ((now_time - read_time) >= FPGA_DBT_READ_TIMEOUT)
			fpgaDbtRegState = DBT_STATE_NONE;
		break;
	}
}*/