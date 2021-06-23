/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

namespace Constexpr {

template <int First, int Second>
struct Pair {
  enum { key = First, value = Second };
};

// 使用 `typedef Map<Pair<value_1, value_2>> map_;` 來建立 Map
template <typename... Pair>
struct Map {};

template <typename T>
struct Identity {
  typedef T type;
};

// 使用 `FindValueBy<Key, Map>::value` 根據 Key 取得 Map 內對應的數值
template <int Key, typename Map>
struct FindValueBy;

template <int I, typename Pair, typename... Other>
struct FindValueBy<I, Constexpr::Map<Pair, Other...>> {
  typedef
      typename std::conditional<I == Pair::key, Identity<Pair>,
                                FindValueBy<I, Constexpr::Map<Other...>>>::type
          meta;
  typedef typename meta::type type;
  enum { value = type::value };
};

template <int I, typename Map>
struct FindValueBy<I, Constexpr::Map<Map>> {
  typedef
      typename std::conditional<I == Map::key, Identity<Map>, void>::type meta;
  typedef typename meta::type type;
  enum { value = type::range };
};

// 使用 `FindKeyBy<Value, Map>::key` 根據 Value 取得 Map 內對應的數值
template <int Value, typename Map>
struct FindKeyBy;

// Linear search by range
template <int Value, typename Pair>
struct FindKeyBy<Value, Constexpr::Map<Pair>> {
  typedef typename std::conditional<Value == Pair::range, Identity<Pair>,
                                    void>::type meta;
  typedef typename meta::type type;
  enum { value = type::key };
};

template <int I, typename Pair, typename... Other>
struct FindKeyBy<I, Constexpr::Map<Pair, Other...>> {
  typedef
      typename std::conditional<I == Pair::range, Identity<Pair>,
                                FindKeyBy<I, Constexpr::Map<Other...>>>::type
          meta;
  typedef typename meta::type type;
  enum { value = type::key };
};
}  // namespace Constexpr

#ifdef Example

class Example {
  typedef Constexpr::Map<Constexpr::Pair<1, 11>, Constexpr::Pair<2, 12>> Maps;

  static void Test() {
    std::cout << Constexpr::FindValueBy<1, Maps>::value << std::endl;
  };
}

#endif
