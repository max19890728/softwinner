/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __GPIO_H__
#define __GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

#define IOCTL_GPIO_INPUT 3
#define IOCTL_GPIO_HIGH  1
#define IOCTL_GPIO_LOW   0

int GPIO_Open();
void GPIO_Close();
int GPIO_IOCTL(int type, int index);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__GPIO_H__
