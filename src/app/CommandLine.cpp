#include "CommandLine.h"

#include <QLineEdit>
#include <QPlainTextEdit>
#include <QVBoxLayout>

CommandLine::CommandLine(QWidget* parent) : QWidget(parent) {
    m_log = new QPlainTextEdit(this);
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(500);
    m_log->setFixedHeight(90);

    m_input = new QLineEdit(this);
    m_input->setPlaceholderText(QStringLiteral("Type a command..."));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->addWidget(m_log);
    layout->addWidget(m_input);

    connect(m_input, &QLineEdit::returnPressed, this, &CommandLine::onReturnPressed);
}

void CommandLine::appendLine(const QString& text) {
    m_log->appendPlainText(text);
}

void CommandLine::onReturnPressed() {
    const QString text = m_input->text();
    if (!text.isEmpty()) appendLine(text);
    m_input->clear();
    emit commandEntered(text);
}
