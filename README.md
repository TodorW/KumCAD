# KumCAD

A free, open-source 2D CAD application in the spirit of AutoCAD, built with C++20 and Qt 6.

KumCAD follows AutoCAD's conventions wherever possible — the command line drives everything, command names and aliases match (`L`, `C`, `TR`, `MI`, ...), and drawings are exchanged as DXF.

## Features

**Drawing** — LINE, CIRCLE, ARC (3-point), PLINE (with Close), RECTANG, ELLIPSE, TEXT, HATCH (solid fill), and linear/aligned DIMENSIONS with arrows and live preview.

**Editing** — MOVE, COPY, ROTATE, SCALE, MIRROR, OFFSET, TRIM, EXTEND, FILLET (tangent arc or sharp corner), ERASE, EXPLODE, and grip editing (drag endpoints, midpoints, centers, quadrants). Full undo/redo; multi-entity operations are single undo steps.

**Blocks** — BLOCK turns a selection into a reusable definition; INSERT places it anywhere with scale and rotation; EXPLODE breaks it back apart.

**Drafting aids** — object snap (endpoint/midpoint/center/quadrant, F3), ortho mode (F8), grid snap (F9), crossing/window selection, measurement commands (DIST, AREA).

**Organization** — layers with visibility, locking, and colors; per-entity color overrides (ByLayer default); a dockable Properties panel and Layers panel.

**Input/output** — DXF read/write (including ACI and true colors, old-style POLYLINE, blocks, hatches, and dimensions), plus fit-to-page printing and PDF export.

## Building

Requires CMake ≥ 3.21, a C++20 compiler, and Qt 6 (Widgets, OpenGLWidgets, PrintSupport).

```sh
cmake -B build
cmake --build build -j
./build/src/app/kumcad
```

Run the test suite (Catch2, fetched automatically):

```sh
ctest --test-dir build --output-on-failure
```

## Usage

Type a command in the command line at the bottom (or use the toolbar) and follow the prompts — points can be clicked in the canvas or typed as `x,y`. Enter/right-click finishes a command, Escape cancels. Select entities first for modify commands (MOVE, TRIM edges, etc.), exactly like AutoCAD's noun-verb workflow.

## Status

KumCAD is young and aims at AutoCAD LT-style 2D drafting. Not yet implemented: DWG format, splines, polyline arc segments (bulges), linetypes, paper space/layouts, and associative dimensions. Contributions welcome.
