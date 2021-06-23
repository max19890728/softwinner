/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <Foundation.h>

#include <memory>

#include "UIKit/Component/image_view.h"
#include "UIKit/Control/control.h"
#include "UIKit/Struct/image.h"

namespace UI {

class Switch : public UI::Control {
  // MARK: - 初始化器
 public:
  static inline auto init() {
    auto building = std::make_shared<UI::Switch>();
    building->Layout();
    return building;
  }

  Switch();

  // MARK: - 開關狀態

 public:
  Bool is_on_;

  #if 0
  void setOn(bool, /* with animation */ bool);
  #endif

 private:
  void IsOnDidSet();

  // MARK: - 開關外觀
 private:
  UIImageView switch_image_view_;

 public:
  UIImage on_image_;
  UIImage off_image_;

  /* * * * * 繼承類別 * * * * */

  // MARK: - UI::View

 public:
  void Layout(UI::Coder decoder = {}) override;

  // MARK: - UI::Control

  std::vector<Control::Event> AllControlEvents() override;

  /* * * * * 其他成員 * * * * */

 public:
 private:
  void TouchAction(UIControl);
};
}  // namespace UI

typedef std::shared_ptr<UI::Switch> UISwitch;
