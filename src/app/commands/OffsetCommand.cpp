#include "commands/OffsetCommand.h"

#include "core/document/Commands.h"
#include "core/geometry/Arc.h"
#include "core/geometry/Circle.h"
#include "core/geometry/Ellipse.h"
#include "core/geometry/Line.h"
#include "core/geometry/PolylineOps.h"
#include "core/geometry/Spline.h"

#include <cmath>

QString OffsetCommand::start() {
    return QStringLiteral("OFFSET  %1 found\nSpecify offset distance (type it, or pick two points):")
        .arg(static_cast<int>(m_ids.size()));
}

std::optional<QString> OffsetCommand::onScalar(double value) {
    if (m_hasDistance || value < 1e-9) return std::nullopt;
    m_distance = value;
    m_hasDistance = true;
    return QStringLiteral("Specify point on side to offset:");
}

std::optional<QString> OffsetCommand::onPoint(const lcad::Point2D& pt) {
    if (!m_hasDistance) {
        if (!m_hasDistBase) {
            m_distBase = pt;
            m_hasDistBase = true;
            return QStringLiteral("Specify second point (distance):");
        }
        const double d = m_distBase.distanceTo(pt);
        if (d < 1e-9) return QStringLiteral("Specify second point (distance):"); // degenerate, re-prompt
        m_distance = d;
        m_hasDistance = true;
        return QStringLiteral("Specify point on side to offset:");
    }

    const QString result = commit(pt);
    m_finished = true;
    return result;
}

QString OffsetCommand::commit(const lcad::Point2D& sidePoint) {
    auto batch = std::make_unique<lcad::BatchCommand>("Offset");
    int made = 0;
    int skipped = 0;

    for (lcad::EntityId id : m_ids) {
        const lcad::Entity* source = m_document.findEntity(id);
        if (!source) continue;

        std::unique_ptr<lcad::Entity> copy;
        switch (source->type()) {
        case lcad::EntityType::Line: {
            const auto& line = static_cast<const lcad::LineEntity&>(*source);
            const lcad::Point2D dir = line.end() - line.start();
            const double len = dir.length();
            if (len < 1e-9) break;
            const lcad::Point2D normal(-dir.y / len, dir.x / len);
            const double side = (sidePoint - line.start()).dot(normal) >= 0 ? 1.0 : -1.0;
            const lcad::Point2D delta = normal * (m_distance * side);
            copy = std::make_unique<lcad::LineEntity>(m_document.reserveEntityId(), source->layer(),
                                                      line.start() + delta, line.end() + delta);
            break;
        }
        case lcad::EntityType::Circle: {
            const auto& circle = static_cast<const lcad::CircleEntity&>(*source);
            const bool outward = sidePoint.distanceTo(circle.center()) >= circle.radius();
            const double newRadius = outward ? circle.radius() + m_distance : circle.radius() - m_distance;
            if (newRadius < 1e-9) break;
            copy = std::make_unique<lcad::CircleEntity>(m_document.reserveEntityId(), source->layer(),
                                                        circle.center(), newRadius);
            break;
        }
        case lcad::EntityType::Arc: {
            const auto& arc = static_cast<const lcad::ArcEntity&>(*source);
            const bool outward = sidePoint.distanceTo(arc.center()) >= arc.radius();
            const double newRadius = outward ? arc.radius() + m_distance : arc.radius() - m_distance;
            if (newRadius < 1e-9) break;
            copy = std::make_unique<lcad::ArcEntity>(m_document.reserveEntityId(), source->layer(), arc.center(),
                                                     newRadius, arc.startAngle(), arc.endAngle());
            break;
        }
        case lcad::EntityType::Polyline: {
            const auto& pl = static_cast<const lcad::PolylineEntity&>(*source);
            copy = lcad::offsetPolyline(pl, m_document.reserveEntityId(), m_distance, sidePoint);
            break;
        }
        case lcad::EntityType::Ellipse: {
            // An ellipse's offset isn't an ellipse; sample the curve, push
            // each point along its normal, and fit a spline through them --
            // the same shape AutoCAD produces.
            const auto& ellipse = static_cast<const lcad::EllipseEntity&>(*source);
            const double rx = ellipse.radiusX();
            const double ry = ellipse.radiusY();
            if (rx < 1e-9 || ry < 1e-9) break;
            const lcad::Point2D localSide =
                rotateAround(sidePoint - ellipse.center(), lcad::Point2D(), -ellipse.rotation());
            const bool outward =
                (localSide.x * localSide.x) / (rx * rx) + (localSide.y * localSide.y) / (ry * ry) > 1.0;
            if (!outward && m_distance >= std::min(rx, ry) - 1e-9) break; // would self-intersect
            const double d = outward ? m_distance : -m_distance;
            std::vector<lcad::Point2D> fit;
            const int samples = 40;
            for (int i = 0; i <= samples; ++i) {
                const double t = 2.0 * M_PI * i / samples;
                const lcad::Point2D local(rx * std::cos(t), ry * std::sin(t));
                const lcad::Point2D tangent(-rx * std::sin(t), ry * std::cos(t));
                const double tlen = tangent.length();
                if (tlen < 1e-12) continue;
                // Outward normal of the CCW-parameterized curve is its right normal.
                const lcad::Point2D normal(tangent.y / tlen, -tangent.x / tlen);
                fit.push_back(ellipse.center() +
                              rotateAround(local + normal * d, lcad::Point2D(), ellipse.rotation()));
            }
            copy = lcad::SplineEntity::fromFitPoints(m_document.reserveEntityId(), source->layer(), std::move(fit));
            break;
        }
        default:
            break;
        }

        if (copy) {
            batch->add(std::make_unique<lcad::AddEntityCommand>(m_document, std::move(copy)));
            ++made;
        } else {
            ++skipped;
        }
    }

    if (!batch->empty()) m_document.commandStack().execute(std::move(batch));
    if (skipped > 0) {
        return QStringLiteral("*%1 offset, %2 skipped (unsupported entity type or degenerate distance)*")
            .arg(made)
            .arg(skipped);
    }
    return QStringLiteral("*%1 offset*").arg(made);
}

void OffsetCommand::onPreviewPoint(const lcad::Point2D& pt) {
    m_previewPoint = pt;
    m_hasPreview = true;
}

std::vector<std::pair<lcad::Point2D, lcad::Point2D>> OffsetCommand::previewSegments() const {
    if (!m_hasDistance && m_hasDistBase && m_hasPreview) return {{m_distBase, m_previewPoint}};
    return {};
}
