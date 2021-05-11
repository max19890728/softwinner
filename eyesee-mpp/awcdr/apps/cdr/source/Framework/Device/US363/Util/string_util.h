/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __STRING_UTIL_H__
#define __STRING_UTIL_H__


#ifdef __cplusplus
extern "C" {
#endif

enum string_is_number_result {
	STRING_NOT_NUMBER = 0,
	STRING_IS_NUMBER
};

int stringIsNumber(char *str);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__STRING_UTIL_H__