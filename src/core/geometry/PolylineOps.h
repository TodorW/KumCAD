#pragma once

#include "core/geometry/Polyline.h"

#include <memory>
#include <vector>

namespace lcad {

class Entity;

// Parallel copy of a polyline at the given distance, on the side of
// sidePoint: vertices move along miter (angle-bisector) normals, bulges are
// preserved (an arc's included angle doesn't change under concentric offset).
// Exact for tangent-continuous polylines and line-line corners; sharp
// arc-line corners are approximate. Returns nullptr when the offset
// degenerates (e.g. an inward offset larger than a local arc radius).
std::unique_ptr<PolylineEntity> offsetPolyline(const PolylineEntity& source, EntityId newId, double distance,
                                               const Point2D& sidePoint);

// PEDIT Join: chains lines, arcs, and open polylines whose endpoints touch
// (within tol) into one polyline, preserving arcs as bulges. Pieces are
// reversed as needed. Returns nullptr unless every part joins into a single
// chain; the result is closed when the chain's ends meet.
std::unique_ptr<PolylineEntity> joinToPolyline(EntityId newId, LayerId layer,
                                               const std::vector<const Entity*>& parts, double tol = 1e-6);

} // namespace lcad
