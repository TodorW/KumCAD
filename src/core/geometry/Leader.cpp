#include "core/geometry/Leader.h"

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

BoundingBox LeaderEntity::boundingBox() const {
    BoundingBox box;
    for (const auto& p : m_points) box.expand(p);
    return box;
}

double LeaderEntity::distanceTo(const Point2D& pt) const {
    if (m_points.size() == 1) return pt.distanceTo(m_points.front());
    double best = std::numeric_limits<double>::max();
    for (std::size_t i = 0; i + 1 < m_points.size(); ++i) {
        best = std::min(best, distanceToSegment(pt, m_points[i], m_points[i + 1]));
    }
    return best;
}

void LeaderEntity::translate(const Point2D& delta) {
    for (auto& p : m_points) p = p + delta;
}

void LeaderEntity::rotate(const Point2D& center, double angleRadians) {
    for (auto& p : m_points) p = rotateAround(p, center, angleRadians);
}

void LeaderEntity::scale(const Point2D& center, double factor) {
    for (auto& p : m_points) p = scaleAround(p, center, factor);
    m_arrowSize *= factor;
}

void LeaderEntity::mirror(const Point2D& a, const Point2D& b) {
    for (auto& p : m_points) p = mirrorAcross(p, a, b);
}

std::vector<Point2D> LeaderEntity::gripPoints() const {
    return m_points;
}

void LeaderEntity::moveGripPoint(std::size_t index, const Point2D& newPos) {
    if (index < m_points.size()) m_points[index] = newPos;
}

std::vector<SnapPoint> LeaderEntity::snapCandidates() const {
    std::vector<SnapPoint> result;
    for (const auto& p : m_points) result.push_back({p, SnapKind::Endpoint});
    return result;
}

std::unique_ptr<Entity> LeaderEntity::clone() const {
    return std::make_unique<LeaderEntity>(*this);
}

} // namespace lcad
