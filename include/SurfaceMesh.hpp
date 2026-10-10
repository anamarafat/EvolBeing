#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "Vec3.hpp"

namespace geom 
{

    /* Entity identifier type (alias) */
    using VertexID = std::uint32_t;
    using FaceID = std::uint32_t;
    using EdgeID = std::uint32_t;

    struct Triangle 
    {
        std::array<VertexID, 3> vertices;
    };

    class SurfaceMesh 
    {
        public:

            // Add a vertex and return its index
            VertexID addVertex(const Vec3 &position);

            // Add a triangular face
            void addTriangle(
                VertexID v0,
                VertexID v1,
                VertexID v2
            );

            // Read-only access to all vertices
            [[nodiscard]]
            const std::vector<Vec3> &vertices() const noexcept {
                return vertices_;
            }

            // Read-only access to all faces
            [[nodiscard]]
            const std::vector<Triangle>& faces() const noexcept {
                return faces_;
            }

            // Number of vertices
            [[nodiscard]]
            std::size_t vertexCount() const noexcept {
                return vertices_.size();
            }

            // Number of triangles
            [[nodiscard]]
            std::size_t faceCount() const noexcept {
                return faces_.size();
            }

        private:

            std::vector<Vec3> vertices_;
            std::vector<Triangle> faces_;
    };

}