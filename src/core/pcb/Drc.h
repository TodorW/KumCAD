#pragma once

#include "core/Ids.h"

#include <string>
#include <vector>

namespace lcad {

class Document;

// Design rule check thresholds, in the document's drawing units (typically
// mm). Defaults are conservative hobbyist-fab numbers, not any particular
// fab house's real capability table.
struct DrcRules {
    double minClearance = 0.2;
    double minTrackWidth = 0.15;
    double minViaDiameter = 0.3;
    double minViaDrillDiameter = 0.2;
};

struct DrcViolation {
    std::string message;
    EntityId entityId = 0;
};

// Checks (all approximate -- see runDrc's own comment for what's simplified):
//  - every Track narrower than rules.minTrackWidth;
//  - every Via smaller than rules.minViaDiameter, or with a drill hole not
//    smaller than its own diameter, or a drill under rules.minViaDrillDiameter;
//  - clearance: any two copper features (Track/Via/footprint Pad) that are
//    NOT already electrically joined by existing copper (same connectivity
//    rule as core/pcb/Ratsnest.h) and whose closest approach is under
//    rules.minClearance. Two simplifications, both disclosed: (a)
//    pads/tracks/vias are approximated as circles/capsules -- a rect pad's
//    clearance uses its larger half-dimension as a radius, not exact
//    polygon-to-polygon distance; (b) there's no true multi-layer copper
//    stackup model yet (see ViaEntity's own comment), so this check is
//    layer-agnostic -- a real two-layer board could have safely-overlapping
//    top/bottom traces this flags as violations.
std::vector<DrcViolation> runDrc(const Document& doc, const DrcRules& rules = {});

} // namespace lcad
