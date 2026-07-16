#include "core/civil/SurveyPoints.h"

#include <fstream>
#include <sstream>

namespace lcad {

namespace {

bool parseXyzLine(std::string line, SurveyPoint& out) {
    const auto first = line.find_first_not_of(" \t\r");
    if (first == std::string::npos || line[first] == '#') return false;
    for (char& c : line) {
        if (c == ',') c = ' ';
    }
    std::istringstream iss(line);
    double x = 0.0, y = 0.0, z = 0.0;
    if (!(iss >> x >> y >> z)) return false; // z is required here, unlike the point-cloud reader
    out = SurveyPoint{Point2D(x, y), z};
    return true;
}

} // namespace

std::vector<SurveyPoint> readSurveyPointsXyz(const std::string& path) {
    std::ifstream in(path);
    if (!in) return {};

    std::vector<SurveyPoint> points;
    std::string line;
    SurveyPoint tmp;
    while (std::getline(in, line)) {
        if (parseXyzLine(line, tmp)) points.push_back(tmp);
    }
    return points;
}

} // namespace lcad
