#pragma once

#include "core/civil/Tin.h"

#include <vector>

namespace lcad {

// Computes the ground profile along a horizontal alignment (a road
// centerline, given as its 2D polyline vertices): walks
// the alignment every interval of station (cumulative distance), sampling
// tin's elevation at each point, and returns (station, elevation) pairs
// ready to draw as a profile-view polyline (x = station, y = elevation --
// a different coordinate space than the plan drawing, same idea as any
// profile/section view). Stations outside tin's convex hull are skipped
// rather than extrapolated, so a profile can have gaps where the survey
// doesn't reach.
std::vector<Point2D> computeGroundProfile(const std::vector<Point2D>& alignment, const Tin& tin, double interval);

} // namespace lcad
