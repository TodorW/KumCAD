#include "core/core3d/HalfSpaceBox.h"

#include <BRepPrimAPI_MakeBox.hxx>
#include <gp_Ax2.hxx>
#include <gp_Vec.hxx>

#include <cmath>

namespace lcad {

TopoDS_Shape buildCuttingHalfSpaceBox(const gp_Pnt& origin, const gp_Dir& normal, double half) {
    const gp_Dir arbitrary = (std::abs(normal.Z()) < 0.9) ? gp_Dir(0, 0, 1) : gp_Dir(1, 0, 0);
    const gp_Vec xVec = gp_Vec(arbitrary) - gp_Vec(normal) * gp_Vec(arbitrary).Dot(gp_Vec(normal));
    const gp_Dir xDir(xVec);
    const gp_Ax2 originAxis(origin, normal, xDir); // Z=normal, X=xDir, Y=normal^xDir (right-handed, auto)
    const gp_Pnt corner = origin.Translated(gp_Vec(xDir) * (-half)).Translated(gp_Vec(originAxis.YDirection()) * (-half));
    const gp_Ax2 boxAxis(corner, normal, xDir);
    return BRepPrimAPI_MakeBox(boxAxis, 2.0 * half, 2.0 * half, half).Shape();
}

} // namespace lcad
