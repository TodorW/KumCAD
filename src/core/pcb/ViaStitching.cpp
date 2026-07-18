#include "core/pcb/ViaStitching.h"

#include "core/document/Document.h"
#include "core/geometry/Polyline.h"
#include "core/geometry/PolylineOps.h"
#include "core/geometry/Via.h"

#include <algorithm>
#include <cmath>

namespace lcad {

namespace {
Point2D centroidOf(const std::vector<Point2D>& boundary) {
    double sx = 0.0, sy = 0.0;
    for (const Point2D& p : boundary) {
        sx += p.x;
        sy += p.y;
    }
    const double n = static_cast<double>(boundary.size());
    return Point2D(sx / n, sy / n);
}

// Per-vertex move toward the centroid -- NOT a true parallel offset (a
// vertex near a sharp corner moves less than `inset` perpendicular-to-
// edge, and a very elongated boundary insets unevenly), but a simple,
// always-succeeding fallback for when offsetPolyline (the real parallel
// offset, tried first below) degenerates.
std::vector<Point2D> centroidInset(const std::vector<Point2D>& boundary, const Point2D& centroid, double inset) {
    std::vector<Point2D> result;
    result.reserve(boundary.size());
    for (const Point2D& v : boundary) {
        const double dx = centroid.x - v.x;
        const double dy = centroid.y - v.y;
        const double dist = std::sqrt(dx * dx + dy * dy);
        if (dist < 1e-9) {
            result.push_back(v);
            continue;
        }
        const double t = std::min(inset / dist, 1.0);
        result.emplace_back(v.x + dx * t, v.y + dy * t);
    }
    return result;
}
} // namespace

std::vector<EntityId> stitchVias(Document& doc, LayerId layer, const std::vector<Point2D>& boundary, double spacing,
                                 double inset, double diameter, double drillDiameter) {
    std::vector<EntityId> placed;
    if (boundary.size() < 3 || spacing <= 1e-9) return placed;

    const Point2D centroid = centroidOf(boundary);

    // A true parallel (miter-normal) offset -- PolylineOps.h's own
    // offsetPolyline(), the same algorithm OFFSET already uses -- rather
    // than the old per-vertex centroid move, which distorted concave or
    // elongated boundaries. Falls back to the centroid move only if
    // offsetPolyline itself degenerates (e.g. inset larger than a local
    // corner radius allows).
    const PolylineEntity sourcePolyline(0, layer, boundary, true);
    const std::unique_ptr<PolylineEntity> offsetResult = offsetPolyline(sourcePolyline, 0, inset, centroid);
    const std::vector<Point2D> insetBoundary =
        offsetResult ? offsetResult->flattenedVertices() : centroidInset(boundary, centroid, inset);
    if (insetBoundary.size() < 3) return placed;

    double perimeter = 0.0;
    for (std::size_t i = 0; i < insetBoundary.size(); ++i) {
        perimeter += insetBoundary[i].distanceTo(insetBoundary[(i + 1) % insetBoundary.size()]);
    }
    if (perimeter < 1e-9) return placed;

    const int count = std::max(1, static_cast<int>(std::lround(perimeter / spacing)));
    const double stepLen = perimeter / count;

    // Walk the inset boundary's cumulative arc length, dropping a via
    // every stepLen -- the same even-arc-length resample pattern
    // PolylineOps.cpp's revisionCloud() uses for the original boundary.
    double travelled = 0.0;
    double nextMark = 0.0;
    Point2D cur = insetBoundary[0];
    int emitted = 0;
    for (std::size_t i = 0; i < insetBoundary.size() && emitted < count; ++i) {
        const Point2D& next = insetBoundary[(i + 1) % insetBoundary.size()];
        const double segLen = cur.distanceTo(next);
        if (segLen < 1e-12) {
            cur = next;
            continue;
        }
        while (travelled + segLen >= nextMark - 1e-9 && emitted < count) {
            const double t = std::clamp((nextMark - travelled) / segLen, 0.0, 1.0);
            const Point2D pos(cur.x + (next.x - cur.x) * t, cur.y + (next.y - cur.y) * t);
            auto via = std::make_unique<ViaEntity>(doc.reserveEntityId(), layer, pos, diameter, drillDiameter);
            placed.push_back(via->id());
            doc.addEntity(std::move(via));
            nextMark += stepLen;
            ++emitted;
        }
        travelled += segLen;
        cur = next;
    }

    return placed;
}

} // namespace lcad
