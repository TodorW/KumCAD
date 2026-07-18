#pragma once

#include "core/Ids.h"
#include "core/pcb/Ratsnest.h"

#include <string>
#include <vector>

namespace lcad {

class Document;

// Writes one copper/silkscreen layer's Track/Via/footprint-Pad/copper-pour
// (a solid-fill HatchEntity used as a pour -- see the mega-roadmap memory's
// note that a pour is just a HATCH placed on a copper layer, not a
// dedicated entity type) geometry as a real RS-274X (Gerber X2) file:
// %FS%/%MO% header, real X2 file attributes (%TF.GenerationSoftware%,
// %TF.CreationDate%, %TF.FileFunction% inferred from the layer's own name
// via KiCad-style conventions -- F.Cu/B.Cu/F.SilkS/B.SilkS/F.Mask/B.Mask/
// Edge.Cuts/InN.Cu, falling back to a generic "Other,User" for an
// unrecognized name -- %TF.FilePolarity%, %TF.Part%), one circular
// aperture per distinct diameter used, D01 draws for tracks, D03 flashes
// for pads/vias (each footprint pad's flash wrapped in a real %TO.C%
// component attribute naming its REFDES, cleared with %TD*% once pads are
// done), G36/G37 regions for pours. Assumes the document's units are
// millimeters. Returns false (with *errorOut set, if provided) on a
// file-open failure.
//
// nets (optional, empty by default) resolves each pad's own net by
// matching (REFDES, pad number) against nets' own pins -- the same
// netlist connectivity Ratsnest.h's computeRatsnest() already resolves
// pads through -- and, when found, wraps that pad's flash in a real
// %TO.N,<netname>*% object attribute (cleared with the single-attribute
// %TD.N*%, so it doesn't disturb the outer %TO.C% wrap around it). Left
// empty, no %TO.N% attribute is emitted at all -- the previous behavior,
// unchanged for every existing caller. A pad whose (REFDES, number) isn't
// found in nets is simply left without a %TO.N% attribute, the same way
// a footprint with no REFDES at all skips %TO.C%.
//
// Real, disclosed simplification: only pads get %TO.N% -- Track/Via
// draws and flashes don't carry one, since neither entity has its own
// net field to resolve without a full geometric connectivity trace (the
// same "no dedicated net field" limitation Board3D.h's own stackup
// placement already discloses for pads).
bool writeGerberLayer(const Document& doc, LayerId layer, const std::string& path, std::string* errorOut = nullptr,
                      const std::vector<ImportedNet>& nets = {});

// Writes an Excellon drill file (M48 header, one tool per distinct drill
// diameter among vias and through-hole pads, then per-tool coordinate
// blocks, M30 end) covering every layer.
bool writeExcellonDrill(const Document& doc, const std::string& path, std::string* errorOut = nullptr);

// Writes a pick-and-place file (CSV: RefDes,Footprint,X,Y,RotationDeg) for
// every footprint INSERT in doc. This is KumCAD's own simple CSV shape, not
// a specific fab house's required column layout -- documented here rather
// than pretending to match one.
bool writePickAndPlace(const Document& doc, const std::string& path, std::string* errorOut = nullptr);

} // namespace lcad
