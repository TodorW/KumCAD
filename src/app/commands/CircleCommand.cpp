#include "commands/CircleCommand.h"

#include "core/document/Commands.h"
#include "core/geometry/Circle.h"

QString CircleCommand::start() {
    return QStringLiteral("CIRCLE  Specify center point:");
}

std::optional<QString> CircleCommand::onPoint(const lcad::Point2D& pt) {
    if (!m_hasCenter) {
        m_center = pt;
        m_hasCenter = true;
        return QStringLiteral("Specify radius point:");
    }

    const double radius = pt.distanceTo(m_center);
    const auto id = m_document.reserveEntityId();
    auto entity = std::make_unique<lcad::CircleEntity>(id, m_document.currentLayer(), m_center, radius);
    m_document.commandStack().execute(std::make_unique<lcad::AddEntityCommand>(m_document, std::move(entity)));
    m_finished = true;
    return std::nullopt;
}

void CircleCommand::onPreviewPoint(const lcad::Point2D& pt) {
    m_previewPoint = pt;
    m_hasPreview = true;
}

std::vector<std::pair<lcad::Point2D, lcad::Point2D>> CircleCommand::previewSegments() const {
    if (m_hasCenter && m_hasPreview) return {{m_center, m_previewPoint}};
    return {};
}
