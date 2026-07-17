#include "core/document/Document.h"
#include "core/document/Fields.h"
#include "core/geometry/Insert.h"
#include "core/geometry/MText.h"
#include "core/geometry/Text.h"
#include "core/schematic/SymbolLibrary.h"

#include <catch2/catch_test_macros.hpp>

#include <regex>

using namespace lcad;

TEST_CASE("evaluateFieldTemplate resolves {{DATE}} to today's date in YYYY-MM-DD form", "[document][fields]") {
    Document doc;
    const std::string result = evaluateFieldTemplate(doc, "Drawn: {{DATE}}");
    static const std::regex pattern(R"(Drawn: \d{4}-\d{2}-\d{2})");
    REQUIRE(std::regex_match(result, pattern));
}

TEST_CASE("evaluateFieldTemplate resolves {{FILENAME}} to the caller-supplied file name", "[document][fields]") {
    Document doc;
    REQUIRE(evaluateFieldTemplate(doc, "File: {{FILENAME}}", "bracket.kcad") == "File: bracket.kcad");
    REQUIRE(evaluateFieldTemplate(doc, "File: {{FILENAME}}") == "File: "); // no file name supplied -- empty, not a crash
}

TEST_CASE("evaluateFieldTemplate resolves {{ATTR:refDes.tag}} to a placed INSERT's own attribute value",
         "[document][fields]") {
    Document doc;
    registerBuiltinSymbols(doc);
    const BlockDefinition* block = doc.findBlock("R");
    auto insert = std::make_unique<InsertEntity>(doc.reserveEntityId(), doc.currentLayer(), block, Point2D(0, 0));
    insert->setAttribute("REFDES", "R1");
    insert->setAttribute("VALUE", "10k");
    doc.addEntity(std::move(insert));

    REQUIRE(evaluateFieldTemplate(doc, "Value: {{ATTR:R1.VALUE}}") == "Value: 10k");
    REQUIRE(evaluateFieldTemplate(doc, "{{ATTR:R99.VALUE}}") == "?"); // no such REFDES
    REQUIRE(evaluateFieldTemplate(doc, "{{ATTR:R1.FOOTPRINT}}") == "?"); // REFDES exists, attribute doesn't
}

TEST_CASE("evaluateFieldTemplate leaves an unrecognized or malformed placeholder visible, not silently dropped",
         "[document][fields]") {
    Document doc;
    REQUIRE(evaluateFieldTemplate(doc, "{{BOGUS}}") == "{{BOGUS}}");
    REQUIRE(evaluateFieldTemplate(doc, "{{ATTR:NoDotHere}}") == "{{ATTR:NoDotHere}}");
    // Unterminated placeholder -- no closing }} at all.
    REQUIRE(evaluateFieldTemplate(doc, "prefix {{DATE") == "prefix {{DATE");
}

TEST_CASE("evaluateFieldTemplate resolves multiple placeholders in one string", "[document][fields]") {
    Document doc;
    registerBuiltinSymbols(doc);
    const BlockDefinition* block = doc.findBlock("R");
    auto insert = std::make_unique<InsertEntity>(doc.reserveEntityId(), doc.currentLayer(), block, Point2D(0, 0));
    insert->setAttribute("REFDES", "R1");
    insert->setAttribute("VALUE", "4.7k");
    doc.addEntity(std::move(insert));

    const std::string result = evaluateFieldTemplate(doc, "Part {{ATTR:R1.VALUE}} in {{FILENAME}}", "board.kcad");
    REQUIRE(result == "Part 4.7k in board.kcad");
}

TEST_CASE("TextEntity/MTextEntity carry an independent fieldTemplate alongside their own displayed text",
         "[document][fields]") {
    TextEntity text(1, 0, Point2D(0, 0), "10k", 2.5);
    REQUIRE(text.fieldTemplate().empty());
    text.setFieldTemplate("{{ATTR:R1.VALUE}}");
    REQUIRE(text.fieldTemplate() == "{{ATTR:R1.VALUE}}");
    REQUIRE(text.text() == "10k"); // the displayed value is untouched by setting the template alone

    MTextEntity mtext(2, 0, Point2D(0, 0), "10k", 2.5);
    mtext.setFieldTemplate("{{ATTR:R1.VALUE}}");
    REQUIRE(mtext.fieldTemplate() == "{{ATTR:R1.VALUE}}");
}
