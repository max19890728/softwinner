/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "UIKit/Component/table_view_controller.h"

#include <Foundation.h>

#include "UIKit/Component/table_view_cell.h"
#include "UIKit/Struct/index_path.h"
#include "UIKit/Struct/screen.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "UI::TableViewController"

UITableViewController UI::TableViewController::init(UI::Coder decoder) {
  auto building = std::make_shared<UI::TableViewController>();
  building->Layout(decoder);
  return building;
}

UI::TableViewController::TableViewController()
    : UI::ViewController::ViewController() {}

void UI::TableViewController::Layout(UI::Coder decoder) {
  db_msg("");
  this->UI::ViewController::Layout(decoder);
  title_view_->frame(UI::Rect{
    origin : UI::Point{x : 0, y : 0},
    size : UI::Size{width : 240, height : 38}
  });
  auto title_background = UI::Image::init("TitleBar/child_background");
  auto title_background_view = UI::ImageView::init(title_background);
  title_background_view->frame(title_view_->frame());
  title_view_->addSubView(title_background_view);
  auto title_view_height = title_view_->frame().size.height;
  table_view_ = UI::TableView::init(UI::Screen::bounds);
  table_view_->content_inset_ = UI::
  EdgeInsets{top : title_view_height, bottom : 0, left : 0, right : 0};
  view_->addSubView(table_view_);
  view_->addSubView(title_view_);
}

void UI::TableViewController::ViewDidLoad() {
  this->UI::ViewController::ViewDidLoad();
  if (table_view_->delegate_ == nullptr)
    table_view_->delegate_ = shared_from(this);
  if (!table_view_->data_source_) table_view_->data_source_ = shared_from(this);
}

void UI::TableViewController::ViewDidAppear() {
  this->UI::ViewController::ViewDidAppear();
  table_view_->ReloadData();
}

int UI::TableViewController::NumberOfSectionIn() { return 0; }

int UI::TableViewController::NumberOfRowsInSection(int) { return 0; }

void UI::TableViewController::DidSelectRowAt(UI::IndexPath) {}

UITableViewCell UI::TableViewController::CellForRowAt(UI::IndexPath) {
  return UI::TableViewCell::init();
}
