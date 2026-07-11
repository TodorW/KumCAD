#include "commands/DimRadialCommand.h"

#include "core/document/Commands.h"
#include "core/geometry/Arc.h"
#include "core/geometry/Circle.h"
#include "core/geometry/Dimension.h"

QString DimRadialCommand::start() {
    return QStringLiteral("%1  Select arc or circle:")
        .arg(m_diameter ? QStringLiteral("DIMDIAMETER") : QStringLiteral("DIMRADIUS"));
}

lcad::Point2D DimRadialCommand::curvePointToward(const lcad::Point2D& target) const {
    const lcad::Point2D span = target - m_center;
    const double len = span.length();
    const lcad::Point2D dir = len > 1e-12 ? span * (1.0 / len) : lcad::Point2D(1, 0);
    return m_center + dir * m_radius;
}

std::optional<QString> DimRadialCommand::onPoint(const lcad::Point2D& pt) {
    if (m_stage == Stage::Select) {
        const lcad::Entity* best = nullptr;
        double bestDist = m_tolerance;
        for (const lcad::Entity* e : m_document.entities()) {
            if (e->type() != lcad::EntityType::Circle && e->type() != lcad::EntityType::Arc) continue;
            const double d = e->distanceTo(pt);
            if (d <= bestDist) {
                bestDist = d;
                best = e;
            }
        }
        if (!best) return QStringLiteral("*No arc or circle there* Select arc or circle:");
        if (best->type() == lcad::EntityType::Circle) {
            const auto* circle = static_cast<const lcad::CircleEntity*>(best);
            m_center = circle->center();
            m_radius = circle->radius();
        } else {
            const auto* arc = static_cast<const lcad::ArcEntity*>(best);
            m_center = arc->center();
            m_radius = arc->radius();
        }
        m_stage = Stage::Position;
        return QStringLiteral("Specify dimension line location:");
    }

    const auto id = m_document.reserveEntityId();
    auto entity = std::make_unique<lcad::DimensionEntity>(
        id, m_document.currentLayer(), m_diameter ? lcad::DimensionKind::Diameter : lcad::DimensionKind::Radius,
        m_center, curvePointToward(pt), pt);
    const lcad::DimStyle& style = m_document.dimStyle();
    entity->setStyle(style.textHeight, style.arrowSize, style.decimals);
    const std::string label = entity->geometry().label;
    m_document.commandStack().execute(std::make_unique<lcad::AddEntityCommand>(m_document, std::move(entity)));
    m_finished = true;
    return QStringLiteral("Dimension = %1").arg(QString::fromUtf8(label.c_str()));
}

void DimRadialCommand::onPreviewPoint(const lcad::Point2D& pt) {
    m_previewPoint = pt;
    m_hasPreview = true;
}

std::vector<std::pair<lcad::Point2D, lcad::Point2D>> DimRadialCommand::previewSegments() const {
    if (m_stage != Stage::Position || !m_hasPreview) return {};
    const lcad::Point2D onCurve = curvePointToward(m_previewPoint);
    if (m_diameter) return {{m_center * 2.0 - onCurve, onCurve}, {onCurve, m_previewPoint}};
    return {{m_center, onCurve}, {onCurve, m_previewPoint}};
}
