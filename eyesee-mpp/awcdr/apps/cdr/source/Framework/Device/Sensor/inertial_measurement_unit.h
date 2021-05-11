/* *******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 * *******************************************************************************/

#pragma once

#include <Device/Sensor/sensor.h>
#include <Foundation.h>

#include <map>
#include <string>

namespace Device {

class InertialMeasurementUnit final : public Sensor {
 public:
  struct Data : public Protocol::Property<Data> {
   public:
    Data(int, int, int);

    Int x;
    Int y;
    Int z;

    auto get() const -> const Data& override { return *this; }

    auto set(const Data& new_value) -> const Data& override {
      if (x != new_value.x) x = new_value.x;
      if (y != new_value.x) y = new_value.y;
      if (z != new_value.z) z = new_value.z;
      return *this;
    }
  };

  // - MARK: 初始化器

 public:
  static InertialMeasurementUnit& instance() {
    static InertialMeasurementUnit instance;
    return instance;
  }

 private:
  InertialMeasurementUnit();
  ~InertialMeasurementUnit();
  InertialMeasurementUnit(const InertialMeasurementUnit&) = delete;
  InertialMeasurementUnit& operator=(const InertialMeasurementUnit&) = delete;

  /* * * * * 其他成員 * * * * */

 private:
  const std::map<std::string, std::string> accelerometer_path_;
  const std::map<std::string, std::string> gyroscope_path_;
  const std::map<std::string, std::string> magnetic_path_;

 public:
  Data accelerometer_;
  Data gyroscope_;
  Data magnetic_;

  /* * * * * 繼承類別 * * * * */

  void MonitorAction() override;
};
}  // namespace Device
