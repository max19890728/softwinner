/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <memory>
#include <utility>

#include "UIKit/Struct/image.h"

namespace Protocol {

struct Settable {
  int now_select_;

  virtual int SelectionCount() = 0;

  // first: Normal 標題， second: Highlight 標題
  virtual std::pair<UIImage, UIImage> OptionTitle(int) = 0;

  virtual int NowSelectedIndex() = 0;

  virtual void Select(int) = 0;
};
}  // namespace Protocol
