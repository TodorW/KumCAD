#pragma once

#include "core/geometry/Entity.h"

#include <vector>

namespace lcad {

// Leader line (AutoCAD LEADER): an arrowhead at the first vertex followed by
// straight segments to the landing. The annotation itself is a separate
// MTEXT entity, like AutoCAD's classic LEADER with its detached annotation.
class LeaderEntity : public Entity {
public:
    LeaderEntity(EntityId id, LayerId layer, std::vector<Point2D> points, double arrowSize = 1.25)
        : Entity(id, layer), m_points(std::move(points)), m_arrowSize(arrowSize) {}

    const std::vector<Point2D>& points() const { return m_points; }
    double arrowSize() const { return m_arrowSize; }

    EntityType type() const override { return EntityType::Leader; }
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
    std::vector<Point2D> m_points;
    double m_arrowSize;
};

} // namespace lcad
