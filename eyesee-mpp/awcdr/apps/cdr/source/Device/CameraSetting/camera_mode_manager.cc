/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/CameraSetting/camera_mode_manager.h"

#include <common/app_log.h>
#include <common/extension/filesystem.h>

#include <fstream>
#include <vector>

namespace CameraSetting {

UI::Bundle CameraModeManager::bundle_ = UI::Bundle{"/data/US363/CameraMode/"};

CameraModeManager::CameraModeManager() {
  for (auto mode : filesystem::directory_iterator{bundle_.Path()}) {
    if (filesystem::is_directory(mode)) {
      all_mode_.emplace_back(CameraMode{mode});
    }
  }
  try {
    if (auto setting = GetValue(bundle_.Path("value"))) {
      mode_ = setting;
    }
  } catch (const std::string error_message) {
    mode_ = 0;
  }
}

CameraModeManager::~CameraModeManager() {}

UIImage CameraModeManager::NowModeIcon() { return all_mode_.at(mode_).icon_; }

void CameraModeManager::SetMode(int mode) {
  mode_ = mode;
  SetValue(mode, bundle_.Path("value"));
}
}  // namespace CameraSetting
