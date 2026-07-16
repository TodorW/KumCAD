#pragma once

#include "core/core3d/Document3D.h"

#include <string>

namespace lcad {

// Native 3D document persistence (Sprint 4): a .kcad3d file is a store-only
// zip (see core/io/Zip.h) holding one "document.txt" entry -- a plain
// whitespace/line-tagged text encoding of every sketch and every feature's
// parameters, the same "custom tags a real reader can skip" spirit as this
// codebase's DXF convention -- plus one "imported_<N>.brep" entry per
// Document3D::importedShapes() (OCCT's own native BRep text format, since
// an imported shape has no parametric recipe to just replay on load).
//
// Every ordinary feature (primitives, booleans, sketch-based, patterns) is
// saved as parameters only and *recomputed* on load via the normal
// Document3D::addFeature path -- this is the actual "parametric" promise of
// native persistence, not a fixed geometry snapshot.
bool saveDocument3D(const Document3D& doc, const std::string& path);

// Loads into doc, which should be freshly constructed -- this appends
// sketches/imported-shapes/features onto whatever doc already has rather
// than clearing it first. Returns false (doc left partially populated) on
// any structural or read failure.
bool loadDocument3D(Document3D& doc, const std::string& path);

} // namespace lcad
