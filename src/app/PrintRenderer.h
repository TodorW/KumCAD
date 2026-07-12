#pragma once

class QPainter;
class QPrinter;

namespace lcad {
class Document;
struct Layout;
}

// Renders one page of document onto an already-active painter: layout's
// paper space (fit to the page, viewports clipped and scaled) if given,
// otherwise a fit-all view of model space. Takes QPainter rather than
// QPrinter so a caller can paint several documents' pages in the same
// print job (SheetSetPanel's Publish calls printer.newPage() between
// sheets, around one QPainter that stays open for the whole batch);
// resolutionDpi drives lineweight-to-pixel conversion the same way
// printer.resolution() would. Shared by MainWindow's Print/Export PDF
// (a single page from the live document) and Publish, so both use the
// same page-fit, lineweight, and plot-style-resolution logic.
void renderDocumentPage(QPainter& painter, double resolutionDpi, const lcad::Document& document,
                        const lcad::Layout* layout);

// Convenience for a single-page print job: opens its own painter on printer,
// paints, and closes it.
void renderDocumentPage(QPrinter& printer, const lcad::Document& document, const lcad::Layout* layout);
