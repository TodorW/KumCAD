#include "core/electrical/WireNumbering.h"

#include "core/document/Document.h"
#include "core/geometry/NetLabel.h"
#include "core/geometry/Wire.h"

#include <cctype>
#include <cmath>
#include <cstdint>
#include <map>

namespace lcad {

namespace {

constexpr double kEpsilon = 1e-6;
using Key = std::pair<std::int64_t, std::int64_t>;

Key quantize(const Point2D& p) {
    return {static_cast<std::int64_t>(std::llround(p.x / kEpsilon)), static_cast<std::int64_t>(std::llround(p.y / kEpsilon))};
}

// "W<digits>" parsed as digits, or nullopt otherwise.
std::optional<int> wireNumberOf(const std::string& name) {
    if (name.size() < 2 || (name[0] != 'W' && name[0] != 'w')) return std::nullopt;
    for (std::size_t i = 1; i < name.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(name[i]))) return std::nullopt;
    }
    return std::stoi(name.substr(1));
}

} // namespace

void assignWireNumbers(Document& doc) {
    std::map<Key, bool> labeledPoints; // true where a "W<n>" label already sits
    int nextNumber = 1;
    for (const Entity* e : doc.entities()) {
        if (e->type() != EntityType::NetLabel) continue;
        const auto* label = static_cast<const NetLabelEntity*>(e);
        if (const auto n = wireNumberOf(label->name())) {
            labeledPoints[quantize(label->position())] = true;
            nextNumber = std::max(nextNumber, *n + 1);
        }
    }

    std::vector<const WireEntity*> wires;
    for (const Entity* e : doc.entities()) {
        if (e->type() == EntityType::Wire) wires.push_back(static_cast<const WireEntity*>(e));
    }

    std::vector<std::pair<EntityId, Point2D>> toLabel;
    for (const auto* wire : wires) {
        const auto& verts = wire->vertices();
        if (verts.empty()) continue;
        const Point2D mid = (verts.front() + verts.back()) * 0.5;
        if (labeledPoints.count(quantize(mid))) continue; // this wire already has its own W<n> label
        toLabel.emplace_back(wire->id(), mid);
    }

    for (const auto& [wireId, mid] : toLabel) {
        (void)wireId;
        doc.addEntity(std::make_unique<NetLabelEntity>(doc.reserveEntityId(), doc.currentLayer(), mid,
                                                       "W" + std::to_string(nextNumber++)));
    }
}

} // namespace lcad
