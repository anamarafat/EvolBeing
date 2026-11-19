#include "Geometry2D.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
int main() {
    geom::Geometry2D a;
    auto g=a.createThread("Walls");
    assert(a.setThreadProperties(g,geom::GeometryType::Wall,true,0x336699));
    assert(a.setThreadThickness(g,3.5));
    auto p=a.createPolyline();
    auto v0=a.addVertex(0,0),v1=a.addVertex(2,0),v2=a.addVertex(2,2);
    a.appendVertexToPolyline(p,v0);a.appendVertexToPolyline(p,v1);a.appendVertexToPolyline(p,v2);
    assert(a.assignPolylineToThread(p,g));
    assert(a.effectivePolylineColor(p)==0x336699);
    assert(a.effectivePolylineThickness(p)==3.5);
    assert(a.setPolylineThicknessOverride(p,2.0));
    assert(a.effectivePolylineThickness(p)==2.0);
    assert(a.setPolylineThicknessOverride(p,std::nullopt));
    assert(a.effectivePolylineThickness(p)==3.5);
    geom::Geometry2D b;
    assert(b.importState(a.vertices(),a.edges(),a.polylines(),a.threads()));
    assert(b.threads().at(0).name=="Walls");
    assert(b.effectivePolylineThickness(p)==3.5);
    assert(b.polylines().at(p).edges==a.polylines().at(p).edges);
    auto badEdges=a.edges();badEdges.at(0).vertex0=9999;
    assert(!b.importState(a.vertices(),badEdges,a.polylines(),a.threads()));
    assert(b.polylines().size()==1); // failed import preserves prior state
    std::cout << "Stage4 core tests passed\n";
}
