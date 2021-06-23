/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include <UIKit.h>

#include "US363/InfoTable/info_table_view_controller.h"

#undef Self
#define Self InfoTableViewController

#undef LOG_TAG
#define LOG_TAG "InfoTableViewController"

Self::Self(CameraInformation::CameraInformation& info)
    : UI::TableViewController::TableViewController(), info_(info) {}

/* * * * * 繼承類別 * * * * */

// MARK: - UI::Responder

void Self::Layout(UI::Coder decoder) {
  this->UI::TableViewController::Layout(decoder);
  view_->frame(UI::Screen::bounds);
  view_->background_color_ = UI::Color{0xFF292929};
  auto title_background = UI::Image::init("TitleBar/info");
  title_view_->frame(UI::Rect{
    origin : UI::Point{x : 0, y : 0},
    size : UI::Size{width : 240, height : 53}
  });
  auto title_background_view = UI::ImageView::init(title_background);
  title_background_view->frame(title_view_->frame());
  title_view_->addSubView(title_background_view);
  auto title_text = info_.cell_setting_title_;
  auto title_view = UI::ImageView::init(title_text);
  title_view->frame(UI::Rect{
    origin : UI::Point{x : 0, y : 0},
    size : UI::Size{width : 240, height : 38}
  });
  title_view_->addSubView(title_view);
  #if 0
  table_view_->content_inset_ = UI::EdgeInsets(53, 0, 0, 0);
  #else
  table_view_->frame(UI::Rect{
    origin : UI::Point{x : 0, y : 53},
    size : UI::Size{width: 240, height: 320 - 53}
  });
  #endif
  table_view_->content_size_ = UI::Size{width : 240, height : 320 - 53};
}

// MARK: - UI::TableViewDataSource

int Self::NumberOfSectionIn() { return 1; }

int Self::NumberOfRowsInSection(int) { return 1; }

UITableViewCell Self::CellForRowAt(UI::IndexPath index_path) {
  auto cell = UI::TableViewCell::init();
  if (index_path.row % 2 == 0)
    cell->background_color_ = UI::Color{0xFF292929};
  else
    cell->background_color_ = UI::Color{0xFF464646};
  if (index_path.row == 0) {
    auto state_image = info_.state_image_.at(info_.state_);
    auto image_view = UI::ImageView::init(state_image);
    image_view->frame(UI::Rect{
      origin : UI::Point{x : 0, y : 0},
      size : UI::Size{width : 240, height : 108}
    });
    cell->addSubView(image_view);
  }
  return cell;
}

// MARK: - UI::TableViewDelegate

void Self::DidSelectRowAt(UI::IndexPath) { db_info(""); }

#undef LOG_TAG
#undef Self
