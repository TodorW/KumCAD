#include "core/document/Document.h"
#include "core/electrical/ElectricalLibrary.h"
#include "core/electrical/WireList.h"
#include "core/electrical/WireNumbering.h"
#include "core/geometry/Insert.h"
#include "core/geometry/NetLabel.h"
#include "core/geometry/Table.h"
#include "core/geometry/Wire.h"
#include "core/schematic/Netlist.h"

#include <algorithm>
#include <catch2/catch_test_macros.hpp>

using namespace lcad;

TEST_CASE("registerElectricalSymbols adds a usable, idempotent panel parts library", "[electrical][library]") {
    Document doc;
    registerElectricalSymbols(doc);

    for (const char* name : {"KM", "K", "TB", "MOTOR"}) {
        const BlockDefinition* block = doc.findBlock(name);
        REQUIRE(block);
        REQUIRE(block->isSymbol());
    }
    REQUIRE(doc.findBlock("KM")->pins.size() == 6);
    REQUIRE(doc.findBlock("MOTOR")->pins.size() == 3);

    doc.findBlock("KM")->pins[0].name = "customized";
    registerElectricalSymbols(doc);
    REQUIRE(doc.findBlock("KM")->pins[0].name == "customized");
}

TEST_CASE("assignWireNumbers labels every unlabeled wire once and is idempotent", "[electrical][wirenum]") {
    Document doc;
    doc.addEntity(std::make_unique<WireEntity>(doc.reserveEntityId(), doc.currentLayer(),
                                                std::vector<Point2D>{Point2D(0, 0), Point2D(10, 0)}));
    doc.addEntity(std::make_unique<WireEntity>(doc.reserveEntityId(), doc.currentLayer(),
                                                std::vector<Point2D>{Point2D(0, 20), Point2D(10, 20)}));

    assignWireNumbers(doc);
    int labelCount = 0;
    for (const Entity* e : doc.entities()) {
        if (e->type() == EntityType::NetLabel) ++labelCount;
    }
    REQUIRE(labelCount == 2);

    // Calling again must not add more labels to already-numbered wires.
    assignWireNumbers(doc);
    labelCount = 0;
    for (const Entity* e : doc.entities()) {
        if (e->type() == EntityType::NetLabel) ++labelCount;
    }
    REQUIRE(labelCount == 2);

    // A newly added, still-unlabeled wire gets numbered without colliding
    // with the existing W1/W2.
    doc.addEntity(std::make_unique<WireEntity>(doc.reserveEntityId(), doc.currentLayer(),
                                                std::vector<Point2D>{Point2D(0, 40), Point2D(10, 40)}));
    assignWireNumbers(doc);
    std::vector<std::string> names;
    for (const Entity* e : doc.entities()) {
        if (e->type() == EntityType::NetLabel) names.push_back(static_cast<const NetLabelEntity*>(e)->name());
    }
    REQUIRE(names.size() == 3);
    REQUIRE(std::find(names.begin(), names.end(), "W1") != names.end());
    REQUIRE(std::find(names.begin(), names.end(), "W2") != names.end());
    REQUIRE(std::find(names.begin(), names.end(), "W3") != names.end());
}

TEST_CASE("buildWireListTable reports each net's pins with resolved reference designators",
         "[electrical][wirelist]") {
    Document doc;
    registerElectricalSymbols(doc);
    const BlockDefinition* tb = doc.findBlock("TB");

    auto insertA = std::make_unique<InsertEntity>(doc.reserveEntityId(), doc.currentLayer(), tb, Point2D(0, 0));
    insertA->setAttribute("REFDES", "X1");
    doc.addEntity(std::move(insertA));
    auto insertB = std::make_unique<InsertEntity>(doc.reserveEntityId(), doc.currentLayer(), tb, Point2D(100, 0));
    insertB->setAttribute("REFDES", "X2");
    doc.addEntity(std::move(insertB));
    doc.addEntity(std::make_unique<WireEntity>(doc.reserveEntityId(), doc.currentLayer(),
                                                std::vector<Point2D>{Point2D(20, 0), Point2D(100, 0)}));

    const std::vector<Net> nets = computeNets(doc);
    TableEntity* table = buildWireListTable(doc, nets, Point2D(0, -50));
    REQUIRE(table != nullptr);
    REQUIRE(table->rows() > 1);
    REQUIRE(table->cellText(0, 0) == "Net");
    REQUIRE(table->cellText(0, 1) == "RefDes");

    bool foundX1 = false, foundX2 = false;
    for (int r = 1; r < table->rows(); ++r) {
        if (table->cellText(r, 1) == "X1") foundX1 = true;
        if (table->cellText(r, 1) == "X2") foundX2 = true;
    }
    REQUIRE(foundX1);
    REQUIRE(foundX2);
}
