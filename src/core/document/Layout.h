#pragma once

#include "core/geometry/Point2D.h"

#include <string>
#include <vector>

namespace lcad {

// A rectangular window from paper space into model space: placed on the
// sheet in paper units (mm), showing the model around modelCenter at
// viewScale paper-mm per model unit.
struct Viewport {
    Point2D paperCenter;
    double paperWidth = 100.0;
    double paperHeight = 100.0;
    Point2D modelCenter;
    double viewScale = 1.0;
};

// A paper-space layout (AutoCAD's Layout tabs): a sheet with viewports.
// Paper dimensions are in mm; the default is A4 landscape.
struct Layout {
    std::string name = "Layout1";
    double paperWidth = 297.0;
    double paperHeight = 210.0;
    std::vector<Viewport> viewports;
};

} // namespace lcad
