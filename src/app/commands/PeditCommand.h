#pragma once

#include "commands/DrawCommand.h"
#include "core/document/Document.h"

#include <vector>

// PEDIT over the current selection:
//   Close/Open  - toggle the closed flag of selected polylines
//   Join        - fuse selected touching lines/arcs/polylines into one
//                 polyline (arcs become bulged segments)
//   Decurve     - straighten all arc segments (zero the bulges)
// Each option applies immediately and ends the command, one undo step.
class PeditCommand : public DrawCommand {
public:
    PeditCommand(lcad::Document& document, std::vector<lcad::EntityId> ids)
        : m_document(document), m_ids(std::move(ids)) {}

    QString start() override;
    std::optional<QString> onPoint(const lcad::Point2D& pt) override;
    std::optional<QString> onOption(const QString& option) override;
    bool isFinished() const override { return m_finished; }
    void cancel() override { m_finished = true; }

private:
    QString setClosed(bool closed);
    QString join();
    QString decurve();

    lcad::Document& m_document;
    std::vector<lcad::EntityId> m_ids;
    bool m_finished = false;
};
