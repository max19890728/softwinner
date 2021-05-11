/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <memory>

#include "UIKit/decoder.h"

namespace UI {

template <class T>
struct Buildable {
  static inline std::shared_ptr<T> init(UI::Coder decoder = {}) {
    auto building = std::make_shared<T>();
    building->Layout(decoder);
    return building;
  }
};
}  // namespace UI
