#include "core/document/PartLibrary.h"

#include "core/io/DxfReader.h"
#include "core/io/DxfWriter.h"

namespace lcad {

namespace {

void copyBlockContent(BlockDefinition& dest, const BlockDefinition& source) {
    dest.entities.clear();
    dest.entities.reserve(source.entities.size());
    for (const auto& e : source.entities) dest.entities.push_back(e->clone());
    dest.pins = source.pins;
    dest.pads = source.pads;
    dest.dynamicParam = source.dynamicParam;
    dest.dynamicFlip = source.dynamicFlip;
    dest.dynamicRotation = source.dynamicRotation;
    dest.dynamicVisibility = source.dynamicVisibility;
    dest.dynamicArray = source.dynamicArray;
    dest.dynamicLookup = source.dynamicLookup;
}

} // namespace

bool writePartLibrary(const Document& doc, const std::vector<std::string>& blockNames, const std::string& path,
                     std::string* errorOut) {
    Document scratch;
    int matched = 0;
    for (const std::string& name : blockNames) {
        const BlockDefinition* source = doc.findBlock(name);
        if (!source) continue;
        scratch.addBlock(source->name, {});
        if (BlockDefinition* dest = scratch.findBlock(source->name)) copyBlockContent(*dest, *source);
        ++matched;
    }
    if (matched == 0) {
        if (errorOut) *errorOut = "No matching blocks to save";
        return false;
    }
    return writeDxf(scratch, path, errorOut);
}

bool readPartLibrary(Document& doc, const std::string& path, bool overwrite, std::vector<std::string>* addedNames,
                    std::string* errorOut) {
    Document scratch;
    if (!readDxf(scratch, path, errorOut)) return false;

    for (const auto& source : scratch.blocks()) {
        BlockDefinition* dest = doc.findBlock(source->name);
        if (dest) {
            if (!overwrite) continue;
            // Mutate the existing definition in place so any InsertEntity
            // already pointing at it (raw pointer, see Insert.h) picks up
            // the library's version immediately -- the same contract a
            // real BLOCK redefinition relies on.
            copyBlockContent(*dest, *source);
        } else {
            doc.addBlock(source->name, {});
            dest = doc.findBlock(source->name);
            if (dest) copyBlockContent(*dest, *source);
        }
        if (addedNames && dest) addedNames->push_back(source->name);
    }
    return true;
}

} // namespace lcad
