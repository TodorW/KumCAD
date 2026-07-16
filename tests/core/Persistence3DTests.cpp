#include "core/core3d/Document3D.h"
#include "core/core3d/Persistence3D.h"
#include "core/sketch/SketchGeometry.h"

#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <filesystem>

using namespace lcad;
using Catch::Approx;

namespace {

struct TempPath {
    std::filesystem::path path =
        std::filesystem::temp_directory_path() / ("kumcad_kcad3d_test_" + std::to_string(std::rand()) + ".kcad3d");
    ~TempPath() { std::filesystem::remove(path); }
};

double volumeOf(const TopoDS_Shape& shape) {
    GProp_GProps props;
    BRepGProp::VolumeProperties(shape, props);
    return props.Mass();
}

Sketch makeRectangleSketch(double w, double h) {
    Sketch sketch;
    const int p0 = sketch.addPoint(Point2D(0, 0), true);
    const int p1 = sketch.addPoint(Point2D(w, 0));
    const int p2 = sketch.addPoint(Point2D(w, h));
    const int p3 = sketch.addPoint(Point2D(0, h));
    sketch.addLine(p0, p1);
    sketch.addLine(p1, p2);
    sketch.addLine(p2, p3);
    sketch.addLine(p3, p0);
    SketchConstraint horiz;
    horiz.type = SketchConstraintType::Horizontal;
    horiz.geomA = 0;
    sketch.addConstraint(horiz);
    return sketch;
}

} // namespace

TEST_CASE("Document3D save/load round-trips a plain primitive/boolean feature tree", "[core3d][persistence]") {
    TempPath temp;
    Document3D doc;
    Feature3D boxA;
    boxA.type = FeatureType::Box;
    boxA.name = "Base Block";
    boxA.p1 = boxA.p2 = boxA.p3 = 20.0;
    const int a = doc.addFeature(boxA);

    Feature3D cyl;
    cyl.type = FeatureType::Cylinder;
    cyl.p1 = 3.0;
    cyl.p2 = 20.0;
    cyl.posX = cyl.posY = 10.0;
    const int b = doc.addFeature(cyl);

    Feature3D cut;
    cut.type = FeatureType::Cut;
    cut.inputA = a;
    cut.inputB = b;
    doc.addFeature(cut);

    REQUIRE(saveDocument3D(doc, temp.path.string()));

    Document3D loaded;
    REQUIRE(loadDocument3D(loaded, temp.path.string()));
    REQUIRE(loaded.features().size() == 3);
    REQUIRE(loaded.findFeature(0)->name == "Base Block");
    REQUIRE(loaded.isValid(2));
    REQUIRE(volumeOf(loaded.shapeAt(2)) == Approx(volumeOf(doc.shapeAt(2))).margin(1e-6));
}

TEST_CASE("Document3D save/load round-trips a sketch and a Pad feature built from it", "[core3d][persistence]") {
    TempPath temp;
    Document3D doc;
    const int sketchIdx = doc.addSketch(makeRectangleSketch(8.0, 6.0));

    Feature3D pad;
    pad.type = FeatureType::Pad;
    pad.sketchIndex = sketchIdx;
    pad.p1 = 3.0;
    doc.addFeature(pad);

    REQUIRE(saveDocument3D(doc, temp.path.string()));

    Document3D loaded;
    REQUIRE(loadDocument3D(loaded, temp.path.string()));
    REQUIRE(loaded.sketches().size() == 1);
    REQUIRE(loaded.sketches()[0].lines().size() == 4);
    REQUIRE(loaded.sketches()[0].constraints().size() == 1);
    REQUIRE(loaded.isValid(0));
    REQUIRE(volumeOf(loaded.shapeAt(0)) == Approx(8.0 * 6.0 * 3.0).margin(1e-6));
}

TEST_CASE("Document3D save/load round-trips an Imported feature's embedded BRep geometry", "[core3d][persistence]") {
    TempPath temp;
    Document3D doc;
    Feature3D box;
    box.type = FeatureType::Box;
    box.p1 = box.p2 = box.p3 = 12.0;
    doc.addFeature(box);
    const int imported = doc.addImportedShape(doc.shapeAt(0));

    Feature3D importedFeature;
    importedFeature.type = FeatureType::Imported;
    importedFeature.importIndex = imported;
    doc.addFeature(importedFeature);

    REQUIRE(saveDocument3D(doc, temp.path.string()));

    Document3D loaded;
    REQUIRE(loadDocument3D(loaded, temp.path.string()));
    REQUIRE(loaded.importedShapes().size() == 1);
    REQUIRE(loaded.isValid(1));
    REQUIRE(volumeOf(loaded.shapeAt(1)) == Approx(12.0 * 12.0 * 12.0).margin(1e-6));
}

TEST_CASE("loadDocument3D fails cleanly on a missing or malformed file", "[core3d][persistence]") {
    Document3D doc;
    REQUIRE_FALSE(loadDocument3D(doc, "/nonexistent/path/kumcad_never_exists.kcad3d"));
}
