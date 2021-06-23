/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <Device/observable.h>
#include <UIKit.h>

#include <memory>

class DebugViewController final : public UI::ViewController {
  // MARK: - 初始化器
 public:
  static inline std::shared_ptr<DebugViewController> init() {
    auto building = std::make_shared<DebugViewController>();
    building->Layout();
    return building;
  }

  DebugViewController();

  /* * * * * 其他成員 * * * * */
 public:
  UILabel time_label_;
  UILabel battery_label_;
  UILabel light_sensor_label_;
  UILabel inertial_measurement_unit_label_;
  UILabel ssid_label_;

 private:
  UIView touch_point_test_;

  void RealTimeClockChangeAction(Any);

  void BatteryChangeAction(Any);

  void LightSensorChangeAction(Any);

  void InertialMeasurementUnitChangeAction(Any);

  /* * * * * 繼承類別 * * * * */

  // MARK: UI::ViewController

 public:
  void Layout(UI::Coder = {}) override;

  void ViewWillDisappear() override;

  // MAKR: UI::Responder

 public:
  void TouchesBegan(UITouch const &, UIEvent const &) override;

  void TouchesMoved(UITouch const &, UIEvent const &) override;

  void TouchesEnded(UITouch const &, UIEvent const &) override;
};
