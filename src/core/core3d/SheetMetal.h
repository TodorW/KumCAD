#pragma once

#include "core/document/Document.h"

#include <TopoDS_Shape.hxx>

#include <vector>

namespace lcad {

// A sheet-metal part as a strip of constant width/thickness, described by
// its NEUTRAL-AXIS path: a sequence of flat runs separated by bends. This
// remains a deliberate, disclosed simplification for building a whole part
// from scratch in one shot -- for adding ONE more wall onto an EXISTING
// solid (what real tools call a "Flange"/"Wall" feature), see
// buildFaceFlange below instead, which does use real edge/face picking
// (Pick3D.h) now that that infrastructure exists.
//
// flatLengths.size() must equal bendAngles.size() + 1 (N flats, N-1 bends
// between them). Bend angles are in degrees; positive turns the strip's
// heading left (counter-clockwise) in its own local profile plane.
struct SheetMetalPart {
    double width = 10.0;
    double thickness = 1.0;
    double bendRadius = 1.0; // inside bend radius, shared by every bend
    double kFactor = 0.44;   // standard sheet-metal bend-allowance k-factor
    std::vector<double> flatLengths;
    std::vector<double> bendAngles;
};

// Builds the real 3D solid: each bend is modeled as an actual circular arc
// of radius (bendRadius + kFactor*thickness) around the neutral axis, not
// a sharp corner rounded off afterward -- so the solid's own neutral-axis
// path length exactly matches flatPatternLength()'s formula, by
// construction, rather than by approximation. Returns a null shape if the
// part is geometrically invalid (mismatched array sizes, non-positive
// dimensions, a bend whose inner offset radius would be non-positive, or
// any |bendAngle| >= 180 -- that last one a disclosed scope limit, since
// the neutral-path arc-trimming math assumes less than a half-turn).
TopoDS_Shape buildSheetMetalSolid(const SheetMetalPart& part);

// The exact flat-pattern length along the strip's neutral axis: every flat
// run plus each bend's allowance (BA = angle_rad * (bendRadius +
// kFactor*thickness), the standard sheet-metal formula). Returns 0.0 for
// an invalid part (see buildSheetMetalSolid).
double flatPatternLength(const SheetMetalPart& part);

// Bakes the flat pattern into doc2d as a closed rectangle
// (flatPatternLength(part) x width) on a "FLATPATTERN" layer, with a
// dashed (LineType::Center) bend line across the strip at each bend's
// position along that unfolded length -- matching how real fabrication
// drawings mark where to bend flat stock.
void insertFlatPatternIntoDocument(Document& doc2d, const SheetMetalPart& part, double offsetX, double offsetY);

// Adds a new flat wall ("Flange"/"Wall" in real sheet-metal tools) onto
// target, extending from edgeIndex's edge (numbered the same way Pick3D.h's
// pickEdge does), spanning that edge's own length, extruded by length in a
// direction determined by bendAngleDegrees measured from referenceFaceIndex
// (numbered the same way pickFace's faceIndex does): 0 degrees continues
// coplanar with that face, 90 degrees rises perpendicular to it (the
// common right-angle-flange case), matching the intuitive parametrization
// real sheet-metal tools use for this same operation. thickness is passed
// explicitly rather than measured from target (detecting "the" thickness
// of an arbitrary solid is a much harder, separate problem, out of scope
// here) -- pass the same thickness the rest of the part uses. The new
// wall is fused flat (a sharp corner, no bend-radius fillet) onto target;
// if a rounded bend is wanted, pick the new seam edge afterward (via
// Pick3D.h/Window3D's "List Edges...") and apply a real Fillet feature to
// it -- reusing that existing capability rather than duplicating bend-
// radius logic here. Returns a null shape if target is null, the indices
// are out of range, length/thickness are non-positive, or the picked
// edge/face geometry is too degenerate to resolve a flange direction from
// (e.g. the edge's midpoint coincides with the face's own centroid).
TopoDS_Shape buildFaceFlange(const TopoDS_Shape& target, int edgeIndex, int referenceFaceIndex, double length,
                             double bendAngleDegrees, double thickness);

} // namespace lcad
