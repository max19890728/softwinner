/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <UIKit.h>

#include <memory>

#include "Device/CameraSetting/camera_mode_manager.h"

class CameraModeView final : public UI::ImageView {
 public:
  enum class SetOrOpenSetting { set, open };

  static inline std::shared_ptr<CameraModeView> init(
      CameraSetting::CameraMode mode, int index,
      std::function<void(int, SetOrOpenSetting)> callback) {
    auto building = std::make_shared<CameraModeView>(mode, index, callback);
    building->Layout();
    return building;
  }

  CameraModeView(CameraSetting::CameraMode, int,
                 std::function<void(int, SetOrOpenSetting)>);

  void Layout(UI::Coder = {}) override;

  void TouchesBegan(UITouch const&, UIEvent const&) override;

 private:
  int index_;
  CameraSetting::CameraMode mode_;
  std::function<void(int, SetOrOpenSetting)> callback_;
};
