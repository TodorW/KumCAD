#pragma once

namespace lcad {

class Document;

// Registers a small built-in library of common schematic symbols
// (resistor, capacitor, diode, a generic 2-pin connector, and a generic
// 4-pin IC) into doc as named blocks with pins, skipping any name doc
// already has a block for -- idempotent, so it's safe to call on every new
// document. Body geometry is simplified line/circle art, not real
// standards-compliant (IEEE 315 / IEC 60617) symbol shapes.
void registerBuiltinSymbols(Document& doc);

} // namespace lcad
