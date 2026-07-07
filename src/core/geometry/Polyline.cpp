#include "core/geometry/Polyline.h"

#include <algorithm>
#include <limits>

namespace lcad {

namespace {

double distanceToSegment(const Point2D& pt, const Point2D& a, const Point2D& b) {
    const Point2D seg = b - a;
    const double lenSq = seg.dot(seg);
    if (lenSq < 1e-12) return pt.distanceTo(a);
    double t = (pt - a).dot(seg) / lenSq;
    t = std::clamp(t, 0.0, 1.0);
    return pt.distanceTo(a + seg * t);
}

} // namespace

BoundingBox PolylineEntity::boundingBox() const {
    BoundingBox box;
    for (const auto& v : m_vertices) box.expand(v);
    return box;
}

double PolylineEntity::distanceTo(const Point2D& pt) const {
    double best = std::numeric_limits<double>::max();
    for (std::size_t i = 0; i + 1 < m_vertices.size(); ++i) {
        best = std::min(best, distanceToSegment(pt, m_vertices[i], m_vertices[i + 1]));
    }
    if (m_closed && m_vertices.size() > 1) {
        best = std::min(best, distanceToSegment(pt, m_vertices.back(), m_vertices.front()));
    }
    return best;
}

std::unique_ptr<Entity> PolylineEntity::clone() const {
    return std::make_unique<PolylineEntity>(*this);
}

} // namespace lcad
