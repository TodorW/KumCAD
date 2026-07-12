#pragma once

#include "core/document/Document.h"

#include <QDialog>

class QListWidget;

// AutoCAD's Plot Style Table editor, simplified (see core/document/PlotStyle.h):
// create/edit/delete named plot styles, each optionally overriding the
// plotted color/lineweight/linetype. Per-layer assignment lives on the
// Layer panel's context menu ("Plot Style" submenu), same pattern as its
// existing "Linetype" submenu.
class PlotStyleDialog : public QDialog {
    Q_OBJECT
public:
    explicit PlotStyleDialog(lcad::Document& document, QWidget* parent = nullptr);

private slots:
    void onNew();
    void onEdit();
    void onDelete();

private:
    void refresh();
    // Shows the New/Edit editor for style (by name; empty means "create new").
    // Returns true if the user saved changes.
    bool editStyle(const QString& existingName);

    lcad::Document& m_document;
    QListWidget* m_list;
};
