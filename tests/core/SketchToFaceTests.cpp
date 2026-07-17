#include "core/core3d/SketchToFace.h"
#include "core/sketch/SketchGeometry.h"

#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

using namespace lcad;
using Catch::Approx;

namespace {
Sketch makeRectangle(double w, double h) {
    Sketch sketch;
    const int p0 = sketch.addPoint(Point2D(0, 0), true);
    const int p1 = sketch.addPoint(Point2D(w, 0));
    const int p2 = sketch.addPoint(Point2D(w, h));
    const int p3 = sketch.addPoint(Point2D(0, h));
    sketch.addLine(p0, p1);
    sketch.addLine(p1, p2);
    sketch.addLine(p2, p3);
    sketch.addLine(p3, p0);
    return sketch;
}
} // namespace

TEST_CASE("sketchToFace builds a real face from a closed rectangle with the correct area",
         "[core3d][sketchtoface]") {
    const Sketch sketch = makeRectangle(20.0, 10.0);
    const auto face = sketchToFace(sketch);
    REQUIRE(face.has_value());

    GProp_GProps props;
    BRepGProp::SurfaceProperties(*face, props);
    REQUIRE(props.Mass() == Approx(200.0).margin(1e-6));
}

TEST_CASE("sketchToFace builds a face from a closed circle with the correct area", "[core3d][sketchtoface]") {
    Sketch sketch;
    const int center = sketch.addPoint(Point2D(0, 0), true);
    sketch.addCircle(center, 5.0);

    const auto face = sketchToFace(sketch);
    REQUIRE(face.has_value());

    GProp_GProps props;
    BRepGProp::SurfaceProperties(*face, props);
    REQUIRE(props.Mass() == Approx(M_PI * 25.0).margin(1e-3));
}

TEST_CASE("sketchToFace treats a smaller closed loop as a hole in a larger one", "[core3d][sketchtoface]") {
    Sketch sketch = makeRectangle(20.0, 20.0);
    const int center = sketch.addPoint(Point2D(10, 10), true);
    sketch.addCircle(center, 3.0);

    const auto face = sketchToFace(sketch);
    REQUIRE(face.has_value());

    GProp_GProps props;
    BRepGProp::SurfaceProperties(*face, props);
    const double expected = 20.0 * 20.0 - M_PI * 9.0;
    REQUIRE(props.Mass() == Approx(expected).margin(1e-3));
}

TEST_CASE("sketchToFace returns nullopt for an open (unclosed) profile", "[core3d][sketchtoface]") {
    Sketch sketch;
    const int p0 = sketch.addPoint(Point2D(0, 0), true);
    const int p1 = sketch.addPoint(Point2D(10, 0));
    const int p2 = sketch.addPoint(Point2D(10, 10));
    sketch.addLine(p0, p1);
    sketch.addLine(p1, p2); // dangling, never closes back to p0

    REQUIRE_FALSE(sketchToFace(sketch).has_value());
}

TEST_CASE("sketchToFace builds a face from a mixed line+arc profile (a stadium/capsule shape)",
          "[core3d][sketchtoface][arc]") {
    // Two straight sides of length L=20 at y=+-R (R=5), joined by two
    // outward-bulging semicircular arcs at each end -- classic "stadium"
    // shape. Area = L*(2R) [rectangle] + pi*R^2 [the two semicircles
    // together make one full circle].
    Sketch sketch;
    const double length = 20.0, radius = 5.0;

    const int p0 = sketch.addPoint(Point2D(0, radius), true);   // top-left
    const int p1 = sketch.addPoint(Point2D(length, radius));    // top-right
    const int p2 = sketch.addPoint(Point2D(length, -radius));   // bottom-right
    const int p3 = sketch.addPoint(Point2D(0, -radius));        // bottom-left
    const int rightCenter = sketch.addPoint(Point2D(length, 0), true);
    const int leftCenter = sketch.addPoint(Point2D(0, 0), true);

    sketch.addLine(p0, p1); // top
    sketch.addLine(p2, p3); // bottom
    // Both arcs traverse clockwise (ccw=false) from their stored start to
    // end so they bulge outward (through the rightmost/leftmost point of
    // their own circle) rather than inward across the capsule's interior.
    sketch.addArc(rightCenter, p1, p2, radius, false);
    sketch.addArc(leftCenter, p3, p0, radius, false);

    const auto face = sketchToFace(sketch);
    REQUIRE(face.has_value());

    GProp_GProps props;
    BRepGProp::SurfaceProperties(*face, props);
    const double expected = length * (2.0 * radius) + M_PI * radius * radius;
    REQUIRE(props.Mass() == Approx(expected).margin(1e-3));
}
