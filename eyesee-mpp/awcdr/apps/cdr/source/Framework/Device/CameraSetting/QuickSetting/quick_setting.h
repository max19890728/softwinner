/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <Foundation.h>
#include <UIKit.h>
#include <common/extension/filesystem.h>

namespace CameraSetting {

struct QuickSetting {
  struct OptionStruct {
    UIImage background_;
    UIImage value_default_;
    UIImage value_lock_;
  };

  UIImage icon_;
  UIImage background_ = nullptr;

  std::vector<UIImage> backgrounds_;
  std::vector<OptionStruct> values_;

  // - MARK: 初始化器

  QuickSetting(filesystem::path);

  /* * * * * 其他成員 * * * * */

  UI::Bundle bundle_;

  void LoadByBundle();

  void LoadByCoder(UICoder);

  // - MARK: 設定值

 public:
  int value_;

 public:
  bool is_auto_;

  void SetValue(int);

  void SetAuto(bool);
};
}
