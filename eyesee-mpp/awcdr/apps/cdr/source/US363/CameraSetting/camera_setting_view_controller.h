/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <Device/CameraSetting/CameraInfo/camera_info_manager.h>
#include <UIKit.h>

#include <map>
#include <memory>
#include <string>

#include "Device/CameraSetting/CameraSetting/camera_setting_manager.h"

class CameraSettingViewController final : public UI::TableViewController {
 public:
  static inline std::shared_ptr<CameraSettingViewController> init() {
    auto building = std::make_shared<CameraSettingViewController>();
    building->Layout();
    return building;
  }

  CameraSettingViewController();

  // MARK: - UI::Responder

  void Layout(UI::Coder = {}) override;

  int NumberOfSectionIn() override;

  int NumberOfRowsInSection(int) override;

  UITableViewCell CellForRowAt(UI::IndexPath) override;

  void DidSelectRowAt(UI::IndexPath) override;

 private:
  SettingManager::CameraSettingManager& manager_;
  SettingManager::CameraInfoManager& info_manager_;
};
