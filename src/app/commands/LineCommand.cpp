#include "commands/LineCommand.h"

#include "core/document/Commands.h"
#include "core/geometry/Line.h"

QString LineCommand::start() {
    return QStringLiteral("LINE  Specify first point:");
}

std::optional<QString> LineCommand::onPoint(const lcad::Point2D& pt) {
    m_points.push_back(pt);
    if (m_points.size() >= 2) {
        const auto id = m_document.reserveEntityId();
        auto entity = std::make_unique<lcad::LineEntity>(
            id, m_document.currentLayer(), m_points[m_points.size() - 2], m_points.back());
        m_document.commandStack().execute(std::make_unique<lcad::AddEntityCommand>(m_document, std::move(entity)));
    }
    return QStringLiteral("Specify next point or [Enter to finish]:");
}

void LineCommand::onPreviewPoint(const lcad::Point2D& pt) {
    m_previewPoint = pt;
    m_hasPreview = true;
}

std::vector<std::pair<lcad::Point2D, lcad::Point2D>> LineCommand::previewSegments() const {
    if (!m_points.empty() && m_hasPreview) return {{m_points.back(), m_previewPoint}};
    return {};
}

bool LineCommand::requestFinish() {
    m_finished = true;
    return !m_points.empty();
}
