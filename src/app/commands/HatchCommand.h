#pragma once

#include "commands/DrawCommand.h"
#include "core/document/Document.h"
#include "core/geometry/HatchPattern.h"

#include <vector>

// AutoCAD-style HATCH over the current selection: prompts for a pattern
// (SOLID or an ANSI line pattern), then scale and angle for patterns, and
// fills every selected closed polyline. One undo step. Enter accepts the
// defaults for all remaining prompts.
class HatchCommand : public DrawCommand {
public:
    HatchCommand(lcad::Document& document, std::vector<lcad::EntityId> ids)
        : m_document(document), m_ids(std::move(ids)) {}

    QString start() override;
    std::optional<QString> onPoint(const lcad::Point2D& pt) override;
    std::optional<QString> onOption(const QString& option) override;
    std::optional<QString> onScalar(double value) override;
    bool requestFinish() override;
    std::optional<QString> resultMessage() const override { return m_result; }
    bool isFinished() const override { return m_finished; }
    void cancel() override { m_finished = true; }

private:
    enum class Stage { Pattern, Scale, Angle };

    void commit();

    lcad::Document& m_document;
    std::vector<lcad::EntityId> m_ids;
    Stage m_stage = Stage::Pattern;
    lcad::HatchPattern m_pattern = lcad::HatchPattern::Solid;
    double m_scale = 1.0;
    double m_angleDeg = 0.0;
    std::optional<QString> m_result;
    bool m_finished = false;
};
