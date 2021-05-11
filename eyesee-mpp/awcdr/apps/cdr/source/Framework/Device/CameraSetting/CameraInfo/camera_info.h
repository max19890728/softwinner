/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <Foundation.h>

#include "UIKit/Struct/bundle.h"
#include "UIKit/Struct/image.h"

namespace CameraInformation {

struct CameraInformation {
  UIImage cell_setting_title_;
  UIImage cell_title_;
  UIImage cell_title_highlight_;
  Property<int> state_;
  std::vector<UIImage> state_image_;
  std::vector<UIImage> state_icon_;

  CameraInformation(UI::Bundle);

  UIImage GetNowIcon();
};
}  // namespace CameraInformation
