#pragma once

#include "core/sketch/SketchGeometry.h"

namespace lcad {

struct SolveResult {
    bool converged = false;
    double finalResidualNorm = 0.0;
    int iterations = 0;
};

// Solves sketch's constraints in place via Levenberg-Marquardt with a
// numerically-differentiated Jacobian -- no analytical derivatives, no
// external linear-algebra dependency (see LinearSolve.h for the small
// from-scratch dense solver backing it).
//
// Disclosed limitation (real, not swept under the rug): an over-
// constrained sketch converges to a least-squares compromise (or fails to
// converge) rather than reporting exactly WHICH constraints conflict --
// that needs real symbolic/numeric rank analysis of the constraint
// Jacobian (a much deeper undertaking, the kind FreeCAD's own solver or a
// commercial one does), not something this pass adds. What IS added
// below is a bounded, honest count-based diagnostic (free variables vs.
// constraint equations) -- it tells you whether a sketch has room left to
// move or more equations than it has freedom for, not which specific
// constraint is the redundant one.
SolveResult solveSketch(Sketch& sketch, int maxIterations = 100, double tolerance = 1e-9);

struct DofReport {
    int totalDof = 0;              // 2*(free points) + (free circle radii) + (free arc radii)
    int constraintEquations = 0;   // user constraints (1 scalar equation each, except Midpoint/Symmetric which are 2 -- see SketchConstraintType's own comment) + 2*(arc count) for the always-on arc radius-consistency equations
    int remainingDof = 0;          // max(0, totalDof - constraintEquations) -- 0 means "no freedom left, at least on a naive count"
    // A necessary, not sufficient, over-constraint signal: constraintEquations
    // > totalDof means the system has strictly more equations than unknowns,
    // so it CANNOT have an exact solution in general (though a specific,
    // non-independent set of equations occasionally still could -- that
    // subtlety is exactly what full rank analysis would resolve and this
    // doesn't attempt to).
    bool likelyOverConstrained = false;
};

// A pure count, computed without running the solver -- see DofReport's own
// field comments for exactly what each number does and doesn't prove.
DofReport analyzeDof(const Sketch& sketch);

} // namespace lcad
