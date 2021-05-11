/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <Device/Sensor/sensor.h>
#include <Foundation.h>

#include <string>

namespace Device {

class RealTimeClock final : public Sensor {
  // - MARK: 初始化器

 public:
  static RealTimeClock &instance() {
    static RealTimeClock instance;
    return instance;
  }

 private:
  RealTimeClock();
  ~RealTimeClock();
  RealTimeClock(const RealTimeClock &) = delete;
  RealTimeClock &operator=(const RealTimeClock &) = delete;

  /* * * * * 其他成員 * * * * */

 private:
  const std::string date_path_;
  const std::string time_path_;

 public:
  Variable<std::string> date_;
  Variable<std::string> time_;

  /* * * * * 繼承類別 * * * * */

  void MonitorAction() override;
};
}  // namespace Device
