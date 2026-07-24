#include "core/geometry/MassProps.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

using namespace lcad;
using Catch::Approx;

TEST_CASE("computePolygonMassProps computes exact area/perimeter/centroid for a CCW rectangle", "[massprops]") {
    const std::vector<Point2D> rect{Point2D(0, 0), Point2D(10, 0), Point2D(10, 4), Point2D(0, 4)};
    const auto props = computePolygonMassProps(rect);
    REQUIRE(props.signedArea == Approx(40.0));   // positive: CCW
    REQUIRE(props.perimeter == Approx(28.0));    // 2*(10+4)
    REQUIRE(props.centroid.x == Approx(5.0));
    REQUIRE(props.centroid.y == Approx(2.0));
}

TEST_CASE("computePolygonMassProps returns a negative signed area for a clockwise loop", "[massprops]") {
    const std::vector<Point2D> cw{Point2D(0, 0), Point2D(0, 4), Point2D(10, 4), Point2D(10, 0)};
    const auto props = computePolygonMassProps(cw);
    REQUIRE(props.signedArea == Approx(-40.0));
    REQUIRE(props.perimeter == Approx(28.0)); // always non-negative regardless of winding
}

TEST_CASE("computePolygonMassProps gets the TRUE area centroid of an L-shape, not the vertex average",
         "[massprops]") {
    // An L-shape: a 10x10 square with a 5x5 bite taken out of the top-right
    // corner. Vertex-averaging would badly mislocate the centroid; the real
    // area centroid must stay left/below the notch.
    const std::vector<Point2D> lshape{Point2D(0, 0),  Point2D(10, 0), Point2D(10, 5),
                                      Point2D(5, 5),  Point2D(5, 10), Point2D(0, 10)};
    const auto props = computePolygonMassProps(lshape);
    REQUIRE(props.signedArea == Approx(75.0)); // 100 - 25
    // A naive vertex average would land near (5, 5); the real centroid is
    // pulled toward the larger lower-left mass.
    REQUIRE(props.centroid.x < 5.0);
    REQUIRE(props.centroid.y < 5.0);
}

TEST_CASE("computePolygonMassProps returns a zero-initialized result for fewer than 3 vertices", "[massprops]") {
    const auto props = computePolygonMassProps({Point2D(0, 0), Point2D(1, 1)});
    REQUIRE(props.signedArea == Approx(0.0));
    REQUIRE(props.perimeter == Approx(0.0));
}

TEST_CASE("computeRegionMassProps subtracts a hole's area and pulls the centroid off-center", "[massprops]") {
    // A 10x10 CCW outer square with a 4x4 CW hole placed off-center (near
    // one corner) -- net area must exclude the hole, and the centroid must
    // shift AWAY from the hole (less material there).
    const std::vector<Point2D> outer{Point2D(0, 0), Point2D(10, 0), Point2D(10, 10), Point2D(0, 10)};
    const std::vector<Point2D> hole{Point2D(1, 1), Point2D(1, 5), Point2D(5, 5), Point2D(5, 1)}; // CW: negative area

    const auto props = computeRegionMassProps({outer, hole});
    REQUIRE(props.area == Approx(100.0 - 16.0));
    REQUIRE(props.perimeter == Approx(40.0 + 16.0)); // outer 40 + hole's own 16, summed regardless of sign
    // Center of mass shifts away from the hole (hole sits toward the
    // low-x/low-y corner), so both coordinates land above the plain 5,5
    // square-only centroid.
    REQUIRE(props.centroid.x > 5.0);
    REQUIRE(props.centroid.y > 5.0);

    REQUIRE(props.boundingBox.min.x == Approx(0.0));
    REQUIRE(props.boundingBox.max.x == Approx(10.0));
}

TEST_CASE("computeRegionMassProps with a single loop matches computePolygonMassProps directly", "[massprops]") {
    const std::vector<Point2D> tri{Point2D(0, 0), Point2D(6, 0), Point2D(0, 6)};
    const auto single = computePolygonMassProps(tri);
    const auto region = computeRegionMassProps({tri});
    REQUIRE(region.area == Approx(std::abs(single.signedArea)));
    REQUIRE(region.centroid.x == Approx(single.centroid.x));
    REQUIRE(region.centroid.y == Approx(single.centroid.y));
}
