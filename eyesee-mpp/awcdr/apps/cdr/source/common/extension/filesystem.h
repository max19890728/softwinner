/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <common/extension/string.h>

#include <experimental/filesystem>
#include <fstream>
#include <memory>
#include <string>

namespace filesystem = std::experimental::filesystem;

inline int GetValue(/* from */ filesystem::path path) {
  std::ifstream input;
  std::string temp_string;
  input.open(path, std::ios::in);
  while (!input.eof()) {
    char temp = '\0';
    input.get(temp);
    if (input.fail()) break;
    temp_string += temp;
  }
  if (temp_string.empty()) throw std::string{"Can't read" + path.string()};
  RemoveNewLineIn(temp_string);
  input.close();
  return std::stoi(temp_string);
}

// @todo: 使用別的執行緒
inline void SetValue(int value,/* to */ filesystem::path path) {
  std::ofstream output;
  output.open(path, std::ios::trunc);
  std::string output_value = std::to_string(value);
  output.write(output_value.c_str(), output_value.size());
  output.close();
}
