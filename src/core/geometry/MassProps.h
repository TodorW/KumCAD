#pragma once

#include "core/geometry/BoundingBox.h"
#include "core/geometry/Point2D.h"

#include <vector>

namespace lcad {

// Real, closed-form 2D mass properties of ONE closed polygon loop
// (implicitly closed, last vertex connects to first) -- the standard
// shoelace-formula area/centroid, the same math real AutoCAD's own
// MASSPROP reports. signedArea follows RegionLoop's own winding
// convention (positive CCW, negative CW); perimeter is always
// non-negative; centroid is the true AREA centroid (Green's theorem),
// not just the average of the vertices, which would be wrong for any
// non-regular polygon.
struct PolygonMassProps {
    double signedArea = 0.0;
    double perimeter = 0.0;
    Point2D centroid;
};
PolygonMassProps computePolygonMassProps(const std::vector<Point2D>& vertices);

// Combines several loops (an outer boundary plus zero or more holes,
// each already signed the way RegionLoop's own winding convention
// requires -- a hole's negative area subtracts its own contribution
// automatically) into ONE net area/perimeter/centroid/bounding box --
// AutoCAD's own MASSPROP report for a region that has holes. area here
// is the unsigned magnitude (matching RegionEntity::area()'s own
// convention); centroid is the area-weighted combination of every
// loop's own centroid, which correctly excludes the holes' interior.
struct RegionMassProps {
    double area = 0.0;
    double perimeter = 0.0;
    Point2D centroid;
    BoundingBox boundingBox;
};
RegionMassProps computeRegionMassProps(const std::vector<std::vector<Point2D>>& loops);

} // namespace lcad
