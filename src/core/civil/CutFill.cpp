#include "core/civil/CutFill.h"

#include <algorithm>
#include <limits>

namespace lcad {

CutFillResult computeCutFillVolume(const Tin& existing, const Tin& proposed, double cellSize) {
    CutFillResult result;
    if (cellSize <= 1e-9) return result;

    double minX = std::numeric_limits<double>::infinity(), minY = minX;
    double maxX = -minX, maxY = -minX;
    for (const Tin* tin : {&existing, &proposed}) {
        for (const auto& p : tin->points) {
            minX = std::min(minX, p.xy.x);
            minY = std::min(minY, p.xy.y);
            maxX = std::max(maxX, p.xy.x);
            maxY = std::max(maxY, p.xy.y);
        }
    }
    if (!(maxX > minX) || !(maxY > minY)) return result;

    const double cellArea = cellSize * cellSize;
    for (double x = minX + cellSize / 2.0; x < maxX; x += cellSize) {
        for (double y = minY + cellSize / 2.0; y < maxY; y += cellSize) {
            const Point2D sample(x, y);
            const auto existingZ = elevationAt(existing, sample);
            const auto proposedZ = elevationAt(proposed, sample);
            if (!existingZ || !proposedZ) continue;

            const double diff = *proposedZ - *existingZ;
            if (diff > 0) result.fillVolume += diff * cellArea;
            else result.cutVolume += -diff * cellArea;
        }
    }
    return result;
}

} // namespace lcad
