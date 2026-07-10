#include "core/geometry/Insert.h"

#include <algorithm>
#include <cmath>

namespace lcad {

std::vector<std::unique_ptr<Entity>> InsertEntity::instantiate() const {
    std::vector<std::unique_ptr<Entity>> result;
    if (!m_block) return result;
    result.reserve(m_block->entities.size());
    for (const auto& child : m_block->entities) {
        std::unique_ptr<Entity> copy = child->clone();
        copy->scale(Point2D(0, 0), m_scale);
        copy->rotate(Point2D(0, 0), m_rotation);
        copy->translate(m_position);
        result.push_back(std::move(copy));
    }
    return result;
}

BoundingBox InsertEntity::boundingBox() const {
    BoundingBox box;
    for (const auto& e : instantiate()) box.expand(e->boundingBox());
    if (!box.isValid()) box.expand(m_position); // empty block: at least the insertion point
    return box;
}

double InsertEntity::distanceTo(const Point2D& pt) const {
    double best = pt.distanceTo(m_position);
    for (const auto& e : instantiate()) best = std::min(best, e->distanceTo(pt));
    return best;
}

void InsertEntity::translate(const Point2D& delta) {
    m_position = m_position + delta;
}

void InsertEntity::rotate(const Point2D& center, double angleRadians) {
    m_position = rotateAround(m_position, center, angleRadians);
    m_rotation += angleRadians;
}

void InsertEntity::scale(const Point2D& center, double factor) {
    m_position = scaleAround(m_position, center, factor);
    m_scale *= factor;
}

void InsertEntity::mirror(const Point2D& a, const Point2D& b) {
    m_position = mirrorAcross(m_position, a, b);
    const double phi = std::atan2(b.y - a.y, b.x - a.x);
    m_rotation = 2.0 * phi - m_rotation;
}

std::vector<Point2D> InsertEntity::gripPoints() const {
    return {m_position};
}

void InsertEntity::moveGripPoint(std::size_t index, const Point2D& newPos) {
    if (index == 0) m_position = newPos;
}

std::vector<SnapPoint> InsertEntity::snapCandidates() const {
    return {{m_position, SnapKind::Endpoint}};
}

std::unique_ptr<Entity> InsertEntity::clone() const {
    return std::make_unique<InsertEntity>(*this);
}

} // namespace lcad
