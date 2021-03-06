/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <Foundation.h>

namespace UI {

struct EdgeInsets : public Protocol::Property<EdgeInsets> {
  static UI::EdgeInsets zero;

  // - MARK: 初始化器

  EdgeInsets(int, int, int, int);

  int top = 0;
  int bottom = 0;
  int left = 0;
  int right = 0;

  /* * * * * 繼承類別 * * * * */

  auto get() const -> const EdgeInsets& override { return *this; }

  auto set(const EdgeInsets& new_value) -> const EdgeInsets& override {
    top = new_value.top;
    bottom = new_value.bottom;
    left = new_value.left;
    right = new_value.right;
    return *this;
  }
};
}  // namespace UI
