#pragma once

#include "core/geometry/Entity.h"

#include <memory>
#include <vector>

namespace lcad {

// JOIN: merges a selection of Lines, Arcs, Circles, and open Polylines into
// one entity, picking the most specific result AutoCAD itself would produce
// rather than always falling back to a generic polyline chain:
//   - All Line, and all collinear (same infinite line): merged into one
//     Line spanning every input's extent, provided the projected intervals
//     chain with no gap wider than tol (arbitrary collinear-but-disjoint
//     lines are refused, same "must actually connect" spirit as the
//     generic chain below).
//   - All Arc, and all on the same circle (matching center+radius): merged
//     into one Arc spanning the chained sweep, or a Circle if the chain
//     wraps a full 360 degrees using every input arc exactly once.
//   - Otherwise: falls back to joinToPolyline (see PolylineOps.h), which
//     handles any endpoint-touching mix of lines/arcs/open polylines.
// Returns nullptr if fewer than 2 parts are given or nothing above applies
// (e.g. disjoint pieces that don't form a single chain).
std::unique_ptr<Entity> joinEntities(EntityId newId, LayerId layer, const std::vector<const Entity*>& parts,
                                     double tol = 1e-6);

} // namespace lcad
