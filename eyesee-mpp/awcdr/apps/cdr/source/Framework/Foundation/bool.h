/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include "Foundation/property.h"

struct Bool : Protocol::Property<bool> {
  // - MARK: 初始化器

  Bool(bool value) : value_(value) {}

 private:
  mutable bool value_;

 public:
  inline auto Toggle() -> const bool& {
    return this->Protocol::Property<bool>::operator=(!value_);
  }

  /* * * * * 運算符重載 * * * * */

 public:
  inline auto operator!() -> const bool { return !value_; }

  inline auto operator=(const Bool& that) -> const Bool& {
    bool new_value = that.getter ? that.getter() : that.get();
    return this->Protocol::Property<bool>::operator=(new_value);
  }

  auto operator=(const bool& new_value) -> const bool& override {
    return this->Protocol::Property<bool>::operator=(new_value);
  }

  operator const bool&() const {
    return this->Protocol::Property<bool>::operator const bool&();
  }

  explicit operator bool() const {
    if (this->getter) return this->getter();
    return get();
  }

  /* * * * * 繼承類別 * * * * */

  // - MARK: Propertyp

 public:
  auto get() const -> const bool& override { return value_; }

  auto set(const bool& new_value) -> const bool& override {
    return value_ = new_value;
  }
};
