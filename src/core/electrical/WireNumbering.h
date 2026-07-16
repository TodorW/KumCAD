#pragma once

namespace lcad {

class Document;

// Assigns a sequential "W<n>" NetLabel at the midpoint of every wire that
// doesn't already have one touching it -- physical wire numbering for
// panel assembly/build sheets, distinct from (and independent of) whatever
// signal-name NetLabel a wire might also carry. Idempotent: existing "W<n>"
// numbers are never reassigned, and a wire that already has one is skipped,
// so it's safe to call after adding new wires to a panel that's already
// partly numbered.
void assignWireNumbers(Document& doc);

} // namespace lcad
