/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <sys/types.h>

#include <memory>

#include "UIKit/Struct/key.h"

namespace UI {

class Window;

class Responder;

struct Press {

  enum class Phase { began, ended };

  // MARK: - 初始化器
  static inline std::unique_ptr<UI::Press> init(int key_code) {
    return std::make_unique<UI::Press>(key_code);
  }

  Press(int raw_code);

  // MARK: - 按鍵觸發的位置

  std::shared_ptr<UI::Window> window_;

  std::shared_ptr<UI::Responder> target_;

  // MAKE: - 按鍵屬性

  UI::Key key_;

  UI::Press::Phase phase_;

  // MARK: - 設定狀態

  void set_phase(uint);
};
} // namespace UI

typedef std::unique_ptr<UI::Press> UIPress;
