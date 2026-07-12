#pragma once

#include "core/Color.h"
#include "core/geometry/Entity.h"
#include "core/geometry/HatchPattern.h"

#include <optional>
#include <string>
#include <vector>

namespace lcad {

// AutoCAD's nine named gradient shapes (DXF group 470's gradient_name).
// Rendering (EntityPainter) approximates each with Qt's linear/radial
// gradients rather than reproducing AutoCAD's exact proprietary curves.
enum class GradientPreset {
    Linear,
    Cylinder,
    InvCylinder,
    Spherical,
    InvSpherical,
    Hemispherical,
    InvHemispherical,
    Curved,
    InvCurved,
};
const char* gradientPresetName(GradientPreset preset);
std::optional<GradientPreset> gradientPresetFromName(const std::string& name);

// Closed polygon region filled solid or with a line-family pattern (the
// AutoCAD HATCH). Boundary vertices are implicitly closed (last connects to
// first). Pattern hatches keep the acad.pat geometry (see HatchPattern.h)
// transformed by patternScale/patternAngle.
class HatchEntity : public Entity {
public:
    HatchEntity(EntityId id, LayerId layer, std::vector<Point2D> vertices)
        : Entity(id, layer), m_vertices(std::move(vertices)) {}

    HatchEntity(EntityId id, LayerId layer, std::vector<Point2D> vertices, HatchPattern pattern, double patternScale,
                double patternAngle)
        : Entity(id, layer), m_vertices(std::move(vertices)), m_pattern(pattern), m_patternScale(patternScale),
          m_patternAngle(patternAngle) {}

    const std::vector<Point2D>& vertices() const { return m_vertices; }

    HatchPattern pattern() const { return m_pattern; }
    double patternScale() const { return m_patternScale; }
    double patternAngle() const { return m_patternAngle; } // radians, added to each family's own angle

    // GRADIENT fill: set means the boundary fills with a gradient from the
    // entity's resolved color (colorOverride, or its layer's) to
    // gradientColor2, shaped by gradientPreset and angled by patternAngle,
    // instead of a flat SOLID fill. One-color gradients (AutoCAD lets you
    // pick a single color plus a tint/shade slider) aren't a distinct mode
    // here -- GradientCommand just computes gradientColor2 from the tint
    // fraction up front, so this entity always ends up with two real colors.
    const std::optional<Color>& gradientColor2() const { return m_gradientColor2; }
    void setGradientColor2(std::optional<Color> color) { m_gradientColor2 = color; }
    bool isGradient() const { return m_gradientColor2.has_value(); }
    GradientPreset gradientPreset() const { return m_gradientPreset; }
    void setGradientPreset(GradientPreset preset) { m_gradientPreset = preset; }

    // The pattern's line work clipped to the boundary (even-odd), as world
    // segments -- what the renderer draws for non-solid patterns. Capped to a
    // sane segment count so absurd scales can't hang the app.
    std::vector<std::pair<Point2D, Point2D>> patternSegments() const;

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
    HatchPattern m_pattern = HatchPattern::Solid;
    double m_patternScale = 1.0;
    double m_patternAngle = 0.0;
    std::optional<Color> m_gradientColor2;
    GradientPreset m_gradientPreset = GradientPreset::Linear;
};

} // namespace lcad
