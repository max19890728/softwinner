/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/CameraSetting/CameraSetting/camera_setting_manager.h"

#include <common/app_log.h>

#include "UIKit/Struct/bundle.h"

#undef LOG_TAG
#define LOG_TAG "CameraSettingManager"

namespace SettingManager {

UI::Bundle CameraSettingManager::bundle_ = UI::Bundle{"/data/US363/Setting/"};

CameraSettingManager::CameraSettingManager() {
  for (auto setting : filesystem::directory_iterator{bundle_.Path()}) {
    if (filesystem::is_directory(setting)) {
      all_setting_.emplace_back(
          CameraSetting::CameraSetting{UI::Bundle{setting.path()}});
    }
  }
}

CameraSettingManager::~CameraSettingManager() { db_info(""); }
}  // namespace SettingManager

#undef LOG_TAG
