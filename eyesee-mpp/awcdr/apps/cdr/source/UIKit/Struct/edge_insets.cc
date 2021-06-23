/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#include "UIKit/Struct/edge_insets.h"

UI::EdgeInsets UI::EdgeInsets::zero = UI::EdgeInsets(0, 0, 0, 0);

UI::EdgeInsets::EdgeInsets(int top, int bottom, int left, int right)
    : top(top), bottom(bottom), left(left), right(right) {
  this->top.didSet = [this] {
    if (this->didSet) this->didSet();
  };
  this->bottom.didSet = [this] {
    if (this->didSet) this->didSet();
  };
  this->left.didSet = [this] {
    if (this->didSet) this->didSet();
  };
  this->right.didSet = [this] {
    if (this->didSet) this->didSet();
  };
}
