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

template<u8 n, u8 m, typename T, typename U>
struct loop_with_counter_impl
{
    template<class F>
    static void run(const std::array<T, n>& from,
                    const std::array<T, n>& to,
                    U& counter,
                    const std::array<U, n>& increment,
                    const F& func,
                    std::array<T, n>& i)
    {
        for (i[n - m] = from[n - m]; i[n - m] < to[n - m];
             ++i[n - m], counter += increment[n - m]) {
            loop_with_counter_impl<n, m - 1, T, U>::run(
                from, to, counter, increment, func, i);
        }
    }
};

template<u8 n, typename T, typename U>
struct loop_with_counter_impl<n, 0, T, U>
{
    template<class F>
    static void run(const std::array<T, n>&,
                    const std::array<T, n>&,
                    U& counter,
                    const std::array<U, n>&,
                    const F& func,
                    std::array<T, n>& i)
    {
        func(i, counter);
    }
};

template<u8 n, typename T = u64, typename U = u64>
struct loop_with_counter
{
    template<typename F>
    loop_with_counter(const std::array<T, n>& from,
                      const std::array<T, n>& to,
                      U counter,
                      const std::array<U, n>& increment,
                      const F& func)
    {
        std::array<T, n> i;
        loop_with_counter_impl<n, n, T, U>::run(
            from, to, counter, increment, func, i);
    }
};
}
