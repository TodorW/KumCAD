#include "commands/DiffPairCommand.h"

#include "core/document/Commands.h"
#include "core/geometry/Track.h"
#include "core/pcb/DiffPair.h"

std::optional<QString> DiffPairCommand::onScalar(double value) {
    if (m_stage == Stage::Gap) {
        if (value <= 0.0) return std::nullopt;
        m_gap = value;
        m_stage = Stage::TrackWidth;
        return QStringLiteral("Track width <%1>:").arg(m_trackWidth);
    }
    if (m_stage == Stage::TrackWidth) {
        if (value <= 0.0) return std::nullopt;
        m_trackWidth = value;
        m_stage = Stage::Centerline;
        return QStringLiteral("Centerline: first point:");
    }
    return std::nullopt;
}

std::optional<QString> DiffPairCommand::onPoint(const lcad::Point2D& pt) {
    switch (m_stage) {
    case Stage::Gap:
    case Stage::TrackWidth:
        return std::nullopt; // these stages only take a typed number
    case Stage::Centerline:
        m_centerline.push_back(pt);
        return QStringLiteral("Centerline: next point (Enter to finish, need at least 2):");
    case Stage::PPad1:
        m_pStart = pt;
        m_stage = Stage::PPad2;
        return QStringLiteral("P net: end pad point:");
    case Stage::PPad2:
        m_pEnd = pt;
        m_stage = Stage::NPad1;
        return QStringLiteral("N net: start pad point:");
    case Stage::NPad1:
        m_nStart = pt;
        m_stage = Stage::NPad2;
        return QStringLiteral("N net: end pad point:");
    case Stage::NPad2:
        m_nEnd = pt;
        build();
        m_finished = true;
        return m_result;
    }
    return std::nullopt;
}

bool DiffPairCommand::requestFinish() {
    if (m_stage != Stage::Centerline) return false; // Enter cancels outside centerline picking
    if (m_centerline.size() < 2) {
        m_result = QStringLiteral("*Need at least 2 centerline points*");
        return true; // accepted, but isFinished() stays false: reprompt and stay active
    }
    m_stage = Stage::PPad1;
    m_result = QStringLiteral("P net: start pad point:");
    return true;
}

void DiffPairCommand::build() {
    const lcad::DiffPairResult routed =
        lcad::routeDiffPair(m_centerline, m_gap, m_pStart, m_pEnd, m_nStart, m_nEnd);
    if (!routed.ok) {
        m_result = QStringLiteral("*Could not route: degenerate centerline*");
        return;
    }

    auto batch = std::make_unique<lcad::BatchCommand>("DiffPair");
    batch->add(std::make_unique<lcad::AddEntityCommand>(
        m_document, std::make_unique<lcad::TrackEntity>(m_document.reserveEntityId(), m_document.currentLayer(),
                                                        routed.pPath, m_trackWidth)));
    batch->add(std::make_unique<lcad::AddEntityCommand>(
        m_document, std::make_unique<lcad::TrackEntity>(m_document.reserveEntityId(), m_document.currentLayer(),
                                                        routed.nPath, m_trackWidth)));
    m_document.commandStack().execute(std::move(batch));

    m_result = routed.lengthMatched
                  ? QStringLiteral("*Diff pair routed: P=%1 N=%2 (matched)*").arg(routed.pLength, 0, 'g', 5).arg(routed.nLength, 0, 'g', 5)
                  : QStringLiteral("*Diff pair routed: P=%1 N=%2 (could not fully length-match)*")
                        .arg(routed.pLength, 0, 'g', 5)
                        .arg(routed.nLength, 0, 'g', 5);
}
