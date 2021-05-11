/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __CIS_TABLE_H__
#define __CIS_TABLE_H__

#include "Device/US363/Cmd/us360_define.h"

#ifdef __cplusplus
extern "C" {
#endif

extern CIS_CMD_struct FS_TABLE[CIS_TAB_N];
extern CIS_CMD_struct D2_TABLE[CIS_TAB_N];
extern CIS_CMD_struct D3_TABLE[CIS_TAB_N];

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__CIS_TABLE_H__