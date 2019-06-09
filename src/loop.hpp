#pragma once

#include <array>
#include <util.hpp>

namespace stencil {
template<u8 n, u8 m, typename T>
struct loop_impl
{
    template<class F>
    static void run(const std::array<T, n>& from,
                    const std::array<T, n>& to,
                    const F& func,
                    std::array<T, n>& i)
    {
        for (i[n - m] = from[n - m]; i[n - m] < to[n - m]; ++i[n - m]) {
            loop_impl<n, m - 1, T>::run(from, to, func, i);
        }
    }
};

template<u8 n, typename T>
struct loop_impl<n, 0, T>
{
    template<class F>
    static void run(const std::array<T, n>&,
                    const std::array<T, n>&,
                    const F& func,
                    std::array<T, n>& i)
    {
        func(i);
    }
};

template<u8 n, typename T = u64>
struct loop
{
    template<typename F>
    loop(const std::array<T, n>& from,
         const std::array<T, n>& to,
         const F& func)
    {
        std::array<T, n> i;
        loop_impl<n, n, T>::run(from, to, func, i);
    }
};
}
