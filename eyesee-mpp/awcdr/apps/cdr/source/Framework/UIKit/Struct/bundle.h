/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <string>

#include "common/extension/filesystem.h"

namespace UI {
struct Bundle {
 public:
  static UI::Bundle const main;

 private:
  filesystem::path path_;

 public:
  explicit Bundle(std::string);

  std::string Path();

  std::string ResourcePath();

  std::string Path(std::string forResourceName, std::string ofType = "",
                   std::string inDirectory = "");
};
}  // namespace UI
