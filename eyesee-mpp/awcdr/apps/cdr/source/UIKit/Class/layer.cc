/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "UIKit/Class/layer.h"

#include <Foundation.h>
#include <common/extension/vector.h>

#include "UIKit/Class/application.h"
#include "UIKit/Class/view.h"
#include "UIKit/Class/window.h"
#include "UIKit/Struct/screen.h"
#include "common/app_log.h"
#include "common/extension/chrono.h"
#include "common/extension/string.h"
#include "widgets/ctrlclass.h"

#undef LOG_TAG
#define LOG_TAG "UI::Layer"

UI::Layer::Layer()
    : context_(HWND_INVALID),
      content_(nullptr),
      superlayer_(nullptr),
      frame_(UI::Rect::zero),
      background_color_(UI::Color{0x00000000}),
      shouldRasterize_(false),
      is_need_display_(false),
      is_need_layout_(false),
      is_hidden_(false) {
  frame_.willSet = std::bind(&Layer::FrameWillSet, this, std::placeholders::_1);
  frame_.didSet = std::bind(&UI::Layer::FrameDidSet, this);
}

void UI::Layer::AddSublayer(UILayer sublayer) {
  sublayer->superlayer_ = shared_from_this();
  sublayers_.push_back(sublayer);
}

void UI::Layer::RemoveFromSuperlayer() {
  removeElementIn(superlayer_->sublayers_, shared_from(this));
  superlayer_->SetNeedDisplay();
  superlayer_ = nullptr;
}

void UI::Layer::UnloadContent() {
  if (content_) content_->UnloadBitmap();
}

// - MARK: Layout Function

void UI::Layer::SetNeedLayout() { is_need_layout_ = true; }

void UI::Layer::LayoutIfNeed() {
  if (is_need_layout_) {
    for (auto const& sublayer : sublayers_) sublayer->LayoutIfNeed();
    if (content_) SetContent(content_);
    SetBackground(background_color_);
    SetHidden(is_hidden_);
    is_need_layout_ = false;
  }
}

// - MARK: Get Function

UI::Color const& UI::Layer::BackgroundColor() { return background_color_; }

bool UI::Layer::IsHidden() { return is_hidden_; }

// - MARK: Set Function

void UI::Layer::SetBackground(UI::Color const& new_color) {
  background_color_ = new_color;
  SetNeedDisplay();
}

void UI::Layer::SetContent(UIImage const& image) {
  content_ = image;
  SetNeedDisplay();
}

void UI::Layer::SetHidden(bool hidden) {
  is_hidden_ = hidden;
  SetNeedDisplay();
}

// - MARK: 準備圖層內容

/**
 * - todo: 以 `is_need_display_` 判斷是否需要重畫。
 **/
void UI::Layer::Draw(/* in */ HDC context) {
  if (auto delegate = delegate_.lock())
    delegate->Draw(shared_from(this), context);
  for (auto const& sublayer : sublayers_) sublayer->Draw(context);
}

// - MARK: 管理圖層位置與邊界

auto UI::Layer::FrameWillSet(const UI::Rect& new_frame) const
    -> const UI::Rect& {
  frame_change_flag_ = frame_ == new_frame;
  return new_frame;
}

void UI::Layer::FrameDidSet() {
  if (frame_change_flag_ && superlayer_) superlayer_->SetNeedDisplay();
  SetNeedDisplay();
}

// - MARK: 更新圖層顯示

void UI::Layer::SetNeedDisplay() {
  SetNeedDisplay(Convert(frame_, UI::Convert::to, RootLayer()));
}

void UI::Layer::SetNeedDisplay(UI::Rect const& rect) {
  is_need_display_ = true;
  ::InvalidateRect(RootLayer()->context_, rect, FALSE);
}

void UI::Layer::DisplayIfNeeded() {
  ::UpdateWindow(RootLayer()->context_, FALSE);
}

bool UI::Layer::NeedDisplay() { return is_need_display_; }

// - MARK: 不同座標系間的轉換。

// - todo: 簡化它
UI::Point UI::Layer::Convert(UI::Point point, UI::Convert convert_flag,
                             UILayer const& target_layer) {
  auto start_layer = shared_from(this);
  auto result = point;
  for (auto ancestor_layer : start_layer->AllAncestorLayer()) {
    switch (convert_flag) {
      case UI::Convert::to:
        result += ancestor_layer->frame_.origin;
        break;
      case UI::Convert::from:
        result -= ancestor_layer->frame_.origin;
        break;
    }
  }
  if (convert_flag == UI::Convert::from) {
    result -= start_layer->frame_.origin;
  }
  if (target_layer) {
    for (auto to_ancestor_layer : target_layer->AllAncestorLayer()) {
      switch (convert_flag) {
        case UI::Convert::to:
          result -= to_ancestor_layer->frame_.origin;
          break;
        case UI::Convert::from:
          result += to_ancestor_layer->frame_.origin;
          break;
      }
    }
    result -= target_layer->frame_.origin;
  }
  return result;
}

UI::Rect UI::Layer::Convert(UI::Rect rect, UI::Convert convert_flag,
                            UILayer const& to_layer) {
  return {
    origin : Convert(rect.origin, convert_flag, to_layer),
    size : rect.size
  };
}

// - - - - - MARK: 私有方法

// - MARK: 圖層樹處理

/**
 * 遍歷所有 `superlayer_` 找到未擁有 `superlayer` 的 `UI::Layer` 為根
 *`UI::Layer`
 **/
UILayer UI::Layer::RootLayer() {
  auto layer = shared_from(this);
  while (layer->superlayer_) layer = layer->superlayer_;
  return layer;
}

std::vector<UILayer> UI::Layer::AllAncestorLayer() {
  std::vector<UILayer> result;
  auto layer = shared_from(this);
  while (layer->superlayer_) {
    layer = layer->superlayer_;
    if (layer) result.emplace_back(layer);
  }
  return result;
}
