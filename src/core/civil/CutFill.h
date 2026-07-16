#pragma once

#include "core/civil/Tin.h"

namespace lcad {

struct CutFillResult {
    double cutVolume = 0.0;  // existing surface above proposed -- material to remove
    double fillVolume = 0.0; // proposed surface above existing -- material to add
};

// Estimates cut/fill volume between two TINs (e.g. existing ground vs. a
// proposed grade) by sampling a regular grid of cellSize spacing over the
// two surfaces' combined extent, at each cell taking the elevation
// difference where both surfaces have coverage. This is a grid-sampled
// approximation, not an exact triangle-by-triangle solid decomposition of
// the difference volume -- a real cut/fill engine would compute the latter;
// accuracy here improves as cellSize shrinks, at the cost of more sampling.
// Cells outside either TIN's convex hull are skipped, not extrapolated.
CutFillResult computeCutFillVolume(const Tin& existing, const Tin& proposed, double cellSize);

} // namespace lcad
