#include "core/schematic/Bom.h"

#include "core/document/Document.h"
#include "core/geometry/Insert.h"
#include "core/geometry/Table.h"

#include <algorithm>
#include <map>

namespace lcad {

std::vector<BomRow> generateBom(const Document& doc) {
    // Keyed by (part, value); a group's own std::map keeps refDes
    // entries sorted as they're inserted, avoiding a second sort pass.
    std::map<std::pair<std::string, std::string>, std::vector<std::string>> groups;

    for (const Entity* e : doc.entities()) {
        if (e->type() != EntityType::Insert) continue;
        const auto* insert = static_cast<const InsertEntity*>(e);
        if (!insert->block() || !insert->block()->isSymbol()) continue;
        const std::string* refDes = insert->attributeValue("REFDES");
        if (!refDes || refDes->empty()) continue;

        const std::string* value = insert->attributeValue("VALUE");
        const std::pair<std::string, std::string> key{insert->block()->name, value ? *value : std::string()};
        groups[key].push_back(*refDes);
    }

    std::vector<BomRow> rows;
    for (auto& [key, refDesList] : groups) {
        std::sort(refDesList.begin(), refDesList.end());
        BomRow row;
        row.part = key.first;
        row.value = key.second;
        row.refDes = std::move(refDesList);
        row.quantity = static_cast<int>(row.refDes.size());
        rows.push_back(std::move(row));
    }
    return rows;
}

TableEntity* buildBomTable(Document& doc2d, const std::vector<BomRow>& rows, Point2D position) {
    const int rowCount = static_cast<int>(rows.size()) + 1;
    std::vector<double> rowHeights(static_cast<std::size_t>(rowCount), 4.0);
    std::vector<double> colWidths = {15.0, 15.0, 10.0, 40.0};
    std::vector<std::string> cells(static_cast<std::size_t>(rowCount) * 4);

    cells[0] = "Part";
    cells[1] = "Value";
    cells[2] = "Qty";
    cells[3] = "Ref Des";

    for (std::size_t i = 0; i < rows.size(); ++i) {
        const BomRow& row = rows[i];
        const std::size_t r = i + 1;
        cells[r * 4 + 0] = row.part;
        cells[r * 4 + 1] = row.value;
        cells[r * 4 + 2] = std::to_string(row.quantity);
        std::string refDesJoined;
        for (std::size_t j = 0; j < row.refDes.size(); ++j) {
            if (j > 0) refDesJoined += ", ";
            refDesJoined += row.refDes[j];
        }
        cells[r * 4 + 3] = refDesJoined;
    }

    auto table = std::make_unique<TableEntity>(doc2d.reserveEntityId(), doc2d.currentLayer(), position, rowHeights,
                                                colWidths, cells);
    TableEntity* raw = table.get();
    doc2d.addEntity(std::move(table));
    return raw;
}

} // namespace lcad
