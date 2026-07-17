#include "core/schematic/Spice.h"

#include "core/document/Document.h"
#include "core/geometry/Insert.h"
#include "core/schematic/Netlist.h"
#include "core/sketch/LinearSolve.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <unordered_map>

namespace lcad {

namespace {

// Generic silicon diode model (no .model cards in this minimal solver --
// see Spice.h's own disclosure).
constexpr double kDiodeIs = 1e-14;
constexpr double kDiodeVt = 0.02585;
// Newton damping: clamp how far a diode's linearization point can move in
// one iteration, the standard SPICE "voltage limiting" technique that
// keeps exp() from overflowing and keeps the iteration from overshooting
// wildly on a stiff source.
constexpr double kDiodeDamping = 4.0 * kDiodeVt;

struct MnaSystem {
    std::vector<std::vector<double>> g;
    std::vector<double> i;
    explicit MnaSystem(int size) : g(size, std::vector<double>(size, 0.0)), i(size, 0.0) {}
};

// Stamps a two-terminal device whose current flowing p->n is modeled as
// conductance*(Vp-Vn) - isource (isource flows n->p through the device,
// i.e. is injected into node p from outside). Every element type here
// (resistor, independent current source, capacitor/inductor companion
// models, and the linearized diode) reduces to this one form.
void stampGCurrent(MnaSystem& sys, int p, int n, double conductance, double isource) {
    if (p != 0) {
        sys.g[p - 1][p - 1] += conductance;
        sys.i[p - 1] += isource;
    }
    if (n != 0) {
        sys.g[n - 1][n - 1] += conductance;
        sys.i[n - 1] -= isource;
    }
    if (p != 0 && n != 0) {
        sys.g[p - 1][n - 1] -= conductance;
        sys.g[n - 1][p - 1] -= conductance;
    }
}

// Stamps a branch-current unknown at matrix row/col branchRow (0-indexed,
// already offset past the node rows) for a device that forces Vp-Vn ==
// voltage exactly -- independent voltage sources, and inductors in DC
// mode (an ideal 0V short).
void stampVoltageBranch(MnaSystem& sys, int p, int n, int branchRow, double voltage) {
    if (p != 0) {
        sys.g[p - 1][branchRow] += 1.0;
        sys.g[branchRow][p - 1] += 1.0;
    }
    if (n != 0) {
        sys.g[n - 1][branchRow] -= 1.0;
        sys.g[branchRow][n - 1] -= 1.0;
    }
    sys.i[branchRow] += voltage;
}

double nodeVoltage(const std::vector<double>& x, int node) {
    return node == 0 ? 0.0 : x[node - 1];
}

enum class Mode { Dc, Transient };

// Shared assembly for both DC operating point and one transient time
// step. capHistory[i]/indHistory[i] are only read for element i when it's
// a Capacitor/Inductor and mode==Transient (previous voltage/current);
// diodeV0[i] is the current Newton linearization point for element i when
// it's a Diode (any mode). branchOf[i] gives element i's row in the extra
// branch-current block for VoltageSource always, and Inductor when
// mode==Dc; -1 otherwise.
MnaSystem assemble(const Circuit& circuit, Mode mode, double dt, const std::vector<double>& capHistory,
                   const std::vector<double>& indHistory, const std::vector<double>& diodeV0,
                   const std::vector<int>& branchOf, int branchCount) {
    MnaSystem sys(circuit.nodeCount + branchCount);
    for (std::size_t idx = 0; idx < circuit.elements.size(); ++idx) {
        const CircuitElement& e = circuit.elements[idx];
        switch (e.type) {
        case ElementType::Resistor:
            stampGCurrent(sys, e.nodeP, e.nodeN, e.value > 0 ? 1.0 / e.value : 0.0, 0.0);
            break;
        case ElementType::CurrentSource:
            stampGCurrent(sys, e.nodeP, e.nodeN, 0.0, e.value);
            break;
        case ElementType::Capacitor:
            if (mode == Mode::Transient) {
                const double geq = e.value / dt;
                stampGCurrent(sys, e.nodeP, e.nodeN, geq, geq * capHistory[idx]);
            }
            // DC: an open circuit -- no stamp at all.
            break;
        case ElementType::Inductor:
            if (mode == Mode::Transient) {
                const double geq = dt / e.value;
                stampGCurrent(sys, e.nodeP, e.nodeN, geq, -indHistory[idx]);
            } else {
                stampVoltageBranch(sys, e.nodeP, e.nodeN, circuit.nodeCount + branchOf[idx], 0.0);
            }
            break;
        case ElementType::VoltageSource:
            stampVoltageBranch(sys, e.nodeP, e.nodeN, circuit.nodeCount + branchOf[idx], e.value);
            break;
        case ElementType::Diode: {
            const double v0 = diodeV0[idx];
            const double expArg = std::min(v0 / kDiodeVt, 40.0); // avoid exp() overflow
            const double id = kDiodeIs * (std::exp(expArg) - 1.0);
            const double geq = (kDiodeIs / kDiodeVt) * std::exp(expArg);
            stampGCurrent(sys, e.nodeP, e.nodeN, geq, geq * v0 - id);
            break;
        }
        }
    }
    return sys;
}

// Newton-Raphson wrapper shared by DC and one transient step: solves
// assemble() repeatedly, updating each diode's linearization point with
// damping, until convergence or maxIterations. Returns the final solved
// vector (node voltages + branch currents) and whether it converged.
struct SolveResult {
    bool converged = true;
    std::vector<double> x;
};

SolveResult newtonSolve(const Circuit& circuit, Mode mode, double dt, const std::vector<double>& capHistory,
                        const std::vector<double>& indHistory, const std::vector<int>& branchOf, int branchCount,
                        int maxIterations, double tolerance) {
    std::vector<double> diodeV0(circuit.elements.size(), 0.0);
    bool hasDiode = false;
    for (const CircuitElement& e : circuit.elements) {
        if (e.type == ElementType::Diode) hasDiode = true;
    }

    const int iterations = hasDiode ? std::max(1, maxIterations) : 1;
    SolveResult result;
    for (int iter = 0; iter < iterations; ++iter) {
        const MnaSystem sys = assemble(circuit, mode, dt, capHistory, indHistory, diodeV0, branchOf, branchCount);
        if (!solveLinearSystem(sys.g, sys.i, result.x)) {
            result.converged = false;
            return result;
        }
        if (!hasDiode) return result;

        double maxDelta = 0.0;
        for (std::size_t idx = 0; idx < circuit.elements.size(); ++idx) {
            if (circuit.elements[idx].type != ElementType::Diode) continue;
            const CircuitElement& e = circuit.elements[idx];
            const double vRaw = nodeVoltage(result.x, e.nodeP) - nodeVoltage(result.x, e.nodeN);
            const double delta = std::clamp(vRaw - diodeV0[idx], -kDiodeDamping, kDiodeDamping);
            diodeV0[idx] += delta;
            maxDelta = std::max(maxDelta, std::abs(delta));
        }
        if (maxDelta < tolerance) return result;
    }
    result.converged = false;
    return result;
}

} // namespace

bool parseEngineeringValue(const std::string& s, double& out) {
    std::size_t end = 0;
    double value;
    try {
        value = std::stod(s, &end);
    } catch (...) {
        return false;
    }
    if (end < s.size()) {
        switch (s[end]) {
        case 'p': value *= 1e-12; break;
        case 'n': value *= 1e-9; break;
        case 'u': case '\xb5': value *= 1e-6; break; // 'u' or the mu sign (latin-1 0xB5)
        case 'm': value *= 1e-3; break;
        case 'k': case 'K': value *= 1e3; break;
        case 'M': value *= 1e6; break;
        case 'G': value *= 1e9; break;
        default: break; // unrecognized trailing text: keep the bare numeric prefix
        }
    }
    out = value;
    return true;
}

CircuitBuildResult buildCircuitFromDocument(const Document& doc) {
    CircuitBuildResult result;
    result.nodeNames.push_back("GND");

    const std::vector<Net> nets = computeNets(doc);
    std::unordered_map<EntityId, std::unordered_map<std::string, int>> pinToNode;
    int nextNode = 1;
    for (const Net& net : nets) {
        std::string upper = net.name;
        std::transform(upper.begin(), upper.end(), upper.begin(), [](unsigned char c) { return std::toupper(c); });
        const int node = (upper == "GND") ? 0 : nextNode++;
        if (node != 0) result.nodeNames.push_back(net.name);
        for (const NetPin& pin : net.pins) pinToNode[pin.insertId][pin.pinNumber] = node;
    }
    result.circuit.nodeCount = nextNode - 1;

    static const std::unordered_map<std::string, ElementType> kBlockTypes = {
        {"R", ElementType::Resistor},   {"C", ElementType::Capacitor},     {"L", ElementType::Inductor},
        {"V", ElementType::VoltageSource}, {"I", ElementType::CurrentSource}, {"D", ElementType::Diode},
    };

    for (const Entity* e : doc.entities()) {
        if (e->type() != EntityType::Insert) continue;
        const auto& insert = static_cast<const InsertEntity&>(*e);
        const auto typeIt = kBlockTypes.find(insert.blockName());
        if (typeIt == kBlockTypes.end()) continue;

        const std::string* refDesAttr = insert.attributeValue("REFDES");
        const std::string refDes = refDesAttr ? *refDesAttr : ("U" + std::to_string(insert.id()));

        const auto pins = pinToNode.find(insert.id());
        const int nodeP = (pins != pinToNode.end() && pins->second.count("1")) ? pins->second.at("1") : -1;
        const int nodeN = (pins != pinToNode.end() && pins->second.count("2")) ? pins->second.at("2") : -1;
        if (nodeP < 0 || nodeN < 0) {
            result.skippedRefDes.push_back(refDes);
            continue;
        }

        double value = 0.0;
        if (typeIt->second != ElementType::Diode) {
            const std::string* valueAttr = insert.attributeValue("VALUE");
            if (!valueAttr || !parseEngineeringValue(*valueAttr, value)) {
                result.skippedRefDes.push_back(refDes);
                continue;
            }
        }

        result.circuit.elements.push_back(CircuitElement{typeIt->second, nodeP, nodeN, value, refDes});
    }

    // Compact node numbering: a net only touched by a SKIPPED element (bad
    // VALUE, unrecognized block type, or simply nothing simulatable) still
    // got a raw node number above, but no kept element ever references it
    // -- left in place, it becomes a disconnected row in the MNA matrix
    // (all zeros), which is singular and makes the solver spuriously fail
    // to converge on an otherwise perfectly solvable circuit. Renumber to
    // only the nodes kept elements actually touch.
    std::vector<bool> used(nextNode, false);
    for (const CircuitElement& e : result.circuit.elements) {
        if (e.nodeP > 0) used[e.nodeP] = true;
        if (e.nodeN > 0) used[e.nodeN] = true;
    }
    std::vector<int> remap(nextNode, -1);
    remap[0] = 0;
    std::vector<std::string> compactNames = {"GND"};
    int compact = 1;
    for (int n = 1; n < nextNode; ++n) {
        if (!used[n]) continue;
        remap[n] = compact++;
        compactNames.push_back(result.nodeNames[n]);
    }
    for (CircuitElement& e : result.circuit.elements) {
        e.nodeP = remap[e.nodeP];
        e.nodeN = remap[e.nodeN];
    }
    result.circuit.nodeCount = compact - 1;
    result.nodeNames = std::move(compactNames);

    return result;
}

OperatingPointResult solveDcOperatingPoint(const Circuit& circuit, int maxIterations, double tolerance) {
    std::vector<int> branchOf(circuit.elements.size(), -1);
    int branchCount = 0;
    for (std::size_t idx = 0; idx < circuit.elements.size(); ++idx) {
        const ElementType type = circuit.elements[idx].type;
        if (type == ElementType::VoltageSource || type == ElementType::Inductor) branchOf[idx] = branchCount++;
    }

    const std::vector<double> unused;
    const SolveResult solved =
        newtonSolve(circuit, Mode::Dc, 0.0, unused, unused, branchOf, branchCount, maxIterations, tolerance);

    OperatingPointResult result;
    result.converged = solved.converged;
    result.nodeVoltages.assign(circuit.nodeCount + 1, 0.0);
    result.branchCurrents.assign(circuit.elements.size(), 0.0);
    if (!solved.converged) return result;

    for (int n = 1; n <= circuit.nodeCount; ++n) result.nodeVoltages[n] = solved.x[n - 1];
    for (std::size_t idx = 0; idx < circuit.elements.size(); ++idx) {
        if (branchOf[idx] >= 0) result.branchCurrents[idx] = solved.x[circuit.nodeCount + branchOf[idx]];
    }
    return result;
}

std::vector<TransientPoint> solveTransient(const Circuit& circuit, const OperatingPointResult& op, double tStep,
                                           double tStop, int maxIterations, double tolerance) {
    std::vector<TransientPoint> points;
    if (tStep <= 0.0 || !op.converged) return points;

    std::vector<double> capHistory(circuit.elements.size(), 0.0);
    std::vector<double> indHistory(circuit.elements.size(), 0.0);
    for (std::size_t idx = 0; idx < circuit.elements.size(); ++idx) {
        const CircuitElement& e = circuit.elements[idx];
        if (e.type == ElementType::Capacitor) {
            capHistory[idx] = op.nodeVoltages[e.nodeP] - op.nodeVoltages[e.nodeN];
        } else if (e.type == ElementType::Inductor) {
            indHistory[idx] = op.branchCurrents[idx];
        }
    }

    // No branch-current unknowns in transient mode: inductors use the
    // Norton companion model like capacitors, and only VoltageSource still
    // needs one.
    std::vector<int> branchOf(circuit.elements.size(), -1);
    int branchCount = 0;
    for (std::size_t idx = 0; idx < circuit.elements.size(); ++idx) {
        if (circuit.elements[idx].type == ElementType::VoltageSource) branchOf[idx] = branchCount++;
    }

    for (double t = tStep; t <= tStop + tStep * 1e-6; t += tStep) {
        const SolveResult solved = newtonSolve(circuit, Mode::Transient, tStep, capHistory, indHistory, branchOf,
                                               branchCount, maxIterations, tolerance);
        if (!solved.converged) break;

        TransientPoint point;
        point.time = t;
        point.nodeVoltages.assign(circuit.nodeCount + 1, 0.0);
        for (int n = 1; n <= circuit.nodeCount; ++n) point.nodeVoltages[n] = solved.x[n - 1];
        points.push_back(point);

        for (std::size_t idx = 0; idx < circuit.elements.size(); ++idx) {
            const CircuitElement& e = circuit.elements[idx];
            if (e.type == ElementType::Capacitor) {
                capHistory[idx] = nodeVoltage(solved.x, e.nodeP) - nodeVoltage(solved.x, e.nodeN);
            } else if (e.type == ElementType::Inductor) {
                // Backward-Euler companion current: i(t) = Geq*v(t) + i_prev.
                const double geq = tStep / e.value;
                const double v = nodeVoltage(solved.x, e.nodeP) - nodeVoltage(solved.x, e.nodeN);
                indHistory[idx] = geq * v + indHistory[idx];
            }
        }
    }
    return points;
}

} // namespace lcad
