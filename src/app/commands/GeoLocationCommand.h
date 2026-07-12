#pragma once

#include "commands/DrawCommand.h"
#include "core/document/Document.h"

// AutoCAD GEOGRAPHICLOCATION (simplified): picks a design point, then
// latitude, longitude, and the rotation from world +Y to true north
// (Enter defaults to 0, i.e. +Y is north). No coordinate system picker or
// map/imagery display -- just the reference metadata AutoCAD's own dialog
// also collects, which is what geotagging exported files needs.
class GeoLocationCommand : public DrawCommand {
public:
    explicit GeoLocationCommand(lcad::Document& document) : m_document(document) {}

    QString start() override { return QStringLiteral("GEOGRAPHICLOCATION  Specify design point:"); }
    std::optional<QString> onPoint(const lcad::Point2D& pt) override;
    std::optional<QString> onScalar(double value) override;
    bool requestFinish() override;
    std::optional<QString> resultMessage() const override { return m_result; }
    bool isFinished() const override { return m_finished; }
    void cancel() override { m_finished = true; }

private:
    enum class Stage { Point, Latitude, Longitude, North };

    void commit();

    lcad::Document& m_document;
    Stage m_stage = Stage::Point;
    lcad::GeoLocation m_geo;
    std::optional<QString> m_result;
    bool m_finished = false;
};
