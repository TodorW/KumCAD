#pragma once

#include "core/geometry/Entity.h"

#include <string>

namespace lcad {

// AutoCAD IMAGE (underlay): a raster file positioned by its bottom-left
// corner, sized in world units, with a rotation. core only carries the
// placement; loading and drawing the actual pixels is a Qt-side concern
// (EntityPainter). A single grip (the insertion point) moves the whole
// image, matching InsertEntity -- there's no per-corner resize grip.
class ImageEntity : public Entity {
public:
    ImageEntity(EntityId id, LayerId layer, std::string path, Point2D position, double width, double height,
               double rotationRadians = 0.0)
        : Entity(id, layer), m_path(std::move(path)), m_position(position), m_width(width), m_height(height),
          m_rotation(rotationRadians) {}

    const std::string& path() const { return m_path; }
    const Point2D& position() const { return m_position; }
    double width() const { return m_width; }
    double height() const { return m_height; }
    double rotation() const { return m_rotation; }

    EntityType type() const override { return EntityType::Image; }
    BoundingBox boundingBox() const override;
    double distanceTo(const Point2D& pt) const override;
    void translate(const Point2D& delta) override;
    void rotate(const Point2D& center, double angleRadians) override;
    void scale(const Point2D& center, double factor) override;
    // Approximated like InsertEntity's mirror: repositions the image but
    // doesn't actually flip the underlying pixels.
    void mirror(const Point2D& a, const Point2D& b) override;
    std::vector<Point2D> gripPoints() const override;
    void moveGripPoint(std::size_t index, const Point2D& newPos) override;
    std::vector<SnapPoint> snapCandidates() const override;
    std::unique_ptr<Entity> clone() const override;

private:
    std::string m_path;
    Point2D m_position; // bottom-left corner
    double m_width;
    double m_height;
    double m_rotation;
};

} // namespace lcad
