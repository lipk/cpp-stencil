#include <buffer.hpp>
#include <catch2/catch.hpp>

namespace stencil {
TEST_CASE("create grid", "[grid]")
{
    std::array<u64, 1> s1 = { 5 };
    buffer<1, int> b1(s1);
    grid<1, int> g1(s1, 0, repeat<u64, 1>(0), &b1);
    CHECK(g1.size() == s1);

    std::array<u64, 3> s2 = { 5, 10, 15 };
    buffer<3, int> b2(s2);
    grid<3, int> g2(s2, 2, repeat<u64, 3>(0), &b2);
    CHECK(g2.size() == s2);

    std::array<u64, 10> s3 = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    buffer<10, int> b3(s3);
    grid<10, int> g3(s3, 1, repeat<u64, 10>(0), &b3);
    CHECK(g3.size() == s3);
}

TEST_CASE("read/write elements", "[grid]")
{
    buffer<2, int> buf1({ 5, 5 });
    grid<2, int> grid1({ 5, 5 }, 0, { 0, 0 }, &buf1);
    grid1.get({ 0, 0 }) = 10;
    grid1.get({ 4, 4 }) = 11;
    grid1.get({ 3, 2 }) = 12;
    CHECK(grid1.get({ 0, 0 }) == 10);
    CHECK(grid1.get_raw({ 0, 0 }) == 10);
    CHECK(grid1.get({ 4, 4 }) == 11);
    CHECK(grid1.get_raw({ 4, 4 }) == 11);
    CHECK(grid1.get({ 3, 2 }) == 12);
    CHECK(grid1.get_raw({ 3, 2 }) == 12);

    buffer<2, int> buf2({ 7, 7 });
    grid<2, int> grid2({ 5, 5 }, 1, { 1, 1 }, &buf2);
    grid2.get({ 0, 0 }) = 10;
    grid2.get({ 4, 4 }) = 11;
    grid2.get({ 3, 2 }) = 12;
    CHECK(grid2.get({ 0, 0 }) == 10);
    CHECK(grid2.get_raw({ 1, 1 }) == 10);
    CHECK(grid2.get({ 4, 4 }) == 11);
    CHECK(grid2.get_raw({ 5, 5 }) == 11);
    CHECK(grid2.get({ 3, 2 }) == 12);
    CHECK(grid2.get_raw({ 4, 3 }) == 12);
}

TEST_CASE("iterate", "[grid]")
{
    buffer<2, int> buf1({ 4, 4 });
    grid<2, int> grid1({ 2, 2 }, 1, { 1, 1 }, &buf1);
    iterate<1>(
        [&](const std::array<u64, 2>&, accessor<1, 2, int>& acc) {
            acc.get({ -1, -1 }) = 1;
            acc.get({ -1, 0 }) = 2;
            acc.get({ -1, 1 }) = 3;
            acc.get({ 0, -1 }) = 4;
            acc.get({ 0, 0 }) = 5;
            acc.get({ 0, 1 }) = 6;
            acc.get({ 1, -1 }) = 7;
            acc.get({ 1, 0 }) = 8;
            acc.get({ 1, 1 }) = 9;
        },
        grid1);
    int values[4][4] = {
        { 1, 1, 2, 3 },
        { 1, 1, 2, 3 },
        { 4, 4, 5, 6 },
        { 7, 7, 8, 9 },
    };
    for (u32 i = 0; i < 4; ++i) {
        for (u32 j = 0; j < 4; ++j) {
            INFO(i << " " << j);
            CHECK(values[i][j] == grid1.get_raw({ i, j }));
        }
    }
}
TEST_CASE("iterate_halo", "[grid]")
{
    buffer<2, int> buf1({ 4, 4 });
    grid<2, int> grid1({ 2, 2 }, 1, { 1, 1 }, &buf1);
    iterate<0>(
        [&](const std::array<u64, 2>&, accessor<0, 2, int>& acc) {
            acc.get({ 0, 0 }) = 1;
        },
        grid1);
    iterate_halo<0>(grid1,
                    [&](const std::array<u64, 2>&,
                        accessor<0, 2, int>& acc,
                        const std::array<bool, 2>&) {
                        acc.get({ 0, 0 }) = 2;
                    });
    int values[4][4] = {
        { 2, 2, 2, 2 },
        { 2, 1, 1, 2 },
        { 2, 1, 1, 2 },
        { 2, 2, 2, 2 },
    };
    for (u32 i = 0; i < 4; ++i) {
        for (u32 j = 0; j < 4; ++j) {
            INFO(i << " " << j);
            CHECK(values[i][j] == grid1.get_raw({ i, j }));
        }
    }
}

TEST_CASE("fill_halo", "[grid]")
{
    buffer<2, int> buf1({ 6, 6 });
    grid<2, int> grid1({ 2, 2 }, 2, { 2, 2 }, &buf1);
    iterate<0>(
        [&](const std::array<u64, 2>&, accessor<0, 2, int>& acc) {
            acc.get({ 0, 0 }) = 1;
        },
        grid1);
    grid1.fill_halo(2);
    int values[6][6] = {
        { 2, 2, 2, 2, 2, 2 }, { 2, 2, 2, 2, 2, 2 }, { 2, 2, 1, 1, 2, 2 },
        { 2, 2, 1, 1, 2, 2 }, { 2, 2, 2, 2, 2, 2 }, { 2, 2, 2, 2, 2, 2 },
    };
    for (u32 i = 0; i < 6; ++i) {
        for (u32 j = 0; j < 6; ++j) {
            INFO(i << " " << j);
            CHECK(values[i][j] == grid1.get_raw({ i, j }));
        }
    }
}

TEST_CASE("copy_halo", "[grid]")
{
    buffer<2, int> buf1({ 6, 6 });
    buffer<2, int> buf2({ 6, 6 });
    grid<2, int> grid1({ 2, 2 }, 2, { 2, 2 }, &buf1);
    grid<2, int> grid2({ 2, 2 }, 2, { 2, 2 }, &buf2);
    iterate<0>(
        [&](const std::array<u64, 2>& it, accessor<0, 2, int>& acc) {
            if (it[0] == 0) {
                acc.get({ 0, 0 }) = 0;
            } else {
                acc.get({ 0, 0 }) = 1;
            }
        },
        grid1);
    iterate<0>(
        [&](const std::array<u64, 2>&, accessor<0, 2, int>& acc) {
            acc.get({ 0, 0 }) = 2;
        },
        grid2);
    grid2.fill_halo(2);

    int values[6][6] = {
        { 2, 2, 2, 2, 2, 2 }, { 2, 2, 2, 2, 2, 2 }, { 2, 2, 2, 2, 2, 2 },
        { 2, 2, 2, 2, 2, 2 }, { 2, 2, 2, 2, 2, 2 }, { 2, 2, 2, 2, 2, 2 },
    };
    auto check_values = [&] {
        for (u32 i = 0; i < 6; ++i) {
            for (u32 j = 0; j < 6; ++j) {
                INFO("Coordinates " << i << " " << j);
                CHECK(values[i][j] == grid2.get_raw({ i, j }));
            }
        }
    };
    auto update_values = [&](u32 x, u32 y) {
        values[x][y] = 0;
        values[x][y + 1] = 0;
        values[x + 1][y] = 1;
        values[x + 1][y + 1] = 1;
    };

    for (i32 i = -1; i < 2; ++i) {
        for (i32 j = -1; j < 2; ++j) {
            INFO("Position " << i << " " << j);
            grid2.copy_halo_from(grid1, { i, j });
            update_values(2 * (i + 1), 2 * (j + 1));
            check_values();
        }
    }
}

TEST_CASE("create grid_set", "[grid_set]")
{
    buffer_set<2, int, int> bufs1({ 6, 6 });
    grid_set<2, int, int> grids1({ 2, 2 }, 2, { 2, 2 }, bufs1);
}

TEST_CASE("iterate grid_set", "[grid_set]")
{
    buffer_set<2, int, int> bufs1({ 6, 6 });
    grid_set<2, int, int> grids1({ 2, 2 }, 2, { 2, 2 }, bufs1);
    grids1.subset<0, 1>().iterate<0>(
        [&](const auto& /*it*/, accessor<0, 2, int, int>& acc) {
            acc.get<0>({ 0, 0 }) = 1;
        });
}
}
