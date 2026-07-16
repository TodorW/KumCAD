#pragma once

#include "core/geometry/Point2D.h"

#include <string>
#include <vector>

namespace lcad {

// An (x, y, z) survey point. KumCAD's point-cloud readers (core/io/
// PointCloudFile.h) deliberately discard Z everywhere else in the app --
// "KumCAD is a 2D system throughout" -- but a TIN surface (core/civil/Tin.h)
// is meaningless without real elevation, so this is the one place that
// keeps it, via its own reader rather than reusing (or changing) those.
struct SurveyPoint {
    Point2D xy;
    double z = 0.0;
};

// Reads a plain-text XYZ file (one point per line: "x y z" or "x,y,z",
// blank lines and lines starting with '#' ignored) keeping Z. Same file
// shape as readPointCloudXyz, just without discarding the third column.
std::vector<SurveyPoint> readSurveyPointsXyz(const std::string& path);

} // namespace lcad
