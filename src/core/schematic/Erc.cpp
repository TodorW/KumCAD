#include "core/schematic/Erc.h"

#include "core/document/Document.h"
#include "core/geometry/Insert.h"
#include "core/geometry/NoConnect.h"

namespace lcad {

namespace {

constexpr double kEpsilon = 1e-6;

const InsertEntity* findInsert(const Document& doc, EntityId id) {
    const Entity* e = doc.findEntity(id);
    if (!e || e->type() != EntityType::Insert) return nullptr;
    return static_cast<const InsertEntity*>(e);
}

const Pin* findPin(const InsertEntity& insert, const std::string& number, Point2D* worldOut) {
    for (const auto& pinWorld : insert.pinWorldPositions()) {
        if (pinWorld.pin->number == number) {
            if (worldOut) *worldOut = pinWorld.attach;
            return pinWorld.pin;
        }
    }
    return nullptr;
}

bool hasNoConnectAt(const Document& doc, const Point2D& world) {
    for (const Entity* e : doc.entities()) {
        if (e->type() != EntityType::NoConnect) continue;
        const auto* nc = static_cast<const NoConnectEntity*>(e);
        if (world.distanceTo(nc->position()) < kEpsilon) return true;
    }
    return false;
}

} // namespace

std::vector<ErcIssue> runErc(const Document& doc, const std::vector<Net>& nets) {
    std::vector<ErcIssue> issues;

    for (const Net& net : nets) {
        int outputCount = 0;
        for (const NetPin& np : net.pins) {
            const InsertEntity* insert = findInsert(doc, np.insertId);
            if (!insert) continue;
            Point2D world;
            const Pin* pin = findPin(*insert, np.pinNumber, &world);
            if (!pin) continue;

            if (net.pins.size() == 1) {
                if (pin->electricalType == PinElectricalType::NotConnected) continue;
                if (hasNoConnectAt(doc, world)) continue;
                issues.push_back({ErcIssue::Severity::Warning,
                                  "Pin " + pin->number + " (" + pin->name + ") is unconnected", np.insertId});
            } else if (pin->electricalType == PinElectricalType::NotConnected) {
                issues.push_back({ErcIssue::Severity::Warning,
                                  "Pin " + pin->number + " (" + pin->name +
                                      ") is marked Not Connected but is wired to net \"" + net.name + "\"",
                                  np.insertId});
            }

            if (pin->electricalType == PinElectricalType::Output) ++outputCount;
        }
        if (outputCount > 1) {
            issues.push_back(
                {ErcIssue::Severity::Error, "Net \"" + net.name + "\" has " + std::to_string(outputCount) +
                                                " Output pins driving it (conflict)",
                 0});
        }
    }

    return issues;
}

} // namespace lcad
