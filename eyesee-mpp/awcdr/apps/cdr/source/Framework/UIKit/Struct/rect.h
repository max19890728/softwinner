/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <Foundation.h>

#include "UIKit/Struct/point.h"
#include "UIKit/Struct/size.h"
#include "data/gui.h"

#define Rect2Parameter(rect) \
  rect.origin.x, rect.origin.y, rect.size.width, rect.size.height

#define Rect2RECT(rect)                                          \
  rect.origin.x, rect.origin.y, rect.origin.x + rect.size.width, \
      rect.origin.y + rect.size.height

namespace UI {

class Coder;

struct Rect : Protocol::Property<Rect> {
  // 使用 GDI 的 RECT 結構初始化 UI::Rect
  static UI::Rect init(RECT const &);

  static UI::Rect const zero;

  static UI::Rect const init(UI::Coder decoder);

  Rect(UI::Point origin, UI::Size size) : origin(origin), size(size) {
    origin.didSet = [this] {
      if (this->didSet) this->didSet();
    };
    size.didSet = [this] {
      if (this->didSet) this->didSet();
    };
  }

  UI::Point origin;
  UI::Size size;

  int width() const;

  int height() const;

  /* * * * * 運算符重載 * * * * */

  inline operator const UI::Rect &() const {
    return this->Protocol::Property<UI::Rect>::operator const UI::Rect &();
  }

  auto operator=(const UI::Rect& new_rect) -> const UI::Rect & override {
    return this->Protocol::Property<UI::Rect>::operator=(new_rect);
  }

  inline operator const RECT *() const {
    RECT result{
      left : origin.x,
      top : origin.y,
      right : origin.x + size.width,
      bottom : origin.y + size.height
    };
    return std::move(&result);
  }

  inline auto operator==(const UI::Rect &that) const -> bool override {
    return this->origin == that.origin and this->size == that.size;
  }

  inline auto operator!=(const UI::Rect &that) const -> bool override {
    return !(*this == that);
  }

  inline auto operator+(const UI::Point &that) const -> const UI::Rect {
    return UI::Rect{origin : this->origin + that, size : this->size};
  }

  inline auto operator+=(const UI::Rect &that) const -> const UI::Rect & {
    this->origin += that.origin;
    this->size += that.size;
    return *this;
  }

  /* * * * * 繼承類別 * * * * */

  // - MARK: Protocol::Property

  auto get() const -> const UI::Rect & override { return *this; }

  auto set(const UI::Rect &new_value) -> const UI::Rect & override {
    origin = new_value.origin;
    size = new_value.size;
    return *this;
  }
};

bool PointIsIn(UI::Point const, UI::Rect const);
}  // namespace UI
