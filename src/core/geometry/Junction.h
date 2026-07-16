#pragma once

#include "core/geometry/Entity.h"

namespace lcad {

// A dot marking a real electrical connection where a wire endpoint touches
// another wire's interior vertex (a T or cross connection). Without one
// here, computeNets() treats the crossing as coincidental, not connected --
// matching KiCad's rule that wires crossing without a junction dot are
// electrically separate.
class JunctionEntity : public Entity {
public:
    JunctionEntity(EntityId id, LayerId layer, Point2D position) : Entity(id, layer), m_position(position) {}

    const Point2D& position() const { return m_position; }

    EntityType type() const override { return EntityType::Junction; }
    BoundingBox boundingBox() const override {
        BoundingBox box;
        box.expand(m_position);
        return box;
    }
    double distanceTo(const Point2D& pt) const override { return pt.distanceTo(m_position); }
    void translate(const Point2D& delta) override { m_position = m_position + delta; }
    void rotate(const Point2D& center, double angleRadians) override {
        m_position = rotateAround(m_position, center, angleRadians);
    }
    void scale(const Point2D& center, double factor) override { m_position = scaleAround(m_position, center, factor); }
    void mirror(const Point2D& a, const Point2D& b) override { m_position = mirrorAcross(m_position, a, b); }
    std::vector<Point2D> gripPoints() const override { return {m_position}; }
    void moveGripPoint(std::size_t index, const Point2D& newPos) override {
        if (index == 0) m_position = newPos;
    }
    std::vector<SnapPoint> snapCandidates() const override { return {{m_position, SnapKind::Node}}; }
    std::unique_ptr<Entity> clone() const override { return std::make_unique<JunctionEntity>(*this); }

private:
    Point2D m_position;
};

} // namespace lcad
