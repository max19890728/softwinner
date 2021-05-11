/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "UIKit/Component/label.h"

#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "UI::Label"

UI::Label::Label()
    : UI::View::View(), text_(""), text_color_(UI::Color::black) {
  text_.didSet = std::bind(&Label::TextDidSet, this);
  text_color_.didSet = std::bind(&Label::TextColorDidSet, this);
}

// MARK: - 文本屬性

void UI::Label::TextDidSet() { SetNeedDisplay(); }

void UI::Label::TextColorDidSet() { SetNeedDisplay(); }

/* * * * * 繼承類別 * * * * */

// MARK: - UI::View

void UI::Label::Layout(UI::Coder coder) {
  this->UI::View::Layout(coder);
  if (coder.layout_.contains("title")) {
    text_ = coder.layout_.at("title").get<std::string>();
  }
}

// MARK: - UI::LayerDelegate

void UI::Label::Draw(UILayer layer, /* in */ HDC context) {
  this->UI::View::Draw(layer, context);
  auto on_screen_frame = layer->Convert(layer->frame_, Convert::to);
  if (!text_.is_empty_) {
    SetTextColor(context, text_color_);
    SetBkMode(context, BM_TRANSPARENT);
    RECT text_rect = {Rect2RECT(on_screen_frame)};
    DrawText(context, text_, -1, &text_rect, DT_LEFT);
  }
}
