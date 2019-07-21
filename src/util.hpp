#pragma once

#include <array>
#include <cstdint>
#include <tuple>

namespace stencil {
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

struct not_copyable
{
    not_copyable() {}
    not_copyable(const not_copyable&) = delete;
    not_copyable& operator=(const not_copyable&) = delete;
};

struct not_movable
{
    not_movable() {}
    not_movable(const not_movable&) = delete;
    not_movable& operator=(const not_movable&) = delete;
};

template<typename T>
constexpr inline T ipow(T base, u64 exp)
{
    return exp == 0 ? 1 : base * ipow(base, exp - 1);
}

template<typename T, u64 len>
std::array<T, len> repeat(const T& value)
{
    std::array<T, len> res;
    for (u64 i = 0; i < len; ++i) {
        res[i] = value;
    }
    return res;
}

template<typename T, u64 len>
std::array<T, len> operator+(const std::array<T, len>& x,
                             const std::array<T, len>& y)
{
    std::array<T, len> z;
    for (u32 i = 0; i < len; ++i) {
        z[i] = x[i] + y[i];
    }
    return z;
}

template<typename... T>
struct tuple_counter
{
    std::tuple<T...> values;
};

template<u32 index, typename T, typename... TS>
struct _increment_tuple_counter_impl;

template<typename T, typename... TS>
struct _increment_tuple_counter_impl<0, T, TS...>
{
    static inline void increment(tuple_counter<TS...>&, T){};
};

template<u32 index, typename T, typename... TS>
struct _increment_tuple_counter_impl
{
    static inline void increment(tuple_counter<TS...>& cnt, T inc)
    {
        std::get<index - 1>(cnt.values) += inc;
        _increment_tuple_counter_impl<index - 1, T, TS...>::increment(cnt, inc);
    }
};

template<typename T, typename... TS>
tuple_counter<TS...>& operator+=(tuple_counter<TS...>& cnt, T inc)
{
    _increment_tuple_counter_impl<std::tuple_size<decltype(cnt.values)>::value,
                                  T,
                                  TS...>::increment(cnt, inc);
    return cnt;
}
}
