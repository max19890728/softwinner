/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <Device/CameraSetting/QuickSetting/quick_setting.h>
#include <Foundation.h>
#include <UIKit.h>

#include <memory>

class IconSelectView final : public UI::ScrollView, public UI::ScrollViewDelegate {
  // MARK: - 初始化器
 public:
  static inline std::shared_ptr<IconSelectView> init(std::vector<UIImage> images,
                                                     std::function<void(int)> callback) {
    auto building = std::make_shared<IconSelectView>(images, callback);
    building->Layout();
    return building;
  }

  IconSelectView(std::vector<UIImage>, std::function<void(int)>);

  /* * * * * 其他成員 * * * * */

  std::vector<UIImage> icons_;
  std::vector<UIImageView> icon_views_;
  std::pair<bool, int> tapped_;
  std::function<void(int)> callback_;

  /* * * * * 繼承類別 * * * * */

  // MARK: - UI::Responder

  void TouchesEnded(UITouch const &, UIEvent const &) override;

  // MARK: - UI::ScrollView

  void Layout(UI::Coder = {}) override;

  // MARK: - UI::ScrollViewDelegate

  void DidScroll(UIScrollView) override;

  void DidEndDecelerating(UIScrollView) override;
};
