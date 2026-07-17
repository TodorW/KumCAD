#include "commands/TranCommand.h"

#include "core/schematic/Spice.h"

#include <algorithm>
#include <cstddef>

std::optional<QString> TranCommand::onText(const QString& text) {
    const QString trimmed = text.trimmed();
    if (m_stage == 0) {
        double step = 0.0;
        if (!lcad::parseEngineeringValue(trimmed.toStdString(), step) || step <= 0.0) {
            return QStringLiteral("*Enter a positive time, e.g. 1u or 100n*");
        }
        m_tStep = step;
        m_stage = 1;
        return QStringLiteral("Stop time (e.g. 1m):");
    }

    double stop = 0.0;
    if (!lcad::parseEngineeringValue(trimmed.toStdString(), stop) || stop <= 0.0) {
        return QStringLiteral("*Enter a positive time, e.g. 1m*");
    }
    run(stop);
    m_finished = true;
    return m_result;
}

void TranCommand::run(double tStop) {
    const lcad::CircuitBuildResult built = lcad::buildCircuitFromDocument(m_document);
    if (built.circuit.elements.empty()) {
        m_result = QStringLiteral("*No simulatable components (R/C/L/V/I/D with REFDES+VALUE) found*");
        return;
    }
    const lcad::OperatingPointResult op = lcad::solveDcOperatingPoint(built.circuit);
    if (!op.converged) {
        m_result = QStringLiteral("*DC operating point did not converge -- cannot start transient*");
        return;
    }

    const auto points = lcad::solveTransient(built.circuit, op, m_tStep, tStop);
    if (points.empty()) {
        m_result = QStringLiteral("*Transient analysis produced no points*");
        return;
    }

    // Report a handful of evenly-spaced samples rather than every step --
    // there's no chart widget here to plot a real waveform against.
    constexpr int kMaxSamples = 8;
    const int stride = std::max<int>(1, static_cast<int>(points.size()) / kMaxSamples);
    QString report = QStringLiteral("*TRAN: %1 step(s) to t=%2s*\n").arg(points.size()).arg(tStop);
    for (std::size_t i = 0; i < points.size(); i += static_cast<std::size_t>(stride)) {
        const auto& p = points[i];
        QString line = QStringLiteral("  t=%1s:").arg(p.time, 0, 'g', 4);
        for (int n = 1; n <= built.circuit.nodeCount; ++n) {
            line += QStringLiteral(" %1=%2V").arg(QString::fromStdString(built.nodeNames[n])).arg(p.nodeVoltages[n], 0, 'g', 4);
        }
        report += line + QStringLiteral("\n");
    }
    m_result = report.trimmed();
}
