#pragma once

#include "commands/DrawCommand.h"

// AutoCAD's CAL/QuickCalc: evaluates a single typed arithmetic expression
// (see core/util/Expr.h for the supported grammar) and reports the result.
// A malformed expression re-prompts instead of ending the command; Enter
// with nothing typed cancels.
class QuickCalcCommand : public DrawCommand {
public:
    QString start() override { return QStringLiteral("QUICKCALC (CAL)  Enter expression:"); }
    std::optional<QString> onPoint(const lcad::Point2D& pt) override {
        (void)pt;
        return std::nullopt;
    }
    bool wantsTextInput() const override { return true; }
    std::optional<QString> onText(const QString& text) override;
    bool isFinished() const override { return m_finished; }
    void cancel() override { m_finished = true; }

private:
    bool m_finished = false;
};
