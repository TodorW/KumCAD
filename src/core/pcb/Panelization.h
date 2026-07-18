#pragma once

#include "core/Ids.h"
#include "core/geometry/Point2D.h"

#include <vector>

namespace lcad {

class Document;

enum class PanelSeparator { VScore, MouseBites, None };

struct PanelizeParams {
    int columns = 2;
    int rows = 1;
    double gap = 2.0; // clearance left between adjacent board copies, mm
    PanelSeparator separator = PanelSeparator::VScore;
    double mouseBiteHoleDiameter = 0.5;
    double mouseBiteSpacing = 1.0; // center-to-center along a mouse-bite row
};

// Panelizes doc's own current single-board contents into a columns x rows
// array of copies -- KumCAD's in-house equivalent of the "gang up" step
// PCB fab houses expect (KiCad users typically reach for the external
// kikit tool for this; this is a real, if simpler, built-in version).
// boundary is the single board's own outline (its bounding box sets the
// per-copy pitch); every existing entity in doc is cloned
// columns*rows-1 additional times, translated across a grid of
// (width+gap) x (height+gap) steps, so the original board becomes the
// panel's own (0,0) cell -- REFDES/VALUE attributes are left unchanged
// on every copy, matching KiCad's own panelization (each copy is a
// separate physical board, not one bigger schematic).
//
// Separator geometry is added between adjacent cells along the panel's
// internal column/row boundaries: VScore draws one full-length Line per
// boundary on a "Dwgs.User" documentation layer (created if missing --
// matches KiCad's own scoring-line convention; Edge.Cuts stays reserved
// for each board's actual outline); MouseBites places a row of small
// unfilled Circle entities (drill-hole markers -- this codebase has no
// dedicated NPTH/non-plated-hole entity type) spaced evenly along the
// same line instead; None adds no separator geometry.
//
// Real, disclosed simplification: entities are cloned across the WHOLE
// document, not clipped to boundary -- meant for a document containing
// exactly one board's own worth of geometry (this codebase has no
// general "select everything inside this outline only" clipping).
//
// Returns the ids of every entity added (cloned board copies plus
// separator geometry). Returns empty if boundary has fewer than 3
// points or columns/rows < 1.
std::vector<EntityId> panelizeBoard(Document& doc, const std::vector<Point2D>& boundary, const PanelizeParams& params);

} // namespace lcad
