#include "core/document/Document.h"
#include "core/io/KiCadMod.h"
#include "core/io/SExpr.h"
#include "core/pcb/FootprintGenerator.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstdio>
#include <fstream>
#include <sstream>

using namespace lcad;
using Catch::Approx;

TEST_CASE("parseSExpr/writeSExpr round-trip a nested KiCad-style expression", "[io][kicad][sexpr]") {
    const std::string text = R"((footprint "R_0603" (layer "F.Cu") (at 10 20 90) (attr smd)))";
    auto parsed = parseSExpr(text);
    REQUIRE(parsed.has_value());
    REQUIRE(parsed->isList());
    REQUIRE(parsed->tag() == "footprint");
    REQUIRE(parsed->textAt(0) == "R_0603");

    const SExpr* layer = parsed->child("layer");
    REQUIRE(layer != nullptr);
    REQUIRE(layer->textAt(0) == "F.Cu");

    const SExpr* at = parsed->child("at");
    REQUIRE(at != nullptr);
    REQUIRE(at->numberAt(0) == Approx(10));
    REQUIRE(at->numberAt(1) == Approx(20));
    REQUIRE(at->numberAt(2) == Approx(90));

    const SExpr* attr = parsed->child("attr");
    REQUIRE(attr != nullptr);
    REQUIRE(attr->textAt(0) == "smd");

    // Re-parsing our own pretty-printed output must reproduce an
    // equivalent tree (the actual round-trip contract file formats rely on).
    std::string rewritten = writeSExpr(*parsed);
    auto reparsed = parseSExpr(rewritten);
    REQUIRE(reparsed.has_value());
    REQUIRE(reparsed->tag() == "footprint");
    REQUIRE(reparsed->child("at")->numberAt(2) == Approx(90));
}

TEST_CASE("parseSExpr rejects malformed input", "[io][kicad][sexpr]") {
    REQUIRE_FALSE(parseSExpr("(unbalanced (paren)").has_value());
    REQUIRE_FALSE(parseSExpr("(\"unterminated string)").has_value());
    REQUIRE_FALSE(parseSExpr("(a) trailing garbage").has_value());
}

TEST_CASE("formatKiCadNumber trims trailing zeros like KiCad's own writer", "[io][kicad][sexpr]") {
    REQUIRE(formatKiCadNumber(1.0) == "1");
    REQUIRE(formatKiCadNumber(1.5) == "1.5");
    REQUIRE(formatKiCadNumber(-0.75) == "-0.75");
}

TEST_CASE("writeKiCadMod/readKiCadMod round-trip a real footprint's pads and outline", "[io][kicad][footprint]") {
    Document doc;
    GullWingParams params;
    params.pinCount = 8;
    params.sideCount = 4;
    params.pitch = 1.0;
    params.bodyWidth = 4.0;
    params.bodyLength = 4.0;
    params.padWidth = 0.4;
    params.padLength = 1.0;
    REQUIRE(generateGullWingFootprint(doc, "QFP8", params));
    const BlockDefinition* block = doc.findBlock("QFP8");
    REQUIRE(block);

    const std::string path = "/tmp/kumcad_kicadmod_roundtrip_test.kicad_mod";
    std::string err;
    REQUIRE(writeKiCadMod(doc, *block, path, &err));

    Document doc2;
    const BlockDefinition* readBack = readKiCadMod(doc2, path, &err);
    REQUIRE(readBack != nullptr);
    REQUIRE(readBack->pads.size() == block->pads.size());
    for (std::size_t i = 0; i < block->pads.size(); ++i) {
        REQUIRE(readBack->pads[i].number == block->pads[i].number);
        REQUIRE(readBack->pads[i].position.x == Approx(block->pads[i].position.x));
        REQUIRE(readBack->pads[i].position.y == Approx(block->pads[i].position.y));
    }
    std::remove(path.c_str());
}

TEST_CASE("writeKiCadMod/readKiCadMod round-trip RoundRect and Trapezoid pad shape + shapeParam",
         "[io][kicad][footprint][padshape]") {
    Document doc;
    doc.addBlock("RRTRAP", {});
    BlockDefinition* block = doc.findBlock("RRTRAP");
    Pad roundRect;
    roundRect.number = "1";
    roundRect.shape = PadShape::RoundRect;
    roundRect.position = Point2D(0, 0);
    roundRect.width = 2.0;
    roundRect.height = 1.0;
    roundRect.shapeParam = 0.25;
    block->pads.push_back(roundRect);
    Pad trapezoid;
    trapezoid.number = "2";
    trapezoid.shape = PadShape::Trapezoid;
    trapezoid.position = Point2D(5, 0);
    trapezoid.width = 2.0;
    trapezoid.height = 1.0;
    trapezoid.shapeParam = 0.3;
    block->pads.push_back(trapezoid);

    const std::string path = "/tmp/kumcad_kicadmod_padshape_roundtrip_test.kicad_mod";
    std::string err;
    REQUIRE(writeKiCadMod(doc, *block, path, &err));

    // Real KiCad tokens, not this codebase's own invention -- confirms the
    // shape and its one extra parameter are written the way real KiCad
    // (and thus real KiCad importing this file) actually expects them.
    std::ifstream in(path);
    std::ostringstream contents;
    contents << in.rdbuf();
    REQUIRE(contents.str().find("roundrect") != std::string::npos);
    REQUIRE(contents.str().find("roundrect_rratio") != std::string::npos);
    REQUIRE(contents.str().find("trapezoid") != std::string::npos);
    REQUIRE(contents.str().find("rect_delta") != std::string::npos);

    Document doc2;
    const BlockDefinition* readBack = readKiCadMod(doc2, path, &err);
    REQUIRE(readBack != nullptr);
    REQUIRE(readBack->pads.size() == 2);
    REQUIRE(readBack->pads[0].shape == PadShape::RoundRect);
    REQUIRE(readBack->pads[0].shapeParam == Approx(0.25));
    REQUIRE(readBack->pads[1].shape == PadShape::Trapezoid);
    REQUIRE(readBack->pads[1].shapeParam == Approx(0.3));
    std::remove(path.c_str());
}

TEST_CASE("writeKiCadMod/readKiCadMod round-trip a Custom pad's real gr_poly outline", "[io][kicad][footprint][padshape]") {
    Document doc;
    doc.addBlock("CUSTFP", {});
    BlockDefinition* block = doc.findBlock("CUSTFP");
    Pad pad;
    pad.number = "1";
    pad.shape = PadShape::Custom;
    pad.position = Point2D(0, 0);
    pad.customOutline = {Point2D(-1, -0.5), Point2D(1, -0.5), Point2D(1, 0.5), Point2D(-1, 0.5)};
    block->pads.push_back(pad);

    const std::string path = "/tmp/kumcad_kicadmod_custom_pad_roundtrip_test.kicad_mod";
    std::string err;
    REQUIRE(writeKiCadMod(doc, *block, path, &err));

    std::ifstream in(path);
    std::ostringstream contents;
    contents << in.rdbuf();
    REQUIRE(contents.str().find("custom") != std::string::npos);
    REQUIRE(contents.str().find("gr_poly") != std::string::npos);
    REQUIRE(contents.str().find("primitives") != std::string::npos);

    Document doc2;
    const BlockDefinition* readBack = readKiCadMod(doc2, path, &err);
    REQUIRE(readBack != nullptr);
    REQUIRE(readBack->pads.size() == 1);
    REQUIRE(readBack->pads[0].shape == PadShape::Custom);
    REQUIRE(readBack->pads[0].customOutline.size() == 4);
    REQUIRE(readBack->pads[0].customOutline[0].x == Approx(-1.0));
    REQUIRE(readBack->pads[0].customOutline[0].y == Approx(-0.5));
    REQUIRE(readBack->pads[0].customOutline[2].x == Approx(1.0));
    REQUIRE(readBack->pads[0].customOutline[2].y == Approx(0.5));
    std::remove(path.c_str());
}

namespace {
double polygonAreaExact(const std::vector<Point2D>& pts) {
    double sum = 0.0;
    for (std::size_t i = 0; i < pts.size(); ++i) {
        const Point2D& a = pts[i];
        const Point2D& b = pts[(i + 1) % pts.size()];
        sum += a.x * b.y - b.x * a.y;
    }
    return std::abs(sum) / 2.0;
}
} // namespace

TEST_CASE("readKiCadMod unions two genuinely overlapping gr_rect primitives into one connected outline",
         "[io][kicad][footprint][padshape]") {
    const std::string path = "/tmp/kumcad_kicadmod_custom_pad_rects_test.kicad_mod";
    {
        std::ofstream out(path);
        // rect1 x=[-2,1] y=[-1,1] (area 3*2=6), rect2 x=[-1,2] y=[-2,2]
        // (area 3*4=12), overlapping on x=[-1,1] y=[-1,1] (area 2*2=4) --
        // union = 6+12-4 = 14 by inclusion-exclusion. Deliberately offset
        // in BOTH axes so no edge/vertex of either rect exactly coincides
        // with the other's (unlike two rects merely touching along a
        // shared edge, or sharing an identical y-range, both real
        // degenerate cases for a general polygon clipper's crossing-point
        // detection -- this is a clean transversal overlap instead).
        out << R"((footprint "TestFP"
  (pad "1" smd custom (at 0 0) (size 0.1 0.1) (layers "F.Cu")
    (options (clearance outline) (anchor rect))
    (primitives
      (gr_rect (start -2 -1) (end 1 1) (width 0))
      (gr_rect (start -1 -2) (end 2 2) (width 0))
    )
  )
))";
    }

    Document doc;
    std::string err;
    const BlockDefinition* block = readKiCadMod(doc, path, &err);
    REQUIRE(block != nullptr);
    REQUIRE(block->pads.size() == 1);
    REQUIRE(block->pads[0].shape == PadShape::Custom);
    REQUIRE(polygonAreaExact(block->pads[0].customOutline) == Approx(14.0).margin(1e-6));
    std::remove(path.c_str());
}

TEST_CASE("readKiCadMod reads a gr_circle primitive as a real tessellated circle outline",
         "[io][kicad][footprint][padshape]") {
    const std::string path = "/tmp/kumcad_kicadmod_custom_pad_circle_test.kicad_mod";
    {
        std::ofstream out(path);
        out << R"((footprint "TestFP"
  (pad "1" smd custom (at 0 0) (size 0.1 0.1) (layers "F.Cu")
    (options (clearance outline) (anchor rect))
    (primitives
      (gr_circle (center 0 0) (end 2 0) (width 0))
    )
  )
))";
    }

    Document doc;
    std::string err;
    const BlockDefinition* block = readKiCadMod(doc, path, &err);
    REQUIRE(block != nullptr);
    REQUIRE(block->pads.size() == 1);
    // end is 2 units from center -- radius 2. Tessellated, so within a
    // small epsilon of the true circle area, not exact.
    REQUIRE(polygonAreaExact(block->pads[0].customOutline) == Approx(M_PI * 2.0 * 2.0).epsilon(0.01));
    std::remove(path.c_str());
}

TEST_CASE("readKiCadMod unions a gr_poly and a gr_rect primitive that genuinely overlap",
         "[io][kicad][footprint][padshape]") {
    const std::string path = "/tmp/kumcad_kicadmod_custom_pad_mixed_test.kicad_mod";
    {
        std::ofstream out(path);
        // gr_poly is a rect x=[-3,1],y=[-1,1] (area 4*2=8); gr_rect is a
        // rect x=[-1,3],y=[-2,2] (area 4*4=16), offset in BOTH axes from
        // gr_poly (same non-degenerate-overlap reasoning as the gr_rect/
        // gr_rect test above) -- overlap on x=[-1,1],y=[-1,1] (area
        // 2*2=4), union = 8+16-4 = 20 by inclusion-exclusion.
        out << R"((footprint "TestFP"
  (pad "1" smd custom (at 0 0) (size 0.1 0.1) (layers "F.Cu")
    (options (clearance outline) (anchor rect))
    (primitives
      (gr_poly (pts (xy -3 -1) (xy 1 -1) (xy 1 1) (xy -3 1)) (width 0))
      (gr_rect (start -1 -2) (end 3 2) (width 0))
    )
  )
))";
    }

    Document doc;
    std::string err;
    const BlockDefinition* block = readKiCadMod(doc, path, &err);
    REQUIRE(block != nullptr);
    REQUIRE(polygonAreaExact(block->pads[0].customOutline) == Approx(20.0).margin(1e-6));
    std::remove(path.c_str());
}

TEST_CASE("readKiCadMod reports an error for a missing or malformed file", "[io][kicad][footprint]") {
    Document doc;
    std::string err;
    REQUIRE(readKiCadMod(doc, "/tmp/kumcad_does_not_exist.kicad_mod", &err) == nullptr);
    REQUIRE_FALSE(err.empty());

    const std::string path = "/tmp/kumcad_kicadmod_bad_test.kicad_mod";
    {
        std::ofstream out(path);
        out << "(not_a_footprint 1 2 3)";
    }
    err.clear();
    REQUIRE(readKiCadMod(doc, path, &err) == nullptr);
    REQUIRE_FALSE(err.empty());
    std::remove(path.c_str());
}
