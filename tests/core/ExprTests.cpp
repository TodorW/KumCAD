#include "core/util/Expr.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

using Catch::Approx;

TEST_CASE("evaluateExpression handles arithmetic and precedence", "[expr]") {
    REQUIRE(lcad::evaluateExpression("2+3*4").value() == Approx(14.0));
    REQUIRE(lcad::evaluateExpression("(2+3)*4").value() == Approx(20.0));
    REQUIRE(lcad::evaluateExpression("2^3^2").value() == Approx(512.0)); // right-associative
    REQUIRE(lcad::evaluateExpression("-2^2").value() == Approx(-4.0));   // unary binds looser than ^
    REQUIRE(lcad::evaluateExpression("10/4").value() == Approx(2.5));
    REQUIRE(lcad::evaluateExpression("  1 + 2  ").value() == Approx(3.0));
}

TEST_CASE("evaluateExpression supports constants and functions", "[expr]") {
    REQUIRE(lcad::evaluateExpression("pi").value() == Approx(M_PI));
    REQUIRE(lcad::evaluateExpression("sqrt(16)").value() == Approx(4.0));
    REQUIRE(lcad::evaluateExpression("sin(90)").value() == Approx(1.0));
    REQUIRE(lcad::evaluateExpression("cos(0)").value() == Approx(1.0));
    REQUIRE(lcad::evaluateExpression("atan2(1,1)").value() == Approx(45.0));
    REQUIRE(lcad::evaluateExpression("max(3,7)").value() == Approx(7.0));
    REQUIRE(lcad::evaluateExpression("min(3,7)").value() == Approx(3.0));
    REQUIRE(lcad::evaluateExpression("abs(-5)").value() == Approx(5.0));
}

TEST_CASE("evaluateExpression reports errors instead of crashing", "[expr]") {
    std::string error;
    REQUIRE_FALSE(lcad::evaluateExpression("1/0", &error).has_value());
    REQUIRE_FALSE(error.empty());

    REQUIRE_FALSE(lcad::evaluateExpression("(1+2", &error).has_value());
    REQUIRE_FALSE(lcad::evaluateExpression("1 + ", &error).has_value());
    REQUIRE_FALSE(lcad::evaluateExpression("bogus(1)", &error).has_value());
    REQUIRE_FALSE(lcad::evaluateExpression("sqrt(-1)", &error).has_value());
    REQUIRE_FALSE(lcad::evaluateExpression("sqrt(1,2)", &error).has_value());
    REQUIRE_FALSE(lcad::evaluateExpression("1 2", &error).has_value());
}
