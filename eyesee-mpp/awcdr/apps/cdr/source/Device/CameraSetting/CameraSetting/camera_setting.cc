/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/CameraSetting/CameraSetting/camera_setting.h"

#include <common/app_log.h>
#include <common/extension/filesystem.h>

CameraSetting::CameraSetting::CameraSetting(UI::Bundle bundle)
    : bundle_(bundle) {
  cell_setting_title_ = UI::Image::init("setting_title", "", bundle);
  cell_title_ = UI::Image::init("cell_title", "", bundle);
  cell_title_highlight_ = UI::Image::init("cell_title_highlight", "", bundle);
  for (auto option : filesystem::directory_iterator{bundle.Path()}) {
    if (filesystem::is_directory(option)) {
      auto filename = option.path().filename();
      options_.emplace_back(
          std::make_pair(UI::Image::init("default", filename, bundle),
                         UI::Image::init("highlight", filename, bundle)));
      icons_.emplace_back(UI::Image::init("icon", filename, bundle));
    }
  }
  try {
    if (auto setting = GetValue(bundle_.Path("value"))) {
      value_ = setting;
    }
  } catch (const std::string error_message) {
    value_ = 0;
  }
}

std::pair<UIImage, UIImage> CameraSetting::CameraSetting::CellDetailImage() {
  if (options_.size() > value_) {
    auto result = options_.at(value_);
    return std::make_pair(result.first, result.second);
  } else {
    return std::make_pair(nullptr, nullptr);
  }
}

void CameraSetting::CameraSetting::SetValue(int new_value) {
  value_ = new_value;
  ::SetValue(value_, bundle_.Path("value"));
}
