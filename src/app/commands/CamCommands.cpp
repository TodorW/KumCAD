#include "commands/CamCommands.h"

#include "core/cam/GCodeWriter.h"
#include "core/geometry/Polyline.h"

std::optional<QString> GCodeExportCommand::onPoint(const lcad::Point2D& pt) {
    if (m_stage != Stage::Pick) return std::nullopt;

    const lcad::PolylineEntity* best = nullptr;
    double bestDist = m_pickTolerance;
    for (const lcad::Entity* e : m_document.entities()) {
        if (e->type() != lcad::EntityType::Polyline) continue;
        const auto* pl = static_cast<const lcad::PolylineEntity*>(e);
        if (!pl->closed()) continue;
        const double d = pl->distanceTo(pt);
        if (d <= bestDist) {
            bestDist = d;
            best = pl;
        }
    }
    if (!best) return QStringLiteral("*No closed polyline there*\nSelect a closed polyline profile:");

    m_profileId = best->id();
    m_stage = Stage::ToolDiameter;
    return QStringLiteral("Enter tool diameter:");
}

std::optional<QString> GCodeExportCommand::onText(const QString& text) {
    switch (m_stage) {
    case Stage::ToolDiameter: {
        bool ok = false;
        const double d = text.trimmed().toDouble(&ok);
        if (!ok || d < 0.0) return QStringLiteral("*Invalid tool diameter*");
        m_toolDiameter = d;
        m_stage = Stage::CutSide;
        return QStringLiteral("Cut side [Outside/Inside/OnLine] <OnLine>:");
    }
    case Stage::CutSide: {
        const QString side = text.trimmed().toUpper();
        if (side.isEmpty() || side == QLatin1String("ONLINE")) m_cutSide = lcad::CutSide::OnLine;
        else if (side == QLatin1String("OUTSIDE")) m_cutSide = lcad::CutSide::Outside;
        else if (side == QLatin1String("INSIDE")) m_cutSide = lcad::CutSide::Inside;
        else return QStringLiteral("*Unrecognized cut side*\nCut side <OnLine>:");
        m_stage = Stage::Path;
        return QStringLiteral("Enter output G-code file path:");
    }
    case Stage::Path: {
        m_finished = true;
        const auto* profile = static_cast<const lcad::PolylineEntity*>(m_document.findEntity(m_profileId));
        if (!profile) return QStringLiteral("*Profile no longer exists*");

        lcad::ToolpathParams params;
        params.toolDiameter = m_toolDiameter;
        params.side = m_cutSide;
        const std::vector<lcad::Point2D> path = lcad::computeToolpath(*profile, params);
        if (path.size() < 2) return QStringLiteral("*Toolpath computation failed (tool too large for a tight corner?)*");

        std::string error;
        if (!lcad::writeGCode(path, params, text.trimmed().toStdString(), &error)) {
            return QStringLiteral("*%1*").arg(QString::fromStdString(error));
        }
        return QStringLiteral("*G-code written to %1 (%2 point(s))*").arg(text.trimmed()).arg(path.size());
    }
    default:
        return std::nullopt;
    }
}
