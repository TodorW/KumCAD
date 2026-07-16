#pragma once

#include "commands/DrawCommand.h"
#include "core/document/Document.h"

#include <vector>

// Schematic WIRE: collects vertices as the user clicks, committing a single
// WireEntity (one undo step) when finished via Enter/right-click or Escape,
// same shape as PLINE but always straight -- electrical connections are
// drawn as orthogonal/straight runs, never bulged.
class WireCommand : public DrawCommand {
public:
    explicit WireCommand(lcad::Document& document) : m_document(document) {}

    QString start() override;
    std::optional<QString> onPoint(const lcad::Point2D& pt) override;
    void onPreviewPoint(const lcad::Point2D& pt) override;
    std::vector<std::pair<lcad::Point2D, lcad::Point2D>> previewSegments() const override;
    bool requestFinish() override;
    std::optional<lcad::Point2D> anchorPoint() const override {
        return m_points.empty() ? std::nullopt : std::optional<lcad::Point2D>(m_points.back());
    }
    bool isFinished() const override { return m_finished; }
    void cancel() override { m_finished = true; }

private:
    lcad::Document& m_document;
    std::vector<lcad::Point2D> m_points;
    lcad::Point2D m_previewPoint;
    bool m_hasPreview = false;
    bool m_finished = false;
};
