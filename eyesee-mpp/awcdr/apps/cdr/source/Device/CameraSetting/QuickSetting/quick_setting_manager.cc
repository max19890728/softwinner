/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/CameraSetting/QuickSetting/quick_setting_manager.h"

#include <common/app_log.h>

#include "Device/CameraSetting/QuickSetting/quick_setting.h"
#include "UIKit/Struct/bundle.h"

#undef LOG_TAG
#define LOG_TAG "QuickValueTableView"

namespace SettingManager {

UI::Bundle QuickSettingManager::bundle_ = UI::Bundle{"data/US363/QuickSetting"};

QuickSettingManager::QuickSettingManager() {
  for (auto setting : filesystem::directory_iterator{bundle_.Path()}) {
    if (filesystem::is_directory(setting)) {
      settings_.emplace_back(CameraSetting::QuickSetting{setting});
    }
  }
}

QuickSettingManager::~QuickSettingManager() { db_info(""); }
}  // namespace SettingManager

#undef LOG_TAG
