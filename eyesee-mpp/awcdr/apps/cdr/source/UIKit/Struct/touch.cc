/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "UIKit/Struct/touch.h"

UITouch UI::Touch::init(UI::Point point) {
  return std::make_unique<UI::Touch>(point);
}

UI::Touch::Touch(UI::Point point)
    : phase_(UI::Touch::Phase::begin), touch_point_(point), tap_count_(1) {}
