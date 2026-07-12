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
// except briefly, to list its layout names when adding a sheet.
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
