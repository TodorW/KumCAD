#pragma once

#include "core/geometry/Entity.h"

#include <algorithm>
#include <string>
#include <utility>

namespace lcad {

// A named net tag (KiCad's "label"/"global label"): when its position
// coincides with a point in a computed net, that net takes this name
// instead of an auto-generated one. Two labels of the same name touching
// different, otherwise-unconnected wire groups make those groups the same
// logical net without a drawn wire between them, same as real schematic
// tools.
class NetLabelEntity : public Entity {
public:
    NetLabelEntity(EntityId id, LayerId layer, Point2D position, std::string name, double height = 2.5)
        : Entity(id, layer), m_position(position), m_name(std::move(name)), m_height(height) {}

    const Point2D& position() const { return m_position; }
    const std::string& name() const { return m_name; }
    double height() const { return m_height; }

    EntityType type() const override { return EntityType::NetLabel; }
    BoundingBox boundingBox() const override {
        BoundingBox box;
        box.expand(m_position);
        box.expand(m_position + Point2D(0.6 * m_height * static_cast<double>(m_name.size()), m_height));
        return box;
    }
    double distanceTo(const Point2D& pt) const override {
        const BoundingBox box = boundingBox();
        const Point2D clamped(std::clamp(pt.x, box.min.x, box.max.x), std::clamp(pt.y, box.min.y, box.max.y));
        return pt.distanceTo(clamped);
    }
    void translate(const Point2D& delta) override { m_position = m_position + delta; }
    void rotate(const Point2D& center, double angleRadians) override {
        m_position = rotateAround(m_position, center, angleRadians);
    }
    void scale(const Point2D& center, double factor) override {
        m_position = scaleAround(m_position, center, factor);
        m_height *= factor;
    }
    void mirror(const Point2D& a, const Point2D& b) override { m_position = mirrorAcross(m_position, a, b); }
    std::vector<Point2D> gripPoints() const override { return {m_position}; }
    void moveGripPoint(std::size_t index, const Point2D& newPos) override {
        if (index == 0) m_position = newPos;
    }
    std::vector<SnapPoint> snapCandidates() const override { return {{m_position, SnapKind::Endpoint}}; }
    std::unique_ptr<Entity> clone() const override { return std::make_unique<NetLabelEntity>(*this); }

private:
    Point2D m_position;
    std::string m_name;
    double m_height;
};

} // namespace lcad
