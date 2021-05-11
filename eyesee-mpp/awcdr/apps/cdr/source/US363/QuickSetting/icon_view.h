/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <Foundation.h>
#include <UIKit.h>

class QuickIconView final : public UI::ImageView {
  // MARK: - 初始化器
 public:
  static inline std::shared_ptr<QuickIconView> init(UIImage image, std::function<void(int)> callback) {
    auto building = std::make_shared<QuickIconView>(callback);
    building->Layout();
    building->image(image);
    return building;
  }

  QuickIconView(std::function<void(int)>);

  /* * * * * 其他成員 * * * * */

  std::function<void(int)> callback_;

  /* * * * * 繼承類別 * * * * */

  // MARK: - UI::Responder

  void TouchesBegan(UITouch const &, UIEvent const &) override;
};
