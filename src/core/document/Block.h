#pragma once

#include "core/geometry/Entity.h"

#include <memory>
#include <string>
#include <vector>

namespace lcad {

// A reusable group of entities (an AutoCAD block definition). Child geometry
// is stored relative to the block's base point, i.e. already translated so
// the base point is the local origin; an InsertEntity places, scales, and
// rotates it. Definitions are owned by the Document and live for its
// lifetime, so entities may hold plain pointers to them.
struct BlockDefinition {
    std::string name;
    std::vector<std::unique_ptr<Entity>> entities;
};

} // namespace lcad
