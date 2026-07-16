#include "core/civil/Alignment.h"

namespace lcad {

std::vector<Point2D> computeGroundProfile(const std::vector<Point2D>& alignment, const Tin& tin, double interval) {
    std::vector<Point2D> profile;
    if (alignment.size() < 2 || interval <= 1e-9) return profile;

    std::vector<double> segLengths(alignment.size() - 1);
    double totalLength = 0.0;
    for (std::size_t i = 0; i + 1 < alignment.size(); ++i) {
        segLengths[i] = alignment[i].distanceTo(alignment[i + 1]);
        totalLength += segLengths[i];
    }

    for (double station = 0.0; station <= totalLength + 1e-9; station += interval) {
        double remaining = station;
        Point2D sample = alignment.back();
        for (std::size_t i = 0; i + 1 < alignment.size(); ++i) {
            if (remaining <= segLengths[i] || i + 2 == alignment.size()) {
                const double segLen = segLengths[i];
                const double t = segLen < 1e-12 ? 0.0 : std::min(1.0, remaining / segLen);
                sample = alignment[i] + (alignment[i + 1] - alignment[i]) * t;
                break;
            }
            remaining -= segLengths[i];
        }

        if (const auto z = elevationAt(tin, sample)) profile.emplace_back(station, *z);
    }

    return profile;
}

} // namespace lcad
