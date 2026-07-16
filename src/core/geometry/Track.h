#pragma once

#include "core/geometry/Entity.h"

#include <vector>

namespace lcad {

// A PCB copper trace: a straight-segment polyline (like WireEntity) with a
// copper width, on whichever named layer (e.g. "Top Copper") its Entity
// layer() points to.
class TrackEntity : public Entity {
public:
    TrackEntity(EntityId id, LayerId layer, std::vector<Point2D> vertices, double width = 0.25)
        : Entity(id, layer), m_vertices(std::move(vertices)), m_width(width) {}

    const std::vector<Point2D>& vertices() const { return m_vertices; }
    double width() const { return m_width; }

    EntityType type() const override { return EntityType::Track; }
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
    double m_width;
};

} // namespace lcad
