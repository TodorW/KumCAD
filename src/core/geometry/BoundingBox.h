#pragma once

#include "core/geometry/Point2D.h"

#include <algorithm>
#include <limits>

namespace lcad {

struct BoundingBox {
    Point2D min{std::numeric_limits<double>::max(), std::numeric_limits<double>::max()};
    Point2D max{std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest()};

    bool isValid() const { return min.x <= max.x && min.y <= max.y; }

    void expand(const Point2D& p) {
        min.x = std::min(min.x, p.x);
        min.y = std::min(min.y, p.y);
        max.x = std::max(max.x, p.x);
        max.y = std::max(max.y, p.y);
    }

    void expand(const BoundingBox& other) {
        if (!other.isValid()) return;
        expand(other.min);
        expand(other.max);
    }

    bool contains(const Point2D& p) const {
        return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y;
    }

    // Used for AutoCAD-style window selection: entity must be fully inside.
    bool containsBox(const BoundingBox& other) const { return contains(other.min) && contains(other.max); }

    // Used for AutoCAD-style crossing selection: any overlap counts.
    bool intersects(const BoundingBox& other) const {
        return min.x <= other.max.x && max.x >= other.min.x && min.y <= other.max.y && max.y >= other.min.y;
    }
};

} // namespace lcad
