/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <memory>
#include <string>

namespace UI {

enum class KeybordHIDUsage {
  home = 102, // KEY_HOME
  power = 116, // KEY_POWER
  menu = 139, // KEY_MENU
  back = 158, // KEY_BACK
  uknow = -1
};

struct Key {

  UI::KeybordHIDUsage key_code_;

  Key(int raw_code);

  std::string characters();
};
}  // namespace UI
