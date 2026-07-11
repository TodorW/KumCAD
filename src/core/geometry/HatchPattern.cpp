#include "core/geometry/HatchPattern.h"

#include <algorithm>
#include <cctype>

namespace lcad {

const char* hatchPatternName(HatchPattern pattern) {
    switch (pattern) {
    case HatchPattern::Solid: return "SOLID";
    case HatchPattern::Ansi31: return "ANSI31";
    case HatchPattern::Ansi32: return "ANSI32";
    case HatchPattern::Ansi33: return "ANSI33";
    case HatchPattern::Ansi37: return "ANSI37";
    }
    return "SOLID";
}

std::optional<HatchPattern> hatchPatternFromName(const std::string& name) {
    std::string upper = name;
    std::transform(upper.begin(), upper.end(), upper.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    for (HatchPattern pattern : allHatchPatterns()) {
        if (upper == hatchPatternName(pattern)) return pattern;
    }
    return std::nullopt;
}

const std::vector<HatchPatternLine>& hatchPatternLines(HatchPattern pattern) {
    // Definitions from acad.pat (inches at scale 1).
    static const std::vector<HatchPatternLine> kNone{};
    static const std::vector<HatchPatternLine> kAnsi31{
        {45.0, {0, 0}, {0, 0.125}, {}},
    };
    static const std::vector<HatchPatternLine> kAnsi32{
        {45.0, {0, 0}, {0, 0.375}, {}},
        {45.0, {0.176776695, 0}, {0, 0.375}, {}},
    };
    static const std::vector<HatchPatternLine> kAnsi33{
        {45.0, {0, 0}, {0, 0.25}, {}},
        {45.0, {0.176776695, 0}, {0, 0.25}, {0.125, -0.0625}},
    };
    static const std::vector<HatchPatternLine> kAnsi37{
        {45.0, {0, 0}, {0, 0.125}, {}},
        {135.0, {0, 0}, {0, 0.125}, {}},
    };
    switch (pattern) {
    case HatchPattern::Solid: return kNone;
    case HatchPattern::Ansi31: return kAnsi31;
    case HatchPattern::Ansi32: return kAnsi32;
    case HatchPattern::Ansi33: return kAnsi33;
    case HatchPattern::Ansi37: return kAnsi37;
    }
    return kNone;
}

const std::vector<HatchPattern>& allHatchPatterns() {
    static const std::vector<HatchPattern> kAll{
        HatchPattern::Solid, HatchPattern::Ansi31, HatchPattern::Ansi32,
        HatchPattern::Ansi33, HatchPattern::Ansi37,
    };
    return kAll;
}

} // namespace lcad
