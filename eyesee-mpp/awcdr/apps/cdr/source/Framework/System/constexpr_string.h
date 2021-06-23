/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <linux/types.h>

namespace Constexpr {
template <unsigned...>
struct seq {
  using type = seq;
};

template <unsigned N, unsigned... Is>
struct gen_seq_x : gen_seq_x<N - 1, N - 1, Is...> {};

template <unsigned... Is>
struct gen_seq_x<0, Is...> : seq<Is...> {};

template <unsigned N>
using gen_seq = typename gen_seq_x<N>::type;

template <size_t Size>
using size = std::integral_constant<size_t, Size>;

template <class T, size_t N>
constexpr size<N> length(T const (&)[N]) {
  return {};
}

template <class T, size_t N>
constexpr size<N> length(std::array<T, N> const&) {
  return {};
}

template <class T>
using length_t = decltype(length(std::declval<T>()));

constexpr size_t string_size() { return 0; }

template <class... Ts>
constexpr size_t string_size(size_t i, Ts... ts) {
  return (i ? i - 1 : 0) + string_size(ts...);
}

template <class... Ts>
using LengthOf = size<string_size(length_t<Ts>{}...)>;

template <class... Ts>
using String = std::array<char, LengthOf<Ts...>{} + 1>;

template <class Lhs, class Rhs, unsigned... I1, unsigned... I2>
constexpr const String<Lhs, Rhs> concat_impl(Lhs const& lhs, Rhs const& rhs,
                                             seq<I1...>, seq<I2...>) {
  return {{lhs[I1]..., rhs[I2]..., '\0'}};
}

template <class Lhs, class Rhs>
constexpr const String<Lhs, Rhs> Merge(Lhs const& lhs, Rhs const& rhs) {
  return concat_impl(lhs, rhs, gen_seq<LengthOf<Lhs>{}>{},
                     gen_seq<LengthOf<Rhs>{}>{});
}

template <class T0, class T1, class... Ts>
constexpr const String<T0, T1, Ts...> Merge(T0 const& t0, T1 const& t1,
                                            Ts const&... ts) {
  return Merge(t0, Merge(t1, ts...));
}

template <class T>
constexpr const String<T> Merge(T const& t) {
  return Merge(t, "");
}

constexpr const String<> Merge() { return Merge(""); }
}  // namespace Constexpr
