#include "SurfaceMesh.hpp"

#include <limits>
#include <stdexcept>

namespace geom
{ 

    VertexID SurfaceMesh::addVertex(const Vec3 &position) 
    {
        // Check that the vertex index fits in VertexID
        if (vertices_.size() >
            std::numeric_limits<VertexID>::max()) 
        {
            throw std::overflow_error("Vertex ID limit exceeded");
        }

        // New vertex index = current number of vertices
        VertexID id = static_cast<VertexID>(vertices_.size());

        // Append the vertex coordinates
        vertices_.push_back(position);

        // Return the new vertex index
        return id;
    }

    void SurfaceMesh::addTriangle(
        VertexID v0,
        VertexID v1,
        VertexID v2
    ) {
        // Get the number of vertices
        const auto n = vertices_.size();

        // Validate that all vertex indices exist
        if (v0 >= n || v1 >= n || v2 >= n) {
            throw std::out_of_range("Invalid triangle vertex index");
        }

        // Append a new triangle
        faces_.push_back({{v0, v1, v2}});
    }

} // namespace geom