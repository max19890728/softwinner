/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "US363/CameraSetting/camera_setting_select_view_controller.h"

#include <memory>
#include <string>
#include <utility>

#include "Device/CameraSetting/CameraSetting/camera_setting.h"

#undef Self
#define Self CameraSettingSelectViewController

#undef LOG_TAG
#define LOG_TAG "CameraSettingSelectViewController"

Self::Self(CameraSetting::CameraSetting &setting)
    : UI::TableViewController::TableViewController(), setting_(setting) {}

void Self::Layout(UI::Coder) {
  std::string layout_config{"CameraValueSetting"};
  this->UI::TableViewController::Layout(UI::Coder{layout_config});
  view_->frame(UI::Screen::bounds);
  view_->background_color_ = UI::Color{0xFF464646};
  auto title_background = UI::Image::init("TitleBar/background");
  auto title_background_view = UI::ImageView::init(title_background);
  title_background_view->frame(title_view_->frame());
  title_view_->addSubView(title_background_view);
  auto title_image_view = UI::ImageView::init(setting_.cell_setting_title_);
  title_image_view->frame(title_view_->frame());
  title_view_->addSubView(title_image_view);
}

int Self::NumberOfSectionIn() { return 1; }

int Self::NumberOfRowsInSection(int) { return setting_.options_.size(); }

UITableViewCell Self::CellForRowAt(UI::IndexPath index_path) {
  auto cell = UI::TableViewCell::init(UI::TableViewCell::Style::setting_value);
  cell->frame({UI::Point::zero, {width : view_->frame().width(), height : 86}});
  cell->set_selection_style(UI::TableViewCell::SelectionStyle::effected);
  if (index_path.row % 2 == 0)
    cell->background_color_ = UI::Color{0xFF292929};
  else
    cell->background_color_ = UI::Color{0xFF464646};
  auto title_image = setting_.options_.at(index_path.row);
  cell->set_title(title_image.first, title_image.second);
  cell->set_selected(index_path.row == setting_.value_);
  return cell;
}

void Self::DidSelectRowAt(UI::IndexPath index_path) {
  if (setting_.value_ == index_path.row) {
    Dismiss();
  } else {
    setting_.SetValue(index_path.row);
    table_view_->ReloadData();
  }
}

#undef LOG_TAG
#undef Self
