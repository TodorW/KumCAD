#include "core/geometry/JoinOps.h"

#include "core/geometry/Arc.h"
#include "core/geometry/Circle.h"
#include "core/geometry/Line.h"
#include "core/geometry/PolylineOps.h"

#include <algorithm>
#include <cmath>

namespace lcad {

namespace {

double normalizeAngle(double angle) {
    constexpr double kTwoPi = 2.0 * M_PI;
    angle = std::fmod(angle, kTwoPi);
    if (angle < 0) angle += kTwoPi;
    return angle;
}

// All-Line, all-collinear case: merges into the single Line spanning every
// input's projected extent along their shared direction, provided the
// projected intervals chain with no gap wider than tol.
std::unique_ptr<Entity> tryJoinLines(EntityId newId, LayerId layer, const std::vector<const Entity*>& parts,
                                     double tol) {
    std::vector<const LineEntity*> lines;
    for (const Entity* e : parts) {
        if (e->type() != EntityType::Line) return nullptr;
        lines.push_back(static_cast<const LineEntity*>(e));
    }

    const LineEntity* longest = lines.front();
    for (const LineEntity* l : lines) {
        if (l->start().distanceTo(l->end()) > longest->start().distanceTo(longest->end())) longest = l;
    }
    const double longestLen = longest->start().distanceTo(longest->end());
    if (longestLen < 1e-12) return nullptr;
    const Point2D dir = (longest->end() - longest->start()) * (1.0 / longestLen);
    const Point2D refPoint = longest->start();

    struct Interval {
        double lo;
        double hi;
    };
    std::vector<Interval> intervals;
    for (const LineEntity* l : lines) {
        const Point2D v = l->end() - l->start();
        const double len = v.length();
        if (len < 1e-12) return nullptr;
        const Point2D u = v * (1.0 / len);
        const double cross = dir.x * u.y - dir.y * u.x;
        if (std::abs(cross) > 1e-9) return nullptr; // not parallel to the reference line

        const Point2D toStart = l->start() - refPoint;
        const double perp = std::abs(toStart.x * dir.y - toStart.y * dir.x);
        if (perp > tol) return nullptr; // parallel but offset onto a different infinite line

        const double tStart = (l->start() - refPoint).dot(dir);
        const double tEnd = (l->end() - refPoint).dot(dir);
        intervals.push_back({std::min(tStart, tEnd), std::max(tStart, tEnd)});
    }

    std::sort(intervals.begin(), intervals.end(), [](const Interval& a, const Interval& b) { return a.lo < b.lo; });
    double lo = intervals.front().lo;
    double hi = intervals.front().hi;
    for (std::size_t i = 1; i < intervals.size(); ++i) {
        if (intervals[i].lo > hi + tol) return nullptr; // gap: these don't actually connect
        hi = std::max(hi, intervals[i].hi);
    }

    return std::make_unique<LineEntity>(newId, layer, refPoint + dir * lo, refPoint + dir * hi);
}

// All-Arc, all-on-the-same-circle case: chains the sweeps in angular order
// and merges into one Arc, or a Circle if every input arc chains into a
// full 360-degree loop.
std::unique_ptr<Entity> tryJoinArcs(EntityId newId, LayerId layer, const std::vector<const Entity*>& parts,
                                    double tol) {
    std::vector<const ArcEntity*> arcs;
    for (const Entity* e : parts) {
        if (e->type() != EntityType::Arc) return nullptr;
        arcs.push_back(static_cast<const ArcEntity*>(e));
    }

    const Point2D center = arcs.front()->center();
    const double radius = arcs.front()->radius();
    if (radius < 1e-9) return nullptr;
    for (const ArcEntity* a : arcs) {
        if (a->center().distanceTo(center) > tol) return nullptr;
        if (std::abs(a->radius() - radius) > tol) return nullptr;
    }

    const double angTol = std::max(tol / radius, 1e-9);
    struct Sweep {
        double start;
        double end;
    };
    std::vector<Sweep> sweeps;
    for (const ArcEntity* a : arcs) sweeps.push_back({normalizeAngle(a->startAngle()), normalizeAngle(a->endAngle())});
    std::sort(sweeps.begin(), sweeps.end(), [](const Sweep& a, const Sweep& b) { return a.start < b.start; });

    const double chainStart = sweeps.front().start;
    double chainEnd = sweeps.front().end;
    for (std::size_t i = 1; i < sweeps.size(); ++i) {
        const double gap = normalizeAngle(sweeps[i].start - chainEnd);
        if (gap > angTol && gap < (2.0 * M_PI - angTol)) return nullptr; // doesn't touch the chain so far
        chainEnd = sweeps[i].end;
    }

    const double closeGap = normalizeAngle(chainStart - chainEnd);
    if (closeGap <= angTol || closeGap >= (2.0 * M_PI - angTol)) {
        return std::make_unique<CircleEntity>(newId, layer, center, radius);
    }
    return std::make_unique<ArcEntity>(newId, layer, center, radius, chainStart, chainEnd);
}

} // namespace

std::unique_ptr<Entity> joinEntities(EntityId newId, LayerId layer, const std::vector<const Entity*>& parts,
                                     double tol) {
    std::vector<const Entity*> valid;
    for (const Entity* e : parts) {
        if (e) valid.push_back(e);
    }
    if (valid.size() < 2) return nullptr;

    if (auto result = tryJoinLines(newId, layer, valid, tol)) return result;
    if (auto result = tryJoinArcs(newId, layer, valid, tol)) return result;
    return joinToPolyline(newId, layer, valid, tol);
}

} // namespace lcad
