#include "Geometry2D.hpp"
#include <cassert>
#include <iostream>

int main() {
    geom::Geometry2D g;
    auto group = g.createThread("walls");
    auto p = g.createPolyline();
    assert(g.assignPolylineToThread(p, group));
    auto a = g.addVertex(0,0), b = g.addVertex(1,0), c = g.addVertex(2,0), d = g.addVertex(3,0);
    for (auto v : {a,b,c,d}) g.appendVertexToPolyline(p,v);
    assert(g.validate().empty());
    auto removedEdge = g.polylines()[p].edges[1];
    assert(g.deleteEdge(removedEdge->id));
    assert(!g.isEdgeActive(removedEdge->id));
    assert(g.polylines()[p].edges.size() == 1);
    assert(g.polylines().size() == 2);
    assert(g.polylines()[1].edges.size() == 1);
    assert(g.polylines()[1].thread->id == group);
    assert(g.validate().empty());
    assert(g.deleteVertex(a));
    assert(!g.isVertexActive(a));
    assert(g.validate().empty());
    auto q = g.createPolyline();
    auto x = g.addVertex(0,1), y = g.addVertex(1,1), z = g.addVertex(1,2);
    for (auto v : {x,y,z}) g.appendVertexToPolyline(q,v);
    g.closePolyline(q);
    assert(g.validate().empty());
    auto closeEdge = g.polylines()[q].edges.back();
    assert(g.deleteEdge(closeEdge->id));
    assert(!g.polylines()[q].closed);
    assert(g.validate().empty());
    auto r = g.createPolyline();
    g.appendVertexToPolyline(r, g.addVertex(5,5));
    g.appendVertexToPolyline(r, g.addVertex(6,5));
    g.discardLastPolyline();
    assert(g.polylines()[r].deleted);
    assert(g.validate().empty());
    auto next = g.createPolyline();
    assert(next > r);
    std::cout << "Geometry2D tests passed\n";
}
