#pragma once

#include <cstdint>
#include <vector>
#include <cstddef>
#include <string>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <iterator>
#include <type_traits>
#include <utility>

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

    struct GeometryThreadRecord
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
    // Geometry edge
    // ----------------------------------------
    struct GeometryEdgeRecord
    {
        GeometryEdgeID id = 0;

        GeometryVertexID vertex0 = 0;
        GeometryVertexID vertex1 = 0;
        bool deleted = false;
    };

    // ----------------------------------------
    // Polyline
    // ----------------------------------------
    struct PolylineRecord
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

    struct Polyline2D;
    struct GeometryThread2D
    {
        GeometryThreadID id = 0;
        std::string name;
        GeometryType type = GeometryType::Unassigned;
        bool active = true;
        std::uint32_t colorRGB = 0xDC2828;
        double thickness = 1.0;
        std::vector<Polyline2D*> polylines;
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

        GeometryVertex2D* vertex0 = nullptr;
        GeometryVertex2D* vertex1 = nullptr;
        bool deleted = false;
    };

    // ----------------------------------------
    // Polyline
    // ----------------------------------------
    struct Polyline2D
    {
        PolylineID id = 0;

        std::vector<GeometryVertex2D*> vertices;
        std::vector<GeometryEdge2D*> edges;
        std::string name;
        GeometryThread2D* thread = nullptr;
        std::optional<std::uint32_t> colorOverrideRGB;
        std::optional<double> thicknessOverride;
        bool closed = false;
        bool deleted = false; 
    };

    template<class T> class EntityStorage
    {
        std::vector<std::unique_ptr<T>> data_;
        template<bool Const> class Iterator
        {
            using Base = std::conditional_t<Const, typename std::vector<std::unique_ptr<T>>::const_iterator, typename std::vector<std::unique_ptr<T>>::iterator>;
            Base it_;
            friend class EntityStorage;
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using reference = std::conditional_t<Const, const T&, T&>;
            using pointer = std::conditional_t<Const, const T*, T*>;
            explicit Iterator(Base it) : it_(it) {}
            Iterator() = default;
            reference operator*() const { return **it_; }
            pointer operator->() const { return it_->get(); }
            Iterator& operator++() { ++it_; return *this; }
            Iterator operator++(int) { auto old=*this; ++*this; return old; }
            bool operator==(const Iterator&) const = default;
        };
    public:
        auto begin() { return Iterator<false>(data_.begin()); }
        auto end() { return Iterator<false>(data_.end()); }
        auto begin() const { return Iterator<true>(data_.begin()); }
        auto end() const { return Iterator<true>(data_.end()); }
        T& operator[](std::size_t i) { return *data_[i]; }
        const T& operator[](std::size_t i) const { return *data_[i]; }
        T& at(std::size_t i) { return *data_.at(i); }
        const T& at(std::size_t i) const { return *data_.at(i); }
        T& back() { return *data_.back(); }
        std::size_t size() const noexcept { return data_.size(); }
        bool empty() const noexcept { return data_.empty(); }
        void clear() noexcept { data_.clear(); }
        void push_back(T value) { data_.push_back(std::make_unique<T>(std::move(value))); }
        void erase(Iterator<false> it) { data_.erase(it.it_); }
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

        Geometry2D() = default;
        Geometry2D(const Geometry2D&);
        Geometry2D& operator=(const Geometry2D&);
        Geometry2D(Geometry2D&&) noexcept = default;
        Geometry2D& operator=(Geometry2D&&) noexcept = default;
        std::vector<GeometryVertex2D> vertexRecords() const;
        std::vector<GeometryEdgeRecord> edgeRecords() const;
        std::vector<PolylineRecord> polylineRecords() const;
        std::vector<GeometryThreadRecord> threadRecords() const;
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
                         std::vector<GeometryEdgeRecord> edges,
                         std::vector<PolylineRecord> polylines,
                         std::vector<GeometryThreadRecord> threads);
        bool setPolylineColorOverride(PolylineID id, std::optional<std::uint32_t> colorRGB);
        std::uint32_t effectivePolylineColor(PolylineID id) const noexcept;

        bool renameThread(
            GeometryThreadID threadID,
            const std::string& name);

        const EntityStorage<GeometryThread2D>&
        threads() const noexcept;

        // Read-only access
        const EntityStorage<GeometryVertex2D>& vertices() const noexcept;
        const EntityStorage<GeometryEdge2D>& edges() const noexcept;
        const EntityStorage<Polyline2D>& polylines() const noexcept;

        std::size_t vertexCount() const noexcept;
        std::size_t edgeCount() const noexcept;
        std::size_t polylineCount() const noexcept;

    private:

        EntityStorage<GeometryVertex2D> vertices_;
        EntityStorage<GeometryEdge2D> edges_;
        EntityStorage<Polyline2D> polylines_;
        EntityStorage<GeometryThread2D> threads_;
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
            
            struct LoopCandidate
            {
                const GeometryVertex2D* startVertex = nullptr;
                const GeometryVertex2D* currentVertex = nullptr;
                std::vector<HalfEdge2D*> halfEdges;
                std::vector<const Polyline2D*> polylines;
                std::unordered_set<const GeometryVertex2D*>
                    visitedVertices;
            };

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
            std::unordered_map<const GeometryEdge2D*, HalfEdge2D*> edgeHalfEdges_;

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

            void initializePolylineClassification(const Geometry2D& geometry);
            void eliminateOpenPolylines(const Geometry2D& geometry);
            bool hasOpenEndpoint(const Polyline2D& polyline) const;
            bool isEdgeEligible(const GeometryEdge2D* edge) const;
            HalfEdge2D* findDirectedHalfEdge(const GeometryEdge2D* edge,
                                             const GeometryVertex2D* origin) const;
            bool buildPolylinePath(const Polyline2D& polyline,const GeometryVertex2D* startVertex,
                                   std::vector<HalfEdge2D*>& path) const;
            bool registerLoop(const std::vector<HalfEdge2D*>& path,
                const std::vector<const Polyline2D*>& polylines);
            void registerClosedPolylines(const Geometry2D& geometry);
            bool extendLoopCandidate(LoopCandidate& candidate,
                    const Polyline2D& polyline) const;
            bool searchLoopFromPolyline(const Polyline2D& seed);
            void discoverMultiPolylineLoops(const Geometry2D& geometry);
            double computeSignedArea(const GeometryLoop2D& loop) const;
            void reverseLoop(GeometryLoop2D& loop);
            void computeLoopProperties(GeometryLoop2D& loop);
            bool finalizeLoopProperties();

    };
    
}
