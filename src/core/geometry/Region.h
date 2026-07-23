#pragma once

#include "core/geometry/Entity.h"

#include <vector>

namespace lcad {

// One closed loop of a Region: either the outer boundary or a hole.
// Winding direction is significant and authoritative -- outer loops wind
// counterclockwise (positive signed area), holes clockwise (negative),
// the same convention real ACIS/DXF regions use to classify loops without
// a separate stored flag.
struct RegionLoop {
    std::vector<Point2D> vertices; // implicitly closed (last connects to first)

    double signedArea() const;
    bool isHole() const { return signedArea() < 0.0; }
};

enum class RegionBoolOp { Union, Subtract, Intersect };

// AutoCAD's REGION: a closed planar area (possibly multiple disjoint outer
// boundaries, each with zero or more holes) supporting boolean
// union/subtract/intersect against another Region. This codebase has no
// ACIS/SAT solid-modeling kernel (see DxfWriter.h's own precedent for
// TRACK/VIA persistence) -- Region is a real polygon-based area, and its
// boolean ops are computed by this file's own Weiler-Atherton/Greiner-
// Hormann-style polygon clipper (regionBoolean(), exposed directly for
// tests), not a solid-modeling kernel. That clipper handles the real
// cases boolean composition needs -- overlapping loops, one loop fully
// containing the other (producing a hole), and disjoint loops -- for
// simple (non-self-intersecting) single loops with no pre-existing holes
// on either side; composing a Region that already has holes with another
// Region is unsupported (booleanOp returns nullptr) and edges that
// exactly graze without crossing (a tangent touch, or an intersection
// landing exactly on a vertex) are a disclosed limitation rather than
// something the clipper guarantees to handle -- the same "real subset,
// disclosed" spirit as this codebase's DRC/autorouter approximations.
class RegionEntity : public Entity {
public:
    RegionEntity(EntityId id, LayerId layer, std::vector<RegionLoop> loops);

    const std::vector<RegionLoop>& loops() const { return m_loops; }

    // Total area: outer loops' areas minus their holes'.
    double area() const;

    // Nonzero-style point-in-region test: pt is inside iff it's enclosed
    // by an odd number of loops when counting from outside in (an outer
    // loop then a hole inside it then an island inside that hole, etc).
    bool containsPoint(const Point2D& pt) const;

    // Composes a and b (each required to be a single loop with no holes)
    // into a new Region via regionBoolean(). Returns nullptr if either
    // input has more than one loop (see class comment).
    static std::unique_ptr<RegionEntity> booleanOp(EntityId newId, LayerId layer, const RegionEntity& a,
                                                    const RegionEntity& b, RegionBoolOp op);

    EntityType type() const override { return EntityType::Region; }
    BoundingBox boundingBox() const override;
    double distanceTo(const Point2D& pt) const override;
    void translate(const Point2D& delta) override;
    void rotate(const Point2D& center, double angleRadians) override;
    void scale(const Point2D& center, double factor) override;
    void mirror(const Point2D& a, const Point2D& b) override;
    std::vector<Point2D> gripPoints() const override;
    void moveGripPoint(std::size_t index, const Point2D& newPos) override;
    std::vector<SnapPoint> snapCandidates() const override;
    std::unique_ptr<Entity> clone() const override;

private:
    std::vector<RegionLoop> m_loops;
};

// The clipping engine itself, exposed directly for tests. a and b are each
// a single simple polygon loop (no holes, orientation not required from
// the caller -- normalized internally). Returns the resulting loop(s): 0
// loops for an empty result, 1 for a single boundary, or 2 when the
// result is an outer boundary plus a hole (full containment under
// Subtract) or two disjoint boundaries (disjoint loops under Union).
std::vector<RegionLoop> regionBoolean(const std::vector<Point2D>& a, const std::vector<Point2D>& b, RegionBoolOp op);

} // namespace lcad
