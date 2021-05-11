/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "Device/Sensor/battery.h"

namespace Device {

Battery::Battery()
    : Sensor::Sensor(),
      capacity_path_(Sensor::power_supply_path +
                     std::string{"battery/capacity"}),
      voltage_path_(Sensor::power_supply_path +
                    std::string{"battery/voltage_now"}),
      capacity_(0),
      voltage_(0) {
  capacity_.didSet = [this] {
    this->SendAction(Observable::Event::value_change);
  };
  voltage_.didSet = [this] {
    this->SendAction(Observable::Event::value_change);
  };
}

Battery::~Battery() {}

/* * * * * 繼承類別 * * * * */

void Battery::MonitorAction() {
  capacity_ = Read(capacity_path_);
  voltage_ = Read(voltage_path_);
}
}  // namespace Device
