# Property

包含
1. `Protocol::Property<T>` 用於使繼承的子類擁有屬性觀察器的功能
2. `Variable<T>` 用於使成員包含屬性觀察器的功能

## Property

建構一個子類包含屬性觀察器功能

```c++
class TestProperty : public Protocol::Property<TestProperty> {

  // 覆寫 `virtual auto get() cosnt -> const T& = 0;` 來提供預設的讀取行為
  auto get() const -> const TestProperty& override {
    return *this;
  }

  // 覆寫 `vortual auto set(const T&) -> const T& = 0;` 來提供預設的寫入行為
  auto set(const TestProperty& new_value) -> const TestProperty& override {
    // do data set...
    return *this;
  }
}
```

## Variable

用於包含已經定義好的數據結構

如
```c++
Variable<int> test_variable_;

test_variable_ = 0;
```
