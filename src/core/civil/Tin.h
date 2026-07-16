#pragma once

#include "core/civil/SurveyPoints.h"

#include <array>
#include <optional>
#include <vector>

namespace lcad {

// A triangulated irregular network: points plus a Delaunay triangulation
// over their (x,y), each triangle storing indices into points.
struct Tin {
    std::vector<SurveyPoint> points;
    std::vector<std::array<int, 3>> triangles;
};

// Builds a TIN via the Bowyer-Watson incremental algorithm (a real,
// from-scratch Delaunay triangulation -- no external geometry library is
// available on this system, the same build-vs-vendor call this codebase
// has made before for other bounded algorithmic tools). No spatial index
// backs point insertion, so this is O(n^2)-ish: fine for typical survey
// point counts (a few hundred to a couple thousand), not for a raw LiDAR
// scan's tens of thousands of points -- decimate first if needed (see
// core/io/PointCloudFile.h's own maxPoints parameter for the same idea).
Tin buildTin(std::vector<SurveyPoint> points);

// Barycentric-interpolated elevation at xy, or nullopt if xy falls outside
// the TIN's convex hull (no extrapolation).
std::optional<double> elevationAt(const Tin& tin, const Point2D& xy);

} // namespace lcad
