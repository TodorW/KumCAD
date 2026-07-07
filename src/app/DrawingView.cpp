#include "DrawingView.h"

#include "CommandDispatcher.h"
#include "core/document/Commands.h"
#include "core/geometry/Arc.h"
#include "core/geometry/Circle.h"
#include "core/geometry/Line.h"
#include "core/geometry/Polyline.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

DrawingView::DrawingView(lcad::Document& document, QWidget* parent) : QOpenGLWidget(parent), m_document(document) {
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
}

lcad::Point2D DrawingView::screenToWorld(const QPointF& screen) const {
    return lcad::Point2D((screen.x() - width() / 2.0) / m_scale + m_viewCenter.x,
                          (height() / 2.0 - screen.y()) / m_scale + m_viewCenter.y);
}

QPointF DrawingView::worldToScreen(const lcad::Point2D& world) const {
    return QPointF((world.x - m_viewCenter.x) * m_scale + width() / 2.0,
                    height() / 2.0 - (world.y - m_viewCenter.y) * m_scale);
}

void DrawingView::zoomExtents() {
    lcad::BoundingBox box;
    for (lcad::Entity* e : m_document.entities()) box.expand(e->boundingBox());
    if (!box.isValid()) {
        m_viewCenter = lcad::Point2D(0.0, 0.0);
        m_scale = 10.0;
        update();
        return;
    }
    const double w = std::max(box.max.x - box.min.x, 1e-6);
    const double h = std::max(box.max.y - box.min.y, 1e-6);
    m_viewCenter = lcad::Point2D((box.min.x + box.max.x) / 2.0, (box.min.y + box.max.y) / 2.0);
    const double margin = 1.2;
    m_scale = std::min(width() / (w * margin), height() / (h * margin));
    update();
}

void DrawingView::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor(33, 33, 33));

    drawGrid(painter);

    for (lcad::Entity* e : m_document.entities()) {
        drawEntity(painter, *e, m_selection.count(e->id()) > 0);
    }

    drawPreview(painter);
    drawSelectionBox(painter);

    if (m_lastMouseWorld) {
        const QPointF c = worldToScreen(*m_lastMouseWorld);
        painter.setPen(QPen(QColor(200, 200, 200), 1));
        painter.drawLine(QPointF(c.x() - 10, c.y()), QPointF(c.x() + 10, c.y()));
        painter.drawLine(QPointF(c.x(), c.y() - 10), QPointF(c.x(), c.y() + 10));
    }
}

void DrawingView::drawGrid(QPainter& painter) {
    double spacing = 1.0;
    while (spacing * m_scale < 20.0) spacing *= 10.0;
    while (spacing * m_scale > 200.0) spacing /= 10.0;

    const lcad::Point2D topLeft = screenToWorld(QPointF(0, 0));
    const lcad::Point2D bottomRight = screenToWorld(QPointF(width(), height()));
    const double minX = std::min(topLeft.x, bottomRight.x);
    const double maxX = std::max(topLeft.x, bottomRight.x);
    const double minY = std::min(topLeft.y, bottomRight.y);
    const double maxY = std::max(topLeft.y, bottomRight.y);

    if ((maxX - minX) / spacing > 2000 || (maxY - minY) / spacing > 2000) return;

    painter.setPen(QPen(QColor(60, 60, 60), 1));
    const double startX = std::floor(minX / spacing) * spacing;
    for (double x = startX; x <= maxX; x += spacing) {
        painter.drawLine(worldToScreen(lcad::Point2D(x, minY)), worldToScreen(lcad::Point2D(x, maxY)));
    }
    const double startY = std::floor(minY / spacing) * spacing;
    for (double y = startY; y <= maxY; y += spacing) {
        painter.drawLine(worldToScreen(lcad::Point2D(minX, y)), worldToScreen(lcad::Point2D(maxX, y)));
    }

    painter.setPen(QPen(QColor(110, 60, 60), 1));
    painter.drawLine(worldToScreen(lcad::Point2D(minX, 0)), worldToScreen(lcad::Point2D(maxX, 0)));
    painter.setPen(QPen(QColor(60, 110, 60), 1));
    painter.drawLine(worldToScreen(lcad::Point2D(0, minY)), worldToScreen(lcad::Point2D(0, maxY)));
}

void DrawingView::drawEntity(QPainter& painter, const lcad::Entity& entity, bool selected) {
    const lcad::Layer* layer = m_document.findLayer(entity.layer());
    const QColor color = selected ? QColor(255, 200, 0)
                                   : (layer ? QColor(layer->color.r, layer->color.g, layer->color.b)
                                            : QColor(255, 255, 255));
    painter.setPen(QPen(color, selected ? 2 : 1));

    switch (entity.type()) {
    case lcad::EntityType::Line: {
        const auto& line = static_cast<const lcad::LineEntity&>(entity);
        painter.drawLine(worldToScreen(line.start()), worldToScreen(line.end()));
        break;
    }
    case lcad::EntityType::Circle: {
        const auto& circle = static_cast<const lcad::CircleEntity&>(entity);
        const QPointF c = worldToScreen(circle.center());
        const double r = circle.radius() * m_scale;
        painter.drawEllipse(c, r, r);
        break;
    }
    case lcad::EntityType::Arc: {
        const auto& arc = static_cast<const lcad::ArcEntity&>(entity);
        const QPointF c = worldToScreen(arc.center());
        const double r = arc.radius() * m_scale;
        const QRectF bounds(c.x() - r, c.y() - r, 2 * r, 2 * r);

        auto normalize = [](double a) {
            a = std::fmod(a, 2 * M_PI);
            if (a < 0) a += 2 * M_PI;
            return a;
        };
        const double ns = normalize(arc.startAngle());
        const double ne = normalize(arc.endAngle());
        double sweep = ne - ns;
        if (sweep <= 0) sweep += 2 * M_PI;

        // QPainter::drawArc's angle convention is defined visually (0 = visually
        // right/3 o'clock, positive = visually CCW toward 12 o'clock), same as our
        // world angle convention once world points are Y-flipped into screen space
        // by worldToScreen() -- so no extra sign flip is needed here.
        const double startDeg = qRadiansToDegrees(arc.startAngle());
        const double spanDeg = qRadiansToDegrees(sweep);
        painter.drawArc(bounds, static_cast<int>(startDeg * 16), static_cast<int>(spanDeg * 16));
        break;
    }
    case lcad::EntityType::Polyline: {
        const auto& pl = static_cast<const lcad::PolylineEntity&>(entity);
        const auto& verts = pl.vertices();
        for (std::size_t i = 0; i + 1 < verts.size(); ++i) {
            painter.drawLine(worldToScreen(verts[i]), worldToScreen(verts[i + 1]));
        }
        if (pl.closed() && verts.size() > 1) painter.drawLine(worldToScreen(verts.back()), worldToScreen(verts.front()));
        break;
    }
    }
}

void DrawingView::drawPreview(QPainter& painter) {
    if (!m_dispatcher) return;
    DrawCommand* cmd = m_dispatcher->activeDrawCommand();
    if (!cmd) return;
    painter.setPen(QPen(QColor(0, 200, 255), 1, Qt::DashLine));
    for (const auto& seg : cmd->previewSegments()) {
        painter.drawLine(worldToScreen(seg.first), worldToScreen(seg.second));
    }
}

void DrawingView::drawSelectionBox(QPainter& painter) {
    if (!m_boxSelecting) return;
    const bool crossing = m_boxCurrentScreen.x() < m_boxStartScreen.x();
    painter.setPen(QPen(crossing ? QColor(0, 200, 0) : QColor(0, 120, 255), 1, Qt::DashLine));
    painter.setBrush(crossing ? QColor(0, 200, 0, 30) : QColor(0, 120, 255, 30));
    painter.drawRect(QRectF(m_boxStartScreen, m_boxCurrentScreen).normalized());
}

void DrawingView::updateSelectionFromBox(const QRectF& screenBox, bool crossing) {
    lcad::BoundingBox worldBox;
    worldBox.expand(screenToWorld(screenBox.topLeft()));
    worldBox.expand(screenToWorld(screenBox.bottomRight()));
    for (lcad::Entity* e : m_document.entities()) {
        const lcad::BoundingBox eb = e->boundingBox();
        const bool hit = crossing ? worldBox.intersects(eb) : worldBox.containsBox(eb);
        if (hit) m_selection.insert(e->id());
    }
}

void DrawingView::deleteSelected() {
    for (lcad::EntityId id : m_selection) {
        m_document.commandStack().execute(std::make_unique<lcad::DeleteEntityCommand>(m_document, id));
    }
    m_selection.clear();
}

void DrawingView::mousePressEvent(QMouseEvent* event) {
    const lcad::Point2D worldPt = screenToWorld(event->position());

    if (event->button() == Qt::MiddleButton) {
        m_panning = true;
        m_lastPanPos = event->position();
        return;
    }
    if (event->button() == Qt::LeftButton) {
        if (m_dispatcher && m_dispatcher->hasActiveCommand()) {
            m_dispatcher->handlePointPicked(worldPt);
            update();
            return;
        }
        m_boxSelecting = true;
        m_boxStartScreen = event->position();
        m_boxCurrentScreen = event->position();
        return;
    }
    if (event->button() == Qt::RightButton) {
        if (m_dispatcher && m_dispatcher->hasActiveCommand()) {
            m_dispatcher->handleFinishRequested();
            update();
        }
        return;
    }
}

void DrawingView::mouseMoveEvent(QMouseEvent* event) {
    const lcad::Point2D worldPt = screenToWorld(event->position());
    m_lastMouseWorld = worldPt;
    emit mouseWorldMoved(worldPt);

    if (m_panning) {
        const QPointF delta = event->position() - m_lastPanPos;
        m_viewCenter.x -= delta.x() / m_scale;
        m_viewCenter.y += delta.y() / m_scale;
        m_lastPanPos = event->position();
        update();
        return;
    }
    if (m_boxSelecting) {
        m_boxCurrentScreen = event->position();
        update();
        return;
    }
    if (m_dispatcher && m_dispatcher->hasActiveCommand()) {
        m_dispatcher->handleMouseMoved(worldPt);
    }
    update();
}

void DrawingView::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        m_panning = false;
        return;
    }
    if (event->button() == Qt::LeftButton && m_boxSelecting) {
        m_boxSelecting = false;
        const QPointF delta = m_boxCurrentScreen - m_boxStartScreen;
        const bool addToSelection = event->modifiers() & Qt::ShiftModifier;

        if (std::abs(delta.x()) < 3 && std::abs(delta.y()) < 3) {
            const lcad::Point2D worldPt = screenToWorld(m_boxStartScreen);
            const double tolerance = 6.0 / m_scale;
            lcad::Entity* best = nullptr;
            double bestDist = tolerance;
            for (lcad::Entity* e : m_document.entities()) {
                const double d = e->distanceTo(worldPt);
                if (d <= bestDist) {
                    bestDist = d;
                    best = e;
                }
            }
            if (!addToSelection) m_selection.clear();
            if (best) {
                auto it = m_selection.find(best->id());
                if (it != m_selection.end()) m_selection.erase(it);
                else m_selection.insert(best->id());
            }
        } else {
            const bool crossing = m_boxCurrentScreen.x() < m_boxStartScreen.x();
            if (!addToSelection) m_selection.clear();
            updateSelectionFromBox(QRectF(m_boxStartScreen, m_boxCurrentScreen).normalized(), crossing);
        }
        update();
        return;
    }
}

void DrawingView::wheelEvent(QWheelEvent* event) {
    const double factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
    const lcad::Point2D before = screenToWorld(event->position());
    m_scale = std::clamp(m_scale * factor, 0.01, 100000.0);
    const lcad::Point2D after = screenToWorld(event->position());
    m_viewCenter.x += (before.x - after.x);
    m_viewCenter.y += (before.y - after.y);
    update();
}

void DrawingView::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete) {
        deleteSelected();
        update();
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        if (m_dispatcher) m_dispatcher->handleEscape();
        m_selection.clear();
        update();
        return;
    }
    QOpenGLWidget::keyPressEvent(event);
}
