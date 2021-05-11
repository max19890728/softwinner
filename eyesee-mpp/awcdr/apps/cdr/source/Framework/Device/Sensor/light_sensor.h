/* *******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 * *******************************************************************************/

#pragma once

#include <Device/Sensor/sensor.h>
#include <Foundation.h>

#include <string>

namespace Device {

class LightSensor final : public Sensor {
  // - MARK: 初始化器

 public:
  static LightSensor& instance() {
    static LightSensor instance;
    return instance;
  }

 private:
  LightSensor();
  ~LightSensor();
  LightSensor(const LightSensor&) = delete;
  LightSensor& operator=(const LightSensor&) = delete;

  /* * * * * 其他成員 * * * * */

 private:
  const std::string path_;

 public:
  Variable<int> value_;

  /* * * * * 繼承類別 * * * * */

  void MonitorAction() override;
};
}  // namespace Device
