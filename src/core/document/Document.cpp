#include "core/document/Document.h"

#include <algorithm>

namespace lcad {

Document::Document() {
    // Layer "0" is always present, mirroring AutoCAD's default layer.
    m_layers.push_back(Layer{0, "0", Color{255, 255, 255}, true, false});
    m_nextLayerId = 1;
    m_currentLayer = 0;
}

LayerId Document::addLayer(const std::string& name, Color color) {
    const LayerId id = m_nextLayerId++;
    m_layers.push_back(Layer{id, name, color, true, false});
    return id;
}

Layer* Document::findLayer(LayerId id) {
    auto it = std::find_if(m_layers.begin(), m_layers.end(), [id](const Layer& l) { return l.id == id; });
    return it != m_layers.end() ? &(*it) : nullptr;
}

const Layer* Document::findLayer(LayerId id) const {
    auto it = std::find_if(m_layers.begin(), m_layers.end(), [id](const Layer& l) { return l.id == id; });
    return it != m_layers.end() ? &(*it) : nullptr;
}

void Document::addEntity(std::unique_ptr<Entity> entity) {
    const EntityId id = entity->id();
    m_entityOrder.push_back(id);
    m_entityMap.emplace(id, std::move(entity));
}

std::unique_ptr<Entity> Document::removeEntity(EntityId id) {
    auto it = m_entityMap.find(id);
    if (it == m_entityMap.end()) return nullptr;
    std::unique_ptr<Entity> entity = std::move(it->second);
    m_entityMap.erase(it);
    m_entityOrder.erase(std::remove(m_entityOrder.begin(), m_entityOrder.end(), id), m_entityOrder.end());
    return entity;
}

Entity* Document::findEntity(EntityId id) {
    auto it = m_entityMap.find(id);
    return it != m_entityMap.end() ? it->second.get() : nullptr;
}

const Entity* Document::findEntity(EntityId id) const {
    auto it = m_entityMap.find(id);
    return it != m_entityMap.end() ? it->second.get() : nullptr;
}

std::vector<Entity*> Document::entities() {
    std::vector<Entity*> result;
    result.reserve(m_entityOrder.size());
    for (EntityId id : m_entityOrder) result.push_back(m_entityMap.at(id).get());
    return result;
}

std::vector<const Entity*> Document::entities() const {
    std::vector<const Entity*> result;
    result.reserve(m_entityOrder.size());
    for (EntityId id : m_entityOrder) result.push_back(m_entityMap.at(id).get());
    return result;
}

} // namespace lcad
