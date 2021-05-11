/* *******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 * *******************************************************************************/

#pragma once

#define IIO_DEVICES_PATH "/sys/bus/iio/devices/iio\:device"
#define POWER_SUPPLY_PATH "/sys/class/power_supply/"

#undef LOG_TAG
#define LOG_TAG "SCNSOR_READER"

#define SCNSOR_PATH_TABLE                                   \
  X(LIGHT_SCNSOR, IIO_DEVICES_PATH "7/in_illuminance_raw")  \
  X(ACCELEROMETER_X, IIO_DEVICES_PATH "0/in_accel_x_raw")   \
  X(ACCELEROMETER_Y, IIO_DEVICES_PATH "0/in_accel_y_raw")   \
  X(ACCELEROMETER_Z, IIO_DEVICES_PATH "0/in_accel_z_raw")   \
  X(GYROSCOPE_X, IIO_DEVICES_PATH "1/in_anglvel_x_raw")     \
  X(GYROSCOPE_Y, IIO_DEVICES_PATH "1/in_anglvel_y_raw")     \
  X(GYROSCOPE_Z, IIO_DEVICES_PATH "1/in_anglvel_z_raw")     \
  X(MAGNETIC_X, IIO_DEVICES_PATH "6/in_magn_x_raw")         \
  X(MAGNETIC_Y, IIO_DEVICES_PATH "6/in_magn_y_raw")         \
  X(MAGNETIC_Z, IIO_DEVICES_PATH "6/in_magn_z_raw")         \
  X(BATTERY_CAPACITY, POWER_SUPPLY_PATH "battery/capacity") \
  X(BATTERY_VOLTAGE, POWER_SUPPLY_PATH "battery/voltage_now")

#define X(a, b) a,
enum READABLE_SCNSOR { SCNSOR_PATH_TABLE };
#undef X

#define X(a, b) b,
inline static const char *sensor_path(READABLE_SCNSOR scnsor) {
  static char *table[] = {SCNSOR_PATH_TABLE};
  return table[scnsor];
}
#undef X

int ReadSensor(READABLE_SCNSOR scnsor);
