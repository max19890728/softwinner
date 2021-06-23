/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma ones

#include <endian.h>  // __BYTE_ORDER __LITTLE_ENDIAN

#include <algorithm>  // std::reverse()

template <typename T>
constexpr T hton(T value) noexcept {
#if __BYTE_ORDER == __LITTLE_ENDIAN
  char* ptr = reinterpret_cast<char*>(&value);
  std::reverse(ptr, ptr + sizeof(T));
#endif
  return value;
}

template <typename T>
constexpr T ntoh(T value) noexcept {
#if __BYTE_ORDER == __LITTLE_ENDIAN
  char* ptr = reinterpret_cast<char*>(&value);
  std::reverse(ptr, ptr + sizeof(T));
#endif
  return value;
}
