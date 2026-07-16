#pragma once

#include <TopoDS_Shape.hxx>
#include <gp_Trsf.hxx>

#include <string>
#include <vector>

namespace lcad {

// A placed component in an assembly -- typically a shape imported from its
// own STEP file (see StepIges.h) or a saved .kcad3d, since KumCAD is still
// single-document at the Document3D level (matching the same "file-based,
// not live-linked" simplification Phase 1's schematic/PCB netlist import
// made). shape is in the component's own local frame; placement carries it
// into assembly-world space.
struct AssemblyComponent {
    std::string name;
    TopoDS_Shape shape;
    gp_Trsf placement;   // world placement; identity until fixed or solved
    bool fixed = false;  // fixed components are never moved by Assembly::solve()
};

// Which pair of reference elements a mate aligns. There's no face/edge
// picking in the still-unverified 3D viewport (see Viewport3D.h's own
// disclosure), so each mate is defined directly by a point+direction typed
// in on each component's local frame -- the same kind of blunt, disclosed
// scope cut this sprint's siblings (Fillet/Chamfer's "every edge") made.
//
// Rather than a general nonlinear multi-body DOF solver (what FreeCAD's
// Assembly workbench or a real mechanical CAD mate stack actually builds --
// the "second-highest risk" item called out when this phase was planned),
// each mate is solved in closed form: it places componentB directly
// relative to componentA's current placement, so a chain of mates solves
// like Document3D's own feature tree does -- one forward pass in list
// order, with "append order is topological order" as the standing
// assumption (componentA must already have its final placement, either
// because it's `fixed` or because an earlier mate already placed it as its
// own componentB).
enum class MateType {
    Coincident, // point-to-point coincidence, reference directions anti-parallel
                // (the common "face-to-face" mate convention)
    Concentric, // point-to-point coincidence, reference directions parallel
                // (the common "axis-to-axis" mate convention)
    Distance,   // like Coincident, offset along the shared direction by `value`
    Angle,      // like Concentric, plus an extra rotation of `value` degrees
                // around the shared axis after aligning
};

struct Mate {
    MateType type = MateType::Coincident;
    int componentA = -1;
    int componentB = -1;

    // Reference point + direction on componentA, in ITS OWN local frame.
    double ax = 0.0, ay = 0.0, az = 0.0;
    double adx = 0.0, ady = 0.0, adz = 1.0;

    // Reference point + direction on componentB, in ITS OWN local frame.
    double bx = 0.0, by = 0.0, bz = 0.0;
    double bdx = 0.0, bdy = 0.0, bdz = 1.0;

    double value = 0.0; // Distance offset, or Angle degrees
};

class Assembly {
public:
    int addComponent(AssemblyComponent component);
    void addMate(Mate mate);

    // Solves every mate in list order (see MateType's comment for the
    // solving model). Out-of-range component indices are skipped.
    void solve();

    const std::vector<AssemblyComponent>& components() const { return m_components; }
    std::vector<AssemblyComponent>& components() { return m_components; }
    const std::vector<Mate>& mates() const { return m_mates; }

private:
    std::vector<AssemblyComponent> m_components;
    std::vector<Mate> m_mates;
};

} // namespace lcad
