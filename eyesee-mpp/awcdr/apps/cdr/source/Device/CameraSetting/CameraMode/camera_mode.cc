/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/CameraSetting/CameraMode/camera_mode.h"

#include "Device/CameraSetting/CameraSetting/camera_setting.h"

CameraSetting::CameraMode::CameraMode(filesystem::path path) {
  auto bundle = UI::Bundle{path.string() + "/"};
  icon_ = UI::Image::init("state", "", bundle);
  info_background_ = UI::Image::init("info_background", "", bundle);
  title_ = UI::Image::init("info_title", "", bundle);
  detail_right_ = UI::Image::init("detail_right", "", bundle);
  detail_left_ = UI::Image::init("detail_left", "", bundle);
  table_title_ = UI::Image::init("table_title", "", bundle);
  for (auto setting : filesystem::directory_iterator{path}) {
    if (filesystem::is_directory(setting)) {
      settings_.emplace_back(CameraSetting(UI::Bundle(setting.path())));
    }
  }
}
