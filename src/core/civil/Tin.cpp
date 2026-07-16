#include "core/civil/Tin.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace lcad {

namespace {

struct Tri {
    int a, b, c;
};

// Standard in-circle predicate: true if p lies inside the circumcircle of
// (a, b, c), which must be given in CCW order (checked/fixed internally).
bool inCircumcircle(Point2D a, Point2D b, Point2D c, const Point2D& p) {
    const double orient = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    if (orient < 0) std::swap(b, c);

    const double ax = a.x - p.x, ay = a.y - p.y;
    const double bx = b.x - p.x, by = b.y - p.y;
    const double cx = c.x - p.x, cy = c.y - p.y;
    const double det = (ax * ax + ay * ay) * (bx * cy - cx * by) - (bx * bx + by * by) * (ax * cy - cx * ay) +
                       (cx * cx + cy * cy) * (ax * by - bx * ay);
    return det > 1e-9;
}

} // namespace

Tin buildTin(std::vector<SurveyPoint> points) {
    Tin tin;
    tin.points = points;
    if (points.size() < 3) return tin;

    double minX = std::numeric_limits<double>::infinity(), minY = minX;
    double maxX = -minX, maxY = -minX;
    for (const auto& p : points) {
        minX = std::min(minX, p.xy.x);
        minY = std::min(minY, p.xy.y);
        maxX = std::max(maxX, p.xy.x);
        maxY = std::max(maxY, p.xy.y);
    }
    const double deltaMax = std::max(maxX - minX, maxY - minY) + 10.0;
    const Point2D mid((minX + maxX) / 2.0, (minY + maxY) / 2.0);

    std::vector<SurveyPoint> allPoints = points;
    const int superBase = static_cast<int>(allPoints.size());
    allPoints.push_back({Point2D(mid.x - 20 * deltaMax, mid.y - deltaMax), 0.0});
    allPoints.push_back({Point2D(mid.x, mid.y + 20 * deltaMax), 0.0});
    allPoints.push_back({Point2D(mid.x + 20 * deltaMax, mid.y - deltaMax), 0.0});

    std::vector<Tri> triangles = {{superBase, superBase + 1, superBase + 2}};

    for (int pi = 0; pi < superBase; ++pi) {
        const Point2D& p = allPoints[static_cast<std::size_t>(pi)].xy;

        std::vector<Tri> badTriangles;
        for (const Tri& t : triangles) {
            if (inCircumcircle(allPoints[static_cast<std::size_t>(t.a)].xy, allPoints[static_cast<std::size_t>(t.b)].xy,
                               allPoints[static_cast<std::size_t>(t.c)].xy, p)) {
                badTriangles.push_back(t);
            }
        }

        std::vector<std::pair<int, int>> edges;
        for (const Tri& t : badTriangles) {
            edges.emplace_back(t.a, t.b);
            edges.emplace_back(t.b, t.c);
            edges.emplace_back(t.c, t.a);
        }
        std::vector<std::pair<int, int>> boundary;
        for (std::size_t i = 0; i < edges.size(); ++i) {
            bool shared = false;
            for (std::size_t j = 0; j < edges.size(); ++j) {
                if (i == j) continue;
                if (edges[i].first == edges[j].second && edges[i].second == edges[j].first) {
                    shared = true;
                    break;
                }
            }
            if (!shared) boundary.push_back(edges[i]);
        }

        std::vector<Tri> kept;
        for (const Tri& t : triangles) {
            const bool isBad = std::any_of(badTriangles.begin(), badTriangles.end(), [&](const Tri& bt) {
                return t.a == bt.a && t.b == bt.b && t.c == bt.c;
            });
            if (!isBad) kept.push_back(t);
        }
        triangles = std::move(kept);
        for (const auto& [u, v] : boundary) triangles.push_back({u, v, pi});
    }

    for (const Tri& t : triangles) {
        if (t.a >= superBase || t.b >= superBase || t.c >= superBase) continue;
        tin.triangles.push_back({t.a, t.b, t.c});
    }
    return tin;
}

std::optional<double> elevationAt(const Tin& tin, const Point2D& xy) {
    for (const auto& tri : tin.triangles) {
        const Point2D& a = tin.points[static_cast<std::size_t>(tri[0])].xy;
        const Point2D& b = tin.points[static_cast<std::size_t>(tri[1])].xy;
        const Point2D& c = tin.points[static_cast<std::size_t>(tri[2])].xy;

        const double denom = (b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y);
        if (std::abs(denom) < 1e-12) continue;
        const double w1 = ((b.y - c.y) * (xy.x - c.x) + (c.x - b.x) * (xy.y - c.y)) / denom;
        const double w2 = ((c.y - a.y) * (xy.x - c.x) + (a.x - c.x) * (xy.y - c.y)) / denom;
        const double w3 = 1.0 - w1 - w2;
        if (w1 >= -1e-9 && w2 >= -1e-9 && w3 >= -1e-9) {
            return w1 * tin.points[static_cast<std::size_t>(tri[0])].z + w2 * tin.points[static_cast<std::size_t>(tri[1])].z +
                   w3 * tin.points[static_cast<std::size_t>(tri[2])].z;
        }
    }
    return std::nullopt;
}

} // namespace lcad
