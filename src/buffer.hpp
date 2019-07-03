#pragma once

#include <array>
#include <loop.hpp>
#include <typelist/typelist.hpp>
#include <util.hpp>

namespace stencil {

template<u32 dim, typename T>
class buffer
    : not_copyable
    , not_movable /*TODO: make movable*/
{
    template<u32 rad>
    friend class accessor;

    const std::array<u64, dim> m_size;
    const u32 m_halo_size;
    const std::array<u64, dim> m_stride, m_offset_with_halo;
    T* const m_data;

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
        , m_halo_size(halo_size)
        , m_stride(_init_offset(size, halo_size))
        , m_offset_with_halo(repeat<u64, dim>(halo_size))
        , m_data(_init_data(size, halo_size))
    {}

    ~buffer() { delete m_data; }

    inline T& get(const std::array<u64, dim>& coords)
    {
        return this->m_data[_compute_index(coords, m_offset_with_halo)];
    }

    inline const T& get(const std::array<u64, dim>& coords) const
    {
        return this->m_data[_compute_index(coords, m_offset_with_halo)];
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

    template<u32 rad>
    class accessor
    {
        friend class buffer<dim, T>;
        const std::array<i64, ipow(2 * rad + 1, dim)> m_offset_table;
        T* m_middle;

        inline static constexpr u64 _compute_table_index_impl(
            const std::array<i64, dim>& coords,
            u32 i)
        {
            return i == coords.size()
                       ? 0
                       : (ipow(2 * rad + 1, i) * (coords[i] + rad) +
                          _compute_table_index_impl(coords, i + 1));
        }

        inline static constexpr u64 _compute_table_index(
            const std::array<i64, dim>& coords)
        {
            return _compute_table_index_impl(coords, 0);
        }

        inline static std::array<i64, ipow(2 * rad + 1, dim)>
        _init_offset_table(const std::array<u64, dim>& buffer_stride)
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

private:
    template<u32 rad, typename Func>
    inline void _iterate(const std::array<u64, dim>& from,
                         const std::array<u64, dim>& to,
                         const std::array<u64, dim>& offset,
                         const Func& func)
    {
        std::array<u64, dim> jumps;
        jumps[0] = m_stride[0];
        for (size_t i = 1; i < dim; ++i) {
            jumps[i] = m_stride[i - 1] *
                       (m_size[i] + 2 * m_halo_size - to[i] + from[i]);
        }
        accessor<rad> acc(m_stride);
        loop_with_counter<dim, u64, T*, u64>(
            from,
            to,
            m_data + _compute_index(from, offset),
            jumps,
            [&](std::array<u64, dim>& it, T* cnt) {
                acc.set_middle(cnt);
                func(it, acc);
            });
    }

public:
    template<u32 rad, typename Func>
    inline void iterate(const Func& func)
    {
        _iterate<rad, Func>(
            repeat<u64, dim>(0), m_size, m_offset_with_halo, func);
    }

    template<u32 rad, typename Func>
    inline void iterate_halo(const Func& func)
    {
        auto to = m_size;
        for (auto& i : to) {
            i += m_halo_size * 2;
        }
        _iterate<rad>(
            repeat<u64, dim>(0),
            to,
            repeat<u64, dim>(0),
            [&](std::array<u64, dim>& it, buffer<dim, T>::accessor<rad>& acc) {
                std::array<bool, dim> dir;
                bool skip = true;
                for (u32 i = 0; i < dim; ++i) {
                    if (it[i] < m_halo_size) {
                        dir[i] = false;
                        skip = false;
                    } else if (it[i] >= m_halo_size + m_size[i]) {
                        dir[i] = true;
                        skip = false;
                    }
                }
                if (skip) {
                    // TODO: skip whole rows
                    return;
                }
                func(it, acc, dir);
            });
    }

    inline void halo_fill(const T& value)
    {
        iterate_halo<0>([&](const std::array<u64, dim>&,
                            buffer<dim, T>::accessor<0>& acc,
                            const std::array<bool, dim>&) {
            acc.get(repeat<i64, dim>(0)) = value;
        });
    }
};
}
