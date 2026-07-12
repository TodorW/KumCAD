#pragma once

#include "commands/DrawCommand.h"
#include "core/document/Document.h"

// AutoCAD IMAGEATTACH (simplified): file path, insertion point, then a
// width in world units (height follows the image's own aspect ratio). No
// image manager dialog, brightness/contrast/fade, or clipping boundary.
//
// Also doubles as PDFATTACH when the path ends in ".pdf" (needs this build
// to have Poppler-Qt6; see LCAD_HAS_PDF): rasterizes one page as the
// underlay, prompting for which page first if the PDF has more than one.
// No vector rendering and no per-layer visibility within the PDF -- see
// ImageEntity's header comment for the full disclosure.
class ImageAttachCommand : public DrawCommand {
public:
    explicit ImageAttachCommand(lcad::Document& document) : m_document(document) {}

    QString start() override { return QStringLiteral("IMAGEATTACH  Enter image or PDF file path:"); }
    bool wantsTextInput() const override { return m_stage == Stage::Path; }
    std::optional<QString> onText(const QString& text) override;
    std::optional<QString> onPoint(const lcad::Point2D& pt) override;
    std::optional<QString> onScalar(double value) override;
    bool requestFinish() override;
    std::optional<QString> resultMessage() const override { return m_result; }
    bool isFinished() const override { return m_finished; }
    void cancel() override { m_finished = true; }

private:
    enum class Stage { Path, Page, Position, Width };

    std::optional<QString> choosePage(int pageIndex);

    lcad::Document& m_document;
    Stage m_stage = Stage::Path;
    QString m_path;
    lcad::Point2D m_position;
    double m_aspectRatio = 1.0; // height / width, from the source's own page/pixel size
    int m_pdfPageCount = 0;     // >0 while Stage::Page is choosing among this many PDF pages
    int m_pdfPage = 0;
    std::optional<QString> m_result;
    bool m_finished = false;
};
