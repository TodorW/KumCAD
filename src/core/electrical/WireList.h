#pragma once

#include "core/geometry/Point2D.h"
#include "core/schematic/Netlist.h"

namespace lcad {

class Document;
class TableEntity;

// Builds a wire-list/cross-reference TABLE (one row per net pin: Net,
// RefDes, Pin) at position and adds it to doc, returning the entity. A
// real panel cross-reference report also lists each wire's physical number
// (see WireNumbering.h) and which sheet/rung a component appears on; this
// is the reference-designator/net cross-reference core of that, not the
// full report.
TableEntity* buildWireListTable(Document& doc, const std::vector<Net>& nets, Point2D position);

} // namespace lcad
