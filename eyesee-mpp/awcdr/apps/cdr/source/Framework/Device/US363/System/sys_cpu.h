/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __SYS_CPU_H__
#define __SYS_CPU_H__

#ifdef __cplusplus
extern "C" {
#endif

#define CPU_SPEED_FULL          1152000     /** CpuFullSpeed */
#define CPU_SPEED_AUTO_MOVE     600000      /** CpuAutoMoveSpeed, 4核 x 600Mhz */
#define CPU_SPEED_HIGH          600000      /** CpuAutoMoveSpeed, 2核 x 600Mhz */
#define CPU_SPEED_MIDDLE        480000      /** CpuAutoMoveSpeed, 4核 x 600Mhz */
#define CPU_SPEED_LOW           480000      /** CpuAutoMoveSpeed, 2核 x 600Mhz */

void setCpuFreq(int cpu_num, int freq);
void SetCpu(int cpu_num, int freq);
int getCPUTemprature();
int getCpuCoreNum();

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__SYS_CPU_H__