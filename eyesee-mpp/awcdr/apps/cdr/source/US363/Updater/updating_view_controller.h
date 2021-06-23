/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <System/logger.h>
#include <System/updater.h>
#include <UIKit.h>

#include "US363/Updater/updating_progress_view.h"

class UpdatingViewController final : public UI::ViewController,
                                     public System::UpdaterDelegate {
  // - MARK: 初始化器
 public:
  static inline auto init() {
    auto building = std::make_shared<UpdatingViewController>();
    building->Layout();
    return building;
  }

  UpdatingViewController();

  /* * * * * 其他成員 * * * * */

 private:
  System::Logger logger;

  System::Updater& updater_;

  std::map<System::Burntable, std::shared_ptr<UpdatingProgressView>>
      progress_views_;

  /* * * * * 繼承類別 * * * * */

  // - MARK: UI::ViewController

 public:
  void Layout(UI::Coder = {}) override;

  void ViewDidLoad() override;

  void ViewDidAppear() override;

  // - MARK: System::UpdaterDelegate

  void BurningProgress(/* on */ System::Burntable /* part */,
                       int /* progress */) override;
};
