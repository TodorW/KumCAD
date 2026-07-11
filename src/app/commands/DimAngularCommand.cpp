#include "commands/DimAngularCommand.h"

#include "core/document/Commands.h"
#include "core/geometry/Dimension.h"

#include <cmath>

QString DimAngularCommand::start() {
    return QStringLiteral("DIMANGULAR  Specify angle vertex:");
}

std::optional<QString> DimAngularCommand::onPoint(const lcad::Point2D& pt) {
    switch (m_stage) {
    case Stage::Vertex:
        m_vertex = pt;
        m_stage = Stage::FirstRay;
        return QStringLiteral("Specify first angle endpoint:");
    case Stage::FirstRay:
        if (pt.distanceTo(m_vertex) < 1e-9) return QStringLiteral("Specify first angle endpoint:");
        m_p1 = pt;
        m_stage = Stage::SecondRay;
        return QStringLiteral("Specify second angle endpoint:");
    case Stage::SecondRay:
        if (pt.distanceTo(m_vertex) < 1e-9) return QStringLiteral("Specify second angle endpoint:");
        m_p2 = pt;
        m_stage = Stage::ArcPosition;
        return QStringLiteral("Specify dimension arc line location:");
    case Stage::ArcPosition:
        break;
    }

    const auto id = m_document.reserveEntityId();
    auto entity = std::make_unique<lcad::DimensionEntity>(id, m_document.currentLayer(),
                                                          lcad::DimensionKind::Angular, m_p1, m_p2, pt, m_vertex);
    const lcad::DimStyle& style = m_document.dimStyle();
    entity->setStyle(style.textHeight, style.arrowSize, style.decimals);
    const std::string label = entity->geometry().label;
    m_document.commandStack().execute(std::make_unique<lcad::AddEntityCommand>(m_document, std::move(entity)));
    m_finished = true;
    return QStringLiteral("Angle = %1").arg(QString::fromUtf8(label.c_str()));
}

void DimAngularCommand::onPreviewPoint(const lcad::Point2D& pt) {
    m_previewPoint = pt;
    m_hasPreview = true;
}

std::vector<std::pair<lcad::Point2D, lcad::Point2D>> DimAngularCommand::previewSegments() const {
    if (!m_hasPreview) return {};
    switch (m_stage) {
    case Stage::FirstRay:
        return {{m_vertex, m_previewPoint}};
    case Stage::SecondRay:
        return {{m_vertex, m_p1}, {m_vertex, m_previewPoint}};
    case Stage::ArcPosition: {
        // Preview the measured arc as chords via a throwaway entity.
        const lcad::DimensionEntity preview(0, 0, lcad::DimensionKind::Angular, m_p1, m_p2, m_previewPoint, m_vertex);
        const auto geo = preview.geometry();
        std::vector<std::pair<lcad::Point2D, lcad::Point2D>> segs{{m_vertex, m_p1}, {m_vertex, m_p2}};
        const int steps = 24;
        lcad::Point2D prev = geo.dimA;
        for (int i = 1; i <= steps; ++i) {
            const double angle =
                geo.arcStartAngle + (geo.arcEndAngle - geo.arcStartAngle) * i / steps;
            const lcad::Point2D next =
                geo.arcCenter + lcad::Point2D(std::cos(angle), std::sin(angle)) * geo.arcRadius;
            segs.emplace_back(prev, next);
            prev = next;
        }
        return segs;
    }
    default:
        return {};
    }
}
