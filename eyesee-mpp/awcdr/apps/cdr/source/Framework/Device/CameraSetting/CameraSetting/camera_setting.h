/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include "UIKit/Struct/image.h"
#include "UIKit/Struct/bundle.h"

namespace CameraSetting {

struct CameraSetting {
 public:
  UIImage cell_setting_title_;
  UIImage cell_title_;
  UIImage cell_title_highlight_;

  int value_ = 0;
  UI::Bundle bundle_;

  std::vector<std::pair<UIImage, UIImage>> options_;

  std::vector<UIImage> icons_;

  CameraSetting(UI::Bundle);

  std::pair<UIImage, UIImage> CellDetailImage();

  void SetValue(int);
};
}  // namespace CameraSetting
