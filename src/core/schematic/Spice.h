#pragma once

#include <string>
#include <vector>

namespace lcad {

class Document;

// A from-scratch, deliberately minimal SPICE-lite circuit solver: Modified
// Nodal Analysis (MNA, the same formulation ngspice/LTspice use) via the
// existing dense Gaussian-elimination solveLinearSystem (core/sketch/
// LinearSolve.h) -- no vendored simulator, matching this codebase's
// standing "write it, don't vendor it" call (PlaneGCS/CalculiX made the
// same one). Supports R/C/L, independent DC voltage/current sources, and
// one nonlinear device (a generic silicon diode, Is=1e-14A n=1, solved by
// damped Newton-Raphson) -- enough to check a divider, an RC/RL transient,
// or a simple rectifier/clamp, not a general-purpose SPICE replacement.
// Explicitly out of scope: BJTs/MOSFETs, AC/frequency-domain analysis,
// .model cards, subcircuits.

enum class ElementType { Resistor, Capacitor, Inductor, VoltageSource, CurrentSource, Diode };

// Two-terminal element between nodeP and nodeN (node 0 is always ground).
// value's unit depends on type: ohms/farads/henries/volts/amps; unused for
// Diode (which always uses the fixed generic-silicon Is/n noted above).
struct CircuitElement {
    ElementType type;
    int nodeP = 0;
    int nodeN = 0;
    double value = 0.0;
    std::string name; // for reporting, e.g. a REFDES
};

struct Circuit {
    int nodeCount = 0; // non-ground nodes are numbered 1..nodeCount
    std::vector<CircuitElement> elements;
};

// Parses an engineering-notation value string ("10k", "4.7u", "100n") into
// a plain double. Recognized suffixes: p/n/u (or the mu sign)/m/k/M/G,
// case-sensitive so 'm' (milli) and 'M' (mega) don't collide, matching
// SPICE's own convention (though real SPICE also accepts "meg" -- not
// implemented here). Returns false, leaving out untouched, if s has no
// numeric prefix at all.
bool parseEngineeringValue(const std::string& s, double& out);

// Builds a Circuit from doc's computed nets (core/schematic/Netlist.h):
// every placed INSERT whose block is R/C/L/V/I/D contributes one element,
// its two pins ("1"/"2") mapped to whichever net each touches. A net
// named "GND" (case-insensitive) becomes node 0; every other net gets a
// fresh positive index. Elements are skipped (name recorded in
// skippedRefDes) when their VALUE attribute is missing or unparseable, or
// when a pin touches no net at all (unconnected). refDes is read from the
// REFDES attribute, falling back to "U<id>" like Bom.cpp does.
struct CircuitBuildResult {
    Circuit circuit;
    std::vector<std::string> nodeNames; // index 0 = "GND", size nodeCount+1
    std::vector<std::string> skippedRefDes;
};
CircuitBuildResult buildCircuitFromDocument(const Document& doc);

struct OperatingPointResult {
    bool converged = true;
    std::vector<double> nodeVoltages;   // index 0 is ground (always 0V); size nodeCount+1
    std::vector<double> branchCurrents; // one per VoltageSource/Inductor element, same order as circuit.elements
                                        // (0 for every other element type)
};

// DC operating point: capacitors are open circuits, inductors are ideal
// (0V) shorts -- both exact MNA formulations, not approximations.
// Diodes are linearized via damped Newton-Raphson (voltage step per
// iteration clamped to avoid the exponential overflowing, the standard
// SPICE technique) until every node voltage changes by less than
// tolerance or maxIterations is reached (converged=false in that case).
OperatingPointResult solveDcOperatingPoint(const Circuit& circuit, int maxIterations = 100, double tolerance = 1e-9);

struct TransientPoint {
    double time;
    std::vector<double> nodeVoltages; // same indexing as OperatingPointResult
};

// Backward-Euler transient analysis, t=0..tStop in tStep increments.
// Reactive elements start from op's steady state: a capacitor's initial
// voltage is its op-point node-voltage difference, an inductor's initial
// current is its op-point branch current (both exact, since op treats
// them as open/short respectively -- not an approximation carried into
// the transient). Backward Euler is unconditionally stable (unlike
// forward Euler), the standard reason SPICE-family tools default to it.
std::vector<TransientPoint> solveTransient(const Circuit& circuit, const OperatingPointResult& op, double tStep,
                                           double tStop, int maxIterations = 50, double tolerance = 1e-9);

} // namespace lcad
