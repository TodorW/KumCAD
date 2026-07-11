#include "commands/ArrayCommand.h"

#include "core/document/Commands.h"

#include <cmath>

QString ArrayCommand::start() {
    return QStringLiteral("ARRAY  %1 selected. Enter array type [Rectangular/Polar] <R>:")
        .arg(static_cast<int>(m_ids.size()));
}

std::optional<QString> ArrayCommand::onOption(const QString& option) {
    if (m_stage != Stage::Type) return std::nullopt;
    const QString opt = option.toUpper();
    if (opt == QLatin1String("R") || opt == QLatin1String("RECTANGULAR")) {
        m_stage = Stage::Rows;
        return QStringLiteral("Enter number of rows <2>:");
    }
    if (opt == QLatin1String("P") || opt == QLatin1String("POLAR")) {
        m_stage = Stage::Center;
        return QStringLiteral("Specify center point of array:");
    }
    return std::nullopt;
}

std::optional<QString> ArrayCommand::onScalar(double value) {
    switch (m_stage) {
    case Stage::Rows:
        if (value < 1 || value > 1000) return QStringLiteral("*Rows must be 1-1000*");
        m_rows = static_cast<int>(value);
        m_stage = Stage::Columns;
        return QStringLiteral("Enter number of columns <2>:");
    case Stage::Columns:
        if (value < 1 || value > 1000) return QStringLiteral("*Columns must be 1-1000*");
        if (m_rows * static_cast<int>(value) > 10000) return QStringLiteral("*Array too large (max 10000 cells)*");
        m_columns = static_cast<int>(value);
        m_stage = m_rows > 1 ? Stage::RowSpacing : Stage::ColumnSpacing;
        return m_rows > 1 ? QStringLiteral("Enter row spacing:") : QStringLiteral("Enter column spacing:");
    case Stage::RowSpacing:
        if (std::abs(value) < 1e-9) return QStringLiteral("*Spacing cannot be zero*");
        m_rowSpacing = value;
        if (m_columns > 1) {
            m_stage = Stage::ColumnSpacing;
            return QStringLiteral("Enter column spacing:");
        }
        m_finished = true;
        return commitRectangular();
    case Stage::ColumnSpacing:
        if (std::abs(value) < 1e-9) return QStringLiteral("*Spacing cannot be zero*");
        m_columnSpacing = value;
        m_finished = true;
        return commitRectangular();
    case Stage::Count:
        if (value < 2 || value > 1000) return QStringLiteral("*Items must be 2-1000*");
        m_count = static_cast<int>(value);
        m_stage = Stage::FillAngle;
        return QStringLiteral("Enter angle to fill (+=CCW) <360>:");
    case Stage::FillAngle:
        if (std::abs(value) < 1e-6 || std::abs(value) > 360.0) return QStringLiteral("*Angle must be within 360*");
        m_fillDeg = value;
        m_finished = true;
        return commitPolar();
    default:
        return std::nullopt;
    }
}

std::optional<QString> ArrayCommand::onPoint(const lcad::Point2D& pt) {
    if (m_stage != Stage::Center) return std::nullopt;
    m_center = pt;
    m_stage = Stage::Count;
    return QStringLiteral("Enter number of items <4>:");
}

QString ArrayCommand::commitRectangular() {
    auto batch = std::make_unique<lcad::BatchCommand>("Array");
    int made = 0;
    for (int r = 0; r < m_rows; ++r) {
        for (int c = 0; c < m_columns; ++c) {
            if (r == 0 && c == 0) continue; // the original stays put
            for (lcad::EntityId id : m_ids) {
                const lcad::Entity* source = m_document.findEntity(id);
                if (!source) continue;
                auto copy = source->clone();
                copy->setId(m_document.reserveEntityId());
                copy->translate(lcad::Point2D(c * m_columnSpacing, r * m_rowSpacing));
                batch->add(std::make_unique<lcad::AddEntityCommand>(m_document, std::move(copy)));
                ++made;
            }
        }
    }
    if (!batch->empty()) m_document.commandStack().execute(std::move(batch));
    return QStringLiteral("*%1 copies placed (%2 x %3)*").arg(made).arg(m_rows).arg(m_columns);
}

QString ArrayCommand::commitPolar() {
    auto batch = std::make_unique<lcad::BatchCommand>("Array");
    // A full 360 fill spaces items over n slots (the last would coincide with
    // the first); a partial fill spreads them across n-1 steps to span it.
    const bool full = std::abs(std::abs(m_fillDeg) - 360.0) < 1e-9;
    const double step = (m_fillDeg * M_PI / 180.0) / (full ? m_count : m_count - 1);
    int made = 0;
    for (int i = 1; i < m_count; ++i) {
        for (lcad::EntityId id : m_ids) {
            const lcad::Entity* source = m_document.findEntity(id);
            if (!source) continue;
            auto copy = source->clone();
            copy->setId(m_document.reserveEntityId());
            copy->rotate(m_center, step * i);
            batch->add(std::make_unique<lcad::AddEntityCommand>(m_document, std::move(copy)));
            ++made;
        }
    }
    if (!batch->empty()) m_document.commandStack().execute(std::move(batch));
    return QStringLiteral("*%1 copies placed around the center*").arg(made);
}
