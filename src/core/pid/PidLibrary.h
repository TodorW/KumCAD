#pragma once

namespace lcad {

class Document;

// Registers a small built-in library of ISA-5.1-flavored P&ID symbols
// (control valve, pump, vessel/tank, generic instrument bubble) into doc as
// named blocks with pins, skipping any name doc already has a block for --
// idempotent, like registerBuiltinSymbols/registerElectricalSymbols, which
// this coexists with in the same document's block table. Body geometry is
// simplified, not real ISA-5.1 line-weight/shape-standard art. Pin/Wire
// connectivity (core/schematic/Netlist.h) works identically for process and
// signal lines -- there's no visual distinction between them yet (a real
// P&ID draws signal lines dashed); this is a disclosed simplification, not
// a different connectivity model.
void registerPidSymbols(Document& doc);

} // namespace lcad
