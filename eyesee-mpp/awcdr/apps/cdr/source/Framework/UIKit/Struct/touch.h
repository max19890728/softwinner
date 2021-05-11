/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <sys/types.h>

#include <chrono>
#include <memory>

#include "UIKit/Struct/point.h"

namespace UI {

struct Touch {
  static std::unique_ptr<UI::Touch> init(Point);

  enum class Phase { begin, moved, stationary, ended };

  Phase phase_;

  UI::Point touch_point_;

  int tap_count_;

  std::chrono::system_clock::time_point timestamp_;

  explicit Touch(UI::Point point);
};
}  // namespace UI

typedef std::unique_ptr<UI::Touch> UITouch;
