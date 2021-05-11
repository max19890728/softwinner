/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/Sensor/real_time_clock.h"

namespace Device {

RealTimeClock::RealTimeClock()
    : Sensor::Sensor(),
      date_path_(Device::Sensor::rtc_path + std::string{"date"}),
      time_path_(Device::Sensor::rtc_path + std::string{"time"}),
      date_(""),
      time_("") {
  auto delay_time = std::chrono::milliseconds(32);
  delay_time_ = std::chrono::duration_cast<std::chrono::seconds>(delay_time);
  date_.didSet = [this] {
    this->SendAction(Device::Observable::Event::value_change);
  };
  time_.didSet = [this] {
    this->SendAction(Device::Observable::Event::value_change);
  };
}

RealTimeClock::~RealTimeClock() {}

/* * * * * 繼承類別 * * * * */

void RealTimeClock::MonitorAction() {
  date_ = Read(date_path_);
  time_ = Read(time_path_);
}
}  // namespace Device
