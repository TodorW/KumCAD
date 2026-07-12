#pragma once

#include <optional>
#include <string>

namespace lcad {

// AutoCAD CAL/QuickCalc-style arithmetic expression evaluator: +, -, *, /,
// ^ (exponent, right-associative), unary +/-, parentheses, the constants pi
// and e, and functions sin/cos/tan/asin/acos/atan/atan2/sqrt/abs/ln/log/exp/
// min/max. Trig functions take/return degrees (AutoCAD CAL's default angle
// unit), not radians. Returns nullopt with *errorOut set on a malformed
// expression (unknown token, mismatched parens, wrong argument count,
// division by zero, domain error).
std::optional<double> evaluateExpression(const std::string& expr, std::string* errorOut = nullptr);

} // namespace lcad
