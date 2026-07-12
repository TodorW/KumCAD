#pragma once

#include <QWidget>
#include <QVector>

class QListWidget;
class QListWidgetItem;

// A simplified Sheet Set Manager: a list of sheets (a drawing file + one of
// its layout names + a display title), each opened by double-clicking.
// Persisted to a small pipe-delimited text file ("title|filePath|layoutName"
// per line) rather than AutoCAD's real .dst XML format. Since KumCAD (unlike
// AutoCAD) only has one drawing open at a time, opening a sheet replaces the
// current document -- the panel itself never loads a file into memory
// except briefly, to list its layout names when adding a sheet, or (for
// Publish) to render each sheet's page in turn.
//
// Publish batch-plots every sheet into one multi-page PDF, the way AutoCAD's
// own Publish does for a sheet set -- each sheet's source file is loaded
// into a scratch Document just long enough to render its page via the same
// renderDocumentPage() the live Print/Export PDF commands use, then
// discarded. Sheets whose file no longer exists are skipped and reported at
// the end rather than aborting the whole batch.
class SheetSetPanel : public QWidget {
    Q_OBJECT
public:
    explicit SheetSetPanel(QWidget* parent = nullptr);

signals:
    // Emitted when the user opens a sheet (double-click); MainWindow loads
    // path and switches to layoutName (empty = Model space).
    void sheetOpenRequested(const QString& path, const QString& layoutName);

private slots:
    void onAddSheet();
    void onRemoveSheet();
    void onNewSet();
    void onSaveSet();
    void onLoadSet();
    void onPublish();
    void onItemDoubleClicked(QListWidgetItem* item);

private:
    struct Sheet {
        QString title;
        QString filePath;
        QString layoutName; // empty = Model space
    };

    void refreshList();

    QListWidget* m_list;
    QVector<Sheet> m_sheets;
    QString m_setPath; // where onSaveSet/onLoadSet last wrote/read, for a default
};
