/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <functional>
#include <string>

namespace DebugKit {

class CommandLine {
 public:
  static void Send(std::string command,
                   std::function<void(std::string)> callback = [](auto) {});
};
}  // namespace DebugKit
