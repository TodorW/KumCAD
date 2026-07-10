#pragma once

#include "commands/DrawCommand.h"
#include "core/document/Document.h"

#include <vector>

// AutoCAD-style BLOCK on a pre-existing selection: name it, pick a base
// point, and the selection becomes a block definition with the originals
// replaced by a single insert at that point -- one undoable step.
class BlockCommand : public DrawCommand {
public:
    BlockCommand(lcad::Document& document, std::vector<lcad::EntityId> ids)
        : m_document(document), m_ids(std::move(ids)) {}

    QString start() override;
    std::optional<QString> onPoint(const lcad::Point2D& pt) override;
    bool wantsTextInput() const override { return m_stage == Stage::Name; }
    std::optional<QString> onText(const QString& text) override;
    bool isFinished() const override { return m_finished; }
    void cancel() override { m_finished = true; }

private:
    enum class Stage { Name, BasePoint };

    lcad::Document& m_document;
    std::vector<lcad::EntityId> m_ids;
    Stage m_stage = Stage::Name;
    std::string m_name;
    bool m_finished = false;
};
