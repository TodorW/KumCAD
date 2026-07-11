#pragma once

#include "commands/DrawCommand.h"
#include "core/document/Document.h"

// DIMRADIUS / DIMDIAMETER: pick a circle or arc, then drag the label into
// place along the ray from the center through the pick. The curve point
// follows the drag direction, like AutoCAD.
class DimRadialCommand : public DrawCommand {
public:
    DimRadialCommand(lcad::Document& document, bool diameter, double pickTolerance)
        : m_document(document), m_diameter(diameter), m_tolerance(pickTolerance) {}

    QString start() override;
    std::optional<QString> onPoint(const lcad::Point2D& pt) override;
    void onPreviewPoint(const lcad::Point2D& pt) override;
    std::vector<std::pair<lcad::Point2D, lcad::Point2D>> previewSegments() const override;
    bool isFinished() const override { return m_finished; }
    void cancel() override { m_finished = true; }

private:
    enum class Stage { Select, Position };

    lcad::Point2D curvePointToward(const lcad::Point2D& target) const;

    lcad::Document& m_document;
    bool m_diameter;
    double m_tolerance;
    Stage m_stage = Stage::Select;
    lcad::Point2D m_center;
    double m_radius = 0.0;
    lcad::Point2D m_previewPoint;
    bool m_hasPreview = false;
    bool m_finished = false;
};
