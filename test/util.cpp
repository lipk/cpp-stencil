#include <catch2/catch.hpp>
#include <util.hpp>

using namespace stencil;

TEST_CASE("tuple_counter", "[util]")
{
    tuple_counter<int, double*, unsigned> cnt;
    cnt.values = std::make_tuple<int, double*, unsigned>(-12, nullptr, 3);

    cnt += 2;
    CHECK(std::get<0>(cnt.values) == -10);
    CHECK(reinterpret_cast<uintptr_t>(std::get<1>(cnt.values)) ==
          2 * sizeof(double));
    CHECK(std::get<2>(cnt.values) == 5);
}
