# cpp-stencil

This project aims to provide efficient and simple to use building blocks for
larger scale scientific computation frameworks. Work in progress.

Main features:

* Supports arbitrarily high dimensions. Most well-known frameworks are built with
physical simulations in mind, so they only support up to 3 dimensional meshes.
Nonetheless, higher dimensional applications exist in the fields of economics and
finance.

* Header-only implementation. This ensures that the compiler is able to inline
pretty much anything it wishes.

* Few external dependencies. The library itself only requires cpp-typelist
(checked in as a git submodule) and a C++11 capable compiler. Catch2 is required
for the unit tests, SDL2 and OpenMP for the demo apps.

## Usage

cpp-stencil is in a pre-alpha stage, but if you want to give it a spin
nonetheless, here's what you need to do.

First, make sure that your compiler can see `src/*.hpp` and
`dep/typelist/typelist.hpp`. The CMake build script is just for the tests and the
demo app, you can ignore it.

Right now, there's only one class of interest in `cpp-stencil`: `buffer`. It
allows you to create and manipulate an N-dimensional mesh. `buffer` takes two
template arguments: the dimensionality of the mesh, and the type of the data
stored therein. The constructor also takes two regular arguments, the size of the
mesh, passed in as an `std::array`, and the size of the halo.

```cpp
// Creates a 400x400 (or 402x402 with halo included) array of doubles
stencil::buffer<2, double> buf({400, 400}, 1);
```

To initialize the buffer, you may want to use `fill`. It sets every cell in the
buffer, halo included, to a particular value. You can also use `fill_halo` to
set the halo cells only.

```cpp
buf.fill(0.0); // set everything to 0
buf.fill_halo(1.0); // set halo to 1
```

You can perform a stencil operation using iterate:

```cpp
buf.iterate<1>([](const std::array<u64, 2>& it,
                  buffer<2, double>::accessor<1>& acc) {
                    acc.get({0, 0}) = some_func(it, acc);
                });
```

There are a couple of things to note here. The template argument of `iterate`
determines the stencil radius. Here, we use 1 which translates to a 3x3 stencil.
I could have used 0 to only operate on single cells, or 2 to have a 5x5 stencil
(although that might not be a good idea on a buffer with only one layer of halo
cells).

The sole argument of `iterate` is a callable which in turn expects two
parameters, the coordinates of the current cell and an `accessor` object. `u64`
is a type alias for `uint64_t` (there's also `u32`, `i32` etc). Cells can be
accessed via `acc` by relative coordinates. `{0, 0}` is the current cell,
`{1, 0}` is the one on its right, and so forth. Accessing cells outside the
specified stencil radius is undefined behaviour.

If you have split your data into more than one regions, as it is normal with
large scale calculations, you'll need to periodically synchronize halo cells
between different buffers. This what `copy_halo_from` is for. You need to pass
in a reference to the other buffer and an array describing the relative position.
The function will copy the edge cells of the specified buffer into the halo
appropriate halo cells.

```cpp
// {-1, 0} means that buf2 is on the left of buf1
buf1.copy_halo_from(buf2, {-1, 0});
```

## TODO

The long list of missing or inadequately implemented features:

* Efficient iteration on halo cells.
* Faster halo exchange.
* Additional halo filling strategies: mirror, wrap.
* Let a single buffer object multiple arrays of different types.
* Halo exchange between buffers with unaligned edges.
* OpenMPI support.
* Performance measurements & comparisons.
* Detailed documentation.
* Better error handling (currently tends to crash on invalid input)
* Make buffer movable.
