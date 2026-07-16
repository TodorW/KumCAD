#include "core/electrical/ElectricalLibrary.h"

#include "core/document/Document.h"
#include "core/geometry/Circle.h"
#include "core/geometry/Line.h"
#include "core/geometry/Polyline.h"
#include "core/geometry/Text.h"

namespace lcad {

namespace {

void addLine(Document& doc, std::vector<std::unique_ptr<Entity>>& body, Point2D a, Point2D b) {
    body.push_back(std::make_unique<LineEntity>(doc.reserveEntityId(), 0, a, b));
}

void addIfMissing(Document& doc, const std::string& name, std::vector<std::unique_ptr<Entity>> body,
                  std::vector<Pin> pins) {
    if (doc.findBlock(name)) return;
    doc.addBlock(name, std::move(body));
    if (BlockDefinition* block = doc.findBlock(name)) block->pins = std::move(pins);
}

} // namespace

void registerElectricalSymbols(Document& doc) {
    // Contactor: a coil plus two normally-open power contacts, each drawn
    // with a small gap to read as an open switch.
    {
        std::vector<std::unique_ptr<Entity>> body;
        body.push_back(std::make_unique<PolylineEntity>(
            doc.reserveEntityId(), LayerId(0),
            std::vector<Point2D>{Point2D(0, 20), Point2D(20, 20), Point2D(20, 28), Point2D(0, 28)}, true));
        addLine(doc, body, Point2D(5, 0), Point2D(5, 6));
        addLine(doc, body, Point2D(5, 9), Point2D(5, 15));
        addLine(doc, body, Point2D(15, 0), Point2D(15, 6));
        addLine(doc, body, Point2D(15, 9), Point2D(15, 15));
        std::vector<Pin> pins = {
            Pin{"A1", "A1", PinElectricalType::Power, Point2D(-5, 24), Point2D(0, 24)},
            Pin{"A2", "A2", PinElectricalType::Power, Point2D(25, 24), Point2D(20, 24)},
            Pin{"1", "1", PinElectricalType::Passive, Point2D(5, -5), Point2D(5, 0)},
            Pin{"2", "2", PinElectricalType::Passive, Point2D(5, 20), Point2D(5, 15)},
            Pin{"3", "3", PinElectricalType::Passive, Point2D(15, -5), Point2D(15, 0)},
            Pin{"4", "4", PinElectricalType::Passive, Point2D(15, 20), Point2D(15, 15)},
        };
        addIfMissing(doc, "KM", std::move(body), std::move(pins));
    }

    // Relay: a smaller coil plus one common/NO contact pair.
    {
        std::vector<std::unique_ptr<Entity>> body;
        body.push_back(std::make_unique<PolylineEntity>(
            doc.reserveEntityId(), LayerId(0),
            std::vector<Point2D>{Point2D(0, 20), Point2D(14, 20), Point2D(14, 28), Point2D(0, 28)}, true));
        addLine(doc, body, Point2D(7, 0), Point2D(7, 6));
        addLine(doc, body, Point2D(7, 9), Point2D(7, 15));
        std::vector<Pin> pins = {
            Pin{"A1", "A1", PinElectricalType::Power, Point2D(-5, 24), Point2D(0, 24)},
            Pin{"A2", "A2", PinElectricalType::Power, Point2D(19, 24), Point2D(14, 24)},
            Pin{"11", "11", PinElectricalType::Passive, Point2D(7, -5), Point2D(7, 0)}, // common
            Pin{"14", "14", PinElectricalType::Passive, Point2D(7, 20), Point2D(7, 15)}, // normally-open
        };
        addIfMissing(doc, "K", std::move(body), std::move(pins));
    }

    // Terminal block: a simple two-pin feedthrough.
    {
        std::vector<std::unique_ptr<Entity>> body;
        body.push_back(std::make_unique<PolylineEntity>(
            doc.reserveEntityId(), LayerId(0),
            std::vector<Point2D>{Point2D(5, -4), Point2D(15, -4), Point2D(15, 4), Point2D(5, 4)}, true));
        std::vector<Pin> pins = {
            Pin{"1", "1", PinElectricalType::Passive, Point2D(0, 0), Point2D(5, 0)},
            Pin{"2", "2", PinElectricalType::Passive, Point2D(20, 0), Point2D(15, 0)},
        };
        addIfMissing(doc, "TB", std::move(body), std::move(pins));
    }

    // Three-phase motor: a circle with a "M" tag and U/V/W power pins.
    {
        std::vector<std::unique_ptr<Entity>> body;
        body.push_back(std::make_unique<CircleEntity>(doc.reserveEntityId(), LayerId(0), Point2D(10, 10), 10));
        body.push_back(std::make_unique<TextEntity>(doc.reserveEntityId(), LayerId(0), Point2D(7, 8), "M", 5.0));
        std::vector<Pin> pins = {
            Pin{"U", "U", PinElectricalType::Power, Point2D(2, -5), Point2D(2, 0)},
            Pin{"V", "V", PinElectricalType::Power, Point2D(10, -5), Point2D(10, 0)},
            Pin{"W", "W", PinElectricalType::Power, Point2D(18, -5), Point2D(18, 0)},
        };
        addIfMissing(doc, "MOTOR", std::move(body), std::move(pins));
    }
}

} // namespace lcad
