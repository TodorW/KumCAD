#pragma once

#include "commands/DrawCommand.h"
#include "core/document/Document.h"

// MVIEW: creates a viewport on the active layout by two paper-space corners.
// The new viewport shows the whole model (zoom-extents fit); adjust with
// VPSCALE afterwards. Layout-mode only.
class MviewCommand : public DrawCommand {
public:
    MviewCommand(lcad::Document& document, int layoutIndex)
        : m_document(document), m_layoutIndex(layoutIndex) {}

    QString start() override { return QStringLiteral("MVIEW  Specify first corner of viewport:"); }

    std::optional<QString> onPoint(const lcad::Point2D& pt) override;
    void onPreviewPoint(const lcad::Point2D& pt) override {
        m_previewPoint = pt;
        m_hasPreview = true;
    }
    std::vector<std::pair<lcad::Point2D, lcad::Point2D>> previewSegments() const override;
    bool isFinished() const override { return m_finished; }
    void cancel() override { m_finished = true; }

private:
    lcad::Document& m_document;
    int m_layoutIndex;
    lcad::Point2D m_corner1;
    bool m_haveCorner1 = false;
    lcad::Point2D m_previewPoint;
    bool m_hasPreview = false;
    bool m_finished = false;
};
