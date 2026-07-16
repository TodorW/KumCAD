#include "core/civil/Contours.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>

namespace lcad {

namespace {

constexpr double kEpsilon = 1e-6;

bool coincident(const Point2D& a, const Point2D& b) { return a.distanceTo(b) < kEpsilon; }

std::vector<std::vector<Point2D>> chainSegments(std::vector<std::pair<Point2D, Point2D>> segments) {
    std::vector<std::vector<Point2D>> chains;
    std::vector<bool> used(segments.size(), false);

    for (std::size_t start = 0; start < segments.size(); ++start) {
        if (used[start]) continue;
        used[start] = true;
        std::vector<Point2D> chain = {segments[start].first, segments[start].second};

        bool extended = true;
        while (extended) {
            extended = false;
            for (std::size_t i = 0; i < segments.size(); ++i) {
                if (used[i]) continue;
                if (coincident(chain.back(), segments[i].first)) {
                    chain.push_back(segments[i].second);
                    used[i] = true;
                    extended = true;
                } else if (coincident(chain.back(), segments[i].second)) {
                    chain.push_back(segments[i].first);
                    used[i] = true;
                    extended = true;
                } else if (coincident(chain.front(), segments[i].first)) {
                    chain.insert(chain.begin(), segments[i].second);
                    used[i] = true;
                    extended = true;
                } else if (coincident(chain.front(), segments[i].second)) {
                    chain.insert(chain.begin(), segments[i].first);
                    used[i] = true;
                    extended = true;
                }
            }
        }
        chains.push_back(std::move(chain));
    }
    return chains;
}

} // namespace

std::vector<std::vector<Point2D>> computeContours(const Tin& tin, double interval) {
    if (interval <= 1e-9 || tin.triangles.empty()) return {};

    double minZ = std::numeric_limits<double>::infinity();
    double maxZ = -minZ;
    for (const auto& p : tin.points) {
        minZ = std::min(minZ, p.z);
        maxZ = std::max(maxZ, p.z);
    }

    std::map<double, std::vector<std::pair<Point2D, Point2D>>> segmentsByLevel;
    for (double level = std::ceil(minZ / interval) * interval; level <= maxZ + 1e-9; level += interval) {
        for (const auto& tri : tin.triangles) {
            const SurveyPoint pts[3] = {tin.points[static_cast<std::size_t>(tri[0])],
                                        tin.points[static_cast<std::size_t>(tri[1])],
                                        tin.points[static_cast<std::size_t>(tri[2])]};
            std::vector<Point2D> crossings;
            for (int i = 0; i < 3; ++i) {
                const SurveyPoint& p1 = pts[i];
                const SurveyPoint& p2 = pts[(i + 1) % 3];
                if ((p1.z - level) * (p2.z - level) < 0.0) {
                    const double t = (level - p1.z) / (p2.z - p1.z);
                    crossings.push_back(p1.xy + (p2.xy - p1.xy) * t);
                }
            }
            if (crossings.size() == 2) segmentsByLevel[level].emplace_back(crossings[0], crossings[1]);
        }
    }

    std::vector<std::vector<Point2D>> result;
    for (auto& [level, segments] : segmentsByLevel) {
        (void)level;
        for (auto& chain : chainSegments(std::move(segments))) {
            if (chain.size() >= 2) result.push_back(std::move(chain));
        }
    }
    return result;
}

} // namespace lcad
