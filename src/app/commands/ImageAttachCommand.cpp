#include "commands/ImageAttachCommand.h"

#include "core/document/Commands.h"
#include "core/geometry/Image.h"

#include <QImageReader>

std::optional<QString> ImageAttachCommand::onText(const QString& text) {
    if (m_stage != Stage::Path) return std::nullopt;
    const QString path = text.trimmed();
    if (path.isEmpty()) {
        m_finished = true;
        return QStringLiteral("*Cancelled*");
    }
    const QImageReader reader(path);
    const QSize size = reader.size();
    if (!size.isValid() || size.width() <= 0) {
        return QStringLiteral("*Could not read \"%1\" as an image*\nEnter image file path:").arg(path);
    }
    m_path = path;
    m_aspectRatio = static_cast<double>(size.height()) / size.width();
    m_stage = Stage::Position;
    return QStringLiteral("Specify insertion point (bottom-left):");
}

std::optional<QString> ImageAttachCommand::onPoint(const lcad::Point2D& pt) {
    if (m_stage != Stage::Position) return std::nullopt;
    m_position = pt;
    m_stage = Stage::Width;
    return QStringLiteral("Specify width <10>:");
}

std::optional<QString> ImageAttachCommand::onScalar(double value) {
    if (m_stage != Stage::Width) return std::nullopt;
    if (value <= 0) return QStringLiteral("*Width must be positive*");

    auto image = std::make_unique<lcad::ImageEntity>(m_document.reserveEntityId(), m_document.currentLayer(),
                                                      m_path.toStdString(), m_position, value, value * m_aspectRatio);
    m_document.commandStack().execute(std::make_unique<lcad::AddEntityCommand>(m_document, std::move(image)));
    m_finished = true;
    m_result = QStringLiteral("*Image attached*");
    return m_result;
}

bool ImageAttachCommand::requestFinish() {
    if (m_stage == Stage::Width) {
        onScalar(10.0); // the "<10>" default
        return true;
    }
    m_finished = true;
    return false;
}
