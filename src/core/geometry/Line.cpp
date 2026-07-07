#include "core/geometry/Line.h"

#include <algorithm>

namespace lcad {

BoundingBox LineEntity::boundingBox() const {
    BoundingBox box;
    box.expand(m_start);
    box.expand(m_end);
    return box;
}

double LineEntity::distanceTo(const Point2D& pt) const {
    const Point2D seg = m_end - m_start;
    const double lenSq = seg.dot(seg);
    if (lenSq < 1e-12) return pt.distanceTo(m_start);

    double t = (pt - m_start).dot(seg) / lenSq;
    t = std::clamp(t, 0.0, 1.0);
    return pt.distanceTo(m_start + seg * t);
}

std::unique_ptr<Entity> LineEntity::clone() const {
    return std::make_unique<LineEntity>(*this);
}

} // namespace lcad
