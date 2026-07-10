#include <catch2/catch_test_macros.hpp> // (almost always sufficient)
//       <catch2/matchers/catch_matchers_*.hpp — add if/when you use REQUIRE_THAT
//       <catch2/benchmark/catch_benchmark.hpp — add if/when you use BENCHMARK

#include "example.h"


// The [example] tag means you can run just these tests with ./myapp_tests '[example]' if needed.

TEST_CASE("Example test group", "[example]")
{
    SECTION("integers")
    {
        int x = getNumber();
        REQUIRE(x == 54);
        REQUIRE(x > 0);
    }

    SECTION("booleans")
    {
        bool flag = true;
        REQUIRE(flag);
        REQUIRE_FALSE(!flag);
    }

    SECTION("strings")
    {
        std::string s = "hello";
        REQUIRE(s == "hello");
        REQUIRE(s.size() == 5);
    }
}

TEST_CASE("Another example test group", "[example]")
{
    SECTION("floating point")
    {
        double val = 3.14;
        REQUIRE(val > 3.0);
        REQUIRE(val < 4.0);
    }

    SECTION("vectors")
    {
        std::vector<int> v = {1, 2, 3};
        REQUIRE(v.size() == 3);
        REQUIRE(v[0] == 1);
        REQUIRE(v.back() == 3);
    }
}