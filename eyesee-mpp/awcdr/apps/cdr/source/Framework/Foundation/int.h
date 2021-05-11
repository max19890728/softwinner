/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <Foundation/property.h>

#include <string>

struct Int : Protocol::Property<int> {
  // - MARK: 初始化器

  Int(int value) : value_(value) {}

 private:
  mutable int value_;

  // - MARK: 運算符重載

 public:
  auto operator=(const int& new_value) -> const int& override {
    return this->Protocol::Property<int>::operator=(new_value);
  }

  operator const int&() const {
    return this->Protocol::Property<int>::operator const int&();
  }

  /* * * * * 運算符重載 * * * * */

  inline auto operator+=(const int& that) const -> const Int& {
    this->value_ += that;
    return *this;
  }

  inline auto operator-=(const int& that) const -> const Int& {
    this->value_ -= that;
    return *this;
  }

  template <typename T>
  inline auto operator*=(const T& that) const -> const Int& {
    this->value_ *= that;
    return *this;
  }

  /* * * * * 繼承類別 * * * * */

  // - MARK: Propertyp

 public:
  auto get() const -> const int& override { return value_; }

  auto set(const int& new_value) -> const int& override {
    return value_ = new_value;
  }
};
