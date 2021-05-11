/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <functional>

namespace Protocol {

template <typename T>
class Property {
 public:
  /**
   * 當使用取得數值 `=` 時會先檢查是否有設定 `getter` 有則執行 `getter`
   * 否則執行預設的 `get()` 取得數值
   **/
  operator const T&() const {
    if (getter) return getter();
    return get();
  }

  /**
   * 當 `willSet` 有被設定時在執行 `set` 前會先呼叫 `willSet`
   * 並使用它的回傳值進行
   * `set` 當 `didSet`有被設定時，在 `set` 執行結束時會呼叫 `didSet`
   **/
  virtual auto operator=(const T& new_value) -> const T& {
    auto& result = willSet ? set(willSet(new_value)) : set(new_value);
    if (didSet) didSet();
    return result;
  }

  /**
   * 判斷新設定的數值是否相等，相等則可以不觸發設定機制，可覆寫來決定相等邏輯
   **/
  virtual auto operator==(const T& that) const -> bool {
    return static_cast<const T&>(*this) == that;
  }

  virtual auto operator!=(const T& that) const -> bool {
    return static_cast<const T&>(*this) != that;
  }

  // 可以使變數變成計算型變數
  std::function<const T&()> getter;

  // 當數值被設定前呼叫
  std::function<const T&(const T& /* new_value */)> willSet;

  // 當數值被設定完成後呼叫
  std::function<void()> didSet;

 private:
  /**
   * 子類必須定義數值被取得時的預設行為
   **/
  virtual auto get() const -> const T& = 0;

  /**
   * 子類必須定義數值被設定時的預設行為
   **/
  virtual auto set(const T&) -> const T& = 0;
};
}  // namespace Protocol

template <typename T>
class Variable : public Protocol::Property<T> {
 public:
  T t;

  explicit Variable(T t) : t(t) {}

  operator const T&() const {
    return this->Protocol::Property<T>::operator const T&();
  }

  auto operator=(const T& new_value) -> const T& override {
    return this->Protocol::Property<T>::operator=(new_value);
  }

  auto operator==(const T& that) const -> bool override {
    return static_cast<const T&>(*this) == that;
  }

 private:
  auto get() const -> const T& override { return t; }

  auto set(const T& new_value) -> const T& override { return t = new_value; }
};

#if 1
template <typename T>
class Property {
 public:
  Property() {}
  operator const T&() const {
    // Call override getter if we have it
    if (getter) return getter();
    return get();
  }
  const T& operator=(const T& other) {
    // Call override setter if we have it
    if (setter) return setter(other);
    return set(other);
  }
  bool operator==(const T& other) const {
    // Static cast makes sure our getter operator is called, so we could use
    // overrides if those are in place
    return static_cast<const T&>(*this) == other;
  }
  // Use this to always get without overrides, useful for use with overriding
  // implementations
  const T& get() const { return t; }
  // Use this to always set without overrides, useful for use with overriding
  // implementations
  const T& set(const T& other) { return t = other; }
  // Assign getter and setter to these properties
  std::function<const T&()> getter;
  std::function<const T&(const T&)> setter;

 private:
  T t;
};
#endif

#ifdef Example

// Basic usage, no override
struct Test {
  Property<int> prop;
};

// Override getter and setter
struct TestWithOverride {
  TestWithOverride() {
    prop.setter = [&](const int& other) {
      std::cout << "Custom setter called" << std::endl;
      return prop.set(other);
    };
    prop.setter =
        std::bind(&TestWithOverride::setProp, this, std::placeholders::_1);
    prop.getter = std::bind(&TestWithOverride::getProp, this);
  }
  Property<int> prop;

 private:
  const int& getProp() const {
    std::cout << "Custom getter called" << std::endl;
    return prop.get();
  }
  const int& setProp(const int& other) {
    std::cout << "Custom setter called" << std::endl;
    return prop.set(other);
  }
};

#endif
