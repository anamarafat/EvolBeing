#include "Geometry2D.hpp"
#include <cassert>
#include <iostream>
using namespace geom;
static PolylineID chain(Geometry2D& g, int n, bool closed=false) {
 auto p=g.createPolyline();
 for(int i=0;i<n;++i) g.appendVertexToPolyline(p,g.addVertex(double(i),double(i*i+1)));
 if(closed) g.closePolyline(p);
 return p;
}
static int active(const Geometry2D& g) { int n=0; for(const auto& p:g.polylines()) if(!p.deleted) ++n; return n; }
int main(){
 { Geometry2D g; auto p=chain(g,4,true); auto v=g.polylines()[p].vertices[1]; assert(g.deleteVertex(v->id)); assert(active(g)==1); assert(!g.polylines()[p].closed); assert(g.polylines()[p].edges.size()==2); assert(g.validate().empty()); }
 { Geometry2D g; auto p=chain(g,5); auto t=g.createThread("walls"); assert(g.assignPolylineToThread(p,t)); assert(g.renamePolyline(p,"Original")); auto v=g.polylines()[p].vertices[2]; assert(g.deleteVertex(v->id)); assert(active(g)==2); assert(g.polylines()[p].name=="Original"); assert(g.polylines()[1].name!="Original"); assert(g.polylines()[1].thread->id==t); assert(g.validate().empty()); }
 for(int index:{1,2}) { Geometry2D g; auto p=chain(g,4); auto isolated=g.polylines()[p].vertices[index==1?0:3]; auto v=g.polylines()[p].vertices[index]; assert(g.deleteVertex(v->id)); assert(active(g)==1); assert(!g.isVertexActive(isolated->id)); assert(g.polylines()[p].edges.size()==1); assert(g.validate().empty()); }
 { Geometry2D g; auto p=chain(g,4); auto oldLast=g.polylines()[p].vertices.back(); auto oldEdge=g.polylines()[p].edges.back(); assert(g.closePolylineReusingLastEdge(p)); assert(active(g)==1); assert(g.polylines()[p].closed); assert(g.polylines()[p].vertices.size()==3); assert(g.polylines()[p].edges.back()==oldEdge); assert(!g.isVertexActive(oldLast->id)); assert(g.validate().empty()); }
 { Geometry2D g; auto p=chain(g,3); assert(!g.closePolylineReusingLastEdge(p)); assert(!g.polylines()[p].closed); assert(g.validate().empty()); }
 { Geometry2D g; auto p=chain(g,4); auto edge=g.polylines()[p].edges[1]; assert(g.deleteEdge(edge->id)); assert(active(g)==2); assert(g.validate().empty()); }
 { Geometry2D g; auto p=chain(g,3); auto edge=g.polylines()[p].edges.front(); assert(g.deleteEdge(edge->id)); assert(active(g)==1); assert(g.validate().empty()); }
 std::cout << "Geometry2D correction tests passed\n";
}
