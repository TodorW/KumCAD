#include "core/geometry/Tolerance.h"

#include <cmath>
#include <sstream>

namespace lcad {

const char* geoCharacteristicSymbol(GeoCharacteristic c) {
    switch (c) {
    case GeoCharacteristic::Straightness: return "[Straightness]";
    case GeoCharacteristic::Flatness: return "[Flatness]";
    case GeoCharacteristic::Circularity: return "[Circularity]";
    case GeoCharacteristic::Cylindricity: return "[Cylindricity]";
    case GeoCharacteristic::ProfileLine: return "[Profile of a Line]";
    case GeoCharacteristic::ProfileSurface: return "[Profile of a Surface]";
    case GeoCharacteristic::Angularity: return "[Angularity]";
    case GeoCharacteristic::Perpendicularity: return "[Perpendicularity]";
    case GeoCharacteristic::Parallelism: return "[Parallelism]";
    case GeoCharacteristic::Position: return "[Position]";
    case GeoCharacteristic::Concentricity: return "[Concentricity]";
    case GeoCharacteristic::Symmetry: return "[Symmetry]";
    case GeoCharacteristic::CircularRunout: return "[Circular Runout]";
    case GeoCharacteristic::TotalRunout: return "[Total Runout]";
    }
    return "[?]";
}

char materialConditionSymbol(MaterialCondition m) {
    switch (m) {
    case MaterialCondition::None: return '\0';
    case MaterialCondition::MMC: return 'M';
    case MaterialCondition::LMC: return 'L';
    }
    return '\0';
}

namespace {
double approximateRowWidth(const ToleranceRow& row, double height) {
    // A rough character-count estimate, the same spirit as TextEntity's
    // own approximateWidth() heuristic.
    std::size_t chars = std::string(geoCharacteristicSymbol(row.characteristic)).size() + 8;
    for (const ToleranceDatumRef& d : row.datums) chars += d.letter.size() + 4;
    return 0.6 * height * static_cast<double>(chars);
}
} // namespace

std::string ToleranceEntity::rowText(const ToleranceRow& row) {
    std::ostringstream out;
    out << geoCharacteristicSymbol(row.characteristic) << ' ';
    if (row.diameterSymbol) out << "DIA ";
    out << row.toleranceValue;
    if (const char mod = materialConditionSymbol(row.toleranceModifier)) out << " (" << mod << ')';
    for (const ToleranceDatumRef& d : row.datums) {
        out << " | " << d.letter;
        if (const char mod = materialConditionSymbol(d.modifier)) out << " (" << mod << ')';
    }
    return out.str();
}

BoundingBox ToleranceEntity::boundingBox() const {
    double maxWidth = 0.0;
    for (const ToleranceRow& row : m_rows) maxWidth = std::max(maxWidth, approximateRowWidth(row, m_textHeight));
    const double height = m_textHeight * std::max<std::size_t>(1, m_rows.size());
    BoundingBox box;
    box.expand(m_position);
    box.expand(Point2D(m_position.x + maxWidth, m_position.y + height));
    return box;
}

double ToleranceEntity::distanceTo(const Point2D& pt) const {
    const BoundingBox box = boundingBox();
    const double dx = pt.x < box.min.x ? box.min.x - pt.x : (pt.x > box.max.x ? pt.x - box.max.x : 0.0);
    const double dy = pt.y < box.min.y ? box.min.y - pt.y : (pt.y > box.max.y ? pt.y - box.max.y : 0.0);
    return std::sqrt(dx * dx + dy * dy);
}

void ToleranceEntity::translate(const Point2D& delta) { m_position = m_position + delta; }

void ToleranceEntity::rotate(const Point2D& center, double angleRadians) {
    m_position = rotateAround(m_position, center, angleRadians);
}

void ToleranceEntity::scale(const Point2D& center, double factor) {
    m_position = scaleAround(m_position, center, factor);
    m_textHeight *= std::abs(factor);
}

void ToleranceEntity::mirror(const Point2D& a, const Point2D& b) { m_position = mirrorAcross(m_position, a, b); }

std::vector<Point2D> ToleranceEntity::gripPoints() const { return {m_position}; }

void ToleranceEntity::moveGripPoint(std::size_t index, const Point2D& newPos) {
    if (index == 0) m_position = newPos;
}

std::vector<SnapPoint> ToleranceEntity::snapCandidates() const { return {{m_position, SnapKind::Endpoint}}; }

std::unique_ptr<Entity> ToleranceEntity::clone() const { return std::make_unique<ToleranceEntity>(*this); }

} // namespace lcad
