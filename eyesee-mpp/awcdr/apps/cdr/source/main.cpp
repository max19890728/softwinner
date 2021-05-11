/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "source/main.h"

#include <thread>
#include <utility>
#include <iostream>
#include <mpi_sys.h>

#include "DebugKit/updater.h"
#include "Device/audio.h"
#include "Device/us363_camera.h"
#include "Device/frame_buffer.h"
#include "Framework/UIKit.h"
#include "US363/preview_view_controller.h"
#include "common/app_def.h"
#include "common/app_log.h"
#include "device_model/storage_manager.h"

#undef LOG_TAG
#define LOG_TAG "main.cpp"

sem_t g_app_exit;
int g_exit_action = EXIT_APP;

//Device::US363Camera *US363;

int Main(int args, const char *argv[]) {
/*
  US363 = &Device::US363Camera::instance();
  std::thread([] { US363->InitCamera(); }).join();
  std::thread([] { US363->StartPreview(); }).join();
*/  
  std::thread([] { initCamera(); }).join();
  std::thread([] { startPreview(); }).join();
  
  std::thread([] { Device::FrameBuffer::instance().Clean(); }).join();
  // std::thread([] { AW_MPI_VENC_SetVEFreq(MM_INVALID_CHN, 520); }).detach();
  std::thread([args, argv] { InitGUI(args, argv); }).join();
  std::thread([] { AW_MPI_SYS_Init_S2(); }).join();
#if AUDIO_ENABLE
  std::thread([] {
    Device::Audio::instance().Play(Device::Audio::Sound::start_up);
  }).detach();
#endif
  SetKeyLongPressTime(100);
  if (DebugKit::Updater::instance().CheckFirmwareFileExist()) {
    DebugKit::Updater::instance().StartUpdate();
  } else {
    //EyeseeLinux::StorageManager::GetInstance()->MountToPC();
    auto window = UI::Window::init();

    window->root_viewcontroller(PreviewViewController::init());
    window->MakeKeyAndVisible();
  }
  
  return 0;
}
