#include <iostream>
#include <numbers>

#include "SurfaceMesh.hpp"
#include "ObjIO.hpp"

int main()
{
    std::cout << "EvolBeing_Gen_Alpha \n";
    std::cout << "Pi = " << std::numbers::pi << '\n';
    

    /* Temporary Test of reading .obj file */
    try
    {
        std::filesystem::path meshPath = "geom/obj/test.obj";
        std::cout << "Mesh file: " << std::filesystem::absolute(meshPath) << '\n';
        geom::SurfaceMesh mesh = geom::loadOBJ(meshPath);

        std::cout << "Vertices: " << mesh.vertexCount() << '\n';
        std::cout << "Triangles: " << mesh.faceCount() << '\n';
    }
    catch (const std::exception& error)
    {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }


    return 0;
}