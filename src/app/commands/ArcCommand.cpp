#include "commands/ArcCommand.h"

#include "core/document/Commands.h"
#include "core/geometry/Arc.h"

#include <cmath>

namespace {

constexpr double kTwoPi = 2.0 * M_PI;

double normalizeAngle(double angle) {
    angle = std::fmod(angle, kTwoPi);
    if (angle < 0) angle += kTwoPi;
    return angle;
}

bool angleBetweenCcw(double angle, double start, double end) {
    const double a = normalizeAngle(angle);
    const double s = normalizeAngle(start);
    const double e = normalizeAngle(end);
    if (s <= e) return a >= s && a <= e;
    return a >= s || a <= e;
}

} // namespace

QString ArcCommand::start() {
    return QStringLiteral("ARC  Specify start point:");
}

std::optional<QString> ArcCommand::onPoint(const lcad::Point2D& pt) {
    m_points[m_pointCount++] = pt;

    if (m_pointCount == 1) return QStringLiteral("Specify second point on arc:");
    if (m_pointCount == 2) return QStringLiteral("Specify end point:");

    // Circumcircle through the three picked points.
    const lcad::Point2D& a = m_points[0];
    const lcad::Point2D& b = m_points[1];
    const lcad::Point2D& c = m_points[2];
    const double d = 2.0 * (a.x * (b.y - c.y) + b.x * (c.y - a.y) + c.x * (a.y - b.y));
    if (std::abs(d) < 1e-9) {
        // Collinear points: no valid arc, just abandon this attempt.
        m_finished = true;
        return std::nullopt;
    }

    const double aSq = a.x * a.x + a.y * a.y;
    const double bSq = b.x * b.x + b.y * b.y;
    const double cSq = c.x * c.x + c.y * c.y;
    const double ux = (aSq * (b.y - c.y) + bSq * (c.y - a.y) + cSq * (a.y - b.y)) / d;
    const double uy = (aSq * (c.x - b.x) + bSq * (a.x - c.x) + cSq * (b.x - a.x)) / d;
    const lcad::Point2D center(ux, uy);
    const double radius = center.distanceTo(a);

    double startAngle = std::atan2(a.y - center.y, a.x - center.x);
    double endAngle = std::atan2(c.y - center.y, c.x - center.x);
    const double midAngle = std::atan2(b.y - center.y, b.x - center.x);
    if (!angleBetweenCcw(midAngle, startAngle, endAngle)) std::swap(startAngle, endAngle);

    const auto id = m_document.reserveEntityId();
    auto entity = std::make_unique<lcad::ArcEntity>(id, m_document.currentLayer(), center, radius, startAngle, endAngle);
    m_document.commandStack().execute(std::make_unique<lcad::AddEntityCommand>(m_document, std::move(entity)));
    m_finished = true;
    return std::nullopt;
}

void ArcCommand::onPreviewPoint(const lcad::Point2D& pt) {
    m_previewPoint = pt;
    m_hasPreview = true;
}

std::vector<std::pair<lcad::Point2D, lcad::Point2D>> ArcCommand::previewSegments() const {
    if (m_pointCount == 0 || !m_hasPreview) return {};
    std::vector<std::pair<lcad::Point2D, lcad::Point2D>> segs;
    for (int i = 0; i + 1 < m_pointCount; ++i) segs.emplace_back(m_points[i], m_points[i + 1]);
    segs.emplace_back(m_points[m_pointCount - 1], m_previewPoint);
    return segs;
}
