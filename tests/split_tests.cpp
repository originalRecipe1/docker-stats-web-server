#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "../split.hpp"

TEST_CASE("Comma separated input") {
    auto tokens = split("a,b,c", ',');
    REQUIRE(tokens.size() == 3);
    REQUIRE(tokens[0] == "a");
    REQUIRE(tokens[1] == "b");
    REQUIRE(tokens[2] == "c");
}

TEST_CASE("Single element") {
    auto tokens = split("single", ',');
    REQUIRE(tokens.size() == 1);
    REQUIRE(tokens[0] == "single");
}

TEST_CASE("Empty string") {
    auto tokens = split("", ',');
    REQUIRE(tokens.empty());
}
