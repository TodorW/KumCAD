#pragma once

#include <cmath>

namespace lcad {

struct Point2D {
    double x = 0.0;
    double y = 0.0;

    Point2D() = default;
    Point2D(double x_, double y_) : x(x_), y(y_) {}

    Point2D operator+(const Point2D& o) const { return {x + o.x, y + o.y}; }
    Point2D operator-(const Point2D& o) const { return {x - o.x, y - o.y}; }
    Point2D operator*(double s) const { return {x * s, y * s}; }

    double dot(const Point2D& o) const { return x * o.x + y * o.y; }
    double length() const { return std::sqrt(x * x + y * y); }
    double distanceTo(const Point2D& o) const { return (*this - o).length(); }
};

} // namespace lcad
