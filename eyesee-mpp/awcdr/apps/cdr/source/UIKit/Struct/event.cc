/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "UIKit/Struct/event.h"

#include "UIKit/Struct/point.h"
#include "UIKit/Struct/touch.h"
#include "data/gui.h"

UI::Event::Event() {}

UI::Event::Event(UI::Point point) : touche_(UI::Touch::init(point)) {}

void UI::Event::timestamp(std::chrono::system_clock::time_point time) {
  touche_->timestamp_ = time;
}

void UI::Event::SetPoint(UI::Point new_point) {
  touche_->touch_point_ = new_point;
}

void UI::Event::SetState(int state) {
  switch (state) {
    case MSG_MOUSEMOVE:
      touche_->phase_ = UI::Touch::Phase::moved;
      break;
    case MSG_LBUTTONDOWN:
      touche_->phase_ = UI::Touch::Phase::begin;
      break;
    case MSG_LBUTTONUP:
      touche_->phase_ = UI::Touch::Phase::ended;
      break;
    default:
      break;
  }
}

void UI::Event::SetKeyWindow(std::shared_ptr<UI::Window> new_window) {
  window_ = new_window;
}

void UI::Event::SetFirstResponder(std::shared_ptr<UI::View> new_view) {
  view_ = new_view;
}
