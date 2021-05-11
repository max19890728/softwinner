/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "UIKit/Struct/press.h"

#include <sys/types.h>

#include "UIKit/Struct/key.h"
#include "data/gui.h"

UI::Press::Press(int raw_code) 
  : window_(nullptr), 
    key_(UI::Key(raw_code)),
    phase_(UI::Press::Phase::began) {}

void UI::Press::set_phase(uint new_phase) {
  switch (new_phase) {
  case MSG_KEYDOWN:
    phase_ = Phase::began;
    break;
  case MSG_KEYUP:
    phase_ = Phase::ended;
    break;
  }
}
