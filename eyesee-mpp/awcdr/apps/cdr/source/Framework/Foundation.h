/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <sys/types.h>

#include "Framework/Foundation/bool.h"
#include "Framework/Foundation/int.h"
#include "Framework/Foundation/property.h"
#include "Framework/Foundation/shared_from.h"
#include "Framework/Foundation/string.h"

#define Selector(function) std::bind(&function, this, std::placeholders::_1)

using Any = void*;

using UInt = Variable<uint>;
