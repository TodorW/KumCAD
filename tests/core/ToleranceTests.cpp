#include "core/document/Document.h"
#include "core/geometry/Tolerance.h"
#include "core/io/DxfReader.h"
#include "core/io/DxfWriter.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>

using namespace lcad;
using Catch::Approx;

namespace {
struct TempDxfPath {
    std::filesystem::path path =
        std::filesystem::temp_directory_path() / ("kumcad_tolerance_test_" + std::to_string(std::rand()) + ".dxf");
    ~TempDxfPath() { std::filesystem::remove(path); }
};
} // namespace

TEST_CASE("ToleranceEntity::rowText renders a real, readable feature-control-frame row", "[geometry][tolerance]") {
    ToleranceRow row;
    row.characteristic = GeoCharacteristic::Position;
    row.diameterSymbol = true;
    row.toleranceValue = 0.05;
    row.toleranceModifier = MaterialCondition::MMC;
    row.datums.push_back({"A", MaterialCondition::None});
    row.datums.push_back({"B", MaterialCondition::MMC});

    const std::string text = ToleranceEntity::rowText(row);
    REQUIRE(text.find("Position") != std::string::npos);
    REQUIRE(text.find("DIA") != std::string::npos);
    REQUIRE(text.find("0.05") != std::string::npos);
    REQUIRE(text.find("(M)") != std::string::npos);
    REQUIRE(text.find("| A") != std::string::npos);
    REQUIRE(text.find("| B") != std::string::npos);
}

TEST_CASE("ToleranceEntity::rowText omits modifier parens when None", "[geometry][tolerance]") {
    ToleranceRow row;
    row.characteristic = GeoCharacteristic::Flatness;
    row.toleranceValue = 0.1;
    const std::string text = ToleranceEntity::rowText(row);
    REQUIRE(text.find("Flatness") != std::string::npos);
    REQUIRE(text.find('(') == std::string::npos);
}

TEST_CASE("ToleranceEntity translate/scale/mirror move its insertion point and text height",
         "[geometry][tolerance]") {
    std::vector<ToleranceRow> rows{ToleranceRow{}};
    ToleranceEntity tol(1, 0, Point2D(0, 0), rows, 2.5);

    tol.translate(Point2D(5, 5));
    REQUIRE(tol.position().x == Approx(5.0));
    REQUIRE(tol.position().y == Approx(5.0));

    tol.scale(Point2D(5, 5), 2.0);
    REQUIRE(tol.textHeight() == Approx(5.0));

    tol.mirror(Point2D(0, 0), Point2D(0, 1)); // mirror across the Y axis
    REQUIRE(tol.position().x == Approx(-5.0));
}

TEST_CASE("DXF round-trips a ToleranceEntity's rows, datums, and modifiers", "[dxf][tolerance]") {
    TempDxfPath temp;
    Document doc;

    std::vector<ToleranceRow> rows;
    ToleranceRow row1;
    row1.characteristic = GeoCharacteristic::Perpendicularity;
    row1.diameterSymbol = false;
    row1.toleranceValue = 0.02;
    row1.toleranceModifier = MaterialCondition::LMC;
    row1.datums.push_back({"A", MaterialCondition::None});
    rows.push_back(row1);
    ToleranceRow row2;
    row2.characteristic = GeoCharacteristic::Position;
    row2.diameterSymbol = true;
    row2.toleranceValue = 0.1;
    row2.toleranceModifier = MaterialCondition::MMC;
    row2.datums.push_back({"A", MaterialCondition::None});
    row2.datums.push_back({"B", MaterialCondition::MMC});
    row2.datums.push_back({"C", MaterialCondition::None});
    rows.push_back(row2);

    doc.addEntity(std::make_unique<ToleranceEntity>(doc.reserveEntityId(), doc.currentLayer(), Point2D(12.0, 34.0),
                                                     rows, 3.0));

    REQUIRE(writeDxf(doc, temp.path.string()));
    Document loaded;
    REQUIRE(readDxf(loaded, temp.path.string()));

    int found = 0;
    for (const Entity* e : loaded.entities()) {
        if (e->type() != EntityType::Tolerance) continue;
        const auto& tol = static_cast<const ToleranceEntity&>(*e);
        REQUIRE(tol.position().x == Approx(12.0));
        REQUIRE(tol.position().y == Approx(34.0));
        REQUIRE(tol.textHeight() == Approx(3.0));
        REQUIRE(tol.rows().size() == 2);
        REQUIRE(tol.rows()[0].characteristic == GeoCharacteristic::Perpendicularity);
        REQUIRE(tol.rows()[0].toleranceValue == Approx(0.02));
        REQUIRE(tol.rows()[0].toleranceModifier == MaterialCondition::LMC);
        REQUIRE(tol.rows()[0].datums.size() == 1);
        REQUIRE(tol.rows()[0].datums[0].letter == "A");

        REQUIRE(tol.rows()[1].characteristic == GeoCharacteristic::Position);
        REQUIRE(tol.rows()[1].diameterSymbol);
        REQUIRE(tol.rows()[1].datums.size() == 3);
        REQUIRE(tol.rows()[1].datums[1].letter == "B");
        REQUIRE(tol.rows()[1].datums[1].modifier == MaterialCondition::MMC);
        REQUIRE(tol.rows()[1].datums[2].letter == "C");
        ++found;
    }
    REQUIRE(found == 1);
}
