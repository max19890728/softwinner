/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/US363/Cmd/fpga_download.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <malloc.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include "common/app_log.h"
#include "Device/gpio.h"
#include "Device/spi.h"
#include "Device/US363/Cmd/us363_spi.h"
#include "Device/US363/Cmd/spi_cmd.h"
#include "Device/US363/Cmd/fpga_driver.h"
#include "Device/US363/Kernel/k_spi_cmd.h"

#undef LOG_TAG
#define LOG_TAG "US363::FPGADownload"

const char* _fpga_path_       = "/usr/share/minigui/res/fpga/S2_BitStream.bin\0";
const char* _fpga_debug_path_ = "/mnt/sdcard/FPGA_Debug/S2_BitStream.bin\0";

char* fpga_data = NULL;
int fpga_size = 0;
int init_fpga_checksum = 0;
int fpga_checksum = 0;

int ReadFile() {
	int ret = -1;
    int size = 0;
    struct stat sti;
    FILE *fp = NULL;
	const char* path;
	
	if(-1 == stat(_fpga_debug_path_, &sti) )		//fpga debug 不存在
		path = _fpga_path_;
	else											//fpga debug 存在
		path = _fpga_debug_path_;
	
	if(-1 == stat(path, &sti) ) {			//不存在
		db_error("fpag file not find! path=%s\n", path);
		goto error;
	}
	else {											//存在
		fp = fopen(path, "rb");
		if(fp != NULL) {
			size = sti.st_size;
			if(fpga_data == NULL) {
				fpga_data = (char*)malloc(size);
			}

			if(fpga_data != NULL){
				ret = fread(fpga_data, size, 1, fp);
				if(ret <= 0) 
					goto error;
				fpga_size = size;
			}
			else {
				db_error("malloc fpga_data error!\n");
				goto error;
			}
		}
		else {
			db_error("file open error!\n");
			goto error;
		}
	}
	
	if(fp != NULL) 
		fclose(fp);
    return 0;
error:
	if(fp != NULL) 
		fclose(fp);
	return -1;
}

void CalFpgaCheckSum(const char* buf, const int size) {
    for(int i=0; i<size; i++) {
        fpga_checksum += (int)buf[i];
    }
    db_debug("fpga_checksum = 0x%x\n", fpga_checksum);
}

int GetFpgaCheckSum() {
	return fpga_checksum;
}

// PG13:nCONFIG  PG14:CONF_DONE  PG17:FPGA Power  PG18:Sensor Power
int FpgaDownload(const char *buf, const int size) {
    struct stat sti;
    int spi_fd = -1;
    int ret = 0;

	ret = GPIO_Open();
    if(ret < 0) 
		return -1;
    db_debug("fpga download start! size=%d\n", fpga_size);
	
	ret = GPIO_IOCTL(IOCTL_GPIO_HIGH, 18);
    if(ret < 0) {
        GPIO_Close();
        return -3;
    }
    usleep(10000);

    if(init_fpga_checksum == 0) {
        CalFpgaCheckSum(buf, size);
        init_fpga_checksum = 1;
    }

	if(SPI_Open() < 0) {
		db_error("FpgaDownload spi open error!\n");
		return -11;
	}

    // FPGA Download ======================================
    for(int j=0; j<5; j++) {
		ret = GPIO_IOCTL(IOCTL_GPIO_HIGH, 13);
        if(ret < 0) {
            GPIO_Close();
            return -3;
        }
        usleep(10000);

		ret = GPIO_IOCTL(IOCTL_GPIO_LOW, 13);
        if(ret < 0) {
            GPIO_Close();
            return -5;
        }
        usleep(1);

        //read CONF_DONE
        ret = -1;
		ret = GPIO_IOCTL(IOCTL_GPIO_INPUT, 14);
        if(ret != 0) {
            db_error("read CONF_DONE 00 goto start! ret=%d\n", ret);
            if(j == 4) {
                GPIO_Close();
                return -6;
            }
            else       
				continue;
        }

		ret = GPIO_IOCTL(IOCTL_GPIO_HIGH, 13);
        if(ret < 0) {
            GPIO_Close();
            return -7;
        }
        usleep(2000);		//delay 2ms

        pthread_mutex_lock(&pthread_spi_mutex);
        check_spi_irq_state("fpgaDownload", 1);
        //send data
        for(int i=0; i<size; i+=2048) {
            if( (i + 2048) < size)
				ret = SPI_Write(&buf[i], 2048);
            else
				ret = SPI_Write(&buf[i], size & 2047);

            if (ret < 0) {
                GPIO_Close();
                check_spi_irq_state("fpgaDownload", 0);
                pthread_mutex_unlock(&pthread_spi_mutex);           
                return -8;
            }
        }
        check_spi_irq_state("fpgaDownload", 0);
        pthread_mutex_unlock(&pthread_spi_mutex);
        usleep(1000);		//delay 1ms

        //read CONF_DONE
        ret = -1;
		ret = GPIO_IOCTL(IOCTL_GPIO_INPUT, 14);
        if(ret != 1)  {
            db_error("read CONF_DONE 01 goto start! ret %d\n", ret);
            if(j == 4) {
                GPIO_Close();
                return -9;
            }
            else        
				continue;
        }
        else break;
    }
    // FPGA Download ======================================

    GPIO_Close();
    return 0;
}

//extern int Waiting_State;
int DownloadProc() {
	int ret;
    int c_mode = getCameraMode();
	
	if(ReadFile() < 0)
		return -1;

//tmp    setImgReadyFlag(0);
    for(int j=0; j<3; j++) {
//tmp        Waiting_State = 1;
        ret = FpgaDownload(fpga_data, fpga_size);
        if(ret < 0) {
        	db_error("fpga download error ret=%d\n", ret);
           	continue;
        }
           
        for(int i=0; i<5; i++) {  
//tmp            Waiting_State = 2;
            writeSPIUSB(); 
//tmp            ret = checkUVC();
            if(ret == 0) {
//tmp                Waiting_State = 3;
                break;
            }
        }

        if(ret == 0)
            break;
    }
	
    // init other function
    set_Init_Gamma_Table_En();         // rex+ 180913
//tmp    if(c_mode == CAMERA_MODE_NIGHT || c_mode == CAMERA_MODE_NIGHT_HDR || c_mode == CAMERA_MODE_M_MODE)		//Night / NightHDR / M-Mode
//tmp      	Set_Skip_Frame_Cnt(3);
//tmp    else
//tmp       	Set_Skip_Frame_Cnt(6);
		
    return ret;
}

/*
 *     關閉FPGA Power
 */
int fpgaPowerOff() {
    // close fpga power
//tmp	setImgReadyFlag(0);
	setSensorPowerOff();
	FPGA_Sleep_En = 0;	//SetFPGASleepEn(0);	//tmp

    int ret;
	ret = GPIO_Open();
    if(ret < 0) {
		db_error("fpgaPowerOff gpio open error!\n");
		return -1;
	}

	ret = GPIO_IOCTL(IOCTL_GPIO_LOW, 13);
    if(ret < 0) {
        db_error("fpgaPowerOff: -3 ret=%d\n", ret);
        GPIO_Close();
        return -3;
    }
    usleep(100);

	ret = GPIO_IOCTL(IOCTL_GPIO_HIGH, 13);
	if(ret < 0) {
		db_error("fpgaPowerOff: -3 ret=%d\n", ret);
		GPIO_Close();
		return -4;
	}

    GPIO_Close();
    return 0;
}