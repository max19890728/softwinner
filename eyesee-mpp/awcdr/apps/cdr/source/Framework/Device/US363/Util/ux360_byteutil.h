/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __UX360_BYTEUTIL_H__
#define __UX360_BYTEUTIL_H__


#ifdef __cplusplus
extern "C" {
#endif

void int2Byte(int intValue, char *buf);
int byte2Int(char *buf, int size);
long byte2Long(char *buf, int size);
double byte2Double(char *buf, int size);
void swap32(unsigned int *val);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__UX360_BYTEUTIL_H__