/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "source/main.h"

#include <System/updater.h>
#include <UIKit.h>

#include <iostream>
#include <thread>
#include <utility>
#include <iostream>
#include <mpi_sys.h>

#include "Device/us363_camera.h"
#include "Device/audio.h"
#include "Device/frame_buffer.h"
#include "US363/Updater/updating_view_controller.h"
#include "US363/preview_view_controller.h"
#include "common/app_def.h"
#include "common/app_log.h"
#include "device_model/storage_manager.h"
#include "device_model/system/net/net_manager.h"

#undef LOG_TAG
#define LOG_TAG "main.cpp"

sem_t g_app_exit;
int g_exit_action = EXIT_APP;

int Main(int args, const char *argv[]) {
printf("Main() 00\n");    
  std::thread([] { Device::FrameBuffer::instance().Clean(); }).join();
  // std::thread([] { AW_MPI_VENC_SetVEFreq(MM_INVALID_CHN, 520); }).detach();
  std::thread([args, argv] { InitGUI(args, argv); }).join();
  std::thread([] { AW_MPI_SYS_Init_S2(); }).join();
#if AUDIO_ENABLE
  std::thread([] {
    Device::Audio::instance().Play(Device::Audio::Sound::start_up);
  }).detach();
#endif
printf("Main() 01\n");   
  do {
    auto window = UI::Window::init();
    UIViewController root_viewcontroller;
    if (System::Updater::instance().CheckFirmwareFileExist()) {
      root_viewcontroller = UpdatingViewController::init();
    } else {
      EyeseeLinux::StorageManager::GetInstance()->MountToPC();
printf("Main() 02\n");         
      initCamera();
printf("Main() 03\n");   
      root_viewcontroller = PreviewViewController::init();
    }
    window->root_viewcontroller(root_viewcontroller);
printf("Main() 04\n");       
    window->MakeKeyAndVisible();
printf("Main() 05\n");       
  } while (false);
printf("Main() 06\n");   
  destroyCamera();
  return 0;
}
