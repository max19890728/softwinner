/* *****************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 * ****************************************************************************/

#pragma once

#include <memory>
#include <type_traits>

#include "DispatchQueue/ClosureTask.h"
#include "DispatchQueue/QueueTask.h"

namespace Dispatch {

class DispatchQueue {
 public:
  explicit DispatchQueue(int thread_count) {}

  virtual ~DispatchQueue() {}

  template <class T, typename std::enable_if<
                         std::is_copy_constructible<T>::value>::type* = nullptr>
  void sync(const T& task) {
    sync(std::shared_ptr<QueueTask>(new ClosureTask<T>(task)));
  }

  void sync(std::shared_ptr<QueueTask> task) {
    if (task != nullptr) sync_imp(task);
  }

  template <class T, typename std::enable_if<
                         std::is_copy_constructible<T>::value>::type* = nullptr>
  int64_t async(const T& task) {
    return async(std::shared_ptr<QueueTask>(new ClosureTask<T>(task)));
  }

  int64_t async(std::shared_ptr<QueueTask> task) {
    if (task != nullptr) return async_imp(task);
    return -1;
  }

 protected:
  virtual void sync_imp(std::shared_ptr<QueueTask> task) = 0;

  virtual int64_t async_imp(std::shared_ptr<QueueTask> task) = 0;
};

std::shared_ptr<DispatchQueue> creat(int thread_count = 1);
}  // namespace Dispatch
