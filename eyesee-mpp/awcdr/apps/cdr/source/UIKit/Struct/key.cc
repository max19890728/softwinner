/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "UIKit/Struct/key.h"

#include <memory>
#include <string>

UI::Key::Key(int raw_code) {
  switch (raw_code) {
  case 102:
    key_code_ = KeybordHIDUsage::home;
    break;
  case 116:
    key_code_ = KeybordHIDUsage::power;
    break;
  case 139:
    key_code_ = KeybordHIDUsage::menu;
    break;
  case 158:
    key_code_ = KeybordHIDUsage::back;
    break;
  default:
    key_code_ = KeybordHIDUsage::uknow;
    break;
  }
}

std::string UI::Key::characters() {
  switch (key_code_) {
  case KeybordHIDUsage::home:
    /* code */
    break;
  case KeybordHIDUsage::power:
    /* code */
    break;
  case KeybordHIDUsage::menu:
    /* code */
    break;
  case KeybordHIDUsage::back:
    /* code */
    break;
  }
}
