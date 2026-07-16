#pragma once

#include <string>
#include <utility>
#include <vector>

namespace lcad {

// A minimal ZIP writer (store method only, no compression) with no external
// dependency -- eTransmit's transmittal package doesn't need compression,
// just a single file real CAD users can unzip. entries are (archive name,
// file content) pairs; archive names should already be unique (the caller's
// job, e.g. de-duplicating same-named dependencies from different folders).
// Returns false if the output file can't be written.
bool writeZip(const std::string& path, const std::vector<std::pair<std::string, std::string>>& entries);

// The matching reader for archives writeZip produced (store method only --
// this does not decompress DEFLATE, so it cannot open arbitrary third-party
// zips that used real compression). Reads local file headers directly
// rather than the central directory, since that's all writeZip's own output
// needs. Returns false (leaving entries untouched) if the file can't be
// read or doesn't look like a store-only zip.
bool readZip(const std::string& path, std::vector<std::pair<std::string, std::string>>& entries);

} // namespace lcad
