#include "core/geometry/MText.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace lcad {

namespace {

constexpr double kCharAspect = 0.6; // approximate glyph width / height

double clampedAxisDistance(double p, double lo, double hi) {
    if (p < lo) return lo - p;
    if (p > hi) return p - hi;
    return 0.0;
}

} // namespace

std::string decodeMTextContent(const std::string& raw) {
    std::string out;
    out.reserve(raw.size());
    for (std::size_t i = 0; i < raw.size(); ++i) {
        const char c = raw[i];
        if (c == '{' || c == '}') continue; // formatting group braces
        if (c != '\\') {
            out.push_back(c);
            continue;
        }
        if (i + 1 >= raw.size()) break;
        const char code = raw[++i];
        switch (code) {
        case 'P':
        case 'p':
            out.push_back('\n');
            break;
        case '~':
            out.push_back(' ');
            break;
        case '\\':
        case '{':
        case '}':
            out.push_back(code);
            break;
        // Inline formatting with a parameter, e.g. \fArial|b0;, \H2.5;, \C1;:
        // skip through the terminating semicolon.
        case 'f':
        case 'F':
        case 'H':
        case 'h':
        case 'C':
        case 'c':
        case 'T':
        case 'Q':
        case 'W':
        case 'A':
        case 'S': { // stacked fractions keep their text, lose the markup
            std::string param;
            while (i + 1 < raw.size() && raw[i + 1] != ';') param.push_back(raw[++i]);
            if (i + 1 < raw.size()) ++i; // consume ';'
            if (code == 'S') {
                for (char pc : param) out.push_back(pc == '^' || pc == '#' ? '/' : pc);
            }
            break;
        }
        default:
            break; // unknown single-letter code: drop it
        }
    }
    return out;
}

std::string encodeMTextContent(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (char c : text) {
        if (c == '\n') out += "\\P";
        else if (c == '\\') out += "\\\\";
        else if (c == '{') out += "\\{";
        else if (c == '}') out += "\\}";
        else out.push_back(c);
    }
    return out;
}

std::vector<std::string> MTextEntity::wrappedLines() const {
    std::vector<std::string> lines;
    const double charWidth = kCharAspect * m_height;
    const std::size_t maxChars =
        m_width > 1e-9 && charWidth > 1e-12 ? std::max<std::size_t>(1, static_cast<std::size_t>(m_width / charWidth))
                                            : std::string::npos;

    std::istringstream stream(m_text);
    std::string paragraph;
    while (std::getline(stream, paragraph)) {
        if (maxChars == std::string::npos || paragraph.size() <= maxChars) {
            lines.push_back(paragraph);
            continue;
        }
        // Greedy word wrap; a single overlong word gets hard-broken.
        std::string current;
        std::istringstream words(paragraph);
        std::string word;
        while (words >> word) {
            while (word.size() > maxChars) {
                if (!current.empty()) {
                    lines.push_back(current);
                    current.clear();
                }
                lines.push_back(word.substr(0, maxChars));
                word = word.substr(maxChars);
            }
            const std::size_t needed = current.empty() ? word.size() : current.size() + 1 + word.size();
            if (needed > maxChars && !current.empty()) {
                lines.push_back(current);
                current = word;
            } else {
                if (!current.empty()) current += ' ';
                current += word;
            }
        }
        lines.push_back(current);
    }
    if (lines.empty()) lines.push_back("");
    return lines;
}

double MTextEntity::blockWidth() const {
    double maxChars = 0;
    for (const std::string& line : wrappedLines()) maxChars = std::max(maxChars, static_cast<double>(line.size()));
    return kCharAspect * m_height * maxChars;
}

double MTextEntity::blockHeight() const {
    return static_cast<double>(wrappedLines().size()) * lineAdvance();
}

BoundingBox MTextEntity::boundingBox() const {
    const double w = blockWidth();
    const double h = blockHeight();
    BoundingBox box;
    // The block extends rightward and DOWN from the top-left insertion point.
    box.expand(rotateAround(m_position, m_position, m_rotation));
    box.expand(rotateAround(Point2D(m_position.x + w, m_position.y), m_position, m_rotation));
    box.expand(rotateAround(Point2D(m_position.x, m_position.y - h), m_position, m_rotation));
    box.expand(rotateAround(Point2D(m_position.x + w, m_position.y - h), m_position, m_rotation));
    return box;
}

double MTextEntity::distanceTo(const Point2D& pt) const {
    const Point2D local = rotateAround(pt, m_position, -m_rotation) - m_position;
    const double dx = clampedAxisDistance(local.x, 0.0, blockWidth());
    const double dy = clampedAxisDistance(local.y, -blockHeight(), 0.0);
    return std::sqrt(dx * dx + dy * dy);
}

void MTextEntity::translate(const Point2D& delta) {
    m_position = m_position + delta;
}

void MTextEntity::rotate(const Point2D& center, double angleRadians) {
    m_position = rotateAround(m_position, center, angleRadians);
    m_rotation += angleRadians;
}

void MTextEntity::scale(const Point2D& center, double factor) {
    m_position = scaleAround(m_position, center, factor);
    m_height *= factor;
    m_width *= factor;
}

void MTextEntity::mirror(const Point2D& a, const Point2D& b) {
    // Keep the text readable (MIRRTEXT=0), same as TextEntity.
    m_position = mirrorAcross(m_position, a, b);
    const double phi = std::atan2(b.y - a.y, b.x - a.x);
    double reflected = 2.0 * phi - m_rotation;
    reflected = std::fmod(reflected, 2.0 * M_PI);
    if (reflected < 0) reflected += 2.0 * M_PI;
    if (reflected > M_PI / 2 && reflected <= 3 * M_PI / 2) reflected += M_PI;
    m_rotation = reflected;
}

std::vector<Point2D> MTextEntity::gripPoints() const {
    return {m_position};
}

void MTextEntity::moveGripPoint(std::size_t index, const Point2D& newPos) {
    if (index == 0) m_position = newPos;
}

std::vector<SnapPoint> MTextEntity::snapCandidates() const {
    return {{m_position, SnapKind::Endpoint}};
}

std::unique_ptr<Entity> MTextEntity::clone() const {
    return std::make_unique<MTextEntity>(*this);
}

} // namespace lcad
