/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <sstream>
#include <string>

struct CustomDebugStringConvertible {
  virtual auto PrintDebugStringOn(std::ostream&) const -> std::ostream& = 0;
};

inline auto operator<<(std::ostream& output,
                       const CustomDebugStringConvertible& value)
    -> std::ostream& {
  return value.PrintDebugStringOn(output);
}
