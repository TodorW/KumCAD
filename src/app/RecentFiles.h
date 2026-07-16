#pragma once

#include <QString>
#include <QStringList>

// Persisted (QSettings-backed) most-recently-opened drawing list, shared
// between the welcome screen (displays it) and MainWindow (appends to it
// whenever a file is opened or saved).
namespace RecentFiles {

constexpr int kMaxEntries = 8;

// Newest first. Entries whose file no longer exists on disk are dropped.
QStringList list();

// Moves path to the front, dropping older duplicates and truncating to
// kMaxEntries.
void add(const QString& path);

} // namespace RecentFiles
