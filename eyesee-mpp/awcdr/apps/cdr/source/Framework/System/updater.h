/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <System/logger.h>

#include <map>
#include <string>
#include <vector>

#include "Foundation.h"
#include "common/extension/filesystem.h"

namespace System {

enum class Burntable : int { Boot0, UBoot, Boot, RootFS };

class UpdaterDelegate {
 public:
  virtual void BurningProgress(/* on */ Burntable /* part */,
                               int /* progress */) = 0;
};

class Updater {
 public:
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
  System::Logger logger;

  std::string _firmware_file_path_;

 public:
  std::weak_ptr<UpdaterDelegate> delegate_;

  std::map<System::Burntable, std::string> _image_name_list_;

  auto CheckFirmwareFileExist() -> bool;

  void StartUpdate();

 private:
  auto GetMessageIn(const Error&) -> std::string;

  auto GetFirmwareFileList(std::string /* tar file */)
      -> std::vector<std::string>;

  void UnpackFileTo(std::string /* file name */,
                    /* from */ std::string /* path */,
                    /* to */ std::string /* dir */);

  auto GetFileMD5In(filesystem::path /* path */) -> std::string;

  void CheckImageMD5(std::string /* path */);

  auto GetBurnCommand(Burntable /* part */, std::string /* path */)
      -> std::string;

  void Burn(Burntable /* part */);
};
}  // namespace System
