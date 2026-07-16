#include "core/pid/InstrumentTagging.h"

#include "core/document/Document.h"
#include "core/geometry/Insert.h"

#include <algorithm>
#include <cctype>

namespace lcad {

namespace {

// "INST-<digits>" parsed as digits, or nullopt otherwise.
std::optional<int> instrumentNumberOf(const std::string& refdes) {
    constexpr const char* prefix = "INST-";
    if (refdes.rfind(prefix, 0) != 0) return std::nullopt;
    const std::string digits = refdes.substr(5);
    if (digits.empty()) return std::nullopt;
    for (char c : digits) {
        if (!std::isdigit(static_cast<unsigned char>(c))) return std::nullopt;
    }
    return std::stoi(digits);
}

} // namespace

void assignInstrumentTags(Document& doc) {
    int nextNumber = 1;
    for (const Entity* e : doc.entities()) {
        if (e->type() != EntityType::Insert) continue;
        const auto* insert = static_cast<const InsertEntity*>(e);
        if (const std::string* refdes = insert->attributeValue("REFDES")) {
            if (const auto n = instrumentNumberOf(*refdes)) nextNumber = std::max(nextNumber, *n + 1);
        }
    }

    for (Entity* e : doc.entities()) {
        if (e->type() != EntityType::Insert) continue;
        auto* insert = static_cast<InsertEntity*>(e);
        if (!insert->block() || insert->blockName() != "INSTRUMENT") continue;
        if (insert->attributeValue("REFDES")) continue; // already tagged
        insert->setAttribute("REFDES", "INST-" + std::to_string(nextNumber++));
    }
}

} // namespace lcad
