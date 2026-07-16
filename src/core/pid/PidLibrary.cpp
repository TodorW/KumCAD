#include "core/pid/PidLibrary.h"

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

void registerPidSymbols(Document& doc) {
    // Control valve: a bowtie (two triangles meeting at the stem).
    {
        std::vector<std::unique_ptr<Entity>> body;
        addLine(doc, body, Point2D(5, -5), Point2D(5, 5));
        addLine(doc, body, Point2D(5, -5), Point2D(10, 0));
        addLine(doc, body, Point2D(5, 5), Point2D(10, 0));
        addLine(doc, body, Point2D(15, -5), Point2D(15, 5));
        addLine(doc, body, Point2D(15, -5), Point2D(10, 0));
        addLine(doc, body, Point2D(15, 5), Point2D(10, 0));
        addLine(doc, body, Point2D(10, 0), Point2D(10, 8)); // actuator stem
        std::vector<Pin> pins = {
            Pin{"IN", "IN", PinElectricalType::Passive, Point2D(0, 0), Point2D(5, 0)},
            Pin{"OUT", "OUT", PinElectricalType::Passive, Point2D(20, 0), Point2D(15, 0)},
        };
        addIfMissing(doc, "VALVE", std::move(body), std::move(pins));
    }

    // Centrifugal pump: a circle with an impeller triangle.
    {
        std::vector<std::unique_ptr<Entity>> body;
        body.push_back(std::make_unique<CircleEntity>(doc.reserveEntityId(), LayerId(0), Point2D(10, 10), 8));
        addLine(doc, body, Point2D(4, 6), Point2D(16, 10));
        addLine(doc, body, Point2D(16, 10), Point2D(4, 14));
        std::vector<Pin> pins = {
            Pin{"IN", "IN", PinElectricalType::Passive, Point2D(0, -5), Point2D(2, 2)},
            Pin{"OUT", "OUT", PinElectricalType::Passive, Point2D(20, 25), Point2D(18, 18)},
        };
        addIfMissing(doc, "PUMP", std::move(body), std::move(pins));
    }

    // Vessel/tank: a simple rectangular body.
    {
        std::vector<std::unique_ptr<Entity>> body;
        body.push_back(std::make_unique<PolylineEntity>(
            doc.reserveEntityId(), LayerId(0),
            std::vector<Point2D>{Point2D(0, 0), Point2D(20, 0), Point2D(20, 30), Point2D(0, 30)}, true));
        std::vector<Pin> pins = {
            Pin{"IN", "IN", PinElectricalType::Passive, Point2D(10, 35), Point2D(10, 30)},
            Pin{"OUT", "OUT", PinElectricalType::Passive, Point2D(10, -5), Point2D(10, 0)},
        };
        addIfMissing(doc, "VESSEL", std::move(body), std::move(pins));
    }

    // Generic instrument bubble: a circle with a placeholder function-code
    // tag (real ISA-5.1 tagging is done per-instance -- see
    // core/pid/InstrumentTagging.h) and one signal-line connection point.
    {
        std::vector<std::unique_ptr<Entity>> body;
        body.push_back(std::make_unique<CircleEntity>(doc.reserveEntityId(), LayerId(0), Point2D(10, 10), 10));
        body.push_back(std::make_unique<TextEntity>(doc.reserveEntityId(), LayerId(0), Point2D(4, 8), "FT", 4.0));
        std::vector<Pin> pins = {
            Pin{"SIG", "SIG", PinElectricalType::Passive, Point2D(10, -5), Point2D(10, 0)},
        };
        addIfMissing(doc, "INSTRUMENT", std::move(body), std::move(pins));
    }
}

} // namespace lcad
