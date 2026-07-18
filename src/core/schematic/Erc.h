#pragma once

#include "core/Ids.h"
#include "core/schematic/Netlist.h"

#include <string>
#include <vector>

namespace lcad {

class Document;

// One ERC finding. Scoped to the two headline checks real schematic tools
// lead with -- a full configurable pin-conflict matrix (KiCad's ERC rules
// file) is out of scope; see runErc's own comment for exactly what's
// checked.
struct ErcIssue {
    enum class Severity { Warning, Error };
    Severity severity;
    std::string message;
    EntityId insertId = 0; // the symbol instance the issue is about, 0 if net-wide
};

// Runs ERC over nets (as returned by computeNets(doc)):
//  - a pin with no NoConnect marker sitting alone in its own net (nothing
//    wired to it) -- Warning, unless its electrical type is NotConnected.
//  - a pin whose electrical type is NotConnected but whose net has more
//    than one pin (it *is* wired to something) -- Warning.
//  - a net with more than one Output-type pin (driver conflict) -- Error.
//  - a net with a Power-type pin (a power INPUT -- an IC's VCC/GND, a
//    relay coil terminal, etc.) but no Output or PowerOutput pin
//    anywhere on it (nothing actually sourcing that power) -- Warning,
//    real KiCad's own "input power pin not driven" check, one of its
//    most common real findings (a supply pin left dangling). PowerOutput
//    is the distinct pin type a real power source (a battery/regulator
//    terminal) uses -- see PinElectricalType in Block.h.
std::vector<ErcIssue> runErc(const Document& doc, const std::vector<Net>& nets);

} // namespace lcad
