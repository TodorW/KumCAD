#include "core/geometry/Hatch.h"

#include <algorithm>
#include <cmath>
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

// Chops the span [t0, t1] of one pattern line into pen-down pieces following
// the dash elements (alternating down/up, dot = short tick), with the phase
// anchored at the line's parameter origin so dashes align between lines.
void appendDashedSpan(std::vector<std::pair<Point2D, Point2D>>& out, const Point2D& origin, const Point2D& dir,
                      double t0, double t1, const std::vector<double>& dashes, double scale) {
    double period = 0.0;
    for (double d : dashes) period += std::abs(d) * scale;
    if (period < 1e-9) {
        out.emplace_back(origin + dir * t0, origin + dir * t1);
        return;
    }
    double pos = t0 - std::fmod(t0, period);
    if (pos > t0) pos -= period;
    while (pos < t1) {
        double cursor = pos;
        for (double d : dashes) {
            const double len = std::abs(d) * scale;
            const double downLen = d > 0 ? len : (d == 0 ? scale * 0.01 : 0.0); // dot = tiny tick
            if (downLen > 0) {
                const double a = std::max(cursor, t0);
                const double b = std::min(cursor + downLen, t1);
                if (b > a) out.emplace_back(origin + dir * a, origin + dir * b);
            }
            cursor += d == 0 ? 0.0 : len;
        }
        pos += period;
    }
}

} // namespace

std::vector<std::pair<Point2D, Point2D>> HatchEntity::patternSegments() const {
    std::vector<std::pair<Point2D, Point2D>> out;
    if (m_pattern == HatchPattern::Solid || m_vertices.size() < 3 || m_patternScale < 1e-9) return out;
    constexpr std::size_t kMaxSegments = 20000;

    for (const HatchPatternLine& family : hatchPatternLines(m_pattern)) {
        const double angle = family.angleDeg * M_PI / 180.0 + m_patternAngle;
        const Point2D dir(std::cos(angle), std::sin(angle));
        const Point2D normal(-dir.y, dir.x);
        // The whole .pat definition rotates by the hatch angle; the per-line
        // offset is expressed relative to the (rotated) line direction.
        const Point2D base = rotateAround(family.base * m_patternScale, Point2D(), m_patternAngle);
        const Point2D disp = rotateAround(family.offset * m_patternScale, Point2D(), angle);
        const double spacing = family.offset.y * m_patternScale; // disp component along normal
        if (std::abs(spacing) < 1e-9) continue;

        double lo = std::numeric_limits<double>::max();
        double hi = std::numeric_limits<double>::lowest();
        for (const Point2D& v : m_vertices) {
            const double p = v.dot(normal);
            lo = std::min(lo, p);
            hi = std::max(hi, p);
        }
        const double basePos = base.dot(normal);
        long kLo = static_cast<long>(std::floor((lo - basePos) / spacing)) - 1;
        long kHi = static_cast<long>(std::ceil((hi - basePos) / spacing)) + 1;
        if (spacing < 0) std::swap(kLo, kHi);

        for (long k = kLo; k <= kHi; ++k) {
            const Point2D origin = base + disp * static_cast<double>(k);
            // Intersect the infinite line origin + t*dir with every boundary
            // edge; sorted parameter pairs are the inside spans (even-odd).
            std::vector<double> ts;
            const std::size_t n = m_vertices.size();
            for (std::size_t i = 0; i < n; ++i) {
                const Point2D& a = m_vertices[i];
                const Point2D& b = m_vertices[(i + 1) % n];
                const Point2D e = b - a;
                const double denom = dir.x * e.y - dir.y * e.x;
                if (std::abs(denom) < 1e-12) continue;
                const Point2D ao = a - origin;
                const double u = (ao.x * dir.y - ao.y * dir.x) / denom;
                if (u < 0.0 || u >= 1.0) continue; // half-open: shared vertices count once
                ts.push_back((ao.x * e.y - ao.y * e.x) / denom);
            }
            std::sort(ts.begin(), ts.end());
            for (std::size_t i = 0; i + 1 < ts.size(); i += 2) {
                if (ts[i + 1] - ts[i] < 1e-9) continue;
                if (family.dashes.empty()) {
                    out.emplace_back(origin + dir * ts[i], origin + dir * ts[i + 1]);
                } else {
                    appendDashedSpan(out, origin, dir, ts[i], ts[i + 1], family.dashes, m_patternScale);
                }
                if (out.size() >= kMaxSegments) return out;
            }
        }
    }
    return out;
}

bool HatchEntity::containsPoint(const Point2D& pt) const {
    bool inside = false;
    const std::size_t n = m_vertices.size();
    for (std::size_t i = 0, j = n - 1; i < n; j = i++) {
        const Point2D& vi = m_vertices[i];
        const Point2D& vj = m_vertices[j];
        if ((vi.y > pt.y) != (vj.y > pt.y) &&
            pt.x < (vj.x - vi.x) * (pt.y - vi.y) / (vj.y - vi.y) + vi.x) {
            inside = !inside;
        }
    }
    return inside;
}

BoundingBox HatchEntity::boundingBox() const {
    BoundingBox box;
    for (const auto& v : m_vertices) box.expand(v);
    return box;
}

double HatchEntity::distanceTo(const Point2D& pt) const {
    if (m_vertices.size() >= 3 && containsPoint(pt)) return 0.0;
    double best = std::numeric_limits<double>::max();
    const std::size_t n = m_vertices.size();
    for (std::size_t i = 0; i < n; ++i) {
        best = std::min(best, distanceToSegment(pt, m_vertices[i], m_vertices[(i + 1) % n]));
    }
    return best;
}

void HatchEntity::translate(const Point2D& delta) {
    for (auto& v : m_vertices) v = v + delta;
}

void HatchEntity::rotate(const Point2D& center, double angleRadians) {
    for (auto& v : m_vertices) v = rotateAround(v, center, angleRadians);
    m_patternAngle += angleRadians;
}

void HatchEntity::scale(const Point2D& center, double factor) {
    for (auto& v : m_vertices) v = scaleAround(v, center, factor);
    m_patternScale *= std::abs(factor);
}

void HatchEntity::mirror(const Point2D& a, const Point2D& b) {
    for (auto& v : m_vertices) v = mirrorAcross(v, a, b);
    // Reflect the pattern direction across the mirror axis.
    const Point2D d = b - a;
    if (d.dot(d) > 1e-12) {
        const double axisAngle = std::atan2(d.y, d.x);
        m_patternAngle = 2.0 * axisAngle - m_patternAngle;
    }
}

std::vector<Point2D> HatchEntity::gripPoints() const {
    return m_vertices;
}

void HatchEntity::moveGripPoint(std::size_t index, const Point2D& newPos) {
    if (index < m_vertices.size()) m_vertices[index] = newPos;
}

std::vector<SnapPoint> HatchEntity::snapCandidates() const {
    std::vector<SnapPoint> result;
    for (const auto& v : m_vertices) result.push_back({v, SnapKind::Endpoint});
    return result;
}

std::unique_ptr<Entity> HatchEntity::clone() const {
    return std::make_unique<HatchEntity>(*this);
}

} // namespace lcad
