
#include "Geometry2D.hpp"
#include <unordered_set>

#include <cmath>
#include <limits>
#include <stdexcept>
#include <algorithm>
#include <utility>
#include <queue>

namespace geom
{
    GeometryVertexID Geometry2D::addVertex(
        double x,
        double y)
    {
        if (!std::isfinite(x) || !std::isfinite(y))
            throw std::invalid_argument(
                "Vertex coordinates must be finite.");

        if (vertices_.size() >=
            static_cast<std::size_t>(
                std::numeric_limits<GeometryVertexID>::max()))
        {
            throw std::overflow_error(
                "Too many geometry vertices.");
        }

        const auto id =
            static_cast<GeometryVertexID>(vertices_.size());

        vertices_.push_back({id, x, y, false});

        return id;
    }

    GeometryEdgeID Geometry2D::addEdge(
        GeometryVertexID v0,
        GeometryVertexID v1)
    {
        if (v0 >= vertices_.size() ||
            v1 >= vertices_.size())
        {
            throw std::out_of_range(
                "Invalid geometry vertex ID.");
        }

        if (!isVertexActive(v0) || !isVertexActive(v1))
            throw std::invalid_argument("Edge endpoint is deleted.");

        if (v0 == v1)
            throw std::invalid_argument(
                "An edge must connect distinct vertices.");

        if (edges_.size() >=
            static_cast<std::size_t>(
                std::numeric_limits<GeometryEdgeID>::max()))
        {
            throw std::overflow_error(
                "Too many geometry edges.");
        }

        const auto id =
            static_cast<GeometryEdgeID>(edges_.size());

        edges_.push_back({id, &vertices_[v0], &vertices_[v1], false});

        return id;
    }

    PolylineID Geometry2D::createPolyline()
    {
        if (polylines_.size() >=
            static_cast<std::size_t>(
                std::numeric_limits<PolylineID>::max()))
        {
            throw std::overflow_error(
                "Too many polylines.");
        }

        const auto id =
            static_cast<PolylineID>(polylines_.size());

        Polyline2D polyline;
        polyline.id = id;
        polyline.name = "Polyline_" + std::to_string(polyline.id);
        polylines_.push_back(std::move(polyline));

        return id;
    }

    void Geometry2D::appendVertexToPolyline(
        PolylineID polylineID,
        GeometryVertexID vertexID)
    {
        if (polylineID >= polylines_.size())
            throw std::out_of_range(
                "Invalid polyline ID.");

        if (!isVertexActive(vertexID))
            throw std::out_of_range(
                "Invalid geometry vertex ID.");

        auto& polyline = polylines_[polylineID];

        if (polyline.deleted)
            throw std::logic_error("Cannot append to a deleted polyline.");

        if (polyline.closed)
            throw std::logic_error(
                "Cannot append to a closed polyline.");

        if (!polyline.vertices.empty())
        {
            const auto previousVertex =
                polyline.vertices.back();

            // Create connecting edge
            const auto edgeID =
                addEdge(previousVertex->id, vertexID);

            polyline.edges.push_back(&edges_[edgeID]);
        }

        polyline.vertices.push_back(&vertices_[vertexID]);
    }

    void Geometry2D::closePolyline(PolylineID polylineID)
    {
        if (polylineID >= polylines_.size())
            throw std::out_of_range(
                "Invalid polyline ID.");

        auto& polyline = polylines_[polylineID];

        if (polyline.deleted)
            throw std::logic_error("Cannot close a deleted polyline.");

        if (polyline.closed)
            return;

        if (polyline.vertices.size() < 3)
            throw std::logic_error(
                "A closed polyline requires three vertices.");

        const auto first = polyline.vertices.front();
        const auto last = polyline.vertices.back();

        const auto edgeID = addEdge(last->id, first->id);

        polyline.edges.push_back(&edges_[edgeID]);
        polyline.closed = true;
    }

    void Geometry2D::discardLastPolyline()
    {
        if (polylines_.empty()) return;
        const auto id = polylines_.back().id;
        // Never pop: IDs already issued must not be reused.
        deletePolyline(id);
    }

    void Geometry2D::clear()
    {
        vertices_.clear();
        edges_.clear();
        polylines_.clear();
        threads_.clear();
        nextThreadID_ = 0;
    }

    const EntityStorage<GeometryVertex2D>&
        Geometry2D::vertices() const noexcept
    {
        return vertices_;
    }

    const EntityStorage<GeometryEdge2D>&
        Geometry2D::edges() const noexcept
    {
        return edges_;
    }

    const EntityStorage<Polyline2D>&
        Geometry2D::polylines() const noexcept
    {
        return polylines_;
    }

    std::size_t Geometry2D::vertexCount() const noexcept
    {
        return vertices_.size();
    }

    std::size_t Geometry2D::edgeCount() const noexcept
    {
        return edges_.size();
    }

    std::size_t Geometry2D::polylineCount() const noexcept
    {
        return polylines_.size();
    }


    GeometryThreadID Geometry2D::createThread(
        const std::string& name)
    {
        const GeometryThreadID id = nextThreadID_++;

        GeometryThread2D thread;
        thread.id = id;
        thread.name = name;

        threads_.push_back(std::move(thread));

        return id;
    }

    bool Geometry2D::assignPolylineToThread(
        PolylineID polylineID,
        std::optional<GeometryThreadID> threadID)
    {
        if (polylineID >= polylines_.size())
            return false;

        if (polylines_[polylineID].deleted)
            return false;

        GeometryThread2D* target = nullptr;

        if (threadID)
        {
            for (auto& thread : threads_)
            {
                if (thread.id == *threadID)
                {
                    target = &thread;
                    break;
                }
            }

            if (!target)
                return false;
        }

        auto& polyline = polylines_[polylineID];

        if (polyline.thread == target) return true;
        if (polyline.thread) {
            auto& members=polyline.thread->polylines;
            members.erase(std::remove(members.begin(),members.end(),&polyline),members.end());
        }
        polyline.thread=target;
        if(target) target->polylines.push_back(&polyline);
        return true;
    }

    bool Geometry2D::renamePolyline(
        PolylineID polylineID,
        const std::string& name)
    {
        if (polylineID >= polylines_.size())
            return false;
        if (polylines_[polylineID].deleted)
            return false;
        polylines_[polylineID].name = name;
        return true;
    }

    bool Geometry2D::renameThread(
        GeometryThreadID threadID,
        const std::string& name)
    {
        for (auto& thread : threads_)
        {
            if (thread.id == threadID)
            {
                thread.name = name;
                return true;
            }
        }

        return false;
    }

    bool Geometry2D::setThreadProperties(GeometryThreadID id, GeometryType type,
                                         bool active, std::uint32_t colorRGB)
    {
        if (colorRGB > 0xFFFFFF) return false;
        for (auto& group : threads_)
            if (group.id == id)
            {
                group.type = type;
                group.active = active;
                group.colorRGB = colorRGB;
                return true;
            }
        return false;
    }

    bool Geometry2D::setThreadThickness(GeometryThreadID id, double value)
    {
        if (!std::isfinite(value) || value < 0.1 || value > 50.0) return false;
        for (auto& g : threads_) if (g.id == id) { g.thickness = value; return true; }
        return false;
    }

    bool Geometry2D::setPolylineThicknessOverride(PolylineID id, std::optional<double> value)
    {
        if (id >= polylines_.size() || polylines_[id].deleted ||
            (value && (!std::isfinite(*value) || *value < 0.1 || *value > 50.0))) return false;
        polylines_[id].thicknessOverride = value;
        return true;
    }

    double Geometry2D::effectivePolylineThickness(PolylineID id) const noexcept
    {
        if (id >= polylines_.size()) return 1.0;
        const auto& poly = polylines_[id];
        if (poly.thicknessOverride) return *poly.thicknessOverride;
        if (poly.thread) return poly.thread->thickness;
        return 1.0;
    }

    bool Geometry2D::importState(std::vector<GeometryVertex2D> vertices,
                                  std::vector<GeometryEdgeRecord> edges,
                                  std::vector<PolylineRecord> polylines,
                                  std::vector<GeometryThreadRecord> threads)
    {
        for (std::size_t i=0; i<vertices.size(); ++i)
            if (vertices[i].id != i || !std::isfinite(vertices[i].x) || !std::isfinite(vertices[i].y)) return false;
        for (std::size_t i=0; i<edges.size(); ++i)
            if (edges[i].id != i || edges[i].vertex0 >= vertices.size() ||
                edges[i].vertex1 >= vertices.size() ||
                (!edges[i].deleted && (edges[i].vertex0 == edges[i].vertex1 ||
                vertices[edges[i].vertex0].deleted || vertices[edges[i].vertex1].deleted))) return false;
        std::unordered_set<GeometryThreadID> ids;
        GeometryThreadID next = 0;
        for (const auto& g : threads) {
            if (!ids.insert(g.id).second || g.colorRGB > 0xFFFFFF ||
                !std::isfinite(g.thickness) || g.thickness < 0.1 || g.thickness > 50.0 ||
                g.type > GeometryType::Region || g.id == std::numeric_limits<GeometryThreadID>::max()) return false;
            next = std::max(next, static_cast<GeometryThreadID>(g.id+1));
        }
        for (std::size_t i=0; i<polylines.size(); ++i) {
            const auto& poly=polylines[i];
            if (poly.id != i || (poly.threadID && !ids.contains(*poly.threadID)) ||
                (poly.colorOverrideRGB && *poly.colorOverrideRGB > 0xFFFFFF) ||
                (poly.thicknessOverride && (!std::isfinite(*poly.thicknessOverride) ||
                 *poly.thicknessOverride < 0.1 || *poly.thicknessOverride > 50.0))) return false;
            for (auto v : poly.vertices) if (v >= vertices.size() || (!poly.deleted && vertices[v].deleted)) return false;
            for (auto e : poly.edges) if (e >= edges.size() || (!poly.deleted && edges[e].deleted)) return false;
            if (!poly.deleted) {
                if (poly.vertices.size() < 2 || poly.edges.size() != poly.vertices.size() - (poly.closed ? 0 : 1)) return false;
                for (std::size_t j=0; j<poly.edges.size(); ++j) {
                    const auto& edge=edges[poly.edges[j]];
                    auto a=poly.vertices[j], b=poly.vertices[(j+1)%poly.vertices.size()];
                    if (!((edge.vertex0==a && edge.vertex1==b) || (edge.vertex1==a && edge.vertex0==b))) return false;
                }
            }
        }
        for (const auto& g : threads)
            for (auto id : g.polylines)
                if (id >= polylines.size() || polylines[id].deleted || polylines[id].threadID != g.id) return false;
        Geometry2D fresh;
        for (auto v : vertices) fresh.vertices_.push_back(v);
        for (auto e : edges) fresh.edges_.push_back({e.id, &fresh.vertices_[e.vertex0], &fresh.vertices_[e.vertex1], e.deleted});
        for (const auto& g : threads) fresh.threads_.push_back({g.id,g.name,g.type,g.active,g.colorRGB,g.thickness,{}});
        for (const auto& r : polylines) {
            Polyline2D p; p.id=r.id; p.name=r.name; p.closed=r.closed; p.deleted=r.deleted;
            p.colorOverrideRGB=r.colorOverrideRGB; p.thicknessOverride=r.thicknessOverride;
            for (auto v:r.vertices) p.vertices.push_back(&fresh.vertices_[v]);
            for (auto e:r.edges) p.edges.push_back(&fresh.edges_[e]);
            if (r.threadID) for (auto& g:fresh.threads_) if(g.id==*r.threadID) p.thread=&g;
            fresh.polylines_.push_back(std::move(p));
        }
        for (auto& p:fresh.polylines_) if(p.thread && !p.deleted) p.thread->polylines.push_back(&p);
        fresh.nextThreadID_=next;
        *this=std::move(fresh);
        return true;
    }

    Geometry2D::Geometry2D(const Geometry2D& o) {
        for (const auto& v:o.vertices_) vertices_.push_back(v);
        for (const auto& e:o.edges_) edges_.push_back({e.id,&vertices_[e.vertex0->id],&vertices_[e.vertex1->id],e.deleted});
        for (const auto& g:o.threads_) { auto c=g; c.polylines.clear(); threads_.push_back(std::move(c)); }
        for (const auto& p:o.polylines_) {
            auto c=p; for(auto& v:c.vertices) v=&vertices_[v->id]; for(auto& e:c.edges) e=&edges_[e->id];
            c.thread=nullptr; if(p.thread) for(auto& g:threads_) if(g.id==p.thread->id) c.thread=&g;
            polylines_.push_back(std::move(c));
        }
        for(auto& p:polylines_) if(p.thread && !p.deleted) p.thread->polylines.push_back(&p);
        nextThreadID_=o.nextThreadID_;
    }
    Geometry2D& Geometry2D::operator=(const Geometry2D& o) { if(this!=&o) { Geometry2D copy(o); *this=std::move(copy); } return *this; }
    std::vector<GeometryVertex2D> Geometry2D::vertexRecords() const { return {vertices_.begin(),vertices_.end()}; }
    std::vector<GeometryEdgeRecord> Geometry2D::edgeRecords() const { std::vector<GeometryEdgeRecord> r; for(const auto& e:edges_) r.push_back({e.id,e.vertex0->id,e.vertex1->id,e.deleted}); return r; }
    std::vector<PolylineRecord> Geometry2D::polylineRecords() const { std::vector<PolylineRecord> r; for(const auto& p:polylines_) { PolylineRecord c; c.id=p.id; c.name=p.name; c.closed=p.closed; c.deleted=p.deleted; c.colorOverrideRGB=p.colorOverrideRGB; c.thicknessOverride=p.thicknessOverride; if(p.thread)c.threadID=p.thread->id; for(auto v:p.vertices)c.vertices.push_back(v->id); for(auto e:p.edges)c.edges.push_back(e->id); r.push_back(std::move(c)); } return r; }
    std::vector<GeometryThreadRecord> Geometry2D::threadRecords() const { std::vector<GeometryThreadRecord> r; for(const auto& g:threads_) { GeometryThreadRecord c{g.id,g.name,g.type,g.active,g.colorRGB,g.thickness,{}}; for(auto p:g.polylines)c.polylines.push_back(p->id); r.push_back(std::move(c)); } return r; }

    bool Geometry2D::setPolylineColorOverride(PolylineID id,
                                                std::optional<std::uint32_t> colorRGB)
    {
        if (id >= polylines_.size() || polylines_[id].deleted ||
            (colorRGB && *colorRGB > 0xFFFFFF)) return false;
        polylines_[id].colorOverrideRGB = colorRGB;
        return true;
    }

    std::uint32_t Geometry2D::effectivePolylineColor(PolylineID id) const noexcept
    {
        constexpr std::uint32_t defaultColor = 0xDC2828;
        if (id >= polylines_.size()) return defaultColor;
        const auto& poly = polylines_[id];
        if (poly.colorOverrideRGB) return *poly.colorOverrideRGB;
        if (poly.thread) return poly.thread->colorRGB;
        return defaultColor;
    }

    const EntityStorage<GeometryThread2D>&
    Geometry2D::threads() const noexcept
    {
        return threads_;
    }


    bool Geometry2D::deletePolyline(PolylineID id)
    {
        if (id >= polylines_.size())
            return false;

        auto& polyline = polylines_[id];

        if (polyline.deleted)
            return false;

        if (polyline.thread) {
            auto& members=polyline.thread->polylines;
            members.erase(std::remove(members.begin(),members.end(),&polyline),members.end());
        }
        polyline.thread=nullptr;
        polyline.deleted = true;
        for (auto edgeID : polyline.edges)
        {
            bool shared = false;
            for (const auto& other : polylines_)
                if (!other.deleted && std::find(other.edges.begin(), other.edges.end(), edgeID) != other.edges.end())
                    { shared = true; break; }
            if (!shared) edgeID->deleted = true;
        }
        return true;
    }

    bool Geometry2D::deleteThread(GeometryThreadID id)
    {
        auto it = std::find_if(
            threads_.begin(),
            threads_.end(),
            [id](const GeometryThread2D& thread)
            {
                return thread.id == id;
            });

        if (it == threads_.end())
            return false;

        // Preserve geometry; move its parts to Unassigned.
        for (const auto polylineID : it->polylines)
        {
            polylineID->thread = nullptr;
        }

        threads_.erase(it);
        return true;
    }

}

namespace geom
{
    bool Geometry2D::isVertexActive(GeometryVertexID id) const noexcept
    {
        return id < vertices_.size() && !vertices_[id].deleted;
    }

    bool Geometry2D::isEdgeActive(GeometryEdgeID id) const noexcept
    {
        return id < edges_.size() && !edges_[id].deleted;
    }

    // Retire an orphan vertex only if no active edge or polyline refers to it.
    // Called after the affected polylines have been reconstructed.
    void Geometry2D::splitPolylineAtEdge(PolylineID id, std::size_t position)
    {
        const auto old = polylines_.at(id);
        if (position >= old.edges.size()) return;
        if (old.closed)
        {
            std::vector<GeometryVertex2D*> vs;
            std::vector<GeometryEdge2D*> es;
            for (std::size_t k = 0; k < old.vertices.size(); ++k)
                vs.push_back(old.vertices[(position + 1 + k) % old.vertices.size()]);
            for (std::size_t k = 1; k < old.edges.size(); ++k)
                es.push_back(old.edges[(position + k) % old.edges.size()]);
            auto& p = polylines_[id];
            p.vertices = std::move(vs);
            p.edges = std::move(es);
            p.closed = false;
            return;
        }
        const bool leftValid = position > 0;
        const bool rightValid = position + 1 < old.edges.size();
        if (!leftValid && !rightValid)
        {
            deletePolyline(id);
            return;
        }
        if (leftValid)
        {
            polylines_[id].vertices.assign(old.vertices.begin(), old.vertices.begin() + position + 1);
            polylines_[id].edges.assign(old.edges.begin(), old.edges.begin() + position);
        }
        else
        {
            polylines_[id].vertices.assign(old.vertices.begin() + position + 1, old.vertices.end());
            polylines_[id].edges.assign(old.edges.begin() + position + 1, old.edges.end());
        }
        if (leftValid && rightValid)
        {
            const auto newID = createPolyline();
            auto& other = polylines_[newID];
            other.vertices.assign(old.vertices.begin() + position + 1, old.vertices.end());
            other.edges.assign(old.edges.begin() + position + 1, old.edges.end());
            other.name = old.name + "_split_" + std::to_string(newID);
            if (old.thread) assignPolylineToThread(newID, old.thread->id);
        }
    }

    bool Geometry2D::deleteEdge(GeometryEdgeID id)
    {
        if (!isEdgeActive(id)) return false;
        const auto endpoint0 = edges_[id].vertex0;
        const auto endpoint1 = edges_[id].vertex1;
        const auto originalCount = polylines_.size();
        for (std::size_t p = 0; p < originalCount; ++p)
        {
            if (polylines_[p].deleted) continue;
            const auto& es = polylines_[p].edges;
            const auto it = std::find(es.begin(), es.end(), &edges_[id]);
            if (it != es.end())
                splitPolylineAtEdge(static_cast<PolylineID>(p),
                    static_cast<std::size_t>(it - es.begin()));
        }
        edges_[id].deleted = true;
        // Remove endpoints left without any references, not vertices still in a polyline.
        for (const auto v : {endpoint0, endpoint1})
        {
            bool used = false;
            for (const auto& e : edges_)
                if (!e.deleted && (e.vertex0 == v || e.vertex1 == v)) { used = true; break; }
            if (!used)
                for (const auto& poly : polylines_)
                    if (!poly.deleted && std::find(poly.vertices.begin(), poly.vertices.end(), v) != poly.vertices.end())
                        { used = true; break; }
            if (!used) v->deleted = true;
        }
        return true;
    }

    bool Geometry2D::deleteVertex(GeometryVertexID id)
    {
        if (!isVertexActive(id)) return false;
        // Snapshot all affected polylines before editing. Do NOT call deleteEdge()
        // for each incident edge: that produces intermediate singleton fragments.
        const auto originalCount = polylines_.size();
        for (std::size_t pi = 0; pi < originalCount; ++pi)
        {
            const auto old = polylines_[pi];
            if (old.deleted) continue;
            const auto it = std::find(old.vertices.begin(), old.vertices.end(), &vertices_[id]);
            if (it == old.vertices.end()) continue;
            const auto n = old.vertices.size();
            const auto at = static_cast<std::size_t>(it - old.vertices.begin());
            if (old.closed)
            {
                // A cycle minus one vertex is exactly one open chain.
                std::vector<GeometryVertex2D*> vs;
                std::vector<GeometryEdge2D*> es;
                for (std::size_t k = 1; k < n; ++k)
                    vs.push_back(old.vertices[(at + k) % n]);
                for (std::size_t k = 1; k + 1 < n; ++k)
                    es.push_back(old.edges[(at + k) % n]);
                if (es.empty()) deletePolyline(static_cast<PolylineID>(pi));
                else
                {
                    auto& poly = polylines_[pi];
                    poly.vertices = std::move(vs);
                    poly.edges = std::move(es);
                    poly.closed = false;
                }
            }
            else
            {
                // Left and right components; only components with edges survive.
                const bool left = at >= 2;
                const bool right = at + 2 < n;
                if (!left && !right)
                {
                    deletePolyline(static_cast<PolylineID>(pi));
                }
                else
                {
                    if (left)
                    {
                        polylines_[pi].vertices.assign(old.vertices.begin(), old.vertices.begin() + at);
                        polylines_[pi].edges.assign(old.edges.begin(), old.edges.begin() + at - 1);
                    }
                    else
                    {
                        polylines_[pi].vertices.assign(old.vertices.begin() + at + 1, old.vertices.end());
                        polylines_[pi].edges.assign(old.edges.begin() + at + 1, old.edges.end());
                    }
                    if (left && right)
                    {
                        const auto newID = createPolyline();
                        auto& other = polylines_[newID];
                        other.vertices.assign(old.vertices.begin() + at + 1, old.vertices.end());
                        other.edges.assign(old.edges.begin() + at + 1, old.edges.end());
                        other.name = old.name + "_split_" + std::to_string(newID);
                        if (old.thread) assignPolylineToThread(newID, old.thread->id);
                    }
                }
            }
        }
        // Retire incident edges that are no longer referenced by any active polyline.
        for (auto& edge : edges_)
        {
            if (edge.deleted || (edge.vertex0 != &vertices_[id] && edge.vertex1 != &vertices_[id])) continue;
            bool referenced = false;
            for (const auto& poly : polylines_)
                if (!poly.deleted && std::find(poly.edges.begin(), poly.edges.end(), &edge) != poly.edges.end())
                    { referenced = true; break; }
            if (!referenced) edge.deleted = true;
        }
        // A shared vertex may be used by several polylines; the requested vertex
        // is removed from every affected polyline above.
        vertices_[id].deleted = true;
        // Remove newly orphaned vertices, including isolated endpoints.
        for (auto& vertex : vertices_)
        {
            if (vertex.deleted) continue;
            bool used = false;
            for (const auto& e : edges_)
                if (!e.deleted && (e.vertex0 == &vertex || e.vertex1 == &vertex)) { used = true; break; }
            if (!used)
                for (const auto& poly : polylines_)
                    if (!poly.deleted && std::find(poly.vertices.begin(), poly.vertices.end(), &vertex) != poly.vertices.end())
                        { used = true; break; }
            if (!used) vertex.deleted = true;
        }
        return true;
    }

    bool Geometry2D::closePolylineReusingLastEdge(PolylineID id)
    {
        if (id >= polylines_.size()) return false;
        auto& poly = polylines_[id];
        if (poly.deleted || poly.closed || poly.vertices.size() < 4 ||
            poly.edges.size() + 1 != poly.vertices.size()) return false;
        const auto first = poly.vertices.front();
        const auto penultimate = poly.vertices[poly.vertices.size() - 2];
        const auto last = poly.vertices.back();
        if (first == penultimate) return false;
        const auto& a = *first;
        const auto& b = *penultimate;
        if (a.x == b.x && a.y == b.y) return false;
        const auto edgeID = poly.edges.back();
        if (!edgeID || edgeID->deleted) return false;
        auto& edge = *edgeID;
        // Avoid changing an edge shared by another active polyline.
        for (const auto& other : polylines_)
            if (!other.deleted && other.id != id &&
                std::find(other.edges.begin(), other.edges.end(), edgeID) != other.edges.end())
                return false;
        if (edge.vertex0 == last) edge.vertex0 = first;
        else if (edge.vertex1 == last) edge.vertex1 = first;
        else return false;
        poly.vertices.pop_back();
        poly.closed = true;
        bool referenced = false;
        for (const auto& e : edges_)
            if (!e.deleted && (e.vertex0 == last || e.vertex1 == last)) { referenced = true; break; }
        if (!referenced)
            for (const auto& other : polylines_)
                if (!other.deleted && std::find(other.vertices.begin(), other.vertices.end(), last) != other.vertices.end())
                    { referenced = true; break; }
        if (!referenced) last->deleted = true;
        return true;
    }

    std::vector<std::string> Geometry2D::validate() const
    {
        std::vector<std::string> errors;
        for (const auto& edge : edges_)
        {
            if (edge.deleted) continue;
            if (!edge.vertex0 || !edge.vertex1 || edge.vertex0->deleted || edge.vertex1->deleted || edge.vertex0 == edge.vertex1)
                errors.push_back("Invalid endpoints on edge " + std::to_string(edge.id));
            else if (edge.vertex0->x == edge.vertex1->x &&
                     edge.vertex0->y == edge.vertex1->y)
                errors.push_back("Zero-length edge " + std::to_string(edge.id));
        }
        for (const auto& p : polylines_)
        {
            if (p.deleted) continue;
            const auto expected = p.closed ? p.vertices.size() : (p.vertices.empty() ? 0 : p.vertices.size() - 1);
            if (p.edges.size() != expected)
                errors.push_back("Edge count mismatch on polyline " + std::to_string(p.id));
            if (p.closed && p.vertices.size() < 3)
                errors.push_back("Closed polyline has fewer than three vertices " + std::to_string(p.id));
            for (auto v : p.vertices)
                if (!v || v->deleted) errors.push_back("Invalid vertex reference in polyline " + std::to_string(p.id));
            for (std::size_t i = 0; i < p.edges.size(); ++i)
            {
                auto e = p.edges[i];
                if (!e || e->deleted) { errors.push_back("Invalid edge reference in polyline " + std::to_string(p.id)); continue; }
                if (p.vertices.empty()) continue;
                const auto a = p.vertices[i % p.vertices.size()];
                const auto b = p.vertices[(i + 1) % p.vertices.size()];
                const auto& edge = *e;
                if (!((edge.vertex0 == a && edge.vertex1 == b) || (edge.vertex0 == b && edge.vertex1 == a)))
                    errors.push_back("Connectivity mismatch in polyline " + std::to_string(p.id));
            }
            if (p.thread && std::find(p.thread->polylines.begin(),p.thread->polylines.end(),&p)==p.thread->polylines.end())
                errors.push_back("Invalid thread membership in polyline " + std::to_string(p.id));
        }
        return errors;
    }

    bool Geometry2D::moveVertex(GeometryVertexID id, double x, double y)
    {
        if (!isVertexActive(id) || !std::isfinite(x) || !std::isfinite(y)) return false;
        vertices_[id].x = x;
        vertices_[id].y = y;
        return true;
    }
}

/* Finalization of Geometry */
namespace geom 
{
    void FinalizedGeometry2D::clear() noexcept
    {
        // Remove loops before destroying their half-edges.
        loopThread_.loops.clear();

        polylineStatus_.clear();
        eligibleEdgeCount_.clear();
        endpointPolylines_.clear();
        
        // Remove non-owning connectivity.
        outgoingHalfEdges_.clear();
        edgePolylines_.clear();

        // Destroy the actual half-edge objects.
        halfEdges_.clear();
        edgeHalfEdges_.clear();

        // Clear previous diagnostics.
        errors_.clear();
    }

    void FinalizedGeometry2D::buildHalfEdges(const Geometry2D& geometry)
    {
        const auto& vertices = geometry.vertices();
        const auto& edges = geometry.edges();

        // Reserve storage for the smart pointers.
        halfEdges_.reserve(2 * edges.size());

        outgoingHalfEdges_.reserve(vertices.size());
        edgeHalfEdges_.reserve(edges.size());

        for (const auto& edge : edges)
        {
            if (edge.deleted)
                continue;

            // Retrieve the original geometry vertices.
            const auto* v0 = edge.vertex0;
            const auto* v1 = edge.vertex1;

            // Allocate two half-edges.
            auto h0 = std::make_unique<HalfEdge2D>();
            auto h1 = std::make_unique<HalfEdge2D>();

            // Set the origin vertices.
            h0->origin = v0;
            h1->origin = v1;

            // Both half-edges reference the same geometry edge.
            h0->parentEdge = &edge;
            h1->parentEdge = &edge;

            // Establish twin connectivity.
            h0->twin = h1.get();
            h1->twin = h0.get();

            edgeHalfEdges_.emplace(&edge, h0.get());

            // Register outgoing half-edges.
            outgoingHalfEdges_[v0].push_back(h0.get());
            outgoingHalfEdges_[v1].push_back(h1.get());

            // Transfer ownership to the finalized geometry.
            halfEdges_.push_back(std::move(h0));
            halfEdges_.push_back(std::move(h1));

            
        }
    }

    void FinalizedGeometry2D::buildEdgePolylineConnectivity(const Geometry2D& geometry)
    {
        const auto& edges = geometry.edges();
        const auto& polylines = geometry.polylines();

        edgePolylines_.reserve(edges.size());

        for (const auto& polyline : polylines)
        {
            if (polyline.deleted)
                continue;

            for (const auto edgeID : polyline.edges)
            {
                const auto& edge = *edgeID;

                if (edge.deleted)
                    continue;

                edgePolylines_[&edge].push_back(&polyline);
            }
        }
    }

    bool FinalizedGeometry2D::isEdgeEligible(
    const GeometryEdge2D* edge) const
    {
        const auto edgeIt = edgePolylines_.find(edge);

        /* Check if the edge is associated with any polylines */
        /* if edge key does not exist the .find would return end() iterator */
        if (edgeIt == edgePolylines_.end())
            return false;

        for (const auto* polyline : edgeIt->second)
        {
            const auto statusIt =
                polylineStatus_.find(polyline);

            if (statusIt == polylineStatus_.end())
                continue;

            if (statusIt->second != PolylineStatus::Open)
                return true;
        }

        return false;
    }

    void FinalizedGeometry2D::initializePolylineClassification(
                            const Geometry2D& geometry)
    {
        const auto& vertices = geometry.vertices();
        const auto& polylines = geometry.polylines();

        polylineStatus_.clear();
        eligibleEdgeCount_.clear();
        endpointPolylines_.clear();

        polylineStatus_.reserve(polylines.size());
        eligibleEdgeCount_.reserve(vertices.size());
        endpointPolylines_.reserve(vertices.size());

        // Initialize all valid polylines as candidates.
        for (const auto& polyline : polylines)
        {
            if (polyline.deleted)
                continue;

            if (polyline.edges.empty())
            {
                polylineStatus_[&polyline] =
                    PolylineStatus::Open;

                continue;
            }

            polylineStatus_[&polyline] =
                PolylineStatus::Candidate;

            const auto* startVertex =
                polyline.vertices.front();

            const auto* endVertex =
                polyline.vertices.back();

            endpointPolylines_[startVertex]
                .push_back(&polyline);

            if (startVertex != endVertex)
            {
                endpointPolylines_[endVertex]
                    .push_back(&polyline);
            }
        }

        // Count eligible physical edges at each vertex.
        for (const auto& [vertex, outgoing] : outgoingHalfEdges_)
        {
            std::size_t count = 0;
            for (const auto* halfEdge : outgoing)
            {
                if (isEdgeEligible(halfEdge->parentEdge))
                    ++count;
            }
            eligibleEdgeCount_[vertex] = count;
        }
    }

    bool FinalizedGeometry2D::hasOpenEndpoint(const Polyline2D& polyline) const
    {
        if (polyline.vertices.empty() ||
            polyline.edges.empty())
        {
            return true;
        }

        const auto* startVertex =
            polyline.vertices.front();

        const auto* endVertex =
            polyline.vertices.back();

        // An explicitly closed polyline is not
        // eliminated by endpoint pruning.
        if (polyline.closed ||
            startVertex == endVertex)
        {
            return false;
        }

        const auto startIt =
            eligibleEdgeCount_.find(startVertex);

        const auto endIt =
            eligibleEdgeCount_.find(endVertex);

        const std::size_t startCount =
            startIt != eligibleEdgeCount_.end()
            ? startIt->second
            : 0;

        const std::size_t endCount =
            endIt != eligibleEdgeCount_.end()
            ? endIt->second
            : 0;

        return startCount <= 1 || endCount <= 1;
    }

    void FinalizedGeometry2D::eliminateOpenPolylines(
    const Geometry2D& geometry)
    {
        const auto& polylines = geometry.polylines();

        std::queue<const Polyline2D*> pending;

        // ----------------------------------------
        // Stage 1: Identify initial open candidates
        // ----------------------------------------

        for (const auto& polyline : polylines)
        {
            if (polyline.deleted)
                continue;

            if (polylineStatus_.at(&polyline) !=
                PolylineStatus::Candidate)
            {
                continue;
            }

            if (hasOpenEndpoint(polyline))
            {
                pending.push(&polyline);
            }
        }

        // ----------------------------------------
        // Stage 2: Propagate open classification
        // ----------------------------------------

        while (!pending.empty())
        {
            const auto* polyline = pending.front();
            pending.pop();

            auto& status = polylineStatus_.at(polyline);

            // The same polyline may have been queued
            // from more than one neighbouring vertex.
            if (status != PolylineStatus::Candidate)
                continue;

            // Collect currently eligible physical edges
            // belonging to this polyline.
            std::unordered_set<const GeometryEdge2D*>
                affectedEdges;

            affectedEdges.reserve(polyline->edges.size());

            for (const auto edgeID : polyline->edges)
            {
                const auto* edge = edgeID;

                if (edge->deleted)
                    continue;

                if (isEdgeEligible(edge))
                    affectedEdges.insert(edge);
            }

            // Exclude the polyline from loop discovery.
            status = PolylineStatus::Open;

            // Update connectivity for affected edges.
            for (const auto* edge : affectedEdges)
            {
                // Another candidate polyline may still
                // reference this physical edge.
                if (isEdgeEligible(edge))
                    continue;

                const GeometryVertex2D* endpoints[2] =
                {
                    edge->vertex0,
                    edge->vertex1
                };

                for (const auto* vertex : endpoints)
                {
                    auto& count =
                        eligibleEdgeCount_.at(vertex);

                    if (count == 0)
                        continue;

                    --count;

                    // Only degree-one vertices require
                    // further open-polyline propagation.
                    if (count != 1)
                        continue;

                    const auto endpointIt =
                        endpointPolylines_.find(vertex);

                    if (endpointIt ==
                        endpointPolylines_.end())
                    {
                        continue;
                    }

                    for (const auto* adjacent :
                        endpointIt->second)
                    {
                        if (polylineStatus_.at(adjacent) !=
                            PolylineStatus::Candidate)
                        {
                            continue;
                        }

                        if (hasOpenEndpoint(
                                *adjacent))
                        {
                            pending.push(adjacent);
                        }
                    }
                }
            }
        }
    }

    HalfEdge2D* FinalizedGeometry2D::findDirectedHalfEdge(const GeometryEdge2D* edge,
                                                const GeometryVertex2D* origin) const
    {
        const auto it = edgeHalfEdges_.find(edge);

        if (it == edgeHalfEdges_.end())
            return nullptr;

        auto* halfEdge = it->second;

        if (halfEdge->origin == origin)
            return halfEdge;

        if (halfEdge->twin->origin == origin)
            return halfEdge->twin;

        return nullptr;
    }

    bool FinalizedGeometry2D::buildPolylinePath(const Polyline2D& polyline,
                                                const GeometryVertex2D* startVertex,
                                                std::vector<HalfEdge2D*>& path) const
    {

        if (polyline.deleted ||
            polyline.vertices.empty() ||
            polyline.edges.empty() ||
            startVertex == nullptr)
        {
            return false;
        }

        const auto* firstVertex =
            polyline.vertices.front();

        const auto* lastVertex =
            polyline.vertices.back();

        bool reverse = false;

        if (startVertex == firstVertex)
        {
            reverse = false;
        }
        else if (!polyline.closed &&
                startVertex == lastVertex)
        {
            reverse = true;
        }
        else
        {
            return false;
        }

        // Temporary storage for the ordered path.
        std::vector<HalfEdge2D*> localPath;
        localPath.reserve(polyline.edges.size());

        const auto* currentVertex = startVertex;
        const std::size_t nEdges = polyline.edges.size();

        for (std::size_t i = 0; i < nEdges; ++i)
        {
            const std::size_t edgeIndex =
                reverse ? nEdges - 1 - i : i;

            const auto* edge = polyline.edges[edgeIndex];

            if (edge == nullptr || edge->deleted)
                return false;

            auto* halfEdge = findDirectedHalfEdge(edge, currentVertex);
            if (halfEdge == nullptr)
                return false;

            localPath.push_back(halfEdge);

            // Advance to the destination vertex.
            currentVertex = halfEdge->twin->origin;
        }

        const auto* expectedEnd =
            polyline.closed
            ? startVertex
            : (reverse ? firstVertex : lastVertex);

        if (currentVertex != expectedEnd)
            return false;

        // Update the supplied path only after success.
        /* insert the local path into the supplied path end*/
        path.insert(path.end(),localPath.begin(),localPath.end());
        return true;
    }

    bool FinalizedGeometry2D::registerLoop(
        const std::vector<HalfEdge2D*>& path,
        const std::vector<const Polyline2D*>& polylines)
    {
        if (path.size() < 3 || polylines.empty())
            return false;

        std::unordered_set<const GeometryVertex2D*>visitedVertices;
        std::unordered_set<const GeometryEdge2D*>visitedEdges;

        visitedVertices.reserve(path.size());
        visitedEdges.reserve(path.size());

        // Validate all half-edges.
        for (const auto* halfEdge : path)
        {
            if (halfEdge == nullptr ||
                halfEdge->origin == nullptr ||
                halfEdge->parentEdge == nullptr ||
                halfEdge->twin == nullptr ||
                halfEdge->twin->origin == nullptr)
            {
                return false;
            }

            if (halfEdge->origin->deleted ||
                halfEdge->twin->origin->deleted ||
                halfEdge->parentEdge->deleted)
            {
                return false;
            }
        }

        // Validate continuity and repeated entities.
        for (std::size_t i = 0; i < path.size(); ++i)
        {
            const auto* current = path[i];

            const auto* next =
                path[(i + 1) % path.size()];

            if (current->twin->origin != next->origin)
                return false;

            if (!visitedVertices.insert(current->origin).second)
            {
                return false;
            }

            if (!visitedEdges.insert(
                    current->parentEdge).second)
            {
                return false;
            }
        }

        // Check contributing polylines.
        for (const auto* polyline : polylines)
        {
            if (polyline == nullptr || polyline->deleted)
                return false;

            const auto it = polylineStatus_.find(polyline);

            if (it == polylineStatus_.end() ||
                it->second == PolylineStatus::Open)
            {
                return false;
            }
        }

        // Check whether the same physical loop
        // has already been registered.
        for (const auto& existingLoop : loopThread_.loops)
        {
            if (existingLoop->halfEdges.size() != path.size())
                continue;

            bool sameLoop = true;

            for (const auto* halfEdge : existingLoop->halfEdges)
            {
                if (!visitedEdges.contains(
                        halfEdge->parentEdge))
                {
                    sameLoop = false;
                    break;
                }
            }

            if (sameLoop)
                return false;
        }

        // Allocate a new loop.
        auto loop = std::make_unique<GeometryLoop2D>();

        loop->name = "Loop " + std::to_string(loopThread_.loops.size() + 1);
        loop->halfEdges = path;
        loop->polylines = polylines;

        // Transfer ownership to the loop thread.
        loopThread_.loops.push_back(std::move(loop));

        // Update contributing polyline classifications.
        for (const auto* polyline : polylines)
        {
            polylineStatus_.at(polyline) =
                PolylineStatus::Closed;
        }

        return true;
    }

    void FinalizedGeometry2D::registerClosedPolylines(const Geometry2D& geometry)
    {
        for (const auto& polyline : geometry.polylines())
        {
            if (polyline.deleted)
                continue;

            if (!polyline.closed)
                continue;

            if (polylineStatus_.at(&polyline) != PolylineStatus::Candidate)
                continue;

            if (polyline.vertices.empty())
                continue;

            std::vector<HalfEdge2D*> path;
            const auto* startVertex = polyline.vertices.front();

            if (!buildPolylinePath( polyline, startVertex, path))
            {
                errors_.push_back("Invalid closed polyline: " + polyline.name);
                continue;
            }

            const std::vector<const Polyline2D*>
                contributingPolylines = {&polyline};

            registerLoop(path, contributingPolylines);
        }
    }

    bool FinalizedGeometry2D::extendLoopCandidate(LoopCandidate& candidate,
                                          const Polyline2D& polyline) const
    {
        if (polyline.deleted || polyline.closed)
            return false;

        const auto statusIt = polylineStatus_.find(&polyline);

        if (statusIt == polylineStatus_.end() ||
            statusIt->second == PolylineStatus::Open)
        {
            return false;
        }

        // Construct the directed path through
        // the complete polyline.
        std::vector<HalfEdge2D*> extension;

        if (!buildPolylinePath(polyline, candidate.currentVertex, extension))
            return false;

        if (extension.empty())
            return false;

        // Check the vertices along the extension.
        for (std::size_t i = 0;
            i < extension.size(); ++i)
        {
            const auto* halfEdge = extension[i];
            const auto* destination = halfEdge->twin->origin;

            // Returning to the starting vertex
            // is permitted only at the final edge.
            if (destination == candidate.startVertex)
            {
                if (i != extension.size() - 1)
                    return false;
                continue;
            }

            // Reject any other repeated vertex.
            if (!candidate.visitedVertices.insert(destination).second)
                return false;
        }

        // Append the successful extension.
        candidate.halfEdges.insert(candidate.halfEdges.end(),extension.begin(),
                                   extension.end());
        candidate.polylines.push_back(&polyline);
        candidate.currentVertex = extension.back()->twin->origin;

        return true;
    }

    bool FinalizedGeometry2D::searchLoopFromPolyline(const Polyline2D& seed)
    {
        if (seed.deleted || seed.closed || seed.vertices.empty())
            return false;

        if (polylineStatus_.at(&seed) != PolylineStatus::Candidate)
            return false;

        // ----------------------------------------
        // Stage 1: Initialize the seed candidate.
        // ----------------------------------------

        LoopCandidate initial;

        initial.startVertex = seed.vertices.front();
        initial.currentVertex = initial.startVertex;
        initial.visitedVertices.insert(initial.startVertex);

        if (!extendLoopCandidate(initial, seed))
            return false;

        std::queue<LoopCandidate> pending;
        pending.push(std::move(initial));

        // ----------------------------------------
        // Stage 2: Explore candidate routes.
        // ----------------------------------------

        while (!pending.empty())
        {
            LoopCandidate candidate = std::move(pending.front());
            pending.pop();

            // Check whether the candidate is closed.
            if (candidate.currentVertex == candidate.startVertex)
            {
                if (registerLoop(candidate.halfEdges, candidate.polylines))
                    return true;

                // Invalid or duplicate closure.
                // Continue investigating other routes.
                continue;
            }

            // Find polylines attached to the
            // current terminal vertex.
            const auto endpointIt = endpointPolylines_.find(candidate.currentVertex);
            if (endpointIt == endpointPolylines_.end())
                continue;

            // ------------------------------------
            // Stage 3: Explore eligible branches.
            // ------------------------------------
            for (const auto* adjacent : endpointIt->second)
            {
                if (adjacent->deleted || adjacent->closed)
                    continue;

                if (polylineStatus_.at(adjacent) == PolylineStatus::Open)
                    continue;

                // Do not reuse a polyline already
                // traversed by this candidate.
                if (std::find(candidate.polylines.begin(),
                              candidate.polylines.end(),
                              adjacent) != candidate.polylines.end())
                {
                    continue;
                }

                // Create an independent route.
                LoopCandidate nextCandidate = candidate;
                if (!extendLoopCandidate(nextCandidate, *adjacent))
                    continue;

                // Queue the successful extension.
                pending.push(std::move(nextCandidate));
            }
        }

        // All permitted routes exhausted.
        return false;
    }


    void FinalizedGeometry2D::discoverMultiPolylineLoops(
        const Geometry2D& geometry)
    {
        for (const auto& polyline :geometry.polylines())
        {
            if (polyline.deleted || polyline.closed)
                continue;

            auto& status = polylineStatus_.at(&polyline);

            // Ignore open polylines and those
            // already participating in a loop.
            if (status != PolylineStatus::Candidate)
                continue;

            // Explore all permitted routes until
            // a valid new loop is registered.
            if (!searchLoopFromPolyline(polyline))
            {
                status = PolylineStatus::Open;
            }
        }
    }

    double FinalizedGeometry2D::computeSignedArea(const GeometryLoop2D& loop) const
    {
        if (loop.halfEdges.size() < 3)
            return 0.0;

        const auto* referenceVertex = loop.halfEdges.front()->origin;
        const double xr = referenceVertex->x;
        const double yr = referenceVertex->y;

        double twiceArea = 0.0;

        for (const auto* halfEdge : loop.halfEdges)
        {
            const auto* v0 = halfEdge->origin;
            const auto* v1 = halfEdge->twin->origin;

            const double x0 = v0->x - xr;
            const double y0 = v0->y - yr;

            const double x1 = v1->x - xr;
            const double y1 = v1->y - yr;

            twiceArea += x0 * y1 - x1 * y0;
        }

        return 0.5 * twiceArea;
    }

    void FinalizedGeometry2D::reverseLoop(GeometryLoop2D& loop)
    {
        std::reverse(loop.halfEdges.begin(),loop.halfEdges.end());
        for (auto*& halfEdge : loop.halfEdges)
        {
            halfEdge = halfEdge->twin;
        }
        std::reverse(loop.polylines.begin(),loop.polylines.end());
    }

    void FinalizedGeometry2D::computeLoopProperties(GeometryLoop2D& loop)
    {
        if (loop.halfEdges.empty())
            return;

        const auto* firstVertex =
            loop.halfEdges.front()->origin;

        loop.xmin = firstVertex->x;
        loop.xmax = firstVertex->x;

        loop.ymin = firstVertex->y;
        loop.ymax = firstVertex->y;

        for (const auto* halfEdge : loop.halfEdges)
        {
            const auto* vertex = halfEdge->origin;

            loop.xmin = std::min(
                loop.xmin, vertex->x);

            loop.xmax = std::max(
                loop.xmax, vertex->x);

            loop.ymin = std::min(
                loop.ymin, vertex->y);

            loop.ymax = std::max(
                loop.ymax, vertex->y);
        }

        loop.signedArea = computeSignedArea(loop);
    }

    bool FinalizedGeometry2D::finalizeLoopProperties()
    {
        constexpr double relativeAreaTolerance = 1.0e-12;

        bool valid = true;

        for (auto& loopOwner : loopThread_.loops)
        {
            auto& loop = *loopOwner;

            computeLoopProperties(loop);

            const double dx =
                loop.xmax - loop.xmin;

            const double dy =
                loop.ymax - loop.ymin;

            const double lengthScale =
                std::max(dx, dy);

            const double areaTolerance =
                relativeAreaTolerance *
                lengthScale * lengthScale;

            if (!std::isfinite(loop.signedArea) ||
                !std::isfinite(areaTolerance) ||
                std::abs(loop.signedArea) <=
                    areaTolerance)
            {
                errors_.push_back(
                    "Degenerate or invalid area: " +
                    loop.name);

                valid = false;

                continue;
            }

            // Ensure counterclockwise orientation.
            if (loop.signedArea < 0.0)
            {
                reverseLoop(loop);

                loop.signedArea =
                    -loop.signedArea;
            }
        }

        return valid;
    }

    bool FinalizedGeometry2D::validateGeometry(const Geometry2D& geometry)
    {
        errors_ = geometry.validate();
        return errors_.empty();
    }

    bool FinalizedGeometry2D::finalize(const Geometry2D& geometry)
    {
        // Clear previous finalized topology.
        clear();

        // Validate original geometry.
        if (!validateGeometry(geometry))
            return false;

        // Construct half-edge topology.
        buildHalfEdges(geometry);

        // Construct edge-polyline connectivity.
        buildEdgePolylineConnectivity(geometry);

        // Initialize polyline classifications.
        initializePolylineClassification(geometry);

        // Recursively eliminate open polylines.
        eliminateOpenPolylines(geometry);

        // Register explicitly closed polylines.
        registerClosedPolylines(geometry);

        // Discover loops formed by multiple polylines.
        discoverMultiPolylineLoops(geometry);

        // Calculate properties and orient loops.
        const bool validLoops =
            finalizeLoopProperties();

        if (loopThread_.loops.empty())
        {
            errors_.push_back(
                "No closed geometry loops found.");

            return false;
        }

        return validLoops && errors_.empty();
    }



    PolylineStatus FinalizedGeometry2D::polylineStatus(
                        const Polyline2D& polyline) const
    {
        return polylineStatus_.at(&polyline);
    }
    
    /* following are few public acces funtions */
    /* Gui can access the address to the private members without modifying them ENCAPSULATION */
    const LoopThread2D& FinalizedGeometry2D::loopThread() const noexcept
    {
        return loopThread_;
    }       

    std::size_t FinalizedGeometry2D::halfEdgeCount() const noexcept
    {
        return halfEdges_.size();
    }

    std::size_t FinalizedGeometry2D::loopCount() const noexcept
    {
        return loopThread_.loops.size();
    }

    const std::vector<std::string>& FinalizedGeometry2D::errors() const noexcept
    {
        return errors_;
    }

}
