/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <UIKit.h>

#include "Device/CameraSetting/CameraInfo/camera_info.h"

class InfoTableViewController final : public UI::TableViewController {
  // MARK: - 初始化器
 public:
  static inline std::shared_ptr<InfoTableViewController> init(
      CameraInformation::CameraInformation& info) {
    auto building = std::make_shared<InfoTableViewController>(info);
    building->Layout();
    return building;
  }

  InfoTableViewController(CameraInformation::CameraInformation&);

  /* * * * * 其他成員 * * * * */

  CameraInformation::CameraInformation& info_;

  /* * * * * 繼承類別 * * * * */

  // MARK: - UI::Responder

  void Layout(UI::Coder = {}) override;

  // MARK: - UI::TableViewDataSource

  int NumberOfSectionIn();

  int NumberOfRowsInSection(int);

  UITableViewCell CellForRowAt(UI::IndexPath);

  // MARK: - UI::TableViewDelegate

  void DidSelectRowAt(UI::IndexPath);
};
