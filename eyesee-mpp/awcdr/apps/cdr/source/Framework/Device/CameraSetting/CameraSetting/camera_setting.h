/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <Foundation.h>
#include <System/logger.h>

#include <map>

#include "UIKit/Struct/bundle.h"
#include "UIKit/Struct/image.h"

namespace CameraSetting {

struct CameraSetting {
 public:
  UIImage cell_setting_title_;
  UIImage cell_title_;
  UIImage cell_title_highlight_;

  std::string key_;

  Int value_ = 0;
  int default_;
  UI::Bundle bundle_;

  std::vector<int> check_array_;

  // std::map<int, int> check_map_;

  std::vector<std::pair<UIImage, UIImage>> options_;

  std::vector<std::pair<UIImage, UIImage>> details_;

  std::vector<UIImage> icons_;

  CameraSetting(UI::Bundle);

  std::pair<UIImage, UIImage> CellDetailImage();

  void SetByRaw(int);

  void SetValue(int);

 private:
  System::Logger logger;
};
}  // namespace CameraSetting
