#include "core/geometry/Region.h"

#include <algorithm>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

using namespace lcad;
using Catch::Approx;

namespace {
double totalArea(const std::vector<RegionLoop>& loops) {
    double total = 0.0;
    for (const RegionLoop& loop : loops) total += std::abs(loop.signedArea());
    return total;
}
std::vector<Point2D> square(double x0, double y0, double side) {
    return {{x0, y0}, {x0 + side, y0}, {x0 + side, y0 + side}, {x0, y0 + side}};
}
} // namespace

// Two unit-2 squares overlapping diagonally: A=[0,2]x[0,2], B=[1,3]x[1,3].
// Crossings at (2,1) and (1,2), both strictly interior to their edges (not
// at a vertex) -- the concrete case this clipper's direction rules were
// hand-derived and verified against (see Region.cpp's own comments).
TEST_CASE("regionBoolean unions two diagonally-overlapping squares into their envelope", "[geometry][region]") {
    auto a = square(0, 0, 2);
    auto b = square(1, 1, 2);
    auto result = regionBoolean(a, b, RegionBoolOp::Union);
    REQUIRE(result.size() == 1);
    REQUIRE(totalArea(result) == Approx(7.0)); // 2*2 + 2*2 - 1 (overlap)
    REQUIRE(result.front().signedArea() > 0.0); // CCW outer loop
}

TEST_CASE("regionBoolean intersects two diagonally-overlapping squares into the shared lens",
         "[geometry][region]") {
    auto a = square(0, 0, 2);
    auto b = square(1, 1, 2);
    auto result = regionBoolean(a, b, RegionBoolOp::Intersect);
    REQUIRE(result.size() == 1);
    REQUIRE(totalArea(result) == Approx(1.0)); // the [1,2]x[1,2] overlap
    REQUIRE(result.front().signedArea() > 0.0);
}

TEST_CASE("regionBoolean subtracts the overlap, leaving an L-shape", "[geometry][region]") {
    auto a = square(0, 0, 2);
    auto b = square(1, 1, 2);
    auto result = regionBoolean(a, b, RegionBoolOp::Subtract);
    REQUIRE(result.size() == 1);
    REQUIRE(totalArea(result) == Approx(3.0)); // 4 - 1 overlap
    REQUIRE(result.front().signedArea() > 0.0);

    // The other direction (B - A) should also be a 3-area L-shape, mirrored.
    auto resultBA = regionBoolean(b, a, RegionBoolOp::Subtract);
    REQUIRE(resultBA.size() == 1);
    REQUIRE(totalArea(resultBA) == Approx(3.0));
}

TEST_CASE("regionBoolean full containment: Subtract produces an outer loop plus a hole",
         "[geometry][region]") {
    auto outer = square(0, 0, 10);
    auto inner = square(2, 2, 3); // fully inside outer, no edge touches
    auto result = regionBoolean(outer, inner, RegionBoolOp::Subtract);
    REQUIRE(result.size() == 2);
    // One outer (CCW, positive area) and one hole (CW, negative area).
    const bool hasOuter = result[0].signedArea() > 0.0 || result[1].signedArea() > 0.0;
    const bool hasHole = result[0].signedArea() < 0.0 || result[1].signedArea() < 0.0;
    REQUIRE(hasOuter);
    REQUIRE(hasHole);
    REQUIRE(totalArea(result) == Approx(100.0 + 9.0)); // outer area + hole area counted separately
}

TEST_CASE("regionBoolean full containment: Union keeps just the outer, Intersect keeps just the inner",
         "[geometry][region]") {
    auto outer = square(0, 0, 10);
    auto inner = square(2, 2, 3);

    auto unionResult = regionBoolean(outer, inner, RegionBoolOp::Union);
    REQUIRE(unionResult.size() == 1);
    REQUIRE(totalArea(unionResult) == Approx(100.0));

    auto intersectResult = regionBoolean(outer, inner, RegionBoolOp::Intersect);
    REQUIRE(intersectResult.size() == 1);
    REQUIRE(totalArea(intersectResult) == Approx(9.0));
}

TEST_CASE("regionBoolean disjoint loops: Union keeps both, Intersect is empty, Subtract is unchanged",
         "[geometry][region]") {
    auto a = square(0, 0, 2);
    auto b = square(100, 100, 2);

    auto unionResult = regionBoolean(a, b, RegionBoolOp::Union);
    REQUIRE(unionResult.size() == 2);
    REQUIRE(totalArea(unionResult) == Approx(8.0));

    auto intersectResult = regionBoolean(a, b, RegionBoolOp::Intersect);
    REQUIRE(intersectResult.empty());

    auto subtractResult = regionBoolean(a, b, RegionBoolOp::Subtract);
    REQUIRE(subtractResult.size() == 1);
    REQUIRE(totalArea(subtractResult) == Approx(4.0));
}

TEST_CASE("RegionEntity::booleanOp composes two single-loop regions and refuses regions with holes",
         "[geometry][region]") {
    RegionEntity a(1, 0, {RegionLoop{square(0, 0, 2)}});
    RegionEntity b(2, 0, {RegionLoop{square(1, 1, 2)}});
    auto unioned = RegionEntity::booleanOp(3, 0, a, b, RegionBoolOp::Union);
    REQUIRE(unioned != nullptr);
    REQUIRE(unioned->area() == Approx(7.0));
    REQUIRE(unioned->type() == EntityType::Region);

    // A region with a hole (2 loops) is out of scope for booleanOp.
    RegionLoop outer{square(0, 0, 10)};
    RegionLoop hole{square(2, 2, 3)};
    std::vector<Point2D> holeVerts = hole.vertices;
    std::reverse(holeVerts.begin(), holeVerts.end());
    RegionEntity withHole(4, 0, {outer, RegionLoop{holeVerts}});
    REQUIRE(RegionEntity::booleanOp(5, 0, withHole, b, RegionBoolOp::Union) == nullptr);
}

TEST_CASE("RegionEntity reports area, containment, and bounding box for a region with a hole",
         "[geometry][region]") {
    RegionLoop outer{square(0, 0, 10)};
    std::vector<Point2D> holeVerts = square(2, 2, 3);
    std::reverse(holeVerts.begin(), holeVerts.end()); // hole winds CW
    RegionEntity region(1, 0, {outer, RegionLoop{holeVerts}});

    REQUIRE(region.area() == Approx(100.0 + 9.0));
    REQUIRE(region.containsPoint(Point2D(0.5, 0.5))); // in outer, outside hole
    REQUIRE_FALSE(region.containsPoint(Point2D(3.5, 3.5))); // inside the hole
    REQUIRE(region.containsPoint(Point2D(9.5, 9.5)));

    BoundingBox box = region.boundingBox();
    REQUIRE(box.min.x == Approx(0.0));
    REQUIRE(box.max.x == Approx(10.0));
}

TEST_CASE("RegionEntity translate/rotate/scale/clone preserve area", "[geometry][region]") {
    RegionEntity region(1, 0, {RegionLoop{square(0, 0, 4)}});
    REQUIRE(region.area() == Approx(16.0));

    region.translate(Point2D(5, -3));
    REQUIRE(region.area() == Approx(16.0));
    region.rotate(Point2D(0, 0), M_PI / 4.0);
    REQUIRE(region.area() == Approx(16.0));
    region.scale(Point2D(0, 0), 2.0);
    REQUIRE(region.area() == Approx(64.0));

    auto clone = region.clone();
    REQUIRE(clone->type() == EntityType::Region);
    REQUIRE(static_cast<RegionEntity*>(clone.get())->area() == Approx(64.0));
}
