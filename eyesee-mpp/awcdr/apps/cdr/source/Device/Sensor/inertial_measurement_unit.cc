/* *******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 * *******************************************************************************/

#include "Device/Sensor/inertial_measurement_unit.h"

#include <map>
#include <string>

namespace Device {

InertialMeasurementUnit::Data::Data(int x, int y, int z) : x(x), y(y), z(z) {
  this->x.didSet = [this] {
    if (this->didSet) this->didSet();
  };
  this->y.didSet = [this] {
    if (this->didSet) this->didSet();
  };
  this->z.didSet = [this] {
    if (this->didSet) this->didSet();
  };
}

InertialMeasurementUnit::InertialMeasurementUnit()
    : Device::Sensor::Sensor(),
      accelerometer_path_(
          {{"x", Sensor::iio_device_path + std::string{"0/in_accel_x_raw"}},
           {"y", Sensor::iio_device_path + std::string{"0/in_accel_y_raw"}},
           {"z", Sensor::iio_device_path + std::string{"0/in_accel_z_raw"}}}),
      gyroscope_path_({{"x", Sensor::iio_device_path + std::string{"1/in_anglvel_x_raw"}},
                       {"y", Sensor::iio_device_path + std::string{"1/in_anglvel_y_raw"}},
                       {"z", Sensor::iio_device_path + std::string{"1/in_anglvel_z_raw"}}}),
      magnetic_path_({{"x", Sensor::iio_device_path + std::string{"6/in_magn_x_raw"}},
                      {"y", Sensor::iio_device_path + std::string{"6/in_magn_y_raw"}},
                      {"z", Sensor::iio_device_path + std::string{"6/in_magn_z_raw"}}}),
      accelerometer_(Data{0, 0, 0}),
      gyroscope_(Data{0, 0, 0}),
      magnetic_(Data{0, 0, 0}) {
  accelerometer_.didSet = [this] {
    this->SendAction(Observable::Event::value_change);
  };
  gyroscope_.didSet = [this] {
    this->SendAction(Observable::Event::value_change);
  };
  magnetic_.didSet = [this] {
    this->SendAction(Observable::Event::value_change);
  };
}

InertialMeasurementUnit::~InertialMeasurementUnit() {}

/* * * * * 繼承類別 * * * * */

void InertialMeasurementUnit::MonitorAction() {
  accelerometer_.x = Read(accelerometer_path_.at("x"));
  accelerometer_.y = Read(accelerometer_path_.at("y"));
  accelerometer_.z = Read(accelerometer_path_.at("z"));
  gyroscope_.x = Read(gyroscope_path_.at("x"));
  gyroscope_.y = Read(gyroscope_path_.at("y"));
  gyroscope_.z = Read(gyroscope_path_.at("z"));
  magnetic_.x = Read(magnetic_path_.at("x"));
  magnetic_.y = Read(magnetic_path_.at("y"));
  magnetic_.z = Read(magnetic_path_.at("z"));
}
}  // namespace Device
