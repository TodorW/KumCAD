#pragma once

#include "core/Ids.h"
#include "core/document/Block.h"
#include "core/document/Command.h"
#include "core/document/Layer.h"
#include "core/geometry/Entity.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace lcad {

class Document {
public:
    Document();

    // Layers
    LayerId addLayer(const std::string& name, Color color);
    Layer* findLayer(LayerId id);
    const Layer* findLayer(LayerId id) const;
    const std::vector<Layer>& layers() const { return m_layers; }
    LayerId currentLayer() const { return m_currentLayer; }
    void setCurrentLayer(LayerId id) { m_currentLayer = id; }

    // Entities. addEntity/removeEntity are the low-level primitives that Commands
    // (see Commands.h) wrap to make every mutation undoable.
    EntityId reserveEntityId() { return m_nextEntityId++; }
    void addEntity(std::unique_ptr<Entity> entity);
    std::unique_ptr<Entity> removeEntity(EntityId id);
    Entity* findEntity(EntityId id);
    const Entity* findEntity(EntityId id) const;
    std::vector<Entity*> entities();
    std::vector<const Entity*> entities() const;

    // Blocks. Definitions are append-only (deleting one would dangle any
    // InsertEntity referencing it) and owned by the document, so pointers to
    // them stay valid for the document's lifetime, including across moves.
    const BlockDefinition* addBlock(std::string name, std::vector<std::unique_ptr<Entity>> entities);
    const BlockDefinition* findBlock(const std::string& name) const;
    const std::vector<std::unique_ptr<BlockDefinition>>& blocks() const { return m_blocks; }

    CommandStack& commandStack() { return m_commandStack; }

private:
    std::vector<Layer> m_layers;
    LayerId m_nextLayerId = 1;
    LayerId m_currentLayer = 0;

    std::unordered_map<EntityId, std::unique_ptr<Entity>> m_entityMap;
    std::vector<EntityId> m_entityOrder;
    EntityId m_nextEntityId = 1;

    std::vector<std::unique_ptr<BlockDefinition>> m_blocks;

    CommandStack m_commandStack;
};

} // namespace lcad
