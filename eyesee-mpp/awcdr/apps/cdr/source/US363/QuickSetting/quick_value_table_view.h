/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <Device/CameraSetting/QuickSetting/quick_setting.h>
#include <Foundation.h>
#include <UIKit.h>

class QuickValueTableView final : public UI::TableView,
                                  public UI::TableViewDelegate,
                                  public UI::TableViewDataSource {
  // - MARK: 初始化器
 public:
  static inline std::shared_ptr<QuickValueTableView> init(
      CameraSetting::QuickSetting& setting) {
    auto building = std::make_shared<QuickValueTableView>(setting);
    building->Layout();
    return building;
  }

  QuickValueTableView(CameraSetting::QuickSetting&);

  /* * * * * 其他成員 * * * * */

  CameraSetting::QuickSetting* setting_;

  void SetInsetAndReload();

  void SetNewSetting(CameraSetting::QuickSetting&);

  /* * * * * 繼承類別 * * * * */

 public:
  // - MARK: UI::TableView

  void Layout(UI::Coder = {}) override;

  // - MARK: UI::TableViewDataSource

  int NumberOfSectionIn() override;

  int NumberOfRowsInSection(int) override;

  UITableViewCell CellForRowAt(UI::IndexPath) override;

  // - MARK: UI::TableViewDelegate

  void DidSelectRowAt(UI::IndexPath) override;

  void DidEndDecelerating(UIScrollView) override;
};
