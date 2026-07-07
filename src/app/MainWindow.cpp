#include "MainWindow.h"

#include "CommandDispatcher.h"
#include "CommandLine.h"
#include "DrawingView.h"

#include <QAction>
#include <QDockWidget>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("LinuxCAD"));
    resize(1280, 800);

    m_view = new DrawingView(m_document, this);
    setCentralWidget(m_view);

    m_commandLine = new CommandLine(this);
    m_dispatcher = new CommandDispatcher(m_document, *m_commandLine, this);
    m_view->setDispatcher(m_dispatcher);

    connect(m_dispatcher, &CommandDispatcher::documentChanged, m_view, QOverload<>::of(&QWidget::update));
    connect(m_dispatcher, &CommandDispatcher::previewChanged, m_view, QOverload<>::of(&QWidget::update));
    connect(m_view, &DrawingView::mouseWorldMoved, this, &MainWindow::updateCoordLabel);

    setupDocks();
    setupMenusAndToolbar();

    m_coordLabel = new QLabel(QStringLiteral("0.000, 0.000"), this);
    statusBar()->addPermanentWidget(m_coordLabel);
    statusBar()->showMessage(QStringLiteral("Ready"));

    m_commandLine->appendLine(QStringLiteral("LinuxCAD — type a command (LINE, CIRCLE, ARC, PLINE, UNDO, REDO) and press Enter."));
    m_commandLine->appendLine(QStringLiteral("Command:"));
    m_commandLine->input()->setFocus();
}

void MainWindow::setupDocks() {
    auto* dock = new QDockWidget(QStringLiteral("Command Line"), this);
    dock->setObjectName(QStringLiteral("CommandLineDock"));
    dock->setWidget(m_commandLine);
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    addDockWidget(Qt::BottomDockWidgetArea, dock);
}

void MainWindow::setupMenusAndToolbar() {
    QMenu* fileMenu = menuBar()->addMenu(QStringLiteral("&File"));
    fileMenu->addAction(QStringLiteral("E&xit"), QKeySequence::Quit, this, &QWidget::close);

    QMenu* editMenu = menuBar()->addMenu(QStringLiteral("&Edit"));
    editMenu->addAction(QStringLiteral("&Undo"), QKeySequence::Undo, m_dispatcher, &CommandDispatcher::undo);
    editMenu->addAction(QStringLiteral("&Redo"), QKeySequence::Redo, m_dispatcher, &CommandDispatcher::redo);

    QMenu* viewMenu = menuBar()->addMenu(QStringLiteral("&View"));
    viewMenu->addAction(QStringLiteral("Zoom &Extents"), this, [this]() { m_view->zoomExtents(); });

    QToolBar* drawToolbar = addToolBar(QStringLiteral("Draw"));
    auto addCommandAction = [this, drawToolbar](const QString& label, const QString& command) {
        drawToolbar->addAction(label, this, [this, command]() { m_dispatcher->handleCommandText(command); });
    };
    addCommandAction(QStringLiteral("Line"), QStringLiteral("LINE"));
    addCommandAction(QStringLiteral("Circle"), QStringLiteral("CIRCLE"));
    addCommandAction(QStringLiteral("Arc"), QStringLiteral("ARC"));
    addCommandAction(QStringLiteral("Polyline"), QStringLiteral("PLINE"));
}

void MainWindow::updateCoordLabel(const lcad::Point2D& pt) {
    if (m_coordLabel) m_coordLabel->setText(QStringLiteral("%1, %2").arg(pt.x, 0, 'f', 3).arg(pt.y, 0, 'f', 3));
}
