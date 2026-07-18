#include "core/core3d/SheetMetal.h"

#include "core/geometry/Line.h"

#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepGProp.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <GProp_GProps.hxx>
#include <Geom_Circle.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <GeomLProp_SLProps.hxx>
#include <IntCurvesFace_Intersector.hxx>
#include <Standard_Failure.hxx>
#include <Standard_Real.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
#include <gp_Dir.hxx>
#include <gp_Lin.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

#include <cmath>

namespace lcad {

namespace {

bool partIsWellFormed(const SheetMetalPart& part) {
    if (part.flatLengths.empty()) return false;
    if (part.flatLengths.size() != part.bendAngles.size() + 1) return false;
    if (part.width <= 1e-9 || part.thickness <= 1e-9) return false;
    for (double length : part.flatLengths) {
        if (length <= 1e-9) return false;
    }
    const double neutralRadius = part.bendRadius + part.kFactor * part.thickness;
    for (double angleDeg : part.bendAngles) {
        if (std::abs(angleDeg) < 1e-9 || std::abs(angleDeg) >= 180.0) return false;
        if (neutralRadius - part.thickness / 2.0 <= 1e-6) return false;
    }
    return true;
}

// The result of walking the strip's neutral axis: two full boundary edge
// chains (offset +/- thickness/2 from the neutral axis) plus the four
// corner points needed to cap the two open ends into one closed profile.
struct SpineWalk {
    std::vector<TopoDS_Edge> sideAEdges;
    std::vector<TopoDS_Edge> sideBEdges;
    gp_Pnt sideAStart, sideBStart, sideAEnd, sideBEnd;
    double neutralLength = 0.0;
};

// Walks flatLengths/bendAngles as a turtle-graphics path, emitting both
// offset boundaries. See SheetMetal.h for why a bend is a real arc (radius
// bendRadius + kFactor*thickness) rather than a sharp corner -- that's what
// makes the 3D solid's path length equal flatPatternLength()'s formula
// exactly rather than approximately.
SpineWalk walkSpine(const SheetMetalPart& part) {
    SpineWalk result;
    const double halfT = part.thickness / 2.0;
    double x = 0.0, y = 0.0, theta = 0.0;

    for (std::size_t i = 0; i < part.flatLengths.size(); ++i) {
        const double length = part.flatLengths[i];
        const double dirX = std::cos(theta), dirY = std::sin(theta);
        const double leftX = -dirY, leftY = dirX;
        const double startX = x, startY = y;
        const double endX = x + dirX * length, endY = y + dirY * length;

        const gp_Pnt sideAStart(startX + leftX * halfT, startY + leftY * halfT, 0.0);
        const gp_Pnt sideBStart(startX - leftX * halfT, startY - leftY * halfT, 0.0);
        const gp_Pnt sideAEnd(endX + leftX * halfT, endY + leftY * halfT, 0.0);
        const gp_Pnt sideBEnd(endX - leftX * halfT, endY - leftY * halfT, 0.0);

        result.sideAEdges.push_back(BRepBuilderAPI_MakeEdge(sideAStart, sideAEnd));
        result.sideBEdges.push_back(BRepBuilderAPI_MakeEdge(sideBStart, sideBEnd));
        result.neutralLength += length;

        if (i == 0) {
            result.sideAStart = sideAStart;
            result.sideBStart = sideBStart;
        }
        x = endX;
        y = endY;

        if (i + 1 < part.flatLengths.size()) {
            const double angleRad = part.bendAngles[i] * M_PI / 180.0;
            const double neutralRadius = part.bendRadius + part.kFactor * part.thickness;
            const double turnSign = angleRad > 0.0 ? 1.0 : -1.0;
            const double centerX = x + neutralRadius * turnSign * leftX;
            const double centerY = y + neutralRadius * turnSign * leftY;
            const double startAngle = std::atan2(y - centerY, x - centerX);
            const double endAngle = startAngle + angleRad;
            const double radialX = (x - centerX) / neutralRadius;
            const double radialY = (y - centerY) / neutralRadius;
            const double sign = leftX * radialX + leftY * radialY; // +1 or -1: is leftNormal outward or inward here

            const double sideARadius = neutralRadius + sign * halfT;
            const double sideBRadius = neutralRadius - sign * halfT;
            const double u1 = std::min(startAngle, endAngle);
            const double u2 = std::max(startAngle, endAngle);

            // XDirection is pinned to world +X so the circle's own
            // parameter is a plain polar angle, matching startAngle/
            // endAngle computed via atan2 -- gp_Ax2's 2-argument
            // constructor would pick an arbitrary XDirection instead,
            // silently breaking that correspondence.
            const gp_Ax2 axis(gp_Pnt(centerX, centerY, 0.0), gp_Dir(0, 0, 1), gp_Dir(1, 0, 0));
            Handle(Geom_Circle) circleA = new Geom_Circle(axis, sideARadius);
            Handle(Geom_Circle) circleB = new Geom_Circle(axis, sideBRadius);
            Handle(Geom_TrimmedCurve) trimmedA = new Geom_TrimmedCurve(circleA, u1, u2);
            Handle(Geom_TrimmedCurve) trimmedB = new Geom_TrimmedCurve(circleB, u1, u2);

            result.sideAEdges.push_back(BRepBuilderAPI_MakeEdge(trimmedA));
            result.sideBEdges.push_back(BRepBuilderAPI_MakeEdge(trimmedB));
            result.neutralLength += std::abs(angleRad) * neutralRadius;

            x = centerX + neutralRadius * std::cos(endAngle);
            y = centerY + neutralRadius * std::sin(endAngle);
            theta += angleRad;
        } else {
            result.sideAEnd = sideAEnd;
            result.sideBEnd = sideBEnd;
        }
    }
    return result;
}

} // namespace

TopoDS_Shape buildSheetMetalSolid(const SheetMetalPart& part) {
    if (!partIsWellFormed(part)) return TopoDS_Shape();

    const SpineWalk walk = walkSpine(part);

    BRepBuilderAPI_MakeWire wireBuilder;
    for (const TopoDS_Edge& edge : walk.sideAEdges) wireBuilder.Add(edge);
    wireBuilder.Add(BRepBuilderAPI_MakeEdge(walk.sideAEnd, walk.sideBEnd));
    for (auto it = walk.sideBEdges.rbegin(); it != walk.sideBEdges.rend(); ++it) wireBuilder.Add(*it);
    wireBuilder.Add(BRepBuilderAPI_MakeEdge(walk.sideBStart, walk.sideAStart));
    if (!wireBuilder.IsDone()) return TopoDS_Shape();

    BRepBuilderAPI_MakeFace faceBuilder(wireBuilder.Wire());
    if (!faceBuilder.IsDone()) return TopoDS_Shape();

    return BRepPrimAPI_MakePrism(faceBuilder.Face(), gp_Vec(0.0, 0.0, part.width)).Shape();
}

double flatPatternLength(const SheetMetalPart& part) {
    if (!partIsWellFormed(part)) return 0.0;
    return walkSpine(part).neutralLength;
}

void insertFlatPatternIntoDocument(Document& doc2d, const SheetMetalPart& part, double offsetX, double offsetY) {
    const double length = flatPatternLength(part);
    if (length <= 0.0) return;

    LayerId flatLayer = 0;
    bool found = false;
    for (const Layer& layer : doc2d.layers()) {
        if (layer.name == "FLATPATTERN") {
            flatLayer = layer.id;
            found = true;
            break;
        }
    }
    if (!found) flatLayer = doc2d.addLayer("FLATPATTERN", Color{255, 165, 0});

    auto addLine = [&](Point2D a, Point2D b, bool centerline) {
        const EntityId id = doc2d.reserveEntityId();
        auto line = std::make_unique<LineEntity>(id, flatLayer, Point2D(a.x + offsetX, a.y + offsetY),
                                                  Point2D(b.x + offsetX, b.y + offsetY));
        if (centerline) line->setLinetypeOverride(LineType::Center);
        doc2d.addEntity(std::move(line));
    };

    // The boundary rectangle.
    addLine(Point2D(0.0, 0.0), Point2D(length, 0.0), false);
    addLine(Point2D(length, 0.0), Point2D(length, part.width), false);
    addLine(Point2D(length, part.width), Point2D(0.0, part.width), false);
    addLine(Point2D(0.0, part.width), Point2D(0.0, 0.0), false);

    // One dashed bend line per bend, at its neutral-axis position along
    // the unfolded strip.
    double cumulative = part.flatLengths.empty() ? 0.0 : part.flatLengths[0];
    const double neutralRadius = part.bendRadius + part.kFactor * part.thickness;
    for (std::size_t i = 0; i < part.bendAngles.size(); ++i) {
        const double allowance = std::abs(part.bendAngles[i] * M_PI / 180.0) * neutralRadius;
        const double bendCenter = cumulative + allowance / 2.0;
        addLine(Point2D(bendCenter, 0.0), Point2D(bendCenter, part.width), true);
        cumulative += allowance;
        if (i + 1 < part.flatLengths.size()) cumulative += part.flatLengths[i + 1];
    }
}

namespace {
// Measures target's own local wall thickness at startPoint by casting a
// ray inward from just outside startFace (along -normal) and finding the
// nearest OTHER face it hits -- the same real ray-face intersection
// Pick3D.h's own pickFace uses (IntCurvesFace_Intersector), just walked
// over every face of target instead of stopping at the first hit, since
// the first hit here would just be startFace's own near-zero-distance
// re-entry. Returns 0.0 if no second face is hit (e.g. the ray exits the
// solid entirely without crossing anything parallel -- not every solid
// is sheet-stock-thin along its own face normal).
double detectThicknessAlongNormal(const TopoDS_Shape& target, const TopoDS_Face& startFace, const gp_Pnt& startPoint,
                                  const gp_Dir& normal) {
    const gp_Pnt origin = startPoint.Translated(gp_Vec(normal) * 1e-4);
    const gp_Lin line(origin, normal.Reversed());

    TopTools_IndexedMapOfShape faceMap;
    TopExp::MapShapes(target, TopAbs_FACE, faceMap);

    bool found = false;
    double best = 0.0;
    for (int i = 1; i <= faceMap.Extent(); ++i) {
        const TopoDS_Face candidate = TopoDS::Face(faceMap(i));
        if (candidate.IsSame(startFace)) continue;
        IntCurvesFace_Intersector intersector(candidate, 1e-6);
        intersector.Perform(line, 0.0, Standard_Real(RealLast()));
        if (!intersector.IsDone()) continue;
        for (int j = 1; j <= intersector.NbPnt(); ++j) {
            const double w = intersector.WParameter(j);
            if (w < 0.0) continue;
            if (!found || w < best) {
                best = w;
                found = true;
            }
        }
    }
    return found ? best : 0.0;
}
} // namespace

TopoDS_Shape buildFaceFlange(const TopoDS_Shape& target, int edgeIndex, int referenceFaceIndex, double length,
                             double bendAngleDegrees, double thickness) {
    if (target.IsNull() || length <= 1e-9 || thickness < 0.0) return TopoDS_Shape();

    TopTools_IndexedMapOfShape edgeMap;
    TopTools_IndexedMapOfShape faceMap;
    TopExp::MapShapes(target, TopAbs_EDGE, edgeMap);
    TopExp::MapShapes(target, TopAbs_FACE, faceMap);
    if (edgeIndex < 0 || edgeIndex >= edgeMap.Extent()) return TopoDS_Shape();
    if (referenceFaceIndex < 0 || referenceFaceIndex >= faceMap.Extent()) return TopoDS_Shape();

    const TopoDS_Edge edge = TopoDS::Edge(edgeMap(edgeIndex + 1));
    const TopoDS_Face face = TopoDS::Face(faceMap(referenceFaceIndex + 1));

    TopoDS_Vertex v1, v2;
    TopExp::Vertices(edge, v1, v2);
    if (v1.IsNull() || v2.IsNull()) return TopoDS_Shape();
    const gp_Pnt p1 = BRep_Tool::Pnt(v1);
    const gp_Pnt p2 = BRep_Tool::Pnt(v2);
    const gp_Vec edgeVec(p1, p2);
    const double edgeLen = edgeVec.Magnitude();
    if (edgeLen < 1e-9) return TopoDS_Shape();
    const gp_Dir edgeDir(edgeVec);

    // The reference face's own outward normal at a representative (mid-UV)
    // point, and its centroid -- both needed to work out which in-plane
    // direction actually points "away from the face, past the edge" (see
    // below), the same real ray-picking-adjacent geometry Pick3D.h's
    // pickFace uses for its own normal.
    Standard_Real umin = 0, umax = 0, vmin = 0, vmax = 0;
    BRepTools::UVBounds(face, umin, umax, vmin, vmax);
    GeomLProp_SLProps props(BRep_Tool::Surface(face), (umin + umax) / 2.0, (vmin + vmax) / 2.0, 1, 1e-6);
    gp_Dir normal(0, 0, 1);
    if (props.IsNormalDefined()) normal = props.Normal();
    if (face.Orientation() == TopAbs_REVERSED) normal.Reverse();

    GProp_GProps massProps;
    BRepGProp::SurfaceProperties(face, massProps);
    const gp_Pnt faceCentroid = massProps.CentreOfMass();
    const gp_Pnt edgeMid((p1.X() + p2.X()) / 2.0, (p1.Y() + p2.Y()) / 2.0, (p1.Z() + p2.Z()) / 2.0);

    // thickness == 0 requests auto-detection: measure target's own local
    // wall thickness at the reference face by casting a ray straight
    // through it (see detectThicknessAlongNormal above), rather than
    // requiring the caller to already know it -- matching real
    // sheet-metal tools' own "pick a face, thickness is read from the
    // part" behavior. Falls through to the invalid-input return below if
    // detection fails (e.g. target isn't actually sheet-stock-thin there).
    if (thickness < 1e-9) thickness = detectThicknessAlongNormal(target, face, faceCentroid, normal);
    if (thickness < 1e-9) return TopoDS_Shape();

    // The in-plane direction pointing from the face's own center toward
    // the picked edge (and, by continuation, past it into open space) --
    // found by taking the raw centroid-to-edge vector and stripping out
    // whatever component it has along the edge itself and along the
    // face's normal, leaving only the in-plane-perpendicular-to-edge part.
    gp_Vec tangentVec(faceCentroid, edgeMid);
    tangentVec -= gp_Vec(edgeDir) * tangentVec.Dot(gp_Vec(edgeDir));
    tangentVec -= gp_Vec(normal) * tangentVec.Dot(gp_Vec(normal));
    if (tangentVec.Magnitude() < 1e-9) return TopoDS_Shape(); // degenerate: can't resolve an outward direction
    const gp_Dir tangent(tangentVec);

    // 0 degrees continues coplanar with the face (along tangent); 90
    // degrees rises perpendicular to it (along normal) -- see
    // buildFaceFlange's own header comment for the full convention.
    const double angleRad = bendAngleDegrees * M_PI / 180.0;
    const gp_Vec extrusionVec = gp_Vec(tangent) * std::cos(angleRad) + gp_Vec(normal) * std::sin(angleRad);
    if (extrusionVec.Magnitude() < 1e-9) return TopoDS_Shape();
    const gp_Dir extrusionDir(extrusionVec);

    gp_Dir thicknessDir;
    try {
        thicknessDir = edgeDir.Crossed(extrusionDir);
    } catch (const Standard_Failure&) {
        return TopoDS_Shape(); // edgeDir and extrusionDir came out parallel -- degenerate
    }

    // A box built on this axis system spans local X (=edgeDir) by edgeLen,
    // local Z (=thicknessDir, the main direction) by thickness, and local
    // Y -- which gp_Ax2 itself derives as (thicknessDir x edgeDir), proven
    // to equal extrusionDir exactly here since edgeDir/extrusionDir/
    // thicknessDir form a right-handed orthonormal set by construction --
    // by length, so the wall extrudes in exactly the intended direction.
    const gp_Ax2 flangeAxes(p1, thicknessDir, edgeDir);
    const TopoDS_Shape flangeBox = BRepPrimAPI_MakeBox(flangeAxes, edgeLen, length, thickness).Shape();

    BRepAlgoAPI_Fuse fuse(target, flangeBox);
    if (!fuse.IsDone()) return TopoDS_Shape();
    return fuse.Shape();
}

} // namespace lcad
