#include "commands/GeoLocationCommand.h"

#include <cmath>

std::optional<QString> GeoLocationCommand::onPoint(const lcad::Point2D& pt) {
    if (m_stage != Stage::Point) return std::nullopt;
    m_geo.designPoint = pt;
    m_stage = Stage::Latitude;
    return QStringLiteral("Enter latitude (degrees, + north):");
}

std::optional<QString> GeoLocationCommand::onScalar(double value) {
    switch (m_stage) {
    case Stage::Latitude:
        if (value < -90.0 || value > 90.0) return QStringLiteral("*Latitude must be -90..90*");
        m_geo.latitude = value;
        m_stage = Stage::Longitude;
        return QStringLiteral("Enter longitude (degrees, + east):");
    case Stage::Longitude:
        if (value < -180.0 || value > 180.0) return QStringLiteral("*Longitude must be -180..180*");
        m_geo.longitude = value;
        m_stage = Stage::North;
        return QStringLiteral("Enter north direction angle (degrees from +Y) <0>:");
    case Stage::North:
        m_geo.northRotation = value * M_PI / 180.0;
        commit();
        return m_result;
    default:
        return std::nullopt;
    }
}

bool GeoLocationCommand::requestFinish() {
    if (m_stage == Stage::North) {
        commit();
        return true;
    }
    m_finished = true;
    return false;
}

void GeoLocationCommand::commit() {
    m_document.setGeoLocation(m_geo);
    m_finished = true;
    m_result = QStringLiteral("*Geographic location set: %1, %2*").arg(m_geo.latitude).arg(m_geo.longitude);
}
