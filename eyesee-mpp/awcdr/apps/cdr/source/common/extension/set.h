/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <functional>
#include <set>

template <typename T>
void removeAll(std::set<T> &set, std::function<bool(T const &object)> when) {
  for (auto iter = set.begin(); iter != set.end();) {
    if (when(*iter)) {
      iter = set.erase(iter);
    } else {
      ++iter;
    }
  }
}

template <typename T>
auto dropFirst(std::set<T> &set, std::function<bool(T const &object)> when) {
  typename std::set<T>::iterator iter;
  for (iter = set.begin(); iter != set.end(); ++iter) {
    if (when(*iter)) break;
  }
  return iter;
}
