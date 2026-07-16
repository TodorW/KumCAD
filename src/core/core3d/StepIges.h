#pragma once

#include "core/core3d/Document3D.h"

#include <TopoDS_Shape.hxx>

#include <string>

namespace lcad {

// STEP/IGES interchange (Sprint 4). Export writes every "tip" shape in the
// document -- a feature that's valid and not itself consumed as another
// feature's inputA/inputB -- as one shape each in the output file. Most
// simple parts have exactly one tip (the end of a single boolean/feature
// chain); a document with several independent, never-combined solids
// exports all of them, which is the useful and expected behavior (matching
// how a real STEP file can hold more than one body).
bool writeStep(const Document3D& doc, const std::string& path);
bool writeIges(const Document3D& doc, const std::string& path);

// Import reads the whole file into a single shape (a compound, if the file
// held more than one top-level body) -- the caller then wraps it in a
// FeatureType::Imported feature via Document3D::addImportedShape. Returns a
// null shape (IsNull() true) on any read failure.
TopoDS_Shape readStep(const std::string& path);
TopoDS_Shape readIges(const std::string& path);

} // namespace lcad
