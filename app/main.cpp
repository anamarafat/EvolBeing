#include <iostream>
#include <numbers>
#include "SurfaceMesh.hpp"

int main()
{
    std::cout << "EvolBeing_Gen_Alpha \n";
    std::cout << "Pi = " << std::numbers::pi << '\n';

    /* Temporary Test */
    geom::SurfaceMesh mesh;

    // Add four vertices
    auto v0 = mesh.addVertex({0.0, 0.0, 0.0});
    auto v1 = mesh.addVertex({1.0, 0.0, 0.0});
    auto v2 = mesh.addVertex({1.0, 1.0, 0.0});
    auto v3 = mesh.addVertex({0.0, 1.0, 0.0});

    // Create two triangles
    mesh.addTriangle(v0, v1, v2);
    mesh.addTriangle(v0, v2, v3);

    std::cout << "EvolBeing_Gen_Alpha\n";
    std::cout << "Vertices: " << mesh.vertexCount() << '\n';
    std::cout << "Triangles: " << mesh.faceCount() << '\n';

    // Print all vertices
    for (const auto& vertex : mesh.vertices()) 
    {
        std::cout << vertex.x << " "
                  << vertex.y << " "
                  << vertex.z << '\n';
    }
    /* End Temporary Test */

    return 0;
}