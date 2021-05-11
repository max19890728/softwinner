/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <sys/types.h>

#include <chrono>

namespace std {
namespace chrono {

inline int64_t time_point2microseconds(system_clock::time_point time_point) {
  auto time_point_microseconds = time_point_cast<microseconds>(time_point);
  auto since_epoch = time_point_microseconds.time_since_epoch();
  return duration_cast<microseconds>(since_epoch).count();
}

inline int64_t count_time(system_clock::time_point start,
                          system_clock::time_point end) {
  return duration_cast<milliseconds>(end - start).count();
}
}
}  // namespace std
