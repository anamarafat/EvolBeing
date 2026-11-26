#include "Geometry2D.hpp"
#include <cassert>
int main() {
    geom::Geometry2D g;
    auto a=g.addVertex(0,0), b=g.addVertex(1,0), c=g.addVertex(2,0);
    auto p=g.createPolyline(), t=g.createThread("Wall");
    g.appendVertexToPolyline(p,a); g.appendVertexToPolyline(p,b); g.appendVertexToPolyline(p,c);
    assert(g.assignPolylineToThread(p,t));
    auto* v=g.polylines()[p].vertices.front();
    auto* e=g.polylines()[p].edges.front();
    auto* group=g.polylines()[p].thread;
    auto* poly=group->polylines.front();
    for(int i=0;i<2048;++i) { g.addVertex(i,1); g.createPolyline(); g.createThread("extra"); }
    assert(v==&g.vertices()[a] && e==&g.edges()[0] && poly==&g.polylines()[p] && group==&g.threads()[0]);
    geom::Geometry2D snapshot=g;
    assert(snapshot.polylines()[p].vertices.front()==&snapshot.vertices()[a]);
    assert(snapshot.polylines()[p].vertices.front()!=v);
    assert(snapshot.polylines()[p].edges.front()->vertex0==&snapshot.vertices()[a]);
    assert(snapshot.polylines()[p].thread->polylines.front()==&snapshot.polylines()[p]);
    assert(g.deleteVertex(a)); assert(v->deleted); assert(e->deleted);
    assert(!snapshot.vertices()[a].deleted && !snapshot.edges()[0].deleted);
    assert(g.validate().empty() && snapshot.validate().empty());
    assert(g.deleteThread(t)); assert(g.polylines()[p].thread==nullptr);
    g=std::move(snapshot); assert(g.validate().empty());
    assert(g.polylines()[p].vertices.front()==&g.vertices()[a]);
}
