/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <Foundation.h>

#include <memory>

#include "UIKit/Component/image_view.h"
#include "UIKit/Control/control.h"

namespace UI {

class Button : public UI::Control {
  // MARK: - 初始化器
 public:
  static inline auto init() {
    auto building = std::make_shared<UI::Button>();
    building->Layout();
    return building;
  }

  Button();

  // MARK: - 配置按鈕標題

  // MARK: - 配置按鈕影像
 private:
  std::map<UI::Control::State, UIImage> image_table_;

 public:
  UIImage Image(/* for */ UI::Control::State);

  void SetImage(UIImage, /* for */ UI::Control::State);

  // MARK: - 當前按鈕狀態

 public:
  UIImageView image_view_;

  // MARK: -

  /* * * * * 繼承類別 * * * * */

  // MARK: - UI::Control

  std::vector<Control::Event> AllControlEvents() override;
};
}  // namespace UI

typedef std::shared_ptr<UI::Button> UIButton;
