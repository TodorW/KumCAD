#include "commands/PolylineCommand.h"

#include "core/document/Commands.h"
#include "core/geometry/Polyline.h"

QString PolylineCommand::start() {
    return QStringLiteral("PLINE  Specify first point:");
}

std::optional<QString> PolylineCommand::onPoint(const lcad::Point2D& pt) {
    m_points.push_back(pt);
    return QStringLiteral("Specify next point or [Enter to finish]:");
}

void PolylineCommand::onPreviewPoint(const lcad::Point2D& pt) {
    m_previewPoint = pt;
    m_hasPreview = true;
}

std::vector<std::pair<lcad::Point2D, lcad::Point2D>> PolylineCommand::previewSegments() const {
    std::vector<std::pair<lcad::Point2D, lcad::Point2D>> segs;
    for (std::size_t i = 0; i + 1 < m_points.size(); ++i) segs.emplace_back(m_points[i], m_points[i + 1]);
    if (!m_points.empty() && m_hasPreview) segs.emplace_back(m_points.back(), m_previewPoint);
    return segs;
}

bool PolylineCommand::requestFinish() {
    m_finished = true;
    if (m_points.size() < 2) return false;

    const auto id = m_document.reserveEntityId();
    auto entity = std::make_unique<lcad::PolylineEntity>(id, m_document.currentLayer(), m_points, false);
    m_document.commandStack().execute(std::make_unique<lcad::AddEntityCommand>(m_document, std::move(entity)));
    return true;
}
