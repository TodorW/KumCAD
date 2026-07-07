#pragma once

#include "commands/DrawCommand.h"
#include "core/document/Document.h"
#include "core/geometry/Point2D.h"

#include <QObject>
#include <QString>

#include <memory>

class CommandLine;

// Owns the currently-active DrawCommand (if any) and routes input from the
// CommandLine widget and DrawingView (clicks, mouse-move, Escape) to it,
// mirroring AutoCAD's command-line-driven interaction model.
class CommandDispatcher : public QObject {
    Q_OBJECT
public:
    CommandDispatcher(lcad::Document& document, CommandLine& commandLine, QObject* parent = nullptr);

    bool hasActiveCommand() const { return m_activeCommand != nullptr; }
    DrawCommand* activeDrawCommand() const { return m_activeCommand.get(); }

    void handlePointPicked(const lcad::Point2D& pt);
    void handleMouseMoved(const lcad::Point2D& pt);
    void handleFinishRequested();
    void handleEscape();

public slots:
    void handleCommandText(const QString& text);
    void undo();
    void redo();

signals:
    void documentChanged();
    void previewChanged();

private:
    void startCommand(std::unique_ptr<DrawCommand> command, const QString& name);
    void finishCommand();
    static bool tryParsePoint(const QString& text, lcad::Point2D& out);

    lcad::Document& m_document;
    CommandLine& m_commandLine;
    std::unique_ptr<DrawCommand> m_activeCommand;
};
