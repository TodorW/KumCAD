#pragma once

#include "core/geometry/Point2D.h"

#include <optional>
#include <string>
#include <vector>

namespace lcad {

// The hatch patterns KumCAD ships with (acad.pat subset). Solid is a filled
// polygon; the others are line-family patterns clipped to the boundary.
enum class HatchPattern {
    Solid,
    Ansi31,
    Ansi32,
    Ansi33,
    Ansi37,
};

// One family of parallel lines from a .pat definition, at pattern scale 1:
// successive lines are displaced by `offset` expressed in the line's own
// rotated coordinate system (x along the line -- staggers dashes -- and y
// perpendicular). Dash elements alternate pen-down/pen-up; empty = solid.
struct HatchPatternLine {
    double angleDeg = 0.0;
    Point2D base;
    Point2D offset;
    std::vector<double> dashes;
};

const char* hatchPatternName(HatchPattern pattern);
std::optional<HatchPattern> hatchPatternFromName(const std::string& name);
const std::vector<HatchPatternLine>& hatchPatternLines(HatchPattern pattern);
const std::vector<HatchPattern>& allHatchPatterns();

} // namespace lcad
