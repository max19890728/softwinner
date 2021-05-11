/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __SYS_CPU_H__
#define __SYS_CPU_H__

#ifdef __cplusplus
extern "C" {
#endif

void SetCpu(int cpu_num, int freq);
int getCPUTemprature();
int getCpuCoreNum();

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__SYS_CPU_H__