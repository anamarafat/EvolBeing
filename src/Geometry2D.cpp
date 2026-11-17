
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

        vertices_.push_back({id, x, y});

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

        edges_.push_back({id, v0, v1});

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

        if (vertexID >= vertices_.size())
            throw std::out_of_range(
                "Invalid geometry vertex ID.");

        auto& polyline = polylines_[polylineID];

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
        if (polylines_.empty())
            return;

        const auto& polyline = polylines_.back();

        // This initial implementation assumes the last
        // polyline owns the most recently added vertices
        // and edges, with no interleaved geometry creation.

        const std::size_t vertexCount =
            polyline.vertices.size();

        const std::size_t edgeCount =
            polyline.edges.size();

        if (vertexCount > vertices_.size() ||
            edgeCount > edges_.size())
        {
            throw std::logic_error(
                "Invalid polyline geometry state.");
        }

        vertices_.resize(vertices_.size() - vertexCount);
        edges_.resize(edges_.size() - edgeCount);

        polylines_.pop_back();
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
