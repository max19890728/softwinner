/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include "Device/CameraSetting/CameraInfo/camera_info.h"
#include "UIKit/Struct/bundle.h"

namespace SettingManager {

class CameraInfoManager {
 public:
  static CameraInfoManager& instance() {
    static CameraInfoManager instance_;
    return instance_;
  }

 private:
  CameraInfoManager();
  ~CameraInfoManager();
  CameraInfoManager(CameraInfoManager const&) = delete;
  CameraInfoManager& operator=(CameraInfoManager const&) = delete;

  // MARK: -

 private:
  static UI::Bundle bundle_;

 public:
  std::vector<CameraInformation::CameraInformation> all_info_;
};
}  // namespace SettingManager
