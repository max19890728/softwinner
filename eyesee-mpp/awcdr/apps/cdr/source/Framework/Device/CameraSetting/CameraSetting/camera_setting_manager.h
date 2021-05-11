/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include "Device/CameraSetting/CameraSetting/camera_setting.h"

namespace SettingManager {

class CameraSettingManager {
 public:
  static CameraSettingManager& instance() {
    static CameraSettingManager instance_;
    return instance_;
  }

 private:
  CameraSettingManager();
  ~CameraSettingManager();
  CameraSettingManager(CameraSettingManager const&) = delete;
  CameraSettingManager& operator=(CameraSettingManager const&) = delete;

  // MARK: -

 private:
  static UI::Bundle bundle_;

 public:
  std::vector<CameraSetting::CameraSetting> all_setting_;
};
}  // namespace SettingManager
