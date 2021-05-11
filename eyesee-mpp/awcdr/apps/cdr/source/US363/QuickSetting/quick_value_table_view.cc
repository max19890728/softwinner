/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "US363/QuickSetting/quick_value_table_view.h"

#include <Device/CameraSetting/QuickSetting/quick_setting.h>
#include <Foundation.h>
#include <UIKit.h>

#undef Self
#define Self QuickValueTableView

#undef LOG_TAG
#define LOG_TAG "QuickValueTableView"

Self::Self(CameraSetting::QuickSetting& setting)
    : UI::TableView::TableView(), setting_(&(setting)) {}

/* * * * * 其他成員 * * * * */

void Self::SetInsetAndReload() {
  db_debug("");
  auto cell_image = setting_->values_.at(0).value_default_;
  auto _ = cell_image->Bitmap();
  auto inset_lenght = 160 - cell_image->rect_.height() / 2;
  content_inset_ = UI::EdgeInsets(inset_lenght, inset_lenght, 0, 0);
  db_debug("");
  ReloadData();
  db_debug("");
  // ScrollToRow({section : 0, row : setting_->value_}, ScrollPosition::middle);
  db_debug("");
}

void Self::SetNewSetting(CameraSetting::QuickSetting& new_setting) {
  db_debug("");
  setting_ = &(new_setting);
  SetInsetAndReload();
  db_debug("");
}

/* * * * * 繼承類別 * * * * */

// - MARK: UI::TableView

void Self::Layout(UI::Coder decoder) {
  this->UI::TableView::Layout(decoder);
  delegate_ = shared_from(this);
  data_source_ = shared_from(this);
  db_debug("");
  SetInsetAndReload();
  db_debug("");
}

// - MARK: UI::TableViewDataSource

int Self::NumberOfSectionIn() { return 1; }

int Self::NumberOfRowsInSection(int) {
  db_debug("%d", setting_->values_.size());
  return setting_->values_.size();
}

UITableViewCell Self::CellForRowAt(UI::IndexPath index_path) {
  auto cell = UI::TableViewCell::init();
  auto option = setting_->values_.at(index_path.row);
  auto number_image = option.value_default_;
  auto _ = number_image->Bitmap();
  cell->frame(UI::Rect{
    origin : UI::Point::zero,
    size : UI::Size{width : 110, height : number_image->rect_.height()}
  });
  auto background_image = option.background_;
  auto background_image_view = UI::ImageView::init(background_image);
  background_image_view->frame(cell->frame());
  cell->background_view_->addSubView(background_image_view);
  cell->background_view_->frame(cell->frame());
  auto number_image_view = UI::ImageView::init(number_image);
  number_image_view->frame(
      UI::Rect{origin : UI::Point::zero, size : number_image->rect_.size});
  cell->content_view_->addSubView(number_image_view);
  cell->content_view_->frame(cell->frame());
  return cell;
}

// - MARK: UI::TableViewDelegate

void Self::DidSelectRowAt(UI::IndexPath index_path) {
  ScrollToRow(index_path, ScrollPosition::middle);
  setting_->SetValue(index_path.row);
}

void Self::DidEndDecelerating(UIScrollView) {
  for (auto const& cell : cell_views_) {
    auto cell_frame = cell.second->frame();
    auto cell_bottom = cell_frame.origin.y + cell_frame.height();
    UI::Point content_offset = content_offset_;
    UI::EdgeInsets content_inset = content_inset_;
    auto offset = -(content_offset.y) + content_inset.top;
    auto middle_line = offset + cell_frame.height() / 2;
    if (cell_bottom > middle_line) {
      ScrollToRow(cell.first, ScrollPosition::middle);
      setting_->SetValue(cell.first.row);
      break;
    }
  }
}

#undef LOG_TAG
#undef Self
