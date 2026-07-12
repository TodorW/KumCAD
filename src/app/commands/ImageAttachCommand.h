#pragma once

#include "commands/DrawCommand.h"
#include "core/document/Document.h"

// AutoCAD IMAGEATTACH (simplified): file path, insertion point, then a
// width in world units (height follows the image's own aspect ratio). No
// image manager dialog, brightness/contrast/fade, or clipping boundary.
class ImageAttachCommand : public DrawCommand {
public:
    explicit ImageAttachCommand(lcad::Document& document) : m_document(document) {}

    QString start() override { return QStringLiteral("IMAGEATTACH  Enter image file path:"); }
    bool wantsTextInput() const override { return m_stage == Stage::Path; }
    std::optional<QString> onText(const QString& text) override;
    std::optional<QString> onPoint(const lcad::Point2D& pt) override;
    std::optional<QString> onScalar(double value) override;
    bool requestFinish() override;
    std::optional<QString> resultMessage() const override { return m_result; }
    bool isFinished() const override { return m_finished; }
    void cancel() override { m_finished = true; }

private:
    enum class Stage { Path, Position, Width };

    lcad::Document& m_document;
    Stage m_stage = Stage::Path;
    QString m_path;
    lcad::Point2D m_position;
    double m_aspectRatio = 1.0; // height / width, from the image's own pixel size
    std::optional<QString> m_result;
    bool m_finished = false;
};
