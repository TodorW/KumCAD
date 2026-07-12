#pragma once

#include "commands/DrawCommand.h"
#include "core/document/Document.h"
#include "core/geometry/Hatch.h"

#include <vector>

// AutoCAD GRADIENT: like HATCH (pre-selected closed polyline, or pick
// internal points), but fills with a gradient instead of a pattern, shaped
// by one of nine named presets (GradientPreset) and either one color (plus
// a tint/shade percentage AutoCAD's dialog calls "Shade/Tint" -- blended
// toward white) or two explicit colors. Colors are entered as AutoCAD Color
// Index numbers (1-255), the same palette DXF ACI colors already use
// elsewhere in KumCAD.
class GradientCommand : public DrawCommand {
public:
    GradientCommand(lcad::Document& document, std::vector<lcad::EntityId> ids)
        : m_document(document), m_ids(std::move(ids)),
          m_stage(m_ids.empty() ? Stage::BoundaryPick : Stage::Preset) {}

    QString start() override;
    std::optional<QString> onPoint(const lcad::Point2D& pt) override;
    std::optional<QString> onScalar(double value) override;
    std::optional<QString> onOption(const QString& option) override;
    bool requestFinish() override;
    std::optional<QString> resultMessage() const override {
        if (m_finished) return m_result;
        if (m_stage == Stage::Preset) return presetPrompt();
        return std::nullopt;
    }
    bool isFinished() const override { return m_finished; }
    void cancel() override { m_finished = true; }

private:
    enum class Stage { BoundaryPick, Preset, OneColorChoice, Color1, Color2, Tint, Angle };

    QString presetPrompt() const;
    void commit();

    lcad::Document& m_document;
    std::vector<lcad::EntityId> m_ids;
    std::vector<std::vector<lcad::Point2D>> m_pickedBoundaries;
    Stage m_stage;
    lcad::GradientPreset m_preset = lcad::GradientPreset::Linear;
    bool m_oneColor = false;
    int m_aci1 = 5;
    int m_aci2 = 150;
    double m_tintPercent = 50.0; // one-color mode: how far from color1 toward white
    double m_angleDeg = 0.0;
    std::optional<QString> m_result;
    bool m_finished = false;
};
