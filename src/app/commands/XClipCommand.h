#pragma once

#include "commands/DrawCommand.h"
#include "core/document/Document.h"

#include <vector>

// AutoCAD-style XCLIP: clips a single block/xref Insert's rendered geometry
// to a boundary. Real AutoCAD's command works on any block reference, not
// just external references, and this mirrors that (matching this codebase's
// InsertEntity, which represents both). ON/OFF toggle the existing boundary
// without discarding it; Delete clears it; New (the default) prompts for a
// Rectangular (two corners, the default) or Polygonal (multi-point, Enter
// to close) boundary.
class XClipCommand : public DrawCommand {
public:
    XClipCommand(lcad::Document& document, lcad::EntityId targetId);

    QString start() override;
    std::optional<QString> onPoint(const lcad::Point2D& pt) override;
    std::optional<QString> onOption(const QString& option) override;
    bool requestFinish() override;
    std::optional<QString> resultMessage() const override { return m_result; }
    bool isFinished() const override { return m_finished; }
    void cancel() override { m_finished = true; }

private:
    enum class Stage { Option, BoundaryKind, RectFirst, RectSecond, Polygon };

    void applyClip(std::vector<lcad::Point2D> boundary, bool enabled, const QString& message);
    void finishWith(const QString& message);

    lcad::Document& m_document;
    lcad::EntityId m_targetId;
    Stage m_stage = Stage::Option;
    lcad::Point2D m_rectFirst;
    std::vector<lcad::Point2D> m_polyPoints;
    std::optional<QString> m_result;
    bool m_finished = false;
};
