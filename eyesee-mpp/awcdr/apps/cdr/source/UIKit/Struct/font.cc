/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "UIKit/Struct/font.h"

#include <vector>

#include "data/gui.h"

/* * * * * FontFamily * * * * */

const std::vector<std::string> UI::Font::family_name_ = {"arialuni"};

/* * * * * FontType * * * * */

const char UI::Font::FontType::SXF[] = FONT_TYPE_NAME_BITMAP_SXF;
const char UI::Font::FontType::RAW[] = FONT_TYPE_NAME_BITMAP_RAW;
const char UI::Font::FontType::VAR[] = FONT_TYPE_NAME_BITMAP_VAR;
const char UI::Font::FontType::QPF[] = FONT_TYPE_NAME_BITMAP_QPF;
const char UI::Font::FontType::UPF[] = FONT_TYPE_NAME_BITMAP_UPF;
const char UI::Font::FontType::BMP[] = FONT_TYPE_NAME_BITMAP_BMP;
const char UI::Font::FontType::TTF[] = FONT_TYPE_NAME_SCALE_TTF;
const char UI::Font::FontType::T1F[] = FONT_TYPE_NAME_SCALE_T1F;
const char UI::Font::FontType::ALL[] = FONT_TYPE_NAME_ALL;

/* * * * * UI::Font * * * * */

UI::Font::Font(std::string font_name, int size, const char* type)
    : font_name_(font_name),
      point_size_(size),
      font_type_(type),
      font_created_(false),
      __font__(nullptr) {}

UI::Font::~Font() {
  if (font_created_) {
    DestroyLogFont(__font__);
  }
}
