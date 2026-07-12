#include "PlotStyleDialog.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

PlotStyleDialog::PlotStyleDialog(lcad::Document& document, QWidget* parent) : QDialog(parent), m_document(document) {
    setWindowTitle(QStringLiteral("Plot Style Table"));
    resize(320, 360);

    m_list = new QListWidget(this);

    auto* newButton = new QPushButton(QStringLiteral("New..."), this);
    auto* editButton = new QPushButton(QStringLiteral("Edit..."), this);
    auto* deleteButton = new QPushButton(QStringLiteral("Delete"), this);
    auto* closeButton = new QPushButton(QStringLiteral("Close"), this);
    connect(newButton, &QPushButton::clicked, this, &PlotStyleDialog::onNew);
    connect(editButton, &QPushButton::clicked, this, &PlotStyleDialog::onEdit);
    connect(deleteButton, &QPushButton::clicked, this, &PlotStyleDialog::onDelete);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_list, &QListWidget::itemDoubleClicked, this, &PlotStyleDialog::onEdit);

    auto* buttons = new QHBoxLayout();
    buttons->addWidget(newButton);
    buttons->addWidget(editButton);
    buttons->addWidget(deleteButton);
    buttons->addStretch();
    buttons->addWidget(closeButton);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(m_list);
    layout->addLayout(buttons);

    refresh();
}

void PlotStyleDialog::refresh() {
    m_list->clear();
    for (const lcad::PlotStyle& style : m_document.plotStyles()) {
        m_list->addItem(QString::fromStdString(style.name));
    }
}

void PlotStyleDialog::onNew() {
    if (editStyle(QString())) refresh();
}

void PlotStyleDialog::onEdit() {
    QListWidgetItem* item = m_list->currentItem();
    if (!item) return;
    if (editStyle(item->text())) refresh();
}

void PlotStyleDialog::onDelete() {
    QListWidgetItem* item = m_list->currentItem();
    if (!item) return;
    if (QMessageBox::question(this, QStringLiteral("Delete Plot Style"),
                               QStringLiteral("Delete plot style \"%1\"?").arg(item->text()))
        != QMessageBox::Yes) {
        return;
    }
    m_document.deletePlotStyle(item->text().toStdString());
    refresh();
}

bool PlotStyleDialog::editStyle(const QString& existingName) {
    const bool isNew = existingName.isEmpty();
    lcad::PlotStyle style;
    style.name = existingName.toStdString();
    if (!isNew) {
        if (const lcad::PlotStyle* found = m_document.findPlotStyle(style.name)) style = *found;
    }

    QDialog dlg(this);
    dlg.setWindowTitle(isNew ? QStringLiteral("New Plot Style") : QStringLiteral("Edit Plot Style"));

    auto* nameEdit = new QLineEdit(QString::fromStdString(style.name), &dlg);
    nameEdit->setEnabled(isNew);

    lcad::Color pickedColor = style.color.value_or(lcad::Color{255, 255, 255});
    auto* colorCheck = new QCheckBox(QStringLiteral("Override color"), &dlg);
    colorCheck->setChecked(style.color.has_value());
    auto* colorButton = new QPushButton(QStringLiteral("Pick..."), &dlg);
    connect(colorButton, &QPushButton::clicked, &dlg, [&dlg, &pickedColor, colorCheck, colorButton]() {
        const QColor picked = QColorDialog::getColor(QColor(pickedColor.r, pickedColor.g, pickedColor.b), &dlg,
                                                      QStringLiteral("Plot Color"));
        if (!picked.isValid()) return;
        pickedColor = lcad::Color{static_cast<unsigned char>(picked.red()), static_cast<unsigned char>(picked.green()),
                                  static_cast<unsigned char>(picked.blue())};
        colorCheck->setChecked(true);
        colorButton->setStyleSheet(QStringLiteral("background-color: %1").arg(picked.name()));
    });
    colorButton->setStyleSheet(
        QStringLiteral("background-color: rgb(%1,%2,%3)").arg(pickedColor.r).arg(pickedColor.g).arg(pickedColor.b));

    auto* weightCheck = new QCheckBox(QStringLiteral("Override lineweight (mm)"), &dlg);
    weightCheck->setChecked(style.lineweight.has_value());
    auto* weightSpin = new QDoubleSpinBox(&dlg);
    weightSpin->setRange(0.0, 5.0);
    weightSpin->setSingleStep(0.05);
    weightSpin->setValue(style.lineweight.value_or(0.25));

    auto* typeCheck = new QCheckBox(QStringLiteral("Override linetype"), &dlg);
    typeCheck->setChecked(style.linetype.has_value());
    auto* typeCombo = new QComboBox(&dlg);
    for (lcad::LineType type : lcad::allLineTypes()) {
        typeCombo->addItem(QLatin1String(lcad::lineTypeName(type)), static_cast<int>(type));
    }
    if (style.linetype) {
        const int idx = typeCombo->findData(static_cast<int>(*style.linetype));
        if (idx >= 0) typeCombo->setCurrentIndex(idx);
    }

    auto* form = new QFormLayout();
    form->addRow(QStringLiteral("Name:"), nameEdit);
    auto* colorRow = new QHBoxLayout();
    colorRow->addWidget(colorCheck);
    colorRow->addWidget(colorButton);
    form->addRow(colorRow);
    auto* weightRow = new QHBoxLayout();
    weightRow->addWidget(weightCheck);
    weightRow->addWidget(weightSpin);
    form->addRow(weightRow);
    auto* typeRow = new QHBoxLayout();
    typeRow->addWidget(typeCheck);
    typeRow->addWidget(typeCombo);
    form->addRow(typeRow);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    connect(buttonBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    auto* dlgLayout = new QVBoxLayout(&dlg);
    dlgLayout->addLayout(form);
    dlgLayout->addWidget(buttonBox);

    if (dlg.exec() != QDialog::Accepted) return false;

    const QString name = nameEdit->text().trimmed();
    if (name.isEmpty()) return false;

    lcad::PlotStyle result;
    result.name = name.toStdString();
    if (colorCheck->isChecked()) result.color = pickedColor;
    if (weightCheck->isChecked()) result.lineweight = weightSpin->value();
    if (typeCheck->isChecked()) result.linetype = static_cast<lcad::LineType>(typeCombo->currentData().toInt());

    m_document.savePlotStyle(std::move(result));
    return true;
}
