#pragma once

#include "core/Color.h"
#include "core/Ids.h"

#include <string>

namespace lcad {

struct Layer {
    LayerId id = 0;
    std::string name;
    Color color;
    bool visible = true;
    bool locked = false;
};

} // namespace lcad
