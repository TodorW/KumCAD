#pragma once

#include "core/geometry/Point2D.h"

#include <vector>

namespace lcad {

// Outline polygons (closed, CCW, in pad-local space centered on the pad's
// own position, local +Y "up") for the two PadShape kinds whose boundary
// isn't a native primitive (Round is a circle, Rect/Oval keep using each
// consumer's own rect/ellipse call) -- shared by 2D rendering
// (EntityPainter), 3D solid generation (Board3D), and Gerber aperture
// macros (GerberWriter) so all three agree on the exact same geometry.
//
// cornerArcSegments controls how many straight segments approximate each
// rounded corner's quarter-circle -- the same "polygon approximation of a
// curve" convention DonutOps/PolygonOps already use elsewhere.
std::vector<Point2D> roundRectPadOutline(double width, double height, double cornerRatio, int cornerArcSegments = 8);

// delta > 0 widens the bottom edge (-Y) and narrows the top edge (+Y) by
// the same amount, matching real KiCad's rect_delta.x convention.
std::vector<Point2D> trapezoidPadOutline(double width, double height, double delta);

} // namespace lcad
