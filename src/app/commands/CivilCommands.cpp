#include "commands/CivilCommands.h"

#include "core/civil/Alignment.h"
#include "core/civil/Contours.h"
#include "core/civil/CutFill.h"
#include "core/civil/SurveyPoints.h"
#include "core/civil/Tin.h"
#include "core/document/Commands.h"
#include "core/geometry/Polyline.h"

std::optional<QString> TinInfoCommand::onText(const QString& text) {
    m_finished = true;
    const std::vector<lcad::SurveyPoint> points = lcad::readSurveyPointsXyz(text.trimmed().toStdString());
    if (points.empty()) return QStringLiteral("*Could not read survey points from %1*").arg(text.trimmed());
    const lcad::Tin tin = lcad::buildTin(points);
    return QStringLiteral("*TIN: %1 point(s), %2 triangle(s)*").arg(points.size()).arg(tin.triangles.size());
}

std::optional<QString> ContourCommand::onText(const QString& text) {
    if (m_stage == Stage::Path) {
        m_path = text.trimmed().toStdString();
        m_stage = Stage::Interval;
        return QStringLiteral("Enter contour interval:");
    }

    m_finished = true;
    bool ok = false;
    const double interval = text.trimmed().toDouble(&ok);
    if (!ok || interval <= 0.0) return QStringLiteral("*Invalid interval*");

    const std::vector<lcad::SurveyPoint> points = lcad::readSurveyPointsXyz(m_path);
    if (points.empty()) return QStringLiteral("*Could not read survey points from %1*").arg(QString::fromStdString(m_path));
    const lcad::Tin tin = lcad::buildTin(points);
    const auto chains = lcad::computeContours(tin, interval);

    for (const auto& chain : chains) {
        m_document.commandStack().execute(std::make_unique<lcad::AddEntityCommand>(
            m_document,
            std::make_unique<lcad::PolylineEntity>(m_document.reserveEntityId(), m_document.currentLayer(), chain)));
    }
    return QStringLiteral("*%1 contour(s) added*").arg(chains.size());
}

std::optional<QString> CutFillCommand::onText(const QString& text) {
    switch (m_stage) {
    case Stage::ExistingPath:
        m_existingPath = text.trimmed().toStdString();
        m_stage = Stage::ProposedPath;
        return QStringLiteral("Enter proposed-surface XYZ file path:");
    case Stage::ProposedPath:
        m_proposedPath = text.trimmed().toStdString();
        m_stage = Stage::CellSize;
        return QStringLiteral("Enter sample cell size:");
    case Stage::CellSize: {
        m_finished = true;
        bool ok = false;
        const double cellSize = text.trimmed().toDouble(&ok);
        if (!ok || cellSize <= 0.0) return QStringLiteral("*Invalid cell size*");

        const std::vector<lcad::SurveyPoint> existingPts = lcad::readSurveyPointsXyz(m_existingPath);
        const std::vector<lcad::SurveyPoint> proposedPts = lcad::readSurveyPointsXyz(m_proposedPath);
        if (existingPts.empty() || proposedPts.empty()) return QStringLiteral("*Could not read one or both surfaces*");

        const lcad::Tin existing = lcad::buildTin(existingPts);
        const lcad::Tin proposed = lcad::buildTin(proposedPts);
        const lcad::CutFillResult result = lcad::computeCutFillVolume(existing, proposed, cellSize);
        return QStringLiteral("*Cut: %1, Fill: %2 (cubic units, grid-sampled estimate)*")
            .arg(result.cutVolume, 0, 'f', 2)
            .arg(result.fillVolume, 0, 'f', 2);
    }
    }
    return std::nullopt;
}

QString ProfileCommand::start() {
    return QStringLiteral("PROFILE  Specify alignment point:");
}

std::optional<QString> ProfileCommand::onPoint(const lcad::Point2D& pt) {
    if (m_stage == Stage::Alignment) {
        m_alignment.push_back(pt);
        return QStringLiteral("Specify next alignment point or [Enter to finish]:");
    }
    if (m_stage == Stage::InsertionPoint) {
        const std::vector<lcad::SurveyPoint> points = lcad::readSurveyPointsXyz(m_path);
        if (points.empty()) {
            m_finished = true;
            return QStringLiteral("*Could not read survey points from %1*").arg(QString::fromStdString(m_path));
        }
        const lcad::Tin tin = lcad::buildTin(points);
        std::vector<lcad::Point2D> profile = lcad::computeGroundProfile(m_alignment, tin, m_interval);
        for (lcad::Point2D& p : profile) p = p + pt; // offset from plan-view coordinates into profile-view space

        m_finished = true;
        if (profile.size() < 2) return QStringLiteral("*Not enough ground coverage to build a profile*");
        m_document.commandStack().execute(std::make_unique<lcad::AddEntityCommand>(
            m_document, std::make_unique<lcad::PolylineEntity>(m_document.reserveEntityId(), m_document.currentLayer(),
                                                                profile)));
        return QStringLiteral("*Profile added (%1 station(s))*").arg(profile.size());
    }
    return std::nullopt;
}

bool ProfileCommand::requestFinish() {
    if (m_stage != Stage::Alignment) return false;
    if (m_alignment.size() < 2) {
        m_finished = true;
        return false;
    }
    // Enter after the alignment points: move on to the text-input stages,
    // same pattern as LeaderCommand's Points -> Text transition.
    m_stage = Stage::Path;
    return true;
}

std::optional<QString> ProfileCommand::onText(const QString& text) {
    if (m_stage == Stage::Path) {
        m_path = text.trimmed().toStdString();
        m_stage = Stage::Interval;
        return QStringLiteral("Enter sample interval:");
    }
    bool ok = false;
    const double interval = text.trimmed().toDouble(&ok);
    if (!ok || interval <= 0.0) return QStringLiteral("*Invalid interval*");
    m_interval = interval;
    m_stage = Stage::InsertionPoint;
    return QStringLiteral("Specify insertion point for the profile view:");
}
