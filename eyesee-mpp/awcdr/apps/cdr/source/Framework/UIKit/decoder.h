/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <common/extension/filesystem.h>

#include <string>

#include "common/utils/json.hpp"

#include <common/app_log.h>

#undef LOG_TAG
#define LOG_TAG "UI::Coder"

static const std::string &layout_extension = ".layout";
static const std::string &layout_path = "/usr/share/minigui/res/layout";

namespace UI {

class Coder {
  // - MARK: 初始化器
 public:
  static inline auto init(filesystem::path path) -> std::shared_ptr<Coder> {
    db_debug("%s", path.c_str());
    if (filesystem::exists(path)) {
      return std::make_shared<Coder>(path.filename(), path.parent_path());
    } else {
      return nullptr;
    }
  }

 public:
  nlohmann::json layout_;

  bool is_load_;

  Coder();

  explicit Coder(nlohmann::json layout);

  explicit Coder(std::string name, std::string path = layout_path);
};
}  // namespace UI

// 用來抽象 Json 結構的類別
using UICoder = std::shared_ptr<UI::Coder>;

#undef LOG_TAG
