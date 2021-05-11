/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

namespace UI {

struct IndexPath {
  int section;
  int row;

  bool operator<(UI::IndexPath const &) const;
};
}  // namespace UI
