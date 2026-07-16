#pragma once

#include "core/cam/Toolpath.h"

#include <string>

namespace lcad {

// Writes toolpath as G-code: G21/G90 header, rapid to the start point,
// plunge to -cutDepth at plungeRate, G1 moves along every vertex at
// feedRate, retract to safeHeight, M30 end. One pass at the full requested
// depth -- no stepped multi-pass roughing for thick material yet. This is
// a plain, common-subset G-code dialect (no tool-change codes, no arcs --
// toolpath is already a flattened polyline), not tuned to any specific
// controller's quirks. Returns false (with *errorOut set, if provided) on
// a file-open failure, or if toolpath has fewer than 2 points.
bool writeGCode(const std::vector<Point2D>& toolpath, const ToolpathParams& params, const std::string& path,
                std::string* errorOut = nullptr);

} // namespace lcad
