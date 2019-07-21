#pragma once

#include <array>
#include <loop.hpp>
#include <type_traits>
#include <typelist/typelist.hpp>
#include <util.hpp>

namespace stencil {
template<u32 dim, typename T>
class buffer;

template<u32 rad, typename Func, u32 dim, typename T>
void _iterate_impl(buffer<dim, T>& buf,
                   const std::array<u64, dim>& from,
                   const std::array<u64, dim>& to,
                   T* cnt_init,
                   const Func& func);

template<u32 rad, typename Func, u32 dim, typename T>
void iterate_halo(buffer<dim, T>& buf, const Func& func);

template<u32 rad, u32 dim, typename T>
class accessor;

template<u32 dim, typename T>
class buffer
    : not_copyable
    , not_movable /*TODO: make movable*/
{
    template<u32, u32, typename>
    friend class accessor;

    const std::array<u64, dim> m_size, m_raw_size;
    const u32 m_halo_size;
    const std::array<u64, dim> m_stride, m_offset_with_halo;
    T* const m_data;

    static std::array<u64, dim> _init_raw_size(const std::array<u64, dim>& size,
                                               u32 halo_size)
    {
        std::array<u64, dim> result;
        for (u32 i = 0; i < dim; ++i) {
            result[i] = size[i] + 2 * halo_size;
        }
        return result;
    }

    static std::array<u64, dim> _init_offset(const std::array<u64, dim>& size,
                                             u32 halo_size)
    {
        std::array<u64, dim> result;
        result[0] = 1;
        for (u32 i = 1; i < dim; ++i) {
            result[i] = result[i - 1] * (size[i - 1] + 2 * halo_size);
        }
        return result;
    }

    static T* _init_data(const std::array<u64, dim>& size, u32 halo_size)
    {
        u64 buffer_length = 1;
        for (u32 i = 0; i < dim; ++i) {
            buffer_length *= (size[i] + 2 * halo_size);
        }
        return new T[buffer_length];
    }

    inline u64 _compute_index(const std::array<u64, dim>& coords,
                              const std::array<u64, dim>& offset) const
    {
        u64 result = coords[0] + offset[0];
        for (u32 i = 1; i < dim; ++i) {
            result += m_stride[i] * (coords[i] + offset[i]);
        }
        return result;
    }

public:
    buffer(const std::array<u64, dim>& size, u32 halo_size)
        : m_size(size)
        , m_raw_size(_init_raw_size(size, halo_size))
        , m_halo_size(halo_size)
        , m_stride(_init_offset(size, halo_size))
        , m_offset_with_halo(repeat<u64, dim>(halo_size))
        , m_data(_init_data(size, halo_size))
    {}

    ~buffer() { delete[] m_data; }

    inline T& get(const std::array<u64, dim>& coords)
    {
        return m_data[_compute_index(coords, m_offset_with_halo)];
    }

    inline T& get(const std::array<u64, dim>& coords) const
    {
        return m_data[_compute_index(coords, m_offset_with_halo)];
    }

    inline T& get_raw(const std::array<u64, dim>& coords)
    {
        return m_data[_compute_index(coords, repeat<u64, dim>(0))];
    }

    inline const T& get_raw(const std::array<u64, dim>& coords) const
    {
        return m_data[_compute_index(coords, repeat<dim, u64>(0))];
    }

    const std::array<u64, dim>& size() const { return m_size; }
    const std::array<u64, dim>& size_with_halo() const { return m_raw_size; }
    const std::array<u64, dim>& stride() const { return m_stride; }
    u32 halo_size() const { return m_halo_size; }

    void fill_halo(const T& value)
    {
        iterate_halo<0>(*this,
                        [&](const std::array<u64, dim>&,
                            accessor<0, dim, T>& acc,
                            const std::array<bool, dim>&) {
                            acc.get(repeat<i64, dim>(0)) = value;
                        });
    }

    void fill(const T& value)
    {
        _iterate_impl<0>(*this,
                         repeat<u64, dim>(0),
                         m_raw_size,
                         &get_raw(repeat<u64, dim>(0)),
                         [&](std::array<u64, dim>&, accessor<0, dim, T>& acc) {
                             acc.get({ 0, 0 }) = value;
                         });
    }

    void copy_halo_from(const buffer<dim, T>& other,
                        const std::array<i32, dim>& relpos)
    {
        // TODO: efficient implementation with counter loop
        std::array<u64, dim> to, from_src, from_dst;
        for (u32 i = 0; i < dim; ++i) {
            if (relpos[i] < 0) {
                to[i] = other.m_halo_size;
                from_src[i] = other.m_size[i] - other.m_halo_size;
                from_dst[i] = 0;
            } else if (relpos[i] == 0) {
                to[i] = other.m_size[i];
                from_src[i] = 0;
                from_dst[i] = m_halo_size;
            } else {
                to[i] = other.m_halo_size;
                from_src[i] = 0;
                from_dst[i] = m_raw_size[i] - m_halo_size;
            }
        }
        loop<dim>(repeat<u64, dim>(0), to, [&](const std::array<u64, dim>& it) {
            get_raw(from_dst + it) = other.get(from_src + it);
        });
    }
};

template<u32 rad, u32 dim, typename T>
class accessor
{
    friend class buffer<dim, T>;
    const std::array<i64, ipow(2 * rad + 1, dim)> m_offset_table;
    T* m_middle;

    inline static constexpr u64 _compute_table_index_impl(
        const std::array<i64, dim>& coords,
        u32 i)
    {
        return i == coords.size() ? 0
                                  : (ipow(2 * rad + 1, i) * (coords[i] + rad) +
                                     _compute_table_index_impl(coords, i + 1));
    }

    inline static constexpr u64 _compute_table_index(
        const std::array<i64, dim>& coords)
    {
        return _compute_table_index_impl(coords, 0);
    }

    inline static std::array<i64, ipow(2 * rad + 1, dim)> _init_offset_table(
        const std::array<u64, dim>& buffer_stride)
    {
        std::array<i64, dim> from;
        from.fill(-static_cast<i64>(rad));
        std::array<i64, dim> to;
        to.fill(rad + 1);
        std::array<i64, ipow(2 * rad + 1, dim)> table;
        loop<dim, i64>(from, to, [&](const std::array<i64, dim>& it) {
            u64 index = _compute_table_index(it);
            table[index] = 0;
            for (u32 i = 0; i < dim; ++i) {
                table[index] += it[i] * buffer_stride[i];
            }
        });
        return table;
    }

public:
    accessor(const std::array<u64, dim>& buffer_stride)
        : m_offset_table(_init_offset_table(buffer_stride))
    {}

    inline void step(u64 n) { m_middle += n; }

    inline void set_middle(T* middle) { m_middle = middle; }

public:
    inline T& get(const std::array<i64, dim>& coords)
    {
        return *(m_middle + m_offset_table[_compute_table_index(coords)]);
    }

    inline const T& get(const std::array<i64, dim>& coords) const
    {
        return *(m_middle + m_offset_table[_compute_table_index(coords)]);
    }
};

template<u32 rad, typename Func, u32 dim, typename T>
void _iterate_impl(buffer<dim, T>& buf,
                   const std::array<u64, dim>& from,
                   const std::array<u64, dim>& to,
                   T* cnt_init,
                   const Func& func)
{
    std::array<u64, dim> jumps;
    const auto stride = buf.stride();
    const auto raw_size = buf.size_with_halo();
    jumps[0] = stride[0];
    for (size_t i = 1; i < dim; ++i) {
        jumps[i] = stride[i - 1] * (raw_size[i] - to[i] + from[i]);
    }
    accessor<rad, dim, T> acc(stride);
    loop_with_counter<dim, u64, T*, u64>(
        from, to, cnt_init, jumps, [&](std::array<u64, dim>& it, T* cnt) {
            acc.set_middle(cnt);
            func(it, acc);
        });
}

template<u32 rad, typename Func, u32 dim, typename... T>
void _iterate_multi_impl(std::tuple<buffer<dim, T>...>& buf,
                         const std::array<u64, dim>& from,
                         const std::array<u64, dim>& to,
                         std::tuple<T*...> cnt_init,
                         const Func& func)
{
    std::array<u64, dim> jumps;
    const auto stride = buf.stride();
    const auto raw_size = buf.size_with_halo();
    jumps[0] = stride[0];
    for (size_t i = 1; i < dim; ++i) {
        jumps[i] = stride[i - 1] * (raw_size[i] - to[i] + from[i]);
    }
    auto acc = std::make_tuple(accessor<rad, dim, T>(stride)...);
    loop_with_counter<dim, u64, std::tuple<T*...>, u64>(
        from,
        to,
        cnt_init,
        jumps,
        [&](std::array<u64, dim>& it, const std::tuple<T*...>& cnt) {
            acc.set_middle(cnt);
            func(it, acc);
        });
}

template<u32 rad, typename Func, u32 dim, typename T>
void iterate(buffer<dim, T>& buf, const Func& func)
{
    _iterate_impl<rad, Func, dim, T>(buf,
                                     repeat<u64, dim>(0),
                                     buf.size(),
                                     &buf.get(repeat<u64, dim>(0)),
                                     func);
}

template<u32 rad, typename Func, u32 dim, typename T>
void iterate_halo(buffer<dim, T>& buf, const Func& func)
{
    auto wrapper = [&](std::array<u64, dim>& it, accessor<rad, dim, T>& acc) {
        std::array<bool, dim> dir;
        bool skip = true;
        for (u32 i = 0; i < dim; ++i) {
            if (it[i] < buf.halo_size()) {
                dir[i] = false;
                skip = false;
            } else if (it[i] >= buf.halo_size() + buf.size()[i]) {
                dir[i] = true;
                skip = false;
            }
        }
        if (skip) {
            // TODO: skip whole rows
            return;
        }
        func(it, acc, dir);
    };
    _iterate_impl<rad, decltype(wrapper), dim, T>(
        buf,
        repeat<u64, dim>(0),
        buf.size_with_halo(),
        &buf.get_raw(repeat<u64, dim>(0)),
        wrapper);
}
}
