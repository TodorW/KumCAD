#include "core/cam/Toolpath.h"

#include "core/geometry/BoundingBox.h"
#include "core/geometry/PolylineOps.h"

namespace lcad {

namespace {

Point2D centroidOf(const std::vector<Point2D>& verts) {
    Point2D sum(0, 0);
    for (const Point2D& v : verts) sum = sum + v;
    return verts.empty() ? sum : sum * (1.0 / static_cast<double>(verts.size()));
}

} // namespace

std::vector<Point2D> computeToolpath(const PolylineEntity& profile, const ToolpathParams& params) {
    if (params.side == CutSide::OnLine || params.toolDiameter <= 1e-9) return profile.flattenedVertices();

    const Point2D centroid = centroidOf(profile.vertices());
    BoundingBox box = profile.boundingBox();
    // A point guaranteed to be farther from the profile than any of its own
    // vertices, so offsetPolyline reads it as "the outside".
    const Point2D farOutside = box.max + Point2D(box.max.x - box.min.x + 1.0, box.max.y - box.min.y + 1.0);

    const Point2D sidePoint = params.side == CutSide::Inside ? centroid : farOutside;
    const auto offset = offsetPolyline(profile, profile.id(), params.toolDiameter / 2.0, sidePoint);
    if (!offset) return {};
    return offset->flattenedVertices();
}

} // namespace lcad
