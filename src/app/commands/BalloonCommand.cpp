#include "commands/BalloonCommand.h"

#include "core/document/Commands.h"
#include "core/geometry/Circle.h"
#include "core/geometry/Leader.h"
#include "core/geometry/Text.h"

QString BalloonCommand::start() {
    return QStringLiteral("BALLOON  Specify leader arrowhead location:");
}

std::optional<QString> BalloonCommand::onPoint(const lcad::Point2D& pt) {
    if (m_stage == Stage::Attach) {
        m_attachPoint = pt;
        m_stage = Stage::Bubble;
        return QStringLiteral("Specify bubble location:");
    }
    if (m_stage == Stage::Bubble) {
        m_bubbleCenter = pt;
        m_stage = Stage::Text;
        return QStringLiteral("Enter balloon text:");
    }
    return std::nullopt; // Text stage takes typed input only
}

void BalloonCommand::onPreviewPoint(const lcad::Point2D& pt) {
    m_previewPoint = pt;
    m_hasPreview = true;
}

std::vector<std::pair<lcad::Point2D, lcad::Point2D>> BalloonCommand::previewSegments() const {
    if (m_stage == Stage::Bubble && m_hasPreview) return {{m_attachPoint, m_previewPoint}};
    return {};
}

std::optional<QString> BalloonCommand::onText(const QString& text) {
    m_finished = true;
    const lcad::DimStyle& style = m_document.dimStyle();
    const double radius = 1.2 * style.textHeight;
    const std::string content = text.trimmed().isEmpty() ? std::string("1") : text.trimmed().toStdString();

    auto batch = std::make_unique<lcad::BatchCommand>("Balloon");
    batch->add(std::make_unique<lcad::AddEntityCommand>(
        m_document, std::make_unique<lcad::LeaderEntity>(m_document.reserveEntityId(), m_document.currentLayer(),
                                                          std::vector<lcad::Point2D>{m_attachPoint, m_bubbleCenter},
                                                          style.arrowSize)));
    batch->add(std::make_unique<lcad::AddEntityCommand>(
        m_document, std::make_unique<lcad::CircleEntity>(m_document.reserveEntityId(), m_document.currentLayer(),
                                                          m_bubbleCenter, radius)));

    // A simple character-count-based centering estimate -- TextEntity has
    // no true centered-justification mode (see this file's own header
    // comment).
    const double approxHalfWidth = 0.3 * style.textHeight * static_cast<double>(content.size());
    const lcad::Point2D textPos(m_bubbleCenter.x - approxHalfWidth, m_bubbleCenter.y - 0.35 * style.textHeight);
    batch->add(std::make_unique<lcad::AddEntityCommand>(
        m_document, std::make_unique<lcad::TextEntity>(m_document.reserveEntityId(), m_document.currentLayer(),
                                                        textPos, content, style.textHeight)));

    m_document.commandStack().execute(std::move(batch));
    return std::nullopt;
}
