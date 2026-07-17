#pragma once

#include "commands/DrawCommand.h"
#include "core/document/Document.h"

#include <vector>

// DIFFPAIR: routes a differential pair (see core/pcb/DiffPair.h) along a
// picked centerline, connecting to four picked real pad positions (P
// start/end, N start/end), with automatic length matching.
class DiffPairCommand : public DrawCommand {
public:
    explicit DiffPairCommand(lcad::Document& document) : m_document(document) {}

    QString start() override { return QStringLiteral("DIFFPAIR  Gap between tracks:"); }
    std::optional<QString> onScalar(double value) override;
    std::optional<QString> onPoint(const lcad::Point2D& pt) override;
    bool requestFinish() override;
    std::optional<QString> resultMessage() const override { return m_result; }
    bool isFinished() const override { return m_finished; }
    void cancel() override { m_finished = true; }

private:
    enum class Stage { Gap, TrackWidth, Centerline, PPad1, PPad2, NPad1, NPad2 };
    void build();

    lcad::Document& m_document;
    Stage m_stage = Stage::Gap;
    double m_gap = 0.3;
    double m_trackWidth = 0.25;
    std::vector<lcad::Point2D> m_centerline;
    lcad::Point2D m_pStart, m_pEnd, m_nStart, m_nEnd;
    std::optional<QString> m_result;
    bool m_finished = false;
};
