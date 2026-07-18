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
// real sheet-metal tools use for this same operation. Pass thickness
// explicitly to use exactly that value, or 0.0 to auto-detect it: a ray
// cast straight through the reference face finds the nearest OTHER face
// target itself has along that normal, and its distance becomes the
// detected thickness -- matching real sheet-metal tools' own "pick a
// face, thickness is read from the part" behavior. Auto-detection only
// works when target really is sheet-stock-thin at that face (a straight
// ray exits through exactly one more face); anything else (a thick
// block, a non-parallel opposite face, an open edge) fails detection and
// the whole call returns a null shape the same as any other invalid
// input -- pass an explicit thickness instead for those cases. The new
// wall is fused flat (a sharp corner, no bend-radius fillet) onto target.
// Deliberately NOT auto-rounded here: filleting the fresh seam a flush
// edge-to-edge boolean fuse like this one produces was tried (radius
// applied automatically, right after the fuse) and found to reliably
// crash OCCT's own ChFi3d fillet builder deep inside libTKFillet on
// perfectly ordinary inputs -- a real upstream robustness bug in
// filleting a Boolean result's own seam edge near the T-junction
// vertices a partial-edge weld like this creates, not a bug in this
// function's own edge-finding logic. Picking the new seam edge afterward
// (via Pick3D.h/Window3D's "List Edges...") and applying Fillet as a
// separate, later feature on the ALREADY-COMMITTED shape carries the
// same real risk if attempted immediately -- if it happens, undo and
// either accept the sharp corner or try a larger/smaller radius; this
// isn't something buildFaceFlange itself can detect or prevent.
// Returns a null shape if target is null, the indices are out of range,
// length is non-positive, thickness is negative (or zero and
// auto-detection also failed), or the picked edge/face geometry is too
// degenerate to resolve a flange direction from
// (e.g. the edge's midpoint coincides with the face's own centroid).
TopoDS_Shape buildFaceFlange(const TopoDS_Shape& target, int edgeIndex, int referenceFaceIndex, double length,
                             double bendAngleDegrees, double thickness);

} // namespace lcad
