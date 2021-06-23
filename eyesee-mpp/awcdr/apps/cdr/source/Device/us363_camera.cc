/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/us363_camera.h"

#include <mpi_sys.h>
#include <mpi_venc.h>

#include "Device/spi.h"
#include "Device/qspi.h"
#include "Device/US363/us360.h"
#include "Device/US363/Kernel/k_spi_cmd.h"
#include "Device/US363/Driver/Lidar/lidar.h"
#include "Device/US363/Net/ux363_network_manager.h"
#include "Device/US363/Net/ux360_wifiserver.h"
#include "Device/US363/Data/databin.h"
#include "Device/US363/Data/wifi_config.h"
#include "Device/US363/Data/pcb_version.h"
#include "Device/US363/Data/customer.h"
#include "Device/US363/Data/country.h"
#include "Device/US363/Data/us363_folder.h"
#include "Device/US363/System/sys_time.h"
#include "Device/US363/System/sys_cpu.h"
#include "Device/US363/System/sys_power.h"
#include "Device/US363/Test/test.h"

#include "device_model/display.h"
#include "device_model/media/camera/camera.h"
#include "device_model/media/camera/camera_factory.h"
#include "device_model/storage_manager.h"
//#include "device_model/system/uevent_manager.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "US363Camera"

//==================== parameter ====================

char mWifiApSsid[32] = "US_0000\0";             /** ssid */
char mWifiApPassword[16] = "88888888\0";        /** pwd */
int mWifiApChannel = 6;

//==================== variable ==================== 


//==================== get/set =====================


//==================== fucntion ====================

void destroyCamera()
{
	free_us360_buf();
	free_wifiserver_buf();
	free_lidar_buf();
	SPI_Close();
	QSPI_Close();
}

int initSysConfig() 
{
	MPP_SYS_CONF_S sys_config{};
	memset(&sys_config, 0, sizeof(sys_config));
	sys_config.nAlignWidth = 32;
	AW_MPI_SYS_SetConf(&sys_config);
	return AW_MPI_SYS_Init_S1();
}

void initCamera()
{
	if(malloc_us360_buf() < 0) 	    goto end;
	if(malloc_wifiserver_buf() < 0) goto end;
//  if(malloc_lidar_buf() < 0)	    goto end;			// 記憶體需求過大, 導致程式無法執行

	initSysConfig();

	if(EyeseeLinux::StorageManager::GetInstance()->IsMounted() ) {
		makeUS360Folder();
		makeTestFolder();
		readWifiConfig(&mWifiApSsid[0], &mWifiApPassword[0]);
	}
	else {
		getNewSsidPassword(&mWifiApSsid[0], &mWifiApPassword[0]);
	}
	db_debug("US363Camera() ssid=%s pwd=%s", mWifiApSsid, mWifiApPassword);

	stratWifiAp(&mWifiApSsid[0], &mWifiApPassword[0], mWifiApChannel, 0);
	start_wifi_server(8555);
	
	us360_init();
	return;
	
end:
	db_debug("US363Camera() Error!");
	destroyCamera();
    return;
}

void startPreview()
{
#if 1 //max+
	EyeseeLinux::Camera* camera =
      EyeseeLinux::CameraFactory::GetInstance()->CreateCamera(
          EyeseeLinux::CAM_NORMAL_0);
	camera->StartPreview();
	camera->ShowPreview();
#endif
}