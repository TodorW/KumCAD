#pragma once

#include "commands/DrawCommand.h"
#include "core/document/Document.h"

// TRAN: prompts for a step size and stop time, then runs a backward-Euler
// transient simulation (see core/schematic/Spice.h) from the circuit's DC
// operating point and reports a handful of evenly-spaced sample points --
// there's no chart widget in this app to plot a real waveform against, a
// disclosed simplification vs. a real SPICE tool's transient plot.
class TranCommand : public DrawCommand {
public:
    explicit TranCommand(lcad::Document& document) : m_document(document) {}

    QString start() override { return QStringLiteral("TRAN  Step time (e.g. 1u, 100n):"); }
    bool wantsTextInput() const override { return true; }
    std::optional<QString> onText(const QString& text) override;
    std::optional<QString> onPoint(const lcad::Point2D&) override { return std::nullopt; }
    std::optional<QString> resultMessage() const override { return m_result; }
    bool isFinished() const override { return m_finished; }
    void cancel() override { m_finished = true; }

private:
    void run(double tStop);

    lcad::Document& m_document;
    int m_stage = 0; // 0 = step time, 1 = stop time
    double m_tStep = 0.0;
    std::optional<QString> m_result;
    bool m_finished = false;
};
