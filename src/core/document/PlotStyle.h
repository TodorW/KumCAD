#pragma once

#include "core/Color.h"
#include "core/document/LineType.h"

#include <optional>
#include <string>

namespace lcad {

// AutoCAD Plot Style Table (STB-style, simplified): a named style that can
// override an object's plotted color/lineweight/linetype independent of its
// on-screen appearance. Assigned per-layer (Layer::plotStyle names one of
// Document::plotStyles() by name; empty means "plot as displayed", i.e.
// AutoCAD's "Normal" style). Real AutoCAD ships plot styles as an external,
// binary .stb/.ctb file edited only through its own Plot Style Editor and
// referenced per-layout; this keeps the table inside the drawing itself
// instead, the same simplification this codebase already made for other
// "manager" data (LAYERSTATE, DATALINK) -- no color-dependent (CTB) table,
// no screening/dithering/line-end-style controls, no per-layout table
// selection (one table applies to the whole document).
struct PlotStyle {
    std::string name;
    std::optional<Color> color;
    std::optional<double> lineweight; // mm
    std::optional<LineType> linetype;
};

// The color/lineweight/linetype an entity actually plots with: layer, then
// entity override, then (if the layer has one assigned) the named plot
// style's overrides -- matching AutoCAD's own precedence, where a plot
// style can repaint an object regardless of its ByLayer/ByObject color.
struct PlotAppearance {
    Color color;
    double lineweight = 0.25; // mm
    LineType linetype = LineType::Continuous;
};

} // namespace lcad
