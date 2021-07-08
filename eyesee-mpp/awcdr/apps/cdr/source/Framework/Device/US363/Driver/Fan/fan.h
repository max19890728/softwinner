/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __FAN_H__
#define __FAN_H__

#ifdef __cplusplus
extern "C" {
#endif

void SetFanAlwaysOn(int en);
int GetFanAlwaysOn();
void SetTask1mLockFlag(int flag);
int GetTask1mLockFlag();
int CheckTask1mLockTime(unsigned long long now_t);
void setFanSpeed(int speed);
int getFanSpeed();
void FanCtrlFunc(int fan_ctrl);
void setFanRotateSpeed(int speed);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__FAN_H__