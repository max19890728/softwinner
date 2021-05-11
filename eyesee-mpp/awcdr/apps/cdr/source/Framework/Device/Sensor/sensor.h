/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <Device/observable.h>
#include <common/extension/filesystem.h>

#include <atomic>
#include <chrono>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>

#include "common/app_log.h"

#define LOG_TAG "Device::Sensor"

namespace Device {

class Sensor : public Device::Observable {
 public:
  static const char iio_device_path[];

  static const char rtc_path[];

  static const char power_supply_path[];

  class Result {
   public:
    std::string result_;

    operator const std::string&() const { return result_; }

    operator const int() const {
      try {
        return std::stoi(result_);
      } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 0;
      }
    }

    const std::string& operator=(const std::string& new_value) {
      result_ = new_value;
      return result_;
    }
  };

  std::mutex mutex_;
  std::atomic<bool> in_monitor_;
  std::thread monitor_;
  std::chrono::seconds delay_time_;

  Sensor();

  ~Sensor();

  static inline auto Read(filesystem::path path) -> Result {
    std::ifstream input;
    std::string result;
    input.open(path, std::ios::in);
    while (!input.eof()) {
      char temp = '\0';
      input.get(temp);
      if (input.fail()) break;
      result += temp;
    }
    RemoveNewLineIn(result);
    input.close();
    return {result};
  }

 private:
  virtual void MonitorAction() = 0;
};
}  // namespace Device
