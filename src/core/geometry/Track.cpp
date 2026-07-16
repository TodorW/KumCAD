#include "core/geometry/Track.h"

#include <algorithm>
#include <limits>

namespace lcad {

BoundingBox TrackEntity::boundingBox() const {
    BoundingBox box;
    for (const Point2D& v : m_vertices) box.expand(v);
    return box;
}

double TrackEntity::distanceTo(const Point2D& pt) const {
    double best = std::numeric_limits<double>::infinity();
    for (std::size_t i = 0; i + 1 < m_vertices.size(); ++i) {
        const Point2D& a = m_vertices[i];
        const Point2D& b = m_vertices[i + 1];
        const Point2D seg = b - a;
        const double lenSq = seg.dot(seg);
        double t = lenSq < 1e-12 ? 0.0 : (pt - a).dot(seg) / lenSq;
        t = std::clamp(t, 0.0, 1.0);
        best = std::min(best, pt.distanceTo(a + seg * t));
    }
    if (m_vertices.size() == 1) best = pt.distanceTo(m_vertices.front());
    return std::max(0.0, best - m_width / 2.0);
}

void TrackEntity::translate(const Point2D& delta) {
    for (Point2D& v : m_vertices) v = v + delta;
}

void TrackEntity::rotate(const Point2D& center, double angleRadians) {
    for (Point2D& v : m_vertices) v = rotateAround(v, center, angleRadians);
}

void TrackEntity::scale(const Point2D& center, double factor) {
    for (Point2D& v : m_vertices) v = scaleAround(v, center, factor);
    m_width *= factor;
}

void TrackEntity::mirror(const Point2D& a, const Point2D& b) {
    for (Point2D& v : m_vertices) v = mirrorAcross(v, a, b);
}

std::vector<Point2D> TrackEntity::gripPoints() const { return m_vertices; }

void TrackEntity::moveGripPoint(std::size_t index, const Point2D& newPos) {
    if (index < m_vertices.size()) m_vertices[index] = newPos;
}

std::vector<SnapPoint> TrackEntity::snapCandidates() const {
    std::vector<SnapPoint> pts;
    pts.reserve(m_vertices.size());
    for (const Point2D& v : m_vertices) pts.push_back({v, SnapKind::Endpoint});
    return pts;
}

std::unique_ptr<Entity> TrackEntity::clone() const { return std::make_unique<TrackEntity>(*this); }

} // namespace lcad
