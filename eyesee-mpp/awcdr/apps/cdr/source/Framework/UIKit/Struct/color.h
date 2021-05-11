/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <Foundation.h>
#include <sys/types.h>

#include <string>

namespace UI {

struct Color : public Protocol::Property<Color> {
  static const UI::Color white;

  static const UI::Color black;
  
  static const UI::Color alpha;

  // MARK: - 初始化器

 public:
  static inline UI::Color init(uint /* with */ rgba = 0) {
    return UI::Color{rgba};
  }

  Color();

  explicit Color(uint argb);

  explicit Color(std::string hex_string);

  /* * * * * 其他成員 * * * * */

 public:
  operator const uint&() const {
    return value_;
  }

  auto operator!=(const uint& that) const -> bool {
    return !(value_ == that);
  }

 private:
  uint value_;

  /* * * * * 繼承類別 * * * * */

  auto get() const -> const UI::Color& override { return *this; }

  auto set(const UI::Color& new_value) -> const UI::Color& override {
    value_ = new_value.value_;
    return *this;
  }
};
}  // namespace UI
