/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "UIKit/Component/table_view.h"

#include <Foundation.h>

#include <memory>

#include "UIKit/Component/table_view_cell.h"
#include "UIKit/Struct/event.h"
#include "UIKit/Struct/index_path.h"
#include "UIKit/Struct/screen.h"
#include "UIKit/Struct/touch.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "UI::TableView"

UI::TableView::TableView()
    : UI::ScrollView::ScrollView(),
      tapped_cell_(std::make_pair(false, UI::IndexPath{section : 0, row : 0})),
      delegate_(nullptr) {
  delegate_.didSet = [this] {
    std::shared_ptr<UI::TableViewDelegate> temp = this->delegate_;
    this->UI::ScrollView::delegate_ = temp;
  };
}

void UI::TableView::ReloadData() {
  if (!data_source_) return;
  for (auto const& cell : content_view_->subviews_) {
    cell->layer_->RemoveFromSuperlayer();
  }
  content_view_->subviews_.clear();
  cell_views_.clear();
  int offset = 0;
  int section_count = data_source_->NumberOfSectionIn();
  for (int section = 0; section < section_count; section++) {
    int row_count = data_source_->NumberOfRowsInSection(section);
    for (int row = 0; row < row_count; row++) {
      UI::IndexPath index_path{section : section, row : row};
      auto cell = data_source_->CellForRowAt(index_path);
      cell->frame(UI::Rect{
        origin : UI::Point{x : 0, y : offset},
        size : cell->frame().size
      });
      offset += cell->frame().size.height;
      content_view_->addSubView(cell);
      cell_views_.emplace(index_path, cell);
    }
  }
  if (offset < frame().height()) offset = frame().height();
  content_size_ = UI::Size{width : frame().width(), height : offset};
  SetNeedDisplay();
}

void UI::TableView::DehighlightAllCell() {
  for (auto const& cell : cell_views_) {
    cell.second->set_highlight(false);
  }
}

std::pair<bool, UI::IndexPath> UI::TableView::FindCellBy(UI::Point point) {
  for (int section = 0; section < data_source_->NumberOfSectionIn();
       section++) {
    for (int row = 0; row < data_source_->NumberOfRowsInSection(section);
         row++) {
      UI::IndexPath index_path{section : section, row : row};
      auto on_child = cell_views_.at(index_path)->Convert(point, Convert::from);
      if (UI::PointIsIn(on_child, RectForRow(index_path))) {
        return std::make_pair(true, index_path);
      }
    }
  }
  return std::make_pair(false, UI::IndexPath{0, 0});
}

// @todo: catch cell_views_.at() 超出邊界時的錯誤。
UI::Rect UI::TableView::RectForRow(UI::IndexPath index_path) {
  auto cell_frame = cell_views_.at(index_path)->frame();
  return {origin : UI::Point::zero, size : cell_frame.size};
}

// MARK: - 滾動表格視圖

void UI::TableView::ScrollToRow(UI::IndexPath index_path,
                                ScrollPosition position) {
  auto cell = cell_views_[index_path];
  auto new_offset = -(cell->frame().origin.y);
  if (position == ScrollPosition::bottom) {
    new_offset += frame().height() - cell->frame().height();
  } else if (position == ScrollPosition::middle) {
    new_offset += (frame().height() - cell->frame().height()) / 2;
  }
  content_offset_ = UI::Point{x : 0, y : new_offset};
  SetNeedDisplay();
}

/* * * * * 繼承類別 * * * * */

// MARK: UI::Responder

void UI::TableView::TouchesBegan(UITouch const& touch, UIEvent const& event) {
  this->UI::ScrollView::TouchesBegan(touch, event);
  DehighlightAllCell();
  tapped_cell_ = FindCellBy(touch->touch_point_);
  if (tapped_cell_.first) {
    cell_views_.at(tapped_cell_.second)->set_highlight(true);
  }
}

void UI::TableView::TouchesMoved(UITouch const& touch, UIEvent const& event) {
  this->UI::ScrollView::TouchesMoved(touch, event);
  if (is_dragging_) {
    DehighlightAllCell();
    tapped_cell_.first = false;
  }
}

void UI::TableView::TouchesEnded(UITouch const& touch, UIEvent const& event) {
  this->UI::ScrollView::TouchesEnded(touch, event);
  DehighlightAllCell();
  // @fixme:
  std::shared_ptr<UI::TableViewDelegate> delegate = delegate_;
  if (tapped_cell_.first) delegate->DidSelectRowAt(tapped_cell_.second);
}
