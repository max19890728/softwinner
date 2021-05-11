/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/CameraSetting/QuickSetting/quick_setting.h"

#include <common/app_log.h>
#include <common/extension/filesystem.h>

#undef LOG_TAG
#define LOG_TAG "QuickSetting"

CameraSetting::QuickSetting::QuickSetting(filesystem::path path)
    : bundle_(UI::Bundle{path.string() + "/"}) {
  db_debug("");
  if (auto coder = UI::Coder::init(bundle_.Path() + "layout")) {
    db_debug("");
#if 1
    LoadByCoder(coder);
#else
    LoadByBundle();
#endif
  } else {
    db_debug("");
    LoadByBundle();
  }
}

void CameraSetting::QuickSetting::LoadByCoder(UICoder coder) {
  auto& layout = coder->layout_;
  if (layout.contains("icon")) {
    icon_ = UI::Image::init(layout.at("icon").get<std::string>(), "", bundle_);
  }
  if (layout.contains("background")) {
    background_ = UI::Image::init(layout.at("background").get<std::string>(),
                                  "QuickSetting/Background");
  }
  if (layout.contains("options")) {
    auto options = layout.at("options");
    for (auto& option : options) {
#if 0
      OptionStruct option_result{
        background_ :
            UI::Image::init(option.at("background").get<std::string>(),
                            "QuickSetting/CellBackground"),
        value_default_ : UI::Image::init(
            option.at("default").get<std::string>(), "", bundle_),
        value_lock_ :
            UI::Image::init(option.at("lock").get<std::string>(), "", bundle_)
      };
      values_.push_back(option_result);
#else
      values_.emplace_back(OptionStruct{
        background_ :
            UI::Image::init(option.at("background").get<std::string>(),
                            "QuickSetting/CellBackground"),
        value_default_ : UI::Image::init(
            option.at("default").get<std::string>(), "", bundle_),
        value_lock_ :
            UI::Image::init(option.at("lock").get<std::string>(), "", bundle_)
      });
#endif
    }
  }
  if (layout.contains("value_path")) {
  } else {
  }
  if (layout.contains("auto")) {
    if (layout.contains("path")) {
    } else {
    }
  }
}

void CameraSetting::QuickSetting::LoadByBundle() {
  auto bundle = bundle_;
  icon_ = UI::Image::init("icon", "", bundle);
  auto background_bundle = UI::Bundle{bundle.Path() + "Background/"};
  for (auto background :
       filesystem::directory_iterator{background_bundle.Path()}) {
    if (background.path().extension() != ".png") continue;
    if (background.path().filename().c_str()[0] == '.') continue;
    auto file_name = background.path().stem();
    auto image = UI::Image::init(file_name, "Background", bundle);
    backgrounds_.push_back(image);
  }
  auto option_bundle = UI::Bundle{bundle.Path() + "Option/"};
  for (auto option : filesystem::directory_iterator{option_bundle.Path()}) {
    if (!filesystem::is_directory(option)) continue;
    auto index = option.path().filename();
    OptionStruct option_result{
      background_ : backgrounds_.at(0),
      value_default_ : UI::Image::init("default", index, option_bundle),
      value_lock_ : UI::Image::init("lock", index, option_bundle)
    };
    values_.push_back(option_result);
  }
  try {
    if (auto setting = GetValue(bundle_.Path("value"))) {
      value_ = setting;
    }
  } catch (const std::string error_message) {
    value_ = 0;
  }
  try {
    if (auto setting = GetValue(bundle_.Path("auto"))) {
      is_auto_ = setting == 0 ? false : true;
    }
  } catch (const std::string error_message) {
    is_auto_ = false;
  }
}

void CameraSetting::QuickSetting::SetValue(int new_value) {
  value_ = new_value;
  ::SetValue(value_, bundle_.Path("value"));
}

void CameraSetting::QuickSetting::SetAuto(bool new_value) {
  is_auto_ = new_value;
  ::SetValue(new_value ? 1 : 0, bundle_.Path("auto"));
}

#undef LOG_TAG
