#include "core/geometry/Image.h"

#include <algorithm>
#include <cmath>

namespace lcad {

BoundingBox ImageEntity::boundingBox() const {
    BoundingBox box;
    for (const Point2D& corner : {Point2D(0, 0), Point2D(m_width, 0), Point2D(m_width, m_height), Point2D(0, m_height)}) {
        box.expand(rotateAround(corner, Point2D(), m_rotation) + m_position);
    }
    return box;
}

double ImageEntity::distanceTo(const Point2D& pt) const {
    const Point2D local = rotateAround(pt, m_position, -m_rotation) - m_position;
    if (local.x >= 0.0 && local.x <= m_width && local.y >= 0.0 && local.y <= m_height) return 0.0;
    const double dx = std::max({-local.x, local.x - m_width, 0.0});
    const double dy = std::max({-local.y, local.y - m_height, 0.0});
    return std::sqrt(dx * dx + dy * dy);
}

void ImageEntity::translate(const Point2D& delta) { m_position = m_position + delta; }

void ImageEntity::rotate(const Point2D& center, double angleRadians) {
    m_position = rotateAround(m_position, center, angleRadians);
    m_rotation += angleRadians;
}

void ImageEntity::scale(const Point2D& center, double factor) {
    m_position = scaleAround(m_position, center, factor);
    m_width *= factor;
    m_height *= factor;
}

void ImageEntity::mirror(const Point2D& a, const Point2D& b) {
    m_position = mirrorAcross(m_position, a, b);
    const double phi = std::atan2(b.y - a.y, b.x - a.x);
    m_rotation = 2.0 * phi - m_rotation;
}

std::vector<Point2D> ImageEntity::gripPoints() const { return {m_position}; }

void ImageEntity::moveGripPoint(std::size_t index, const Point2D& newPos) {
    if (index == 0) m_position = newPos;
}

std::vector<SnapPoint> ImageEntity::snapCandidates() const {
    std::vector<SnapPoint> result;
    for (const Point2D& corner : {Point2D(0, 0), Point2D(m_width, 0), Point2D(m_width, m_height), Point2D(0, m_height)}) {
        result.push_back({rotateAround(corner, Point2D(), m_rotation) + m_position, SnapKind::Endpoint});
    }
    return result;
}

std::unique_ptr<Entity> ImageEntity::clone() const { return std::make_unique<ImageEntity>(*this); }

} // namespace lcad
