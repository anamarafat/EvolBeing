#pragma once

#include <cstdint>
#include <vector>
#include <cstddef>
#include <string>
#include <optional>
#include <unordered_map>
#include <memory>

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

    enum class PolylineStatus : std::uint8_t
    {
        Candidate,
        Open,
        Closed
    };

    struct GeometryThread2D
    {
        GeometryThreadID id = 0;
        std::string name;
        GeometryType type = GeometryType::Unassigned;
        bool active = true;
        std::uint32_t colorRGB = 0xDC2828;
        double thickness = 1.0;
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
        std::optional<std::uint32_t> colorOverrideRGB;
        std::optional<double> thicknessOverride;
        bool closed = false;
        bool deleted = false; 
    };

    // ----------------------------------------
    // Geometry half-edge
    // ----------------------------------------
    struct HalfEdge2D
    {
        const GeometryVertex2D* origin = nullptr;

        const GeometryEdge2D* parentEdge = nullptr;

        HalfEdge2D* twin = nullptr;
        HalfEdge2D* next = nullptr;

        bool visited = false;
    };

    // ----------------------------------------
    // Closed geometry loop
    // ----------------------------------------
    struct GeometryLoop2D
    {
        std::string name;

        std::vector<HalfEdge2D*> halfEdges;

        std::vector<const Polyline2D*> polylines;

        double signedArea = 0.0;

        double xmin = 0.0;
        double ymin = 0.0;

        double xmax = 0.0;
        double ymax = 0.0;
    };

    // ----------------------------------------
    // Geometry loop thread
    // ----------------------------------------
    struct LoopThread2D
    {
        std::string name = "Loops";
        std::vector<std::unique_ptr<GeometryLoop2D>> loops;
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
        // Reconnect the last edge to the first vertex; do not allocate an edge.
        bool closePolylineReusingLastEdge(PolylineID polylineID);
        void discardLastPolyline();
        void clear();
        bool deletePolyline(PolylineID id);
        // Edits preserve existing IDs. New split components receive new IDs.
        bool deleteVertex(GeometryVertexID id);
        bool moveVertex(GeometryVertexID id, double x, double y);
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

        bool setThreadProperties(GeometryThreadID id, GeometryType type, bool active, std::uint32_t colorRGB);
        bool setThreadThickness(GeometryThreadID id, double thickness);
        bool setPolylineThicknessOverride(PolylineID id, std::optional<double> thickness);
        double effectivePolylineThickness(PolylineID id) const noexcept;
        bool importState(std::vector<GeometryVertex2D> vertices,
                         std::vector<GeometryEdge2D> edges,
                         std::vector<Polyline2D> polylines,
                         std::vector<GeometryThread2D> threads);
        bool setPolylineColorOverride(PolylineID id, std::optional<std::uint32_t> colorRGB);
        std::uint32_t effectivePolylineColor(PolylineID id) const noexcept;

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

    // ----------------------------------------
    // Finalized geometry topology
    // ----------------------------------------
    class FinalizedGeometry2D
    {
        public:

            FinalizedGeometry2D() = default;
            FinalizedGeometry2D(
                const FinalizedGeometry2D&) = delete;

            FinalizedGeometry2D& operator=(
                const FinalizedGeometry2D&) = delete;

            // Main geometry finalization function
            bool finalize(const Geometry2D& geometry);

            // Clear finalized topology
            void clear() noexcept;

            // Read-only access to generated loops
            const LoopThread2D& loopThread() const noexcept;
            std::size_t halfEdgeCount() const noexcept;
            std::size_t loopCount() const noexcept;
            PolylineStatus polylineStatus(const Polyline2D& polyline) const;

            // Finalization diagnostics
            const std::vector<std::string>& errors() const noexcept;

            

        private:

            // Half-edge ownership
            std::vector<std::unique_ptr<HalfEdge2D>> halfEdges_;

            // Outgoing half-edges associated with each vertex
            std::unordered_map<
                const GeometryVertex2D*,
                std::vector<HalfEdge2D*>> 
                outgoingHalfEdges_;

            // Original edge to contributing polylines
            std::unordered_map<
                const GeometryEdge2D*,
                std::vector<const Polyline2D*>> 
                edgePolylines_;

            std::unordered_map<const Polyline2D*,PolylineStatus> polylineStatus_;
            std::unordered_map<const GeometryVertex2D*,std::size_t> eligibleEdgeCount_;
            std::unordered_map<const GeometryVertex2D*,std::vector<const Polyline2D*>>endpointPolylines_;

            // Ownership of identified loops
            LoopThread2D loopThread_;

            // Geometry validation errors
            std::vector<std::string> errors_;

            // Geometry processing functions
            bool validateGeometry(const Geometry2D& geometry);
            void buildHalfEdges(const Geometry2D& geometry);
            void buildEdgePolylineConnectivity(
                const Geometry2D& geometry);
            void sortOutgoingHalfEdges();
            void connectHalfEdges();
            void extractLoops();
            void populateLoopPolylines(GeometryLoop2D& loop);
            void computeLoopProperties(GeometryLoop2D& loop);

            void initializePolylineClassification(const Geometry2D& geometry);
            void eliminateOpenPolylines(const Geometry2D& geometry);
            bool hasOpenEndpoint(const Polyline2D& polyline,
                                 const Geometry2D& geometry) const;
            bool isEdgeEligible(const GeometryEdge2D* edge) const;
    };
    
}
