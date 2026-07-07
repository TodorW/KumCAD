#include "core/document/Document.h"
#include "core/geometry/Line.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Document has a default layer", "[document]") {
    lcad::Document doc;
    REQUIRE(doc.layers().size() == 1);
    REQUIRE(doc.layers()[0].name == "0");
    REQUIRE(doc.currentLayer() == 0);
}

TEST_CASE("Document layer add/find", "[document]") {
    lcad::Document doc;
    const lcad::LayerId id = doc.addLayer("Walls", lcad::Color{255, 0, 0});
    REQUIRE(doc.layers().size() == 2);

    const lcad::Layer* layer = doc.findLayer(id);
    REQUIRE(layer != nullptr);
    REQUIRE(layer->name == "Walls");
    REQUIRE(doc.findLayer(999) == nullptr);
}

TEST_CASE("Document entity add/remove/find", "[document]") {
    lcad::Document doc;
    const lcad::EntityId id = doc.reserveEntityId();
    doc.addEntity(std::make_unique<lcad::LineEntity>(id, doc.currentLayer(), lcad::Point2D(0, 0), lcad::Point2D(1, 1)));

    REQUIRE(doc.entities().size() == 1);
    REQUIRE(doc.findEntity(id) != nullptr);

    auto removed = doc.removeEntity(id);
    REQUIRE(removed != nullptr);
    REQUIRE(doc.entities().empty());
    REQUIRE(doc.findEntity(id) == nullptr);
}
