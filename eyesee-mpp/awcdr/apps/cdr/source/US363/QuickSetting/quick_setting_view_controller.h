/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <Device/CameraSetting/QuickSetting/quick_setting_manager.h>
#include <UIKit.h>

#include <memory>

#include "US363/QuickSetting/quick_value_table_view.h"

class QuickSettingViewController final : public UI::ViewController {
 public:
  static inline std::shared_ptr<QuickSettingViewController> init() {
    auto building = std::make_shared<QuickSettingViewController>();
    building->Layout();
    return building;
  }

  QuickSettingViewController();

  /* * * * * 其他成員 * * * * */

 public:
  SettingManager::QuickSettingManager& manager_;
  int now_setting_index_;
  CameraSetting::QuickSetting* now_setting_;
  std::shared_ptr<QuickValueTableView> setting_table_view_;
  UISwitch auto_switch_;

  void LayoutAutoButton();

  void LayoutIconScrollView();

  void LayoutValueTableView();

  void SetValueScrollView(int);

  void AutoSwitchAction(std::shared_ptr<UI::Control>);

  /* * * * * 繼承類別 * * * * */

  // MARK: - UI::ViewController

  void Layout(UI::Coder = {}) override;
};
