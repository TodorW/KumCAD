#include "core/document/Document.h"
#include "core/document/PartLibrary.h"
#include "core/geometry/Insert.h"
#include "core/schematic/SymbolLibrary.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <filesystem>

using namespace lcad;

namespace {
struct TempLibPath {
    std::filesystem::path path =
        std::filesystem::temp_directory_path() / ("kumcad_lib_test_" + std::to_string(std::rand()) + ".dxf");
    ~TempLibPath() { std::filesystem::remove(path); }
};
} // namespace

TEST_CASE("writePartLibrary/readPartLibrary round-trips a symbol's pins and body", "[partlibrary]") {
    TempLibPath temp;

    Document source;
    registerBuiltinSymbols(source); // installs "R" (resistor) with 2 pins + zigzag body

    std::string writeError;
    REQUIRE(writePartLibrary(source, {"R"}, temp.path.string(), &writeError));
    REQUIRE(writeError.empty());

    Document target; // a fresh document with no "R" block at all
    std::vector<std::string> added;
    std::string readError;
    REQUIRE(readPartLibrary(target, temp.path.string(), false, &added, &readError));
    REQUIRE(readError.empty());
    REQUIRE(added == std::vector<std::string>{"R"});

    const BlockDefinition* loaded = target.findBlock("R");
    REQUIRE(loaded);
    REQUIRE(loaded->pins.size() == 2);
    REQUIRE(loaded->pins[0].number == "1");
    REQUIRE(loaded->pins[1].number == "2");
    REQUIRE_FALSE(loaded->entities.empty()); // the zigzag body geometry came through too
    REQUIRE(loaded->isSymbol());
    REQUIRE_FALSE(loaded->isFootprint());
}

TEST_CASE("writePartLibrary round-trips a footprint's pads", "[partlibrary]") {
    TempLibPath temp;

    Document source;
    registerBuiltinSymbols(source); // installs "R_FP" with 2 rect pads

    REQUIRE(writePartLibrary(source, {"R_FP"}, temp.path.string()));

    Document target;
    REQUIRE(readPartLibrary(target, temp.path.string()));
    const BlockDefinition* loaded = target.findBlock("R_FP");
    REQUIRE(loaded);
    REQUIRE(loaded->pads.size() == 2);
    REQUIRE(loaded->isFootprint());
}

TEST_CASE("writePartLibrary saves multiple blocks and skips unknown names", "[partlibrary]") {
    TempLibPath temp;

    Document source;
    registerBuiltinSymbols(source);

    REQUIRE(writePartLibrary(source, {"R", "C", "NoSuchBlock"}, temp.path.string()));

    Document target;
    std::vector<std::string> added;
    REQUIRE(readPartLibrary(target, temp.path.string(), false, &added));
    REQUIRE(added.size() == 2); // NoSuchBlock never made it into the library at all
    REQUIRE(target.findBlock("R") != nullptr);
    REQUIRE(target.findBlock("C") != nullptr);
}

TEST_CASE("writePartLibrary fails when nothing matches", "[partlibrary]") {
    TempLibPath temp;
    Document source;
    std::string error;
    REQUIRE_FALSE(writePartLibrary(source, {"DoesNotExist"}, temp.path.string(), &error));
    REQUIRE_FALSE(error.empty());
}

TEST_CASE("readPartLibrary skips an existing block by default, overwrites in place when asked", "[partlibrary]") {
    TempLibPath temp;

    Document source;
    registerBuiltinSymbols(source);
    REQUIRE(writePartLibrary(source, {"R"}, temp.path.string()));

    Document target;
    // A DIFFERENT "R" already present (one pin only), to detect whether it
    // survives (skip) or gets replaced (overwrite).
    std::vector<std::unique_ptr<Entity>> stub;
    target.addBlock("R", std::move(stub));
    if (BlockDefinition* r = target.findBlock("R")) {
        r->pins.push_back(Pin{"1", "1", PinElectricalType::Passive, Point2D(0, 0), Point2D(1, 0)});
    }

    std::vector<std::string> addedSkip;
    REQUIRE(readPartLibrary(target, temp.path.string(), false, &addedSkip));
    REQUIRE(addedSkip.empty()); // skipped: "R" already existed
    REQUIRE(target.findBlock("R")->pins.size() == 1); // untouched

    std::vector<std::string> addedOverwrite;
    REQUIRE(readPartLibrary(target, temp.path.string(), true, &addedOverwrite));
    REQUIRE(addedOverwrite == std::vector<std::string>{"R"});
    REQUIRE(target.findBlock("R")->pins.size() == 2); // now the library's real resistor
}

TEST_CASE("readPartLibrary overwrite keeps existing INSERTs pointing at the same definition, now updated",
          "[partlibrary]") {
    TempLibPath temp;

    Document source;
    registerBuiltinSymbols(source);
    REQUIRE(writePartLibrary(source, {"R"}, temp.path.string()));

    Document target;
    std::vector<std::unique_ptr<Entity>> stub;
    target.addBlock("R", std::move(stub)); // starts with 0 pins
    const BlockDefinition* blockBeforeReload = target.findBlock("R");

    auto insert = std::make_unique<InsertEntity>(target.reserveEntityId(), target.currentLayer(), blockBeforeReload,
                                                  Point2D(0, 0));
    const EntityId insertId = insert->id();
    target.addEntity(std::move(insert));

    REQUIRE(readPartLibrary(target, temp.path.string(), true));

    const auto* placedInsert = static_cast<const InsertEntity*>(target.findEntity(insertId));
    REQUIRE(placedInsert->block() == blockBeforeReload); // same object, mutated in place
    REQUIRE(placedInsert->block()->pins.size() == 2);     // now shows the library's real resistor
}

TEST_CASE("readPartLibrary fails cleanly on a missing file", "[partlibrary]") {
    Document doc;
    std::string error;
    REQUIRE_FALSE(readPartLibrary(doc, "/nonexistent/path/does_not_exist.dxf", false, nullptr, &error));
    REQUIRE_FALSE(error.empty());
}
