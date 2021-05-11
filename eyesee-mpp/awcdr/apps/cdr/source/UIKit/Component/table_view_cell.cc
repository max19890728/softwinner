/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "UIKit/Component/table_view_cell.h"

#include "UIKit/Struct/point.h"
#include "UIKit/Struct/rect.h"
#include "UIKit/Struct/size.h"
#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "UI::TableViewCell"

UI::TableViewCell::TableViewCell()
    : UI::View::View(),
      background_view_(UI::View::init()),
      content_view_(UI::View::init()),
      accessory_type_(UI::TableViewCell::AccessoryType::none),
      selection_style_(UI::TableViewCell::SelectionStyle::none),
      is_highlighted_(false),
      is_selected_(false),
      title_view_(UI::ImageView::init()),
      detail_view_(UI::ImageView::init()),
      accessory_view_(UI::ImageView::init()),
      selection_view_(UI::ImageView::init()) {}

void UI::TableViewCell::Layout(UI::Coder decoder) {
  this->UI::View::Layout(decoder);
  addSubView(background_view_);
  addSubView(content_view_);
  addSubView(title_view_);
  addSubView(detail_view_);
  addSubView(accessory_view_);
  addSubView(selection_view_);
}

void UI::TableViewCell::set_title(UIImage normal, UIImage highlight) {
  title_view_->image(normal);
  title_view_->highlight_image(highlight);
}

void UI::TableViewCell::set_detail(UIImage normal, UIImage highlight) {
  detail_view_->image(normal);
  detail_view_->highlight_image(highlight);
}

void UI::TableViewCell::set_accessory_type(AccessoryType new_type) {
  accessory_type_ = new_type;
  switch (new_type) {
    case UI::TableViewCell::AccessoryType::none:
      break;
    case UI::TableViewCell::AccessoryType::disclosure: {
      auto normal = UI::Image::init("disclosure", "TableViewCell");
      auto highlight = UI::Image::init("disclosure_highligh", "TableViewCell");
      // - @fix: 圖片必須要載入後才能獲得元素的大小。
      auto _ = normal->Bitmap();
      auto x = UI::Screen::bounds.width() - normal->rect_.width();
      UI::Rect frame{origin : {x : x, y : 0}, size : normal->rect_.size};
      accessory_view_->frame(frame);
      accessory_view_->image(normal);
      accessory_view_->highlight_image(highlight);
      break;
    }
    default:
      break;
  }
}

void UI::TableViewCell::set_selection_style(SelectionStyle style) {
  if (selection_style_ == style) return;
  selection_style_ = style;
  switch (selection_style_) {
    case UI::TableViewCell::SelectionStyle::none:
      break;
    case UI::TableViewCell::SelectionStyle::effected: {
      auto normal = UI::Image::init("effected", "TableViewCell");
      auto highlight = UI::Image::init("effected_highlight", "TableViewCell");
      auto _ = normal->Bitmap();
      UI::Size size{width : normal->rect_.width(), height : frame().height()};
      UI::Rect frame{origin : UI::Point::zero, size : size};
      selection_view_->frame(frame);
      selection_view_->image(normal);
      selection_view_->highlight_image(highlight);
      break;
    }
  }
}

void UI::TableViewCell::set_selected(bool is_selected) {
  selection_view_->SetHighlight(is_selected);
  set_highlight(is_selected);
  is_selected_ = is_selected;
}

void UI::TableViewCell::set_highlight(bool is_highlighted) {
  if (is_selected_ && !is_highlighted) return;
  is_highlighted_ = is_highlighted;
  title_view_->SetHighlight(is_highlighted);
  detail_view_->SetHighlight(is_highlighted);
  accessory_view_->SetHighlight(is_highlighted);
}
