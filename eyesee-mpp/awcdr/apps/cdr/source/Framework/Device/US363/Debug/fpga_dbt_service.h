#pragma once

#include "Device/US363/Debug/fpga_dbt.h"

void fpgaDbtReadWriteDdrService(fpga_ddr_rw_struct* ddr_p);
void fpgaDbtReadWriteRegService(fpga_reg_rw_struct* reg_p);