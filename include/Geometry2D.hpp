#pragma once

#include <cstdint>
#include <vector>
#include <cstddef>
#include <string>
#include <optional>
#include <unordered_map>

namespace geom
{
    using GeometryVertexID = std::uint32_t;
    using GeometryEdgeID = std::uint32_t;
    using PolylineID = std::uint32_t;
    using GeometryThreadID = std::uint32_t;

    enum class GeometryType : std::uint8_t
    {
        Unassigned,
        Wall,
        Entrance,
        Exit,
        Obstacle,
        Region
    };

    struct GeometryThread2D
    {
        GeometryThreadID id = 0;
        std::string name;
        GeometryType type = GeometryType::Unassigned;
        std::vector<PolylineID> polylines;
    };

    // ----------------------------------------
    // Geometry vertex
    // ----------------------------------------
    struct GeometryVertex2D
    {
        GeometryVertexID id = 0;

        double x = 0.0;
        double y = 0.0;
        bool deleted = false;
    };

    // ----------------------------------------
    // Geometry edge
    // ----------------------------------------
    struct GeometryEdge2D
    {
        GeometryEdgeID id = 0;

        GeometryVertexID vertex0 = 0;
        GeometryVertexID vertex1 = 0;
        bool deleted = false;
    };

    // ----------------------------------------
    // Polyline
    // ----------------------------------------
    struct Polyline2D
    {
        PolylineID id = 0;

        std::vector<GeometryVertexID> vertices;
        std::vector<GeometryEdgeID> edges;
        std::string name;
        std::optional<GeometryThreadID> threadID;
        bool closed = false;
        bool deleted = false; 
    };

    // ----------------------------------------
    // Geometry container
    // ----------------------------------------
    class Geometry2D
    {
    public:

        GeometryVertexID addVertex(
            double x,
            double y);

        GeometryEdgeID addEdge(
            GeometryVertexID v0,
            GeometryVertexID v1);

        PolylineID createPolyline();

        void appendVertexToPolyline(
            PolylineID polylineID,
            GeometryVertexID vertexID);

        void closePolyline(PolylineID polylineID);
        void discardLastPolyline();
        void clear();
        bool deletePolyline(PolylineID id);
        // Edits preserve existing IDs. New split components receive new IDs.
        bool deleteVertex(GeometryVertexID id);
        bool deleteEdge(GeometryEdgeID id);
        bool isVertexActive(GeometryVertexID id) const noexcept;
        bool isEdgeActive(GeometryEdgeID id) const noexcept;
        std::vector<std::string> validate() const;
        bool deleteThread(GeometryThreadID id);

        GeometryThreadID createThread(const std::string& name);

        bool assignPolylineToThread(
            PolylineID polylineID,
            std::optional<GeometryThreadID> threadID);

        bool renamePolyline(
            PolylineID polylineID,
            const std::string& name);

        bool renameThread(
            GeometryThreadID threadID,
            const std::string& name);

        const std::vector<GeometryThread2D>&
        threads() const noexcept;

        // Read-only access
        const std::vector<GeometryVertex2D>& vertices() const noexcept;
        const std::vector<GeometryEdge2D>& edges() const noexcept;
        const std::vector<Polyline2D>& polylines() const noexcept;

        std::size_t vertexCount() const noexcept;
        std::size_t edgeCount() const noexcept;
        std::size_t polylineCount() const noexcept;

    private:

        std::vector<GeometryVertex2D> vertices_;
        std::vector<GeometryEdge2D> edges_;
        std::vector<Polyline2D> polylines_;
        std::vector<GeometryThread2D> threads_;
        GeometryThreadID nextThreadID_ = 0;
        void splitPolylineAtEdge(PolylineID id, std::size_t edgePosition);
    };
}
