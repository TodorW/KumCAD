#pragma once

#include "commands/DrawCommand.h"
#include "core/document/Document.h"

// AutoCAD-style INSERT: name an existing block, pick an insertion point.
// Scale/rotation default to 1/0; use SCALE and ROTATE afterwards.
class InsertCommand : public DrawCommand {
public:
    explicit InsertCommand(lcad::Document& document) : m_document(document) {}

    QString start() override;
    std::optional<QString> onPoint(const lcad::Point2D& pt) override;
    bool wantsTextInput() const override { return m_stage == Stage::Name; }
    std::optional<QString> onText(const QString& text) override;
    bool isFinished() const override { return m_finished; }
    void cancel() override { m_finished = true; }

private:
    enum class Stage { Name, Position };

    QString availableBlocks() const;

    lcad::Document& m_document;
    Stage m_stage = Stage::Name;
    const lcad::BlockDefinition* m_block = nullptr;
    bool m_finished = false;
};
