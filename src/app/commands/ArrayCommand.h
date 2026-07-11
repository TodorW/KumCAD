#pragma once

#include "commands/DrawCommand.h"
#include "core/document/Document.h"

#include <vector>

// ARRAY over the current selection, classic prompt-driven form:
//   Rectangular: rows, columns, row spacing, column spacing
//   Polar: center point, item count, fill angle (items are rotated with the
//          array, like AutoCAD's default)
// All copies land as one undo step.
class ArrayCommand : public DrawCommand {
public:
    ArrayCommand(lcad::Document& document, std::vector<lcad::EntityId> ids)
        : m_document(document), m_ids(std::move(ids)) {}

    QString start() override;
    std::optional<QString> onPoint(const lcad::Point2D& pt) override;
    std::optional<QString> onScalar(double value) override;
    std::optional<QString> onOption(const QString& option) override;
    bool isFinished() const override { return m_finished; }
    void cancel() override { m_finished = true; }

private:
    enum class Stage { Type, Rows, Columns, RowSpacing, ColumnSpacing, Center, Count, FillAngle };

    QString commitRectangular();
    QString commitPolar();

    lcad::Document& m_document;
    std::vector<lcad::EntityId> m_ids;
    Stage m_stage = Stage::Type;
    int m_rows = 2;
    int m_columns = 2;
    double m_rowSpacing = 0.0;
    double m_columnSpacing = 0.0;
    lcad::Point2D m_center;
    int m_count = 4;
    double m_fillDeg = 360.0;
    bool m_finished = false;
};
