/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "US363/CameraModeSetting/camera_mode_view_controller.h"

#include <Foundation.h>
#include <common/app_log.h>

#include <memory>

#include "US363/CameraModeSetting/camera_mode_setting_table_view_controller.h"
#include "US363/CameraModeSetting/camera_mode_view.h"

#undef Self
#define Self CameraModeViewController

#undef LOG_TAG
#define LOG_TAG "CameraModeViewController"

Self::Self()
    : UI::ViewController::ViewController(),
      camera_mode_(CameraSetting::CameraModeManager::instance()),
      scroll_view_(UI::ScrollView::init()) {
  db_info("");
}

void Self::Layout(UI::Coder decoder) {
  view_->frame(UI::Screen::bounds);
  view_->background_color_ = UI::Color::white;
  view_->addSubView(scroll_view_);
  scroll_view_->frame(UI::Screen::bounds);
  scroll_view_->content_size_ = UI::Size{
    width : 9 + (camera_mode_.all_mode_.size() * (10 + 202)) + 19,
    height : UI::Screen::bounds.height()
  };
  scroll_view_->content_offset_ = {
    x : -(camera_mode_.mode_ * (9 + 202)),
    y : 0
  };
  scroll_view_->background_color_ = UI::Color::white;
  auto index = 0;
  for (auto const& mode : camera_mode_.all_mode_) {
    auto mode_view =
        CameraModeView::init(mode, index, [&](auto index, auto flag) {
          need_set_mode_ = std::make_pair(true, index);
          tap_flag_ = flag;
        });
    auto offset_x = 19 + (202 + 10) * index;
    mode_view->frame(UI::Rect{
      origin : UI::Point{x : offset_x, y : 8},
      size : UI::Size{width : 202, height : 306}
    });
    scroll_view_->content_view_->addSubView(mode_view);
    index++;
  }
  scroll_view_->delegate_ = shared_from(this);
}

void Self::DidScroll(UIScrollView) { need_set_mode_.first = false; }

void Self::DidEndDragging(UIScrollView, bool) {
  if (need_set_mode_.first) {
    switch (tap_flag_) {
      case CameraModeView::SetOrOpenSetting::set:
        camera_mode_.SetMode(need_set_mode_.second);
        Dismiss();
        break;
      case CameraModeView::SetOrOpenSetting::open:
        auto& mode = camera_mode_.all_mode_.at(need_set_mode_.second);
        Present(CameraModeSettingTableViewController::init(mode));
        break;
    }
  }
}

#undef LOG_TAG
#undef Self
