#pragma once

#include "core/geometry/Entity.h"

#include <QColor>
#include <QPointF>

#include <functional>

class QPainter;

// Entity drawing shared by the interactive canvas (DrawingView) and the
// print/PDF path. The world-to-screen mapping is injected so each caller
// brings its own viewport (pan/zoom vs. fit-to-page).
namespace EntityPainter {

using WorldToScreen = std::function<QPointF(const lcad::Point2D&)>;

// scale = device pixels per world unit (for radii and text sizes).
void paint(QPainter& painter, const lcad::Entity& entity, const WorldToScreen& toScreen, double scale,
           const QColor& color, double penWidth);

} // namespace EntityPainter
