/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <UIKit.h>

class UpdatingProgressView final : public UI::View {
  // - MARK: 初始化器
 public:
  static inline auto init(std::string name) {
    auto building = std::make_shared<UpdatingProgressView>(name);
    building->Layout();
    return building;
  }

  UpdatingProgressView(std::string);

  /* * * * * 其他成員 * * * * */

 private:
  UIView progress_bar_;
  UILabel name_label_;

 public:
  void SetProgress(int /* progress */);

  /* * * * * 繼承類別 * * * * */

  // - MARK: UI::View

  void Layout(UI::Coder = {}) override;
};
