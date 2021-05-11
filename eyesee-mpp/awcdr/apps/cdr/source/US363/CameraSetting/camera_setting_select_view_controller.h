/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <memory>

#include "Device/CameraSetting/CameraSetting/camera_setting.h"
#include "Framework/UIKit.h"

class CameraSettingSelectViewController final : public UI::TableViewController {
 public:
  static inline std::shared_ptr<CameraSettingSelectViewController> init(
      CameraSetting::CameraSetting &setting) {
    auto building = std::make_shared<CameraSettingSelectViewController>(setting);
    building->Layout();
    return building;
  }

  CameraSetting::CameraSetting &setting_;

  CameraSettingSelectViewController(CameraSetting::CameraSetting &);

  void Layout(UI::Coder = {}) override;

  int NumberOfSectionIn() override;

  int NumberOfRowsInSection(int) override;

  UITableViewCell CellForRowAt(UI::IndexPath) override;

  void DidSelectRowAt(UI::IndexPath) override;
};
