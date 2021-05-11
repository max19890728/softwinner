/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "UIKit/Struct/rect.h"

#include "UIKit/decoder.h"

UI::Rect UI::Rect::init(RECT const& gdi_rect) {
  return UI::Rect{
      UI::Point{gdi_rect.left, gdi_rect.top},
      UI::Size{gdi_rect.right - gdi_rect.left, gdi_rect.bottom - gdi_rect.top}};
}

UI::Rect const UI::Rect::zero =
    UI::Rect{UI::Point{x : 0, y : 0}, UI::Size{width : 0, height : 0}};

UI::Rect const UI::Rect::init(UI::Coder decoder) {
  auto frame = decoder.layout_;
  Point origin{x : frame.value("x", 0), y : frame.value("y", 0)};
  Size size{width : frame.value("width", 0), height : frame.value("height", 0)};
  return UI::Rect{origin : origin, size : size};
}

int UI::Rect::width() const { return size.width; }

int UI::Rect::height() const { return size.height; }

bool UI::PointIsIn(UI::Point const point, UI::Rect const area) {
  if (point.x < area.origin.x) return false;
  if (point.x > area.origin.x + area.width()) return false;
  if (point.y < area.origin.y) return false;
  if (point.y > area.origin.y + area.height()) return false;
  return true;
}
