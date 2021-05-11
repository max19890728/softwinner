/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include "Device/CameraSetting/QuickSetting/quick_setting.h"
#include "UIKit/Struct/bundle.h"

namespace SettingManager {

class QuickSettingManager {
  // MARK: - 初始化器
 public:
  static QuickSettingManager& instance() {
    static QuickSettingManager instance_;
    return instance_;
  }

 private:
  QuickSettingManager();
  ~QuickSettingManager();
  QuickSettingManager(QuickSettingManager const&) = delete;
  QuickSettingManager& operator=(QuickSettingManager const&) = delete;

  // MARK: -

 private:
  static UI::Bundle bundle_;

 public:
  std::vector<CameraSetting::QuickSetting> settings_;
};

}  // namespace SettingManager
