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
            result += this->m_stride[i] * (coords[i] + offset[i]);
        }
        return result;
    }

    template<u32 rad, typename Func>
    inline void _iterate(const Func& func,
                         const std::array<u64, dim>& from,
                         const std::array<u64, dim>& to)
    {
        accessor<rad> acc(this->m_stride);
        std::array<u64, dim - 1> from_, to_;
        for (u32 i = 0; i < dim - 1; ++i) {
            from_[i] = from[i + 1];
            to_[i] = to[i + 1];
        }
        loop<dim - 1>(from_, to_, [&](const std::array<u64, dim - 1>& it) {
            std::array<u64, dim> curr;
            for (u64 i = 1; i < dim; ++i) {
                curr[i] = it[i - 1];
            }
            curr[0] = from[0];
            acc.set_middle(this->m_data + this->_compute_index(
                                              curr, this->m_offset_with_halo));
            for (; curr[0] < to[0]; ++curr[0], acc.step()) {
                auto x = this->m_data +
                         this->_compute_index(curr, this->m_offset_with_halo);
                func(curr, acc);
            }
        });
    }

public:
    buffer(const std::array<u64, dim>& size, u32 halo_size)
        : m_size(size)
        , m_halo_size(halo_size)
        , m_stride(_init_offset(size, halo_size))
        , m_offset_with_halo(repeat<u64, dim>(halo_size))
        , m_data(_init_data(size, halo_size))
    {}

    ~buffer() { delete this->m_data; }

    inline T& get(const std::array<u64, dim>& coords)
    {
        return this
            ->m_data[this->_compute_index(coords, this->m_offset_with_halo)];
    }

    inline const T& get(const std::array<u64, dim>& coords) const
    {
        return this
            ->m_data[this->_compute_index(coords, this->m_offset_with_halo)];
    }

    inline T& get_raw(const std::array<u64, dim>& coords)
    {
        return this->m_data[this->_compute_index(coords, repeat<u64, dim>(0))];
    }

    inline const T& get_raw(const std::array<u64, dim>& coords) const
    {
        return this->m_data[this->_compute_index(coords, repeat<dim, u64>(0))];
    }

    const std::array<u64, dim>& size() const { return this->m_size; }

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

        void step() { this->m_middle++; }

        void set_middle(T* middle) { this->m_middle = middle; }

    public:
        T& get(const std::array<i64, dim>& coords)
        {
            return *(this->m_middle +
                     this->m_offset_table[this->_compute_table_index(coords)]);
        }

        const T& get(const std::array<i64, dim>& coords) const
        {
            return *(this->m_middle +
                     this->m_offset_table[this->_compute_table_index(coords)]);
        }
    };

    template<u32 rad, typename Func>
    inline void iterate(const Func& func)
    {
        this->_iterate<rad, Func>(func, repeat<u64, dim>(0), this->m_size);
    }
};
}
