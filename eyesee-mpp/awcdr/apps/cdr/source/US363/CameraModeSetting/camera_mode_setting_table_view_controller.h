/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <memory>

#include "Device/CameraSetting/camera_mode_manager.h"
#include "Device/CameraSetting/camera_setting.h"
#include "Framework/UIKit.h"

class CameraModeSettingTableViewController final
    : public UI::TableViewController {
 public:
  static std::shared_ptr<CameraModeSettingTableViewController> init(
      CameraSetting::CameraMode& mode) {
    auto building =
        std::make_shared<CameraModeSettingTableViewController>(mode);
    building->Layout();
    return building;
  }

  CameraModeSettingTableViewController(CameraSetting::CameraMode&);

  CameraSetting::CameraMode& mode_;

  // MARK: * * * * * 繼承類別 * * * * *

  // MARK: - UI::TableViewController
 public:
  void Layout(UI::Coder = {}) override;

  int NumberOfSectionIn() override;

  int NumberOfRowsInSection(int) override;

  std::shared_ptr<UI::TableViewCell> CellForRowAt(UI::IndexPath) override;

  void DidSelectRowAt(UI::IndexPath) override;
};
