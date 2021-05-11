/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "US363/QuickSetting/icon_select_view.h"

#include <Foundation.h>

#include "US363/QuickSetting/icon_view.h"

#undef Self
#define Self IconSelectView

#undef LOG_TAG
#define LOG_TAG "IconSelectView"

Self::Self(std::vector<UIImage> images, std::function<void(int)> callback)
    : UI::ScrollView::ScrollView(), icons_(images), callback_(callback) {}

/* * * * * 繼承類別 * * * * */

// MARK: - UI::Responder

void Self::TouchesEnded(UITouch const& touch, UIEvent const& event) {
  this->UI::ScrollView::TouchesEnded(touch, event);
  if (tapped_.first) {
    auto tapped_icon = icon_views_.at(tapped_.second);
    content_offset_ = UI::Point{
      x : -(tapped_icon->frame().origin.x) +
          (frame().width() - tapped_icon->frame().width()) / 2,
      y : 0
    };
    callback_(tapped_.second);
  }
  tapped_.first = false;
}

// MARK: - UI::ScrollView

void Self::Layout(UI::Coder decoder) {
  this->UI::ScrollView::Layout(decoder);
  delegate_ = shared_from(this);
  content_inset_ = UI::EdgeInsets{top : 0, bottom : 0, left : 96, right : 96};
  content_size_ = UI::Size{width : icons_.size() * 48, height : 38};
  for (auto index = 0; index < icons_.size(); ++index) {
    auto icon = icons_.at(index);
    auto icon_image_view = QuickIconView::init(icon, [this](auto tapped_index) {
      this->tapped_ = std::make_pair(true, tapped_index);
    });
    auto offset_x = 48 * index;
    icon_image_view->frame(UI::Rect{
      origin : UI::Point{x : offset_x, y : 0},
      size : UI::Size{width : 48, height : 38}
    });
    icon_image_view->tag_ = index;
    content_view_->addSubView(icon_image_view);
    icon_views_.push_back(icon_image_view);
  }
  UI::EdgeInsets content_inset = content_inset_;
  content_offset_ = UI::Point{x : content_inset.left, y : 0};
}

// MARK: - UI::ScrollViewDelegate

void Self::DidScroll(UIScrollView) { tapped_.first = false; }

void Self::DidEndDecelerating(UIScrollView) {
  if (tapped_.first) return;
  UI::Point content_offset = content_offset_;
  UI::EdgeInsets content_inset = content_inset_;
  auto offset_x = content_offset.x - content_inset.left;
  auto select_index = -(offset_x - 24) / 48;
  db_info("offset: %d, setting to %d", offset_x, select_index);
  if (select_index < 0 or select_index >= icon_views_.size()) return;
  auto select_icon = icon_views_.at(select_index);
  content_offset_ = UI::Point{
    x : -(select_icon->frame().origin.x) +
        (frame().width() - select_icon->frame().width()) / 2,
    y : 0
  };
  callback_(select_index);
}

#undef LOG_TAG
#undef Self
