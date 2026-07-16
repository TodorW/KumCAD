#include "RecentFiles.h"

#include <QFileInfo>
#include <QSettings>

namespace RecentFiles {

namespace {
constexpr auto kKey = "recentFiles";
}

QStringList list() {
    QSettings settings;
    QStringList files = settings.value(QLatin1String(kKey)).toStringList();
    QStringList existing;
    for (const QString& path : files) {
        if (QFileInfo::exists(path)) existing << path;
    }
    if (existing.size() != files.size()) settings.setValue(QLatin1String(kKey), existing);
    return existing;
}

void add(const QString& path) {
    QSettings settings;
    QStringList files = settings.value(QLatin1String(kKey)).toStringList();
    files.removeAll(path);
    files.prepend(path);
    while (files.size() > kMaxEntries) files.removeLast();
    settings.setValue(QLatin1String(kKey), files);
}

} // namespace RecentFiles
