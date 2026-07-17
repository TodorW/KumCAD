#include "core/document/Fields.h"

#include "core/document/Document.h"
#include "core/geometry/Insert.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace lcad {

namespace {

std::string resolveField(const Document& doc, const std::string& field, const std::string& fileName) {
    if (field == "DATE") {
        const auto now = std::chrono::system_clock::now();
        const std::time_t t = std::chrono::system_clock::to_time_t(now);
        const std::tm local = *std::localtime(&t);
        std::ostringstream oss;
        oss << std::put_time(&local, "%Y-%m-%d");
        return oss.str();
    }
    if (field == "FILENAME") return fileName;
    if (field.rfind("ATTR:", 0) == 0) {
        const std::string rest = field.substr(5);
        const auto dot = rest.find('.');
        if (dot == std::string::npos) return "{{" + field + "}}"; // malformed -- leave visible as-is
        const std::string refDes = rest.substr(0, dot);
        const std::string tag = rest.substr(dot + 1);
        for (const Entity* e : doc.entities()) {
            if (e->type() != EntityType::Insert) continue;
            const auto* insert = static_cast<const InsertEntity*>(e);
            const std::string* rd = insert->attributeValue("REFDES");
            if (rd && *rd == refDes) {
                const std::string* value = insert->attributeValue(tag);
                return value ? *value : "?";
            }
        }
        return "?";
    }
    return "{{" + field + "}}"; // unrecognized -- leave visible, not silently dropped
}

} // namespace

std::string evaluateFieldTemplate(const Document& doc, const std::string& templateText, const std::string& fileName) {
    std::string result;
    std::size_t i = 0;
    while (i < templateText.size()) {
        if (templateText.compare(i, 2, "{{") == 0) {
            const std::size_t close = templateText.find("}}", i + 2);
            if (close == std::string::npos) {
                result += templateText.substr(i); // unterminated -- leave the rest as-is
                break;
            }
            result += resolveField(doc, templateText.substr(i + 2, close - i - 2), fileName);
            i = close + 2;
        } else {
            result += templateText[i];
            ++i;
        }
    }
    return result;
}

} // namespace lcad
