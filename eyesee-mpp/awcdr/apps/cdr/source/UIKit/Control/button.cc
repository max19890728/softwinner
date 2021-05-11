/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "UIKit/Control/button.h"

UI::Button::Button() : UI::Control::Control(), image_view_(nullptr) {}

std::vector<UI::Control::Event> UI::Button::AllControlEvents() {
  std::vector<UI::Control::Event> result;
  result.push_back(Event::touch_dwon);
  result.push_back(Event::touch_up_inside);
  return result;
}

// MARK: - 配置按鈕影像

UIImage UI::Button::Image(UI::Control::State for_state) {
  auto now_image = image_table_.find(for_state);
  if (now_image == image_table_.end()) return nullptr;
  return image_table_[for_state];
}

void UI::Button::SetImage(UIImage image, UI::Control::State for_state) {
  image_table_[for_state] = image;
  if (!image_view_) {
    image_view_ = UI::ImageView::init();
    image_view_->frame(UI::Rect{origin : UI::Point::zero, size : frame().size});
    addSubView(image_view_);
  }
  if (for_state == state_) image_view_->image(image);
}
