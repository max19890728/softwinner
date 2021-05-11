/*****************************************************************************
 Copyright (C), 2015, AllwinnerTech. Co., Ltd.
 File name: check.h
 Author: yangy@allwinnertech.com
 Version: v1.0
 Date: 2015-11-24
 Description:

 History:
*****************************************************************************/

#pragma once

#define NULL_BREAK(pointer) \
  if (NULL == (pointer)) {  \
    break;                  \
  }

#define NULL_RETURN(pointer, value) \
  if (NULL == (pointer)) {          \
    return value;                   \
  }
