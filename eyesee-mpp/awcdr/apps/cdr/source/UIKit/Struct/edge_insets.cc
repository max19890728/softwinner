/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "UIKit/Struct/edge_insets.h"

UI::EdgeInsets UI::EdgeInsets::zero = UI::EdgeInsets(0, 0, 0, 0);

UI::EdgeInsets::EdgeInsets(int top, int bottom, int left, int right)
    : top(top), bottom(bottom), left(left), right(right) {}
