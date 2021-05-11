/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <Foundation.h>
#include <UIKit.h>

#include <memory>
#include <utility>

#include "Device/CameraSetting/camera_mode_manager.h"
#include "US363/CameraModeSetting/camera_mode_view.h"

class CameraModeViewController final : public UI::ViewController,
                                       public UI::ScrollViewDelegate {
 public:
  static inline std::shared_ptr<CameraModeViewController> init() {
    auto building = std::make_shared<CameraModeViewController>();
    building->Layout();
    return building;
  }

  CameraSetting::CameraModeManager& camera_mode_;
  UIScrollView scroll_view_;
  std::pair<bool, int> need_set_mode_;
  CameraModeView::SetOrOpenSetting tap_flag_;

  CameraModeViewController();

  // MARK: - UI::View

  void Layout(UI::Coder = {}) override;

  // MARK: - UI::ScrollViewDelegate

  void DidScroll(UIScrollView) override;

  void DidEndDragging(UIScrollView, bool) override;
};
