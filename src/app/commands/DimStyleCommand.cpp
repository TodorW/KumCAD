#include "commands/DimStyleCommand.h"

std::optional<QString> DimStyleCommand::onText(const QString& text) {
    lcad::DimStyle& style = m_document.dimStyle();
    const QString trimmed = text.trimmed();

    switch (m_stage) {
    case Stage::TextHeight: {
        if (!trimmed.isEmpty()) {
            bool ok = false;
            const double v = trimmed.toDouble(&ok);
            if (!ok || v < 1e-6) return QStringLiteral("*Invalid* Text height <%1>:").arg(style.textHeight);
            style.textHeight = v;
        }
        m_stage = Stage::ArrowSize;
        return QStringLiteral("Arrow size <%1>:").arg(style.arrowSize);
    }
    case Stage::ArrowSize: {
        if (!trimmed.isEmpty()) {
            bool ok = false;
            const double v = trimmed.toDouble(&ok);
            if (!ok || v < 1e-6) return QStringLiteral("*Invalid* Arrow size <%1>:").arg(style.arrowSize);
            style.arrowSize = v;
        }
        m_stage = Stage::Decimals;
        return QStringLiteral("Decimal places <%1>:").arg(style.decimals);
    }
    case Stage::Decimals: {
        if (!trimmed.isEmpty()) {
            bool ok = false;
            const int v = trimmed.toInt(&ok);
            if (!ok || v < 0 || v > 8) return QStringLiteral("*Invalid* Decimal places <%1>:").arg(style.decimals);
            style.decimals = v;
        }
        m_finished = true;
        return QStringLiteral("*DIMSTYLE: text %1, arrows %2, %3 decimals*")
            .arg(style.textHeight)
            .arg(style.arrowSize)
            .arg(style.decimals);
    }
    }
    return std::nullopt;
}
