#include "commands/FieldCommand.h"

#include "core/document/Commands.h"
#include "core/document/Fields.h"
#include "core/geometry/Text.h"

QString FieldCommand::start() {
    return QStringLiteral("FIELD  Specify insertion point:");
}

std::optional<QString> FieldCommand::onPoint(const lcad::Point2D& pt) {
    if (m_stage == Stage::InsertionPoint) {
        m_position = pt;
        m_stage = Stage::Height;
        return QStringLiteral("Specify height:");
    }
    if (m_stage == Stage::Height) {
        const double h = m_position.distanceTo(pt);
        if (h < 1e-6) return QStringLiteral("Specify height:"); // degenerate pick, re-prompt
        m_height = h;
        m_stage = Stage::Content;
        return QStringLiteral("Enter field template (e.g. {{DATE}}, {{ATTR:R1.VALUE}}):");
    }
    return std::nullopt;
}

std::optional<QString> FieldCommand::onScalar(double value) {
    if (m_stage != Stage::Height || value < 1e-6) return std::nullopt;
    m_height = value;
    m_stage = Stage::Content;
    return QStringLiteral("Enter field template (e.g. {{DATE}}, {{ATTR:R1.VALUE}}):");
}

void FieldCommand::onPreviewPoint(const lcad::Point2D& pt) {
    m_previewPoint = pt;
    m_hasPreview = true;
}

std::vector<std::pair<lcad::Point2D, lcad::Point2D>> FieldCommand::previewSegments() const {
    if (m_stage == Stage::Height && m_hasPreview) return {{m_position, m_previewPoint}};
    return {};
}

std::optional<QString> FieldCommand::onText(const QString& text) {
    const QString trimmed = text.trimmed();
    if (!trimmed.isEmpty()) {
        const std::string tmpl = trimmed.toStdString();
        const std::string resolved = lcad::evaluateFieldTemplate(m_document, tmpl, m_fileName.toStdString());
        const auto id = m_document.reserveEntityId();
        auto entity = std::make_unique<lcad::TextEntity>(id, m_document.currentLayer(), m_position, resolved, m_height);
        entity->setStyleName(m_document.currentTextStyleName());
        entity->setFieldTemplate(tmpl);
        m_document.commandStack().execute(std::make_unique<lcad::AddEntityCommand>(m_document, std::move(entity)));
    }
    m_finished = true;
    return std::nullopt;
}
