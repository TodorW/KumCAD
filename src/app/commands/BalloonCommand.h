#pragma once

#include "commands/DrawCommand.h"
#include "core/document/Document.h"

// AutoCAD BALLOON: an attach (arrowhead) point, then a bubble location,
// then the balloon's own text (typically an item number). Composes
// three ordinary entities -- a LeaderEntity (attach -> bubble), a
// CircleEntity (the bubble outline), and a TextEntity (its content,
// roughly centered) -- into one undo step, the same "no new EntityType
// needed, reuse what already exists" precedent LeaderCommand's own
// Leader+MText combo already sets, rather than inventing a dedicated
// Balloon entity (which would ripple through every exhaustive
// switch (EntityType) across DXF I/O, rendering, and the properties
// panel for comparatively little real gain over composing existing
// primitives).
//
// Real, disclosed simplification: the bubble radius is a fixed multiple
// of the document's own dimension style text height, not real AutoCAD's
// per-character auto-fit sizing; the text position is a simple
// character-count-based centering estimate (TextEntity has no true
// centered-justification mode to defer to).
class BalloonCommand : public DrawCommand {
public:
    explicit BalloonCommand(lcad::Document& document) : m_document(document) {}

    QString start() override;
    std::optional<QString> onPoint(const lcad::Point2D& pt) override;
    void onPreviewPoint(const lcad::Point2D& pt) override;
    std::vector<std::pair<lcad::Point2D, lcad::Point2D>> previewSegments() const override;
    bool wantsTextInput() const override { return m_stage == Stage::Text; }
    std::optional<QString> onText(const QString& text) override;
    std::optional<lcad::Point2D> anchorPoint() const override {
        return m_stage == Stage::Bubble ? std::optional<lcad::Point2D>(m_attachPoint) : std::nullopt;
    }
    bool isFinished() const override { return m_finished; }
    void cancel() override { m_finished = true; }

private:
    enum class Stage { Attach, Bubble, Text };

    lcad::Document& m_document;
    Stage m_stage = Stage::Attach;
    lcad::Point2D m_attachPoint;
    lcad::Point2D m_bubbleCenter;
    lcad::Point2D m_previewPoint;
    bool m_hasPreview = false;
    bool m_finished = false;
};
