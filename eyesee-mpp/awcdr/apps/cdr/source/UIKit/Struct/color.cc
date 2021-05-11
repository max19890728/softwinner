/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "UIKit/Struct/color.h"

UI::Color const UI::Color::white = UI::Color{0xFFFFFFFF};

UI::Color const UI::Color::black = UI::Color{0xFF000000};

UI::Color const UI::Color::alpha = UI::Color{0x00FFFFFF};

// MARK: - 初始化器

UI::Color::Color() : value_(0) {}

UI::Color::Color(uint argb) : value_(argb) {}

UI::Color::Color(std::string hex_string)
    : value_(std::stoul(hex_string, nullptr, 16)) {}
