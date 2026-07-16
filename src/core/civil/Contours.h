#pragma once

#include "core/civil/Tin.h"

#include <vector>

namespace lcad {

// Extracts contour lines from tin at every multiple of interval between its
// minimum and maximum elevation, as chained polylines (each a vertex list
// ready for a PolylineEntity). Per level: each triangle whose edges the
// level crosses contributes one linearly-interpolated segment, then
// segments are chained by endpoint coincidence into as few polylines as a
// simple greedy walk finds -- a real contouring engine handles saddle
// points and closed-loop merging more precisely; this is a disclosed
// approximation, not exact for every terrain shape.
std::vector<std::vector<Point2D>> computeContours(const Tin& tin, double interval);

} // namespace lcad
