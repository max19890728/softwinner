/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "US363/CameraSetting/camera_setting_view_controller.h"

#include <Foundation.h>

#include <memory>
#include <string>
#include <utility>

#include "US363/CameraSetting/camera_setting_select_view_controller.h"
#include "US363/InfoTable/info_table_view_controller.h"
#include "common/app_log.h"

#undef Self
#define Self CameraSettingViewController

#undef LOG_TAG
#define LOG_TAG "CameraSettingViewController"

Self::Self()
    : UI::TableViewController::TableViewController(),
      manager_(SettingManager::CameraSettingManager::instance()),
      info_manager_(SettingManager::CameraInfoManager::instance()) {}

void Self::Layout(UI::Coder decoder) {
  this->UI::TableViewController::Layout(decoder);
  view_->frame(UI::Screen::bounds);
  view_->background_color_ = UI::Color{0xFF464646};
  auto title_view = UI::ImageView::init(UI::Image::init("TitleBar/setup"));
  title_view->frame(title_view_->frame());
  title_view_->addSubView(title_view);
}

int Self::NumberOfSectionIn() { return 2; }

int Self::NumberOfRowsInSection(int section) {
  switch (section) {
    case 0:
      return info_manager_.all_info_.size();
      break;
    case 1:
      return manager_.all_setting_.size();
      break;
  }
}

UITableViewCell Self::CellForRowAt(UI::IndexPath index_path) {
  auto cell = UI::TableViewCell::init(UI::TableViewCell::Style::camera_setting);
  cell->frame({UI::Point::zero, {width : view_->frame().width(), height : 86}});
  auto index =
      (index_path.row + (index_path.section * info_manager_.all_info_.size()));
  if (index % 2 == 0)
    cell->background_color_ = UI::Color{0xFF292929};
  else
    cell->background_color_ = UI::Color{0xFF464646};
  switch (index_path.section) {
    case 0: {
      auto info = info_manager_.all_info_.at(index_path.row);
      cell->set_title(info.cell_title_, info.cell_title_highlight_);
      cell->set_accessory_type(UI::TableViewCell::AccessoryType::disclosure);
      break;
    }
    case 1: {
      auto setting = manager_.all_setting_.at(index_path.row);
      auto detail_text = setting.CellDetailImage();
      cell->set_title(setting.cell_title_, setting.cell_title_highlight_);
      cell->set_detail(detail_text.first, detail_text.second);
      cell->set_accessory_type(UI::TableViewCell::AccessoryType::disclosure);
      break;
    }
  }
  return cell;
}

void Self::DidSelectRowAt(UI::IndexPath index_path) {
  switch (index_path.section) {
    case 0: {
      auto& info = info_manager_.all_info_.at(index_path.row);
      Present(InfoTableViewController::init(info));
      break;
    }
    case 1: {
      auto& setting = manager_.all_setting_.at(index_path.row);
      Present(CameraSettingSelectViewController::init(setting));
      break;
    }
  }
}
