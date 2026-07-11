#pragma once

#include "commands/DrawCommand.h"
#include "core/document/Document.h"

// BPARAMETER: makes an existing block dynamic with a linear stretch
// parameter (AutoCAD's Block Editor, simplified into one command instead of
// a separate editing mode -- see DynamicLinearParameter). Prompts for the
// block name, then the parameter's base and end points, then the two
// corners of the stretch frame (a crossing window: child vertices inside it
// move with the end-point grip, like STRETCH). Not undoable, like other
// block/layer management commands (BLOCK, PURGE).
class BlockParamCommand : public DrawCommand {
public:
    explicit BlockParamCommand(lcad::Document& document) : m_document(document) {}

    QString start() override { return QStringLiteral("BPARAMETER  Enter block name to make dynamic:"); }
    bool wantsTextInput() const override { return m_stage == Stage::BlockName; }
    std::optional<QString> onText(const QString& text) override;
    std::optional<QString> onPoint(const lcad::Point2D& pt) override;
    bool isFinished() const override { return m_finished; }
    void cancel() override { m_finished = true; }

private:
    enum class Stage { BlockName, BasePoint, EndPoint, FrameCorner1, FrameCorner2 };

    lcad::Document& m_document;
    Stage m_stage = Stage::BlockName;
    lcad::BlockDefinition* m_block = nullptr;
    lcad::Point2D m_basePoint;
    lcad::Point2D m_endPoint;
    lcad::Point2D m_frameCorner1;
    bool m_finished = false;
};
