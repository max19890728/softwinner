/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <memory>
#include <set>

#include "UIKit/Struct/event.h"
#include "UIKit/Struct/press.h"

namespace UI {

class Responder {
  // MARK: 管理響應者鏈
 public:
  std::shared_ptr<UI::Responder> next_ = nullptr;

  // MARK: 管理觸摸事件
  virtual void TouchesBegan(UITouch const&, UIEvent const&);

  virtual void TouchesMoved(UITouch const&, UIEvent const&);

  virtual void TouchesEnded(UITouch const&, UIEvent const&);

  // MARK: 管理按鍵事件
  virtual void PressesBegin(UIPress const&);

  virtual void PressesChanged(UIPress const&);

  virtual void PressesEnded(UIPress const&);
};
}  // namespace UI
