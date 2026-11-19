
#include "Geometry2D.hpp"

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

    void Geometry2D::splitPolylineAtEdge(PolylineID id, std::size_t position)
    {
        // Snapshot before createPolyline(), which may reallocate polylines_.
        const auto old = polylines_.at(id);
        if (position >= old.edges.size()) return;
        if (old.closed)
        {
            // Removing one edge opens the loop; rotate vertex and edge order.
            auto& p = polylines_[id];
            std::vector<GeometryVertexID> vs;
            std::vector<GeometryEdgeID> es;
            for (std::size_t k = 0; k < old.vertices.size(); ++k)
                vs.push_back(old.vertices[(position + 1 + k) % old.vertices.size()]);
            for (std::size_t k = 1; k < old.edges.size(); ++k)
                es.push_back(old.edges[(position + k) % old.edges.size()]);
            p.vertices = std::move(vs);
            p.edges = std::move(es);
            p.closed = false;
            return;
        }
        std::vector<GeometryVertexID> leftV(old.vertices.begin(), old.vertices.begin() + position + 1);
        std::vector<GeometryEdgeID> leftE(old.edges.begin(), old.edges.begin() + position);
        std::vector<GeometryVertexID> rightV(old.vertices.begin() + position + 1, old.vertices.end());
        std::vector<GeometryEdgeID> rightE(old.edges.begin() + position + 1, old.edges.end());
        polylines_[id].vertices = std::move(leftV);
        polylines_[id].edges = std::move(leftE);
        if (!rightE.empty())
        {
            auto newID = createPolyline();
            auto& other = polylines_[newID];
            other.vertices = std::move(rightV);
            other.edges = std::move(rightE);
            other.name = old.name + "_split_" + std::to_string(newID);
            if (old.threadID) assignPolylineToThread(newID, old.threadID);
        }
        // A single detached vertex is retained as standalone geometry.
    }

    bool Geometry2D::deleteEdge(GeometryEdgeID id)
    {
        if (!isEdgeActive(id)) return false;
        // An edge may be shared by multiple polylines.
        const std::size_t originalCount = polylines_.size();
        for (std::size_t p = 0; p < originalCount; ++p)
        {
            if (polylines_[p].deleted) continue;
            auto& edges = polylines_[p].edges;
            auto it = std::find(edges.begin(), edges.end(), id);
            if (it != edges.end())
                splitPolylineAtEdge(static_cast<PolylineID>(p),
                    static_cast<std::size_t>(it - edges.begin()));
        }
        edges_[id].deleted = true;
        return true;
    }

    bool Geometry2D::deleteVertex(GeometryVertexID id)
    {
        if (!isVertexActive(id)) return false;
        // Delete all incident edges, which splits/opens affected polylines.
        std::vector<GeometryEdgeID> incident;
        for (const auto& edge : edges_)
            if (!edge.deleted && (edge.vertex0 == id || edge.vertex1 == id))
                incident.push_back(edge.id);
        for (auto edgeID : incident) deleteEdge(edgeID);
        // Remove this vertex from any remaining singleton polyline.
        for (auto& p : polylines_)
        {
            if (p.deleted) continue;
            p.vertices.erase(std::remove(p.vertices.begin(), p.vertices.end(), id), p.vertices.end());
        }
        vertices_[id].deleted = true;
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
