/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "DebugKit/updater.h"

#include <algorithm>
#include <string>
#include <vector>

#include "DebugKit/command_line.h"
#include "common/app_log.h"
#include "common/extension/filesystem.h"

#undef LOG_TAG
#define LOG_TAG "DebugKit::Updater"

const char* DebugKit::Updater::__dir_path__ = "/mnt/extsd/";
const char* DebugKit::Updater::__file_name__ = "ota.tar";
const char* DebugKit::Updater::__temp_dir_name__ = "/mnt/extsd/temp/";

DebugKit::Updater::Updater() {
  _firmware_file_path_ = std::string{__dir_path__} + std::string{__file_name__};
  _image_name_list_.insert(std::make_pair(Burntable::Boot0, "boot0.img"));
  _image_name_list_.insert(std::make_pair(Burntable::UBoot, "uboot.img"));
  _image_name_list_.insert(std::make_pair(Burntable::Boot, "boot.img"));
  _image_name_list_.insert(std::make_pair(Burntable::RootFS, "rootfs.img"));
}

DebugKit::Updater::~Updater() {}

bool DebugKit::Updater::CheckFirmwareFileExist() {
  return filesystem::exists(_firmware_file_path_);
}

void DebugKit::Updater::StartUpdate() {
  auto file_list = GetFirmwareFileList(_firmware_file_path_);
  if (filesystem::exists(__temp_dir_name__)) {
    filesystem::remove_all(__temp_dir_name__);
  }
  filesystem::create_directory(__temp_dir_name__);
  try {
    for (const auto& file_name : file_list) {
      UnpackFileTo(file_name, _firmware_file_path_, __temp_dir_name__);
    }
    for (const auto& file_name : file_list) {
      filesystem::path file_path = __temp_dir_name__ + file_name;
      if (file_path.extension() == ".img") CheckImageMD5(file_path);
    }
    filesystem::remove(_firmware_file_path_);
    Burn(Burntable::Boot0);
    Burn(Burntable::UBoot);
    Burn(Burntable::Boot);
    Burn(Burntable::RootFS);
  } catch (const Error& error) {
    db_error("%s", GetMessageIn(error).c_str());
  }
  filesystem::remove_all(__temp_dir_name__);
  DebugKit::CommandLine::Send("reboot");
}

std::vector<std::string> DebugKit::Updater::GetFirmwareFileList(
    std::string tar_path) {
  std::vector<std::string> file_list;
  DebugKit::CommandLine::Send("tar -tf " + tar_path, [&](auto file_name) {
    file_list.push_back(file_name);
  });
  return file_list;
}

std::string DebugKit::Updater::GetMessageIn(const Error& error) {
  std::string message;
  if (!error.file_name.empty()) message += "File: " + error.file_name;
  message += ", " + error.error_message;
  return message;
}

void DebugKit::Updater::UnpackFileTo(std::string name, std::string from,
                                     std::string to_dir) {
  try {
    DebugKit::CommandLine::Send("tar -xv -f " + from + " " + name + " -C " +
                                to_dir);
  } catch (bool isFailed) {
    throw DebugKit::Updater::Error{
      file_name : name,
      error_message : "Unpack " + name + " to " + to_dir + "failed"
    };
  }
}

std::string DebugKit::Updater::GetFileMD5In(filesystem::path file_path) {
  std::string md5_string;
  try {
    DebugKit::CommandLine::Send(
        file_path.extension() == ".img"
            ? "md5sum " + file_path.string() + " | awk '{print $1}'"
            : "cat " + file_path.string(),
        [&](auto callback) { md5_string += callback; });
  } catch (bool isFailed) {
    throw DebugKit::Updater::Error{
      file_name : file_path.filename(),
      error_message : "Get file MD5 failed"
    };
  }
  return md5_string;
}

void DebugKit::Updater::CheckImageMD5(std::string file_path) {
  try {
    if (GetFileMD5In(file_path) != GetFileMD5In(file_path + ".md5")) {
      throw DebugKit::Updater::Error{
        file_name : filesystem::path{file_path}.filename(),
        error_message : "MD5 Check Failed"
      };
    }
  } catch (const Error& error) {
    throw error;
  }
}

std::string DebugKit::Updater::GetBurnCommand(Burntable burn_part,
                                              std::string image_path) {
  switch (burn_part) {
    case Burntable::Boot0:
      return "ota-burnboot0 " + image_path;
    case Burntable::UBoot:
      return "ota-burnuboot " + image_path;
    case Burntable::Boot:
      return "dd if=" + image_path + " of=/dev/by-name/boot conv=fsync";
    case Burntable::RootFS:
      return "dd if=" + image_path + " of=/dev/by-name/rootfs conv=fsync";
  }
}

void DebugKit::Updater::Burn(Updater::Burntable burn_part) {
  std::string file_name;
  auto iter = _image_name_list_.find(burn_part);
  if (iter == _image_name_list_.end()) {
    throw DebugKit::Updater::Error{
      file_name : "Not find",
      error_message : "Not find image file name in list"
    };
  } else {
    file_name = iter->second;
  }
  std::string image_path = __temp_dir_name__ + file_name;
  if (!filesystem::exists(image_path)) {
    throw DebugKit::Updater::Error{
      file_name : file_name,
      error_message : "The image file is not exists"
    };
  }
  try {
    DebugKit::CommandLine::Send(
        GetBurnCommand(burn_part, image_path),
        [](auto message) { db_info("%s", message.c_str()); });
  } catch (bool isFailed) {
    throw DebugKit::Updater::Error{
      file_name : file_name,
      error_message : "Burn part process start failed"
    };
  }
}
