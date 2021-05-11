/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <unistd.h>

#include <algorithm>
#include <ctime>
#include <iostream>
#include <random>
#include <string>

/// 將字串內所有字母轉換為小寫字母。
#define ToLowerCase(string)                                          \
  do {                                                               \
    std::transform(string.begin(), string.end(), string.begin(),     \
                   [](unsigned char c) { return std::tolower(c); }); \
  } while (0)

/// 移除字串內所有換行符號。
#define RemoveNewLineIn(string)                                   \
  do {                                                            \
    string.erase(std::remove(string.begin(), string.end(), '\n'), \
                 string.end());                                   \
  } while (0)

inline std::string Random_String(size_t length = 31) {
  std::string temp_string;
  static const char alphanum[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";
  srand((unsigned)time(NULL) * getpid());
  for (int i = 0; i < length; ++i)
    temp_string += alphanum[rand() % (sizeof(alphanum) - 1)];
  return temp_string;
}
