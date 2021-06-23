/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <type_traits>

namespace System {

namespace Console {

namespace Format {
constexpr const char default_[] = "\033[0m";

namespace Light {
constexpr const char red[] = "\033[1;31m";

constexpr const char green[] = "\033[1;32m";

constexpr const char blue[] = "\033[1;34m";
}  // namespace Light
}  // namespace Format
}  // namespace Console
}  // namespace System
