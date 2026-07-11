#pragma once

#include "commands/DrawCommand.h"
#include "core/document/Document.h"

#include <QStringList>

#include <vector>

// AutoCAD-style MTEXT: first corner, opposite corner (fixes the wrapping
// width and text height at 1/40 of the box width, like MTEXT's default),
// then line-by-line content -- each Enter starts a new line, an empty Enter
// finishes. Empty content cancels without creating anything.
class MTextCommand : public DrawCommand {
public:
    explicit MTextCommand(lcad::Document& document) : m_document(document) {}

    QString start() override;
    std::optional<QString> onPoint(const lcad::Point2D& pt) override;
    std::optional<QString> onScalar(double value) override;
    void onPreviewPoint(const lcad::Point2D& pt) override;
    std::vector<std::pair<lcad::Point2D, lcad::Point2D>> previewSegments() const override;
    // Height also takes free text so a bare Enter can accept the default.
    bool wantsTextInput() const override { return m_stage == Stage::Height || m_stage == Stage::Content; }
    std::optional<QString> onText(const QString& text) override;
    bool isFinished() const override { return m_finished; }
    void cancel() override { m_finished = true; }

private:
    enum class Stage { FirstCorner, OppositeCorner, Height, Content };

    void commit();

    lcad::Document& m_document;
    Stage m_stage = Stage::FirstCorner;
    lcad::Point2D m_corner1;
    lcad::Point2D m_topLeft;
    double m_width = 0.0;
    double m_height = 0.0;
    QStringList m_lines;
    lcad::Point2D m_previewPoint;
    bool m_hasPreview = false;
    bool m_finished = false;
};
