#include "SheetSetPanel.h"

#include "PrintRenderer.h"

#include "core/document/Document.h"
#include "core/io/DwgReader.h"
#include "core/io/DxfReader.h"

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPainter>
#include <QPrinter>
#include <QPushButton>
#include <QTextStream>
#include <QVBoxLayout>

namespace {

// Loads path into a scratch document just far enough to list its layout
// names, for the "which layout is this sheet" picker.
QStringList peekLayoutNames(const QString& path) {
    lcad::Document scratch;
    const bool isDwg = path.endsWith(QStringLiteral(".dwg"), Qt::CaseInsensitive);
    const bool ok = isDwg ? lcad::readDwg(scratch, path.toStdString()) : lcad::readDxf(scratch, path.toStdString());
    QStringList names;
    if (!ok) return names;
    for (const lcad::Layout& layout : scratch.layouts()) names << QString::fromStdString(layout.name);
    return names;
}

} // namespace

SheetSetPanel::SheetSetPanel(QWidget* parent) : QWidget(parent) {
    m_list = new QListWidget(this);
    connect(m_list, &QListWidget::itemDoubleClicked, this, &SheetSetPanel::onItemDoubleClicked);

    auto* hint = new QLabel(QStringLiteral("Double-click a sheet to open it (replaces the current drawing)."), this);
    hint->setWordWrap(true);
    hint->setStyleSheet(QStringLiteral("color: #888;"));

    auto* addButton = new QPushButton(QStringLiteral("Add Sheet..."), this);
    connect(addButton, &QPushButton::clicked, this, &SheetSetPanel::onAddSheet);
    auto* removeButton = new QPushButton(QStringLiteral("Remove Sheet"), this);
    connect(removeButton, &QPushButton::clicked, this, &SheetSetPanel::onRemoveSheet);
    auto* newButton = new QPushButton(QStringLiteral("New Set"), this);
    connect(newButton, &QPushButton::clicked, this, &SheetSetPanel::onNewSet);
    auto* saveButton = new QPushButton(QStringLiteral("Save Set..."), this);
    connect(saveButton, &QPushButton::clicked, this, &SheetSetPanel::onSaveSet);
    auto* loadButton = new QPushButton(QStringLiteral("Load Set..."), this);
    connect(loadButton, &QPushButton::clicked, this, &SheetSetPanel::onLoadSet);
    auto* publishButton = new QPushButton(QStringLiteral("Publish..."), this);
    connect(publishButton, &QPushButton::clicked, this, &SheetSetPanel::onPublish);

    auto* buttonRow1 = new QHBoxLayout();
    buttonRow1->addWidget(addButton);
    buttonRow1->addWidget(removeButton);
    auto* buttonRow2 = new QHBoxLayout();
    buttonRow2->addWidget(newButton);
    buttonRow2->addWidget(saveButton);
    buttonRow2->addWidget(loadButton);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(hint);
    layout->addWidget(m_list);
    layout->addLayout(buttonRow1);
    layout->addLayout(buttonRow2);
    layout->addWidget(publishButton);
}

void SheetSetPanel::refreshList() {
    m_list->clear();
    for (int i = 0; i < m_sheets.size(); ++i) {
        const Sheet& sheet = m_sheets[i];
        const QString layoutPart = sheet.layoutName.isEmpty() ? QStringLiteral("Model") : sheet.layoutName;
        auto* item = new QListWidgetItem(
            QStringLiteral("%1  [%2 — %3]").arg(sheet.title, QFileInfo(sheet.filePath).fileName(), layoutPart),
            m_list);
        item->setData(Qt::UserRole, i);
    }
}

void SheetSetPanel::onAddSheet() {
    const QString filter = QStringLiteral("Drawings (*.dxf *.dwg);;DXF Files (*.dxf);;DWG Files (*.dwg)");
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Add Sheet"), QString(), filter);
    if (path.isEmpty()) return;

    const QStringList layouts = peekLayoutNames(path);
    QStringList options{QStringLiteral("(Model space)")};
    options << layouts;
    bool ok = false;
    const QString chosen =
        QInputDialog::getItem(this, QStringLiteral("Add Sheet"), QStringLiteral("Layout:"), options, 0, false, &ok);
    if (!ok) return;
    const QString layoutName = chosen == QStringLiteral("(Model space)") ? QString() : chosen;

    const QString defaultTitle =
        layoutName.isEmpty() ? QFileInfo(path).baseName() : QFileInfo(path).baseName() + QStringLiteral(" - ") + layoutName;
    const QString title =
        QInputDialog::getText(this, QStringLiteral("Add Sheet"), QStringLiteral("Sheet title:"), QLineEdit::Normal,
                              defaultTitle, &ok);
    if (!ok || title.isEmpty()) return;

    m_sheets.push_back({title, path, layoutName});
    refreshList();
}

void SheetSetPanel::onRemoveSheet() {
    const int row = m_list->currentRow();
    if (row < 0 || row >= m_sheets.size()) return;
    m_sheets.removeAt(row);
    refreshList();
}

void SheetSetPanel::onNewSet() {
    m_sheets.clear();
    m_setPath.clear();
    refreshList();
}

void SheetSetPanel::onSaveSet() {
    const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("Save Sheet Set"), m_setPath,
                                                       QStringLiteral("KumCAD Sheet Set (*.kcss)"));
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("Save Sheet Set"), QStringLiteral("Could not write the file."));
        return;
    }
    QTextStream out(&file);
    for (const Sheet& sheet : m_sheets) {
        out << sheet.title << '|' << sheet.filePath << '|' << sheet.layoutName << '\n';
    }
    m_setPath = path;
}

void SheetSetPanel::onLoadSet() {
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Load Sheet Set"), m_setPath,
                                                       QStringLiteral("KumCAD Sheet Set (*.kcss)"));
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("Load Sheet Set"), QStringLiteral("Could not read the file."));
        return;
    }
    QVector<Sheet> loaded;
    QTextStream in(&file);
    while (!in.atEnd()) {
        const QStringList parts = in.readLine().split(QLatin1Char('|'));
        if (parts.size() != 3) continue;
        loaded.push_back({parts[0], parts[1], parts[2]});
    }
    m_sheets = loaded;
    m_setPath = path;
    refreshList();
}

void SheetSetPanel::onPublish() {
    if (m_sheets.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Publish"), QStringLiteral("The sheet set is empty."));
        return;
    }
    QString path =
        QFileDialog::getSaveFileName(this, QStringLiteral("Publish Sheet Set"), QString(), QStringLiteral("PDF Files (*.pdf)"));
    if (path.isEmpty()) return;
    if (!path.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive)) path += QStringLiteral(".pdf");

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(path);

    QPainter painter(&printer);
    int published = 0;
    QStringList failed;
    bool firstPage = true;
    for (const Sheet& sheet : m_sheets) {
        lcad::Document scratch;
        const bool isDwg = sheet.filePath.endsWith(QStringLiteral(".dwg"), Qt::CaseInsensitive);
        const bool ok = isDwg ? lcad::readDwg(scratch, sheet.filePath.toStdString())
                              : lcad::readDxf(scratch, sheet.filePath.toStdString());
        if (!ok) {
            failed << sheet.title;
            continue;
        }
        const lcad::Layout* layout = nullptr;
        if (!sheet.layoutName.isEmpty()) {
            for (const lcad::Layout& l : scratch.layouts()) {
                if (QString::fromStdString(l.name) == sheet.layoutName) {
                    layout = &l;
                    break;
                }
            }
        }
        if (!firstPage) printer.newPage();
        firstPage = false;
        renderDocumentPage(painter, printer.resolution(), scratch, layout);
        ++published;
    }
    painter.end();

    if (published == 0) {
        QFile::remove(path); // nothing rendered; don't leave a blank/broken PDF behind
        QMessageBox::warning(this, QStringLiteral("Publish"), QStringLiteral("No sheets could be published."));
        return;
    }

    QString message = QStringLiteral("Published %1 sheet(s) to %2").arg(published).arg(QFileInfo(path).fileName());
    if (!failed.isEmpty()) {
        message += QStringLiteral("\n%1 sheet(s) could not be read: %2")
                       .arg(failed.size())
                       .arg(failed.join(QStringLiteral(", ")));
    }
    QMessageBox::information(this, QStringLiteral("Publish"), message);
}

void SheetSetPanel::onItemDoubleClicked(QListWidgetItem* item) {
    if (!item) return;
    const int index = item->data(Qt::UserRole).toInt();
    if (index < 0 || index >= m_sheets.size()) return;
    emit sheetOpenRequested(m_sheets[index].filePath, m_sheets[index].layoutName);
}
