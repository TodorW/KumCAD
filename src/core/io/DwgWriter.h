#pragma once

#include "core/document/Document.h"

#include <string>

namespace lcad {

// True when this build carries LibreDWG-backed DWG export.
bool dwgWriteSupportAvailable();

// Writes the document as an AutoCAD R2000 DWG via LibreDWG's (experimental)
// add API. Covers lines, circles, arcs, straight polylines, text, mtext,
// ellipses, splines-with-fit-points, blocks/inserts,
// linear/aligned/radial/diameter/angular dimensions, leaders (LEADER per
// MLEADER leg), and the first layout's paper space (viewports + sheet
// entities -- LibreDWG's add API only sets up one paper-space block, so
// later layouts are skipped). Bulged polyline segments explode to arcs;
// hatches export as their boundary outline only (no fill, since the add
// API's associative-boundary HATCH is riskier than a visible outline); a
// TABLE explodes into grid lines plus cell text. skippedOut reports what
// couldn't be expressed (attribute definitions and anything above). DXF
// remains the lossless format.
bool writeDwg(const Document& document, const std::string& path, std::string* errorOut = nullptr,
              int* skippedOut = nullptr);

} // namespace lcad
