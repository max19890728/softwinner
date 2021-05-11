/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __SYS_POWER_H__
#define __SYS_POWER_H__

#ifdef __cplusplus
extern "C" {
#endif

void setPowerMode(int mode);
void SetAxp81xChg(int state);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__SYS_POWER_H__