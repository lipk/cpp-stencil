#include <buffer.hpp>
#include <catch2/catch.hpp>
#include <iostream>

namespace stencil {
TEST_CASE("create", "[buffer]")
{
    std::array<u64, 1> s1 = { 5 };
    buffer<1, int> b1(s1, 0);
    CHECK(b1.size() == s1);

    std::array<u64, 3> s2 = { 5, 10, 15 };
    buffer<3, int> b2(s2, 2);
    CHECK(b2.size() == s2);

    std::array<u64, 10> s3 = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    buffer<10, int> b3(s3, 1);
    CHECK(b3.size() == s3);
}

TEST_CASE("read/write elements", "[buffer]")
{
    buffer<2, int> buf1({ 5, 5 }, 0);
    buf1.get({ 0, 0 }) = 10;
    buf1.get({ 4, 4 }) = 11;
    buf1.get({ 3, 2 }) = 12;
    CHECK(buf1.get({ 0, 0 }) == 10);
    CHECK(buf1.get_raw({ 0, 0 }) == 10);
    CHECK(buf1.get({ 4, 4 }) == 11);
    CHECK(buf1.get_raw({ 4, 4 }) == 11);
    CHECK(buf1.get({ 3, 2 }) == 12);
    CHECK(buf1.get_raw({ 3, 2 }) == 12);

    buffer<2, int> buf2({ 5, 5 }, 1);
    buf2.get({ 0, 0 }) = 10;
    buf2.get({ 4, 4 }) = 11;
    buf2.get({ 3, 2 }) = 12;
    CHECK(buf2.get({ 0, 0 }) == 10);
    CHECK(buf2.get_raw({ 1, 1 }) == 10);
    CHECK(buf2.get({ 4, 4 }) == 11);
    CHECK(buf2.get_raw({ 5, 5 }) == 11);
    CHECK(buf2.get({ 3, 2 }) == 12);
    CHECK(buf2.get_raw({ 4, 3 }) == 12);
}

TEST_CASE("iterate", "[buffer]")
{
    buffer<2, int> buf1({ 2, 2 }, 1);
    buf1.iterate<1>(
        [&](const std::array<u64, 2>&, buffer<2, int>::accessor<1>& acc) {
            acc.get({ -1, -1 }) = 1;
            acc.get({ -1, 0 }) = 2;
            acc.get({ -1, 1 }) = 3;
            acc.get({ 0, -1 }) = 4;
            acc.get({ 0, 0 }) = 5;
            acc.get({ 0, 1 }) = 6;
            acc.get({ 1, -1 }) = 7;
            acc.get({ 1, 0 }) = 8;
            acc.get({ 1, 1 }) = 9;
        });
    int values[4][4] = {
        { 1, 1, 2, 3 },
        { 1, 1, 2, 3 },
        { 4, 4, 5, 6 },
        { 7, 7, 8, 9 },
    };
    for (u32 i = 0; i < 4; ++i) {
        for (u32 j = 0; j < 4; ++j) {
            INFO(i << " " << j);
            CHECK(values[i][j] == buf1.get_raw({ i, j }));
        }
    }
}

TEST_CASE("iterate_halo", "[buffer]")
{
    buffer<2, int> buf1({ 2, 2 }, 1);
    buf1.iterate<0>(
        [&](const std::array<u64, 2>&, buffer<2, int>::accessor<0>& acc) {
            acc.get({ 0, 0 }) = 1;
        });
    buf1.iterate_halo<0>([&](const std::array<u64, 2>&,
                             buffer<2, int>::accessor<0>& acc,
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
            CHECK(values[i][j] == buf1.get_raw({ i, j }));
        }
    }
}

// TEST_CASE("fill_halo", "[buffer]")
//{
//    buffer<2, int> buf1({ 2, 2 }, 2);
//    buf1.iterate<0>(
//        [&](const std::array<u64, 2>&, buffer<2, int>::accessor<0>& acc) {
//            acc.get({ 0, 0 }) = 1;
//        });
//    buf1.halo_fill(2);
//    int values[6][6] = {
//        { 2, 2, 2, 2, 2, 2 }, { 2, 2, 2, 2, 2, 2 }, { 2, 2, 1, 1, 2, 2 },
//        { 2, 2, 1, 1, 2, 2 }, { 2, 2, 2, 2, 2, 2 }, { 2, 2, 2, 2, 2, 2 },
//    };
//    for (u32 i = 0; i < 6; ++i) {
//        for (u32 j = 0; j < 6; ++j) {
//            INFO(i << " " << j);
//            CHECK(values[i][j] == buf1.get_raw({ i, j }));
//        }
//    }
//}
}
