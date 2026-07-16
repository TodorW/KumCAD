#pragma once

#include "core/Ids.h"

#include <string>

namespace lcad {

class Document;

// Writes one copper/silkscreen layer's Track/Via/footprint-Pad/copper-pour
// (a solid-fill HatchEntity used as a pour -- see the mega-roadmap memory's
// note that a pour is just a HATCH placed on a copper layer, not a
// dedicated entity type) geometry as a real RS-274X (Gerber X2-flavored)
// file: %FS%/%MO% header, one circular aperture per distinct diameter used,
// D01 draws for tracks, D03 flashes for pads/vias, G36/G37 regions for
// pours. Assumes the document's units are millimeters. Returns false (with
// *errorOut set, if provided) on a file-open failure.
bool writeGerberLayer(const Document& doc, LayerId layer, const std::string& path, std::string* errorOut = nullptr);

// Writes an Excellon drill file (M48 header, one tool per distinct drill
// diameter among vias and through-hole pads, then per-tool coordinate
// blocks, M30 end) covering every layer.
bool writeExcellonDrill(const Document& doc, const std::string& path, std::string* errorOut = nullptr);

// Writes a pick-and-place file (CSV: RefDes,Footprint,X,Y,RotationDeg) for
// every footprint INSERT in doc. This is KumCAD's own simple CSV shape, not
// a specific fab house's required column layout -- documented here rather
// than pretending to match one.
bool writePickAndPlace(const Document& doc, const std::string& path, std::string* errorOut = nullptr);

} // namespace lcad
