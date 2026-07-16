#include "core/civil/Alignment.h"
#include "core/civil/Contours.h"
#include "core/civil/CutFill.h"
#include "core/civil/SurveyPoints.h"
#include "core/civil/Tin.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>

using namespace lcad;
using Catch::Approx;

namespace {
struct TempPath {
    std::filesystem::path path =
        std::filesystem::temp_directory_path() / ("kumcad_civil_test_" + std::to_string(std::rand()) + ".xyz");
    ~TempPath() { std::filesystem::remove(path); }
};
} // namespace

TEST_CASE("readSurveyPointsXyz keeps elevation, unlike the point-cloud readers", "[civil][survey]") {
    TempPath temp;
    std::ofstream out(temp.path);
    out << "# comment\n0 0 10\n10 0 20\n0 10 30\n10 10 40\n";
    out.close();

    const std::vector<SurveyPoint> points = readSurveyPointsXyz(temp.path.string());
    REQUIRE(points.size() == 4);
    REQUIRE(points[0].z == Approx(10.0));
    REQUIRE(points[3].z == Approx(40.0));
}

TEST_CASE("buildTin triangulates a simple square into two triangles", "[civil][tin]") {
    std::vector<SurveyPoint> points = {
        {Point2D(0, 0), 0.0}, {Point2D(10, 0), 10.0}, {Point2D(0, 10), 0.0}, {Point2D(10, 10), 10.0},
    };
    const Tin tin = buildTin(points);
    REQUIRE(tin.triangles.size() == 2);
}

TEST_CASE("elevationAt interpolates across a tilted plane and returns nullopt outside the hull",
         "[civil][tin]") {
    std::vector<SurveyPoint> points = {
        {Point2D(0, 0), 0.0}, {Point2D(10, 0), 10.0}, {Point2D(0, 10), 0.0}, {Point2D(10, 10), 10.0},
    };
    const Tin tin = buildTin(points);

    const auto center = elevationAt(tin, Point2D(5, 5));
    REQUIRE(center.has_value());
    REQUIRE(*center == Approx(5.0).margin(1e-6));

    REQUIRE_FALSE(elevationAt(tin, Point2D(50, 50)).has_value());
}

TEST_CASE("computeContours extracts parallel lines from a tilted plane", "[civil][contours]") {
    std::vector<SurveyPoint> points = {
        {Point2D(0, 0), 0.0}, {Point2D(10, 0), 10.0}, {Point2D(0, 10), 0.0}, {Point2D(10, 10), 10.0},
    };
    const Tin tin = buildTin(points);

    const auto contours = computeContours(tin, 2.0);
    // z ranges 0..10 over interval 2 -> levels at 2,4,6,8 (10 sits on a hull edge).
    REQUIRE(contours.size() >= 4);
    for (const auto& chain : contours) REQUIRE(chain.size() >= 2);
}

TEST_CASE("computeCutFillVolume is zero for two identical surfaces and positive fill for a raised one",
         "[civil][cutfill]") {
    std::vector<SurveyPoint> flat = {
        {Point2D(0, 0), 5.0}, {Point2D(10, 0), 5.0}, {Point2D(0, 10), 5.0}, {Point2D(10, 10), 5.0},
    };
    const Tin existing = buildTin(flat);
    const Tin same = buildTin(flat);

    const CutFillResult identical = computeCutFillVolume(existing, same, 1.0);
    REQUIRE(identical.cutVolume == Approx(0.0).margin(1e-6));
    REQUIRE(identical.fillVolume == Approx(0.0).margin(1e-6));

    std::vector<SurveyPoint> raised = {
        {Point2D(0, 0), 6.0}, {Point2D(10, 0), 6.0}, {Point2D(0, 10), 6.0}, {Point2D(10, 10), 6.0},
    };
    const Tin proposed = buildTin(raised);
    const CutFillResult raisedResult = computeCutFillVolume(existing, proposed, 1.0);
    // 10x10 area raised by 1 unit -> ~100 volume units of fill, no cut.
    REQUIRE(raisedResult.fillVolume == Approx(100.0).margin(5.0));
    REQUIRE(raisedResult.cutVolume == Approx(0.0).margin(1e-6));
}

TEST_CASE("computeGroundProfile samples elevation along an alignment at regular stations",
         "[civil][alignment]") {
    std::vector<SurveyPoint> points = {
        {Point2D(0, 0), 0.0}, {Point2D(20, 0), 20.0}, {Point2D(0, 10), 0.0}, {Point2D(20, 10), 20.0},
    };
    const Tin tin = buildTin(points);

    const std::vector<Point2D> alignment = {Point2D(0, 5), Point2D(20, 5)};
    const std::vector<Point2D> profile = computeGroundProfile(alignment, tin, 5.0);

    REQUIRE(profile.size() >= 4);
    // At station 10 (x=10 along the alignment), the plane's z == x == 10.
    bool foundMidpoint = false;
    for (const Point2D& p : profile) {
        if (std::abs(p.x - 10.0) < 1e-6) {
            REQUIRE(p.y == Approx(10.0).margin(1e-6));
            foundMidpoint = true;
        }
    }
    REQUIRE(foundMidpoint);
}
