/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "US363/debug_view_controller.h"

#include <Device/Sensor/battery.h>
#include <Device/Sensor/inertial_measurement_unit.h>
#include <Device/Sensor/light_sensor.h>
#include <Device/Sensor/real_time_clock.h>
#include <Foundation.h>
#include <UIKit.h>

#include <string>

#include "common/app_log.h"

#undef LOG_TAG
#define LOG_TAG "DebugViewController"

#undef Self
#define Self DebugViewController

Self::Self()
    : UI::ViewController::ViewController(),
      time_label_(UI::Label::init()),
      battery_label_(UI::Label::init()),
      light_sensor_label_(UI::Label::init()),
      inertial_measurement_unit_label_(UI::Label::init()),
      touch_point_test_(UI::View::init(UI::Rect{
        origin : UI::Point{x : 0, y : 0},
        size : UI::Size{width : 80, height : 80}
      })) {
  Device::RealTimeClock::instance().AddTarget(
      this,
      std::bind(&Self::RealTimeClockChangeAction, this, std::placeholders::_1),
      Device::Observable::Event::value_change);
  Device::Battery::instance().AddTarget(
      this, std::bind(&Self::BatteryChangeAction, this, std::placeholders::_1),
      Device::Observable::Event::value_change);
  Device::LightSensor::instance().AddTarget(
      this,
      std::bind(&Self::LightSensorChangeAction, this, std::placeholders::_1),
      Device::Observable::Event::value_change);
  Device::InertialMeasurementUnit::instance().AddTarget(
      this,
      std::bind(&Self::InertialMeasurementUnitChangeAction, this,
                std::placeholders::_1),
      Device::Observable::Event::value_change);
}

/* * * * * 其他成員 * * * * */

void Self::RealTimeClockChangeAction(Any) {
  auto time_text = std::string{};
  time_text += "Real Time Clock:\n";
  time_text += "    date: ";
  time_text += Device::RealTimeClock::instance().date_;
  time_text += "\n    time: ";
  time_text += Device::RealTimeClock::instance().time_;
  time_label_->text_ = time_text;
}

void Self::BatteryChangeAction(Any) {
  auto battery_text = std::string{};
  battery_text += "Battery:\n";
  battery_text += "    Capacity: ";
  battery_text += std::to_string(Device::Battery::instance().capacity_);
  battery_text += "\n    Voltage: ";
  battery_text += std::to_string(Device::Battery::instance().voltage_);
  battery_label_->text_ = battery_text;
}

void Self::LightSensorChangeAction(Any) {
  light_sensor_label_->text_ =
      "Light Sensor: " + std::to_string(Device::LightSensor::instance().value_);
}

void Self::InertialMeasurementUnitChangeAction(Any) {
  auto &imu = Device::InertialMeasurementUnit::instance();
  std::string imu_text = "";
  imu_text += "Inertial Measurement Unit:\n";
  imu_text += "  Acceleromete:\n";
  imu_text += "    x:" + std::to_string(imu.accelerometer_.x);
  imu_text += ", y:" + std::to_string(imu.accelerometer_.y);
  imu_text += ", z:" + std::to_string(imu.accelerometer_.z) + "\n";
  imu_text += "  Gyroscope:\n";
  imu_text += "    x:" + std::to_string(imu.gyroscope_.x);
  imu_text += ", y:" + std::to_string(imu.gyroscope_.y);
  imu_text += ", z:" + std::to_string(imu.gyroscope_.z) + "\n";
  imu_text += "  Measyrement:\n";
  imu_text += "    x:" + std::to_string(imu.magnetic_.x);
  imu_text += ", y:" + std::to_string(imu.magnetic_.y);
  imu_text += ", z:" + std::to_string(imu.magnetic_.z) + "\n";
  inertial_measurement_unit_label_->text_ = imu_text;
}

/* * * * * 繼承類別 * * * * */

// MAKR: UI::ViewController

void Self::Layout(UI::Coder) {
  view_->frame(UI::Screen::bounds);
  view_->background_color_ = UI::Color::white;
  touch_point_test_->is_hidden_ = true;
  view_->addSubView(touch_point_test_);
  touch_point_test_->background_color_ = UI::Color{0xFFFF0000};
  time_label_->frame(UI::Rect{
    origin : UI::Point{x : 0, y : 0},
    size : UI::Size{width : 240, height : 48}
  });
  view_->addSubView(time_label_);
  battery_label_->frame(UI::Rect{
    origin : UI::Point{x : 0, y : 48},
    size : UI::Size{width : 240, height : 48}
  });
  view_->addSubView(battery_label_);
  light_sensor_label_->frame(UI::Rect{
    origin : UI::Point{x : 0, y : 96},
    size : UI::Size{width : 240, height : 16}
  });
  view_->addSubView(light_sensor_label_);
  inertial_measurement_unit_label_->frame(UI::Rect{
    origin : UI::Point{x : 0, y : 112},
    size : UI::Size{width : 240, height : 112}
  });
  view_->addSubView(inertial_measurement_unit_label_);
}

void Self::ViewWillDisappear() {
  Device::RealTimeClock::instance().RemoveTarget(this);
  Device::Battery::instance().RemoveTarget(this);
  Device::LightSensor::instance().RemoveTarget(this);
  Device::InertialMeasurementUnit::instance().RemoveTarget(this);
}

// MAKR: UI::Responder

void Self::TouchesBegan(UITouch const &touch, UIEvent const &event) {
  touch_point_test_->frame(UI::Rect{
    origin : {x : touch->touch_point_.x - 40, y : touch->touch_point_.y - 40},
    size : touch_point_test_->frame().size
  });
  touch_point_test_->is_hidden_ = false;
}

void Self::TouchesMoved(UITouch const &touch, UIEvent const &event) {
  touch_point_test_->frame(UI::Rect{
    origin : {x : touch->touch_point_.x - 40, y : touch->touch_point_.y - 40},
    size : touch_point_test_->frame().size
  });
}

void Self::TouchesEnded(UITouch const &touch, UIEvent const &event) {
  touch_point_test_->is_hidden_ = true;
}

#undef LOG_TAG
#undef Self
