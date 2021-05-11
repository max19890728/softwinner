/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <string>
#include <utility>
#include <vector>

#include "Device/CameraSetting/CameraMode/camera_mode.h"
#include "UIKit/Struct/image.h"

namespace CameraSetting {

struct CameraModeManager {
  // MARK: - 初始化器
 public:
  static CameraModeManager& instance() {
    static CameraModeManager instance_;
    return instance_;
  }

 private:
  CameraModeManager();
  ~CameraModeManager();
  CameraModeManager(CameraModeManager const&) = delete;
  CameraModeManager& operator=(CameraModeManager const&) = delete;

  // MARK: -

 private:
  static UI::Bundle bundle_;

 public:
  std::vector<CameraMode> all_mode_;
  int mode_;

  UIImage NowModeIcon();

  void SetMode(int);
};
}  // namespace CameraSetting
