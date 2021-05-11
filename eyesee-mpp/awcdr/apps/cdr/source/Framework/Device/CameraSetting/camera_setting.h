/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <utility>

#include "Device/CameraSetting/infomation.h"
#include "Device/CameraSetting/settable.h"
#include "UIKit/Struct/image.h"

namespace Protocol {

struct CameraSetting : public Protocol::Settable, public Protocol::Infomation {
  // 當只有一個設定值時為 `true` 且啟用單選表格選單，預設值為 `true`。
  virtual bool OnlySettable() { return true; }

  // 當設定有大型資訊時為 `true`，預設值為 `false`。
  virtual bool HaveInfomation() { return false; }

  // 子表格導航列標題文字。
  virtual UIImage NavigationBarTitle() = 0;

  // 設定表格標題文字，first: Normal 標題， second: Highlight 標題
  virtual std::pair<UIImage, UIImage> Title() = 0;

  // first: Normal 標題， second: Highlight 標題
  virtual std::pair<UIImage, UIImage> DetailLabel() = 0;
};
}  // namespace Protocol
