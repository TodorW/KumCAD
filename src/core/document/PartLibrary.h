#pragma once

#include "core/document/Document.h"

#include <string>
#include <vector>

namespace lcad {

// A part library file: literally a DXF file whose model space is empty
// and whose BLOCKS table holds the named parts -- not a new bespoke
// format, but direct reuse of writeDxf/readDxf's own existing block
// persistence (pins/pads/dynamic block parameters already round-trip
// through a BLOCK entry's custom group codes, see DxfWriter.cpp). A
// genuine plus over inventing a format: a saved library is itself a
// normal drawing file, openable in this app (or any DXF viewer, for the
// body geometry at least) to inspect the parts visually. This is NOT a
// real KiCad .kicad_sym/.kicad_mod library and isn't intended to
// interoperate with one.

// Writes every block in blockNames (silently skipping any name doc has
// no block for) to path as a library. Returns false (with *errorOut set)
// on a write failure, or if none of blockNames matched an existing block.
bool writePartLibrary(const Document& doc, const std::vector<std::string>& blockNames, const std::string& path,
                     std::string* errorOut = nullptr);

// Reads path and merges its blocks into doc. A name doc already has a
// block for is skipped unless overwrite is true, in which case that
// block's geometry/pins/pads/dynamic parameters are replaced in place
// (existing INSERTs of it keep pointing at the same BlockDefinition,
// now showing the library's version -- the same "definition mutated in
// place" contract BLOCK's own redefinition already relies on elsewhere
// in this codebase). addedNames (if non-null) receives the names
// actually added or overwritten. Returns false (with *errorOut set) on
// a read failure.
bool readPartLibrary(Document& doc, const std::string& path, bool overwrite = false,
                    std::vector<std::string>* addedNames = nullptr, std::string* errorOut = nullptr);

} // namespace lcad
