/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <memory>
#include <utility>

#include "UIKit/Class/view.h"
#include "UIKit/Component/image_view.h"
#include "UIKit/Component/label.h"
#include "UIKit/Struct/screen.h"

namespace UI {

class TableViewCell : public UI::View {
 public:
  enum class Style { camera_setting, setting_value };

  enum class AccessoryType { none, disclosure };

  enum class SelectionStyle { none, effected };

  static inline auto init(UI::Coder decoder = {}) {
    auto building = std::make_shared<UI::TableViewCell>();
    building->Layout(decoder);
    return building;
  }

  static inline auto init(UI::TableViewCell::Style style) {
    auto building = UI::TableViewCell::init();
    switch (style) {
      case UI::TableViewCell::Style::camera_setting:
        building->title_view_->frame(UI::Rect{
          origin : UI::Point{x : 0, y : 10},
          size : UI::Size{width : UI::Screen::bounds.width(), height : 35}
        });
        building->detail_view_->frame(UI::Rect{
          origin : UI::Point{x : 0, y : 46},
          size : UI::Size{width : UI::Screen::bounds.width(), height : 30}
        });
        break;
      case UI::TableViewCell::Style::setting_value:
        building->title_view_->frame(UI::Rect{
          origin : UI::Point{x : 0, y : 0},
          size : UI::Size{width : UI::Screen::bounds.width(), height : 86}
        });
        break;
      default:
        break;
    }
    return building;
  }

  UIView background_view_;
  UIView content_view_;
  UIImageView title_view_;

 protected:
  UI::TableViewCell::AccessoryType accessory_type_;
  UI::TableViewCell::SelectionStyle selection_style_;
  bool is_highlighted_;
  bool is_selected_;

 private:
  UIImageView detail_view_;
  UIImageView accessory_view_;
  UIImageView selection_view_;

 public:
  TableViewCell();

  void Layout(UI::Coder) override;

  // MARK: -

  void set_title(UIImage normal, UIImage highlight = nullptr);

  void set_detail(UIImage normal, UIImage highlight = nullptr);

  void set_accessory_type(UI::TableViewCell::AccessoryType);

  void set_selection_style(UI::TableViewCell::SelectionStyle);

  void set_selected(bool);

  void set_highlight(bool);
};
}  // namespace UI

typedef std::shared_ptr<UI::TableViewCell> UITableViewCell;
