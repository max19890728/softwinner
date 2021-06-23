/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/CameraSetting/CameraSetting/camera_setting.h"

#include <Device/databin.h>
#include <common/extension/filesystem.h>

CameraSetting::CameraSetting::CameraSetting(UI::Bundle bundle)
    : bundle_(bundle), logger(System::Logger(std::string{"CameraSetting"})) {
  cell_setting_title_ = UI::Image::init("setting_title", "", bundle);
  cell_title_ = UI::Image::init("cell_title", "", bundle);
  cell_title_highlight_ = UI::Image::init("cell_title_highlight", "", bundle);
  for (auto option : filesystem::directory_iterator{bundle.Path()}) {
    if (filesystem::is_directory(option)) {
      auto filename = option.path().filename();
      options_.emplace_back(
          std::make_pair(UI::Image::init("default", filename, bundle),
                         UI::Image::init("highlight", filename, bundle)));
      details_.emplace_back(std::make_pair(
          UI::Image::init("detail", filename, bundle),
          UI::Image::init("detail_highlight", filename, bundle)));
      icons_.emplace_back(UI::Image::init("icon", filename, bundle));
    }
  }
  auto type_layout_path = bundle.Path() + "/type.layout";
  Log(LogLevel::Debug) << type_layout_path << '\n';
  if (filesystem::exists(type_layout_path)) {
    std::ifstream file{type_layout_path};
    Json layout;
    layout << file;
    try {  // 取得設定鍵
      Log(LogLevel::Debug) << '\n';
      key_ = layout.at("key").get<std::string>();
      Log(LogLevel::Debug) << key_ << '\n';
      logger.SetLoggerName(key_);
      Log(LogLevel::Debug) << value_ << '\n';
    } catch (Json::parse_error& error) {
      Log(LogLevel::Error) << '\n';
      value_ = 0;
    }
    try {  // 取得檢查表
      auto option_list = layout.at("option_list");
      for (auto const& option : option_list) {
        check_array_.emplace_back(option.at("raw").get<int>());
      }
    } catch (Json::parse_error& error) {
      Log(LogLevel::Error) << '\n';
    }
    if (!key_.empty()) {
      SetByRaw(Device::DataBin::instance().GetValue<int>(key_));
    }
    Log(LogLevel::Debug) << '\n';
    try {  // 取得預設
      // default_ = layout.at("default").get<int>();
    } catch (Json::parse_error& error) {
      Log(LogLevel::Error) << '\n';
    }
    Log(LogLevel::Debug) << value_ << '\n';
    // value_.willSet = [](auto& new_value) { return new_value; };
  }
}

std::pair<UIImage, UIImage> CameraSetting::CameraSetting::CellDetailImage() {
  if (details_.size() > value_) {
    Log(LogLevel::Debug) << '\n';
    auto result = details_.at(value_);
    return std::make_pair(result.first, result.second);
  } else if (options_.size() > value_) {
    Log(LogLevel::Debug) << '\n';
    auto result = options_.at(value_);
    return std::make_pair(result.first, result.second);
  } else {
    Log(LogLevel::Debug) << '\n';
    return std::make_pair(nullptr, nullptr);
  }
}

void CameraSetting::CameraSetting::SetByRaw(int raw_value) {
  Log(LogLevel::Debug) << raw_value << '\n';
  auto iter = std::find(check_array_.begin(), check_array_.end(), raw_value);
  if (iter == check_array_.end()) {
    value_ = 0;
  } else {
    value_ = std::distance(check_array_.begin(), iter);
  }
  Log(LogLevel::Debug) << value_ << '\n';
}

void CameraSetting::CameraSetting::SetValue(int new_value) {
  Log(LogLevel::Debug) << "Old: " << value_ << ", ";
  value_ = new_value;
  Log(LogLevel::Debug) << "New: " << new_value << '\n';
  if (new_value < check_array_.size()) {
    Device::DataBin::instance().SetValue(key_, check_array_[new_value]);
  }
  Log(LogLevel::Debug) << '\n';
}
