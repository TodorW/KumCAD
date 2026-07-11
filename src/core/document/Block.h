#pragma once

#include "core/geometry/Entity.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace lcad {

// A dynamic block's linear stretch parameter (a simplified subset of
// AutoCAD's Parameter+Action pair, merged into one object): dragging the
// INSERT's grip at endPoint moves it along the basePoint->endPoint axis, and
// every child vertex inside the frame [frameMin, frameMax] (block-local
// coordinates, same as basePoint/endPoint) slides with it, the same windowed
// stretch as the STRETCH command (see ModifyOps::stretchedClone). Other
// AutoCAD dynamic parameter kinds (flip, rotation, visibility, array,
// lookup) aren't implemented.
struct DynamicLinearParameter {
    Point2D basePoint;
    Point2D endPoint;
    Point2D frameMin;
    Point2D frameMax;
};

// A reusable group of entities (an AutoCAD block definition). Child geometry
// is stored relative to the block's base point, i.e. already translated so
// the base point is the local origin; an InsertEntity places, scales, and
// rotates it. Definitions are owned by the Document and live for its
// lifetime, so entities may hold plain pointers to them.
//
// xrefPath marks an external reference: the entities are a cached snapshot
// of the file at that path, refreshed by XREF Reload (and on open when the
// file is reachable). Empty for ordinary blocks.
struct BlockDefinition {
    std::string name;
    std::vector<std::unique_ptr<Entity>> entities;
    std::string xrefPath;
    std::optional<DynamicLinearParameter> dynamicParam;

    bool isXref() const { return !xrefPath.empty(); }
    bool isDynamic() const { return dynamicParam.has_value(); }
};

} // namespace lcad
