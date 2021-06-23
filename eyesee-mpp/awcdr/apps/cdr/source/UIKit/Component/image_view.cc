/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "UIKit/Component/image_view.h"

UI::ImageView::ImageView()
    : UI::View::View(),
      image_(nullptr),
      highlight_image_(nullptr),
      is_highlighted_(false) {}

void UI::ImageView::image(UIImage new_image) {
  image_ = new_image;
  SetNeedDisplay();
}

void UI::ImageView::highlight_image(UIImage new_image) {
  highlight_image_ = new_image;
}

void UI::ImageView::SetHighlight(bool to_hightlight) {
  is_highlighted_ = to_hightlight;
  SetNeedDisplay();
}

/* * * * * 繼承類別 * * * * */

// MARK: UI::LayerDelegate

void UI::ImageView::Draw(UILayer layer, HDC context) {
  this->UI::View::Draw(layer, context);
  if (!is_hidden_) {
    if (highlight_image_) {
      auto content = is_highlighted_ ? highlight_image_ : image_;
      Load(content, context, layer->Convert(layer->frame_, Convert::to));
    } else {
      if (auto content = image_) {
        Load(content, context, layer->Convert(layer->frame_, Convert::to));
      }
    }
  }
}
