#include "core/cam/GCodeWriter.h"

#include <fstream>

namespace lcad {

bool writeGCode(const std::vector<Point2D>& toolpath, const ToolpathParams& params, const std::string& path,
                std::string* errorOut) {
    if (toolpath.size() < 2) {
        if (errorOut) *errorOut = "Toolpath has fewer than 2 points";
        return false;
    }

    std::ofstream out(path, std::ios::binary);
    if (!out) {
        if (errorOut) *errorOut = "Could not open " + path + " for writing";
        return false;
    }

    out << "; KumCAD G-code export\n";
    out << "G21 ; millimeters\n";
    out << "G90 ; absolute positioning\n";
    out << "G0 Z" << params.safeHeight << "\n";
    out << "G0 X" << toolpath.front().x << " Y" << toolpath.front().y << "\n";
    out << "G1 Z" << -params.cutDepth << " F" << params.plungeRate << "\n";
    for (std::size_t i = 1; i < toolpath.size(); ++i) {
        out << "G1 X" << toolpath[i].x << " Y" << toolpath[i].y << " F" << params.feedRate << "\n";
    }
    out << "G0 Z" << params.safeHeight << "\n";
    out << "M30\n";
    return true;
}

} // namespace lcad
