#pragma once

#include "commands/DrawCommand.h"
#include "core/document/Document.h"

// LIBSAVE: comma-separated block names, then a file path -- writes those
// blocks as a part library (see core/document/PartLibrary.h).
class LibSaveCommand : public DrawCommand {
public:
    explicit LibSaveCommand(lcad::Document& document) : m_document(document) {}

    QString start() override { return QStringLiteral("LIBSAVE  Block name(s), comma-separated:"); }
    bool wantsTextInput() const override { return true; }
    std::optional<QString> onText(const QString& text) override;
    std::optional<QString> onPoint(const lcad::Point2D&) override { return std::nullopt; }
    std::optional<QString> resultMessage() const override { return m_result; }
    bool isFinished() const override { return m_finished; }
    void cancel() override { m_finished = true; }

private:
    lcad::Document& m_document;
    int m_stage = 0; // 0 = names, 1 = path
    std::vector<std::string> m_names;
    std::optional<QString> m_result;
    bool m_finished = false;
};

// LIBLOAD: a file path, then whether to overwrite blocks already present
// by name -- merges a part library into the current document.
class LibLoadCommand : public DrawCommand {
public:
    explicit LibLoadCommand(lcad::Document& document) : m_document(document) {}

    QString start() override { return QStringLiteral("LIBLOAD  Enter library file path:"); }
    bool wantsTextInput() const override { return true; }
    std::optional<QString> onText(const QString& text) override;
    std::optional<QString> onPoint(const lcad::Point2D&) override { return std::nullopt; }
    std::optional<QString> resultMessage() const override { return m_result; }
    bool isFinished() const override { return m_finished; }
    void cancel() override { m_finished = true; }

private:
    lcad::Document& m_document;
    int m_stage = 0; // 0 = path, 1 = overwrite?
    QString m_path;
    std::optional<QString> m_result;
    bool m_finished = false;
};
