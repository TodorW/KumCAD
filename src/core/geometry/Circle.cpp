#include "core/geometry/Circle.h"

#include <cmath>

namespace lcad {

BoundingBox CircleEntity::boundingBox() const {
    BoundingBox box;
    box.expand(Point2D(m_center.x - m_radius, m_center.y - m_radius));
    box.expand(Point2D(m_center.x + m_radius, m_center.y + m_radius));
    return box;
}

double CircleEntity::distanceTo(const Point2D& pt) const {
    return std::abs(pt.distanceTo(m_center) - m_radius);
}

std::unique_ptr<Entity> CircleEntity::clone() const {
    return std::make_unique<CircleEntity>(*this);
}

} // namespace lcad
