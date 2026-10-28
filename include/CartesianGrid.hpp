#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace geom
{
    using CellID = std::uint32_t;
    using FaceID = std::uint32_t;

    struct CellThread;
    struct FaceThread;
    struct Face;

    struct Cell
    {
        CellID id = 0;

        // Hierarchy
        CellThread* thread = nullptr;
        Cell* parent = nullptr;
        std::array<Cell*, 4> children{};
        std::uint8_t numChildren = 0;

        // Connectivity
        std::vector<Face*> faces;

        // Bitwise flags
        std::uint32_t flags = 0;
    };

    struct Face
    {
        FaceID id = 0;

        // Adjacent cells
        Cell* owner = nullptr;
        Cell* neighbor = nullptr;

        // Hierarchy
        FaceThread* thread = nullptr;
        Face* parent = nullptr;
        std::array<Face*, 2> children{};
        std::uint8_t numChildren = 0;

        // Bitwise flags
        std::uint32_t flags = 0;
    };

    /* Thread Business */
    enum class RegionType : std::uint8_t
    {
        CartesianMesh,
        RefinedRegion,
        SpecialRegion
    };

    struct CellThread
    {
        std::uint32_t id = 0;
        RegionType type = RegionType::CartesianMesh;

        std::vector<Cell*> cells;
        std::uint32_t flags = 0;
    };

    struct FaceThread
    {
        std::uint32_t id = 0;
        RegionType type = RegionType::CartesianMesh;

        std::vector<Face*> faces;
        std::uint32_t flags = 0;
    };

    class CartesianGrid
    {
    public:
        void generate(
            std::uint32_t nx,
            std::uint32_t ny,
            double cellSize
        );

        // Mesh statistics
        std::size_t cellCount() const { return cells_.size(); }
        std::size_t faceCount() const { return faces_.size(); }
        std::size_t cellThreadCount() const { return cellThreads_.size(); }
        std::size_t faceThreadCount() const { return faceThreads_.size(); }

    private:
        std::vector<std::unique_ptr<Cell>> cells_;
        std::vector<std::unique_ptr<Face>> faces_;

        // Independent region collections
        std::vector<std::unique_ptr<CellThread>> cellThreads_;
        std::vector<std::unique_ptr<FaceThread>> faceThreads_;

        std::uint32_t nx_ = 0;
        std::uint32_t ny_ = 0;
        double cellSize_ = 1.0;
    };
}