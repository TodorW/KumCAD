#pragma once

#include <string>

namespace lcad {

class Document;

// Real AutoCAD FIELD capability: a field template embeds {{...}}
// placeholders (this codebase's own deliberately simple syntax, not real
// AutoCAD's own much more elaborate %<\AcExpr ...>% field-expression
// language) that evaluateFieldTemplate resolves against doc's own live
// state. Supported placeholders:
//   {{DATE}}           -- today's date (YYYY-MM-DD)
//   {{FILENAME}}        -- the fileName argument, passed in by the caller
//                          since Document itself doesn't track its own
//                          associated file path
//   {{ATTR:refDes.tag}} -- a placed block INSERT's own attribute value
//                          (matched by REFDES, the same convention
//                          Ratsnest.h's own pad/net resolution uses), or
//                          "?" if no such INSERT/attribute exists
// An unrecognized or malformed placeholder is left in the output exactly
// as written (not silently dropped), so a typo stays visible instead of
// disappearing -- matching real AutoCAD's own "field code stays visible
// until it resolves" behavior for an as-yet-unresolvable field.
std::string evaluateFieldTemplate(const Document& doc, const std::string& templateText, const std::string& fileName = "");

} // namespace lcad
