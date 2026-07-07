#include "core/document/Command.h"

namespace lcad {

void CommandStack::execute(std::unique_ptr<Command> command) {
    command->execute();
    m_undoStack.push_back(std::move(command));
    m_redoStack.clear();
}

void CommandStack::undo() {
    if (m_undoStack.empty()) return;
    std::unique_ptr<Command> command = std::move(m_undoStack.back());
    m_undoStack.pop_back();
    command->undo();
    m_redoStack.push_back(std::move(command));
}

void CommandStack::redo() {
    if (m_redoStack.empty()) return;
    std::unique_ptr<Command> command = std::move(m_redoStack.back());
    m_redoStack.pop_back();
    command->execute();
    m_undoStack.push_back(std::move(command));
}

} // namespace lcad
