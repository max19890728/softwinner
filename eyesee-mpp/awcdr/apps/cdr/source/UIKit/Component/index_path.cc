/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "UIKit/Struct/index_path.h"

bool UI::IndexPath::operator<(UI::IndexPath const& that) const {
  if (this->section < that.section) return true;
  if (this->section > that.section) return false;
  if (this->row < that.row) return true;
  return false;
}
