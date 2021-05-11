/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include "US363/CameraModeSetting/camera_mode_view.h"

#include <memory>

#undef Self
#define Self CameraModeView

Self::Self(CameraSetting::CameraMode mode, int index,
           std::function<void(int, SetOrOpenSetting)> callback)
    : UI::ImageView::ImageView(),
      index_(index),
      mode_(mode),
      callback_(callback) {}

void Self::Layout(UI::Coder) {
  this->UI::View::Layout();
  image(mode_.info_background_);
  auto title = UI::ImageView::init();
  title->frame({
    origin : UI::Point{x : 0, y : 216},
    size : UI::Size{width : 202, height : 35}
  });
  title->image(mode_.title_);
  addSubView(title);
  auto first_detail = UI::ImageView::init();
  first_detail->frame({
    origin : UI::Point{x : 18, y : 256},
    size : UI::Size{width : 68, height : 35}
  });
  first_detail->image(mode_.detail_left_);
  addSubView(first_detail);
  auto second_detail = UI::ImageView::init();
  second_detail->frame({
    origin : UI::Point{x : 90, y : 256},
    size : UI::Size{width : 94, height : 35}
  });
  second_detail->image(mode_.detail_right_);
  addSubView(second_detail);
}

void Self::TouchesBegan(UITouch const& touch, UIEvent const& event) {
  if (touch->touch_point_.y < 202) {
    callback_(index_, SetOrOpenSetting::set);
  } else {
    callback_(index_, SetOrOpenSetting::open);
  }
  this->UI::View::TouchesBegan(touch, event);
}

#undef Self
