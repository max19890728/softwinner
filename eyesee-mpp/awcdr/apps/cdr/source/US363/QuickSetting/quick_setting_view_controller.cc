/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "US363/QuickSetting/quick_setting_view_controller.h"

#include <Foundation.h>
#include <UIKit.h>
#include <common/app_log.h>

#include "US363/QuickSetting/icon_select_view.h"
#include "US363/QuickSetting/quick_value_table_view.h"

#undef Self
#define Self QuickSettingViewController

#undef LOG_TAG
#define LOG_TAG "QuickSettingViewController"

Self::Self()
    : UI::ViewController::ViewController(),
      manager_(SettingManager::QuickSettingManager::instance()),
      now_setting_index_(0),
      now_setting_(&(manager_.settings_.at(now_setting_index_))) {}

/* * * * * 其他成員 * * * * */

void Self::LayoutAutoButton() {
  auto background_image = UI::Image::init("background", "AutoButton");
  auto background_image_view = UI::ImageView::init(background_image);
  background_image_view->frame(UI::Rect{
    origin : UI::Point{x : 0, y : 284},
    size : UI::Size{width : 153, height : 36}
  });
  view_->addSubView(background_image_view);
  auto_switch_ = UI::Switch::init();
  auto_switch_->frame(UI::Rect{
    origin : UI::Point{x : 0, y : 284},
    size : UI::Size{width : 153, height : 36}
  });
  auto_switch_->on_image_ = UI::Image::init("on", "AutoButton");
  auto_switch_->off_image_ = UI::Image::init("off", "AutoButton");
  auto_switch_->is_on_ = now_setting_->is_auto_;
  auto_switch_->AddTarget(this, Selector(Self::AutoSwitchAction),
                          UI::Control::Event::value_changed);
  view_->addSubView(auto_switch_);
}

void Self::LayoutIconScrollView() {
  auto title_background_image = UI::Image::init("quick_setting", "TitleBar");
  auto title_background_view = UI::ImageView::init(title_background_image);
  title_background_view->frame(UI::Rect{
    origin : UI::Point{x : 0, y : 0},
    size : UI::Size{width : 240, height : 60}
  });
  view_->addSubView(title_background_view);
  std::vector<UIImage> images;
  for (auto setting : manager_.settings_) images.push_back(setting.icon_);
  auto icon_scroll_view = IconSelectView::init(
      images, [this](auto selected) { this->SetValueScrollView(selected); });
  icon_scroll_view->frame(UI::Rect{
    origin : UI::Point{x : 0, y : 0},
    size : UI::Size{width : 240, height : 38}
  });
  view_->addSubView(icon_scroll_view);
}

void Self::LayoutValueTableView() {
  db_debug("");
  UIImage value_table_Background;
  db_debug("");
  if (auto background = now_setting_->background_) {
    value_table_Background = background;
  } else {
    value_table_Background =
        UI::Image::init("quick_setting_background", "TableView");
  }
  db_debug("");
  auto value_table_Background_view =
      UI::ImageView::init(value_table_Background);
  value_table_Background_view->frame(UI::Rect{
    origin : UI::Point{x : 130, y : 0},
    size : UI::Size{width : 110, height : 320}
  });
  view_->addSubView(value_table_Background_view);
  db_debug("");
  setting_table_view_ =
      QuickValueTableView::init(manager_.settings_.at(now_setting_index_));
  setting_table_view_->frame(UI::Rect{
    origin : UI::Point{x : 130, y : 0},
    size : UI::Size{width : 110, height : 320}
  });
  view_->addSubView(setting_table_view_);
  db_debug("");
  auto selecter_image = UI::Image::init("selecter", "TableView");
  auto selecter_image_view = UI::ImageView::init(selecter_image);
  selecter_image_view->frame(UI::Rect{
    origin : UI::Point{x : 130, y : 160 - 22},
    size : UI::Size{width : 110, height : 45}
  });
  view_->addSubView(selecter_image_view);
}

void Self::SetValueScrollView(int index) {
  db_debug("");
  if (index < 0 or index >= manager_.settings_.size()) return;
  now_setting_index_ = index;
  now_setting_ = &(manager_.settings_.at(index));
  auto_switch_->is_on_ = now_setting_->is_auto_;
  setting_table_view_->SetNewSetting(manager_.settings_.at(index));
  view_->SetNeedDisplay();
}

void Self::AutoSwitchAction(UIControl sender) {
  db_debug("");
  now_setting_->SetAuto(auto_switch_->is_on_);
}

/* * * * * 繼承類別 * * * * */

// MARK: - UI::ViewController

void Self::Layout(UI::Coder) {
  view_->frame(UI::Screen::bounds);
  view_->is_exclusive_touch_ = true;
  db_debug("");
  LayoutAutoButton();
  LayoutValueTableView();
  LayoutIconScrollView();
  SetValueScrollView(now_setting_index_);
}

#undef LOG_TAG
#undef Self
