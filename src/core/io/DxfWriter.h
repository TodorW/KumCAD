#pragma once

#include "core/document/Document.h"

#include <string>

namespace lcad {

// Writes an ASCII DXF (R2000-ish, group-code/value pairs) file covering
// every entity type KumCAD's Document can hold -- LINE, CIRCLE, ARC,
// LWPOLYLINE, ELLIPSE, SPLINE, DIMENSION, TEXT, MTEXT, LEADER, MLEADER,
// HATCH, INSERT, TABLE, IMAGE, POINTCLOUD, POINT, XLINE, ATTDEF, WIPEOUT,
// plus PCB/schematic entities (WIRE, JUNCTION, NOCONNECT, NETLABEL, TRACK,
// VIA) -- along with the LAYER table, BLOCK definitions, and paper-space
// LAYOUT objects. Colors are written as DXF true-color (group 420) for
// lossless round-tripping, with a plain ACI fallback (group 62) for older
// readers. Returns true on success; on failure, *errorOut (if given) gets
// a message.
bool writeDxf(const Document& document, const std::string& path, std::string* errorOut = nullptr);

} // namespace lcad
