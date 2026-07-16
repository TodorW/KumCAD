#include "core/electrical/WireList.h"

#include "core/document/Document.h"
#include "core/geometry/Insert.h"
#include "core/geometry/Table.h"

namespace lcad {

namespace {

std::string refDesOf(const Document& doc, EntityId insertId) {
    const Entity* e = doc.findEntity(insertId);
    if (e && e->type() == EntityType::Insert) {
        const auto* insert = static_cast<const InsertEntity*>(e);
        if (const std::string* refdes = insert->attributeValue("REFDES"); refdes && !refdes->empty()) return *refdes;
    }
    return "U" + std::to_string(insertId);
}

} // namespace

TableEntity* buildWireListTable(Document& doc, const std::vector<Net>& nets, Point2D position) {
    int rowCount = 1;
    for (const Net& net : nets) rowCount += static_cast<int>(net.pins.size());

    std::vector<double> rowHeights(static_cast<std::size_t>(rowCount), 4.0);
    std::vector<double> colWidths = {30.0, 20.0, 15.0};
    std::vector<std::string> cells(static_cast<std::size_t>(rowCount) * 3);

    cells[0] = "Net";
    cells[1] = "RefDes";
    cells[2] = "Pin";

    int row = 1;
    for (const Net& net : nets) {
        for (const NetPin& pin : net.pins) {
            cells[static_cast<std::size_t>(row) * 3 + 0] = net.name;
            cells[static_cast<std::size_t>(row) * 3 + 1] = refDesOf(doc, pin.insertId);
            cells[static_cast<std::size_t>(row) * 3 + 2] = pin.pinNumber;
            ++row;
        }
    }

    auto table = std::make_unique<TableEntity>(doc.reserveEntityId(), doc.currentLayer(), position, rowHeights,
                                               colWidths, cells);
    TableEntity* raw = table.get();
    doc.addEntity(std::move(table));
    return raw;
}

} // namespace lcad
