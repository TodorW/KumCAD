#pragma once

#include <memory>
#include <string>
#include <vector>

namespace lcad {

class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual std::string description() const = 0;
};

class CommandStack {
public:
    void execute(std::unique_ptr<Command> command);
    bool canUndo() const { return !m_undoStack.empty(); }
    bool canRedo() const { return !m_redoStack.empty(); }
    void undo();
    void redo();

private:
    std::vector<std::unique_ptr<Command>> m_undoStack;
    std::vector<std::unique_ptr<Command>> m_redoStack;
};

} // namespace lcad
