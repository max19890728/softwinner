/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "UIKit/decoder.h"

#include <fstream>
#include <memory>

#include "common/app_log.h"
#include "common/extension/filesystem.h"

#undef LOG_TAG
#define LOG_TAG "UI::Coder"

UI::Coder::Coder() : is_load_(false) {}

UI::Coder::Coder(nlohmann::json layout) : UI::Coder::Coder() {
  layout_ = layout;
}

UI::Coder::Coder(std::string name, std::string path)
    : UI::Coder::Coder() {
  std::string layout_path;
  if (path.back() != '/') path += "/";
  layout_path = path += name;
  if (filesystem::path(layout_path).extension().empty()) {
    layout_path += layout_extension;
  }
  if (!filesystem::exists(layout_path)) {
    db_error("(%s) not exists", layout_path.c_str());
    return;
  }
  std::ifstream file(layout_path);
  layout_ << file;
}

#undef LOG_TAG
