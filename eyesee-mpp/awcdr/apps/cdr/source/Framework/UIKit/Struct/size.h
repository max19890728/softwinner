/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <Foundation.h>

namespace UI {

struct Size : Protocol::Property<Size> {
  static const Size zero;

  Size(int width, int height) : width(width), height(height) {
    this->width.didSet = [this] {
      if (this->didSet) this->didSet();
    };
    this->height.didSet = [this] {
      if (this->didSet) this->didSet();
    };
  }

  mutable Int width;
  mutable Int height;

  /* * * * * 運算符重載 * * * * */

  inline auto operator=(const UI::Size &new_value)
      -> const UI::Size & override {
    return this->Protocol::Property<UI::Size>::operator=(new_value);
  }

  inline auto operator==(const UI::Size &that) const -> bool override {
    return this->width == that.width and this->height == that.height;
  }

  inline auto operator!=(const UI::Size &that) const -> bool override {
    return !(*this == that);
  }

  inline auto operator+(const UI::Size &that) const -> const UI::Size {
    return UI::Size{this->width + that.width, this->height + that.height};
  }

  inline auto operator-(const UI::Size &that) const -> const UI::Size {
    return UI::Size{this->width - that.width, this->height - that.height};
  }

  inline auto operator+=(const UI::Size &that) const -> const UI::Size & {
    this->width += that.width;
    this->height += that.height;
    return *this;
  }

  /* * * * * 繼承類別 * * * * */

  // - MARK: Protocol::Property

  auto get() const -> const UI::Size & override { return *this; }

  auto set(const UI::Size &new_value) -> const UI::Size & override {
    width = new_value.width;
    height = new_value.height;
    return *this;
  }
};
}  // namespace UI
