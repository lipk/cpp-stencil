#pragma once

#include <array>
#include <loop.hpp>
#include <type_traits>
#include <typelist/typelist.hpp>
#include <util.hpp>

#include <iostream>

namespace stencil {
template<u32 dim, typename T>
class grid;

template<u32 rad, typename Func, u32 dim, typename... T>
void _iterate_impl(std::tuple<grid<dim, T>&...>& buf,
                   const std::array<u64, dim>& from,
                   const std::array<u64, dim>& to,
                   std::tuple<T*...> cnt_init,
                   const Func& func);

template<u32 rad, typename Func, u32 dim, typename T>
void iterate_halo(grid<dim, T>& buf, const Func& func);

template<u32 rad, u32 dim, typename... T>
class accessor;

template<u32 dim, typename T>
class buffer : not_copyable
{
    const std::array<u64, dim> m_size;
    const std::array<u64, dim> m_stride;
    T* m_data;

    static T* _init_data(const std::array<u64, dim>& size)
    {
        u64 buffer_length = 1;
        for (u32 i = 0; i < dim; ++i) {
            buffer_length *= size[i];
        }
        return new T[buffer_length];
    }

    static std::array<u64, dim> _init_stride(const std::array<u64, dim>& size)
    {
        std::array<u64, dim> result;
        result[0] = 1;
        for (u32 i = 1; i < dim; ++i) {
            result[i] = result[i - 1] * (size[i - 1]);
        }
        return result;
    }

public:
    buffer(const std::array<u64, dim>& size)
        : m_size(size)
        , m_stride(_init_stride(size))
        , m_data(_init_data(size))
    {}

    buffer(buffer<dim, T>&& other)
        : m_size(other.m_size)
        , m_stride(other.m_stride)
        , m_data(other.m_data)
    {
        other.m_data = nullptr;
    }

    ~buffer() { delete m_data; }

    inline const std::array<u64, dim>& stride() const { return m_stride; }

    inline const T& get(u64 index) const { return m_data[index]; }

    inline T& get(u64 index) { return m_data[index]; }
};

template<u32 dim, typename T>
class grid : not_copyable
{
    template<u32, u32, typename...>
    friend class accessor;

    const std::array<u64, dim> m_size, m_raw_size;
    const u32 m_halo_size;
    buffer<dim, T>* m_buffer;
    const u64 m_start_offset, m_raw_start_offset;

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

    inline u64 _compute_index(const std::array<u64, dim>& coords) const
    {
        u64 result = coords[0];
        for (u32 i = 1; i < dim; ++i) {
            result += m_buffer->stride()[i] * coords[i];
        }
        return result;
    }

public:
    grid(const std::array<u64, dim>& size,
         u32 halo_size,
         const std::array<u64, dim>& position,
         buffer<dim, T>* buffer)
        : m_size(size)
        , m_raw_size(_init_raw_size(size, halo_size))
        , m_halo_size(halo_size)
        , m_buffer(buffer)
        , m_start_offset(_compute_index(position))
        , m_raw_start_offset(
              _compute_index(position - repeat<u64, dim>(halo_size)))
    {}

    grid(grid<dim, T>&& other)
        : m_size(other.m_size)
        , m_raw_size(other.m_raw_size)
        , m_halo_size(other.m_halo_size)
        , m_buffer(other.m_buffer)
        , m_start_offset(other.m_start_offset)
        , m_raw_start_offset(other.m_raw_start_offset)
    {
        other.m_buffer = nullptr;
    }

    ~grid() {}

    inline T& get(const std::array<u64, dim>& coords)
    {
        return m_buffer->get(m_start_offset + _compute_index(coords));
    }

    inline const T& get(const std::array<u64, dim>& coords) const
    {
        return m_buffer->get(m_start_offset + _compute_index(coords));
    }

    inline T& get_raw(const std::array<u64, dim>& coords)
    {
        return m_buffer->get(m_raw_start_offset + _compute_index(coords));
    }

    inline const T& get_raw(const std::array<u64, dim>& coords) const
    {
        return m_buffer->get(m_raw_start_offset +
                             _compute_index(coords, repeat<u64, dim>(0)));
    }

    const std::array<u64, dim>& size() const { return m_size; }
    const std::array<u64, dim>& size_with_halo() const { return m_raw_size; }
    const std::array<u64, dim>& stride() const { return m_buffer->stride(); }
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
        auto tup = std::tie(*this);
        auto func = [&](std::array<u64, dim>&, accessor<0, dim, T>& acc) {
            acc.get({ 0, 0 }) = value;
        };
        _iterate_impl<0, decltype(func), dim, T>(
            tup,
            repeat<u64, dim>(0),
            m_raw_size,
            std::make_tuple(&get_raw(repeat<u64, dim>(0))),
            func);
    }

    void copy_halo_from(grid<dim, T>& other, const std::array<i32, dim>& relpos)
    {
        // TODO: efficient implementation with counter loop
        std::array<u64, dim> len, from_src, from_dst;
        for (u32 i = 0; i < dim; ++i) {
            if (relpos[i] < 0) {
                len[i] = other.m_halo_size;
                from_src[i] = other.m_size[i] - other.m_halo_size;
                from_dst[i] = 0;
            } else if (relpos[i] == 0) {
                len[i] = other.m_size[i];
                from_src[i] = 0;
                from_dst[i] = m_halo_size;
            } else {
                len[i] = other.m_halo_size;
                from_src[i] = 0;
                from_dst[i] = m_raw_size[i] - m_halo_size;
            }
        }
        loop<dim>(
            repeat<u64, dim>(0), len, [&](const std::array<u64, dim>& it) {
                get_raw(from_dst + it) = other.get(from_src + it);
            });
    }
};

template<u32 rad, u32 dim, typename... T>
class accessor
{
protected:
    template<u32, typename>
    friend class buffer;
    const std::array<i64, ipow(2 * rad + 1, dim)> m_offset_table;
    using data_types = tl::type_list<T...>;
    tuple_counter<T*...> m_middle;

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

    inline void set_middle(const std::tuple<T*...>& middle)
    {
        m_middle.values = middle;
    }

    inline accessor<rad, dim, T...> operator+=(u64 inc)
    {
        m_middle += inc;
        return *this;
    }

    template<u32 i = 0>
    inline typename data_types::template get<i>& get(
        const std::array<i64, dim>& coords)
    {
        return *(std::get<i>(m_middle.values) +
                 m_offset_table[_compute_table_index(coords)]);
    }

    template<u32 i = 0>
    inline const typename data_types::template get<i>& get(
        const std::array<i64, dim>& coords) const
    {
        return *(std::get<i>(m_middle.values) +
                 m_offset_table[_compute_table_index(coords)]);
    }
};

template<u32 rad, typename Func, u32 dim, typename... T>
void _iterate_impl(std::tuple<grid<dim, T>&...>& buf,
                   const std::array<u64, dim>& from,
                   const std::array<u64, dim>& to,
                   std::tuple<T*...> cnt_init,
                   const Func& func)
{
    std::array<u64, dim> jumps;
    const auto stride = std::get<0>(buf).stride();
    const auto raw_size = std::get<0>(buf).size_with_halo();
    jumps[0] = stride[0];
    for (size_t i = 1; i < dim; ++i) {
        jumps[i] = stride[i - 1] * (raw_size[i] - to[i] + from[i]);
    }
    accessor<rad, dim, T...> acc(stride);
    acc.set_middle(cnt_init);
    loop_with_counter<dim, u64, accessor<rad, dim, T...>, u64>(
        from, to, acc, jumps, [&](std::array<u64, dim>& it, auto& cnt) {
            func(it, cnt);
        });
}

template<u32 rad, typename Func, u32 dim, typename... T>
void iterate(const Func& func, grid<dim, T>&... buf)
{
    auto bufs = std::tie(buf...);
    auto size = std::get<0>(bufs).size();
    auto cnt_init =
        tl::type_list<T...>::template for_each_and_collect<std::tuple>(
            [&](auto s) {
                using S = decltype(s);
                return &std::get<S::index>(bufs).get(repeat<u64, dim>(0));
            });
    _iterate_impl<rad, Func, dim, T...>(
        bufs, repeat<u64, dim>(0), size, cnt_init, func);
}

template<u32 rad, typename Func, u32 dim, typename T>
void iterate_halo(grid<dim, T>& buf, const Func& func)
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
    auto bufs = std::tie(buf);
    _iterate_impl<rad, decltype(wrapper), dim, T>(
        bufs,
        repeat<u64, dim>(0),
        buf.size_with_halo(),
        std::make_tuple(&buf.get_raw(repeat<u64, dim>(0))),
        wrapper);
}

template<u32 dim, typename... T>
class buffer_set : not_copyable
{
    std::tuple<buffer<dim, T>...> m_buffers;

public:
    buffer_set(const std::array<u64, dim>& size)
        : m_buffers(std::make_tuple(buffer<dim, T>(size)...))
    {}

    buffer_set(buffer_set<dim, T...>&& other)
        : m_buffers(std::move(other.m_buffers))
    {}

    template<u32 i>
    auto& get()
    {
        return std::get<i>(m_buffers);
    }
};

template<u32 dim, typename... T>
class grid_set : not_copyable
{
    std::tuple<grid<dim, T>...> m_grids;

public:
    grid_set(const std::array<u64, dim>& size,
             u32 halo_size,
             const std::array<u64, dim>& position,
             buffer_set<dim, T...>& buffers)
        : m_grids(
              tl::type_list<T...>::template for_each_and_collect<std::tuple>(
                  [&](auto t) {
                      using item_type = decltype(t);
                      return grid<dim, typename item_type::type>(
                          size,
                          halo_size,
                          position,
                          &buffers.template get<item_type::index>());
                  }))
    {}

    template<typename... S>
    struct subset_t
    {
        std::tuple<grid<dim, S>&...> grids;

        template<u32 rad, typename Func>
        void iterate(const Func& func)
        {
            auto size = std::get<0>(grids).size();
            auto cnt_init =
                tl::type_list<T...>::template for_each_and_collect<std::tuple>(
                    [&](auto r) {
                        using R = decltype(r);
                        return &std::get<R::index>(grids).get(
                            repeat<u64, dim>(0));
                    });
            _iterate_impl<rad, Func, dim, S...>(
                grids, repeat<u64, dim>(0), size, cnt_init, func);
        }

        template<u32 i>
        auto& get()
        {
            return std::get<i>(grids);
        }
    };

    template<u32... indices>
    auto subset()
    {
        typename tl::type_list<T...>::template keep<indices...>::template to<
            subset_t>
            bufs{ std::tie(std::get<indices>(m_grids)...) };
        return bufs;
    }

    template<u32 i>
    auto& get()
    {
        return std::get<i>(m_grids);
    }
};
}
