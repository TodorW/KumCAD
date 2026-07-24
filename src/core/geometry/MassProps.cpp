#include "core/geometry/MassProps.h"

#include <cmath>

namespace lcad {

PolygonMassProps computePolygonMassProps(const std::vector<Point2D>& vertices) {
    PolygonMassProps result;
    const std::size_t n = vertices.size();
    if (n < 3) return result;

    double areaSum = 0.0, cxSum = 0.0, cySum = 0.0, perimeter = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        const Point2D& a = vertices[i];
        const Point2D& b = vertices[(i + 1) % n];
        const double cross = a.x * b.y - b.x * a.y;
        areaSum += cross;
        cxSum += (a.x + b.x) * cross;
        cySum += (a.y + b.y) * cross;
        perimeter += a.distanceTo(b);
    }

    result.signedArea = areaSum / 2.0;
    result.perimeter = perimeter;
    if (std::abs(areaSum) > 1e-12) {
        result.centroid = Point2D(cxSum / (3.0 * areaSum), cySum / (3.0 * areaSum));
    } else {
        // Degenerate (zero-area) loop -- fall back to the plain vertex
        // average rather than dividing by ~zero.
        double sx = 0.0, sy = 0.0;
        for (const Point2D& v : vertices) {
            sx += v.x;
            sy += v.y;
        }
        result.centroid = Point2D(sx / static_cast<double>(n), sy / static_cast<double>(n));
    }
    return result;
}

RegionMassProps computeRegionMassProps(const std::vector<std::vector<Point2D>>& loops) {
    RegionMassProps result;
    double netSignedArea = 0.0, cxSum = 0.0, cySum = 0.0;

    for (const auto& loop : loops) {
        const PolygonMassProps props = computePolygonMassProps(loop);
        netSignedArea += props.signedArea;
        cxSum += props.signedArea * props.centroid.x;
        cySum += props.signedArea * props.centroid.y;
        result.perimeter += props.perimeter;
        for (const Point2D& v : loop) result.boundingBox.expand(v);
    }

    result.area = std::abs(netSignedArea);
    if (std::abs(netSignedArea) > 1e-12) {
        result.centroid = Point2D(cxSum / netSignedArea, cySum / netSignedArea);
    }
    return result;
}

} // namespace lcad
