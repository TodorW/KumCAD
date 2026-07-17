#include "core/document/Document.h"
#include "core/geometry/Insert.h"
#include "core/geometry/NetLabel.h"
#include "core/geometry/Wire.h"
#include "core/schematic/Spice.h"
#include "core/schematic/SymbolLibrary.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

using namespace lcad;
using Catch::Approx;

TEST_CASE("parseEngineeringValue recognizes SPICE-style suffixes", "[spice]") {
    double v = 0.0;
    REQUIRE(parseEngineeringValue("10k", v));
    REQUIRE(v == Approx(10000.0));
    REQUIRE(parseEngineeringValue("4.7u", v));
    REQUIRE(v == Approx(4.7e-6));
    REQUIRE(parseEngineeringValue("100n", v));
    REQUIRE(v == Approx(100e-9));
    REQUIRE(parseEngineeringValue("2.5m", v));
    REQUIRE(v == Approx(2.5e-3));
    REQUIRE(parseEngineeringValue("1M", v));
    REQUIRE(v == Approx(1e6));
    REQUIRE(parseEngineeringValue("1G", v));
    REQUIRE(v == Approx(1e9));
    REQUIRE(parseEngineeringValue("5", v));
    REQUIRE(v == Approx(5.0));
    // 'm' (milli) and 'M' (mega) must not collide.
    REQUIRE(parseEngineeringValue("3m", v));
    REQUIRE(v == Approx(3e-3));

    REQUIRE_FALSE(parseEngineeringValue("abc", v));
    REQUIRE_FALSE(parseEngineeringValue("", v));
}

TEST_CASE("solveDcOperatingPoint: resistive voltage divider matches the analytic ratio", "[spice]") {
    // Node 1 = 10V source, node 2 = divider tap, node 0 = GND.
    // R1 (node1->node2) = 1k, R2 (node2->GND) = 3k => V2 = 10 * 3/(1+3) = 7.5V.
    Circuit circuit;
    circuit.nodeCount = 2;
    circuit.elements = {
        CircuitElement{ElementType::VoltageSource, 1, 0, 10.0, "V1"},
        CircuitElement{ElementType::Resistor, 1, 2, 1000.0, "R1"},
        CircuitElement{ElementType::Resistor, 2, 0, 3000.0, "R2"},
    };

    const OperatingPointResult op = solveDcOperatingPoint(circuit);
    REQUIRE(op.converged);
    REQUIRE(op.nodeVoltages[1] == Approx(10.0));
    REQUIRE(op.nodeVoltages[2] == Approx(7.5));
    // The voltage source's branch current: (10-7.5)/1000 = 2.5mA flowing INTO
    // node 1 from the source (source delivers positive current here).
    REQUIRE(op.branchCurrents[0] == Approx(-0.0025).margin(1e-9));
}

TEST_CASE("solveDcOperatingPoint: current source into a resistor to ground", "[spice]") {
    // 2mA current source pushes into node 1, R=500 to ground => V1 = 1V.
    Circuit circuit;
    circuit.nodeCount = 1;
    circuit.elements = {
        CircuitElement{ElementType::CurrentSource, 1, 0, 0.002, "I1"},
        CircuitElement{ElementType::Resistor, 1, 0, 500.0, "R1"},
    };
    const OperatingPointResult op = solveDcOperatingPoint(circuit);
    REQUIRE(op.converged);
    REQUIRE(op.nodeVoltages[1] == Approx(1.0));
}

TEST_CASE("solveDcOperatingPoint: capacitor is an open circuit, inductor an ideal short", "[spice]") {
    // V1 (5V) -- R1 (1k) -- node2 -- C1 -- GND: no DC current path through C,
    // so no drop across R1: V(node2) = 5V exactly.
    Circuit circuit;
    circuit.nodeCount = 2;
    circuit.elements = {
        CircuitElement{ElementType::VoltageSource, 1, 0, 5.0, "V1"},
        CircuitElement{ElementType::Resistor, 1, 2, 1000.0, "R1"},
        CircuitElement{ElementType::Capacitor, 2, 0, 1e-6, "C1"},
    };
    const OperatingPointResult op = solveDcOperatingPoint(circuit);
    REQUIRE(op.converged);
    REQUIRE(op.nodeVoltages[2] == Approx(5.0));

    // V1 (5V) -- R1 (1k) -- node2 -- L1 -- GND: inductor is a dead short, so
    // node2 == 0V and all 5mA flows through R1 then L1.
    Circuit circuitL;
    circuitL.nodeCount = 2;
    circuitL.elements = {
        CircuitElement{ElementType::VoltageSource, 1, 0, 5.0, "V1"},
        CircuitElement{ElementType::Resistor, 1, 2, 1000.0, "R1"},
        CircuitElement{ElementType::Inductor, 2, 0, 1e-3, "L1"},
    };
    const OperatingPointResult opL = solveDcOperatingPoint(circuitL);
    REQUIRE(opL.converged);
    REQUIRE(opL.nodeVoltages[2] == Approx(0.0).margin(1e-9));
    REQUIRE(opL.branchCurrents[2] == Approx(0.005)); // inductor's own branch current
}

TEST_CASE("solveTransient: RC discharge follows the exact exponential time constant", "[spice]") {
    // Charge C1 to 5V via the DC op point (R1 negligible... actually use an
    // op point where C1 sits directly across the 5V source through a 0.001
    // ohm "wire" resistor so v(0)=5V almost exactly), then remove the
    // source and let it discharge through R1 into ground, tau = R1*C1.
    // Simpler: build op point circuit WITH the source, take capHistory from
    // it, then hand-run a discharge-only circuit at the same op-point via
    // solveTransient's own op-seeding contract (capacitor voltage = op
    // node-voltage difference) by constructing an op result directly.
    constexpr double kR = 1000.0;
    constexpr double kC = 1e-6;
    constexpr double kTau = kR * kC; // 1ms

    Circuit circuit;
    circuit.nodeCount = 1;
    circuit.elements = {
        CircuitElement{ElementType::Resistor, 1, 0, kR, "R1"},
        CircuitElement{ElementType::Capacitor, 1, 0, kC, "C1"},
    };

    OperatingPointResult op;
    op.converged = true;
    op.nodeVoltages = {0.0, 5.0}; // C1 starts charged to 5V, no source present
    op.branchCurrents = {0.0, 0.0};

    const double tStep = kTau / 100.0;
    const double tStop = kTau * 5.0;
    const auto points = solveTransient(circuit, op, tStep, tStop);
    REQUIRE(points.size() > 400);

    // v(t) = 5 * exp(-t/tau); check at t=tau (~1.839V) and t=3*tau (~0.249V).
    // Backward Euler is only first-order accurate, so allow a few percent.
    auto voltageNear = [&](double t) {
        auto it = std::min_element(points.begin(), points.end(),
                                   [t](const TransientPoint& a, const TransientPoint& b) {
                                       return std::abs(a.time - t) < std::abs(b.time - t);
                                   });
        return it->nodeVoltages[1];
    };
    REQUIRE(voltageNear(kTau) == Approx(5.0 * std::exp(-1.0)).epsilon(0.02));
    REQUIRE(voltageNear(3 * kTau) == Approx(5.0 * std::exp(-3.0)).epsilon(0.03));
}

TEST_CASE("solveTransient: RL current rise follows the exact exponential time constant", "[spice]") {
    // Series R-L driven by a step voltage source, current rises as
    // i(t) = (V/R)*(1 - exp(-t/tau)), tau = L/R.
    constexpr double kR = 100.0;
    constexpr double kL = 0.1;
    constexpr double kV = 10.0;
    constexpr double kTau = kL / kR; // 1ms

    Circuit circuit;
    circuit.nodeCount = 2;
    circuit.elements = {
        CircuitElement{ElementType::VoltageSource, 1, 0, kV, "V1"},
        CircuitElement{ElementType::Resistor, 1, 2, kR, "R1"},
        CircuitElement{ElementType::Inductor, 2, 0, kL, "L1"},
    };

    OperatingPointResult op;
    op.converged = true;
    op.nodeVoltages = {0.0, 0.0, 0.0}; // inductor current starts at 0
    op.branchCurrents = {0.0, 0.0, 0.0};

    const double tStep = kTau / 200.0;
    const double tStop = kTau * 3.0;
    const auto points = solveTransient(circuit, op, tStep, tStop);
    REQUIRE_FALSE(points.empty());

    // Final current at t=3*tau should approach (V/R)*(1-exp(-3)).
    const double expected = (kV / kR) * (1.0 - std::exp(-3.0));
    const double actualCurrent = (kV - points.back().nodeVoltages[2]) / kR; // through R1
    REQUIRE(actualCurrent == Approx(expected).epsilon(0.03));
}

TEST_CASE("solveDcOperatingPoint: diode clamps a resistor-fed node near its forward drop", "[spice]") {
    // V1 (5V) -- R1 (1k) -- node1 -- D1 (anode) -> GND (cathode): the
    // node settles near a generic silicon diode's ~0.6-0.7V forward drop,
    // not at 5V, proving the diode is actually doing something (a linear
    // solve with the diode simply omitted would leave the node floating
    // or wildly different).
    // node1 = source+, node2 = diode anode.
    Circuit circuit;
    circuit.nodeCount = 2;
    circuit.elements = {
        CircuitElement{ElementType::VoltageSource, 1, 0, 5.0, "V1"},
        CircuitElement{ElementType::Resistor, 1, 2, 1000.0, "R1"},
        CircuitElement{ElementType::Diode, 2, 0, 0.0, "D1"},
    };

    const OperatingPointResult op = solveDcOperatingPoint(circuit);
    REQUIRE(op.converged);
    REQUIRE(op.nodeVoltages[2] > 0.4);
    REQUIRE(op.nodeVoltages[2] < 0.9);
    // Current through R1 should roughly match (5 - Vd)/1000, a sanity
    // check that KCL actually balances at the diode node.
    const double throughR1 = (5.0 - op.nodeVoltages[2]) / 1000.0;
    REQUIRE(throughR1 > 0.0038);
    REQUIRE(throughR1 < 0.0046);
}

TEST_CASE("buildCircuitFromDocument maps a real schematic's wires/attributes/GND label", "[spice]") {
    Document doc;
    registerBuiltinSymbols(doc); // installs "V", "R", "D" blocks with pins "1"/"2"

    const BlockDefinition* vBlock = doc.findBlock("V");
    const BlockDefinition* rBlock = doc.findBlock("R");
    REQUIRE(vBlock);
    REQUIRE(rBlock);

    // V1 placed at world (0,0): its "V" symbol's pin1/pin2 attach points
    // are local (0,0)/(20,0), so pin2 lands at world (20,0). R1 placed at
    // world (50,0): its pin1/pin2 attach points are local (0,0)/(20,0), so
    // pin2 lands at world (70,0). Tie both pin2s to GND via labels of the
    // same name (so R1's pin2 and V1's pin2 land on the same net without a
    // drawn wire, exercising the label-joins-nets mechanism).
    auto v1 = std::make_unique<InsertEntity>(doc.reserveEntityId(), doc.currentLayer(), vBlock, Point2D(0, 0));
    v1->setAttribute("REFDES", "V1");
    v1->setAttribute("VALUE", "9");
    const EntityId v1Id = v1->id();
    doc.addEntity(std::move(v1));

    auto r1 = std::make_unique<InsertEntity>(doc.reserveEntityId(), doc.currentLayer(), rBlock, Point2D(50, 0));
    r1->setAttribute("REFDES", "R1");
    r1->setAttribute("VALUE", "3k");
    const EntityId r1Id = r1->id();
    doc.addEntity(std::move(r1));

    // A resistor with no VALUE at all -- must be skipped, not crash or default silently.
    auto r2 = std::make_unique<InsertEntity>(doc.reserveEntityId(), doc.currentLayer(), rBlock, Point2D(100, 0));
    r2->setAttribute("REFDES", "R2");
    doc.addEntity(std::move(r2));

    // V1 pin1 (world (0,0)) -- wire -- R1 pin1 (world (50,0)).
    doc.addEntity(std::make_unique<WireEntity>(doc.reserveEntityId(), doc.currentLayer(),
                                               std::vector<Point2D>{Point2D(0, 0), Point2D(50, 0)}));

    // GND labels on V1's pin2 (world (0,-20)) and R1's pin2 (world (70,0)).
    doc.addEntity(std::make_unique<NetLabelEntity>(doc.reserveEntityId(), doc.currentLayer(), Point2D(20, 0), "GND"));
    doc.addEntity(std::make_unique<NetLabelEntity>(doc.reserveEntityId(), doc.currentLayer(), Point2D(70, 0), "GND"));

    const CircuitBuildResult built = buildCircuitFromDocument(doc);
    REQUIRE(built.circuit.elements.size() == 2); // R2 skipped (no VALUE)
    REQUIRE(built.skippedRefDes.size() == 1);
    REQUIRE(built.skippedRefDes[0] == "R2");

    const CircuitElement* vElem = nullptr;
    const CircuitElement* rElem = nullptr;
    for (const CircuitElement& e : built.circuit.elements) {
        if (e.name == "V1") vElem = &e;
        if (e.name == "R1") rElem = &e;
    }
    REQUIRE(vElem);
    REQUIRE(rElem);
    REQUIRE(vElem->value == Approx(9.0));
    REQUIRE(rElem->value == Approx(3000.0));
    REQUIRE(vElem->nodeN == 0); // V1 pin2 landed on GND (node 0)
    REQUIRE(rElem->nodeN == 0); // R1 pin2 landed on GND too, via the second label
    REQUIRE(vElem->nodeP == rElem->nodeP); // V1 pin1 and R1 pin1 share the wired net
    REQUIRE(vElem->nodeP != 0);

    // Solving it end-to-end: with R1's other end also grounded, R1 carries
    // no current from V1's perspective in this topology except through
    // itself to GND, i.e. this is just V1 across R1: node voltage = 9V.
    const OperatingPointResult op = solveDcOperatingPoint(built.circuit);
    REQUIRE(op.converged);
    REQUIRE(op.nodeVoltages[vElem->nodeP] == Approx(9.0));

    (void)v1Id;
    (void)r1Id;
}
