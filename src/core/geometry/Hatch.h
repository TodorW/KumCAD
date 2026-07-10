#pragma once

#include "core/geometry/Entity.h"

#include <vector>

namespace lcad {

// Solid-filled closed polygon region, the SOLID-pattern subset of AutoCAD's
// HATCH. Boundary vertices are implicitly closed (last connects to first).
class HatchEntity : public Entity {
public:
    HatchEntity(EntityId id, LayerId layer, std::vector<Point2D> vertices)
        : Entity(id, layer), m_vertices(std::move(vertices)) {}

    const std::vector<Point2D>& vertices() const { return m_vertices; }

    // Point-in-polygon (ray casting); picking a hatch anywhere inside hits it.
    bool containsPoint(const Point2D& pt) const;

    EntityType type() const override { return EntityType::Hatch; }
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
