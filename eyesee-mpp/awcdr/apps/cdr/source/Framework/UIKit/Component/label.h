/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <Foundation.h>

#include <memory>
#include <string>

#include "UIKit/Class/view.h"
#include "UIKit/buildable.h"

namespace UI {

class Label : public UI::View {
  // - MARK: 初始化器
 public:
  static inline std::shared_ptr<UI::Label> init(UI::Coder decoder = {}) {
    auto building = std::make_shared<UI::Label>();
    building->Layout(decoder);
    return building;
  }

  Label();

  // - MARK: 文本屬性

 public:
  String text_;

  UI::Color text_color_;

  uint32_t format_ = DT_CENTER | DT_VCENTER | DT_SINGLELINE;

 private:
  void TextDidSet();

  void TextColorDidSet();

  /* * * * * 繼承類別 * * * * */

  // - MARK: UI::View

 public:
  void Layout(UI::Coder) override;

  // - MARK: UI::LayerDelegate

 public:
  void Draw(UILayer, HDC) override;
};
}  // namespace UI

typedef std::shared_ptr<UI::Label> UILabel;
