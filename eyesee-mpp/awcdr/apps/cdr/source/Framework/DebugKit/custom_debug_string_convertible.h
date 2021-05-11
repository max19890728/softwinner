/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <string>

namespace DebugKit {

template <class T>
struct CustomDebugStringConvertible {
  std::string DebugDescription() = 0;
};
}  // namespace DebugKit
