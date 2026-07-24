#include "core/geometry/MassProps.h"
#include "core/pcb/PadShapeGeometry.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

using namespace lcad;
using Catch::Approx;

TEST_CASE("roundRectPadOutline degenerates to the exact rect for a zero corner ratio", "[pcb][padshape]") {
    const auto outline = roundRectPadOutline(4.0, 2.0, 0.0);
    REQUIRE(outline.size() == 4);
    const double area = std::abs(computePolygonMassProps(outline).signedArea);
    REQUIRE(area == Approx(4.0 * 2.0)); // exact -- no arc approximation error to account for
}

TEST_CASE("roundRectPadOutline's rounded area sits strictly between the sharp-corner and ideal-circle bounds",
         "[pcb][padshape]") {
    const double w = 4.0, h = 2.0, ratio = 0.25; // r = ratio * min(w,h) = 0.5
    const auto outline = roundRectPadOutline(w, h, ratio, 16);
    REQUIRE(outline.size() == 4 * 16);
    const double area = std::abs(computePolygonMassProps(outline).signedArea);

    const double r = ratio * std::min(w, h);
    // Chords cut inside the true arc, so the real polygon's area is always
    // less than the ideal rounded rect (w*h with each corner's r*r square
    // replaced by a quarter-circle of area (pi/4)*r*r)...
    const double idealRoundedArea = w * h - r * r * (4.0 - M_PI);
    // ...and always more than lopping off the full r*r corner squares
    // (a sharp-cornered chamfer, strictly less material than any rounding).
    const double sharpChamferArea = w * h - 4.0 * r * r;
    REQUIRE(area > sharpChamferArea);
    REQUIRE(area < idealRoundedArea);
    // With 16 segments per corner the polygon should already be very close
    // to the ideal circle -- within 1% of the true rounded-rect area.
    REQUIRE(area == Approx(idealRoundedArea).epsilon(0.01));
}

TEST_CASE("roundRectPadOutline clamps an out-of-range corner ratio to 0.5", "[pcb][padshape]") {
    const auto atMax = roundRectPadOutline(4.0, 2.0, 0.5, 16);
    const auto beyondMax = roundRectPadOutline(4.0, 2.0, 5.0, 16);
    const double areaAtMax = std::abs(computePolygonMassProps(atMax).signedArea);
    const double areaBeyondMax = std::abs(computePolygonMassProps(beyondMax).signedArea);
    REQUIRE(areaAtMax == Approx(areaBeyondMax));
}

TEST_CASE("trapezoidPadOutline matches the exact trapezoid area formula", "[pcb][padshape]") {
    // height*(topWidth+bottomWidth)/2 -- the standard trapezoid area formula.
    const double width = 10.0, height = 4.0, delta = 2.0;
    const auto outline = trapezoidPadOutline(width, height, delta);
    REQUIRE(outline.size() == 4);
    const double area = std::abs(computePolygonMassProps(outline).signedArea);

    const double topWidth = width - delta;
    const double bottomWidth = width + delta;
    REQUIRE(area == Approx(height * (topWidth + bottomWidth) / 2.0));
    REQUIRE(area == Approx(width * height)); // a symmetric taper doesn't change the area at all
}

TEST_CASE("trapezoidPadOutline with zero delta is exactly a rectangle", "[pcb][padshape]") {
    const auto outline = trapezoidPadOutline(6.0, 3.0, 0.0);
    REQUIRE(outline[0].x == Approx(3.0));
    REQUIRE(outline[1].x == Approx(-3.0));
    REQUIRE(outline[2].x == Approx(-3.0));
    REQUIRE(outline[3].x == Approx(3.0));
    const double area = std::abs(computePolygonMassProps(outline).signedArea);
    REQUIRE(area == Approx(18.0));
}

TEST_CASE("trapezoidPadOutline clamps a delta wider than the pad itself instead of crossing over",
         "[pcb][padshape]") {
    // delta > width means the "narrow" edge would go negative -- clamped to
    // 0 (a point/triangle), never a self-intersecting bowtie.
    const auto outline = trapezoidPadOutline(4.0, 2.0, 10.0);
    REQUIRE(outline[0].x == Approx(0.0)); // top-right and top-left collapse to the same point
    REQUIRE(outline[1].x == Approx(0.0));
    REQUIRE(outline[2].x == Approx(-7.0)); // bottom half-width = (4+10)/2 = 7, unclamped
    REQUIRE(outline[3].x == Approx(7.0));
}
