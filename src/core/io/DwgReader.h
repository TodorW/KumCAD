#pragma once

#include "core/document/Document.h"

#include <string>

namespace lcad {

// True when this build carries LibreDWG-backed DWG import.
bool dwgSupportAvailable();

// Reads an AutoCAD DWG file into the document (replacing its contents), via
// LibreDWG: lines, circles, arcs, LWPOLYLINE/old-style POLYLINE_2D, leaders,
// ellipses, splines, text, mtext, dimensions, and inserts, from model space
// only (paper space isn't read back, matching DwgWriter's own scope).
// Unsupported object types are skipped. Without LibreDWG in the build this
// fails with an explanatory error. See DwgWriter.h for DWG *writing*
// (LibreDWG's own add API is still marked experimental there).
bool readDwg(Document& document, const std::string& path, std::string* errorOut = nullptr);

} // namespace lcad
