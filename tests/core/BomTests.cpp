#include "core/document/Document.h"
#include "core/geometry/Insert.h"
#include "core/geometry/Table.h"
#include "core/schematic/Bom.h"
#include "core/schematic/SymbolLibrary.h"

#include <algorithm>
#include <catch2/catch_test_macros.hpp>

using namespace lcad;

namespace {
InsertEntity* placeSymbol(Document& doc, const std::string& blockName, const std::string& refDes,
                         const std::string& value, Point2D at) {
    const BlockDefinition* block = doc.findBlock(blockName);
    auto insert = std::make_unique<InsertEntity>(doc.reserveEntityId(), doc.currentLayer(), block, at);
    insert->setAttribute("REFDES", refDes);
    if (!value.empty()) insert->setAttribute("VALUE", value);
    InsertEntity* raw = insert.get();
    doc.addEntity(std::move(insert));
    return raw;
}

const BomRow* findRow(const std::vector<BomRow>& rows, const std::string& part, const std::string& value) {
    const auto it = std::find_if(rows.begin(), rows.end(),
                                 [&](const BomRow& r) { return r.part == part && r.value == value; });
    return it != rows.end() ? &(*it) : nullptr;
}
} // namespace

TEST_CASE("generateBom groups instances sharing the same part and value into one row", "[schematic][bom]") {
    Document doc;
    registerBuiltinSymbols(doc);
    placeSymbol(doc, "R", "R1", "10k", Point2D(0, 0));
    placeSymbol(doc, "R", "R2", "10k", Point2D(50, 0));
    placeSymbol(doc, "R", "R3", "4.7k", Point2D(100, 0));

    const std::vector<BomRow> rows = generateBom(doc);

    const BomRow* tenK = findRow(rows, "R", "10k");
    REQUIRE(tenK);
    REQUIRE(tenK->quantity == 2);
    REQUIRE(tenK->refDes == std::vector<std::string>{"R1", "R2"});

    const BomRow* fourSevenK = findRow(rows, "R", "4.7k");
    REQUIRE(fourSevenK);
    REQUIRE(fourSevenK->quantity == 1);
    REQUIRE(fourSevenK->refDes == std::vector<std::string>{"R3"});
}

TEST_CASE("generateBom gives components with no VALUE attribute their own empty-value row",
         "[schematic][bom]") {
    Document doc;
    registerBuiltinSymbols(doc);
    placeSymbol(doc, "D", "D1", "", Point2D(0, 0));
    placeSymbol(doc, "D", "D2", "", Point2D(50, 0));

    const std::vector<BomRow> rows = generateBom(doc);
    const BomRow* diodes = findRow(rows, "D", "");
    REQUIRE(diodes);
    REQUIRE(diodes->quantity == 2);
}

TEST_CASE("generateBom skips a component with no REFDES attribute at all", "[schematic][bom]") {
    Document doc;
    registerBuiltinSymbols(doc);
    const BlockDefinition* block = doc.findBlock("R");
    doc.addEntity(std::make_unique<InsertEntity>(doc.reserveEntityId(), doc.currentLayer(), block, Point2D(0, 0)));
    placeSymbol(doc, "R", "R1", "10k", Point2D(50, 0));

    const std::vector<BomRow> rows = generateBom(doc);
    REQUIRE(rows.size() == 1);
    REQUIRE(rows[0].quantity == 1);
}

TEST_CASE("generateBom ignores PCB footprint INSERTs, only counting schematic symbols", "[schematic][bom]") {
    Document doc;
    registerBuiltinSymbols(doc);
    placeSymbol(doc, "R", "R1", "10k", Point2D(0, 0));

    const BlockDefinition* footprint = doc.findBlock("R_FP");
    auto fpInsert =
        std::make_unique<InsertEntity>(doc.reserveEntityId(), doc.currentLayer(), footprint, Point2D(100, 0));
    fpInsert->setAttribute("REFDES", "R1");
    doc.addEntity(std::move(fpInsert));

    const std::vector<BomRow> rows = generateBom(doc);
    REQUIRE(rows.size() == 1);
    REQUIRE(rows[0].part == "R");
}

TEST_CASE("buildBomTable produces a header plus one row per BomRow with the expected cell text",
         "[schematic][bom]") {
    std::vector<BomRow> rows = {{"R", "10k", {"R1", "R2"}, 2}, {"C", "100nF", {"C1"}, 1}};

    Document doc2d;
    TableEntity* table = buildBomTable(doc2d, rows, Point2D(0, 0));
    REQUIRE(table != nullptr);
    REQUIRE(table->rows() == 3); // header + 2 rows
    REQUIRE(table->cellText(0, 0) == "Part");
    REQUIRE(table->cellText(1, 0) == "R");
    REQUIRE(table->cellText(1, 1) == "10k");
    REQUIRE(table->cellText(1, 2) == "2");
    REQUIRE(table->cellText(1, 3) == "R1, R2");
    REQUIRE(table->cellText(2, 0) == "C");
}
