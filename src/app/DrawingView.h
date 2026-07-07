#pragma once

#include "core/document/Document.h"
#include "core/geometry/Point2D.h"

#include <QOpenGLWidget>
#include <QPointF>

#include <optional>
#include <unordered_set>

class CommandDispatcher;
class QPainter;

// The drafting canvas: renders the Document with QPainter on a QOpenGLWidget
// (GPU-composited, simple immediate-mode drawing — no hand-rolled vertex
// buffers needed at this scale), handles pan/zoom, a crosshair cursor, click
// and window/crossing selection, and forwards clicks/moves to the active
// DrawCommand (if any) via CommandDispatcher.
class DrawingView : public QOpenGLWidget {
    Q_OBJECT
public:
    explicit DrawingView(lcad::Document& document, QWidget* parent = nullptr);

    void setDispatcher(CommandDispatcher* dispatcher) { m_dispatcher = dispatcher; }

    lcad::Point2D screenToWorld(const QPointF& screen) const;
    QPointF worldToScreen(const lcad::Point2D& world) const;

    void zoomExtents();

signals:
    void mouseWorldMoved(const lcad::Point2D& pt);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void drawGrid(QPainter& painter);
    void drawEntity(QPainter& painter, const lcad::Entity& entity, bool selected);
    void drawPreview(QPainter& painter);
    void drawSelectionBox(QPainter& painter);
    void updateSelectionFromBox(const QRectF& screenBox, bool crossing);
    void deleteSelected();

    lcad::Document& m_document;
    CommandDispatcher* m_dispatcher = nullptr;

    lcad::Point2D m_viewCenter{0.0, 0.0};
    double m_scale = 10.0; // pixels per world unit

    bool m_panning = false;
    QPointF m_lastPanPos;

    bool m_boxSelecting = false;
    QPointF m_boxStartScreen;
    QPointF m_boxCurrentScreen;

    std::unordered_set<lcad::EntityId> m_selection;

    std::optional<lcad::Point2D> m_lastMouseWorld;
};
