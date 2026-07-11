#pragma once

#include "core/geometry/Entity.h"

#include <string>

namespace lcad {

enum class DimensionKind {
    Linear,   // horizontal/vertical distance (AutoCAD DIMLINEAR)
    Aligned,  // true point-to-point distance (DIMALIGNED)
    Radius,   // "R" + radius of a circle/arc (DIMRADIUS)
    Diameter, // diameter symbol + diameter (DIMDIAMETER)
    Angular,  // angle at a vertex between two rays (DIMANGULAR, 3-point)
};

// Dimension entity covering AutoCAD's linear/aligned/radial/diameter/angular
// kinds. The stored points mean different things per kind:
//   Linear/Aligned: p1/p2 = measured points, linePoint = dimension line drag
//   Radius/Diameter: p1 = center, p2 = point on the curve, linePoint = label
//   Angular: vertex() = corner, p1/p2 = ray points, linePoint = arc position
// All derived drawing geometry (lines, the arc for angular, arrowheads, the
// ready-made label) comes from geometry(), shared by rendering and picking.
class DimensionEntity : public Entity {
public:
    struct Geometry {
        Point2D dimA, dimB;      // dimension line endpoints (arrow tips); for
                                 // angular these are the arc's endpoints
        Point2D ext1A, ext1B;    // extension line from p1 (may be degenerate)
        Point2D ext2A, ext2B;    // extension line from p2
        Point2D textPos;         // label anchor (center of the text)
        double textAngle = 0.0;  // label rotation, radians CCW
        double value = 0.0;      // measured distance (or angle in degrees)
        std::string label;       // formatted text, e.g. "12.50", "R5.00", "45.0\xC2\xB0"
        bool arrow1 = true;      // draw arrowhead at dimA / arc start
        bool arrow2 = true;      // draw arrowhead at dimB / arc end
        bool angular = false;    // dimension line is an arc, not a segment
        Point2D arcCenter;       // angular only: arc parameters, CCW sweep
        double arcRadius = 0.0;
        double arcStartAngle = 0.0;
        double arcEndAngle = 0.0;
    };

    DimensionEntity(EntityId id, LayerId layer, Point2D p1, Point2D p2, Point2D linePoint, bool aligned,
                    double textHeight = 2.5)
        : Entity(id, layer), m_kind(aligned ? DimensionKind::Aligned : DimensionKind::Linear), m_p1(p1), m_p2(p2),
          m_linePoint(linePoint), m_textHeight(textHeight) {}

    DimensionEntity(EntityId id, LayerId layer, DimensionKind kind, Point2D p1, Point2D p2, Point2D linePoint,
                    Point2D vertex = Point2D(), double textHeight = 2.5)
        : Entity(id, layer), m_kind(kind), m_p1(p1), m_p2(p2), m_linePoint(linePoint), m_vertex(vertex),
          m_textHeight(textHeight) {}

    DimensionKind kind() const { return m_kind; }
    const Point2D& point1() const { return m_p1; }
    const Point2D& point2() const { return m_p2; }
    const Point2D& linePoint() const { return m_linePoint; }
    const Point2D& vertex() const { return m_vertex; }
    bool aligned() const { return m_kind == DimensionKind::Aligned; }
    double textHeight() const { return m_textHeight; }

    // Style snapshot (from the document's DIMSTYLE at creation time).
    double arrowSize() const { return m_arrowSize > 1e-12 ? m_arrowSize : 0.5 * m_textHeight; }
    int decimals() const { return m_decimals; }
    void setStyle(double textHeight, double arrowSize, int decimals) {
        m_textHeight = textHeight;
        m_arrowSize = arrowSize;
        m_decimals = decimals;
    }

    void setPoint1(const Point2D& p) { m_p1 = p; }
    void setPoint2(const Point2D& p) { m_p2 = p; }

    // Associativity: optional snap references binding each definition point
    // to another entity. Document::reassociateDimensions() re-resolves them
    // after edits so the dimension follows the measured geometry.
    const std::optional<SnapRef>& anchor1() const { return m_anchor1; }
    const std::optional<SnapRef>& anchor2() const { return m_anchor2; }
    void setAnchor1(std::optional<SnapRef> ref) { m_anchor1 = ref; }
    void setAnchor2(std::optional<SnapRef> ref) { m_anchor2 = ref; }

    Geometry geometry() const;

    EntityType type() const override { return EntityType::Dimension; }
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
    DimensionKind m_kind;
    Point2D m_p1;
    Point2D m_p2;
    Point2D m_linePoint;
    Point2D m_vertex; // angular only
    double m_textHeight;
    double m_arrowSize = 0.0; // 0 = derive from text height
    int m_decimals = 2;
    std::optional<SnapRef> m_anchor1;
    std::optional<SnapRef> m_anchor2;
};

} // namespace lcad
