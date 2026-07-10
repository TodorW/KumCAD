#include "commands/InsertCommand.h"

#include "core/document/Commands.h"
#include "core/geometry/Insert.h"

#include <QStringList>

QString InsertCommand::availableBlocks() const {
    if (m_document.blocks().empty()) return QStringLiteral("(no blocks defined yet -- use BLOCK first)");
    QStringList names;
    for (const auto& block : m_document.blocks()) names << QString::fromStdString(block->name);
    return names.join(QStringLiteral(", "));
}

QString InsertCommand::start() {
    return QStringLiteral("INSERT  Available blocks: %1\nEnter block name:").arg(availableBlocks());
}

std::optional<QString> InsertCommand::onText(const QString& text) {
    const QString name = text.trimmed();
    if (name.isEmpty()) return QStringLiteral("Enter block name:");
    m_block = m_document.findBlock(name.toStdString());
    if (!m_block) {
        return QStringLiteral("*No block named \"%1\". Available: %2*\nEnter block name:")
            .arg(name, availableBlocks());
    }
    m_stage = Stage::Position;
    return QStringLiteral("Specify insertion point:");
}

std::optional<QString> InsertCommand::onPoint(const lcad::Point2D& pt) {
    if (m_stage != Stage::Position || !m_block) return std::nullopt;
    m_document.commandStack().execute(std::make_unique<lcad::AddEntityCommand>(
        m_document,
        std::make_unique<lcad::InsertEntity>(m_document.reserveEntityId(), m_document.currentLayer(), m_block, pt)));
    m_finished = true;
    return std::nullopt;
}
