#include "core/geometry/Region.h"

#include <algorithm>
#include <limits>

namespace lcad {

namespace {

constexpr double kEps = 1e-9;

double distanceToSegment(const Point2D& pt, const Point2D& a, const Point2D& b) {
    const Point2D seg = b - a;
    const double lenSq = seg.dot(seg);
    if (lenSq < 1e-12) return pt.distanceTo(a);
    double t = (pt - a).dot(seg) / lenSq;
    t = std::clamp(t, 0.0, 1.0);
    return pt.distanceTo(a + seg * t);
}

double signedAreaOf(const std::vector<Point2D>& v) {
    double area = 0.0;
    const std::size_t n = v.size();
    for (std::size_t i = 0; i < n; ++i) {
        const Point2D& p0 = v[i];
        const Point2D& p1 = v[(i + 1) % n];
        area += p0.x * p1.y - p1.x * p0.y;
    }
    return area * 0.5;
}

bool pointInPolygon(const Point2D& pt, const std::vector<Point2D>& poly) {
    bool inside = false;
    const std::size_t n = poly.size();
    for (std::size_t i = 0, j = n - 1; i < n; j = i++) {
        const Point2D& vi = poly[i];
        const Point2D& vj = poly[j];
        if ((vi.y > pt.y) != (vj.y > pt.y) && pt.x < (vj.x - vi.x) * (pt.y - vi.y) / (vj.y - vi.y) + vi.x) {
            inside = !inside;
        }
    }
    return inside;
}

std::vector<Point2D> ensureCCW(std::vector<Point2D> v) {
    if (signedAreaOf(v) < 0.0) std::reverse(v.begin(), v.end());
    return v;
}

// Parametric intersection of segments (a1,a2) and (b1,b2): sets t,u in
// (0,1) (the open interval -- exact vertex touches are excluded, a
// disclosed clipper limitation, see Region.h) and returns true iff they
// cross at a single interior point. Parallel (including collinear-
// overlapping) segments return false -- another disclosed limitation.
bool segSegParams(const Point2D& a1, const Point2D& a2, const Point2D& b1, const Point2D& b2, double& t, double& u) {
    const Point2D r = a2 - a1;
    const Point2D s = b2 - b1;
    const double denom = r.x * s.y - r.y * s.x;
    if (std::abs(denom) < kEps) return false;
    const Point2D diff = b1 - a1;
    t = (diff.x * s.y - diff.y * s.x) / denom;
    u = (diff.x * r.y - diff.y * r.x) / denom;
    return t > kEps && t < 1.0 - kEps && u > kEps && u < 1.0 - kEps;
}

struct Event {
    std::size_t edgeA;
    double tA;
    std::size_t edgeB;
    double tB;
    Point2D pt;
};

struct GHVertex {
    Point2D pt;
    bool intersect = false;
    bool entry = false;
    bool visited = false;
    std::size_t neighbor = 0; // valid iff intersect
};

RegionLoop makeLoop(std::vector<Point2D> v, bool asHole) {
    // Outer loops are stored CCW, holes CW -- RegionLoop::isHole() derives
    // its classification from this, so orient on the way out rather than
    // storing a separate flag.
    v = ensureCCW(std::move(v));
    if (asHole) std::reverse(v.begin(), v.end());
    return RegionLoop{std::move(v)};
}

std::vector<GHVertex> buildList(const std::vector<Point2D>& poly, const std::vector<Event>& events, bool isListA) {
    std::vector<GHVertex> out;
    const std::size_t n = poly.size();
    for (std::size_t i = 0; i < n; ++i) {
        out.push_back({poly[i], false, false, false, 0});
        std::vector<const Event*> onThisEdge;
        for (const Event& e : events) {
            if ((isListA && e.edgeA == i) || (!isListA && e.edgeB == i)) onThisEdge.push_back(&e);
        }
        std::sort(onThisEdge.begin(), onThisEdge.end(), [isListA](const Event* x, const Event* y) {
            return (isListA ? x->tA : x->tB) < (isListA ? y->tA : y->tB);
        });
        for (const Event* e : onThisEdge) out.push_back({e->pt, true, false, false, 0});
    }
    return out;
}

// Walks the boundary of the requested boolean op starting at
// listA[startIdx], switching lists at every intersection vertex, until
// back at the start. At each intersection vertex the direction to
// continue (forward or backward through the CURRENT list) is:
//   Intersect: forward iff the vertex's own entry flag is true, in
//              whichever list we're presently in.
//   Union:     forward iff the vertex's own entry flag is FALSE, in
//              whichever list we're presently in (the mirror image of
//              Intersect's rule).
//   Subtract:  asymmetric per list -- forward iff !entry while in A (like
//              Union's rule), forward iff entry while in B (like
//              Intersect's rule). This carves B's boundary out of A: B's
//              contribution is traversed with the *opposite* handedness
//              from A's, which is what turns a shared boundary into a cut
//              rather than a union seam. All three rules, and the start
//              condition regionBoolean chooses per op, were derived and
//              hand-verified against a concrete overlapping-squares case
//              (see the test suite) rather than assumed from memory.
std::vector<Point2D> traceLoop(std::vector<GHVertex>& listA, std::vector<GHVertex>& listB, std::size_t startIdx,
                               RegionBoolOp op) {
    std::vector<Point2D> loop;
    const Point2D startPt = listA[startIdx].pt;
    std::vector<GHVertex>* cur = &listA;
    std::vector<GHVertex>* other = &listB;
    bool onA = true;
    std::size_t idx = startIdx;
    (*cur)[idx].visited = true;
    loop.push_back((*cur)[idx].pt);
    while (true) {
        const GHVertex& v = (*cur)[idx];
        bool forward;
        if (op == RegionBoolOp::Intersect) {
            forward = v.entry;
        } else if (op == RegionBoolOp::Union) {
            forward = !v.entry;
        } else {
            forward = onA ? !v.entry : v.entry;
        }
        do {
            idx = forward ? (idx + 1) % cur->size() : (idx == 0 ? cur->size() - 1 : idx - 1);
            GHVertex& w = (*cur)[idx];
            w.visited = true;
            loop.push_back(w.pt);
        } while (!(*cur)[idx].intersect);
        if ((*cur)[idx].pt.distanceTo(startPt) < kEps) break;
        const std::size_t nbr = (*cur)[idx].neighbor;
        (*other)[nbr].visited = true;
        std::swap(cur, other);
        onA = !onA;
        idx = nbr;
    }
    if (loop.size() > 1 && loop.back().distanceTo(loop.front()) < kEps) loop.pop_back();
    return loop;
}

} // namespace

double RegionLoop::signedArea() const { return signedAreaOf(vertices); }

std::vector<RegionLoop> regionBoolean(const std::vector<Point2D>& aIn, const std::vector<Point2D>& bIn,
                                      RegionBoolOp op) {
    if (aIn.size() < 3 || bIn.size() < 3) return {};
    std::vector<Point2D> a = ensureCCW(aIn);
    std::vector<Point2D> b = ensureCCW(bIn);

    std::vector<Event> events;
    const std::size_t nA = a.size(), nB = b.size();
    for (std::size_t i = 0; i < nA; ++i) {
        const Point2D& a1 = a[i];
        const Point2D& a2 = a[(i + 1) % nA];
        for (std::size_t j = 0; j < nB; ++j) {
            const Point2D& b1 = b[j];
            const Point2D& b2 = b[(j + 1) % nB];
            double t, u;
            if (segSegParams(a1, a2, b1, b2, t, u)) {
                events.push_back({i, t, j, u, a1 + (a2 - a1) * t});
            }
        }
    }

    if (events.empty()) {
        const bool aInB = pointInPolygon(a.front(), b);
        const bool bInA = pointInPolygon(b.front(), a);
        switch (op) {
        case RegionBoolOp::Union:
            if (aInB) return {makeLoop(b, false)};
            if (bInA) return {makeLoop(a, false)};
            return {makeLoop(a, false), makeLoop(b, false)};
        case RegionBoolOp::Intersect:
            if (aInB) return {makeLoop(a, false)};
            if (bInA) return {makeLoop(b, false)};
            return {};
        case RegionBoolOp::Subtract:
            if (bInA) return {makeLoop(a, false), makeLoop(b, true)};
            if (aInB) return {};
            return {makeLoop(a, false)};
        }
    }

    std::vector<GHVertex> listA = buildList(a, events, true);
    std::vector<GHVertex> listB = buildList(b, events, false);

    // Link each event's two insertion points (found independently while
    // building listA/listB) as neighbors of one another. The two lists
    // group intersection vertices by different keys (edgeA vs edgeB), so
    // match by point identity, unambiguous since each event produced one
    // distinct point (barring true duplicate crossings, a disclosed edge
    // case).
    {
        std::vector<std::size_t> aIntersectIdx, bIntersectIdx;
        for (std::size_t k = 0; k < listA.size(); ++k)
            if (listA[k].intersect) aIntersectIdx.push_back(k);
        for (std::size_t k = 0; k < listB.size(); ++k)
            if (listB[k].intersect) bIntersectIdx.push_back(k);
        for (std::size_t ea = 0; ea < aIntersectIdx.size(); ++ea) {
            for (std::size_t eb = 0; eb < bIntersectIdx.size(); ++eb) {
                if (listA[aIntersectIdx[ea]].pt.distanceTo(listB[bIntersectIdx[eb]].pt) < 1e-6) {
                    listA[aIntersectIdx[ea]].neighbor = bIntersectIdx[eb];
                    listB[bIntersectIdx[eb]].neighbor = aIntersectIdx[ea];
                    break;
                }
            }
        }
    }

    // Entry/exit marking: alternate starting from each list's first
    // (non-intersection) vertex's known inside/outside status.
    {
        bool inside = pointInPolygon(listA.front().pt, b);
        for (GHVertex& v : listA) {
            if (v.intersect) {
                v.entry = !inside;
                inside = !inside;
            }
        }
    }
    {
        bool inside = pointInPolygon(listB.front().pt, a);
        for (GHVertex& v : listB) {
            if (v.intersect) {
                v.entry = !inside;
                inside = !inside;
            }
        }
    }

    // Start condition: Intersect traces start where A enters B (entry);
    // Union and Subtract both start where A exits B, but see traceLoop's
    // own per-operation direction rule for why Subtract's B-side traversal
    // still ends up carving out the right region instead of reproducing
    // Union.
    std::vector<RegionLoop> result;
    const bool wantEntry = (op == RegionBoolOp::Intersect);
    for (std::size_t i = 0; i < listA.size(); ++i) {
        if (!listA[i].intersect || listA[i].visited) continue;
        if (listA[i].entry != wantEntry) continue;
        std::vector<Point2D> loop = traceLoop(listA, listB, i, op);
        if (loop.size() >= 3) result.push_back(makeLoop(loop, false));
    }
    return result;
}

std::unique_ptr<RegionEntity> RegionEntity::booleanOp(EntityId newId, LayerId layer, const RegionEntity& a,
                                                      const RegionEntity& b, RegionBoolOp op) {
    if (a.m_loops.size() != 1 || b.m_loops.size() != 1) return nullptr;
    std::vector<RegionLoop> loops = regionBoolean(a.m_loops.front().vertices, b.m_loops.front().vertices, op);
    if (loops.empty()) return nullptr;
    return std::make_unique<RegionEntity>(newId, layer, std::move(loops));
}

RegionEntity::RegionEntity(EntityId id, LayerId layer, std::vector<RegionLoop> loops)
    : Entity(id, layer), m_loops(std::move(loops)) {}

double RegionEntity::area() const {
    double total = 0.0;
    for (const RegionLoop& loop : m_loops) total += std::abs(loop.signedArea());
    return total;
}

bool RegionEntity::containsPoint(const Point2D& pt) const {
    int count = 0;
    for (const RegionLoop& loop : m_loops) {
        if (pointInPolygon(pt, loop.vertices)) ++count;
    }
    return (count % 2) == 1;
}

BoundingBox RegionEntity::boundingBox() const {
    BoundingBox box;
    for (const RegionLoop& loop : m_loops)
        for (const Point2D& v : loop.vertices) box.expand(v);
    return box;
}

double RegionEntity::distanceTo(const Point2D& pt) const {
    if (containsPoint(pt)) return 0.0;
    double best = std::numeric_limits<double>::max();
    for (const RegionLoop& loop : m_loops) {
        const std::size_t n = loop.vertices.size();
        for (std::size_t i = 0; i < n; ++i)
            best = std::min(best, distanceToSegment(pt, loop.vertices[i], loop.vertices[(i + 1) % n]));
    }
    return best;
}

void RegionEntity::translate(const Point2D& delta) {
    for (RegionLoop& loop : m_loops)
        for (Point2D& v : loop.vertices) v = v + delta;
}

void RegionEntity::rotate(const Point2D& center, double angleRadians) {
    for (RegionLoop& loop : m_loops)
        for (Point2D& v : loop.vertices) v = rotateAround(v, center, angleRadians);
}

void RegionEntity::scale(const Point2D& center, double factor) {
    for (RegionLoop& loop : m_loops)
        for (Point2D& v : loop.vertices) v = scaleAround(v, center, factor);
}

void RegionEntity::mirror(const Point2D& a, const Point2D& b) {
    for (RegionLoop& loop : m_loops) {
        for (Point2D& v : loop.vertices) v = mirrorAcross(v, a, b);
        std::reverse(loop.vertices.begin(), loop.vertices.end()); // mirroring flips winding
    }
}

std::vector<Point2D> RegionEntity::gripPoints() const {
    std::vector<Point2D> pts;
    for (const RegionLoop& loop : m_loops)
        for (const Point2D& v : loop.vertices) pts.push_back(v);
    return pts;
}

void RegionEntity::moveGripPoint(std::size_t index, const Point2D& newPos) {
    for (RegionLoop& loop : m_loops) {
        if (index < loop.vertices.size()) {
            loop.vertices[index] = newPos;
            return;
        }
        index -= loop.vertices.size();
    }
}

std::vector<SnapPoint> RegionEntity::snapCandidates() const {
    std::vector<SnapPoint> result;
    for (const RegionLoop& loop : m_loops)
        for (const Point2D& v : loop.vertices) result.push_back({v, SnapKind::Endpoint});
    return result;
}

std::unique_ptr<Entity> RegionEntity::clone() const { return std::make_unique<RegionEntity>(*this); }

} // namespace lcad
