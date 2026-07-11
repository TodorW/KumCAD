#include "commands/LeaderCommand.h"

#include "core/document/Commands.h"
#include "core/geometry/Leader.h"
#include "core/geometry/MText.h"

QString LeaderCommand::start() {
    return QStringLiteral("LEADER  Specify arrowhead point:");
}

std::optional<QString> LeaderCommand::onPoint(const lcad::Point2D& pt) {
    if (m_stage != Stage::Points) return std::nullopt;
    m_points.push_back(pt);
    if (m_points.size() == 1) return QStringLiteral("Specify next point:");
    return QStringLiteral("Specify next point or [Enter for annotation]:");
}

bool LeaderCommand::requestFinish() {
    if (m_stage == Stage::Points) {
        if (m_points.size() < 2) {
            m_finished = true;
            return false;
        }
        // Enter after the points: move on to the annotation. Returning
        // without finishing isn't something requestFinish supports, so we
        // flip the stage and let the dispatcher keep routing text to us.
        m_stage = Stage::Text;
        return true;
    }
    commit();
    m_finished = true;
    return true;
}

std::optional<QString> LeaderCommand::onText(const QString& text) {
    if (!text.isEmpty()) {
        m_lines << text;
        return QStringLiteral("Enter next annotation line (empty line to finish):");
    }
    commit();
    m_finished = true;
    return std::nullopt;
}

void LeaderCommand::commit() {
    if (m_points.size() < 2) return;
    const lcad::DimStyle& style = m_document.dimStyle();

    auto batch = std::make_unique<lcad::BatchCommand>("Leader");
    batch->add(std::make_unique<lcad::AddEntityCommand>(
        m_document, std::make_unique<lcad::LeaderEntity>(m_document.reserveEntityId(), m_document.currentLayer(),
                                                         m_points, style.arrowSize)));

    if (!m_lines.isEmpty()) {
        // Annotation sits just past the landing, on the side the leader ran.
        const lcad::Point2D landing = m_points.back();
        const bool leftward = m_points.size() >= 2 && m_points[m_points.size() - 2].x > landing.x;
        const double gap = 0.6 * style.textHeight;
        const QString content = m_lines.join(QLatin1Char('\n'));
        auto mtext = std::make_unique<lcad::MTextEntity>(
            m_document.reserveEntityId(), m_document.currentLayer(),
            lcad::Point2D(landing.x + (leftward ? -gap : gap), landing.y + 0.8 * style.textHeight),
            content.toStdString(), style.textHeight);
        if (leftward) mtext->translate(lcad::Point2D(-mtext->blockWidth(), 0));
        batch->add(std::make_unique<lcad::AddEntityCommand>(m_document, std::move(mtext)));
    }
    m_document.commandStack().execute(std::move(batch));
}

void LeaderCommand::onPreviewPoint(const lcad::Point2D& pt) {
    m_previewPoint = pt;
    m_hasPreview = true;
}

std::vector<std::pair<lcad::Point2D, lcad::Point2D>> LeaderCommand::previewSegments() const {
    std::vector<std::pair<lcad::Point2D, lcad::Point2D>> segs;
    for (std::size_t i = 0; i + 1 < m_points.size(); ++i) segs.emplace_back(m_points[i], m_points[i + 1]);
    if (m_stage == Stage::Points && !m_points.empty() && m_hasPreview) {
        segs.emplace_back(m_points.back(), m_previewPoint);
    }
    return segs;
}
