#include "commands/LibraryCommands.h"

#include "core/document/PartLibrary.h"

#include <QStringList>

std::optional<QString> LibSaveCommand::onText(const QString& text) {
    if (m_stage == 0) {
        m_names.clear();
        for (const QString& name : text.split(QLatin1Char(','), Qt::SkipEmptyParts)) {
            m_names.push_back(name.trimmed().toStdString());
        }
        if (m_names.empty()) return QStringLiteral("*Enter at least one block name*");
        m_stage = 1;
        return QStringLiteral("Library file path:");
    }

    m_finished = true;
    const QString path = text.trimmed();
    if (path.isEmpty()) {
        m_result = QStringLiteral("*Cancelled*");
        return m_result;
    }
    std::string error;
    if (!lcad::writePartLibrary(m_document, m_names, path.toStdString(), &error)) {
        m_result = QStringLiteral("*Save failed: %1*").arg(QString::fromStdString(error));
        return m_result;
    }
    m_result = QStringLiteral("*Saved %1 block(s) to %2*").arg(m_names.size()).arg(path);
    return m_result;
}

std::optional<QString> LibLoadCommand::onText(const QString& text) {
    if (m_stage == 0) {
        const QString path = text.trimmed();
        if (path.isEmpty()) {
            m_finished = true;
            m_result = QStringLiteral("*Cancelled*");
            return m_result;
        }
        m_path = path;
        m_stage = 1;
        return QStringLiteral("Overwrite existing blocks with the same name? [Yes/No] <No>:");
    }

    m_finished = true;
    const QString option = text.trimmed().toUpper();
    const bool overwrite = option == QLatin1String("Y") || option == QLatin1String("YES");

    std::vector<std::string> added;
    std::string error;
    if (!lcad::readPartLibrary(m_document, m_path.toStdString(), overwrite, &added, &error)) {
        m_result = QStringLiteral("*Load failed: %1*").arg(QString::fromStdString(error));
        return m_result;
    }
    if (added.empty()) {
        m_result = QStringLiteral("*Nothing added (every block already existed -- retry with Yes to overwrite)*");
        return m_result;
    }
    QString names;
    for (const std::string& name : added) {
        if (!names.isEmpty()) names += QStringLiteral(", ");
        names += QString::fromStdString(name);
    }
    m_result = QStringLiteral("*Loaded %1 block(s): %2*").arg(added.size()).arg(names);
    return m_result;
}
