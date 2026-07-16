#include "core/schematic/SymbolLibrary.h"

#include "core/document/Document.h"
#include "core/geometry/Line.h"
#include "core/geometry/Polyline.h"

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

void registerBuiltinSymbols(Document& doc) {
    // Resistor: IEEE-315-flavored zigzag body between x=5 and x=15; pins are
    // the outer tips at x=0/x=20, stubs drawn from the body out to them.
    {
        std::vector<std::unique_ptr<Entity>> body;
        const std::vector<Point2D> zig = {Point2D(5, 0),   Point2D(6.5, 0), Point2D(7.5, 3),  Point2D(9.5, -3),
                                          Point2D(11.5, 3), Point2D(13.5, -3), Point2D(14.5, 0), Point2D(15, 0)};
        for (std::size_t i = 0; i + 1 < zig.size(); ++i) addLine(doc, body, zig[i], zig[i + 1]);
        std::vector<Pin> pins = {
            Pin{"1", "1", PinElectricalType::Passive, Point2D(0, 0), Point2D(5, 0)},
            Pin{"2", "2", PinElectricalType::Passive, Point2D(20, 0), Point2D(15, 0)},
        };
        addIfMissing(doc, "R", std::move(body), std::move(pins));
    }

    // Capacitor: two parallel plates at x=9/x=11; the pin stubs themselves
    // are the leads, so no separate lead-line entities are needed.
    {
        std::vector<std::unique_ptr<Entity>> body;
        addLine(doc, body, Point2D(9, -5), Point2D(9, 5));
        addLine(doc, body, Point2D(11, -5), Point2D(11, 5));
        std::vector<Pin> pins = {
            Pin{"1", "1", PinElectricalType::Passive, Point2D(0, 0), Point2D(9, 0)},
            Pin{"2", "2", PinElectricalType::Passive, Point2D(20, 0), Point2D(11, 0)},
        };
        addIfMissing(doc, "C", std::move(body), std::move(pins));
    }

    // Diode: anode-side triangle (base at x=8) pointing into a cathode bar
    // at x=14.
    {
        std::vector<std::unique_ptr<Entity>> body;
        addLine(doc, body, Point2D(8, -4), Point2D(8, 4));
        addLine(doc, body, Point2D(8, -4), Point2D(14, 0));
        addLine(doc, body, Point2D(8, 4), Point2D(14, 0));
        addLine(doc, body, Point2D(14, -4), Point2D(14, 4));
        std::vector<Pin> pins = {
            Pin{"1", "1", PinElectricalType::Passive, Point2D(0, 0), Point2D(8, 0)},   // anode
            Pin{"2", "2", PinElectricalType::Passive, Point2D(20, 0), Point2D(14, 0)}, // cathode
        };
        addIfMissing(doc, "D", std::move(body), std::move(pins));
    }

    // Generic 2-pin connector: a small body box with both pins on its left
    // edge, like a 2-way header.
    {
        std::vector<std::unique_ptr<Entity>> body;
        body.push_back(std::make_unique<PolylineEntity>(
            doc.reserveEntityId(), LayerId(0),
            std::vector<Point2D>{Point2D(5, -6), Point2D(15, -6), Point2D(15, 6), Point2D(5, 6)}, true));
        std::vector<Pin> pins = {
            Pin{"1", "1", PinElectricalType::Passive, Point2D(0, -3), Point2D(5, -3)},
            Pin{"2", "2", PinElectricalType::Passive, Point2D(0, 3), Point2D(5, 3)},
        };
        addIfMissing(doc, "CONN2", std::move(body), std::move(pins));
    }

    // Generic 4-pin IC: a DIP-style rectangle body with two pins per side.
    {
        std::vector<std::unique_ptr<Entity>> body;
        body.push_back(std::make_unique<PolylineEntity>(
            doc.reserveEntityId(), LayerId(0),
            std::vector<Point2D>{Point2D(0, -10), Point2D(20, -10), Point2D(20, 10), Point2D(0, 10)}, true));
        std::vector<Pin> pins = {
            Pin{"1", "1", PinElectricalType::Input, Point2D(-5, 7), Point2D(0, 7)},
            Pin{"2", "2", PinElectricalType::Power, Point2D(-5, -7), Point2D(0, -7)},
            Pin{"3", "3", PinElectricalType::Output, Point2D(25, -7), Point2D(20, -7)},
            Pin{"4", "4", PinElectricalType::Power, Point2D(25, 7), Point2D(20, 7)},
        };
        addIfMissing(doc, "IC", std::move(body), std::move(pins));
    }

    // A matching PCB footprint for each schematic symbol above (see
    // BlockDefinition::pads) -- named "<Symbol>_FP" so both live in the
    // same document's block table without colliding.
    {
        std::vector<std::unique_ptr<Entity>> body;
        body.push_back(std::make_unique<PolylineEntity>(
            doc.reserveEntityId(), LayerId(0),
            std::vector<Point2D>{Point2D(-2, -2), Point2D(12, -2), Point2D(12, 2), Point2D(-2, 2)}, true));
        std::vector<Pad> pads = {
            Pad{"1", PadShape::Rect, Point2D(0, 0), 1.5, 1.5, 0.0},
            Pad{"2", PadShape::Rect, Point2D(10, 0), 1.5, 1.5, 0.0},
        };
        if (!doc.findBlock("R_FP")) {
            doc.addBlock("R_FP", std::move(body));
            if (BlockDefinition* block = doc.findBlock("R_FP")) block->pads = std::move(pads);
        }
    }
    {
        std::vector<std::unique_ptr<Entity>> body;
        body.push_back(std::make_unique<PolylineEntity>(
            doc.reserveEntityId(), LayerId(0),
            std::vector<Point2D>{Point2D(0, -10), Point2D(20, -10), Point2D(20, 10), Point2D(0, 10)}, true));
        std::vector<Pad> pads = {
            Pad{"1", PadShape::Round, Point2D(0, 7), 1.6, 1.6, 0.8},
            Pad{"2", PadShape::Round, Point2D(0, -7), 1.6, 1.6, 0.8},
            Pad{"3", PadShape::Round, Point2D(20, -7), 1.6, 1.6, 0.8},
            Pad{"4", PadShape::Round, Point2D(20, 7), 1.6, 1.6, 0.8},
        };
        if (!doc.findBlock("IC_FP")) {
            doc.addBlock("IC_FP", std::move(body));
            if (BlockDefinition* block = doc.findBlock("IC_FP")) block->pads = std::move(pads);
        }
    }
}

} // namespace lcad
