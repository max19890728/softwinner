/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "US363/preview_view_controller.h"

#include <Device/CameraSetting/QuickSetting/quick_setting.h>

#include <memory>
#include <string>

#include "Device/CameraSetting/CameraInfo/camera_info_manager.h"
#include "US363/CameraModeSetting/camera_mode_view_controller.h"
#include "US363/CameraSetting/camera_setting_select_view_controller.h"
#include "US363/CameraSetting/camera_setting_view_controller.h"
#include "US363/InfoTable/info_table_view_controller.h"
#include "US363/QuickSetting/quick_setting_view_controller.h"
#include "US363/debug_view_controller.h"
#include "common/app_def.h"
#include "common/app_log.h"
#include "device_model/display.h"
#include "device_model/media/camera/camera.h"
#include "device_model/media/camera/camera_factory.h"

#undef Self
#define Self PreviewViewController

#undef LOG_TAG
#define LOG_TAG "PreviewViewController"

UI::Size Self::button_size = UI::Size{width : 46, height : 27};

Self::Self()
    : UI::ViewController::ViewController(UI::Coder{std::string{"PreviewView"}}),
      camera_info_(SettingManager::CameraInfoManager::instance()),
      camera_mode_(CameraSetting::CameraModeManager::instance()),
      delay_setting_(
          SettingManager::CameraSettingManager::instance().all_setting_.at(0)),
      delay_setting_button_(UI::Button::init()),
      storage_info_button_(UI::Button::init()),
      camera_mode_button_(UI::Button::init()),
      connection_info_button_(UI::Button::init()),
      battery_info_button(UI::Button::init()) {

  SettingManager::QuickSettingManager::instance();
}

void Self::Layout(UI::Coder decoder) {
  delay_setting_button_->frame({origin : {x : 0, y : 0}, size : button_size});
  delay_setting_button_->tag_ = 0;
  delay_setting_button_->AddTarget(this, Selector(Self::ButtonAction),
                                   UI::Control::Event::touch_up_inside);
  view_->addSubView(delay_setting_button_);
  storage_info_button_->frame({origin : {x : 46, y : 0}, size : button_size});
  storage_info_button_->tag_ = 1;
  storage_info_button_->AddTarget(this, Selector(Self::ButtonAction),
                                  UI::Control::Event::touch_up_inside);
  view_->addSubView(storage_info_button_);
  camera_mode_button_->frame(UI::Rect{
    origin : UI::Point{x : 92, y : 0},
    size : UI::Size{width : 56, height : 47}
  });
  camera_mode_button_->tag_ = 2;
  camera_mode_button_->AddTarget(this, Selector(Self::ButtonAction),
                                 UI::Control::Event::touch_up_inside);
  view_->addSubView(camera_mode_button_);
  connection_info_button_->frame(
      {origin : {x : 148, y : 0}, size : button_size});
  connection_info_button_->tag_ = 3;
  connection_info_button_->AddTarget(this, Selector(Self::ButtonAction),
                                     UI::Control::Event::touch_up_inside);
  view_->addSubView(connection_info_button_);
  battery_info_button->frame({origin : {x : 194, y : 0}, size : button_size});
  battery_info_button->tag_ = 4;
  battery_info_button->AddTarget(this, Selector(Self::ButtonAction),
                                 UI::Control::Event::touch_up_inside);
  view_->addSubView(battery_info_button);
}

void Self::ViewDidAppear() {
  this->UI::ViewController::ViewDidAppear();
  delay_setting_button_->SetImage(
      delay_setting_.icons_.at(delay_setting_.value_),
      UI::Control::State::normal);
  storage_info_button_->SetImage(camera_info_.all_info_.at(0).GetNowIcon(),
                                 UI::Control::State::normal);
  camera_mode_button_->SetImage(camera_mode_.NowModeIcon(),
                                UI::Control::State::normal);
  connection_info_button_->SetImage(camera_info_.all_info_.at(1).GetNowIcon(),
                                    UI::Control::State::normal);
  battery_info_button->SetImage(camera_info_.all_info_.at(2).GetNowIcon(),
                                UI::Control::State::normal);
}

void Self::TouchesBegan(UITouch const& touch, UIEvent const& event) {
  db_debug("touches Began, x: %d, y: %d", touch->touch_point_.x,
           touch->touch_point_.y);
  if (touch->touch_point_.y > 280) {
    pan_start_from_ = UI::ViewController::RectEdge::bottom;
  } else if (touch->touch_point_.x > 208) {
    pan_start_from_ = UI::ViewController::RectEdge::right;
  }
  this->UI::Responder::TouchesBegan(touch, event);
}

void Self::TouchesMoved(UITouch const& touch, UIEvent const& event) {
  db_debug("touches Moved, is from edge: %d", pan_start_from_);
  is_pan_ = true;
  this->UI::Responder::TouchesMoved(touch, event);
}

void Self::TouchesEnded(UITouch const& touch, UIEvent const& event) {
  db_debug("touches Ended");
  if (is_pan_) {
    switch (pan_start_from_) {
      case UI::ViewController::RectEdge::bottom:
        Present(CameraSettingViewController::init());
        break;
      case UI::ViewController::RectEdge::right:
        Present(QuickSettingViewController::init());
        break;
    }
  }
  pan_start_from_ = UI::ViewController::RectEdge::not_edge;
  is_pan_ = false;
  this->UI::Responder::TouchesEnded(touch, event);
}

/* * * * * 其他成員 * * * * */

void Self::ButtonAction(UIControl sender) {
  switch (sender->tag_) {
    case 0:
      Present(CameraSettingSelectViewController::init(delay_setting_));
      break;
    case 1:
      Present(InfoTableViewController::init(camera_info_.all_info_.at(0)));
      break;
    case 2:
      Present(CameraModeViewController::init());
      break;
    case 3:
      Present(InfoTableViewController::init(camera_info_.all_info_.at(1)));
      break;
    case 4:
      Present(InfoTableViewController::init(camera_info_.all_info_.at(2)));
      break;
  }
}

/* * * * * 繼承類別 * * * * */

// MARK: - UI::Responder

void Self::PressesEnded(UIPress const& press) {
#if SHOW_DEBUG_VIEW
  if (press->key_.key_code_ == UI::KeybordHIDUsage::menu) {
    Present(DebugViewController::init());
  }
#endif
  this->UI::ViewController::PressesEnded(press);
}
