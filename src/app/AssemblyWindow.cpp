#include "AssemblyWindow.h"
#include "Viewport3D.h"

#include "core/core3d/StepIges.h"

#include <BRepBuilderAPI_Transform.hxx>

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QMessageBox>
#include <QStatusBar>
#include <QStringList>
#include <QToolBar>

using lcad::AssemblyComponent;
using lcad::Mate;
using lcad::MateType;

namespace {

QString mateTypeName(MateType type) {
    switch (type) {
    case MateType::Coincident: return QStringLiteral("Coincident");
    case MateType::Concentric: return QStringLiteral("Concentric");
    case MateType::Distance: return QStringLiteral("Distance");
    case MateType::Angle: return QStringLiteral("Angle");
    }
    return QStringLiteral("Mate");
}

// A mate's reference point/direction on each component is typed in
// directly rather than picked in the viewport -- there's no face/edge
// picking there yet (see Assembly.h's own disclosure, the same scope cut
// Fillet/Chamfer made for edges).
class AddMateDialog : public QDialog {
public:
    AddMateDialog(const std::vector<AssemblyComponent>& components, QWidget* parent = nullptr) : QDialog(parent) {
        setWindowTitle(QStringLiteral("Add Mate"));
        auto* form = new QFormLayout(this);

        m_typeCombo = new QComboBox(this);
        for (MateType type : {MateType::Coincident, MateType::Concentric, MateType::Distance, MateType::Angle}) {
            m_typeCombo->addItem(mateTypeName(type), static_cast<int>(type));
        }
        form->addRow(QStringLiteral("Type:"), m_typeCombo);

        m_compA = new QComboBox(this);
        m_compB = new QComboBox(this);
        for (std::size_t i = 0; i < components.size(); ++i) {
            const QString label = QStringLiteral("[%1] %2").arg(i).arg(QString::fromStdString(components[i].name));
            m_compA->addItem(label, static_cast<int>(i));
            m_compB->addItem(label, static_cast<int>(i));
        }
        if (components.size() > 1) m_compB->setCurrentIndex(1);
        form->addRow(QStringLiteral("Component A (already placed):"), m_compA);
        form->addRow(QStringLiteral("Component B (gets placed):"), m_compB);

        m_ax = makeSpin(0.0); m_ay = makeSpin(0.0); m_az = makeSpin(0.0);
        form->addRow(QStringLiteral("A point X/Y/Z:"), rowOf({m_ax, m_ay, m_az}));
        m_adx = makeSpin(0.0); m_ady = makeSpin(0.0); m_adz = makeSpin(1.0);
        form->addRow(QStringLiteral("A direction X/Y/Z:"), rowOf({m_adx, m_ady, m_adz}));
        m_bx = makeSpin(0.0); m_by = makeSpin(0.0); m_bz = makeSpin(0.0);
        form->addRow(QStringLiteral("B point X/Y/Z:"), rowOf({m_bx, m_by, m_bz}));
        m_bdx = makeSpin(0.0); m_bdy = makeSpin(0.0); m_bdz = makeSpin(1.0);
        form->addRow(QStringLiteral("B direction X/Y/Z:"), rowOf({m_bdx, m_bdy, m_bdz}));
        m_value = makeSpin(0.0);
        form->addRow(QStringLiteral("Distance offset / Angle degrees:"), m_value);

        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        form->addRow(buttons);
    }

    Mate result() const {
        Mate m;
        m.type = static_cast<MateType>(m_typeCombo->currentData().toInt());
        m.componentA = m_compA->currentData().toInt();
        m.componentB = m_compB->currentData().toInt();
        m.ax = m_ax->value(); m.ay = m_ay->value(); m.az = m_az->value();
        m.adx = m_adx->value(); m.ady = m_ady->value(); m.adz = m_adz->value();
        m.bx = m_bx->value(); m.by = m_by->value(); m.bz = m_bz->value();
        m.bdx = m_bdx->value(); m.bdy = m_bdy->value(); m.bdz = m_bdz->value();
        m.value = m_value->value();
        return m;
    }

private:
    QDoubleSpinBox* makeSpin(double value) {
        auto* spin = new QDoubleSpinBox(this);
        spin->setRange(-1e6, 1e6);
        spin->setDecimals(3);
        spin->setValue(value);
        return spin;
    }

    QWidget* rowOf(std::initializer_list<QWidget*> widgets) {
        auto* container = new QWidget(this);
        auto* layout = new QHBoxLayout(container);
        layout->setContentsMargins(0, 0, 0, 0);
        for (QWidget* w : widgets) layout->addWidget(w);
        return container;
    }

    QComboBox* m_typeCombo = nullptr;
    QComboBox* m_compA = nullptr;
    QComboBox* m_compB = nullptr;
    QDoubleSpinBox *m_ax, *m_ay, *m_az, *m_adx, *m_ady, *m_adz;
    QDoubleSpinBox *m_bx, *m_by, *m_bz, *m_bdx, *m_bdy, *m_bdz;
    QDoubleSpinBox* m_value = nullptr;
};

} // namespace

AssemblyWindow::AssemblyWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("KumCAD — Assembly (early preview)"));
    resize(1200, 800);

    m_viewport = new Viewport3D(this);
    setCentralWidget(m_viewport);

    m_componentList = new QListWidget(this);
    auto* componentDock = new QDockWidget(QStringLiteral("Components"), this);
    componentDock->setWidget(m_componentList);
    addDockWidget(Qt::RightDockWidgetArea, componentDock);

    m_mateList = new QListWidget(this);
    auto* mateDock = new QDockWidget(QStringLiteral("Mates"), this);
    mateDock->setWidget(m_mateList);
    addDockWidget(Qt::RightDockWidgetArea, mateDock);

    QToolBar* toolbar = addToolBar(QStringLiteral("Assembly"));
    toolbar->addAction(QStringLiteral("Add Component from STEP..."), this, &AssemblyWindow::addComponentFromStep);
    toolbar->addAction(QStringLiteral("Add Mate..."), this, &AssemblyWindow::addMate);
    toolbar->addAction(QStringLiteral("Solve"), this, &AssemblyWindow::solve);
    toolbar->addAction(QStringLiteral("Check DOF..."), this, &AssemblyWindow::checkDof);

    if (m_viewport->isAvailable()) {
        statusBar()->showMessage(QStringLiteral("Add components from STEP files, define mates between them, then "
                                                "Solve to place everything"));
    } else {
        statusBar()->showMessage(QStringLiteral("3D viewport unavailable on this system (no usable display) -- the "
                                                "assembly solver still works, just not the preview"));
    }
}

void AssemblyWindow::addComponentFromStep() {
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Add Component from STEP"), QString(),
                                                        QStringLiteral("STEP Files (*.step *.stp)"));
    if (path.isEmpty()) return;
    const TopoDS_Shape shape = lcad::readStep(path.toStdString());
    if (shape.IsNull()) {
        QMessageBox::warning(this, QStringLiteral("Import Failed"), QStringLiteral("Could not read that STEP file."));
        return;
    }

    AssemblyComponent component;
    component.name = QFileInfo(path).baseName().toStdString();
    component.shape = shape;
    // The first component added has nothing to mate against yet, so it's
    // fixed by default -- every assembly needs at least one anchor for the
    // rest of the mate chain to solve against (see Assembly.h).
    component.fixed = m_assembly.components().empty();
    m_assembly.addComponent(component);
    refreshComponentList();
    refreshViewport();
}

void AssemblyWindow::addMate() {
    if (m_assembly.components().size() < 2) {
        statusBar()->showMessage(QStringLiteral("Add at least two components first"), 3000);
        return;
    }
    AddMateDialog dialog(m_assembly.components(), this);
    if (dialog.exec() != QDialog::Accepted) return;
    m_assembly.addMate(dialog.result());
    refreshMateList();
}

void AssemblyWindow::solve() {
    m_assembly.solve();
    refreshViewport();
    statusBar()->showMessage(QStringLiteral("Solved"), 2000);
}

void AssemblyWindow::checkDof() {
    const lcad::AssemblyDofReport report = lcad::analyzeAssemblyDof(m_assembly);
    QString message;
    if (report.unplacedComponentIndices.empty() && report.multiplyMatedComponentIndices.empty()) {
        message = QStringLiteral("Every component is either fixed or placed by exactly one mate.");
    } else {
        if (!report.unplacedComponentIndices.empty()) {
            QStringList names;
            for (int idx : report.unplacedComponentIndices) {
                names << QStringLiteral("[%1] %2").arg(idx).arg(
                    QString::fromStdString(m_assembly.components()[static_cast<std::size_t>(idx)].name));
            }
            message += QStringLiteral("Unplaced (neither fixed nor mated): %1\n").arg(names.join(QStringLiteral(", ")));
        }
        if (!report.multiplyMatedComponentIndices.empty()) {
            QStringList names;
            for (int idx : report.multiplyMatedComponentIndices) {
                names << QStringLiteral("[%1] %2").arg(idx).arg(
                    QString::fromStdString(m_assembly.components()[static_cast<std::size_t>(idx)].name));
            }
            message += QStringLiteral("Mated more than once (later mate silently wins): %1").arg(names.join(QStringLiteral(", ")));
        }
    }
    QMessageBox::information(this, QStringLiteral("Assembly DOF Check"), message);
}

void AssemblyWindow::refreshComponentList() {
    m_componentList->clear();
    for (std::size_t i = 0; i < m_assembly.components().size(); ++i) {
        const auto& component = m_assembly.components()[i];
        QString text = QStringLiteral("[%1] %2").arg(i).arg(QString::fromStdString(component.name));
        if (component.fixed) text += QStringLiteral(" (fixed)");
        m_componentList->addItem(text);
    }
}

void AssemblyWindow::refreshMateList() {
    m_mateList->clear();
    for (const Mate& mate : m_assembly.mates()) {
        m_mateList->addItem(QStringLiteral("%1: [%2] -> [%3]")
                                 .arg(mateTypeName(mate.type))
                                 .arg(mate.componentA)
                                 .arg(mate.componentB));
    }
}

void AssemblyWindow::refreshViewport() {
    if (!m_viewport->isAvailable()) return;
    m_viewport->clearShapes();
    for (const auto& component : m_assembly.components()) {
        if (component.shape.IsNull()) continue;
        m_viewport->displayShape(BRepBuilderAPI_Transform(component.shape, component.placement, true).Shape());
    }
    m_viewport->fitAll();
}
