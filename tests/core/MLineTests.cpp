#include "core/document/Document.h"
#include "core/geometry/MLine.h"
#include "core/io/DxfReader.h"
#include "core/io/DxfWriter.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <filesystem>

using namespace lcad;
using Catch::Approx;

namespace {
struct TempDxfPath {
    std::filesystem::path path =
        std::filesystem::temp_directory_path() / ("kumcad_mline_test_" + std::to_string(std::rand()) + ".dxf");
    ~TempDxfPath() { std::filesystem::remove(path); }
};
} // namespace

TEST_CASE("MLineEntity::elementLines offsets a straight centerline by each element's exact signed distance",
         "[geometry][mline]") {
    std::vector<Point2D> vertices{{0, 0}, {10, 0}}; // straight, along +X
    std::vector<MLineElement> elements{{0.5, Color{255, 0, 0}}, {-0.5, Color{0, 255, 0}}};
    MLineEntity mline(1, 0, vertices, elements);

    const auto lines = mline.elementLines();
    REQUIRE(lines.size() == 2);
    // Left of travel (+X direction) is +Y -- offset 0.5 lands at y=0.5.
    REQUIRE(lines[0][0].y == Approx(0.5));
    REQUIRE(lines[0][1].y == Approx(0.5));
    REQUIRE(lines[0][0].x == Approx(0.0));
    REQUIRE(lines[0][1].x == Approx(10.0));
    // Offset -0.5 lands at y=-0.5.
    REQUIRE(lines[1][0].y == Approx(-0.5));
    REQUIRE(lines[1][1].y == Approx(-0.5));
}

TEST_CASE("MLineEntity::elementLines applies a real angle-bisector miter at an interior corner",
         "[geometry][mline]") {
    // A right-angle corner: (0,0) -> (10,0) -> (10,10).
    std::vector<Point2D> vertices{{0, 0}, {10, 0}, {10, 10}};
    std::vector<MLineElement> elements{{1.0, Color{255, 255, 255}}};
    MLineEntity mline(1, 0, vertices, elements);

    const auto lines = mline.elementLines();
    REQUIRE(lines.size() == 1);
    const auto& line = lines[0];
    REQUIRE(line.size() == 3);
    // Straight-segment endpoints offset by exactly 1.0 perpendicular.
    REQUIRE(line[0].y == Approx(1.0));
    REQUIRE(line[2].x == Approx(9.0));
    // The miter at the corner: a 90-degree turn scales the offset by
    // sqrt(2) (1/cos(45deg)) along the bisector -- both the x and y
    // components of the corner's offset move by 1.0 each (1.0*sqrt(2) at
    // 45 degrees from either axis).
    REQUIRE(line[1].x == Approx(9.0));
    REQUIRE(line[1].y == Approx(1.0));
}

TEST_CASE("MLineEntity scale multiplies the effective offset distance", "[geometry][mline]") {
    std::vector<Point2D> vertices{{0, 0}, {10, 0}};
    std::vector<MLineElement> elements{{0.5, Color{255, 0, 0}}};
    MLineEntity mline(1, 0, vertices, elements, 2.0); // scale 2.0
    const auto lines = mline.elementLines();
    REQUIRE(lines[0][0].y == Approx(1.0)); // 0.5 * scale 2.0
}

TEST_CASE("MLineEntity mirror flips element offset sign to keep the same physical side", "[geometry][mline]") {
    std::vector<Point2D> vertices{{0, 0}, {10, 0}};
    std::vector<MLineElement> elements{{0.5, Color{255, 0, 0}}};
    MLineEntity mline(1, 0, vertices, elements);
    mline.mirror(Point2D(0, 0), Point2D(1, 0)); // mirror across the X axis
    REQUIRE(mline.elements()[0].offset == Approx(-0.5));
}

TEST_CASE("MLineEntity translate/rotate move the centerline and preserve offsets", "[geometry][mline]") {
    std::vector<Point2D> vertices{{0, 0}, {10, 0}};
    std::vector<MLineElement> elements{{0.5, Color{255, 0, 0}}};
    MLineEntity mline(1, 0, vertices, elements);
    mline.translate(Point2D(5, 5));
    REQUIRE(mline.vertices()[0].x == Approx(5.0));
    REQUIRE(mline.vertices()[0].y == Approx(5.0));
    REQUIRE(mline.elements()[0].offset == Approx(0.5));
}

TEST_CASE("DXF round-trips an MLineEntity's vertices, elements, caps, and flags", "[dxf][mline]") {
    TempDxfPath temp;
    Document doc;
    std::vector<Point2D> vertices{{0, 0}, {10, 0}, {10, 10}};
    std::vector<MLineElement> elements{{0.5, Color{255, 0, 0}}, {-0.5, Color{0, 0, 255}}};
    doc.addEntity(std::make_unique<MLineEntity>(doc.reserveEntityId(), doc.currentLayer(), vertices, elements, 1.5,
                                                true, MLineCapType::Line, MLineCapType::OuterArc, true));

    REQUIRE(writeDxf(doc, temp.path.string()));
    Document loaded;
    REQUIRE(readDxf(loaded, temp.path.string()));

    int found = 0;
    for (const Entity* e : loaded.entities()) {
        if (e->type() != EntityType::MLine) continue;
        const auto& mline = static_cast<const MLineEntity&>(*e);
        REQUIRE(mline.vertices().size() == 3);
        REQUIRE(mline.vertices()[2].y == Approx(10.0));
        REQUIRE(mline.elements().size() == 2);
        REQUIRE(mline.elements()[0].offset == Approx(0.5));
        REQUIRE(mline.elements()[0].color.r == 255);
        REQUIRE(mline.elements()[1].offset == Approx(-0.5));
        REQUIRE(mline.elements()[1].color.b == 255);
        REQUIRE(mline.scale() == Approx(1.5));
        REQUIRE(mline.closed());
        REQUIRE(mline.startCap() == MLineCapType::Line);
        REQUIRE(mline.endCap() == MLineCapType::OuterArc);
        REQUIRE(mline.showJoints());
        ++found;
    }
    REQUIRE(found == 1);
}
