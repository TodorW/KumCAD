#include "commands/BlockCommand.h"

#include "core/document/Commands.h"
#include "core/geometry/Insert.h"

QString BlockCommand::start() {
    return QStringLiteral("BLOCK  %1 found\nEnter block name:").arg(static_cast<int>(m_ids.size()));
}

std::optional<QString> BlockCommand::onText(const QString& text) {
    const QString name = text.trimmed();
    if (name.isEmpty()) return QStringLiteral("Enter block name:");
    if (m_document.findBlock(name.toStdString())) {
        return QStringLiteral("*A block named \"%1\" already exists*\nEnter block name:").arg(name);
    }
    m_name = name.toStdString();
    m_stage = Stage::BasePoint;
    return QStringLiteral("Specify base point:");
}

std::optional<QString> BlockCommand::onPoint(const lcad::Point2D& pt) {
    if (m_stage != Stage::BasePoint) return std::nullopt;

    // Clone the selection into the definition, base-point-relative.
    std::vector<std::unique_ptr<lcad::Entity>> children;
    for (lcad::EntityId id : m_ids) {
        const lcad::Entity* source = m_document.findEntity(id);
        if (!source) continue;
        std::unique_ptr<lcad::Entity> copy = source->clone();
        copy->translate(lcad::Point2D(-pt.x, -pt.y));
        children.push_back(std::move(copy));
    }
    if (children.empty()) {
        m_finished = true;
        return QStringLiteral("*Nothing to block*");
    }
    const std::size_t childCount = children.size();
    const lcad::BlockDefinition* block = m_document.addBlock(m_name, std::move(children));

    // Replace the originals with a single insert, as one undo step.
    auto batch = std::make_unique<lcad::BatchCommand>("Block");
    for (lcad::EntityId id : m_ids) {
        if (m_document.findEntity(id)) batch->add(std::make_unique<lcad::DeleteEntityCommand>(m_document, id));
    }
    batch->add(std::make_unique<lcad::AddEntityCommand>(
        m_document,
        std::make_unique<lcad::InsertEntity>(m_document.reserveEntityId(), m_document.currentLayer(), block, pt)));
    m_document.commandStack().execute(std::move(batch));

    m_finished = true;
    return QStringLiteral("*Block \"%1\" created (%2 entities)*")
        .arg(QString::fromStdString(m_name))
        .arg(childCount);
}
