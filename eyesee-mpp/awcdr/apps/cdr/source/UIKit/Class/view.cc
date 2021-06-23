/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "UIKit/Class/view.h"

#include <Foundation.h>
#include <common/extension/vector.h>

#include <utility>

#include "UIKit/Class/layer.h"
#include "UIKit/Component/label.h"
#include "UIKit/Struct/event.h"
#include "common/app_log.h"
#include "data/gui.h"

#undef LOG_TAG
#define LOG_TAG "UI::View"

UI::View::View()
    : background_color_(UI::Color::init()),
      is_hidden_(false),
      clears_context_before_drawing_(false),
      layer_(nullptr),
      is_user_interaction_enable_(true),
      is_exclusive_touch_(false),
      frame_(UI::Rect::zero),
      tag_(0) {
  background_color_.didSet = std::bind(&View::BackgroundColorDidSet, this);
#if 0
  frame_.getter = std::bind(&View::FrameGet, this);
  frame_.didSet = std::bind(&View::FrameDidSet, this);
#endif
  is_hidden_.getter = std::bind(&View::IsHiddenGetter, this);
  is_hidden_.didSet = std::bind(&View::IsHiddenDidSet, this);
}

// MARK: - 視圖渲染管理

// 當 `background_color_` 被修改完後會自動呼叫此方法
void UI::View::BackgroundColorDidSet() { SetNeedDisplay(); }

auto UI::View::IsHiddenGetter() const -> const bool& {
  return layer_->is_hidden_;
}

void UI::View::IsHiddenDidSet() { layer_->is_hidden_ = is_hidden_; }

UILayer UI::View::LayerClass() {
  auto layer = std::make_shared<UI::Layer>();
  layer->delegate_ = shared_from(this);
  return layer;
}

// MARK: - 邊界與框架範圍配置

auto UI::View::frame() const -> const UI::Rect& { return layer_->frame_; }

void UI::View::frame(UI::Rect const& new_frame) { layer_->frame_ = new_frame; }

auto UI::View::FrameGet() const -> const UI::Rect& {
  db_debug("");
  return layer_->frame_;
}

void UI::View::FrameDidSet() {
  db_debug("");
  this->layer_->frame_ = this->frame_;
}

// MARK: - 視圖層次結構管理

void UI::View::addSubView(UIView child_view) {
  layer_->AddSublayer(child_view->layer_);
  child_view->next_ = shared_from(this);
  child_view->superview_ = shared_from(this);
  child_view->window_ = window_;  // fixme: 不一定可以取得 `window_`
  subviews_.push_back(child_view);
}

void UI::View::RemoveFromSuperView() {
  layer_->RemoveFromSuperlayer();
  removeElementIn(superview_->subviews_, shared_from(this));
}

void UI::View::BringSubviewToFront(UIView view) {
  moveElementToBackIn(subviews_, view);
}

// MARK: 繪製與更新視圖

void UI::View::SetNeedDisplay() { layer_->SetNeedDisplay(); }

// MARK: - 不同座標系間的轉換

UI::Point UI::View::Convert(UI::Point point, UI::Convert convert_flag,
                            UIView const& target) {
  return layer_->Convert(point, convert_flag,
                         target ? target->layer_ : nullptr);
}

UI::Rect UI::View::Convert(UI::Rect rect, UI::Convert convert_flag,
                           UIView const& target) {
  return layer_->Convert(rect, convert_flag, target ? target->layer_ : nullptr);
}

// MARK: - 視圖命中測試

UIView UI::View::HitTest(UI::Point point, UIEvent const& event) {
  // auto on_self_point = Convert(point, UI::Convert::from, superview_);
  if (!PointIsInside(point, event)) return nullptr;
  for (auto iter = subviews_.rbegin(); iter != subviews_.rend(); ++iter) {
    auto subview = *iter;
    auto on_child = subview->Convert(point, UI::Convert::from);
    if (auto fit_view = subview->HitTest(on_child, event)) return fit_view;
  }
  return shared_from(this);
}

bool UI::View::PointIsInside(UI::Point point, UIEvent const& event) {
  if (point.x < 0 || point.x > frame().width()) return false;
  if (point.y < 0 || point.y > frame().height()) return false;
  return true;
}

// MARK: - - - - -

void UI::View::Layout(UI::Coder decoder) {
  layer_ = LayerClass();
  auto layout = decoder.layout_;
  if (layout.contains("frame")) {
    this->frame(UI::Rect::init(UI::Coder{layout.at("frame")}));
  }
  if (layout.contains("background_color")) {
    auto color = UI::Color{layout.at("background_color").get<std::string>()};
    background_color_ = color;
  }
  if (layout.contains("subviews")) {
    auto subviews = layout.at("subviews");
    for (auto& view_layout : subviews) {
      auto view_type = view_layout.value("type", std::string{});
      db_debug("type: %s", view_type.c_str());
      if (view_type == "label") {
        this->addSubView(UI::Label::init(UI::Coder{view_layout}));
      } else {
        this->addSubView(UI::View::init(UI::Coder{view_layout}));
      }
    }
  }
}

/* * * * * 繼承類別 * * * * */

// MARK: UI::Responder

void UI::View::TouchesBegan(UITouch const& touch, UIEvent const& event) {
  if (!is_exclusive_touch_) {
    this->UI::Responder::TouchesBegan(touch, event);
  }
}

void UI::View::TouchesMoved(UITouch const& touch, UIEvent const& event) {
  if (!is_exclusive_touch_) {
    this->UI::Responder::TouchesMoved(touch, event);
  }
}

void UI::View::TouchesEnded(UITouch const& touch, UIEvent const& event) {
  if (!is_exclusive_touch_) {
    this->UI::Responder::TouchesEnded(touch, event);
  }
}

// MARK: UI::LayerDelegate

void UI::View::Draw(UILayer layer, HDC context) {
  auto on_screen_frame = layer->Convert(layer->frame_, Convert::to);
  if (!is_hidden_) {
    if (background_color_ != 0x00000000) {
      // SetBrushType(context, BT_STIPPLED);
      SetBrushColor(context, background_color_);
      ::FillBox(context, Rect2Parameter(on_screen_frame));
    }
    if (auto const& content = layer->content_) {
      Load(content, context, on_screen_frame);
    }
  }
}
