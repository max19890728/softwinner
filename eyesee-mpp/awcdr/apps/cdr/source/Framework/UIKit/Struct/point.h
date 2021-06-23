/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <Foundation.h>
#include <System/custom_debug_string_convertible.h>

#define ParamToPoint(param)          \
  UI::Point {                        \
  x:                                 \
    LOWORD(param), y : HIWORD(param) \
  }

namespace UI {

enum class Convert : bool { from, to };

struct Point : Protocol::Property<Point>, CustomDebugStringConvertible {
  static const UI::Point zero;

  Point(int x, int y) : x(x), y(y) {
    this->x.didSet = [this] {
      if (this->didSet) this->didSet();
    };
    this->x.didSet = [this] {
      if (this->didSet) this->didSet();
    };
  }

  mutable Int x;
  mutable Int y;

  /* * * * * 運算符重載 * * * * */

  inline auto operator=(const UI::Point &new_value)
      -> const UI::Point & override {
    return this->Protocol::Property<UI::Point>::operator=(new_value);
  }

  inline auto operator==(const UI::Point &that) const -> bool override {
    return this->x == that.x and this->y == that.y;
  }

  inline auto operator!=(const UI::Point &that) const -> bool override {
    return !(*this == that);
  }

  inline auto operator+(const UI::Point &that) const -> const UI::Point {
    return UI::Point{this->x + that.x, this->y + that.y};
  }

  inline auto operator-(const UI::Point &that) const -> const UI::Point {
    return UI::Point{this->x - that.x, this->y - that.y};
  }

  inline auto operator+=(const UI::Point &that) const -> const UI::Point & {
    this->x += that.x;
    this->y += that.y;
    return *this;
  }

  inline auto operator-=(const UI::Point &that) const -> const UI::Point & {
    this->x -= that.x;
    this->y -= that.y;
    return *this;
  }

  template <typename T>
  inline auto operator-(const T &that) const -> const UI::Point {
    return UI::Point{this->x - that, this->y - that};
  }

  template <typename T>
  inline auto operator*(const T &that) const -> const UI::Point {
    return UI::Point{this->x * that, this->y * that};
  }

  template <typename T>
  inline auto operator/(const T &that) const -> const UI::Point {
    return UI::Point{this->x / that, this->y / that};
  }

  template <typename T>
  inline auto operator*=(const T &that) const -> const UI::Point & {
    this->x *= that;
    this->y *= that;
    return *this;
  }

  /* * * * * 繼承類別 * * * * */

  // - MARK: Protocol::Property

  auto get() const -> const UI::Point & override { return *this; }

  auto set(const UI::Point &new_value) -> const UI::Point & override {
    x = new_value.x;
    y = new_value.y;
    return *this;
  }

  // - MARK: CustomDebugStringConvertible

 public:
  inline auto PrintDebugStringOn(std::ostream &output) const -> std::ostream & {
    return output << "(x: " << x << ", y: " << y << ")";
  }
};

float Distance(UI::Point const &, UI::Point const &);
}  // namespace UI
