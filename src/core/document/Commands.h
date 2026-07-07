#pragma once

#include "core/document/Command.h"
#include "core/document/Document.h"
#include "core/geometry/Entity.h"

#include <memory>

namespace lcad {

// Wraps Document::addEntity so it can be undone. Own the entity while it is
// not in the document (i.e. before execute() / after undo()).
class AddEntityCommand : public Command {
public:
    AddEntityCommand(Document& document, std::unique_ptr<Entity> entity)
        : m_document(document), m_entity(std::move(entity)), m_id(m_entity->id()) {}

    void execute() override { m_document.addEntity(std::move(m_entity)); }
    void undo() override { m_entity = m_document.removeEntity(m_id); }
    std::string description() const override { return "Add entity"; }

private:
    Document& m_document;
    std::unique_ptr<Entity> m_entity;
    EntityId m_id;
};

class DeleteEntityCommand : public Command {
public:
    DeleteEntityCommand(Document& document, EntityId id) : m_document(document), m_id(id) {}

    void execute() override { m_entity = m_document.removeEntity(m_id); }
    void undo() override { m_document.addEntity(std::move(m_entity)); }
    std::string description() const override { return "Delete entity"; }

private:
    Document& m_document;
    EntityId m_id;
    std::unique_ptr<Entity> m_entity;
};

} // namespace lcad
