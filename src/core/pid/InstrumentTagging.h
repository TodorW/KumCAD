#pragma once

namespace lcad {

class Document;

// Assigns a sequential "INST-<n>" REFDES attribute to every placed
// INSTRUMENT symbol (see PidLibrary.h) that doesn't already have a REFDES
// -- a simplified stand-in for real ISA-5.1 tagging (e.g. "FT-101"), which
// needs a per-instance function-letter choice this doesn't collect.
// Idempotent: an instrument that already has a REFDES is left alone, and
// existing "INST-<n>" numbers are never reused, so new instruments added
// later number up from the highest one already placed.
void assignInstrumentTags(Document& doc);

} // namespace lcad
