#include "commands/QuickCalcCommand.h"

#include "core/util/Expr.h"

std::optional<QString> QuickCalcCommand::onText(const QString& text) {
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        m_finished = true;
        return QStringLiteral("*Cancelled*");
    }

    std::string error;
    const auto result = lcad::evaluateExpression(trimmed.toStdString(), &error);
    if (!result) {
        return QStringLiteral("*Error: %1*\nEnter expression:").arg(QString::fromStdString(error));
    }
    m_finished = true;
    return QStringLiteral("*= %1*").arg(*result, 0, 'g', 10);
}
