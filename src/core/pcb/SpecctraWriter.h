#pragma once

#include <string>
#include <vector>

namespace lcad {

class Document;
struct ImportedNet;

// A real, honestly-scoped subset of the Specctra DSN format (the same
// interchange format autorouters like FreeRouting import) -- enough to
// actually route from: a rectangular board boundary (the bounding box of
// every placed pad, plus a margin -- this codebase has no separate board-
// outline concept yet), one padstack per distinct pad shape/size, one
// footprint "image" per distinct block definition placed (reused across
// every instance of it, matching how DSN itself expects footprints to
// work), and the net-to-pad connectivity. Each component's own DSN
// `place` side (front/back) is real: a footprint on the document's own
// "B.Cu" layer (by name, the same layer this writer's own fixed 2-layer
// (F.Cu, B.Cu) structure section already assumes) is written "back",
// everything else "front" -- the same "derive it from the footprint's
// own placement layer" idea Board3D.cpp/GerberWriter.h already use, just
// keyed by layer NAME here since this function takes no stackup
// parameter of its own to resolve a LayerId's stackup side from. What's
// NOT here, disclosed rather than silently missing: no keepouts, no
// preserved existing Track/Via copper (an autorouter reading this starts
// from a clean board), and oval pads are approximated as rects -- the
// same "real subset, not full spec" spirit as GerberWriter.h.
//
// nets comes from core/pcb/Ratsnest.h's parseNetlist() (or an equivalent
// already-resolved net list) -- reusing that instead of re-deriving
// connectivity here, same as Ratsnest.h's own computeRatsnest() does.
bool writeSpecctraDsn(const Document& doc, const std::vector<ImportedNet>& nets, const std::string& path,
                      std::string* errorOut = nullptr);

} // namespace lcad
