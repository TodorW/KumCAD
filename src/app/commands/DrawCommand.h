#pragma once

#include "core/geometry/Point2D.h"

#include <QString>

#include <optional>
#include <utility>
#include <vector>

// Interactive drawing command state machine, modeled on AutoCAD's prompt-driven
// command line: CommandDispatcher feeds picked points (from clicks or typed
// coordinates) and mouse-move previews; the command reports the next prompt
// text until it reports itself finished.
class DrawCommand {
public:
    virtual ~DrawCommand() = default;

    virtual QString start() = 0;
    virtual std::optional<QString> onPoint(const lcad::Point2D& pt) = 0;
    virtual void onPreviewPoint(const lcad::Point2D& pt) { (void)pt; }
    virtual std::vector<std::pair<lcad::Point2D, lcad::Point2D>> previewSegments() const { return {}; }

    // Enter/right-click with no point typed: try to finish with the points
    // collected so far. Returns true if the command finished successfully.
    virtual bool requestFinish() { return false; }

    virtual bool isFinished() const = 0;
    virtual void cancel() = 0;
};
