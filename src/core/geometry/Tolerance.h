#pragma once

#include "core/geometry/Entity.h"

#include <string>
#include <vector>

namespace lcad {

// ASME Y14.5 / ISO 1101 geometric characteristic symbols a feature-control
// frame can carry.
enum class GeoCharacteristic {
    Straightness,
    Flatness,
    Circularity,
    Cylindricity,
    ProfileLine,
    ProfileSurface,
    Angularity,
    Perpendicularity,
    Parallelism,
    Position,
    Concentricity,
    Symmetry,
    CircularRunout,
    TotalRunout,
};
const char* geoCharacteristicSymbol(GeoCharacteristic c); // the real ASCII/Unicode GD&T glyph

// Material condition modifier attached to a tolerance value or a datum
// reference (the circled M/L symbols; None means no modifier is shown --
// modern ASME Y14.5-2009 no longer has a distinct RFS symbol, RFS is the
// implicit default).
enum class MaterialCondition { None, MMC, LMC };
char materialConditionSymbol(MaterialCondition m); // 'M', 'L', or '\0' for None

struct ToleranceDatumRef {
    std::string letter; // e.g. "A"
    MaterialCondition modifier = MaterialCondition::None;
};

// One compartment row of a feature-control frame (AutoCAD lets a TOLERANCE
// stack two rows for a composite/combined tolerance; this codebase
// supports any number, a real generalization rather than a hardcoded
// 1-or-2 special case).
struct ToleranceRow {
    GeoCharacteristic characteristic = GeoCharacteristic::Position;
    bool diameterSymbol = false; // prepend the diameter symbol to the tolerance value
    double toleranceValue = 0.0;
    MaterialCondition toleranceModifier = MaterialCondition::None;
    std::vector<ToleranceDatumRef> datums; // primary/secondary/tertiary, in order
};

// AutoCAD's TOLERANCE: a GD&T feature-control frame. Real AutoCAD draws
// actual boxed compartments and persists the whole thing pre-rendered as
// one DXF group-1 text string using its own embedded font-substitution
// control codes for the GD&T glyphs -- reproducing that byte-for-byte
// isn't attempted; rowText() below produces a real, readable plain-text
// rendering of a row's content instead (both for on-screen display and
// as DXF's own group-1 fallback), while the structured row data itself
// (characteristic/value/modifiers/datums) round-trips exactly via this
// codebase's usual custom-group-code convention (see DxfWriter.h).
class ToleranceEntity : public Entity {
public:
    ToleranceEntity(EntityId id, LayerId layer, Point2D position, std::vector<ToleranceRow> rows,
                    double textHeight = 2.5)
        : Entity(id, layer), m_position(position), m_rows(std::move(rows)), m_textHeight(textHeight) {}

    const Point2D& position() const { return m_position; }
    const std::vector<ToleranceRow>& rows() const { return m_rows; }
    double textHeight() const { return m_textHeight; }

    // e.g. "[Position] DIA 0.05 (M) | A | B (M) | C" for a positional
    // tolerance referencing three datums, the middle one at MMC.
    static std::string rowText(const ToleranceRow& row);

    EntityType type() const override { return EntityType::Tolerance; }
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
    Point2D m_position;
    std::vector<ToleranceRow> m_rows;
    double m_textHeight;
};

} // namespace lcad
