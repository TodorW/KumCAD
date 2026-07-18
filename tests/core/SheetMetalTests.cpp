#include "core/core3d/Pick3D.h"
#include "core/core3d/SheetMetal.h"
#include "core/document/Document.h"

#include <BRepGProp.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <GProp_GProps.hxx>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

using namespace lcad;
using Catch::Approx;

namespace {

double volumeOf(const TopoDS_Shape& shape) {
    GProp_GProps props;
    BRepGProp::VolumeProperties(shape, props);
    return props.Mass();
}

SheetMetalPart makeLBracket() {
    SheetMetalPart part;
    part.width = 20.0;
    part.thickness = 2.0;
    part.bendRadius = 1.0;
    part.kFactor = 0.44;
    part.flatLengths = {30.0, 25.0};
    part.bendAngles = {90.0};
    return part;
}

} // namespace

TEST_CASE("flatPatternLength matches the standard bend-allowance formula", "[core3d][sheetmetal]") {
    const SheetMetalPart part = makeLBracket();
    const double neutralRadius = part.bendRadius + part.kFactor * part.thickness;
    const double expected = 30.0 + 25.0 + (90.0 * M_PI / 180.0) * neutralRadius;
    REQUIRE(flatPatternLength(part) == Approx(expected).margin(1e-9));
}

TEST_CASE("buildSheetMetalSolid's volume equals flat-pattern area times width (material is conserved when bending)",
          "[core3d][sheetmetal]") {
    const SheetMetalPart part = makeLBracket();
    const TopoDS_Shape solid = buildSheetMetalSolid(part);
    REQUIRE_FALSE(solid.IsNull());

    const double expectedVolume = flatPatternLength(part) * part.thickness * part.width;
    REQUIRE(volumeOf(solid) == Approx(expectedVolume).margin(1e-6));
}

TEST_CASE("buildSheetMetalSolid handles a multi-bend (U-channel) strip", "[core3d][sheetmetal]") {
    SheetMetalPart part;
    part.width = 15.0;
    part.thickness = 1.5;
    part.bendRadius = 1.5;
    part.kFactor = 0.44;
    part.flatLengths = {20.0, 10.0, 20.0};
    part.bendAngles = {90.0, 90.0}; // a U-channel: down, across, up

    const TopoDS_Shape solid = buildSheetMetalSolid(part);
    REQUIRE_FALSE(solid.IsNull());

    const double expectedVolume = flatPatternLength(part) * part.thickness * part.width;
    REQUIRE(volumeOf(solid) == Approx(expectedVolume).margin(1e-6));
}

TEST_CASE("buildSheetMetalSolid handles a negative (right-turn) bend angle", "[core3d][sheetmetal]") {
    SheetMetalPart part = makeLBracket();
    part.bendAngles = {-90.0};

    const TopoDS_Shape solid = buildSheetMetalSolid(part);
    REQUIRE_FALSE(solid.IsNull());

    const double expectedVolume = flatPatternLength(part) * part.thickness * part.width;
    REQUIRE(volumeOf(solid) == Approx(expectedVolume).margin(1e-6));
}

TEST_CASE("buildSheetMetalSolid rejects geometrically invalid parts", "[core3d][sheetmetal]") {
    SheetMetalPart empty;
    REQUIRE(buildSheetMetalSolid(empty).IsNull());

    SheetMetalPart mismatched = makeLBracket();
    mismatched.bendAngles = {90.0, 45.0}; // 2 flats need exactly 1 bend, not 2
    REQUIRE(buildSheetMetalSolid(mismatched).IsNull());

    SheetMetalPart tooSharp = makeLBracket();
    tooSharp.bendAngles = {179.9};
    tooSharp.flatLengths = {30.0, 25.0};
    REQUIRE_FALSE(buildSheetMetalSolid(tooSharp).IsNull()); // just under the 180 limit is fine

    SheetMetalPart tooFar = makeLBracket();
    tooFar.bendAngles = {180.0};
    REQUIRE(buildSheetMetalSolid(tooFar).IsNull()); // at/over the limit is rejected

    SheetMetalPart negativeThickness = makeLBracket();
    negativeThickness.thickness = -1.0;
    REQUIRE(buildSheetMetalSolid(negativeThickness).IsNull());
}

TEST_CASE("insertFlatPatternIntoDocument bakes the rectangle and one bend line per bend", "[core3d][sheetmetal]") {
    const SheetMetalPart part = makeLBracket();
    Document doc2d;
    insertFlatPatternIntoDocument(doc2d, part, 0.0, 0.0);

    // 4 boundary edges + 1 bend line for this single-bend part.
    REQUIRE(doc2d.entities().size() == 5);

    bool foundLayer = false;
    for (const Layer& layer : doc2d.layers()) {
        if (layer.name == "FLATPATTERN") foundLayer = true;
    }
    REQUIRE(foundLayer);

    int centerlineCount = 0;
    for (const Entity* e : doc2d.entities()) {
        if (e->linetypeOverride().has_value() && *e->linetypeOverride() == LineType::Center) ++centerlineCount;
    }
    REQUIRE(centerlineCount == 1);
}

namespace {
double volumeOfShape(const TopoDS_Shape& shape) {
    GProp_GProps props;
    BRepGProp::VolumeProperties(shape, props);
    return props.Mass();
}
} // namespace

TEST_CASE("buildFaceFlange at 90 degrees adds an exactly-additive perpendicular wall", "[core3d][sheetmetal][flange]") {
    const TopoDS_Shape box = BRepPrimAPI_MakeBox(20.0, 10.0, 2.0).Shape();

    PickRay topRay;
    topRay.origin = {10.0, 5.0, 100.0};
    topRay.direction = {0.0, 0.0, -1.0};
    const auto topFace = pickFace(box, topRay);
    REQUIRE(topFace.has_value());

    PickRay edgeRay;
    edgeRay.origin = {10.0, -0.1, 2.0};
    edgeRay.direction = {0.0, 1.0, 0.0};
    const auto edge = pickEdge(box, edgeRay, 0.5);
    REQUIRE(edge.has_value());

    const TopoDS_Shape flanged = buildFaceFlange(box, edge->edgeIndex, topFace->faceIndex, 5.0, 90.0, 2.0);
    REQUIRE_FALSE(flanged.IsNull());

    // A 90-degree flange rising perpendicular from the picked edge is
    // geometrically disjoint from the original box (they only share a
    // zero-volume seam), so the fused volume is exactly additive: the
    // 20x10x2 box plus a 20 (edge length) x 5 (flange length) x 2
    // (thickness) wall.
    const double expected = 20.0 * 10.0 * 2.0 + 20.0 * 5.0 * 2.0;
    REQUIRE(volumeOfShape(flanged) == Approx(expected).margin(1e-3));
}

TEST_CASE("buildFaceFlange at 0 degrees (coplanar continuation) is also exactly additive",
          "[core3d][sheetmetal][flange]") {
    const TopoDS_Shape box = BRepPrimAPI_MakeBox(20.0, 10.0, 2.0).Shape();

    PickRay topRay;
    topRay.origin = {10.0, 5.0, 100.0};
    topRay.direction = {0.0, 0.0, -1.0};
    const auto topFace = pickFace(box, topRay);
    REQUIRE(topFace.has_value());

    PickRay edgeRay;
    edgeRay.origin = {10.0, -0.1, 2.0};
    edgeRay.direction = {0.0, 1.0, 0.0};
    const auto edge = pickEdge(box, edgeRay, 0.5);
    REQUIRE(edge.has_value());

    const TopoDS_Shape flanged = buildFaceFlange(box, edge->edgeIndex, topFace->faceIndex, 5.0, 0.0, 2.0);
    REQUIRE_FALSE(flanged.IsNull());

    const double expected = 20.0 * 10.0 * 2.0 + 20.0 * 5.0 * 2.0;
    REQUIRE(volumeOfShape(flanged) == Approx(expected).margin(1e-3));
}

TEST_CASE("buildFaceFlange rejects invalid input", "[core3d][sheetmetal][flange]") {
    const TopoDS_Shape box = BRepPrimAPI_MakeBox(20.0, 10.0, 2.0).Shape();
    REQUIRE(buildFaceFlange(TopoDS_Shape(), 0, 0, 5.0, 90.0, 2.0).IsNull());
    REQUIRE(buildFaceFlange(box, 999, 0, 5.0, 90.0, 2.0).IsNull());  // bad edge index
    REQUIRE(buildFaceFlange(box, 0, 999, 5.0, 90.0, 2.0).IsNull());  // bad face index
    REQUIRE(buildFaceFlange(box, 0, 0, 0.0, 90.0, 2.0).IsNull());    // non-positive length
    REQUIRE(buildFaceFlange(box, 0, 0, 5.0, 90.0, -1.0).IsNull());   // negative thickness
}

TEST_CASE("buildFaceFlange with thickness 0.0 auto-detects the wall thickness from the reference face",
         "[core3d][sheetmetal][flange]") {
    const TopoDS_Shape box = BRepPrimAPI_MakeBox(20.0, 10.0, 2.0).Shape();

    PickRay topRay;
    topRay.origin = {10.0, 5.0, 100.0};
    topRay.direction = {0.0, 0.0, -1.0};
    const auto topFace = pickFace(box, topRay);
    REQUIRE(topFace.has_value());

    PickRay edgeRay;
    edgeRay.origin = {10.0, -0.1, 2.0};
    edgeRay.direction = {0.0, 1.0, 0.0};
    const auto edge = pickEdge(box, edgeRay, 0.5);
    REQUIRE(edge.has_value());

    // A ray cast inward from the top face (z=2) hits the bottom face
    // (z=0) after exactly 2.0 units -- the box's own real thickness --
    // so thickness=0.0 should auto-detect 2.0 and build the identical
    // flange an explicit thickness=2.0 call would.
    const TopoDS_Shape autoDetected = buildFaceFlange(box, edge->edgeIndex, topFace->faceIndex, 5.0, 90.0, 0.0);
    REQUIRE_FALSE(autoDetected.IsNull());
    const TopoDS_Shape explicitThickness = buildFaceFlange(box, edge->edgeIndex, topFace->faceIndex, 5.0, 90.0, 2.0);
    REQUIRE_FALSE(explicitThickness.IsNull());
    // A small systematic overshoot is expected: the detection ray starts
    // 1e-4 units outside the reference face (see detectThicknessAlongNormal's
    // own comment) to avoid immediately re-hitting that same face, so the
    // measured distance is thickness + ~1e-4, not exactly thickness.
    REQUIRE(volumeOfShape(autoDetected) == Approx(volumeOfShape(explicitThickness)).margin(0.02));
}

TEST_CASE("buildFaceFlange with thickness 0.0 auto-detects whatever distance is actually there, even on "
         "a non-plate-shaped solid",
         "[core3d][sheetmetal][flange]") {
    // A cube (10x10x10) has no distinguished "thin" direction the way a
    // 20x10x2 plate does, but a ray straight through any face of a
    // convex solid like this one still hits exactly one opposite face,
    // so auto-detection still succeeds (at 10.0, the cube's own full
    // extent) -- confirming detection isn't secretly relying on some
    // "is this sheet-metal-shaped" heuristic, just the geometric ray
    // cast the header documents.
    const TopoDS_Shape cube = BRepPrimAPI_MakeBox(10.0, 10.0, 10.0).Shape();

    PickRay topRay;
    topRay.origin = {5.0, 5.0, 100.0};
    topRay.direction = {0.0, 0.0, -1.0};
    const auto topFace = pickFace(cube, topRay);
    REQUIRE(topFace.has_value());

    PickRay edgeRay;
    edgeRay.origin = {5.0, -0.1, 10.0};
    edgeRay.direction = {0.0, 1.0, 0.0};
    const auto edge = pickEdge(cube, edgeRay, 0.5);
    REQUIRE(edge.has_value());

    const TopoDS_Shape flanged = buildFaceFlange(cube, edge->edgeIndex, topFace->faceIndex, 5.0, 90.0, 0.0);
    REQUIRE_FALSE(flanged.IsNull());
    const double expected = 10.0 * 10.0 * 10.0 + 10.0 * 5.0 * 10.0; // cube plus a 10x5x10 wall
    REQUIRE(volumeOfShape(flanged) == Approx(expected).margin(0.01)); // small detection-epsilon overshoot, see above
}
