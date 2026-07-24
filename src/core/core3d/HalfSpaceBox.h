#pragma once

#include <TopoDS_Shape.hxx>

#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

namespace lcad {

// A generously-sized axis-aligned-to-the-cut-plane box covering the whole
// positive-normal half-space near origin: corner offset -half along both
// in-plane axes (so it's centered on origin across the plane), extending
// 2*half laterally and half along +normal -- exactly the "material to
// remove" half-space BRepAlgoAPI_Cut needs, without relying on
// BRepPrimAPI_MakeHalfSpace's own reference-point convention. Shared
// between TechDraw.cpp's own projectSectionView (2D section views) and
// Document3D.cpp's FeatureType::Slice (a real 3D cut), the same
// half-space-box cutting technique either way.
TopoDS_Shape buildCuttingHalfSpaceBox(const gp_Pnt& origin, const gp_Dir& normal, double half);

} // namespace lcad
