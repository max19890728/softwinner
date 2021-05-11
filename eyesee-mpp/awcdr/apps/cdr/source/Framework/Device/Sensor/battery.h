/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <Device/Sensor/sensor.h>
#include <Foundation.h>

#include <string>

namespace Device {

class Battery final : public Sensor {
  // - MARK: 初始化器

 public:
  static Battery& instance() {
    static Battery instance;
    return instance;
  }

 private:
  Battery();
  ~Battery();
  Battery(const Battery&) = delete;
  Battery& operator=(const Battery&) = delete;

  /* * * * * 其他成員 * * * * */

 private:
  const std::string capacity_path_;
  const std::string voltage_path_;

 public:
  Variable<int> capacity_;
  Variable<int> voltage_;

  /* * * * * 繼承類別 * * * * */

 private:
  void MonitorAction() override;
};
}  // namespace Device
