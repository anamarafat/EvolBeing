
#include "Geometry2D.hpp"
#include <unordered_set>

#include <cmath>
#include <limits>
#include <stdexcept>
#include <algorithm>
#include <utility>

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

        edges_.push_back({id, v0, v1, false});

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
                addEdge(previousVertex, vertexID);

            polyline.edges.push_back(edgeID);
        }

        polyline.vertices.push_back(vertexID);
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

        const auto edgeID = addEdge(last, first);

        polyline.edges.push_back(edgeID);
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

    const std::vector<GeometryVertex2D>&
        Geometry2D::vertices() const noexcept
    {
        return vertices_;
    }

    const std::vector<GeometryEdge2D>&
        Geometry2D::edges() const noexcept
    {
        return edges_;
    }

    const std::vector<Polyline2D>&
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

        if (polyline.threadID == threadID)
            return true;

        // Remove from the previous thread.
        if (polyline.threadID)
        {
            for (auto& thread : threads_)
            {
                if (thread.id == *polyline.threadID)
                {
                    auto& ids = thread.polylines;

                    ids.erase(
                        std::remove(
                            ids.begin(), ids.end(), polylineID),
                        ids.end());

                    break;
                }
            }
        }

        polyline.threadID = threadID;

        if (target)
            target->polylines.push_back(polylineID);

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
        if (poly.threadID)
            for (const auto& g : threads_) if (g.id == *poly.threadID) return g.thickness;
        return 1.0;
    }

    bool Geometry2D::importState(std::vector<GeometryVertex2D> vertices,
                                  std::vector<GeometryEdge2D> edges,
                                  std::vector<Polyline2D> polylines,
                                  std::vector<GeometryThread2D> threads)
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
        vertices_ = std::move(vertices); edges_ = std::move(edges);
        polylines_ = std::move(polylines); threads_ = std::move(threads);
        nextThreadID_ = next;
        return true;
    }

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
        if (poly.threadID)
            for (const auto& group : threads_)
                if (group.id == *poly.threadID) return group.colorRGB;
        return defaultColor;
    }

    const std::vector<GeometryThread2D>&
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

        // Remove membership from the assigned thread.
        if (polyline.threadID)
        {
            for (auto& thread : threads_)
            {
                if (thread.id != *polyline.threadID)
                    continue;

                auto& ids = thread.polylines;

                ids.erase(
                    std::remove(ids.begin(), ids.end(), id),
                    ids.end());

                break;
            }
        }

        polyline.threadID.reset();
        polyline.deleted = true;
        for (auto edgeID : polyline.edges)
        {
            bool shared = false;
            for (const auto& other : polylines_)
                if (!other.deleted && std::find(other.edges.begin(), other.edges.end(), edgeID) != other.edges.end())
                    { shared = true; break; }
            if (!shared && edgeID < edges_.size()) edges_[edgeID].deleted = true;
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
            if (polylineID < polylines_.size())
                polylines_[polylineID].threadID.reset();
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
            std::vector<GeometryVertexID> vs;
            std::vector<GeometryEdgeID> es;
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
            if (old.threadID) assignPolylineToThread(newID, old.threadID);
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
            const auto it = std::find(es.begin(), es.end(), id);
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
            if (!used) vertices_[v].deleted = true;
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
            const auto it = std::find(old.vertices.begin(), old.vertices.end(), id);
            if (it == old.vertices.end()) continue;
            const auto n = old.vertices.size();
            const auto at = static_cast<std::size_t>(it - old.vertices.begin());
            if (old.closed)
            {
                // A cycle minus one vertex is exactly one open chain.
                std::vector<GeometryVertexID> vs;
                std::vector<GeometryEdgeID> es;
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
                        if (old.threadID) assignPolylineToThread(newID, old.threadID);
                    }
                }
            }
        }
        // Retire incident edges that are no longer referenced by any active polyline.
        for (auto& edge : edges_)
        {
            if (edge.deleted || (edge.vertex0 != id && edge.vertex1 != id)) continue;
            bool referenced = false;
            for (const auto& poly : polylines_)
                if (!poly.deleted && std::find(poly.edges.begin(), poly.edges.end(), edge.id) != poly.edges.end())
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
                if (!e.deleted && (e.vertex0 == vertex.id || e.vertex1 == vertex.id)) { used = true; break; }
            if (!used)
                for (const auto& poly : polylines_)
                    if (!poly.deleted && std::find(poly.vertices.begin(), poly.vertices.end(), vertex.id) != poly.vertices.end())
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
        const auto& a = vertices_.at(first);
        const auto& b = vertices_.at(penultimate);
        if (a.x == b.x && a.y == b.y) return false;
        const auto edgeID = poly.edges.back();
        if (!isEdgeActive(edgeID)) return false;
        auto& edge = edges_[edgeID];
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
        if (!referenced) vertices_[last].deleted = true;
        return true;
    }

    std::vector<std::string> Geometry2D::validate() const
    {
        std::vector<std::string> errors;
        for (const auto& edge : edges_)
        {
            if (edge.deleted) continue;
            if (!isVertexActive(edge.vertex0) || !isVertexActive(edge.vertex1) || edge.vertex0 == edge.vertex1)
                errors.push_back("Invalid endpoints on edge " + std::to_string(edge.id));
            else if (vertices_[edge.vertex0].x == vertices_[edge.vertex1].x &&
                     vertices_[edge.vertex0].y == vertices_[edge.vertex1].y)
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
                if (!isVertexActive(v)) errors.push_back("Invalid vertex reference in polyline " + std::to_string(p.id));
            for (std::size_t i = 0; i < p.edges.size(); ++i)
            {
                auto e = p.edges[i];
                if (!isEdgeActive(e)) { errors.push_back("Invalid edge reference in polyline " + std::to_string(p.id)); continue; }
                if (p.vertices.empty()) continue;
                const auto a = p.vertices[i % p.vertices.size()];
                const auto b = p.vertices[(i + 1) % p.vertices.size()];
                const auto& edge = edges_[e];
                if (!((edge.vertex0 == a && edge.vertex1 == b) || (edge.vertex0 == b && edge.vertex1 == a)))
                    errors.push_back("Connectivity mismatch in polyline " + std::to_string(p.id));
            }
            if (p.threadID)
            {
                auto it = std::find_if(threads_.begin(), threads_.end(), [&](const auto& t) { return t.id == *p.threadID; });
                if (it == threads_.end() || std::find(it->polylines.begin(), it->polylines.end(), p.id) == it->polylines.end())
                    errors.push_back("Invalid thread membership in polyline " + std::to_string(p.id));
            }
        }
        return errors;
    }
}

namespace geom {
bool Geometry2D::moveVertex(GeometryVertexID id, double x, double y)
{
    if (!isVertexActive(id) || !std::isfinite(x) || !std::isfinite(y)) return false;
    vertices_[id].x = x;
    vertices_[id].y = y;
    return true;
}
}
