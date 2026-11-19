#include "Geometry2D.hpp"
#include <cassert>
#include <optional>
int main() {
    geom::Geometry2D geometry;
    const auto group = geometry.createThread("Walls");
    const auto poly = geometry.createPolyline();
    assert(geometry.assignPolylineToThread(poly, group));
    assert(geometry.setThreadProperties(group, geom::GeometryType::Wall, true, 0x12AB34));
    assert(geometry.effectivePolylineColor(poly) == 0x12AB34);
    assert(geometry.setPolylineColorOverride(poly, 0xFF00AA));
    assert(geometry.effectivePolylineColor(poly) == 0xFF00AA);
    assert(geometry.setThreadProperties(group, geom::GeometryType::Obstacle, false, 0x123456));
    assert(geometry.effectivePolylineColor(poly) == 0xFF00AA);
    assert(geometry.setPolylineColorOverride(poly, std::nullopt));
    assert(geometry.effectivePolylineColor(poly) == 0x123456);
    assert(!geometry.setPolylineColorOverride(poly, 0x1000000));
}
