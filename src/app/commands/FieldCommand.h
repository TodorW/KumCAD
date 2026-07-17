#pragma once

#include "commands/DrawCommand.h"
#include "core/document/Document.h"

// AutoCAD-style FIELD: insertion point, then height, then a {{...}}-template
// string (see core/document/Fields.h) instead of free text. The template is
// evaluated immediately for display and stored on the resulting TextEntity
// so a later UPDATEFIELD can re-evaluate it. Mirrors TextCommand's own
// staging exactly -- deliberately kept as a separate command rather than
// bolting a "is this a field?" branch onto TextCommand's own onText().
class FieldCommand : public DrawCommand {
public:
    FieldCommand(lcad::Document& document, QString fileName)
        : m_document(document), m_fileName(std::move(fileName)) {}

    QString start() override;
    std::optional<QString> onPoint(const lcad::Point2D& pt) override;
    std::optional<QString> onScalar(double value) override;
    void onPreviewPoint(const lcad::Point2D& pt) override;
    std::vector<std::pair<lcad::Point2D, lcad::Point2D>> previewSegments() const override;
    std::optional<lcad::Point2D> anchorPoint() const override {
        return m_stage != Stage::InsertionPoint ? std::optional<lcad::Point2D>(m_position) : std::nullopt;
    }
    bool wantsTextInput() const override { return m_stage == Stage::Content; }
    std::optional<QString> onText(const QString& text) override;
    bool isFinished() const override { return m_finished; }
    void cancel() override { m_finished = true; }

private:
    enum class Stage { InsertionPoint, Height, Content };

    lcad::Document& m_document;
    QString m_fileName;
    Stage m_stage = Stage::InsertionPoint;
    lcad::Point2D m_position;
    double m_height = 0.0;
    lcad::Point2D m_previewPoint;
    bool m_hasPreview = false;
    bool m_finished = false;
};
