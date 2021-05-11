/* *******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 * *******************************************************************************/

#include "Device/Sensor/light_sensor.h"

namespace Device {

// - MARK: 初始化器

LightSensor::LightSensor()
    : Sensor::Sensor(),
      path_(Sensor::iio_device_path + std::string{"7/in_illuminance_raw"}),
      value_(0) {
  value_.didSet = [this] { this->SendAction(Observable::Event::value_change); };
}

LightSensor::~LightSensor() {}

/* * * * * 繼承類別 * * * * */

void LightSensor::MonitorAction() { value_ = Read(path_); }
}  // namespace Device
