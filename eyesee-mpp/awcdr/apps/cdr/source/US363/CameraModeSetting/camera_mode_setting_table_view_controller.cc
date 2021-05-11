/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "US363/CameraModeSetting/camera_mode_setting_table_view_controller.h"

#include <Foundation.h>

#include <string>

#include "US363/CameraSetting/camera_setting_select_view_controller.h"

#undef Self
#define Self CameraModeSettingTableViewController

#undef LOG_TAG
#define LOG_TAG "CameraModeSettingTableViewController"

Self::Self(CameraSetting::CameraMode& mode)
    : UI::TableViewController::TableViewController(), mode_(mode) {}

void Self::Layout(UI::Coder decoder) {
  this->UI::TableViewController::Layout(decoder);
  view_->frame(UI::Screen::bounds);
  view_->background_color_ = UI::Color{0xFF464646};
  auto title_background = UI::Image::init("background", "CameraMode/TitleBar");
  auto title_background_view = UI::ImageView::init(title_background);
  auto title_height = 38;
  title_view_->frame({UI::Point::zero, {width : 240, height : title_height}});
  title_background_view->frame(title_view_->frame());
  title_view_->addSubView(title_background_view);
  auto title_text_view = UI::ImageView::init(mode_.table_title_);
  title_text_view->frame(title_view_->frame());
  title_view_->addSubView(title_text_view);
}

int Self::NumberOfSectionIn() { return 1; }

int Self::NumberOfRowsInSection(int section) { return mode_.settings_.size(); }

UITableViewCell Self::CellForRowAt(UI::IndexPath index_path) {
  auto cell = UI::TableViewCell::init(UI::TableViewCell::Style::camera_setting);
  cell->frame({UI::Point::zero, {width : view_->frame().width(), height : 86}});
  if (index_path.row % 2 == 0)
    cell->background_color_ = UI::Color{0xFF292929};
  else
    cell->background_color_ = UI::Color{0xFF464646};
  auto setting = mode_.settings_.at(index_path.row);
  auto detail_text = setting.CellDetailImage();
  cell->set_title(setting.cell_title_, setting.cell_title_highlight_);
  cell->set_detail(detail_text.first, detail_text.second);
  cell->set_accessory_type(UI::TableViewCell::AccessoryType::disclosure);
  return cell;
}

void Self::DidSelectRowAt(UI::IndexPath index_path) {
  auto& setting = mode_.settings_.at(index_path.row);
  Present(CameraSettingSelectViewController::init(setting));
}

#undef LOG_TAG
#undef Self
