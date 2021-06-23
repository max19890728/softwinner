/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <Framework/UIKit.h>
#include <System/logger.h>

#include <memory>
#include <string>

#include "Device/CameraSetting/CameraInfo/camera_info_manager.h"
#include "Device/CameraSetting/camera_mode_manager.h"

class PreviewViewController final : public UI::ViewController {
 public:
  static inline std::shared_ptr<PreviewViewController> init() {
    auto building = std::make_shared<PreviewViewController>();
    building->Layout();
    return building;
  }

  PreviewViewController();

  void Layout(UI::Coder = {}) override;

  void ViewDidAppear() override;

  void TouchesBegan(UITouch const&, UIEvent const&) override;

  void TouchesMoved(UITouch const&, UIEvent const&) override;

  void TouchesEnded(UITouch const&, UIEvent const&) override;

  /* * * * * 其他成員 * * * * */

 private:
  static UI::Size button_size;

  System::Logger logger;

  SettingManager::CameraInfoManager& camera_info_;
  CameraSetting::CameraModeManager& camera_mode_;
  CameraSetting::CameraSetting& delay_setting_;
  UIButton delay_setting_button_;
  UIButton storage_info_button_;
  UIButton camera_mode_button_;
  UIButton connection_info_button_;
  UIButton battery_info_button;

  void ButtonAction(UIControl);

  /* * * * * 繼承類別 * * * * */
 public:
  // MARK: - UI::Responder

  void PressesEnded(UIPress const&) override;
};
