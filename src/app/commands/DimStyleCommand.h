#pragma once

#include "commands/DrawCommand.h"
#include "core/document/Document.h"

// DIMSTYLE-lite: walks through the document's dimension style values (text
// height, arrow size, decimal places). Enter keeps the current value at each
// prompt; new dimensions pick the style up at creation.
class DimStyleCommand : public DrawCommand {
public:
    explicit DimStyleCommand(lcad::Document& document) : m_document(document) {}

    QString start() override {
        return QStringLiteral("DIMSTYLE  Text height <%1>:").arg(m_document.dimStyle().textHeight);
    }

    std::optional<QString> onPoint(const lcad::Point2D& pt) override {
        (void)pt;
        return std::nullopt;
    }

    bool wantsTextInput() const override { return true; }
    std::optional<QString> onText(const QString& text) override;

    bool isFinished() const override { return m_finished; }
    void cancel() override { m_finished = true; }

private:
    enum class Stage { TextHeight, ArrowSize, Decimals };

    lcad::Document& m_document;
    Stage m_stage = Stage::TextHeight;
    bool m_finished = false;
};
