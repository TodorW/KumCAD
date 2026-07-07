#pragma once

#include <QWidget>

class QPlainTextEdit;
class QLineEdit;

// Bottom-docked command transcript + input, mirroring AutoCAD's text command
// window: typed text and prompts are appended to the log above the input box.
class CommandLine : public QWidget {
    Q_OBJECT
public:
    explicit CommandLine(QWidget* parent = nullptr);

    void appendLine(const QString& text);
    QLineEdit* input() const { return m_input; }

signals:
    void commandEntered(const QString& text);

private slots:
    void onReturnPressed();

private:
    QPlainTextEdit* m_log;
    QLineEdit* m_input;
};
