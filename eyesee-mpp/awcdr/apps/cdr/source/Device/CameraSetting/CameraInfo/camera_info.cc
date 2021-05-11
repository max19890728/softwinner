/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/CameraSetting/CameraInfo/camera_info.h"

#include "UIKit/Struct/bundle.h"

namespace CameraInformation {

CameraInformation::CameraInformation(UI::Bundle bundle) {
  cell_setting_title_ = UI::Image::init("setting_title", "", bundle);
  cell_title_ = UI::Image::init("cell_title", "", bundle);
  cell_title_highlight_ = UI::Image::init("cell_title_highlight", "", bundle);
  auto state_bundle = UI::Bundle{bundle.Path() + "/State/"};
  for (auto state : filesystem::directory_iterator{state_bundle.Path()}) {
    if (filesystem::is_directory(state)) {
      auto filename = state.path().filename();
      state_image_.emplace_back(
          UI::Image::init("image", filename, state_bundle));
      state_icon_.emplace_back(UI::Image::init("icon", filename, state_bundle));
    }
  }
  state_ = 0;
}

UIImage CameraInformation::GetNowIcon() { return state_icon_.at(state_); }
}  // namespace CameraInformation
