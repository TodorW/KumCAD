#pragma once

#include "core/document/Document.h"
#include "core/pcb/Stackup.h"

#include <QMainWindow>

class Viewport3D;

// A read-only 3D view of a 2D PCB Document -- real KiCad gap this closes
// (see core/pcb/Board3D.h). Since a PCB Document has no dedicated board-
// outline entity yet, the substrate is inferred as the bounding box of
// every Track/Via/footprint pad, padded by a margin -- a real, disclosed
// simplification (not a user-drawn board edge).
class Board3DWindow : public QMainWindow {
    Q_OBJECT
public:
    Board3DWindow(const lcad::Document& doc, const lcad::CopperStackup& stackup, QWidget* parent = nullptr);

private:
    Viewport3D* m_viewport = nullptr;
};
