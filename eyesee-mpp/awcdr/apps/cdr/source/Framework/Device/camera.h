/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <mpi_sys.h>
#include <mpi_venc.h>
#include <string.h>

namespace Device {

static int InitCamera() {
  MPP_SYS_CONF_S sys_config{};
  memset(&sys_config, 0, sizeof(sys_config));
  sys_config.nAlignWidth = 32;
  AW_MPI_SYS_SetConf(&sys_config);
  return AW_MPI_SYS_Init_S1();
}

class Camera {
 private:
  Camera() {}
};
}  // namespace Device
