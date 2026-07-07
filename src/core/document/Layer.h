#pragma once

#include "core/Ids.h"

#include <cstdint>
#include <string>

namespace lcad {

struct Color {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
};

struct Layer {
    LayerId id = 0;
    std::string name;
    Color color;
    bool visible = true;
    bool locked = false;
};

} // namespace lcad
