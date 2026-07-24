#include "core/pcb/PadShapeGeometry.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace lcad {

std::vector<Point2D> roundRectPadOutline(double width, double height, double cornerRatio, int cornerArcSegments) {
    const double hw = width / 2.0;
    const double hh = height / 2.0;
    const double r = std::clamp(cornerRatio, 0.0, 0.5) * std::min(width, height);
    if (r < 1e-9 || cornerArcSegments < 1) {
        return {Point2D(hw, hh), Point2D(-hw, hh), Point2D(-hw, -hh), Point2D(hw, -hh)};
    }

    struct Corner {
        double cx;
        double cy;
        double startAngle;
    };
    const std::array<Corner, 4> corners{{
        {hw - r, hh - r, 0.0},                   // top-right, sweeps 0 -> 90 deg
        {-hw + r, hh - r, M_PI / 2.0},            // top-left, 90 -> 180
        {-hw + r, -hh + r, M_PI},                 // bottom-left, 180 -> 270
        {hw - r, -hh + r, 3.0 * M_PI / 2.0},      // bottom-right, 270 -> 360
    }};

    std::vector<Point2D> pts;
    pts.reserve(static_cast<std::size_t>(4 * cornerArcSegments));
    for (const Corner& c : corners) {
        for (int i = 0; i < cornerArcSegments; ++i) {
            const double t = c.startAngle + (M_PI / 2.0) * (static_cast<double>(i) / cornerArcSegments);
            pts.emplace_back(c.cx + r * std::cos(t), c.cy + r * std::sin(t));
        }
    }
    return pts;
}

std::vector<Point2D> trapezoidPadOutline(double width, double height, double delta) {
    const double hh = height / 2.0;
    const double topHalfWidth = std::max(0.0, (width - delta) / 2.0);
    const double bottomHalfWidth = std::max(0.0, (width + delta) / 2.0);
    return {Point2D(topHalfWidth, hh), Point2D(-topHalfWidth, hh), Point2D(-bottomHalfWidth, -hh),
            Point2D(bottomHalfWidth, -hh)};
}

} // namespace lcad
