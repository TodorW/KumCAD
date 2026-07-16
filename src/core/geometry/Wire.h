#pragma once

#include "core/geometry/Entity.h"

#include <vector>

namespace lcad {

// A schematic wire (KiCad-style): a straight-segment polyline whose
// endpoints and vertices carry electrical connectivity (see
// core/schematic/Netlist.h). Unlike PolylineEntity, wires never bulge --
// electrical connections are drawn as orthogonal/straight runs -- and a
// wire's own vertices always connect to each other along its path; two
// different wires (or a wire and a pin) only connect where their endpoints
// coincide, or at an explicit JunctionEntity for a T/cross connection.
class WireEntity : public Entity {
public:
    WireEntity(EntityId id, LayerId layer, std::vector<Point2D> vertices)
        : Entity(id, layer), m_vertices(std::move(vertices)) {}

    const std::vector<Point2D>& vertices() const { return m_vertices; }

    EntityType type() const override { return EntityType::Wire; }
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
    std::vector<Point2D> m_vertices;
};

} // namespace lcad
