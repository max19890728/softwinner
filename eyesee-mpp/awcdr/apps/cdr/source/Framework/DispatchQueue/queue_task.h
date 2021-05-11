/* *****************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 * ****************************************************************************/

#pragma once

#include <condition_variable>
#include <mutex>

namespace Dispatch {

class QueueTask {
 private:
  bool _signal_;
  std::mutex _mutex_;
  std::condition_variable _condition_;

 public:
  QueueTask() : _signal_(false) {}

  virtual ~QueueTask() {}

  virtual void run() = 0;

  virtual void signal() {
    _signal_ = true;
    _condition_.notify_all();
  }

  virtual void wait() {
    std::unique_lock<std::mutex> lock(_mutex_);
    _condition_.wait(lock, [this]() { return _signal_; });
    _signal_ = false;
  }

  virtual void reset() { _signal_ = false; }
};
}  // namespace Dispatch
