/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <map>
#include <string>
#include <vector>

#include "common/extension/filesystem.h"

namespace DebugKit {

class Updater {
 public:
  enum class Burntable { Boot0, UBoot, Boot, RootFS };

  struct Error {
    std::string file_name;
    std::string error_message;
  };

  static Updater& instance() {
    static Updater instance_;
    return instance_;
  }

 private:
  static const char* __dir_path__;
  static const char* __file_name__;
  static const char* __temp_dir_name__;

  Updater();
  ~Updater();
  Updater(const Updater&) = delete;
  Updater& operator=(const Updater&) = delete;

 private:
  std::string _firmware_file_path_;
  std::map<DebugKit::Updater::Burntable, std::string> _image_name_list_;

 public:
  bool CheckFirmwareFileExist();

  void StartUpdate();

 private:
  std::string GetMessageIn(const Error& error);

  std::vector<std::string> GetFirmwareFileList(std::string tar_path);

  void UnpackFileTo(std::string name, std::string from, std::string to_dir);

  std::string GetFileMD5In(filesystem::path file_path);

  void CheckImageMD5(std::string file_path);

  std::string GetBurnCommand(Burntable burn_part, std::string image_path);

  void Burn(Burntable burn_part);
};
}  // namespace DebugKit
