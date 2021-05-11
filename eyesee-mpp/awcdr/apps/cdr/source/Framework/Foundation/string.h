/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <Foundation/property.h>

#include <string>

struct String : Protocol::Property<std::string> {
  // - MARK: 初始化器

  String(std::string string = "")
      : string_(string), is_empty_(string_.empty()) {}

 private:
  std::string string_;

 public:
  bool is_empty_;

  // - MARK: 運算符重載

 public:
  auto operator=(const std::string& new_value) -> const std::string& override {
    return this->Protocol::Property<std::string>::operator=(new_value);
  }

  operator const std::string&() const {
    return this->Protocol::Property<std::string>::operator const std::string&();
  }

  operator const char*() const { return string_.c_str(); }

  /* * * * * 繼承類別 * * * * */

  // - MARK: Propertyp

 public:
  auto get() const -> const std::string& override { return string_; }

  auto set(const std::string& new_value) -> const std::string& override {
    is_empty_ = new_value.empty();
    return string_ = new_value;
  }
};
