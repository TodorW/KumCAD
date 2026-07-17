#pragma once

#include <QDialog>

#include <optional>
#include <utility>

class SketchView;
class QLabel;

// Phase 2 Sprint 2's sketch editor: draw lines/circles/arcs, select
// geometry, apply constraints, see the solver resolve live.
class SketchEditorDialog : public QDialog {
    Q_OBJECT
public:
    explicit SketchEditorDialog(QWidget* parent = nullptr);

    SketchView* view() const { return m_view; }

private:
    void applyHorizontal();
    void applyVertical();
    void applyParallel();
    void applyPerpendicular();
    void applyEqual();
    void applyTangent();
    void applyDistance();
    void applyRadius();
    void applyArcRadius();
    void applyCircleCircleTangent();
    void toggleConstruction();

    // Resolves the current selection to exactly one selected line's index,
    // or nullopt (with a status message) if that's not what's selected.
    std::optional<int> oneSelectedLine();
    std::optional<std::pair<int, int>> twoSelectedLines();
    // Either two selected points, or one selected line (using its own two
    // endpoints) -- both are valid ways to specify a Distance dimension.
    std::optional<std::pair<int, int>> twoPointsForDistance();
    std::optional<int> oneSelectedCircle();
    std::optional<std::pair<int, int>> lineAndCircle();
    std::optional<int> oneSelectedArc();
    std::optional<std::pair<int, int>> twoSelectedCircles();

    SketchView* m_view = nullptr;
    QLabel* m_statusLabel = nullptr;
};
