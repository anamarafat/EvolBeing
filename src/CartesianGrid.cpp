
#include "CartesianGrid.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace geom
{
    void CartesianGrid::generate(
        std::uint32_t nx,
        std::uint32_t ny,
        double cellSize)
    {
        // ----------------------------------------
        // 1. Validate input
        // ----------------------------------------

        if (nx == 0 || ny == 0)
            throw std::invalid_argument(
                "Grid dimensions must be positive.");

        if (!std::isfinite(cellSize) || cellSize <= 0.0)
            throw std::invalid_argument(
                "Cell size must be finite and positive.");

        const std::uint64_t nCells =
            static_cast<std::uint64_t>(nx) * ny;

        const std::uint64_t nVerticalFaces =
            (static_cast<std::uint64_t>(nx) + 1) * ny;

        const std::uint64_t nHorizontalFaces =
            static_cast<std::uint64_t>(nx) * (ny + 1ULL);

        const std::uint64_t nFaces =
            nVerticalFaces + nHorizontalFaces;

        // IDs start at zero, so the number of entities
        // must not exceed the 32-bit addressable count.
        const std::uint64_t maxEntities =
            static_cast<std::uint64_t>(
                std::numeric_limits<std::uint32_t>::max()) + 1ULL;

        if (nCells > maxEntities || nFaces > maxEntities)
            throw std::overflow_error(
                "Grid exceeds 32-bit entity ID capacity.");

        // ----------------------------------------
        // 2. Reset existing mesh
        // ----------------------------------------

        // Thread lists contain non-owning pointers.
        // Destroy them before the mesh entities.
        cellThreads_.clear();
        faceThreads_.clear();

        cells_.clear();
        faces_.clear();

        nx_ = nx;
        ny_ = ny;
        cellSize_ = cellSize;

        cells_.reserve(static_cast<std::size_t>(nCells));
        faces_.reserve(static_cast<std::size_t>(nFaces));

        // ----------------------------------------
        // 3. Create Cartesian mesh threads
        // ----------------------------------------

        auto cellThread = std::make_unique<CellThread>();
        cellThread->id = 0;
        cellThread->type = RegionType::CartesianMesh;
        cellThread->cells.reserve(
            static_cast<std::size_t>(nCells));

        CellThread* ct = cellThread.get();
        cellThreads_.push_back(std::move(cellThread));

        auto faceThread = std::make_unique<FaceThread>();
        faceThread->id = 0;
        faceThread->type = RegionType::CartesianMesh;
        faceThread->faces.reserve(
            static_cast<std::size_t>(nFaces));

        FaceThread* ft = faceThread.get();
        faceThreads_.push_back(std::move(faceThread));

        // ----------------------------------------
        // 4. Create cells
        // ----------------------------------------

        for (std::uint32_t j = 0; j < ny_; ++j)
        {
            for (std::uint32_t i = 0; i < nx_; ++i)
            {
                auto cell = std::make_unique<Cell>();

                cell->id = static_cast<CellID>(
                    static_cast<std::uint64_t>(j) * nx_ + i);

                cell->thread = ct;
                cell->parent = nullptr;
                cell->numChildren = 0;
                cell->flags = 0;

                cell->faces.reserve(4);

                Cell* ptr = cell.get();

                cells_.push_back(std::move(cell));
                ct->cells.push_back(ptr);
            }
        }

        // Helper: retrieve a cell using Cartesian indices.
        // This is only used during initial construction.
        auto getCell = [this](
            std::uint32_t i,
            std::uint32_t j) -> Cell*
        {
            const std::size_t index =
                static_cast<std::size_t>(j) * nx_ + i;

            return cells_[index].get();
        };

        // ----------------------------------------
        // 5. Generate vertical faces
        // ----------------------------------------

        for (std::uint32_t j = 0; j < ny_; ++j)
        {
            for (std::uint32_t i = 0; i <= nx_; ++i)
            {
                auto face = std::make_unique<Face>();

                face->id = static_cast<FaceID>(faces_.size());
                face->thread = ft;

                face->parent = nullptr;
                face->numChildren = 0;
                face->flags = 0;

                // Left and right adjacent cells.
                // Boundary faces have one null pointer.
                Cell* left =
                    (i > 0) ? getCell(i - 1, j) : nullptr;

                Cell* right =
                    (i < nx_) ? getCell(i, j) : nullptr;

                face->owner = left ? left : right;
                face->neighbor = left ? right : nullptr;

                Face* ptr = face.get();

                // Connect face to adjacent cells
                if (left)
                    left->faces.push_back(ptr);

                if (right)
                    right->faces.push_back(ptr);

                faces_.push_back(std::move(face));
                ft->faces.push_back(ptr);
            }
        }

        // ----------------------------------------
        // 6. Generate horizontal faces
        // ----------------------------------------

        for (std::uint32_t j = 0; j <= ny_; ++j)
        {
            for (std::uint32_t i = 0; i < nx_; ++i)
            {
                auto face = std::make_unique<Face>();

                face->id = static_cast<FaceID>(faces_.size());
                face->thread = ft;

                face->parent = nullptr;
                face->numChildren = 0;
                face->flags = 0;

                // Upper and lower adjacent cells
                // using screen-style Y-down indexing.
                Cell* upper =
                    (j > 0) ? getCell(i, j - 1) : nullptr;

                Cell* lower =
                    (j < ny_) ? getCell(i, j) : nullptr;

                face->owner = upper ? upper : lower;
                face->neighbor = upper ? lower : nullptr;

                Face* ptr = face.get();

                if (upper)
                    upper->faces.push_back(ptr);

                if (lower)
                    lower->faces.push_back(ptr);

                faces_.push_back(std::move(face));
                ft->faces.push_back(ptr);
            }
        }
    }


    Cell* CartesianGrid::cellAt(
        std::uint32_t i,
        std::uint32_t j) const
    {
        if (i >= nx_ || j >= ny_)
            return nullptr;

        std::size_t index =
            static_cast<std::size_t>(j) * nx_ + i;

        return cells_[index].get();
    }


    Face* CartesianGrid::verticalFaceAt(
        std::uint32_t i,
        std::uint32_t j) const
    {
        if (i > nx_ || j >= ny_)
            return nullptr;

        std::size_t index =
            static_cast<std::size_t>(j) * (nx_ + 1) + i;

        return faces_[index].get();
    }


    Face* CartesianGrid::horizontalFaceAt(
        std::uint32_t i,
        std::uint32_t j) const
    {
        if (i >= nx_ || j > ny_)
            return nullptr;

        std::size_t verticalFaceCount =
            static_cast<std::size_t>(nx_ + 1) * ny_;

        std::size_t index =
            verticalFaceCount
            + static_cast<std::size_t>(j) * nx_
            + i;

        return faces_[index].get();
    }

}
