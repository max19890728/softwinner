/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include "DispatchQueue/QueueTask.h"

namespace Dispatch {

template <class T>
class ClosureTask : public QueueTask {
 private:
  T _closure_;

 public:
  explicit ClosureTask(const T& closure) : _closure_(closure) {}

 private:
  void run() override { _closure_(); }
};
}
