#include "commands/ImageAttachCommand.h"

#include "core/document/Commands.h"
#include "core/geometry/Image.h"

#include <QImageReader>

#include <cmath>

#ifdef LCAD_HAS_PDF
#include <poppler-qt6.h>
#endif

std::optional<QString> ImageAttachCommand::onText(const QString& text) {
    if (m_stage != Stage::Path) return std::nullopt;
    const QString path = text.trimmed();
    if (path.isEmpty()) {
        m_finished = true;
        return QStringLiteral("*Cancelled*");
    }

    if (path.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive)) {
#ifdef LCAD_HAS_PDF
        const auto doc = Poppler::Document::load(path);
        if (!doc || doc->isLocked() || doc->numPages() <= 0) {
            return QStringLiteral("*Could not read \"%1\" as a PDF*\nEnter image or PDF file path:").arg(path);
        }
        m_path = path;
        m_pdfPageCount = doc->numPages();
        if (m_pdfPageCount == 1) return choosePage(0);
        return QStringLiteral("*%1 page(s)*\nEnter page number (1-%1) <1>:").arg(m_pdfPageCount);
#else
        return QStringLiteral(
            "*This build has no PDF support (Poppler-Qt6 not found at build time)*\nEnter image file path:");
#endif
    }

    const QImageReader reader(path);
    const QSize size = reader.size();
    if (!size.isValid() || size.width() <= 0) {
        return QStringLiteral("*Could not read \"%1\" as an image*\nEnter image or PDF file path:").arg(path);
    }
    m_path = path;
    m_pdfPageCount = 0;
    m_aspectRatio = static_cast<double>(size.height()) / size.width();
    m_stage = Stage::Position;
    return QStringLiteral("Specify insertion point (bottom-left):");
}

std::optional<QString> ImageAttachCommand::choosePage(int pageIndex) {
#ifdef LCAD_HAS_PDF
    const auto doc = Poppler::Document::load(m_path);
    const auto page = doc ? doc->page(pageIndex) : nullptr;
    if (!page) return QStringLiteral("*Could not read that page*\nEnter page number (1-%1) <1>:").arg(m_pdfPageCount);
    const QSizeF size = page->pageSizeF();
    m_pdfPage = pageIndex;
    m_aspectRatio = size.height() / size.width();
    m_stage = Stage::Position;
    return QStringLiteral("Specify insertion point (bottom-left):");
#else
    (void)pageIndex;
    return std::nullopt;
#endif
}

std::optional<QString> ImageAttachCommand::onScalar(double value) {
    if (m_stage == Stage::Page) {
        const int page = static_cast<int>(std::lround(value));
        if (page < 1 || page > m_pdfPageCount) {
            return QStringLiteral("*Page must be between 1 and %1*\nEnter page number (1-%1) <1>:")
                .arg(m_pdfPageCount);
        }
        return choosePage(page - 1);
    }
    if (m_stage != Stage::Width) return std::nullopt;
    if (value <= 0) return QStringLiteral("*Width must be positive*");

    auto image = std::make_unique<lcad::ImageEntity>(m_document.reserveEntityId(), m_document.currentLayer(),
                                                      m_path.toStdString(), m_position, value, value * m_aspectRatio,
                                                      0.0, m_pdfPage);
    m_document.commandStack().execute(std::make_unique<lcad::AddEntityCommand>(m_document, std::move(image)));
    m_finished = true;
    m_result = QStringLiteral("*Image attached*");
    return m_result;
}

std::optional<QString> ImageAttachCommand::onPoint(const lcad::Point2D& pt) {
    if (m_stage != Stage::Position) return std::nullopt;
    m_position = pt;
    m_stage = Stage::Width;
    return QStringLiteral("Specify width <10>:");
}

bool ImageAttachCommand::requestFinish() {
    if (m_stage == Stage::Page) {
        onScalar(1.0); // the "<1>" default
        return true;
    }
    if (m_stage == Stage::Width) {
        onScalar(10.0); // the "<10>" default
        return true;
    }
    m_finished = true;
    return false;
}
