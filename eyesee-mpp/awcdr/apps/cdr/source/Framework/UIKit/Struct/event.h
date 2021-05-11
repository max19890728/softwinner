/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <sys/types.h>

#include <memory>
#include <set>

#include "common/extension/chrono.h"
#include "UIKit/Struct/touch.h"

namespace UI {

struct Point;

class Window;

class View;

struct Event {
  static inline std::unique_ptr<UI::Event> init(Point point) { 
    return std::make_unique<UI::Event>(point);
  }

  std::unique_ptr<UI::Touch> touche_;

  std::shared_ptr<UI::Window> window_;

  std::shared_ptr<UI::View> view_;

  Event();

  explicit Event(UI::Point point);

  auto timestamp() { return touche_->timestamp_; }

  void timestamp(std::chrono::system_clock::time_point);

  void SetPoint(UI::Point new_point);

  // for MSG_XXX
  void SetState(int state);

  void SetKeyWindow(std::shared_ptr<UI::Window> new_window);

  void SetFirstResponder(std::shared_ptr<UI::View> new_view);
};
}  // namespace UI

typedef std::unique_ptr<UI::Event> UIEvent;
