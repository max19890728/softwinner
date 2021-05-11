/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 *
 * file: us363_touch_lib.h
 ******************************************************************************/

#pragma once

#include <common.h>
#include <ial.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BOOL InitUS363TouchLibInput(INPUT* input, const char* mdev, const char* mtype);

void TermUS363TouchLibInput(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
