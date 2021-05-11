/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <vector>

#include "UIKit/Struct/image.h"
#include "Device/CameraSetting/CameraSetting/camera_setting.h"

namespace CameraSetting {

struct CameraMode {
  UIImage icon_;
  UIImage info_background_;
  UIImage title_;
  UIImage detail_right_;
  UIImage detail_left_;

  UIImage table_title_;

  std::vector<CameraSetting> settings_;

  CameraMode(filesystem::path);
};
}  // namespace CameraSetting
