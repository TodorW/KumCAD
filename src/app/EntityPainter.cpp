#include "EntityPainter.h"

#include "core/geometry/Arc.h"
#include "core/geometry/Circle.h"
#include "core/geometry/Dimension.h"
#include "core/geometry/Ellipse.h"
#include "core/geometry/Hatch.h"
#include "core/geometry/Insert.h"
#include "core/geometry/Line.h"
#include "core/geometry/Polyline.h"
#include "core/geometry/Text.h"

#include <QFont>
#include <QFontMetricsF>
#include <QPainter>
#include <QPolygonF>

#include <algorithm>
#include <cmath>

namespace EntityPainter {

void paint(QPainter& painter, const lcad::Entity& entity, const WorldToScreen& toScreen, double scale,
           const QColor& color, double penWidth) {
    painter.setPen(QPen(color, penWidth));

    switch (entity.type()) {
    case lcad::EntityType::Line: {
        const auto& line = static_cast<const lcad::LineEntity&>(entity);
        painter.drawLine(toScreen(line.start()), toScreen(line.end()));
        break;
    }
    case lcad::EntityType::Circle: {
        const auto& circle = static_cast<const lcad::CircleEntity&>(entity);
        const QPointF c = toScreen(circle.center());
        const double r = circle.radius() * scale;
        painter.drawEllipse(c, r, r);
        break;
    }
    case lcad::EntityType::Arc: {
        const auto& arc = static_cast<const lcad::ArcEntity&>(entity);
        const QPointF c = toScreen(arc.center());
        const double r = arc.radius() * scale;
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
        // by the mapping -- so no extra sign flip is needed here.
        const double startDeg = qRadiansToDegrees(arc.startAngle());
        const double spanDeg = qRadiansToDegrees(sweep);
        painter.drawArc(bounds, static_cast<int>(startDeg * 16), static_cast<int>(spanDeg * 16));
        break;
    }
    case lcad::EntityType::Polyline: {
        const auto& pl = static_cast<const lcad::PolylineEntity&>(entity);
        const auto& verts = pl.vertices();
        for (std::size_t i = 0; i + 1 < verts.size(); ++i) {
            painter.drawLine(toScreen(verts[i]), toScreen(verts[i + 1]));
        }
        if (pl.closed() && verts.size() > 1) painter.drawLine(toScreen(verts.back()), toScreen(verts.front()));
        break;
    }
    case lcad::EntityType::Ellipse: {
        const auto& ellipse = static_cast<const lcad::EllipseEntity&>(entity);
        painter.save();
        painter.translate(toScreen(ellipse.center()));
        // World CCW rotation is clockwise in raw Y-down screen space, hence
        // the sign flip -- same reasoning as the Text case below.
        painter.rotate(-qRadiansToDegrees(ellipse.rotation()));
        painter.drawEllipse(QPointF(0, 0), ellipse.radiusX() * scale, ellipse.radiusY() * scale);
        painter.restore();
        break;
    }
    case lcad::EntityType::Dimension: {
        const auto& dim = static_cast<const lcad::DimensionEntity&>(entity);
        const auto geo = dim.geometry();

        painter.drawLine(toScreen(geo.ext1A), toScreen(geo.ext1B));
        painter.drawLine(toScreen(geo.ext2A), toScreen(geo.ext2B));
        painter.drawLine(toScreen(geo.dimA), toScreen(geo.dimB));

        // Arrowheads: slim filled triangles at the dimension line's ends,
        // pointing outward, sized relative to the label text.
        const lcad::Point2D span = geo.dimB - geo.dimA;
        const double spanLen = span.length();
        if (spanLen > 1e-9) {
            const lcad::Point2D dir = span * (1.0 / spanLen);
            const lcad::Point2D normal(-dir.y, dir.x);
            const double arrow = 0.5 * dim.textHeight();
            auto drawArrow = [&](const lcad::Point2D& tip, const lcad::Point2D& inward) {
                QPolygonF tri;
                tri << toScreen(tip) << toScreen(tip + inward * arrow + normal * (arrow / 3.0))
                    << toScreen(tip + inward * arrow - normal * (arrow / 3.0));
                painter.setBrush(color);
                painter.drawPolygon(tri);
                painter.setBrush(Qt::NoBrush);
            };
            drawArrow(geo.dimA, dir);
            drawArrow(geo.dimB, dir * -1.0);
        }

        const QString label = QString::number(geo.value, 'f', 2);
        QFont font = painter.font();
        font.setPixelSize(std::max(1, static_cast<int>(std::round(dim.textHeight() * scale))));
        painter.save();
        painter.setFont(font);
        painter.translate(toScreen(geo.textPos));
        painter.rotate(-qRadiansToDegrees(geo.textAngle)); // same sign flip as the Text case
        const QFontMetricsF metrics(font);
        painter.drawText(QPointF(-metrics.horizontalAdvance(label) / 2.0, metrics.height() / 4.0), label);
        painter.restore();
        break;
    }
    case lcad::EntityType::Text: {
        const auto& text = static_cast<const lcad::TextEntity&>(entity);
        QFont font = painter.font();
        font.setPixelSize(std::max(1, static_cast<int>(std::round(text.height() * scale))));
        painter.save();
        painter.setFont(font);
        painter.translate(toScreen(text.position()));
        // painter.rotate() is clockwise in raw (Y-down) screen space, which is
        // visually clockwise too since we draw directly in that space with no
        // further flip -- our world angle convention is CCW-positive (visually),
        // so it needs the opposite sign here, same reasoning as the ARC case above.
        painter.rotate(-qRadiansToDegrees(text.rotation()));
        painter.drawText(QPointF(0, 0), QString::fromStdString(text.text()));
        painter.restore();
        break;
    }
    case lcad::EntityType::Hatch: {
        const auto& hatch = static_cast<const lcad::HatchEntity&>(entity);
        QPolygonF poly;
        for (const lcad::Point2D& v : hatch.vertices()) poly << toScreen(v);
        painter.setBrush(color);
        painter.drawPolygon(poly);
        painter.setBrush(Qt::NoBrush);
        break;
    }
    case lcad::EntityType::Insert: {
        // Blocks render as their transformed children, all in the insert's
        // resolved color (v1 simplification; per-child colors would need
        // ByBlock semantics).
        const auto& insert = static_cast<const lcad::InsertEntity&>(entity);
        for (const auto& child : insert.instantiate()) {
            paint(painter, *child, toScreen, scale, color, penWidth);
        }
        break;
    }
    }
}

} // namespace EntityPainter
