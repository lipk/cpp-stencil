#pragma once

#include <array>
#include <cstdint>

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

template<u64 len, typename T>
std::array<T, len> repeat(const T& value)
{
    std::array<T, len> res;
    for (u64 i = 0; i < len; ++i) {
        res[i] = value;
    }
    return res;
}
}
