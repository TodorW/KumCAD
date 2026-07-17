#include "Board3DWindow.h"
#include "Viewport3D.h"

#include "core/geometry/BoundingBox.h"
#include "core/pcb/Board3D.h"

#include <QStatusBar>

Board3DWindow::Board3DWindow(const lcad::Document& doc, const lcad::CopperStackup& stackup, QWidget* parent)
    : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("KumCAD — 3D Board View"));
    resize(1000, 700);

    m_viewport = new Viewport3D(this);
    setCentralWidget(m_viewport);

    // No dedicated board-outline entity exists yet -- infer the
    // substrate as every Track/Via/footprint's own bounding box, padded
    // by a margin, rather than a real user-drawn board edge (see
    // Board3DWindow.h's own disclosed simplification).
    lcad::BoundingBox bbox;
    for (const lcad::Entity* e : doc.entities()) {
        if (e->type() == lcad::EntityType::Track || e->type() == lcad::EntityType::Via ||
            e->type() == lcad::EntityType::Insert) {
            const lcad::BoundingBox eb = e->boundingBox();
            if (eb.isValid()) {
                bbox.expand(eb.min);
                bbox.expand(eb.max);
            }
        }
    }

    std::vector<std::pair<double, double>> outline;
    if (bbox.isValid()) {
        constexpr double kMargin = 2.0;
        outline = {{bbox.min.x - kMargin, bbox.min.y - kMargin},
                  {bbox.max.x + kMargin, bbox.min.y - kMargin},
                  {bbox.max.x + kMargin, bbox.max.y + kMargin},
                  {bbox.min.x - kMargin, bbox.max.y + kMargin}};
    }

    const lcad::Board3DShapes shapes = lcad::buildBoard3D(doc, outline, stackup);

    if (m_viewport->isAvailable()) {
        if (!shapes.substrate.IsNull()) m_viewport->displayShape(shapes.substrate, 0.1, 0.4, 0.1); // FR4 green
        for (const auto& shape : shapes.copper) {
            if (!shape.IsNull()) m_viewport->displayShape(shape, 0.85, 0.55, 0.15); // copper/orange
        }
        for (const auto& shape : shapes.components) {
            if (!shape.IsNull()) m_viewport->displayShape(shape, 0.3, 0.3, 0.3); // generic component gray
        }
        m_viewport->fitAll();
        statusBar()->showMessage(QStringLiteral("%1 copper feature(s), %2 component(s) — left-drag to orbit, "
                                                "wheel to zoom, middle-drag to pan")
                                      .arg(shapes.copper.size())
                                      .arg(shapes.components.size()));
    } else {
        statusBar()->showMessage(QStringLiteral("3D viewport unavailable on this system (no usable display)"));
    }
}
