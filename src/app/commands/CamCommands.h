#pragma once

#include "commands/DrawCommand.h"
#include "core/cam/Toolpath.h"
#include "core/document/Document.h"

// GCODE: select a closed polyline profile, then tool diameter, cut side
// (OUTSIDE/INSIDE/ONLINE), and an output file path -- writes G-code for it
// (see core/cam/Toolpath.h, core/cam/GCodeWriter.h). Feed rate, plunge
// rate, cut depth, and safe height use ToolpathParams's defaults; this
// command doesn't prompt for them yet. Only PolylineEntity profiles are
// supported (a HATCH's traced boundary or a drawn PLINE/RECTANG) -- not
// circles or other closed curves directly.
class GCodeExportCommand : public DrawCommand {
public:
    GCodeExportCommand(lcad::Document& document, double pickTolerance)
        : m_document(document), m_pickTolerance(pickTolerance) {}

    QString start() override { return QStringLiteral("GCODE  Select a closed polyline profile:"); }
    std::optional<QString> onPoint(const lcad::Point2D& pt) override;
    bool wantsTextInput() const override { return m_stage != Stage::Pick; }
    std::optional<QString> onText(const QString& text) override;
    bool isFinished() const override { return m_finished; }
    void cancel() override { m_finished = true; }

private:
    enum class Stage { Pick, ToolDiameter, CutSide, Path };
    lcad::Document& m_document;
    double m_pickTolerance;
    Stage m_stage = Stage::Pick;
    lcad::EntityId m_profileId = 0;
    double m_toolDiameter = 1.0;
    lcad::CutSide m_cutSide = lcad::CutSide::OnLine;
    bool m_finished = false;
};
