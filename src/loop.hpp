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
        for (i[m - 1] = from[m - 1]; i[m - 1] < to[m - 1]; ++i[m - 1]) {
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

template<u8 n, u8 m, typename T, typename C = u64, typename I = u64>
struct loop_with_counter_impl
{
    template<class F>
    static void run(const std::array<T, n>& from,
                    const std::array<T, n>& to,
                    C& counter,
                    const std::array<I, n>& increment,
                    const F& func,
                    std::array<T, n>& i)
    {
        for (i[m - 1] = from[m - 1]; i[m - 1] < to[m - 1];
             ++i[m - 1], counter += increment[m - 1]) {
            loop_with_counter_impl<n, m - 1, T, C, I>::run(
                from, to, counter, increment, func, i);
        }
    }
};

template<u8 n, typename T, typename C, typename I>
struct loop_with_counter_impl<n, 0, T, C, I>
{
    template<class F>
    static void run(const std::array<T, n>&,
                    const std::array<T, n>&,
                    C& counter,
                    const std::array<I, n>&,
                    const F& func,
                    std::array<T, n>& i)
    {
        func(i, counter);
    }
};

template<u8 n, typename T = u64, typename C = u64, typename I = u64>
struct loop_with_counter
{
    template<typename F>
    loop_with_counter(const std::array<T, n>& from,
                      const std::array<T, n>& to,
                      C counter,
                      const std::array<I, n>& increment,
                      const F& func)
    {
        std::array<T, n> i;
        loop_with_counter_impl<n, n, T, C, I>::run(
            from, to, counter, increment, func, i);
    }
};
}
