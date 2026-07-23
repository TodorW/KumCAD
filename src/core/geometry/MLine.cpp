#include "core/geometry/MLine.h"

#include <algorithm>
#include <limits>
#include <optional>

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

std::vector<std::vector<Point2D>> MLineEntity::elementLines() const {
    std::vector<std::vector<Point2D>> result(m_elements.size());
    const std::size_t n = m_vertices.size();
    if (n < 2) return result;

    // Per-vertex miter offset direction (unit vector) and length-scale
    // factor -- shared across every element (only the signed distance
    // differs), computed once. See this class's own header comment for
    // the miter-offset derivation.
    std::vector<Point2D> miterDir(n);
    std::vector<double> miterScale(n, 1.0);
    for (std::size_t i = 0; i < n; ++i) {
        std::optional<Point2D> n1, n2;
        if (i > 0 || m_closed) {
            const Point2D& a = m_vertices[i == 0 ? n - 1 : i - 1];
            const Point2D& b = m_vertices[i];
            Point2D d = b - a;
            const double len = d.length();
            if (len > 1e-12) {
                d = d * (1.0 / len);
                n1 = Point2D(-d.y, d.x);
            }
        }
        if (i + 1 < n || m_closed) {
            const Point2D& a = m_vertices[i];
            const Point2D& b = m_vertices[(i + 1) % n];
            Point2D d = b - a;
            const double len = d.length();
            if (len > 1e-12) {
                d = d * (1.0 / len);
                n2 = Point2D(-d.y, d.x);
            }
        }

        Point2D normal;
        double factor = 1.0;
        if (n1 && n2) {
            const Point2D sum = *n1 + *n2;
            const double sumLen = sum.length();
            if (sumLen > 1e-9) {
                const Point2D miter = sum * (1.0 / sumLen);
                const double denom = n1->dot(miter);
                normal = miter;
                factor = std::abs(denom) > 1e-6 ? 1.0 / denom : 1.0;
            } else {
                normal = *n1; // ~180 deg fold-back: degenerate, fall back to the incoming normal
            }
        } else if (n1) {
            normal = *n1;
        } else if (n2) {
            normal = *n2;
        }
        miterDir[i] = normal;
        miterScale[i] = factor;
    }

    for (std::size_t e = 0; e < m_elements.size(); ++e) {
        const double dist = m_elements[e].offset * m_scale;
        std::vector<Point2D> line(n);
        for (std::size_t i = 0; i < n; ++i) line[i] = m_vertices[i] + miterDir[i] * (dist * miterScale[i]);
        result[e] = std::move(line);
    }
    return result;
}

BoundingBox MLineEntity::boundingBox() const {
    BoundingBox box;
    for (const auto& line : elementLines())
        for (const Point2D& v : line) box.expand(v);
    if (m_elements.empty()) {
        for (const Point2D& v : m_vertices) box.expand(v);
    }
    return box;
}

double MLineEntity::distanceTo(const Point2D& pt) const {
    double best = std::numeric_limits<double>::max();
    bool any = false;
    for (const auto& line : elementLines()) {
        for (std::size_t i = 0; i + 1 < line.size(); ++i) {
            best = std::min(best, distanceToSegment(pt, line[i], line[i + 1]));
            any = true;
        }
        if (m_closed && line.size() >= 2) best = std::min(best, distanceToSegment(pt, line.back(), line.front()));
    }
    if (!any) {
        for (std::size_t i = 0; i + 1 < m_vertices.size(); ++i)
            best = std::min(best, distanceToSegment(pt, m_vertices[i], m_vertices[i + 1]));
    }
    return best;
}

void MLineEntity::translate(const Point2D& delta) {
    for (Point2D& v : m_vertices) v = v + delta;
}

void MLineEntity::rotate(const Point2D& center, double angleRadians) {
    for (Point2D& v : m_vertices) v = rotateAround(v, center, angleRadians);
}

void MLineEntity::scale(const Point2D& center, double factor) {
    for (Point2D& v : m_vertices) v = scaleAround(v, center, factor);
    m_scale *= std::abs(factor);
}

void MLineEntity::mirror(const Point2D& a, const Point2D& b) {
    for (Point2D& v : m_vertices) v = mirrorAcross(v, a, b);
    // Mirroring flips handedness, so every element's signed (left-of-
    // travel) offset must flip sign to keep pointing at the same physical
    // side of the reflected centerline.
    for (MLineElement& el : m_elements) el.offset = -el.offset;
    std::reverse(m_vertices.begin(), m_vertices.end());
}

std::vector<Point2D> MLineEntity::gripPoints() const { return m_vertices; }

void MLineEntity::moveGripPoint(std::size_t index, const Point2D& newPos) {
    if (index < m_vertices.size()) m_vertices[index] = newPos;
}

std::vector<SnapPoint> MLineEntity::snapCandidates() const {
    std::vector<SnapPoint> result;
    for (const Point2D& v : m_vertices) result.push_back({v, SnapKind::Endpoint});
    return result;
}

std::unique_ptr<Entity> MLineEntity::clone() const { return std::make_unique<MLineEntity>(*this); }

} // namespace lcad
