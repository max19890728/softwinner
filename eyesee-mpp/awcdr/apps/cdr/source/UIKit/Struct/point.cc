/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "UIKit/Struct/point.h"

#include <cmath>

UI::Point const UI::Point::zero = UI::Point(0, 0);

float UI::Distance(UI::Point const& first, UI::Point const& sceoned) {
  return std::sqrt(static_cast<float>(std::pow(first.x - sceoned.x, 2)) +
                   static_cast<float>(std::pow(first.y - sceoned.y, 2)));
}
