#include "core/geometry/Arc.h"
#include "core/geometry/Circle.h"
#include "core/geometry/Line.h"
#include "core/geometry/Polyline.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;

TEST_CASE("LineEntity distance and bounding box", "[geometry]") {
    lcad::LineEntity line(1, 0, lcad::Point2D(0, 0), lcad::Point2D(10, 0));

    REQUIRE(line.distanceTo(lcad::Point2D(5, 3)) == Approx(3.0));
    REQUIRE(line.distanceTo(lcad::Point2D(-5, 0)) == Approx(5.0)); // beyond the start endpoint
    REQUIRE(line.distanceTo(lcad::Point2D(15, 0)) == Approx(5.0)); // beyond the end endpoint

    const auto box = line.boundingBox();
    REQUIRE(box.min.x == Approx(0.0));
    REQUIRE(box.max.x == Approx(10.0));
}

TEST_CASE("CircleEntity distance and bounding box", "[geometry]") {
    lcad::CircleEntity circle(1, 0, lcad::Point2D(0, 0), 5.0);

    REQUIRE(circle.distanceTo(lcad::Point2D(5, 0)) == Approx(0.0));
    REQUIRE(circle.distanceTo(lcad::Point2D(0, 0)) == Approx(5.0));
    REQUIRE(circle.distanceTo(lcad::Point2D(10, 0)) == Approx(5.0));

    const auto box = circle.boundingBox();
    REQUIRE(box.min.x == Approx(-5.0));
    REQUIRE(box.max.x == Approx(5.0));
    REQUIRE(box.min.y == Approx(-5.0));
    REQUIRE(box.max.y == Approx(5.0));
}

TEST_CASE("ArcEntity sweep containment", "[geometry]") {
    // Quarter arc from 0 to 90 degrees.
    lcad::ArcEntity arc(1, 0, lcad::Point2D(0, 0), 5.0, 0.0, M_PI / 2);

    REQUIRE(arc.distanceTo(lcad::Point2D(5, 0)) == Approx(0.0));   // on the arc, at the start
    REQUIRE(arc.distanceTo(lcad::Point2D(0, 5)) == Approx(0.0));   // on the arc, at the end
    REQUIRE(arc.distanceTo(lcad::Point2D(-5, 0)) > 0.1);           // outside the sweep

    const auto box = arc.boundingBox();
    REQUIRE(box.min.x == Approx(0.0));
    REQUIRE(box.max.x == Approx(5.0));
    REQUIRE(box.min.y == Approx(0.0));
    REQUIRE(box.max.y == Approx(5.0));
}

TEST_CASE("PolylineEntity distance across segments", "[geometry]") {
    std::vector<lcad::Point2D> verts{{0, 0}, {10, 0}, {10, 10}};
    lcad::PolylineEntity pl(1, 0, verts, false);

    REQUIRE(pl.distanceTo(lcad::Point2D(5, 1)) == Approx(1.0));
    REQUIRE(pl.distanceTo(lcad::Point2D(11, 5)) == Approx(1.0));
    REQUIRE(pl.distanceTo(lcad::Point2D(0, 10)) > 5.0); // not near the open end
}
