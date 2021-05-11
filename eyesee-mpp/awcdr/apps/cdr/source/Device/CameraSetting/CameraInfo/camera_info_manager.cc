/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/CameraSetting/CameraInfo/camera_info_manager.h"

namespace SettingManager {

UI::Bundle CameraInfoManager::bundle_ = UI::Bundle{"/data/US363/CameraInfo"};

CameraInfoManager::CameraInfoManager() {
  for (auto info : filesystem::directory_iterator{bundle_.Path()}) {
    if (filesystem::is_directory(info)) {
      all_info_.emplace_back(CameraInformation::CameraInformation{UI::Bundle{info.path()}});
    }
  }
}

CameraInfoManager::~CameraInfoManager() {}
}  // namespace SettingManager
