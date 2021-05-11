/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/Sensor/sensor.h"

#include <string>

namespace Device {

const char Sensor::iio_device_path[] = "/sys/bus/iio/devices/iio\:device";

const char Sensor::rtc_path[] = "/sys/class/rtc/rtc0/";

const char Sensor::power_supply_path[] = "/sys/class/power_supply/";

Sensor::Sensor() : in_monitor_(true), delay_time_(std::chrono::seconds(1)) {
  monitor_ = std::thread{[&] {
    while (in_monitor_) {
      {
        std::lock_guard<std::mutex> lock(mutex_);
        MonitorAction();
      }
      std::this_thread::sleep_for(delay_time_);
    }
  }};
  monitor_.detach();
}

Sensor::~Sensor() { in_monitor_ = false; }
}  // namespace Device
