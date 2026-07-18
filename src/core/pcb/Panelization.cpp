#include "core/pcb/Panelization.h"

#include "core/document/Document.h"
#include "core/geometry/Circle.h"
#include "core/geometry/Line.h"

#include <algorithm>
#include <limits>

namespace lcad {

namespace {
LayerId findOrCreateLayer(Document& doc, const std::string& name) {
    for (const Layer& layer : doc.layers()) {
        if (layer.name == name) return layer.id;
    }
    return doc.addLayer(name, Color(150, 150, 150));
}

void addSeparatorLine(Document& doc, LayerId layer, PanelSeparator style, Point2D from, Point2D to,
                      double holeDiameter, double spacing, std::vector<EntityId>& out) {
    if (style == PanelSeparator::None) return;

    if (style == PanelSeparator::VScore) {
        auto line = std::make_unique<LineEntity>(doc.reserveEntityId(), layer, from, to);
        out.push_back(line->id());
        doc.addEntity(std::move(line));
        return;
    }

    // MouseBites: a row of small unfilled circles marking drill holes,
    // spaced evenly from `from` to `to`.
    const double length = from.distanceTo(to);
    if (length < 1e-9 || spacing <= 1e-9) return;
    const int holeCount = std::max(2, static_cast<int>(std::lround(length / spacing)) + 1);
    for (int i = 0; i < holeCount; ++i) {
        const double t = static_cast<double>(i) / static_cast<double>(holeCount - 1);
        const Point2D pos(from.x + (to.x - from.x) * t, from.y + (to.y - from.y) * t);
        auto hole = std::make_unique<CircleEntity>(doc.reserveEntityId(), layer, pos, holeDiameter / 2.0);
        out.push_back(hole->id());
        doc.addEntity(std::move(hole));
    }
}
} // namespace

std::vector<EntityId> panelizeBoard(Document& doc, const std::vector<Point2D>& boundary, const PanelizeParams& params) {
    std::vector<EntityId> placed;
    if (boundary.size() < 3 || params.columns < 1 || params.rows < 1) return placed;

    double minX = std::numeric_limits<double>::max();
    double minY = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    double maxY = std::numeric_limits<double>::lowest();
    for (const Point2D& p : boundary) {
        minX = std::min(minX, p.x);
        minY = std::min(minY, p.y);
        maxX = std::max(maxX, p.x);
        maxY = std::max(maxY, p.y);
    }
    const double width = maxX - minX;
    const double height = maxY - minY;
    if (width < 1e-9 || height < 1e-9) return placed;

    const double pitchX = width + params.gap;
    const double pitchY = height + params.gap;

    // Snapshot the original board's own entities before cloning starts --
    // doc.entities() would otherwise see (and re-clone) the copies as
    // they're added.
    const std::vector<Entity*> originals = doc.entities();

    for (int r = 0; r < params.rows; ++r) {
        for (int c = 0; c < params.columns; ++c) {
            if (r == 0 && c == 0) continue; // the original board IS the (0,0) cell
            const Point2D delta(c * pitchX, r * pitchY);
            for (const Entity* src : originals) {
                std::unique_ptr<Entity> copy = src->clone();
                copy->setId(doc.reserveEntityId());
                copy->translate(delta);
                placed.push_back(copy->id());
                doc.addEntity(std::move(copy));
            }
        }
    }

    if (params.separator != PanelSeparator::None) {
        const LayerId scoreLayer = findOrCreateLayer(doc, "Dwgs.User");
        const double panelWidth = width + (params.columns - 1) * pitchX;
        const double panelHeight = height + (params.rows - 1) * pitchY;

        for (int c = 0; c < params.columns - 1; ++c) {
            const double x = minX + c * pitchX + width + params.gap / 2.0;
            addSeparatorLine(doc, scoreLayer, params.separator, Point2D(x, minY), Point2D(x, minY + panelHeight),
                             params.mouseBiteHoleDiameter, params.mouseBiteSpacing, placed);
        }
        for (int r = 0; r < params.rows - 1; ++r) {
            const double y = minY + r * pitchY + height + params.gap / 2.0;
            addSeparatorLine(doc, scoreLayer, params.separator, Point2D(minX, y), Point2D(minX + panelWidth, y),
                             params.mouseBiteHoleDiameter, params.mouseBiteSpacing, placed);
        }
    }

    return placed;
}

} // namespace lcad
