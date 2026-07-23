#pragma once

#include "core/Color.h"
#include "core/geometry/Entity.h"

#include <vector>

namespace lcad {

// One parallel line of an MLINE, offset perpendicular to the centerline.
// offset is signed: positive is to the left of the direction of travel
// (the centerline's own vertex order), matching this codebase's existing
// left-hand-normal-is-inward convention for a CCW loop (Region.h).
struct MLineElement {
    double offset = 0.0;
    Color color{255, 255, 255};
};

enum class MLineCapType { None, Line, OuterArc, InnerArc };

// AutoCAD's MLINE: a centerline plus a fixed set of parallel offset
// elements (real AutoCAD keeps the element list in a separate named
// MLSTYLE table object; this codebase embeds it directly on the entity
// instead -- a real, disclosed simplification, the same "no extra
// indirection" call Hatch already makes for its own pattern selection).
// Segments are always straight (no bulge/arc support, matching real
// MLINE itself, which is straight-segment-only).
class MLineEntity : public Entity {
public:
    MLineEntity(EntityId id, LayerId layer, std::vector<Point2D> vertices, std::vector<MLineElement> elements,
               double scale = 1.0, bool closed = false, MLineCapType startCap = MLineCapType::None,
               MLineCapType endCap = MLineCapType::None, bool showJoints = false)
        : Entity(id, layer), m_vertices(std::move(vertices)), m_elements(std::move(elements)), m_scale(scale),
          m_closed(closed), m_startCap(startCap), m_endCap(endCap), m_showJoints(showJoints) {}

    const std::vector<Point2D>& vertices() const { return m_vertices; }
    const std::vector<MLineElement>& elements() const { return m_elements; }
    double scale() const { return m_scale; }
    bool closed() const { return m_closed; }
    MLineCapType startCap() const { return m_startCap; }
    MLineCapType endCap() const { return m_endCap; }
    bool showJoints() const { return m_showJoints; }

    // Every element's own offset polyline (parallel to the centerline,
    // one entry per element, in the same order as elements()), each
    // vertex placed via the local angle-bisector (miter) normal of the
    // centerline at that vertex -- exact for any two straight segments
    // meeting at an angle, a simple (non-arc-filleted) miter at sharp
    // interior corners, the same real-vs-simplified trade-off
    // PolylineOps::offsetPolyline already discloses for its own corners.
    // Returns one (possibly closed) polyline per element, empty if the
    // centerline has fewer than 2 vertices.
    std::vector<std::vector<Point2D>> elementLines() const;

    EntityType type() const override { return EntityType::MLine; }
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
    std::vector<MLineElement> m_elements;
    double m_scale;
    bool m_closed;
    MLineCapType m_startCap;
    MLineCapType m_endCap;
    bool m_showJoints;
};

} // namespace lcad
