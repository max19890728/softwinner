/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <Foundation.h>

#include <map>
#include <memory>
#include <utility>

#include "UIKit/Class/view.h"
#include "UIKit/Component/scroll_view.h"
#include "UIKit/Struct/event.h"
#include "UIKit/Struct/index_path.h"
#include "UIKit/Struct/touch.h"
#include "common/utils/json.hpp"
#include "UIKit/Struct/rect.h"

namespace UI {

class TableViewCell;

class TableViewDelegate : public UI::ScrollViewDelegate {
 public:
  virtual void DidSelectRowAt(UI::IndexPath) = 0;
};

class TableViewDataSource {
 public:
  virtual int NumberOfSectionIn() = 0;

  virtual int NumberOfRowsInSection(int) = 0;

  virtual std::shared_ptr<UI::TableViewCell> CellForRowAt(UI::IndexPath) = 0;
};

class TableView : public UI::ScrollView, public UI::ScrollViewDelegate {
 public:
  static inline auto init(UI::Coder decoder = {}) {
    auto building = std::make_shared<UI::TableView>();
    building->Layout(decoder);
    return building;
  }

  static inline auto init(UI::Rect frame) {
    auto building = UI::TableView::init();
    building->frame(frame);
    return building;
  }

 public:
  Variable<std::shared_ptr<TableViewDelegate>> delegate_;
  std::shared_ptr<TableViewDataSource> data_source_ = nullptr;

 protected:
  std::map<UI::IndexPath, std::shared_ptr<UI::TableViewCell>> cell_views_;
  std::pair<bool, UI::IndexPath> tapped_cell_;

 public:
  TableView();

  void ReloadData();

  void DehighlightAllCell();

  std::pair<bool, UI::IndexPath> FindCellBy(UI::Point);

  UI::Rect RectForRow(UI::IndexPath at_index);

  // MARK: - 滾動表格視圖

 public:
  enum class ScrollPosition { none, top, middle, bottom };

  void ScrollToRow(UI::IndexPath, ScrollPosition);

  /* * * * * 繼承類別 * * * * */

 public:
  // MARK: UI::Responder

  void TouchesBegan(UITouch const&, UIEvent const&) override;

  void TouchesMoved(UITouch const&, UIEvent const&) override;

  void TouchesEnded(UITouch const&, UIEvent const&) override;
};
}  // namespace UI

typedef std::shared_ptr<UI::TableView> UITableView;
