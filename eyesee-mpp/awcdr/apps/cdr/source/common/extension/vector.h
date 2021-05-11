/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <algorithm>
#include <functional>
#include <vector>

template <typename T>
void removeElementIn(std::vector<T> &vector, T element) {
  for (auto iter = vector.begin(); iter != vector.end();) {
    if (element == *iter) {
      iter = vector.erase(iter);
    } else {
      ++iter;
    }
  }
}

template <typename T>
void moveElementToBackIn(std::vector<T> &vector, T element) {
  for (auto iter = vector.begin(); iter != vector.end();) {
    if (element == *iter) {
      std::rotate(iter, iter + 1, vector.end());
      return;
    } else {
      ++iter;
    }
  }
}

#if 0
template <typename T>
void moveElementToFrontIn(std::vector<T> &vector, T need_move) {
  if ((auto pivot =
           std::find_if(vector.begin(), vector.end(), [&](const T &element) {
             return element == need_move;
           })) != vector.end()) {
    std::rotate(vector.begin(), pivot, pivot + 1);
  }
}
#endif

template <typename T>
auto dropFirst(/* in */ std::vector<T> &vector,
               std::function<bool(T const &object)> when) {
  typename std::vector<T>::iterator iter;
  for (iter = vector.begin(); iter != vector.end(); ++iter) {
    if (when(*iter)) break;
  }
  return iter == vector.end() ? nullptr : *iter;
}

/**
 * 當 `vector` 內包含指定元素時傳回 `ture`，否則傳回 `false`。
 **/
template <typename T>
bool contains(/* in */ std::vector<T> &vector, /* is equal to */ T element) {
  typename std::vector<T>::iterator iter;
  for (iter = vector.begin(); iter != vector.end(); ++iter) {
    if (*iter == element) return true;
  }
  if (iter == vector.end()) return false;
}
