#include "core/document/Document.h"
#include "core/geometry/Insert.h"
#include "core/geometry/Wire.h"
#include "core/pid/InstrumentTagging.h"
#include "core/pid/PidLibrary.h"
#include "core/schematic/Netlist.h"

#include <algorithm>
#include <catch2/catch_test_macros.hpp>

using namespace lcad;

TEST_CASE("registerPidSymbols adds a usable, idempotent P&ID parts library", "[pid][library]") {
    Document doc;
    registerPidSymbols(doc);

    for (const char* name : {"VALVE", "PUMP", "VESSEL", "INSTRUMENT"}) {
        const BlockDefinition* block = doc.findBlock(name);
        REQUIRE(block);
        REQUIRE(block->isSymbol());
    }
    REQUIRE(doc.findBlock("VALVE")->pins.size() == 2);
    REQUIRE(doc.findBlock("INSTRUMENT")->pins.size() == 1);

    doc.findBlock("VALVE")->pins[0].name = "customized";
    registerPidSymbols(doc);
    REQUIRE(doc.findBlock("VALVE")->pins[0].name == "customized");
}

TEST_CASE("A valve, pump, and vessel wired together form one process net", "[pid][netlist]") {
    Document doc;
    registerPidSymbols(doc);

    auto valve = std::make_unique<InsertEntity>(doc.reserveEntityId(), doc.currentLayer(), doc.findBlock("VALVE"),
                                                Point2D(0, 0));
    const EntityId valveId = valve->id();
    doc.addEntity(std::move(valve));
    auto vessel = std::make_unique<InsertEntity>(doc.reserveEntityId(), doc.currentLayer(), doc.findBlock("VESSEL"),
                                                 Point2D(100, -35));
    const EntityId vesselId = vessel->id();
    doc.addEntity(std::move(vessel));
    // Valve OUT (world (20,0)) to Vessel IN (world (110,0)).
    doc.addEntity(std::make_unique<WireEntity>(doc.reserveEntityId(), doc.currentLayer(),
                                                std::vector<Point2D>{Point2D(20, 0), Point2D(110, 0)}));

    const std::vector<Net> nets = computeNets(doc);
    const auto net = std::find_if(nets.begin(), nets.end(), [](const Net& n) { return n.pins.size() == 2; });
    REQUIRE(net != nets.end());
    const bool hasValve =
        std::any_of(net->pins.begin(), net->pins.end(), [&](const NetPin& p) { return p.insertId == valveId; });
    const bool hasVessel =
        std::any_of(net->pins.begin(), net->pins.end(), [&](const NetPin& p) { return p.insertId == vesselId; });
    REQUIRE(hasValve);
    REQUIRE(hasVessel);
}

TEST_CASE("assignInstrumentTags tags only INSTRUMENT symbols, once, and is idempotent", "[pid][tagging]") {
    Document doc;
    registerPidSymbols(doc);

    auto inst1 = std::make_unique<InsertEntity>(doc.reserveEntityId(), doc.currentLayer(), doc.findBlock("INSTRUMENT"),
                                                Point2D(0, 0));
    const EntityId inst1Id = inst1->id();
    doc.addEntity(std::move(inst1));
    auto inst2 = std::make_unique<InsertEntity>(doc.reserveEntityId(), doc.currentLayer(), doc.findBlock("INSTRUMENT"),
                                                Point2D(50, 0));
    const EntityId inst2Id = inst2->id();
    doc.addEntity(std::move(inst2));
    // A non-instrument symbol must never get a REFDES from this function.
    auto valve = std::make_unique<InsertEntity>(doc.reserveEntityId(), doc.currentLayer(), doc.findBlock("VALVE"),
                                                Point2D(100, 0));
    const EntityId valveId = valve->id();
    doc.addEntity(std::move(valve));

    assignInstrumentTags(doc);

    const auto* inst1Ptr = static_cast<const InsertEntity*>(doc.findEntity(inst1Id));
    const auto* inst2Ptr = static_cast<const InsertEntity*>(doc.findEntity(inst2Id));
    const auto* valvePtr = static_cast<const InsertEntity*>(doc.findEntity(valveId));
    REQUIRE(inst1Ptr->attributeValue("REFDES"));
    REQUIRE(inst2Ptr->attributeValue("REFDES"));
    REQUIRE_FALSE(valvePtr->attributeValue("REFDES"));
    REQUIRE(*inst1Ptr->attributeValue("REFDES") != *inst2Ptr->attributeValue("REFDES"));

    const std::string firstTag = *inst1Ptr->attributeValue("REFDES");
    assignInstrumentTags(doc);
    REQUIRE(*inst1Ptr->attributeValue("REFDES") == firstTag); // unchanged on a second call
}
