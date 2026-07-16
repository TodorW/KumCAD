#pragma once

namespace lcad {

class Document;

// Registers a small built-in library of common electrical/panel symbols
// (contactor, relay, terminal block, three-phase motor) into doc as named
// blocks with pins, skipping any name doc already has a block for --
// idempotent, like registerBuiltinSymbols (core/schematic/SymbolLibrary.h),
// which this complements rather than replaces (both can coexist in the
// same document's block table). Body geometry is simplified line/circle
// art, not real standards-compliant (IEC 60617) symbol shapes.
void registerElectricalSymbols(Document& doc);

} // namespace lcad
