#pragma once

#include "commands/DrawCommand.h"
#include "core/document/Document.h"

// DIMANGULAR (3-point form): angle vertex, one point on each ray, then the
// arc position, which also picks which of the two angles gets measured.
class DimAngularCommand : public DrawCommand {
public:
    explicit DimAngularCommand(lcad::Document& document) : m_document(document) {}

    QString start() override;
    std::optional<QString> onPoint(const lcad::Point2D& pt) override;
    void onPreviewPoint(const lcad::Point2D& pt) override;
    std::vector<std::pair<lcad::Point2D, lcad::Point2D>> previewSegments() const override;
    std::optional<lcad::Point2D> anchorPoint() const override {
        return m_stage == Stage::Vertex ? std::nullopt : std::optional<lcad::Point2D>(m_vertex);
    }
    bool isFinished() const override { return m_finished; }
    void cancel() override { m_finished = true; }

private:
    enum class Stage { Vertex, FirstRay, SecondRay, ArcPosition };

    lcad::Document& m_document;
    Stage m_stage = Stage::Vertex;
    lcad::Point2D m_vertex;
    lcad::Point2D m_p1;
    lcad::Point2D m_p2;
    lcad::Point2D m_previewPoint;
    bool m_hasPreview = false;
    bool m_finished = false;
};
