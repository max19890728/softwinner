/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "UIKit/Control/switch.h"

#include <Foundation.h>

#include "UIKit/Control/control.h"
#include "UIKit/Struct/image.h"

#undef LOG_TAG
#define LOG_TAG "UI::Switch"

UI::Switch::Switch()
    : UI::Control::Control(),
      is_on_(false),
      switch_image_view_(UI::ImageView::init()),
      on_image_(UI::Image::init("on", "Switch")),
      off_image_(UI::Image::init("off", "Switch")) {
  is_on_.didSet = std::bind(&Switch::IsOnDidSet, this);
  AddTarget(this, Selector(Switch::TouchAction), Event::touch_up_inside);
}

void UI::Switch::IsOnDidSet() {
  switch_image_view_->image(is_on_ ? on_image_ : off_image_);
  SetNeedDisplay();
  SendActoion(Control::Event::value_changed);
}

void UI::Switch::Layout(UI::Coder) {
  this->UI::View::Layout();
  addSubView(switch_image_view_);
}

std::vector<UI::Control::Event> UI::Switch::AllControlEvents() {
  std::vector<UI::Control::Event> result;
  result.push_back(Event::touch_dwon);
  result.push_back(Event::touch_up_inside);
  result.push_back(Event::value_changed);
  return result;
}

void UI::Switch::TouchAction(UIControl sender) { is_on_.Toggle(); }

#undef LOG_TAG
